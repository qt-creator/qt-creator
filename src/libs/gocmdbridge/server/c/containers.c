// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// The two growable containers the bridge needs, so that nothing is capped by a
// compile-time constant: a FIFO of pointers and a map keyed by int.
//
// These are written here rather than vendored (khash.h, stb_ds.h and friends
// would all do the job and are permissively licensed) because it is ~200 lines,
// because every vendored line would have to build for all seven zig targets
// including musl and mingw, and because third party code in this repository
// belongs under src/libs/3rdparty with a qt_attribution.json entry -- a lot of
// process for two containers.
//
// None of this is thread safe; callers hold the lock that already protects the
// state the container is part of.
//
// Included by cmdbridge.c -- do not compile separately.

/* ================================================================== */
/*  FIFO of pointers                                                  */
/* ================================================================== */

typedef struct
{
    void **items;
    size_t cap;   /* allocated slots */
    size_t head;  /* index of the oldest item */
    size_t count; /* items currently held */
} fifo;

static void fifo_init(fifo *f)
{
    f->items = NULL;
    f->cap = 0;
    f->head = 0;
    f->count = 0;
}

static void fifo_free(fifo *f)
{
    free(f->items);
    fifo_init(f);
}

/* Doubles the ring and unwraps it, so the items sit in order from index 0. */
static bool fifo_grow(fifo *f)
{
    size_t ncap = f->cap ? f->cap * 2 : 16;
    if (ncap < f->cap) /* overflow */
        return false;
    void **ni = (void **) malloc(ncap * sizeof(void *));
    if (!ni)
        return false;
    for (size_t i = 0; i < f->count; i++)
        ni[i] = f->items[(f->head + i) % f->cap];
    free(f->items);
    f->items = ni;
    f->cap = ncap;
    f->head = 0;
    return true;
}

/* Appends `item`. Returns false only when out of memory. */
static bool fifo_push(fifo *f, void *item)
{
    if (f->count == f->cap && !fifo_grow(f))
        return false;
    f->items[(f->head + f->count) % f->cap] = item;
    f->count++;
    return true;
}

/* Removes and returns the oldest item, or NULL when empty. */
static void *fifo_pop(fifo *f)
{
    if (f->count == 0)
        return NULL;
    void *item = f->items[f->head];
    f->head = (f->head + 1) % f->cap;
    f->count--;
    return item;
}

/* ================================================================== */
/*  Map from int key to void *                                        */
/* ================================================================== */

/* Open addressing with linear probing. Deleted slots are tombstoned and
   reclaimed by the next rehash, which keeps removal O(1) and lookups correct
   without moving entries around. */

typedef enum { IMAP_EMPTY = 0, IMAP_USED, IMAP_DEAD } imap_state;

typedef struct
{
    int key;
    void *val;
    uint8_t state;
} imap_slot;

typedef struct
{
    imap_slot *slots;
    size_t cap;  /* always a power of two, or 0 */
    size_t used; /* live entries */
    size_t dead; /* tombstones */
} imap;

/* Values small enough to be an int can live in the value slot directly rather
   than needing an allocation of their own. */
#define IMAP_INT(v) ((void *) (intptr_t) (v))
#define IMAP_TO_INT(p) ((int) (intptr_t) (p))

static void imap_init(imap *m)
{
    m->slots = NULL;
    m->cap = 0;
    m->used = 0;
    m->dead = 0;
}

static void imap_free(imap *m)
{
    free(m->slots);
    imap_init(m);
}

static size_t imap_hash(int key, size_t cap)
{
    /* Knuth's multiplicative hash; ids are handed out sequentially, so the
       low bits alone would cluster badly. */
    uint32_t h = (uint32_t) key * 2654435761u;
    return (size_t) h & (cap - 1);
}

static size_t imap_count(const imap *m)
{
    return m->used;
}

/* Address of the value for `key`, or NULL if there is no such entry. Stays
   valid until the next imap_put(). */
static void **imap_find(const imap *m, int key)
{
    if (m->cap == 0)
        return NULL;
    size_t i = imap_hash(key, m->cap);
    for (size_t probe = 0; probe < m->cap; probe++) {
        imap_slot *s = &m->slots[i];
        if (s->state == IMAP_EMPTY)
            return NULL;
        if (s->state == IMAP_USED && s->key == key)
            return &s->val;
        i = (i + 1) & (m->cap - 1);
    }
    return NULL;
}

static bool imap_contains(const imap *m, int key)
{
    return imap_find(m, key) != NULL;
}

/* Value for `key`, or NULL if absent. Callers that store NULL values use
   imap_contains() instead. */
static void *imap_get(const imap *m, int key)
{
    void **v = imap_find(m, key);
    return v ? *v : NULL;
}

static bool imap_resize(imap *m, size_t ncap)
{
    imap_slot *ns = (imap_slot *) calloc(ncap, sizeof(imap_slot));
    if (!ns)
        return false;
    for (size_t i = 0; i < m->cap; i++) {
        if (m->slots[i].state != IMAP_USED)
            continue;
        size_t j = imap_hash(m->slots[i].key, ncap);
        while (ns[j].state == IMAP_USED)
            j = (j + 1) & (ncap - 1);
        ns[j] = m->slots[i];
    }
    free(m->slots);
    m->slots = ns;
    m->cap = ncap;
    m->dead = 0;
    return true;
}

/* Inserts or replaces the entry for `key`. Returns false only when out of
   memory. */
static bool imap_put(imap *m, int key, void *val)
{
    void **existing = imap_find(m, key);
    if (existing) {
        *existing = val;
        return true;
    }

    /* Keep the load factor under 0.7, counting tombstones: they lengthen
       probes just as live entries do. */
    if ((m->used + m->dead + 1) * 10 >= m->cap * 7) {
        size_t ncap = m->cap ? m->cap * 2 : 16;
        while ((m->used + 1) * 10 >= ncap * 7)
            ncap *= 2;
        if (!imap_resize(m, ncap))
            return false;
    }

    size_t i = imap_hash(key, m->cap);
    while (m->slots[i].state == IMAP_USED)
        i = (i + 1) & (m->cap - 1);
    if (m->slots[i].state == IMAP_DEAD)
        m->dead--;
    m->slots[i].key = key;
    m->slots[i].val = val;
    m->slots[i].state = IMAP_USED;
    m->used++;
    return true;
}

/* Removes the entry for `key`. Returns whether there was one. */
static bool imap_remove(imap *m, int key)
{
    if (m->cap == 0)
        return false;
    size_t i = imap_hash(key, m->cap);
    for (size_t probe = 0; probe < m->cap; probe++) {
        imap_slot *s = &m->slots[i];
        if (s->state == IMAP_EMPTY)
            return false;
        if (s->state == IMAP_USED && s->key == key) {
            s->state = IMAP_DEAD;
            s->val = NULL;
            m->used--;
            m->dead++;
            return true;
        }
        i = (i + 1) & (m->cap - 1);
    }
    return false;
}

/* Walks every entry in unspecified order. Start with *it = 0 and call until it
   returns false. Do not insert while iterating. */
static bool imap_next(const imap *m, size_t *it, int *key, void **val)
{
    for (; *it < m->cap; (*it)++) {
        if (m->slots[*it].state != IMAP_USED)
            continue;
        if (key)
            *key = m->slots[*it].key;
        if (val)
            *val = m->slots[*it].val;
        (*it)++;
        return true;
    }
    return false;
}
