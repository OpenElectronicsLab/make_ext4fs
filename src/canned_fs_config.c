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

typedef struct {
	const char* path;
	unsigned uid;
	unsigned gid;
	unsigned mode;
	uint64_t capabilities;
} Path;

static Path* canned_data = NULL;
static int canned_alloc = 0;
static int canned_used = 0;

static int path_compare(const void* a, const void* b) {
	return strcmp(((Path*)a)->path, ((Path*)b)->path);
}

int load_canned_fs_config(const char* fn) {
	FILE* f = fopen(fn, "r");
	if (f == NULL) {
		fprintf(stderr, "failed to open %s: %s\n", fn, strerror(errno));
		return -1;
	}

	char line[PATH_MAX + 200];
	while (fgets(line, sizeof(line), f)) {
		while (canned_used >= canned_alloc) {
			canned_alloc = (canned_alloc+1) * 2;
			canned_data = (Path*) realloc(canned_data, canned_alloc * sizeof(Path));
		}
		Path* p = canned_data + canned_used;
		p->path = strdup(strtok(line, " \t"));

		if (!p->path || !*p->path || *p->path == '#')
			continue;

		p->uid = atoi(strtok(NULL, " \t"));
		p->gid = atoi(strtok(NULL, " \t"));
		p->mode = strtol(strtok(NULL, " \t"), NULL, 8);   // mode is in octal
		p->capabilities = 0;

		char* token = NULL;
		do {
			token = strtok(NULL, " \t");
			if (token && strncmp(token, "capabilities=", 13) == 0) {
				p->capabilities = strtoll(token+13, NULL, 0);
				break;
			}
		} while (token);

		canned_used++;
	}

	fclose(f);

	qsort(canned_data, canned_used, sizeof(Path), path_compare);
	printf("loaded %d fs_config entries\n", canned_used);

	return 0;
}

int canned_fs_config(const char* path, int dir,
					  unsigned* uid, unsigned* gid, unsigned* mode, uint64_t* capabilities) {
	Path key;
	key.path = path;
	Path* p = (Path*) bsearch(&key, canned_data, canned_used, sizeof(Path), path_compare);
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