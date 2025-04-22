//go:build !windows

package main

import (
	"fmt"
	"os/user"
	"strconv"

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

func owner(path string) string {
	uid := strconv.Itoa(unix.Getuid())
	u, _ := user.LookupId(uid)
	return u.Username
}

func ownerId(path string) int {
	return unix.Getuid()
}

func group(path string) string {
	gid := strconv.Itoa(unix.Getgid())
	g, _ := user.LookupGroupId(gid)
	return g.Name
}

func groupId(path string) int {
	return unix.Getgid()
}
