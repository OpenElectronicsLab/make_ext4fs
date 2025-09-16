 /* SPDX-License-Identifier: Apache-2.0 */
 /* Copyright (C) 2025 Stichting Open Electronics Lab */

#include "uuid5.h"
#include "ext4_utils.h"
#include "check-util.h"

#include <stdio.h>
#include <stddef.h>

int main(void)
{
	int errors = 0;
	uint8_t bytes[16];
	char buf[(2 * 16) + 4 + 1];
	const char *namespace = "01234567-89ab-4cde-8f01-234567890001";
	uint8_t namespace_uuid[16];
	const char *name = "foo";
	const char *expected = "80775d0b-7f70-000e-ae00-fde183b36c82";

	parse_uuid(namespace_uuid, namespace, strlen(namespace));

	uuid5(bytes, namespace_uuid, 16, (uint8_t *)name, strlen(name));

	char *result = uuid_bin_to_str(buf, sizeof(buf), bytes);

	errors += Check_str(expected, result);

	return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
