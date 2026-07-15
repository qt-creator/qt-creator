// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// CBOR encode/decode, value types, map helpers, constructors.
// Included by cmdbridge.c — do not compile separately.

/* ================================================================== */
/*  TinyCBOR -- minimal CBOR encode/decode for CmdBridge              */
/* ================================================================== */

/* Strings and byte strings carry whole file contents (writefile writes a file
   in a single command), so they get no artificial size limit -- the Go
   implementation had none either. They are bounded by the bytes that actually
   arrived, which dval() checks before allocating anything.

   Container element counts still need an upper bound to tell "malformed" from
   "not all here yet": a count beyond this can never be satisfied, however much
   more input arrives. It costs one byte of input per element, so this is a
   very generous limit. */
#define MAX_CBOR_ELEMENTS (64 * 1024 * 1024)

typedef enum { V_STRING, V_BYTES, V_INT, V_UINT, V_BOOL, V_NULL, V_ARRAY, V_MAP } vtype;

/* Allocation helpers for building *outgoing* values, where the sizes are ours
   and a failure is unrecoverable. Matches Go, which aborts on OOM. The decoder
   deals with attacker-controlled sizes and checks every allocation instead. */
static void *xmalloc(size_t n)
{
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "cmdbridge: out of memory\n");
        _exit(70);
    }
    return p;
}

static char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *) xmalloc(n);
    memcpy(p, s, n);
    return p;
}

typedef struct value
{
    vtype type;
    union {
        char *str;
        uint8_t *bytes;
        int64_t i64;
        uint64_t u64;
        bool bval;
    };
    struct value **kids;
    size_t nkids;
} value;

typedef struct
{
    const uint8_t *d;
    size_t len;
    size_t pos;
    /* Set when decoding failed only because the input ended early, i.e. the
       caller should read more bytes rather than discard the value. */
    bool truncated;
} dec;
typedef struct
{
    uint8_t *b;
    size_t len;
    size_t cap;
} enc;

static value *vnew(void)
{
    return (value *) calloc(1, sizeof(value));
}

/* Same, but for values we build ourselves and cannot fail to build. */
static value *vnew_x(void)
{
    value *v = (value *) xmalloc(sizeof(value));
    memset(v, 0, sizeof(*v));
    return v;
}
static void vfree(value *v)
{
    if (!v)
        return;
    if (v->type == V_STRING) {
        free(v->str);
    } else if (v->type == V_BYTES) {
        free(v->bytes);
    } else if ((v->type == V_ARRAY || v->type == V_MAP) && v->kids) {
        for (size_t i = 0; i < v->nkids; i++)
            vfree(v->kids[i]);
        free(v->kids);
    }
    free(v);
}

/* --- Decoder helpers --- */

/* Number of bytes still unread. */
static size_t dleft(const dec *d)
{
    return d->len - d->pos;
}

static bool duint64(dec *d, uint8_t first, uint64_t *r)
{
    uint8_t a = first & 0x1F;
    if (a < 24) {
        *r = a;
        return true;
    }
    if (a == 24) {
        if (dleft(d) < 1) {
            d->truncated = true;
            return false;
        }
        *r = d->d[d->pos++];
        return true;
    }
    if (a == 25) {
        if (dleft(d) < 2) {
            d->truncated = true;
            return false;
        }
        *r = ((uint64_t) d->d[d->pos] << 8) | (uint64_t) d->d[d->pos + 1];
        d->pos += 2;
        return true;
    }
    if (a == 26) {
        if (dleft(d) < 4) {
            d->truncated = true;
            return false;
        }
        *r = ((uint64_t) d->d[d->pos] << 24) | ((uint64_t) d->d[d->pos + 1] << 16)
             | ((uint64_t) d->d[d->pos + 2] << 8) | (uint64_t) d->d[d->pos + 3];
        d->pos += 4;
        return true;
    }
    if (a > 27)
        return false; /* 28..30 reserved, 31 indefinite length: not supported */
    if (dleft(d) < 8) {
        d->truncated = true;
        return false;
    }
    {
        const uint8_t *p = &d->d[d->pos];
        *r = ((uint64_t) p[0] << 56) | ((uint64_t) p[1] << 48) | ((uint64_t) p[2] << 40)
             | ((uint64_t) p[3] << 32) | ((uint64_t) p[4] << 24) | ((uint64_t) p[5] << 16)
             | ((uint64_t) p[6] << 8) | (uint64_t) p[7];
        d->pos += 8;
        return true;
    }
}

#define CBOR_MAX_DEPTH 128

static bool dval(dec *d, value **out, int depth)
{
    if (d->pos >= d->len) {
        d->truncated = true;
        return false;
    }
    if (depth > CBOR_MAX_DEPTH) {
        return false;
    }
    uint8_t b = d->d[d->pos++];
    uint8_t maj = (b >> 5) & 7;
    value *v = vnew();

    switch (maj) {
    case 0: /* unsigned int */
        v->type = V_UINT;
        if (!duint64(d, b, &v->u64))
            goto err;
        break;
    case 1: /* negative int */
        v->type = V_INT;
        {
            uint64_t u;
            if (!duint64(d, b, &u))
                goto err;
            v->i64 = -(int64_t) (u + 1);
        }
        break;
    case 2: /* byte string */
    {
        uint64_t l;
        if (!duint64(d, b, &l))
            goto err;
        if (l > dleft(d)) {
            d->truncated = true;
            goto err;
        }
        v->type = V_BYTES;
        v->bytes = (uint8_t *) malloc(l ? l : 1);
        if (!v->bytes)
            goto err;
        memcpy(v->bytes, &d->d[d->pos], l);
        v->nkids = l;
        d->pos += l;
    } break;
    case 3: /* text string */
    {
        uint64_t l;
        if (!duint64(d, b, &l))
            goto err;
        if (l > dleft(d)) {
            d->truncated = true;
            goto err;
        }
        v->type = V_STRING;
        v->str = (char *) malloc(l + 1);
        if (!v->str)
            goto err;
        memcpy(v->str, &d->d[d->pos], l);
        v->str[l] = '\0';
        d->pos += l;
    } break;
    case 4: /* array */
    {
        uint64_t l;
        if (!duint64(d, b, &l))
            goto err;
        /* A count beyond the largest value we accept can never be satisfied by
           any amount of further input, so it is malformed rather than merely
           incomplete -- reporting it as incomplete would make the reader wait
           for bytes that never come and drop everything behind it. */
        if (l > MAX_CBOR_ELEMENTS)
            goto err;
        /* Every element needs at least one byte, so a count larger than the
           remaining input means the value is cut short. Checking this before
           allocating stops a 5-byte header from reserving gigabytes. */
        if (l > dleft(d)) {
            d->truncated = true;
            goto err;
        }
        v->type = V_ARRAY;
        v->kids = l ? (value **) calloc(l, sizeof(value *)) : NULL;
        if (l && !v->kids)
            goto err;
        {
            size_t decoded = 0;
            for (uint64_t i = 0; i < l; i++) {
                if (!dval(d, &v->kids[i], depth + 1)) {
                    v->nkids = decoded;
                    goto err2;
                }
                decoded++;
            }
            v->nkids = l;
        }
    } break;
    case 5: /* map */
    {
        uint64_t l;
        if (!duint64(d, b, &l))
            goto err;
        /* See the array case. */
        if (l > MAX_CBOR_ELEMENTS / 2)
            goto err;
        /* Each pair needs at least two bytes. */
        if (l > dleft(d) / 2) {
            d->truncated = true;
            goto err;
        }
        v->type = V_MAP;
        v->kids = l ? (value **) calloc(l * 2, sizeof(value *)) : NULL;
        if (l && !v->kids)
            goto err;
        {
            size_t decoded = 0;
            for (uint64_t i = 0; i < l; i++) {
                if (!dval(d, &v->kids[i * 2], depth + 1)) {
                    v->nkids = decoded;
                    goto err2;
                }
                if (!dval(d, &v->kids[i * 2 + 1], depth + 1)) {
                    v->nkids = decoded + 1;
                    goto err2;
                }
                decoded += 2;
            }
            v->nkids = l * 2;
        }
    } break;
    case 6: /* tag -- skip tag number and decode payload */
    {
        uint64_t t;
        if (!duint64(d, b, &t))
            goto err;
        (void) t;
        vfree(v); /* the tag itself carries no value of its own */
        return dval(d, out, depth + 1);
    }
    case 7: /* simple / float */
    {
        uint8_t s = b & 0x1F;
        if (s == 20) {
            v->type = V_BOOL;
            v->bval = false;
        } else if (s == 21) {
            v->type = V_BOOL;
            v->bval = true;
        } else if (s == 22 || s == 23) {
            v->type = V_NULL; /* null / undefined */
        } else if (s == 24) {
            /* One-byte simple value. */
            if (dleft(d) < 1) {
                d->truncated = true;
                goto err;
            }
            v->type = V_UINT;
            v->u64 = d->d[d->pos++];
        } else if (s >= 25 && s <= 27) {
            /* Half/single/double float. The protocol has no float-valued
               fields, but the bytes still have to be consumed so that framing
               stays correct; the value is reported as null. */
            size_t n = (size_t) 1 << (s - 24);
            if (dleft(d) < n) {
                d->truncated = true;
                goto err;
            }
            d->pos += n;
            v->type = V_NULL;
        } else {
            goto err;
        }
    } break;
    default:
        goto err;
    }
    *out = v;
    return true;
err2:;
    for (size_t i = 0; i < v->nkids; i++) {
        value *kid = v->kids[i];
        v->kids[i] = NULL;
        vfree(kid);
    }
    free(v->kids);
    v->kids = NULL;
    v->nkids = 0;
    free(v);
    return false;
err:
    vfree(v);
    return false;
}

static value *decode(const uint8_t *data, size_t len)
{
    dec d = {data, len, 0, false};
    value *r = NULL;
    if (dval(&d, &r, 0))
        return r;
    return NULL;
}

typedef enum {
    CBOR_OK,         /* a complete value was found, *out_size holds its length */
    CBOR_INCOMPLETE, /* the value is cut short, read more and try again */
    CBOR_INVALID     /* the bytes cannot start a valid value */
} cbor_scan_result;

/* Determines how many bytes the CBOR value at data[0..len-1] occupies.
   Distinguishing "incomplete" from "invalid" is what lets the reader wait for
   the rest of a command instead of throwing it away. */
static cbor_scan_result cbor_scan(const uint8_t *data, size_t len, size_t *out_size)
{
    if (len == 0)
        return CBOR_INCOMPLETE;
    dec d = {data, len, 0, false};
    value *v = NULL;
    if (!dval(&d, &v, 0)) {
        vfree(v);
        return d.truncated ? CBOR_INCOMPLETE : CBOR_INVALID;
    }
    vfree(v);
    *out_size = d.pos;
    return CBOR_OK;
}

/* --- Encoder helpers --- */

static bool einit(enc *e)
{
    e->cap = 512;
    e->len = 0;
    e->b = (uint8_t *) malloc(e->cap);
    return (bool) e->b;
}
static void efree(enc *e)
{
    free(e->b);
    e->b = NULL;
}

static bool egrow(enc *e, size_t n)
{
    while (e->len + n > e->cap) {
        size_t nc = e->cap * 2;
        if (nc < e->len + n)
            nc = e->len + n;
        uint8_t *nb = (uint8_t *) realloc(e->b, nc);
        if (!nb)
            return false;
        e->b = nb;
        e->cap = nc;
    }
    return true;
}

static bool eput(enc *e, uint8_t b)
{
    if (!egrow(e, 1))
        return false;
    e->b[e->len++] = b;
    return true;
}

static bool ebytes(enc *e, const void *data, size_t n)
{
    if (!egrow(e, n))
        return false;
    memcpy(&e->b[e->len], data, n);
    e->len += n;
    return true;
}

static bool euint8(enc *e, uint64_t val)
{
    if (val <= 23)
        return eput(e, (uint8_t) val);
    if (val <= 0xFF) {
        if (!eput(e, 24))
            return false;
        uint8_t b = (uint8_t) val;
        return ebytes(e, &b, 1);
    }
    if (val <= 0xFFFF) {
        if (!eput(e, 25))
            return false;
        uint8_t b[2] = {(uint8_t) (val >> 8), (uint8_t) val};
        return ebytes(e, b, 2);
    }
    if (val <= 0xFFFFFFFFULL) {
        if (!eput(e, 26))
            return false;
        uint8_t b[4]
            = {(uint8_t) (val >> 24), (uint8_t) (val >> 16), (uint8_t) (val >> 8), (uint8_t) val};
        return ebytes(e, b, 4);
    }
    if (!eput(e, 27))
        return false;
    uint8_t b[8]
        = {(uint8_t) (val >> 56),
           (uint8_t) (val >> 48),
           (uint8_t) (val >> 40),
           (uint8_t) (val >> 32),
           (uint8_t) (val >> 24),
           (uint8_t) (val >> 16),
           (uint8_t) (val >> 8),
           (uint8_t) val};
    return ebytes(e, b, 8);
}

static bool esz(enc *e, uint64_t len, uint8_t base)
{
    if (len <= 23) {
        return eput(e, base | (uint8_t) len);
    }
    if (len <= 0xFF) {
        if (!eput(e, base | 24))
            return false;
        return eput(e, (uint8_t) len);
    }
    if (len <= 0xFFFF) {
        if (!eput(e, base | 25))
            return false;
        uint8_t b[2] = {(uint8_t) (len >> 8), (uint8_t) len};
        return ebytes(e, b, 2);
    }
    if (len <= 0xFFFFFFFFULL) {
        if (!eput(e, base | 26))
            return false;
        uint8_t b[4]
            = {(uint8_t) (len >> 24), (uint8_t) (len >> 16), (uint8_t) (len >> 8), (uint8_t) len};
        return ebytes(e, b, 4);
    }
    if (!eput(e, base | 27))
        return false;
    uint8_t b[8]
        = {(uint8_t) (len >> 56),
           (uint8_t) (len >> 48),
           (uint8_t) (len >> 40),
           (uint8_t) (len >> 32),
           (uint8_t) (len >> 24),
           (uint8_t) (len >> 16),
           (uint8_t) (len >> 8),
           (uint8_t) len};
    return ebytes(e, b, 8);
}

static bool ebytes_w(enc *e, const uint8_t *data, size_t len)
{
    return esz(e, len, 0x40) && ebytes(e, data, len);
}

static bool ebool(enc *e, bool v)
{
    return eput(e, v ? 0xF5 : 0xF4);
}
static bool enull(enc *e)
{
    return eput(e, 0xF6);
}

static bool evalue(enc *e, value *v)
{
    switch (v->type) {
    case V_STRING: {
        size_t l = strlen(v->str);
        if (!esz(e, l, 0x60))
            return false;
        return ebytes(e, v->str, l);
    }
    case V_UINT:
        return euint8(e, v->u64);
    case V_INT: {
        if (v->i64 >= 0)
            return euint8(e, (uint64_t) v->i64);
        if (!eput(e, 0x20))
            return false; /* major type 1 */
        return euint8(e, (uint64_t) (-(v->i64 + 1)));
    }
    case V_BOOL:
        return ebool(e, v->bval);
    case V_BYTES:
        return ebytes_w(e, v->bytes, v->nkids);
    case V_NULL:
        return enull(e);
    case V_ARRAY: {
        if (!esz(e, v->nkids, 0x80))
            return false;
        for (size_t i = 0; i < v->nkids; i++) {
            if (!evalue(e, v->kids[i]))
                return false;
        }
        return true;
    }
    case V_MAP: {
        size_t pc = v->nkids / 2;
        if (!esz(e, pc, 0xA0))
            return false;
        for (size_t i = 0; i + 1 < v->nkids; i += 2) {
            value *k = v->kids[i], *val = v->kids[i + 1];
            if (!k || k->type != V_STRING || !val)
                return false; /* the protocol only ever uses string keys */
            size_t kl = strlen(k->str);
            if (!esz(e, kl, 0x60))
                return false;
            if (!ebytes(e, k->str, kl))
                return false;
            if (!evalue(e, val))
                return false;
        }
        return true;
    }
    default:
        return false;
    }
}

static uint8_t *encode(value *v, size_t *out_len)
{
    enc e;
    if (!einit(&e))
        return NULL;
    if (!evalue(&e, v)) {
        efree(&e);
        return NULL;
    }
    *out_len = e.len;
    return e.b;
}

/* --- Map helpers --- */

static value *mfind(value *map, const char *key)
{
    if (!map || map->type != V_MAP)
        return NULL;
    for (size_t i = 0; i < map->nkids; i += 2) {
        if (map->kids[i]->type == V_STRING && strcmp(map->kids[i]->str, key) == 0)
            return map->kids[i + 1];
    }
    return NULL;
}

static const char *mstr(value *m, const char *k)
{
    value *v = mfind(m, k);
    return (v && v->type == V_STRING) ? v->str : NULL;
}

static int64_t mint(value *m, const char *k, int64_t def)
{
    value *v = mfind(m, k);
    if (!v)
        return def;
    if (v->type == V_UINT)
        return (int64_t) v->u64;
    if (v->type == V_INT)
        return v->i64;
    if (v->type == V_STRING) {
        char *end;
        int64_t val = strtoll(v->str, &end, 10);
        return (end != v->str && *end == '\0') ? val : def;
    }
    return def;
}

static uint64_t muint(value *m, const char *k, uint64_t def)
{
    value *v = mfind(m, k);
    if (!v)
        return def;
    if (v->type == V_UINT)
        return v->u64;
    if (v->type == V_INT)
        return (uint64_t) v->i64;
    if (v->type == V_STRING) {
        char *end;
        uint64_t val = strtoull(v->str, &end, 10);
        return (*end == '\0') ? val : def;
    }
    return def;
}

static int mkey(value *m, const char *k)
{
    return (int) mint(m, k, 0);
}

/* --- Value constructors --- */

static value *vs(const char *s)
{
    value *v = vnew_x();
    v->type = V_STRING;
    v->str = xstrdup(s);
    return v;
}

static value *vi(int64_t i)
{
    value *v = vnew_x();
    if (i >= 0) {
        v->type = V_UINT;
        v->u64 = (uint64_t) i;
    } else {
        v->type = V_INT;
        v->i64 = i;
    }
    return v;
}

static value *vu(uint64_t u)
{
    value *v = vnew_x();
    v->type = V_UINT;
    v->u64 = u;
    return v;
}

static value *vb(bool b)
{
    value *v = vnew_x();
    v->type = V_BOOL;
    v->bval = b;
    return v;
}

static value *vy(const uint8_t *data, size_t len)
{
    value *v = vnew_x();
    v->type = V_BYTES;
    v->bytes = (uint8_t *) xmalloc(len ? len : 1);
    if (len)
        memcpy(v->bytes, data, len);
    v->nkids = len;
    return v;
}

/* Builds a map from `npairs` (const char *key, value *val) argument pairs.
   Takes ownership of the values. */
static value *mkn(size_t npairs, ...)
{
    value *m = vnew_x();
    m->type = V_MAP;
    m->nkids = npairs * 2;
    m->kids = (value **) xmalloc(m->nkids * sizeof(value *));

    va_list ap;
    va_start(ap, npairs);
    for (size_t i = 0; i < npairs; i++) {
        m->kids[i * 2] = vs(va_arg(ap, const char *));
        m->kids[i * 2 + 1] = va_arg(ap, value *);
    }
    va_end(ap);
    return m;
}

#define mk2(...) mkn(2, __VA_ARGS__)
#define mk3(...) mkn(3, __VA_ARGS__)
#define mk4(...) mkn(4, __VA_ARGS__)
#define mk5(...) mkn(5, __VA_ARGS__)
#define mk7(...) mkn(7, __VA_ARGS__)
#define mk8(...) mkn(8, __VA_ARGS__)

static void mfreekeys(value *m)
{
    if (!m || m->type != V_MAP)
        return;
    for (size_t i = 0; i < m->nkids; i++) {
        if (m->kids[i]) {
            vfree(m->kids[i]);
            m->kids[i] = NULL;
        }
    }
    free(m->kids);
    m->kids = NULL;
    m->nkids = 0;
}
