 /* SPDX-License-Identifier: Apache-2.0 */
 /* Copyright (C) 2025 Stichting Open Electronics Lab */

#include "check-util.h"

#include <stdarg.h>
#include <string.h>

unsigned fcheck_str_m(FILE *err, const char *file, int line, const char *func,
		      const char *expected, const char *actual,
		      const char *msg_fmt, ...)
{
	va_list args;

	if (expected == actual) {
		return 0;
	}
	if (expected && actual && strcmp(expected, actual) == 0) {
		return 0;
	}

	/* "if two or more handles are used, and any one of them is a stream,
	 * the application shall ensure that their actions are coordinated as
	 * described below. If this is not done, the result is undefined."
	 *
	 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_05_01
	 */
	fflush(stdout);

	fprintf(err, "\n%s:%d %s(): FAIL!", file, line, func);

	if (msg_fmt) {
		fprintf(err, " ");
		va_start(args, msg_fmt);
		vfprintf(err, msg_fmt, args);
		va_end(args);
	} else {
		fprintf(err, "\n\texpected '%s'", expected);
		fprintf(err, "\n\t but was '%s'", actual);
	}
	fprintf(err, "\n");
	fflush(err);

	return 1;
}
