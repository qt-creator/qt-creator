// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Command cancellation: the client can ask for a long-running command to stop
// by sending "cancel" with its Id. Used by the exec and find handlers, which
// poll is_cancelled() while they work.
// Included by cmdbridge.c — do not compile separately.

#include <pthread.h>

/* ================================================================== */
/*  Cancellation registry, keyed by command Id                        */
/* ================================================================== */

/* Two sets: the commands that can be cancelled, and those that have been.
   Both grow as needed -- a fixed table would silently stop accepting
   registrations once full, and cancelling would then quietly do nothing. */
static imap tracked_ids;
static imap cancelled_ids;
static pthread_mutex_t cancel_mutex = PTHREAD_MUTEX_INITIALIZER;

static void cancel_init(void)
{
    imap_init(&tracked_ids);
    imap_init(&cancelled_ids);
}

static void register_cancel(int id)
{
    pthread_mutex_lock(&cancel_mutex);
    if (!imap_put(&tracked_ids, id, NULL))
        fprintf(stderr, "cmdbridge: cannot track command %d for cancellation\n", id);
    pthread_mutex_unlock(&cancel_mutex);
}

static bool is_cancelled(int id)
{
    pthread_mutex_lock(&cancel_mutex);
    bool cancelled = imap_contains(&cancelled_ids, id);
    pthread_mutex_unlock(&cancel_mutex);
    return cancelled;
}

static void clear_cancelled(int id)
{
    pthread_mutex_lock(&cancel_mutex);
    imap_remove(&tracked_ids, id);
    imap_remove(&cancelled_ids, id);
    pthread_mutex_unlock(&cancel_mutex);
}

static void mark_cancelled(int id)
{
    pthread_mutex_lock(&cancel_mutex);
    /* Only commands that announced themselves as cancellable; a cancel for
       anything else is not something we can act on. */
    if (imap_contains(&tracked_ids, id))
        imap_put(&cancelled_ids, id, NULL);
    pthread_mutex_unlock(&cancel_mutex);
}
