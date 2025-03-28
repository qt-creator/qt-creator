/*-
 * Copyright (c) 2012 Michihiro NAKAJIMA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archive_platform.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "archive.h"
#include "archive_private.h"
#include "archive_string.h"
#include "archive_write_private.h"

#define LBYTES	57

struct private_b64encode {
	int			mode;
	struct archive_string	name;
	struct archive_string	encoded_buff;
	size_t			bs;
	size_t			hold_len;
	unsigned char		hold[LBYTES];
};

static int archive_filter_b64encode_options(struct archive_write_filter *,
    const char *, const char *);
static int archive_filter_b64encode_open(struct archive_write_filter *);
static int archive_filter_b64encode_write(struct archive_write_filter *,
    const void *, size_t);
static int archive_filter_b64encode_close(struct archive_write_filter *);
static int archive_filter_b64encode_free(struct archive_write_filter *);
static void la_b64_encode(struct archive_string *, const unsigned char *, size_t);
static int64_t atol8(const char *, size_t);

static const char base64[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

/*
 * Add a compress filter to this write handle.
 */
int
archive_write_add_filter_b64encode(struct archive *_a)
{
	struct archive_write *a = (struct archive_write *)_a;
	struct archive_write_filter *f = __archive_write_allocate_filter(_a);
	struct private_b64encode *state;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_write_add_filter_b64encode");

	state = calloc(1, sizeof(*state));
	if (state == NULL) {
		archive_set_error(f->archive, ENOMEM,
		    "Can't allocate data for b64encode filter");
		return (ARCHIVE_FATAL);
	}
	archive_strcpy(&state->name, "-");
	state->mode = 0644;

	f->data = state;
	f->name = "b64encode";
	f->code = ARCHIVE_FILTER_UU;
	f->open = archive_filter_b64encode_open;
	f->options = archive_filter_b64encode_options;
	f->write = archive_filter_b64encode_write;
	f->close = archive_filter_b64encode_close;
	f->free = archive_filter_b64encode_free;

	return (ARCHIVE_OK);
}

/*
 * Set write options.
 */
static int
archive_filter_b64encode_options(struct archive_write_filter *f, const char *key,
    const char *value)
{
	struct private_b64encode *state = (struct private_b64encode *)f->data;

	if (strcmp(key, "mode") == 0) {
		if (value == NULL) {
			archive_set_error(f->archive, ARCHIVE_ERRNO_MISC,
			    "mode option requires octal digits");
			return (ARCHIVE_FAILED);
		}
		state->mode = (int)atol8(value, strlen(value)) & 0777;
		return (ARCHIVE_OK);
	} else if (strcmp(key, "name") == 0) {
		if (value == NULL) {
			archive_set_error(f->archive, ARCHIVE_ERRNO_MISC,
			    "name option requires a string");
			return (ARCHIVE_FAILED);
		}
		archive_strcpy(&state->name, value);
		return (ARCHIVE_OK);
	}

	/* Note: The "warn" return is just to inform the options
	 * supervisor that we didn't handle it.  It will generate
	 * a suitable error if no one used this option. */
	return (ARCHIVE_WARN);
}

/*
 * Setup callback.
 */
static int
archive_filter_b64encode_open(struct archive_write_filter *f)
{
	struct private_b64encode *state = (struct private_b64encode *)f->data;
	size_t bs = 65536, bpb;

	if (f->archive->magic == ARCHIVE_WRITE_MAGIC) {
		/* Buffer size should be a multiple number of the bytes
		 * per block for performance. */
		bpb = archive_write_get_bytes_per_block(f->archive);
		if (bpb > bs)
			bs = bpb;
		else if (bpb != 0)
			bs -= bs % bpb;
	}

	state->bs = bs;
	if (archive_string_ensure(&state->encoded_buff, bs + 512) == NULL) {
		archive_set_error(f->archive, ENOMEM,
		    "Can't allocate data for b64encode buffer");
		return (ARCHIVE_FATAL);
	}

	archive_string_sprintf(&state->encoded_buff, "begin-base64 %o %s\n",
	    (unsigned int)state->mode, state->name.s);

	f->data = state;
	return (0);
}

static void
la_b64_encode(struct archive_string *as, const unsigned char *p, size_t len)
{
	int c;

	for (; len >= 3; p += 3, len -= 3) {
		c = p[0] >> 2;
		archive_strappend_char(as, base64[c]);
		c = ((p[0] & 0x03) << 4) | ((p[1] & 0xf0) >> 4);
		archive_strappend_char(as, base64[c]);
		c = ((p[1] & 0x0f) << 2) | ((p[2] & 0xc0) >> 6);
		archive_strappend_char(as, base64[c]);
		c = p[2] & 0x3f;
		archive_strappend_char(as, base64[c]);
	}
	if (len > 0) {
		c = p[0] >> 2;
		archive_strappend_char(as, base64[c]);
		c = (p[0] & 0x03) << 4;
		if (len == 1) {
			archive_strappend_char(as, base64[c]);
			archive_strappend_char(as, '=');
			archive_strappend_char(as, '=');
		} else {
			c |= (p[1] & 0xf0) >> 4;
			archive_strappend_char(as, base64[c]);
			c = (p[1] & 0x0f) << 2;
			archive_strappend_char(as, base64[c]);
			archive_strappend_char(as, '=');
		}
	}
	archive_strappend_char(as, '\n');
}

/*
 * Write data to the encoded stream.
 */
static int
archive_filter_b64encode_write(struct archive_write_filter *f, const void *buff,
    size_t length)
{
	struct private_b64encode *state = (struct private_b64encode *)f->data;
	const unsigned char *p = buff;
	int ret = ARCHIVE_OK;

	if (length == 0)
		return (ret);

	if (state->hold_len) {
		while (state->hold_len < LBYTES && length > 0) {
			state->hold[state->hold_len++] = *p++;
			length--;
		}
		if (state->hold_len < LBYTES)
			return (ret);
		la_b64_encode(&state->encoded_buff, state->hold, LBYTES);
		state->hold_len = 0;
	}

	for (; length >= LBYTES; length -= LBYTES, p += LBYTES)
		la_b64_encode(&state->encoded_buff, p, LBYTES);

	/* Save remaining bytes. */
	if (length > 0) {
		memcpy(state->hold, p, length);
		state->hold_len = length;
	}
	while (archive_strlen(&state->encoded_buff) >= state->bs) {
		ret = __archive_write_filter(f->next_filter,
		    state->encoded_buff.s, state->bs);
		memmove(state->encoded_buff.s,
		    state->encoded_buff.s + state->bs,
		    state->encoded_buff.length - state->bs);
		state->encoded_buff.length -= state->bs;
	}

	return (ret);
}


/*
 * Finish the compression...
 */
static int
archive_filter_b64encode_close(struct archive_write_filter *f)
{
	struct private_b64encode *state = (struct private_b64encode *)f->data;

	/* Flush remaining bytes. */
	if (state->hold_len != 0)
		la_b64_encode(&state->encoded_buff, state->hold, state->hold_len);
	archive_string_sprintf(&state->encoded_buff, "====\n");
	/* Write the last block */
	archive_write_set_bytes_in_last_block(f->archive, 1);
	return __archive_write_filter(f->next_filter,
	    state->encoded_buff.s, archive_strlen(&state->encoded_buff));
}

static int
archive_filter_b64encode_free(struct archive_write_filter *f)
{
	struct private_b64encode *state = (struct private_b64encode *)f->data;

	archive_string_free(&state->name);
	archive_string_free(&state->encoded_buff);
	free(state);
	return (ARCHIVE_OK);
}

static int64_t
atol8(const char *p, size_t char_cnt)
{
	int64_t l;
	int digit;
        
	l = 0;
	while (char_cnt-- > 0) {
		if (*p >= '0' && *p <= '7')
			digit = *p - '0';
		else
			break;
		p++;
		l <<= 3;
		l |= digit;
	}
	return (l);
}

