/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef _CANNED_FS_CONFIG_H
#define _CANNED_FS_CONFIG_H

#include <inttypes.h>

struct fs_config_path {
	const char *path;
	unsigned uid;
	unsigned gid;
	unsigned mode;
	uint64_t capabilities;
};

struct fs_config_list {
	struct fs_config_path *canned_data;
	size_t canned_alloc;
	size_t canned_used;
};

int load_canned_fs_config(struct fs_config_list *config_list, const char *fn);

int canned_fs_config(struct fs_config_list *config_list, const char *path,
		     int dir, unsigned *uid, unsigned *gid, unsigned *mode,
		     uint64_t *capabilities);

#endif
