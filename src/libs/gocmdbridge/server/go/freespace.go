//go:build linux || darwin || freebsd

// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"golang.org/x/sys/unix"
)

func freeSpace(path string) uint64 {
	var stat unix.Statfs_t
	unix.Statfs(path, &stat)
	// Cast both: Bavail is uint64 on Linux and Darwin but int64 on FreeBSD,
	// and Bsize varies too.
	return uint64(stat.Bavail) * uint64(stat.Bsize)
}
