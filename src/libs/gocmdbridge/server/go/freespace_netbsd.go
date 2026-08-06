// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"golang.org/x/sys/unix"
)

// NetBSD has no statfs(), only the POSIX statvfs(), whose Bavail counts
// Frsize units rather than Bsize ones.
func freeSpace(path string) uint64 {
	var stat unix.Statvfs_t
	unix.Statvfs(path, &stat)
	return uint64(stat.Bavail) * uint64(stat.Frsize)
}
