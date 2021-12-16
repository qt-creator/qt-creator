/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <signal.h>
#include <string.h>

extern "C" {
int responsibility_spawnattrs_setdisclaim(posix_spawnattr_t attrs, int disclaim)
__attribute__((availability(macos,introduced=10.14),weak_import));
}

#define CHECK(expr) \
    if (int err = (expr)) { \
        fprintf(stderr, "%s: %s", #expr, strerror(err)); \
        exit(err); \
    }

int main(int argc, char *const *argv, char *const *envp)
{
    if (argc < 2) {
        printf("Disclaims all TCC responsibilities for the launched process\n"
               "usage: disclaim <command> [arguments]\n");
        return 1;
    }

    posix_spawnattr_t attr;
    CHECK(posix_spawnattr_init(&attr));

    // Behave as exec
    short flags = POSIX_SPAWN_SETEXEC;

    // Reset signal mask
    sigset_t no_signals;
    sigemptyset(&no_signals);
    CHECK(posix_spawnattr_setsigmask(&attr, &no_signals));
    flags |= POSIX_SPAWN_SETSIGMASK;

    // Reset all signals to their default handlers
    sigset_t all_signals;
    sigfillset(&all_signals);
    CHECK(posix_spawnattr_setsigdefault(&attr, &all_signals));
    flags |= POSIX_SPAWN_SETSIGDEF;

    CHECK(posix_spawnattr_setflags(&attr, flags));

    if (@available(macOS 10.14, *)) {
        // Disclaim TCC responsibilities
        if (responsibility_spawnattrs_setdisclaim)
            CHECK(responsibility_spawnattrs_setdisclaim(&attr, 1));
    }

    pid_t pid = 0;
    int err = posix_spawnp(&pid, argv[1], nullptr, &attr, argv + 1, envp);
    fprintf(stderr, "Failed to launch %s: %s", argv[1], strerror(err));

    posix_spawnattr_destroy(&attr);
    return err;
}
