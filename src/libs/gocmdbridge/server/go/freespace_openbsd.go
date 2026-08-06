// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"golang.org/x/sys/unix"
)

// OpenBSD spells the statfs fields with the struct's C prefix kept on.
func freeSpace(path string) uint64 {
	var stat unix.Statfs_t
	unix.Statfs(path, &stat)
	return uint64(stat.F_bavail) * uint64(stat.F_bsize)
}
