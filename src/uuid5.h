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

#ifndef UUID5_H
#define UUID5_H

#include <stdint.h>
#include <stddef.h>

/* see https://www.ietf.org/rfc/rfc9562.pdf */
/* proper version 5 UUIDs are created from a namespace which is itself a UUID,
 * thus the namespace_len should be 16 */
void uuid5(uint8_t dest[16], const uint8_t *namespace, size_t namespace_len,
	   const uint8_t *name, size_t name_len);

/* the uuid5_generate relaxes the constraint for the namespace to be a UUID,
 * and it may be any number of bytes */
void uuid5_generate(uint8_t dest[16], const char *namespace, const char *name);

#endif
