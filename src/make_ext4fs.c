/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ext4_utils.h"
#include "allocate.h"
#include "contents.h"
#include "uuid5.h"
#include "wipe.h"

#include <sparse/sparse.h>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <locale.h>

/* TODO: Not implemented:
   Allocating blocks in the same block group as the file inode
   Hash or binary tree directories
 */

static int filter_dot(const struct dirent *d)
{
	return (strcmp(d->d_name, "..") && strcmp(d->d_name, "."));
}

static u32 build_default_directory_structure(struct fs_info *info,
					     struct fs_aux_info *aux_info,
					     struct sparse_file
					     *ext4_sparse_file, int force,
					     jmp_buf *setjmp_env,
					     time_t fixed_time)
{
	u32 inode;
	u32 root_inode;
	struct dentry dentries = {
		.filename = "lost+found",
		.file_type = EXT4_FT_DIR,
		.mode = S_IRWXU,
		.uid = 0,
		.gid = 0,
		.mtime = (fixed_time != -1) ? fixed_time : 0,
	};
	root_inode = make_directory(info, aux_info, ext4_sparse_file, force,
				    setjmp_env, 0, 1, &dentries, 1);
	inode = make_directory(info, aux_info, ext4_sparse_file, force,
			       setjmp_env, root_inode, 0, NULL, 0);
	*dentries.inode = inode;
	inode_set_permissions(info, aux_info, ext4_sparse_file, setjmp_env,
			      inode, dentries.mode, dentries.uid, dentries.gid,
			      dentries.mtime);

	return root_inode;
}

/* Read a local directory and create the same tree in the generated filesystem.
   Calls itself recursively with each directory in the given directory.
   full_path is an absolute or relative path, with a trailing slash, to the
   directory on disk that should be copied, or NULL if this is a directory
   that does not exist on disk (e.g. lost+found).
   dir_path is an absolute path, with trailing slash, to the same directory
   if the image were mounted at the specified mount point */
static u32 build_directory_structure(struct fs_info *info,
				     struct fs_aux_info *aux_info,
				     struct sparse_file *ext4_sparse_file,
				     struct block_allocation
				     *saved_allocation_head,
				     struct fs_config_list *config_list,
				     int force, jmp_buf *setjmp_env,
				     const char *full_path,
				     const char *dir_path,
				     u32 dir_inode,
				     fs_config_func_t fs_config_func,
				     int verbose, time_t fixed_time)
{
	int entries = 0;
	struct dentry *dentries;
	struct dirent **namelist = NULL;
	struct stat stat;
	int ret;
	int i;
	u32 inode;
	u32 entry_inode;
	u32 dirs = 0;
	bool needs_lost_and_found = false;

	/* alphasort is locale-dependent; let's fix the locale to some sane value */
	setlocale(LC_COLLATE, "C");

	if (full_path) {
		entries =
		    scandir(full_path, &namelist, filter_dot,
			    (void *)alphasort);
		if (entries < 0) {
#ifdef __GLIBC__
			/* The scandir function implemented in glibc has a bug that makes it
			   erroneously fail with ENOMEM under certain circumstances.
			   As a workaround we can retry the scandir call with the same arguments.
			   GLIBC BZ: https://sourceware.org/bugzilla/show_bug.cgi?id=17804 */
			if (errno == ENOMEM)
				entries =
				    scandir(full_path, &namelist, filter_dot,
					    (void *)alphasort);
#endif
			if (entries < 0) {
				error_errno(force, setjmp_env, "scandir");
				return EXT4_ALLOCATE_FAILED;
			}
		}
	}

	if (dir_inode == 0) {
		/* root directory, check if lost+found already exists */
		for (i = 0; i < entries; i++)
			if (strcmp(namelist[i]->d_name, "lost+found") == 0)
				break;
		if (i == entries)
			needs_lost_and_found = true;
	}

	dentries = calloc(entries, sizeof(struct dentry));
	if (dentries == NULL)
		critical_error_errno(setjmp_env, "calloc(%zu, %zu)",
				     (size_t)entries, sizeof(struct dentry));

	for (i = 0; i < entries; i++) {
		dentries[i].filename = strdup(namelist[i]->d_name);
		if (dentries[i].filename == NULL)
			critical_error_errno(setjmp_env, "strdup");

		asprintf(&dentries[i].path, "%s%s", dir_path,
			 namelist[i]->d_name);
		asprintf(&dentries[i].full_path, "%s%s", full_path,
			 namelist[i]->d_name);

		free(namelist[i]);

		ret = lstat(dentries[i].full_path, &stat);
		if (ret < 0) {
			error_errno(force, setjmp_env, "lstat");
			i--;
			entries--;
			continue;
		}

		dentries[i].size = stat.st_size;
		dentries[i].mode =
		    stat.st_mode & (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU |
				    S_IRWXG | S_IRWXO);
		if (fixed_time == -1) {
			dentries[i].mtime = stat.st_mtime;
		} else {
			dentries[i].mtime = fixed_time;
		}
		uint64_t capabilities;
		if (fs_config_func != NULL) {
			unsigned int mode = 0;
			unsigned int uid = 0;
			unsigned int gid = 0;
			int dir = S_ISDIR(stat.st_mode);
			if (fs_config_func(config_list, dentries[i].path, dir,
					   &uid, &gid, &mode, &capabilities)) {
				dentries[i].mode = mode;
				dentries[i].uid = uid;
				dentries[i].gid = gid;
				dentries[i].capabilities = capabilities;
			}
		}

		if (S_ISREG(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_REG_FILE;
		} else if (S_ISDIR(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_DIR;
			dirs++;
		} else if (S_ISCHR(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_CHRDEV;
		} else if (S_ISBLK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_BLKDEV;
		} else if (S_ISFIFO(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_FIFO;
		} else if (S_ISSOCK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_SOCK;
		} else if (S_ISLNK(stat.st_mode)) {
			dentries[i].file_type = EXT4_FT_SYMLINK;
			dentries[i].link = calloc(info->block_size, 1);
			if (!dentries[i].link) {
				critical_error(setjmp_env, "calloc(%zu, 1)",
					       (size_t)info->block_size);
			}
			readlink(dentries[i].full_path, dentries[i].link,
				 info->block_size - 1);
		} else {
			error(force, setjmp_env, "unknown file type on %s",
			      dentries[i].path);
			i--;
			entries--;
		}
	}
	free(namelist);

	if (needs_lost_and_found) {
		/* insert a lost+found directory at the beginning of the dentries */
		struct dentry *tmp = calloc(entries + 1, sizeof(struct dentry));
		if (!tmp) {
			critical_error(setjmp_env, "calloc(%zu, %zu)",
				       (size_t)entries + 1,
				       sizeof(struct dentry));
		}
		memset(tmp, 0, sizeof(struct dentry));
		memcpy(tmp + 1, dentries, entries * sizeof(struct dentry));
		dentries = tmp;

		dentries[0].filename = strdup("lost+found");
		asprintf(&dentries[0].path, "%slost+found", dir_path);
		dentries[0].full_path = NULL;
		dentries[0].size = 0;
		dentries[0].mode = S_IRWXU;
		dentries[0].file_type = EXT4_FT_DIR;
		dentries[0].uid = 0;
		dentries[0].gid = 0;
		entries++;
		dirs++;
	}

	inode = make_directory(info, aux_info, ext4_sparse_file, force,
			       setjmp_env, dir_inode, entries, dentries, dirs);

	for (i = 0; i < entries; i++) {
		if (dentries[i].file_type == EXT4_FT_REG_FILE) {
			entry_inode = make_file(info, aux_info,
						ext4_sparse_file,
						saved_allocation_head,
						force, setjmp_env,
						dentries[i].full_path,
						dentries[i].size);
		} else if (dentries[i].file_type == EXT4_FT_DIR) {
			char *subdir_full_path = NULL;
			char *subdir_dir_path;
			if (dentries[i].full_path) {
				ret =
				    asprintf(&subdir_full_path, "%s/",
					     dentries[i].full_path);
				if (ret < 0)
					critical_error_errno(setjmp_env,
							     "asprintf");
			}
			ret =
			    asprintf(&subdir_dir_path, "%s/", dentries[i].path);
			if (ret < 0)
				critical_error_errno(setjmp_env, "asprintf");
			entry_inode = build_directory_structure(info, aux_info,
								ext4_sparse_file,
								saved_allocation_head,
								config_list,
								force,
								setjmp_env,
								subdir_full_path,
								subdir_dir_path,
								inode,
								fs_config_func,
								verbose,
								fixed_time);
			free(subdir_full_path);
			free(subdir_dir_path);
		} else if (dentries[i].file_type == EXT4_FT_SYMLINK) {
			entry_inode = make_link(info, aux_info,
						ext4_sparse_file, force,
						setjmp_env, dentries[i].link);
		} else if (dentries[i].file_type == EXT4_FT_CHRDEV ||
			   dentries[i].file_type == EXT4_FT_BLKDEV ||
			   dentries[i].file_type == EXT4_FT_SOCK ||
			   dentries[i].file_type == EXT4_FT_FIFO) {
			entry_inode = make_special(info, aux_info,
						   ext4_sparse_file, force,
						   setjmp_env,
						   dentries[i].full_path);
		} else {
			error(force, setjmp_env, "unknown file type on %s",
			      dentries[i].path);
			entry_inode = 0;
		}
		*dentries[i].inode = entry_inode;

		ret = inode_set_permissions(info, aux_info, ext4_sparse_file,
					    setjmp_env, entry_inode,
					    dentries[i].mode, dentries[i].uid,
					    dentries[i].gid, dentries[i].mtime);
		if (ret)
			error(force, setjmp_env,
			      "failed to set permissions on %s",
			      dentries[i].path);

		ret = inode_set_capabilities(info, aux_info, ext4_sparse_file,
					     force, setjmp_env, entry_inode,
					     dentries[i].capabilities);
		if (ret)
			error(force, setjmp_env,
			      "failed to set capability on %s",
			      dentries[i].path);

		free(dentries[i].path);
		free(dentries[i].full_path);
		free(dentries[i].link);
		free((void *)dentries[i].filename);
	}

	free(dentries);
	return inode;
}

static u32 compute_block_size(void)
{
	return 4096;
}

static u32 compute_journal_blocks(struct fs_info *info)
{
	u32 journal_blocks = DIV_ROUND_UP(info->len, info->block_size) / 64;
	if (journal_blocks < 1024)
		journal_blocks = 1024;
	if (journal_blocks > 32768)
		journal_blocks = 32768;
	return journal_blocks;
}

static u32 compute_blocks_per_group(struct fs_info *info)
{
	return info->block_size * 8;
}

static u32 compute_inodes(struct fs_info *info)
{
	return DIV_ROUND_UP(info->len, info->block_size) / 4;
}

static u32 compute_inodes_per_group(struct fs_info *info)
{
	u32 blocks = DIV_ROUND_UP(info->len, info->block_size);
	u32 block_groups = DIV_ROUND_UP(blocks, info->blocks_per_group);
	u32 inodes = DIV_ROUND_UP(info->inodes, block_groups);
	inodes = EXT4_ALIGN(inodes, (info->block_size / info->inode_size));

	/* After properly rounding up the number of inodes/group,
	 * make sure to update the total inodes field in the info struct.
	 */
	info->inodes = inodes * block_groups;

	return inodes;
}

static u32 compute_bg_desc_reserve_blocks(struct fs_info *info)
{
	u32 blocks = DIV_ROUND_UP(info->len, info->block_size);
	u32 block_groups = DIV_ROUND_UP(blocks, info->blocks_per_group);
	u32 bg_desc_blocks =
	    DIV_ROUND_UP(block_groups * sizeof(struct ext2_group_desc),
			 info->block_size);

	u32 bg_desc_reserve_blocks =
	    DIV_ROUND_UP(block_groups * 1024 * sizeof(struct ext2_group_desc),
			 info->block_size) - bg_desc_blocks;

	if (bg_desc_reserve_blocks > info->block_size / sizeof(u32))
		bg_desc_reserve_blocks = info->block_size / sizeof(u32);

	return bg_desc_reserve_blocks;
}

/* return a newly-malloc'd string that is a copy of str.  The new string
   is guaranteed to have a trailing slash.  If absolute is true, the new string
   is also guaranteed to have a leading slash.
*/
static char *canonicalize_slashes(jmp_buf *setjmp_env, const char *str,
				  bool absolute)
{
	char *ret;
	int len = strlen(str);
	int newlen = len;
	char *ptr;

	if (len == 0) {
		if (absolute)
			return strdup("/");
		else
			return strdup("");
	}

	if (str[0] != '/' && absolute) {
		newlen++;
	}
	if (str[len - 1] != '/') {
		newlen++;
	}
	ret = malloc(newlen + 1);
	if (!ret) {
		critical_error(setjmp_env, "malloc(%zu)", (size_t)newlen + 1);
	}

	ptr = ret;
	if (str[0] != '/' && absolute) {
		*ptr++ = '/';
	}

	strcpy(ptr, str);
	ptr += len;

	if (str[len - 1] != '/') {
		*ptr++ = '/';
	}

	if (ptr != ret + newlen) {
		critical_error(setjmp_env, "assertion failed\n");
	}

	*ptr = '\0';

	return ret;
}

static char *canonicalize_rel_slashes(jmp_buf *setjmp_env, const char *str)
{
	return canonicalize_slashes(setjmp_env, str, false);
}

int make_ext4fs_internal(struct fs_info *info, struct fs_aux_info *aux_info,
			 struct sparse_file *ext4_sparse_file,
			 struct block_allocation *saved_allocation_head,
			 struct fs_config_list *config_list,
			 int force, jmp_buf *setjmp_env,
			 int uuid_user_specified, int fd,
			 const char *_directory,
			 fs_config_func_t fs_config_func, int gzip, int sparse,
			 int crc, int wipe, int verbose, time_t fixed_time,
			 FILE *block_list_file)
{
	u32 root_inode_num;
	u16 root_mode;
	char *directory = NULL;
	char buf[40];

	if (setjmp(*setjmp_env))
		return EXIT_FAILURE;	/* Handle a call to longjmp() */

	if (_directory)
		directory = canonicalize_rel_slashes(setjmp_env, _directory);

	if (info->len <= 0)
		info->len = get_file_size(info, fd);

	if (info->len <= 0) {
		fprintf(stderr, "Need size of filesystem\n");
		return EXIT_FAILURE;
	}

	ftruncate(fd, 0);

	if (info->block_size <= 0)
		info->block_size = compute_block_size();

	/* Round down the filesystem length to be a multiple of the block size */
	info->len &= ~((u64)info->block_size - 1);

	if (info->journal_blocks == 0)
		info->journal_blocks = compute_journal_blocks(info);

	if (info->no_journal == 0)
		info->feat_compat = EXT4_FEATURE_COMPAT_HAS_JOURNAL;
	else
		info->journal_blocks = 0;

	if (info->blocks_per_group <= 0)
		info->blocks_per_group = compute_blocks_per_group(info);

	if (info->inodes <= 0)
		info->inodes = compute_inodes(info);

	if (info->inode_size <= 0)
		info->inode_size = 256;

	if (info->label == NULL)
		info->label = "";

	info->inodes_per_group = compute_inodes_per_group(info);

	info->feat_compat |=
	    EXT4_FEATURE_COMPAT_RESIZE_INODE | EXT4_FEATURE_COMPAT_EXT_ATTR;

	info->feat_ro_compat |=
	    EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER |
	    EXT4_FEATURE_RO_COMPAT_LARGE_FILE | EXT4_FEATURE_RO_COMPAT_GDT_CSUM;

	info->feat_incompat |=
	    EXT4_FEATURE_INCOMPAT_EXTENTS | EXT4_FEATURE_INCOMPAT_FILETYPE;

	info->bg_desc_reserve_blocks = compute_bg_desc_reserve_blocks(info);

	if (!uuid_user_specified) {
		uuid5_generate(info->uuid, "extandroid/make_ext4fs",
			       info->label);
	}

	printf("Creating filesystem with parameters:\n");
	printf("    Size: %" PRIu64 "\n", info->len);
	printf("    Block size: %d\n", info->block_size);
	printf("    Blocks per group: %d\n", info->blocks_per_group);
	printf("    Inodes per group: %d\n", info->inodes_per_group);
	printf("    Inode size: %d\n", info->inode_size);
	printf("    Journal blocks: %d\n", info->journal_blocks);
	printf("    Label: %s\n", info->label);
	printf("    UUID: %s\n", uuid_bin_to_str(buf, sizeof(buf), info->uuid));

	ext4_init_fs_aux_info(info, aux_info, setjmp_env);

	printf("    Blocks: %" PRIu64 "\n", aux_info->len_blocks);
	printf("    Block groups: %d\n", aux_info->groups);
	printf("    Reserved blocks: %" PRIu64 "\n",
	       (aux_info->len_blocks / 100) * info->reserve_pcnt);
	printf("    Reserved block group size: %d\n",
	       info->bg_desc_reserve_blocks);

	ext4_sparse_file = sparse_file_new(info->block_size, info->len);

	block_allocator_init(info, aux_info, ext4_sparse_file, force,
			     setjmp_env);

	ext4_fill_in_sb(info, aux_info, ext4_sparse_file, setjmp_env);

	if (reserve_inodes(aux_info, 0, 10) == EXT4_ALLOCATE_FAILED)
		error(force, setjmp_env, "failed to reserve first 10 inodes");

	if (info->feat_compat & EXT4_FEATURE_COMPAT_HAS_JOURNAL)
		ext4_create_journal_inode(info, aux_info, ext4_sparse_file,
					  force, setjmp_env);

	if (info->feat_compat & EXT4_FEATURE_COMPAT_RESIZE_INODE)
		ext4_create_resize_inode(info, aux_info, ext4_sparse_file,
					 force, setjmp_env);

	if (directory)
		root_inode_num = build_directory_structure(info, aux_info,
							   ext4_sparse_file,
							   saved_allocation_head,
							   config_list, force,
							   setjmp_env,
							   directory, "", 0,
							   fs_config_func,
							   verbose, fixed_time);
	else
		root_inode_num = build_default_directory_structure(info,
								   aux_info,
								   ext4_sparse_file,
								   force,
								   setjmp_env,
								   fixed_time);

	root_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	inode_set_permissions(info, aux_info, ext4_sparse_file, setjmp_env,
			      root_inode_num, root_mode, 0, 0,
			      (fixed_time != 1) ? fixed_time : 0);

	ext4_update_free(aux_info);

	ext4_queue_sb(info, aux_info, ext4_sparse_file, setjmp_env);

	if (block_list_file) {
		size_t dirlen = strlen(directory);
		struct block_allocation *p = saved_allocation_head;
		while (p) {
			if (strncmp(p->filename, directory, dirlen) == 0) {
				fprintf(block_list_file, "%s",
					p->filename + dirlen);
			} else {
				fprintf(block_list_file, "%s", p->filename);
			}
			print_blocks(block_list_file, p);
			struct block_allocation *pn = p->next;
			free_alloc(p);
			p = pn;
		}
	}

	printf("Created filesystem with %d/%d inodes and %d/%d blocks\n",
	       aux_info->sb->s_inodes_count - aux_info->sb->s_free_inodes_count,
	       aux_info->sb->s_inodes_count,
	       aux_info->sb->s_blocks_count_lo -
	       aux_info->sb->s_free_blocks_count_lo,
	       aux_info->sb->s_blocks_count_lo);

	if (wipe && WIPE_IS_SUPPORTED) {
		wipe_block_device(fd, info->len);
	}

	write_ext4_image(ext4_sparse_file, fd, gzip, sparse, crc);

	sparse_file_destroy(ext4_sparse_file);
	ext4_sparse_file = NULL;

	free(directory);

	return 0;
}
