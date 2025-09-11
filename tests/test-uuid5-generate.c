 /* SPDX-License-Identifier: Apache-2.0 */
 /* Copyright (C) 2025 Stichting Open Electronics Lab */

#include "uuid5.h"
#include "ext4_utils.h"

#include <stdio.h>
#include <stddef.h>

int main(void)
{
	int errors = 0;
	uint8_t bytes[16];
	char buf[(2 * 16) + 4 + 1];
	const char *namespace = "namespace";
	const char *name = "foo";
	const char *expected = "34f84db6-0424-0b41-94bc-851b55cbda3d";

	uuid5_generate(bytes, namespace, name);

	char *result = uuid_bin_to_str(buf, sizeof(buf), bytes);

	if (strcmp(result, expected) != 0) {
		++errors;
		fprintf(stderr, "expected '%s'\n"
			" but was '%s'\n", expected, result);
	}

	return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
