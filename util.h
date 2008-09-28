#ifndef UTIL_H
#define UTIL_H

/*
 * Retrieve a set of key-value pairs from a file.
 *
 * The file should follow the format:
 *
 * [SPACE]<KEY>[SPACE]sep[SPACE]<VALUE>
 *
 * Any number of keys may be retrieved, and each value will be
 * returned using scanf.  For example:
 *
 * char *buf = NULL;
 * int a, b;
 * file_get_keys ("my_kv_file", ':',
 *                "my_string_key", "%as", 1, &buf,
 *                "my_int_pair_key", "%d %d", 2, &a, &b,
 *                NULL);
 *
 * If a required key is not found, the pointer arguments for that key
 * will not be modified.  file_get_keys() itself does not parse the
 * scanf format string, so the number of arguments for each format
 * must be specified by the caller.
 */
void
file_get_keys (const char *filename, char sep, ...)
//	       const char *key, const char *scanfmt, size_t nargs, ...)
  __attribute__ ((__sentinel__, __nonnull__ (1, 3, 4)));

#endif	/* UTIL_H */
