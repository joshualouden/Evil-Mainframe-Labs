/*
 * Copyright (c) 2009, 2013-2015 Paul Mattes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of Paul Mattes, Jeff Sparkes, GTRC nor the names of
 *       their contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	readres.c
 *		A displayless 3270 Terminal Emulator
 *		Resource file reader.
 */

#include "globals.h"

#include <errno.h>

#include "glue.h"
#include "readres.h"
#include "utils.h"

/*
 * Make sure a resource definition begins with the application name, then
 * split it into the name and the value.
 */
int
validate_and_split_resource(const char *where, const char *arg,
	const char **left, unsigned *rnlenp, const char **right)
{
    unsigned match_len;
    unsigned rnlen;
    const char *s = arg;
    static char *me_dot = NULL;
    static char *me_star = NULL;
    static size_t me_len = 0;

    if (me_dot == NULL) {
	me_dot = xs_buffer("%s.", app);
	me_star = xs_buffer("%s*", app);
	me_len = strlen(me_dot);
    }

    /* Enforce "-3270." or "-3270*" or "*". */
    if (!strncmp(s, me_dot, me_len)) {
	match_len = me_len;
    } else if (!strncmp(arg, me_star, me_len)) {
	match_len = me_len;
    } else if (arg[0] == '*') {
	match_len = 1;
    } else {
	xs_warning("%s: Invalid resource syntax '%.*s', name must begin with "
		"'%s'", where, (int)me_len, arg, me_dot);
	return -1;
    }

    /* Separate the parts. */
    s = arg + match_len;
    while (*s && *s != ':' && !isspace(*s)) {
	s++;
    }
    rnlen = s - (arg + match_len);
    if (!rnlen) {
	xs_warning("%s: Invalid resource syntax, missing resource name",
		where);
	return -1;
    }
    while (isspace(*s)) {
	s++;
    }
    if (*s != ':') {
	xs_warning("%s: Invalid resource syntax, missing ':'", where);
	return -1;
    }
    s++;
    while (isspace(*s)) {
	s++;
    }

    /* Return what we got. */
    *left = arg + match_len;
    *rnlenp = rnlen;
    *right = s;
    return 0;
}

/* Read resources from a file. */
bool
read_resource_filex(const char *filename, bool fatal)
{
    FILE *f;
    int ilen;
    char buf[4096];
    char *where;
    int lno = 0;

    f = fopen(filename, "r");
    if (f == NULL) {
	if (fatal) {
	    xs_warning("Cannot open '%s': %s", filename, strerror(errno));
	}
	return false;
    }

    /* Merge in what's in the file into the resource database. */
    ilen = 0;
    while (fgets(buf + ilen, sizeof(buf) - ilen, f) != NULL || ilen) {
	char *s;
	unsigned sl;
	bool bsl = false;

	lno++;

	/* Stip any trailing newline. */
	sl = strlen(buf + ilen);
	if (sl && (buf + ilen)[sl-1] == '\n') {
	    (buf + ilen)[--sl] = '\0';
	}

	/* Check for a trailing backslash. */
	s = buf + ilen;
	if ((sl > 0) && (s[sl - 1] == '\\')) {
	    s[sl - 1] = '\0';
	    bsl = true;
	}

	/* Skip leading whitespace. */
	s = buf;
	while (isspace(*s)) {
	    s++;
	}

	/* If this line is a continuation, try again. */
	if (bsl) {
	    ilen += strlen(buf + ilen);
	    if ((unsigned)ilen >= sizeof(buf) - 1) {
		xs_warning("%s:%d: Line too long\n", filename, lno);
		break;
	    }
	    continue;
	}

	/* Skip comments. */
	if (*s == '!') {
	    ilen = 0;
	    continue;
	}
	if (*s == '#') {
	    xs_warning("%s:%d: Invalid profile syntax ('#' ignored)", filename,
		    lno);
	    ilen = 0;
	    continue;
	}

	/* Strip trailing whitespace and check for empty lines. */
	sl = strlen(s);
	while (sl && isspace(s[sl-1])) {
	    s[--sl] = '\0';
	}
	if (!sl) {
	    ilen = 0;
	    continue;
	}

	/* Digest it. */
	where = xs_buffer("%s:%d", filename, lno);
	parse_xrm(s, where);
	Free(where);

	/* Get ready for the next iteration. */
	ilen = 0;
    }
    fclose(f);
    return true;
}
