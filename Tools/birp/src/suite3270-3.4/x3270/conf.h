/* conf.h.  Generated from conf.h.in by configure.  */
/*
 * Copyright (c) 2000-2009, 2014-2015 Paul Mattes.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Paul Mattes nor his contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	conf.h
 *              System-specific #defines for libraries and library functions.
 */


/* Libraries. */
#define HAVE_LIBSSL 1

/* Header files. */
#define HAVE_SYS_SELECT_H 1
/* #undef HAVE_READLINE_HISTORY_H */
#define HAVE_PTY_H 1
/* #undef HAVE_LIBUTIL_H */
/* #undef HAVE_UTIL_H */
#define HAVE_GETOPT_H 1

/* Uncommon functions. */
#define HAVE_VASPRINTF 1
#define HAVE_FSEEKO 1
#define HAVE_FORKPTY 1

/* Configuration options. */
/* #undef USE_ICONV */

/* Standard types. */
/* #undef HAVE_DECL___UINT32_T */
/* #undef HAVE_DECL_UINT32_T */
/* #undef HAVE_DECL___UINT16T */
/* #undef HAVE_DECL_UINT16_T */

/* Optional parts. */
#define X3270_DBCS 1
#define X3270_LOCAL_PROCESS 1
#define X3270_IPV6 1
