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

#include <string.h>

#include "sha1.h"

#include "uuid5.h"

static void sha1_hash(unsigned char sha1[SHA1_DIGEST_LENGTH],
		      const uint8_t *namespace, size_t namespace_len,
		      const uint8_t *name, size_t name_len)
{
	SHA1_CTX ctx;
	SHA1Init(&ctx);
	SHA1Update(&ctx, namespace, namespace_len);
	SHA1Update(&ctx, name, name_len);
	SHA1Final(sha1, &ctx);
}

/* see https://www.ietf.org/rfc/rfc9562.pdf */
void uuid5(uint8_t dest[16], const uint8_t *namespace, size_t namespace_len,
	   const uint8_t *name, size_t name_len)
{
	unsigned char sha1[SHA1_DIGEST_LENGTH];

	sha1_hash(sha1, namespace, namespace_len, name, name_len);
	memcpy(dest, sha1, 16);

	const size_t version_byte = 6;
	const size_t variant_byte = 8;
	// clear the upper 4 bits (version)
	// then set the version to 5 (0b0101)
	dest[version_byte] &= 0x0F;
	dest[version_byte] |= (5 << 12);

	// clear upper two bits (variant) of 'clk_seq_hi_res'
	// then set variant to 2 (0b10)
	dest[variant_byte] &= 0x3F;
	dest[variant_byte] |= (2 << 6);
}

void uuid5_generate(uint8_t dest[16], const char *namespace, const char *name)
{
	uuid5(dest, (const uint8_t *)namespace, strlen(namespace),
	      (const uint8_t *)name, strlen(name));
}
