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

#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

struct dentry {
	char *path;
	char *full_path;
	const char *filename;
	char *link;
	unsigned long size;
	u8 file_type;
	u16 mode;
	u16 uid;
	u16 gid;
	u32 *inode;
	u32 mtime;
	uint64_t capabilities;
};

u32 make_directory(struct fs_info *info, struct fs_aux_info *aux_info,
		   struct sparse_file *ext4_sparse_file, int force,
		   jmp_buf *setjmp_env, u32 dir_inode_num, u32 entries,
		   struct dentry *dentries, u32 dirs);
u32 make_file(struct fs_info *info, struct fs_aux_info *aux_info,
	      struct sparse_file *ext4_sparse_file, int force,
	      jmp_buf *setjmp_env, const char *filename, u64 len);
u32 make_link(struct fs_info *info, struct fs_aux_info *aux_info,
	      struct sparse_file *ext4_sparse_file, int force,
	      jmp_buf *setjmp_env, const char *link);
u32 make_special(struct fs_info *info, struct fs_aux_info *aux_info,
		 struct sparse_file *ext4_sparse_file, int force,
		 jmp_buf *setjmp_env, const char *path);
int inode_set_permissions(struct fs_info *info, struct fs_aux_info *aux_info,
			  struct sparse_file *ext4_sparse_file, int force,
			  jmp_buf *setjmp_env, u32 inode_num, u16 mode, u16 uid,
			  u16 gid, u32 mtime);
int inode_set_capabilities(struct fs_info *info, struct fs_aux_info *aux_info,
			   struct sparse_file *ext4_sparse_file, int force,
			   jmp_buf *setjmp_env, u32 inode_num,
			   uint64_t capabilities);
struct block_allocation* get_saved_allocation_chain(void);

#endif
