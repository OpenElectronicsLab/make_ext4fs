 /* SPDX-License-Identifier: Apache-2.0 */
 /* Copyright (C) 2025 Stichting Open Electronics Lab */

#include "check-util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

unsigned check_null_null(void)
{
	unsigned errors = 0;
	char *buf = NULL;
	size_t buf_sz = 0;
	FILE *err = open_memstream(&buf, &buf_sz);
	assert(err);

	errors += Fcheck_str(err, NULL, NULL);
	assert(!errors);

	fclose(err);
	errors += buf_sz ? 1 : 0;
	assert(!errors);

	free(buf);

	return errors;
}

unsigned check_foo_foo(void)
{
	unsigned errors = 0;
	char foo[4] = { 'f', 'o', 'o', '\0' };
	char *buf = NULL;
	size_t buf_sz = 0;
	FILE *err = open_memstream(&buf, &buf_sz);
	assert(err);

	errors += Fcheck_str(err, "foo", foo);
	assert(!errors);

	fclose(err);
	errors += buf_sz ? 1 : 0;
	assert(!errors);

	free(buf);

	return errors;
}

unsigned check_foo_null(void)
{
	unsigned errors = 0;
	char *buf = NULL;
	size_t buf_sz = 0;
	FILE *err = open_memstream(&buf, &buf_sz);
	assert(err);

	errors += (Fcheck_str(err, "foo", NULL) ? 0 : 1);
	assert(!errors);

	fclose(err);
	errors += (buf_sz == 0) ? 1 : 0;
	assert(!errors);

	free(buf);

	return errors;
}

unsigned check_foo_bar_m(void)
{
	unsigned errors = 0;
	char *buf = NULL;
	size_t buf_sz = 0;
	FILE *err = open_memstream(&buf, &buf_sz);
	assert(err);

	errors += (Fcheck_str_m(err, "foo", "bar", "f:%s", "bar") == 0) ? 1 : 0;
	assert(!errors);

	fclose(err);
	errors += (buf_sz == 0) ? 1 : 0;
	assert(!errors);

	errors += strstr(buf, "f:bar") ? 0 : 1;
	assert(!errors);

	free(buf);

	return errors;
}

int main(void)
{
	unsigned errors = 0;

	errors += check_null_null();
	errors += check_foo_foo();
	errors += check_foo_null();
	errors += check_foo_bar_m();

	return errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
