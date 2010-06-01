/** 
 * @brief strutl.c
 * @brief misc. routines for string handling
 */

#ifndef _STRUTL_H_
#define _STRUTL_H_

#include <string.h>

/**
 * @brief strcmp with explicit start/stop 
 * @param s1, e1 start and stop of first string
 * @param s2, e2 start and stop of second string
 * @return -1, 0, 1 as for strcmp
 */
int strcountcmp(const char *s1, const char *e1, const char *s2, const char *e2);

/**
 * @brief Strips leading and trailing spaces from a string; also 
 *        strips trialing newline characters from the string
 * @param buf string to strip
 */
char *strstrip(char *buf);

/**
 * @brief Converts a string to lowercase
 * @param buf string to convert
 */
char *strlower(char *buf);

/**
 * @brief concatenates arbitrary number of strings to a buffer, with 
 *        bounds checking
 * @param buf output buffer
 * @param len Length of buffer
 * @param ... one or or strings to concatenate (end with NULL)
 */
void strvacat(char *buf, size_t len, ...)
#if defined(__GNUC__) && __GNUC__ >= 4
	__attribute__((sentinel))
#endif
	;
int strparsecword(char **inbuf, char *outbuf, size_t maxlen);
int strparsequoteword(char **inbuf, char *outbuf, size_t maxlen);
int strgetargc(const char *inbuf);
int strchoicesplit(const char *inbuf, char **argv, size_t maxnarg);
int strchoicesplitsort(const char *origbuf, const char *transbuf, const char *listorder, char **oargv, char **targv, int *tindex, size_t maxnarg);
int strcmdsplit(char *inbuf, char **argv, size_t maxnarg);
void strunescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote);
void strescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote);
char *unescapestr(const char *in);
char *escapestr(const char *in);
int strwrap(const char *str, const int width, char *lines[], int maxlines);
int strlongest(char **strs, int count);

size_t strwidth(const char *width);
int strtruncate (char *what, size_t maxsize);

#endif
