//go:build !windows

package main

import (
	"fmt"

	"golang.org/x/sys/unix"
)


func isWritable(path string) bool {
	return unix.Access(path, unix.W_OK) == nil
}

func isReadable(path string) bool {
	return unix.Access(path, unix.R_OK) == nil
}

func isExecutable(path string) bool {
	return unix.Access(path, unix.X_OK) == nil
}

func numberOfHardLinks(path string) int {
	var stat unix.Stat_t
	unix.Stat(path, &stat)
	return int(stat.Nlink)
}

func fileId(path string) string {
	var stat unix.Stat_t
	unix.Stat(path, &stat)
	return fmt.Sprintf("%x:%x", stat.Dev, stat.Ino)
}

func freeSpace(path string) uint64 {
	var stat unix.Statfs_t
	unix.Statfs(path, &stat)
	return stat.Bavail * uint64(stat.Bsize)
}
