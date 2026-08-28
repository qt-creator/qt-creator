package main

import (
	"os"

	"github.com/fxamacker/cbor/v2"
)


const (
	ReadableFile = 0
	WritableFile = 1
	ExecutableFile = 2
	ReadableDir = 3
	WritableDir = 4
	File = 5
	Dir = 6
	Exists = 7
	Symlink = 8
)

type is struct {
	Path string
	Check int
}

type isresult struct {
	Type string
	Id int
	Result bool
}

func check(Cmd command) (bool, error) {
	return checkPath(Cmd.Is.Path, Cmd.Is.Check)
}

func checkPath(path string, wanted int) (bool, error) {
	fileInfo, err := os.Stat(path)

	if err != nil {
		return false, nil
	}

	if (wanted == ReadableDir || wanted == WritableDir || wanted == Dir) {
		if !fileInfo.IsDir() {
			return false, nil
		}
	}
	if (wanted == ReadableFile || wanted == WritableFile || wanted == ExecutableFile) {
		if fileInfo.IsDir() {
			return false, nil
		}
	}
	if (wanted == Symlink) {
		return (fileInfo.Mode() & os.ModeSymlink != 0), nil;
	}
	if (wanted == Exists) {
		return true, nil
	}
	if (wanted == File) {
		return (!fileInfo.IsDir()), nil
	}

	switch(wanted) {
	case ReadableFile:
		return isReadable(path), nil
	case WritableFile:
		return isWritable(path), nil
	case ExecutableFile:
		return isExecutable(path), nil
	case ReadableDir:
		return isReadable(path), nil
	case WritableDir:
		return isWritable(path), nil
	}

	return true, nil
}

func processIs(cmd command, out chan<- []byte) {
	r, err := check(cmd)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(isresult{
		Type:   "isresult",
		Id:     cmd.Id,
		Result: r,
	})
	out <- result
}
