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
#ifndef EXT4FIXUP_H
#define EXTRFIXUP_H

int ext4fixup(struct fs_info *info, struct fs_aux_info *aux_info, int verbose,
	      int force, jmp_buf *setjmp_env, int no_write, char *fsdev);

int ext4fixup_internal(struct fs_info *info, struct fs_aux_info *aux_info,
		       int force, jmp_buf *setjmp_env, char *fsdev, int verbose,
		       int no_write, int stop_phase, int stop_loc,
		       int stop_count);

#endif /* #ifndef EXT4FIXUP_H */
