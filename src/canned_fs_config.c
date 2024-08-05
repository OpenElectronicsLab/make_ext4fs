/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "canned_fs_config.h"


static int fs_config_path_compare(const void *a, const void *b)
{
	return strcmp(((struct fs_config_path *)a)->path,
		      ((struct fs_config_path *)b)->path);
}

static int fs_config_list_grow(struct fs_config_list *config_list)
{
	config_list->canned_alloc = (config_list->canned_alloc + 1) * 2;
	size_t size = config_list->canned_alloc * sizeof(struct fs_config_path);
	errno = 0;
	void *ptr = realloc(config_list->canned_data, size);
	if (!ptr) {
		if (errno != 0) {
			fprintf(stderr, "realloc (%zu) failed: %s\n", size,
				strerror(errno));
		} else {
			fprintf(stderr, "realloc (%zu) failed\n", size);
		}
		return -1;
	}
	config_list->canned_data = (struct fs_config_path *)ptr;
	return 0;
}

int load_canned_fs_config(struct fs_config_list *config_list, const char *fn)
{
	FILE *f = fopen(fn, "r");
	if (f == NULL) {
		fprintf(stderr, "failed to open %s: %s\n", fn, strerror(errno));
		return -1;
	}

	/* TODO: document why +200 */
	char line[PATH_MAX + 200];

	while (fgets(line, sizeof(line), f)) {
		while (config_list->canned_used >= config_list->canned_alloc) {
			if (fs_config_list_grow(config_list)) {
				return -1;
			}
		}
		struct fs_config_path *p =
		    config_list->canned_data + config_list->canned_used;
		p->path = strdup(strtok(line, " \t"));

		if (!p->path || !*p->path || *p->path == '#')
			continue;

		p->uid = atoi(strtok(NULL, " \t"));
		p->gid = atoi(strtok(NULL, " \t"));
		p->mode = strtol(strtok(NULL, " \t"), NULL, 8);	// mode is in octal
		p->capabilities = 0;

		char *token = NULL;
		do {
			token = strtok(NULL, " \t");
			if (token && strncmp(token, "capabilities=", 13) == 0) {
				p->capabilities = strtoll(token + 13, NULL, 0);
				break;
			}
		} while (token);

		config_list->canned_used++;
	}

	fclose(f);

	qsort(config_list->canned_data, config_list->canned_used,
	      sizeof(struct fs_config_path), fs_config_path_compare);

	printf("loaded %zu fs_config entries\n", config_list->canned_used);

	return 0;
}

int canned_fs_config(struct fs_config_list *config_list, const char *path,
		     int dir, unsigned *uid, unsigned *gid, unsigned *mode,
		     uint64_t *capabilities)
{
	struct fs_config_path key;
	key.path = path;
	struct fs_config_path *p =
	    (struct fs_config_path *)bsearch(&key, config_list->canned_data,
					     config_list->canned_used,
					     sizeof(struct fs_config_path),
					     fs_config_path_compare);
	if (p == NULL) {
		return 0;
	}
	(void)dir;
	*uid = p->uid;
	*gid = p->gid;
	*mode = p->mode;
	*capabilities = p->capabilities;

#if 0
	// for debugging, run the built-in fs_config and compare the results.

	unsigned c_uid, c_gid, c_mode;
	uint64_t c_capabilities;
	fs_config(path, dir, &c_uid, &c_gid, &c_mode, &c_capabilities);

	if (c_uid != *uid) printf("%s uid %d %d\n", path, *uid, c_uid);
	if (c_gid != *gid) printf("%s gid %d %d\n", path, *gid, c_gid);
	if (c_mode != *mode) printf("%s mode 0%o 0%o\n", path, *mode, c_mode);
	if (c_capabilities != *capabilities) printf("%s capabilities %llx %llx\n", path, *capabilities, c_capabilities);
#endif

	return 1;
}
