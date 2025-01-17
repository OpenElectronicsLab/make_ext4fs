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

#ifndef _EXT4_UTILS_H_
#define _EXT4_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE 1
#include <sys/types.h>

#ifndef __APPLE__
#include <sys/sysmacros.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>

#include "ext4_sb.h"
struct sparse_file;
struct block_allocation;

// TODO: replace stderr with FILE *log

#define warn(fmt, args...) \
	do { \
		fprintf(stderr, "warning: %s: " fmt "\n", __func__, ##args); \
	} while (0)

#define error(force, jmpbuf_ptr, fmt, args...) \
	do { \
		fprintf(stderr, "error: %s: " fmt "\n", __func__, ##args); \
		if (!force) \
			longjmp(*jmpbuf_ptr, EXIT_FAILURE); \
	} while (0)

#define error_errno(force, jmpbuf_ptr, s, args...) \
	error(force, jmpbuf_ptr, s ": %s", ##args, strerror(errno))

#define critical_error(jmpbuf_ptr, fmt, args...) \
	do { \
		fprintf(stderr, "critical error: %s: " fmt "\n", \
			__func__, ## args); \
		longjmp(*jmpbuf_ptr, EXIT_FAILURE); \
	} while (0)

#define critical_error_errno(jmpbuf_ptr, s, args...) \
	critical_error(jmpbuf_ptr, s ": %s", ##args, strerror(errno))

#define EXT4_JNL_BACKUP_BLOCKS 1

#ifndef min			/* already defined by windows.h */
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1)/(y))
#define EXT4_ALIGN(x, y) ((y) * DIV_ROUND_UP((x), (y)))

/* XXX */
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)

#ifdef __LP64__
typedef unsigned long u64;
typedef signed long s64;
#else
typedef unsigned long long u64;
typedef signed long long s64;
#endif
typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;

struct fs_config_list;
typedef int (*fs_config_func_t)(struct fs_config_list * config_list,
				const char *path, int dir, unsigned *uid,
				unsigned *gid, unsigned *mode,
				uint64_t *capabilities);

struct block_group_info;
struct xattr_list_element;

struct ext2_group_desc {
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_flags;
	u32 bg_reserved[2];
	u16 bg_reserved16;
	u16 bg_checksum;
};

struct fs_aux_info {
	struct ext4_super_block *sb;
	struct ext4_super_block **backup_sb;
	struct ext2_group_desc *bg_desc;
	struct block_group_info *bgs;
	struct xattr_list_element *xattrs;
	u32 first_data_block;
	u64 len_blocks;
	u32 inode_table_blocks;
	u32 groups;
	u32 bg_desc_blocks;
	u32 default_i_flags;
	u32 blocks_per_ind;
	u32 blocks_per_dind;
	u32 blocks_per_tind;
};

// extern jmp_buf setjmp_env;

static inline int log_2(int j)
{
	int i;

	for (i = 0; j > 0; i++)
		j >>= 1;

	return i - 1;
}

int bitmap_get_bit(u8 *bitmap, u32 bit);
void bitmap_clear_bit(u8 *bitmap, u32 bit);
int ext4_bg_has_super_block(struct fs_info *info, int bg);
void read_sb(jmp_buf *setjmp_env, int fd, struct ext4_super_block *sb);
void write_sb(jmp_buf *setjmp_env, int fd, unsigned long long offset,
	      struct ext4_super_block *sb);
void write_ext4_image(struct sparse_file *ext4_sparse_file, int fd, int gz,
		      int sparse, int crc);
void ext4_init_fs_aux_info(struct fs_info *info, struct fs_aux_info *aux_info,
			   jmp_buf *setjmp_env);
void ext4_free_fs_aux_info(struct fs_aux_info *aux_info);
void ext4_fill_in_sb(struct fs_info *info, struct fs_aux_info *aux_info,
		     struct sparse_file *ext4_sparse_file, jmp_buf *setjmp_env);
void ext4_create_resize_inode(struct fs_info *info,
			      struct fs_aux_info *aux_info,
			      struct sparse_file *ext4_sparse_file,
			      int force, jmp_buf *setjmp_env);
void ext4_create_journal_inode(struct fs_info *info,
			       struct fs_aux_info *aux_info,
			       struct sparse_file *ext4_sparse_file,
			       int force, jmp_buf *setjmp_env);
void ext4_update_free(struct fs_aux_info *aux_info);
void ext4_queue_sb(struct fs_info *info, struct fs_aux_info *aux_info,
		   struct sparse_file *ext4_sparse_file, jmp_buf *setjmp_env);
u64 get_block_device_size(int fd);
int is_block_device_fd(int fd);
u64 get_file_size(struct fs_info *info, int fd);
u64 parse_num(const char *arg);
void ext4_parse_sb_info(struct fs_info *info, struct fs_aux_info *aux_info,
			int force, jmp_buf *setjmp_env,
			struct ext4_super_block *sb);
u16 ext4_crc16(u16 crc_in, const void *buf, int size);
uint8_t *parse_uuid(uint8_t bytes[16], const char *str, size_t len);
char *uuid_bin_to_str(char *buf, size_t buf_size, const uint8_t bytes[16]);

int make_ext4fs_internal(struct fs_info *info, struct fs_aux_info *aux_info,
			 struct sparse_file *ext4_sparse_file,
			 struct block_allocation *saved_allocation_head,
			 struct fs_config_list *config_list,
			 int force, jmp_buf *setjmp_env,
			 int uuid_user_specified, int fd,
			 const char *directory, fs_config_func_t fs_config_func,
			 int gzip, int sparse, int crc, int wipe, int verbose,
			 time_t fixed_time, FILE *block_list_file);

int read_ext(struct fs_info *info, struct fs_aux_info *aux_info, int force,
	     jmp_buf *setjmp_env, int fd, int verbose);

#ifdef __cplusplus
}
#endif

#endif
