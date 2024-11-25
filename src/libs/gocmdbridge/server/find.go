package main

import (
	"io/fs"
	"os"
	"path/filepath"
	"time"

	"github.com/fxamacker/cbor/v2"
)

const (
	Dirs       = 0x001
	Files      = 0x002
	Drives     = 0x004
	NoSymLinks = 0x008
	AllEntries = Dirs | Files | Drives
	TypeMask   = 0x00f

	Readable       = 0x010
	Writable       = 0x020
	Executable     = 0x040
	PermissionMask = 0x070

	Modified = 0x080
	Hidden   = 0x100
	System   = 0x200

	AccessMask = 0x3F0

	AllDirs        = 0x400
	CaseSensitive  = 0x800
	NoDot          = 0x2000
	NoDotDot       = 0x4000
	NoDotAndDotDot = NoDot | NoDotDot

	NoFilter = -1
)

const (
	NoIteratorFlags = 0x0
	FollowSymlinks  = 0x1
	Subdirectories  = 0x2
)

type find struct {
	Directory     string
	NameFilters   []string
	FileFilters   int
	IteratorFlags int
}

type finddata struct {
	Type    string
	Id      int
	Path    string
	Size    int64
	Mode    fs.FileMode
	IsDir   bool
	ModTime time.Time
}

func processFind(cmd command, out chan<- []byte) {
	writer := NewChannelWriter(out)

	fileSystem := os.DirFS(cmd.Find.Directory)

	fs.WalkDir(fileSystem, ".", func(path string, dirEntry fs.DirEntry, err error) error {
		if err != nil {
			return nil
		}

		if path == "." {
			return nil
		}

		var skipdir error = nil

		fullPath := filepath.Join(cmd.Find.Directory,path)


		if dirEntry.IsDir() && path != "." && (cmd.Find.IteratorFlags&Subdirectories == 0)  {
			skipdir = filepath.SkipDir
		}

		info, err := dirEntry.Info()
		if err != nil {
			return nil
		}

		if info.Mode()&fs.ModeSymlink != 0 {
			linkedpath, err := filepath.EvalSymlinks(fullPath)
			if err == nil {
				newinfo, err := os.Stat(linkedpath)
				if err == nil {
					info = newinfo
				}
			}
		}

		// TODO: fs.WalkDir does not follow symlinks, so we need to implement this ourselves
		//if (cmd.Find.IteratorFlags&FollowSymlinks == 0) && (info.Mode()&fs.ModeSymlink != 0) && info.IsDir() {
		//	skipdir = filepath.SkipDir
		//}

		if cmd.Find.FileFilters != NoFilter {
			if (cmd.Find.FileFilters&Drives) == 0 && ((info.Mode()&fs.ModeDevice != 0) || (info.Mode()&fs.ModeCharDevice != 0)) {
				return skipdir
			}
			if (cmd.Find.FileFilters&Dirs) == 0 && info.IsDir() {
				return skipdir
			}
			if (cmd.Find.FileFilters&Files) == 0 && !info.IsDir() {
				return skipdir
			}
			if (cmd.Find.FileFilters&NoSymLinks) != 0 && info.Mode()&fs.ModeSymlink != 0 {
				return skipdir
			}
			if (cmd.Find.FileFilters&Readable) != 0 && !isReadable(fullPath) {
				return skipdir
			}
			if (cmd.Find.FileFilters&Writable) != 0 && !isWritable(fullPath) {
				return skipdir
			}
			if (cmd.Find.FileFilters&Executable) != 0 && !isExecutable(fullPath) {
				return skipdir
			}
		}
		hasMatch := true

		if len(cmd.Find.NameFilters) > 0 {
			hasMatch = false
			for _, filter := range cmd.Find.NameFilters {
				match, _ := filepath.Match(filter, info.Name());
				if match {
					hasMatch = true
					break
				}
			}
		}

		if !hasMatch {
			return skipdir
		}


		result, err := cbor.Marshal(finddata{
			Type:    "finddata",
			Id:      cmd.Id,
			Path:    filepath.Join(cmd.Find.Directory,path),
			Size:    info.Size(),
			Mode:    info.Mode(),
			IsDir:   info.IsDir(),
			ModTime: info.ModTime(),
		})

		if err == nil {
			writer.Write(result)
		}

		return skipdir
	})

	result, _ := cbor.Marshal(finddata{
		Type: "findend",
		Id:   cmd.Id,
	})

	writer.Write(result)
	writer.Flush()
}
