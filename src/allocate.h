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

#ifndef _ALLOCATE_H_
#define _ALLOCATE_H_

#define EXT4_ALLOCATE_FAILED (u32)(~0)

#include "ext4_utils.h"

struct region;

struct region_list {
	struct region *first;
	struct region *last;
	struct region *iter;
	u32 partial_iter;
};

struct block_allocation {
	struct region_list list;
	struct region_list oob_list;
	char *filename;
	struct block_allocation *next;
};

struct sparse_file;

void block_allocator_init(struct fs_info *info, struct fs_aux_info *aux_info,
			  struct sparse_file *ext4_sparse_file, int force,
			  jmp_buf *setjmp_env);
void block_allocator_free(struct fs_aux_info *aux_info);
u32 allocate_block(struct fs_aux_info *aux_info, int force,
		   jmp_buf *setjmp_env);
struct block_allocation *allocate_blocks(struct fs_aux_info *aux_info,
					 int force, jmp_buf *setjmp_env,
					 u32 len);
int block_allocation_num_regions(struct block_allocation *alloc);
int block_allocation_len(struct block_allocation *alloc);
struct ext4_inode *get_inode(struct fs_info *info, struct fs_aux_info *aux_info,
			     struct sparse_file *ext4_sparse_file,
			     jmp_buf *setjmp_env, u32 inode);
struct ext4_xattr_header *get_xattr_block_for_inode(struct fs_info *info, struct fs_aux_info
						    *aux_info, struct sparse_file
						    *ext4_sparse_file,
						    int force,
						    jmp_buf *setjmp_env,
						    struct ext4_inode *inode);
void reduce_allocation(struct fs_aux_info *aux_info,
		       struct block_allocation *alloc, u32 len);
u32 get_block(struct block_allocation *alloc, u32 block);
u32 get_oob_block(struct block_allocation *alloc, u32 block);
void get_next_region(struct block_allocation *alloc);
void get_region(struct block_allocation *alloc, u32 *block, u32 *len);
u32 get_free_blocks(struct fs_aux_info *aux_info, u32 bg);
u32 get_free_inodes(struct fs_aux_info *aux_info, u32 bg);
u32 reserve_inodes(struct fs_aux_info *aux_info, int bg, u32 inodes);
void add_directory(struct fs_info *info, struct fs_aux_info *aux_info,
		   u32 inode);
u16 get_directories(struct fs_aux_info *aux_info, int bg);
u16 get_bg_flags(struct fs_aux_info *aux_info, int bg);
u32 allocate_inode(struct fs_info *info, struct fs_aux_info *aux_info);
void free_alloc(struct block_allocation *alloc);
int reserve_oob_blocks(struct block_allocation *alloc, jmp_buf *setjmp_env,
		       int blocks);
int advance_blocks(struct block_allocation *alloc, int blocks);
int advance_oob_blocks(struct block_allocation *alloc, int blocks);
int last_region(struct block_allocation *alloc);
void rewind_alloc(struct block_allocation *alloc);
void append_region(struct block_allocation *alloc, jmp_buf *setjmp_env,
		   u32 block, u32 len, int bg);
struct block_allocation *create_allocation(jmp_buf *setjmp_env);
int append_oob_allocation(struct fs_aux_info *aux_info, int force,
			  jmp_buf *setjmp_env, struct block_allocation *alloc,
			  u32 len);
void print_blocks(FILE *f, struct block_allocation *alloc);

#endif
