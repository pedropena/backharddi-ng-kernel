#include "common.h"
#include "strutl.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <wchar.h>

#ifdef HAVE_LIBTEXTWRAP
#include <textwrap.h>
#endif
#include <assert.h>

/*  Structure used to sort translated lists  */
typedef struct {
    char *string;
    int index;
} sort_str_t;

int strcountcmp(const char *s1, const char *e1, const char *s2, const char *e2)
{
	for (; s1 != e1 && s2 != e2 && *s1 == *s2; s1++, s2++) ;

	if (s1 == e1 && s2 == e2)
		return 0;

	if (s1 == e1)
		return 1;
	if (s2 == e2)
		return -1;
	if (*s1 < *s2)
		return -1;
	return 1;
}

char *strstrip(char *buf)
{
	char *end;

	/* skip initial spaces */
	for (; *buf != 0 && isspace(*buf); buf++) ;

	if (*buf == 0)
		return buf;

	end = buf + strlen(buf) - 1;
	while (end != buf - 1)
	{
		if (isspace(*end) == 0)
			break;

		*end = 0;
		end--;
	}
	return buf;
}

char *strlower(char *buf)
{
	char *p = buf;
	while (*p != 0) *p = tolower(*p);
	return buf;
}

void strvacat(char *buf, size_t maxlen, ...)
{
	va_list ap;
	char *str;
	size_t len = strlen(buf);

	va_start(ap, maxlen);
	
	while (1)
	{
		str = va_arg(ap, char *);
		if (str == NULL)
			break;
		if (len + strlen(str) > maxlen)
			break;
		strcat(buf, str);
		len += strlen(str);
	}
	va_end(ap);
}

int strparsecword(char **inbuf, char *outbuf, size_t maxlen)
{
	char buffer[maxlen];
	char *buf = buffer;
	char *c = *inbuf;
	char *start;

	for (; *c != 0 && isspace(*c); c++);

	if (*c == 0) 
		return 0;
	if (strlen(*inbuf) > maxlen)
		return 0;
	for (; *c != 0; c++)
	{
		if (*c == '"')
		{
			start = c+1;
			for (c++; *c != 0 && *c != '"'; c++)
			{
				if (*c == '\\')
				{
					c++;
					if (*c == 0)
						return 0;
				}
			}
			if (*c == 0)
				return 0;
			/* dequote the string */
			strunescape(start, buf, (int) (c - start + 1), 1);
			buf += strlen(buf);
			continue;
		}
		
		if (c != *inbuf && isspace(*c) != 0 && isspace(c[-1]) != 0)
			continue;
		if (isspace(*c) == 0)
			return 0;
		*buf++ = ' ';
	}
	*buf = 0;
	strncpy(outbuf, buffer, maxlen);
	*inbuf = c;

	return 1;
}

int strparsequoteword(char **inbuf, char *outbuf, size_t maxlen)
{
	char *start = *inbuf;
	char *c;

	/* skip ws, return if empty */
	for (; *start != 0 && isspace(*start); start++); 
	if (*start == 0)
		return 0;

	c = start;
	
	for (; *c != 0 && isspace(*c) == 0; c++)
	{
		if (*c == '"')
		{
			for (c++; *c != 0 && *c != '"'; c++)
			{
				if (*c == '\\')
				{
					c++;
					if (*c == 0)
						return 0;
				}
			}
			if (*c == 0)
				return 0;
		}
		if (*c == '[')
		{
			for (c++; *c != 0 && *c != ']'; c++);
			if (*c == 0)
				return 0;
		}
	}

	/* dequote the string */
	strunescape(start, outbuf, (int) (c - start + 1), 1);

	/* skip trailing spaces */
	for (; *c != 0 && isspace(*c); c++);
	*inbuf = c;

	return 1;
}

int strgetargc(const char *inbuf)
{
    int count = 1;
    const char *s;

    if (inbuf == 0 || *inbuf == 0)
        return 0;
    for (s=inbuf; *s != 0; s++)
    {
        if (*s == '\\' && *(s+1) == ',')
            s++;
        else if (*s == ',')
            count++;
    }
    return count;
}

int strchoicesplit(const char *inbuf, char **argv, size_t maxnarg)
{
    int argc = 0, i;
    const char *s = inbuf, *e, *c;
    char *p;

    if (inbuf == 0) return 0;

    INFO(INFO_VERBOSE, "Splitting [%s]", inbuf);
    while (*s != 0 && argc < maxnarg)
    {
        /* skip initial spaces */
        while (isspace(*s)) s++;

        /* find end */
        e = s;
        while (*e != 0)
        {
            if (*e == '\\' && (*(e+1) == ',' || *(e+1) == ' '))
                e += 2;
            else if (*e == ',')
                break;
            else
                e++;
        }

        argv[argc] = malloc(e-s+1);
        for (c = s, i = 0; c < e; c++, i++)
        {
            if (*c == '\\' && c < (e-1) && (*(c+1) == ',' || *(c+1) == ' '))
            {
                argv[argc][i] = *(c+1);
                c++;
            }
            else
                argv[argc][i] = *c;
        }
        argv[argc][i] = 0;
        p = &argv[argc][i-1];
        /* strip off trailing spaces */
        while (p > argv[argc] && *p == ' ') *p-- = 0;
        argc++;

        s = e;
        if (*s == ',') s++;
    }
    return argc;
}

int strchoicesplitsort(const char *origbuf, const char *transbuf, const char *indices, char **oargv, char **targv, int *oindex, size_t maxnarg)
{
    int i;
    char **cindex;
    char **sorted_targv;

    assert(oindex);
    assert(oargv);
    assert(targv);
    assert(origbuf);
    assert(transbuf);

    if (strchoicesplit(origbuf, oargv, maxnarg) != maxnarg)
        return DC_NOTOK;
    if (strchoicesplit(transbuf, targv, maxnarg) != maxnarg)
        return DC_NOTOK;
    if (indices == NULL || *indices == '\0') {
        for (i = 0; i < maxnarg; i++)
            oindex[i] = i;
    } else {
        cindex = malloc(sizeof(char *) * maxnarg);
        if (strchoicesplit(indices, cindex, maxnarg) != maxnarg) {
            INFO(INFO_WARN, "length of indices list '%s' != expected length %zd", indices, maxnarg);
            /* fall back semi-gracefully to unsorted list */
            for (i = 0; i < maxnarg; i++)
                oindex[i] = i;
            return maxnarg;
        }
        sorted_targv = malloc(sizeof(char *) * maxnarg);
        for (i = 0; i < maxnarg; i++) {
            oindex[i] = strtol(cindex[i], NULL, 10) - 1;
            if (oindex[i] < 0 || oindex[i] >= maxnarg) {
                int j;
                INFO(INFO_WARN, "index %d in indices list '%s' out of range", oindex[i] + 1, indices);
                /* fall back semi-gracefully to unsorted list */
                for (j = 0; j < maxnarg; j++)
                    oindex[j] = j;
                return maxnarg;
            }
            sorted_targv[i] = STRDUP(targv[oindex[i]]);
        }
        for (i = 0; i < maxnarg; i++) {
            free(targv[i]);
            targv[i] = sorted_targv[i];
        }
        free(sorted_targv);
        free(cindex);
    }
    return maxnarg;
}

int strcmdsplit(char *inbuf, char **argv, size_t maxnarg)
{
	int argc = 0;
	int inspace = 1;

	if (maxnarg == 0) return 0;
	
	for (; *inbuf != 0; inbuf++)
	{
		if (isspace(*inbuf) != 0)
		{
			inspace = 1;
			*inbuf = 0;
		}
		else if (inspace != 0)
		{
			inspace = 0;
			argv[argc++] = inbuf;
			if (argc >= maxnarg)
				break;
		}
	}

	return argc;
}

void strunescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote)
{
	const char *p = inbuf;
	int i = 0;
	while (*p != 0 && i < maxlen-1)
	{
		/*  Debconf only escapes \n  */
		if (*p == '\\')
		{
			if (*(p+1) == 'n')
			{
				outbuf[i++] = '\n';
				p += 2;
			}
			else if (quote != 0 && (*(p+1) == '"' || *(p+1) == '\\'))
			{
				outbuf[i++] = *(p+1);
				p += 2;
			}
			else
				outbuf[i++] = *p++;
		}
		else
			outbuf[i++] = *p++;
	}
	outbuf[i] = 0;
}

void strescape(const char *inbuf, char *outbuf, const size_t maxlen, const int quote)
{
	const char *p = inbuf;
	int i = 0;
	while (*p != 0 && i < maxlen-1)
	{
		/*  Debconf only escapes \n  */
		if (*p == '\n')
		{
			if (i + 2 >= maxlen) break;
			outbuf[i++] = '\\';
			outbuf[i++] = 'n';
			p++;
		}
		else if (quote != 0 && (*p == '"' || *p == '\\'))
		{
			if (i + 2 >= maxlen) break;
			outbuf[i++] = '\\';
			outbuf[i++] = *p++;
		}
		else
			outbuf[i++] = *p++;
	}
	outbuf[i] = 0;
}

/* These versions allocate their own memory.
 * TODO: rename to something more self-explanatory
 */

char *unescapestr(const char *in)
{
	static size_t buflen = 0;
	static char *buf = NULL;
	size_t inlen;

	if (in == 0) return 0;

	inlen = strlen(in) + 1;
	/* This is slightly too much memory, because each escaped newline
	 * will be unescaped; but an upper bound is fine.
	 */
	if (buflen < inlen) {
		buflen = inlen;
		buf = realloc(buf, buflen * sizeof *buf);
		if (!buf)
			DIE("Out of memory");
	}

	strunescape(in, buf, buflen, 0);
	return buf;
}

char *escapestr(const char *in)
{
	static size_t buflen = 0;
	static char *buf = NULL;
	size_t inlen;
	const char *p;

	if (in == 0) return 0;

	inlen = strlen(in) + 1;

	/* Each newline will consume an additional byte due to escaping. */
	for (p = in; *p; ++p)
		if (*p == '\n')
			++inlen;

	if (buflen < inlen) {
		buflen = inlen;
		buf = realloc(buf, buflen * sizeof *buf);
		if (!buf)
			DIE("Out of memory");
	}

	strescape(in, buf, buflen, 0);
	return buf;
}

int strwrap(const char *str, const int width, char *lines[], int maxlines)
{
#ifdef HAVE_LIBTEXTWRAP
	textwrap_t p;
	int j;
	char *s;
	char *s0;
	char *t;

	textwrap_init(&p);
	textwrap_columns(&p, width);
	s0 = s = textwrap(&p, str);
	for (j=0; j<maxlines; j++)
	{
		t = strchr(s, '\n');
		if (t == NULL)
		{
			lines[j] = (char *)malloc(strlen(s) + 1);
			strcpy(lines[j], s);
			free(s0);
			return j + 1;
		}
		else
		{
			lines[j] = (char *)malloc(t - s + 1);
			strncpy(lines[j], s, t-s); lines[j][t-s] = 0;
			s = t + 1;
		}
	}
	return j;
#else
	/* "Simple" greedy line-wrapper */
	int len = STRLEN(str);
	int l = 0;
	const char *s, *e, *end, *lb;

	if (str == 0) return 0;

	/*
	 * s = first character of current line, 
	 * e = last character of current line
	 * end = last character of the input string (the trailing \0)
	 */
	s = e = str;
	end = str + len;
	
	while (len > 0)
	{
		/* try to fit the most characters */
		e = s + width - 1;
		
		/* simple case: we got everything */
		if (e >= end) 
		{
			e = end;
		}
		else
		{
			/* find a breaking point */
			while (e > s && !isspace(*e) && *e != '.' && *e != ',')
				e--;
		}
		/* no word-break point found, so just unconditionally break 
		 * the line */
		if (e == s) e = s + width;

		/* if there's an explicit linebreak, honor it */
		lb = strchr(s, '\n');
		if (lb != NULL && lb < e) e = lb;

		lines[l] = (char *)malloc(e-s+2);
		strncpy(lines[l], s, e-s+1);
		lines[l][e-s+1] = 0;
		CHOMP(lines[l]);

		len -= (e-s+1);
		s = e + 1;
		if (++l >= maxlines) break;
	}
	return l;
#endif /* HAVE_LIBTEXTWRAP */
}

int strlongest(char **strs, int count)
{
	int i, max = 0;
	size_t width;

	for (i = 0; i < count; i++)
	{
		width = strwidth(strs[i]);
		if (width > max)
			max = width;
	}
	return max;
}

size_t
strwidth (const char *what)
{
    size_t res;
    int k;
    const char *p;
    wchar_t c;

    for (res = 0, p = what ; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0 ; p += k)
        res += wcwidth (c);

    return res;
}  

int
strtruncate (char *what, size_t maxsize)
{
    size_t pos;
    int k;
    char *p;
    wchar_t c;

    if (strwidth(what) <= maxsize)
        return 0;

    /*  Replace end of string with ellipsis "..."; as the last character
     *  may have a double width, stops 4 characters before the end
     */
    pos = 0;
    for (p = what; (k = mbtowc (&c, p, MB_LEN_MAX)) > 0 && pos < maxsize-5; p += k)
        pos += wcwidth (c);

    for (k=0; k < 3; k++, p++)
        *p = '.';
    *p = '\0';
    return 1;
}  

