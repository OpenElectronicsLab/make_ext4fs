 /* SPDX-License-Identifier: Apache-2.0 */
 /* Copyright (C) 2025 Stichting Open Electronics Lab */

#ifndef CHECK_UTIL_H
#define CHECK_UTIL_H

#include <stdio.h>

unsigned fcheck_str_m(FILE *err, const char *file, int line, const char *func,
		      const char *expected, const char *actual,
		      const char *msg_fmt, ...);

#define Fcheck_str_m(err, expected, actual, msg_fmt, ...) \
	fcheck_str_m(err, __FILE__, __LINE__, __func__, expected, actual,\
			msg_fmt __VA_OPT__(,) __VA_ARGS__)

#define Fcheck_str(err, expected, actual) \
	Fcheck_str_m(err, expected, actual, NULL)

#define Check_str_m(expected, actual, ...) \
	Fcheck_str_m(stderr, expected, actual, __VA_ARGS__)

#define Check_str(expected, actual) \
	Fcheck_str_m(stderr, expected, actual, NULL)

#endif /* #ifndef CHECK_UTIL_H */
