// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Find command handler and output batcher. The directory walk itself is
// platform specific and included below.
// Included by cmdbridge.c - do not compile separately.

/* ================================================================== */
/*  Find -- recursive directory walk                                  */
/* ================================================================== */

/* File filter flags (match Go find.go constants) */
#define FF_DIRS 0x001
#define FF_FILES 0x002
#define FF_DRIVES 0x004
#define FF_NOSYMLINKS 0x008
#define FF_TYPEMASK 0x00f
#define FF_READABLE 0x010
#define FF_WRITABLE 0x020
#define FF_EXECUTABLE 0x040
#define FF_MODIFIED 0x080
#define FF_HIDDEN 0x100
#define FF_SYSTEM 0x200
#define FF_ALLDIRS 0x400
#define FF_CASESENSITIVE 0x800
#define FF_NODOT 0x2000
#define FF_NODOTDOT 0x4000
/* QDir::NoFilter, which Go spells NoFilter and checks for explicitly. Reading
   it as a bit mask instead turns every filter on, which drops the plain files
   it is meant to let through. An empty mask means the same thing here. */
#define FF_NO_FILTER (-1)

/* Whether `ffilt` asks for any filtering at all. */
static bool find_filtering(int ffilt)
{
    return ffilt != 0 && ffilt != FF_NO_FILTER;
}

/* ================================================================== */
/*  Find output batcher (matches Go ChannelWriter batching, >1KB flush) */
/* ================================================================== */

/* Messages are collected until the accumulated size passes the threshold and
   are then written out back to back. Each message still goes over the wire as
   its own magic-marker packet: the client decodes exactly one CBOR value per
   packet, so concatenating several into one packet would silently drop all but
   the first. This mirrors Go's ChannelWriter, which batches channel sends but
   still emits one packet per message. */
#define FIND_BATCH_THRESHOLD 1024
#define FIND_BATCH_MAX_MSGS 128

typedef struct
{
    uint8_t *data;
    size_t len;
} find_msg_t;

typedef struct
{
    find_msg_t msgs[FIND_BATCH_MAX_MSGS];
    size_t count;
    size_t bytes;
} find_batcher_t;

static _Thread_local find_batcher_t find_batch;

static void find_batch_init(void)
{
    find_batch.count = 0;
    find_batch.bytes = 0;
}

static void find_batch_flush(void)
{
    if (find_batch.count == 0)
        return;
    /* One lock acquisition for the whole batch, but one packet per message. */
    pthread_mutex_lock(&output_mutex);
    for (size_t i = 0; i < find_batch.count; i++) {
        send_pkt_locked(find_batch.msgs[i].data, find_batch.msgs[i].len);
        free(find_batch.msgs[i].data);
    }
    pthread_mutex_unlock(&output_mutex);
    find_batch.count = 0;
    find_batch.bytes = 0;
}

/* Takes ownership of `data`. */
static void find_batch_add(uint8_t *data, size_t len)
{
    if (find_batch.count == FIND_BATCH_MAX_MSGS)
        find_batch_flush();

    find_batch.msgs[find_batch.count].data = data;
    find_batch.msgs[find_batch.count].len = len;
    find_batch.count++;
    find_batch.bytes += len;

    if (find_batch.bytes > FIND_BATCH_THRESHOLD)
        find_batch_flush();
}

static void find_batch_free(void)
{
    find_batch_flush();
}

/* Joins `dir` and `name` with exactly one separator.

   Go built these paths with filepath.Join(), which collapses the doubled
   separator you would otherwise get for a directory that already ends in one.
   It matters: listing "/" produced "//bin", and the client reads a leading
   double slash as a host-qualified path, so FilePath::fromUserInput() returns
   an empty path and every entry in the listing collapses onto the device root.
   That is what made a container's root look like a list of identical files. */
static void find_join(char *out, size_t cap, const char *dir, const char *name, char sep)
{
    size_t len = strlen(dir);
    while (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
        len--;
    snprintf(out, cap, "%.*s%c%s", (int) len, dir, sep, name);
}

/* Queues one finddata entry. Both walks report through this, so they cannot
   disagree about the fields or their units - `mtime` is seconds since the Unix
   epoch, which is what the client reads it as. */
static void find_emit(int id, const char *path, int64_t size, uint32_t mode, bool is_dir,
                      int64_t mtime)
{
    value *m = mk7(
        "Type",
        vs("finddata"),
        "Id",
        vi(id),
        "Path",
        vs(path),
        "Size",
        vi(size),
        "Mode",
        vu(mode),
        "IsDir",
        vb(is_dir),
        "ModTime",
        vi(mtime));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c)
        find_batch_add(c, l);
    mfreekeys(m);
    vfree(m);
}

/* ------------------------------------------------------------------
 * The directory walk for this platform, which reports every entry that
 * passes the filters through find_batch_add() above:
 *
 *   void find_walk(const char *dir, const char **filters, int nfilt,
 *                  int ffilt, int iflags, int id)
 *   bool find_list_drives(const char *dir, int ffilt, int id)
 *       Windows lists the available drives when asked for the root;
 *       everywhere else this does nothing and returns false.
 * ------------------------------------------------------------------ */

#include "find_posix.c"
#include "find_win.c"

static void h_find(value *cmd)
{
    value *find_map = mfind(cmd, "Find");
    const char *dir = find_map ? mstr(find_map, "Directory") : mstr(cmd, "Directory");
    if (!dir) {
        send_err(mkey(cmd, "Id"), "missing Directory");
        return;
    }
    int id = mkey(cmd, "Id");
    register_cancel(id);
    find_batch_init();
    int ffilt = (int) muint(find_map ? find_map : cmd, "FileFilters", 0);
    int iflags = (int) muint(find_map ? find_map : cmd, "IteratorFlags", 0);
    value *nf = find_map ? mfind(find_map, "NameFilters") : mfind(cmd, "NameFilters");

    /* On Windows an empty root lists the drives instead of walking. */
    if (find_list_drives(dir, ffilt, id))
        return;

    const char **filters = NULL;
    int nfilt = 0;
    if (nf && nf->type == V_ARRAY) {
        nfilt = (int) nf->nkids;
        filters = (const char **) xmalloc((size_t) nfilt * sizeof(const char *));
        memset(filters, 0, (size_t) nfilt * sizeof(const char *));
        for (int i = 0; i < nfilt; i++)
            if (nf->kids[i]->type == V_STRING)
                filters[i] = nf->kids[i]->str;
    }
    find_batch_flush();
    find_walk(dir, filters, nfilt, ffilt, iflags, id);
    clear_cancelled(id);
    free(filters);
    find_batch_flush();
    value *m = mk2("Type", vs("findend"), "Id", vi(id));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        find_batch_add(c, l);
    }
    mfreekeys(m);
    vfree(m);
    find_batch_flush();
    find_batch_free();
}
