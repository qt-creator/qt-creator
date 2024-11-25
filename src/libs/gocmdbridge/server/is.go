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
	fileInfo, err := os.Stat(Cmd.Is.Path)

	if err != nil {
		return false, nil
	}

	if (Cmd.Is.Check == ReadableDir || Cmd.Is.Check == WritableDir || Cmd.Is.Check == Dir) {
		if !fileInfo.IsDir() {
			return false, nil
		}
	}
	if (Cmd.Is.Check == Symlink) {
		return (fileInfo.Mode() & os.ModeSymlink != 0), nil;
	}
	if (Cmd.Is.Check == Exists) {
		return true, nil
	}
	if (Cmd.Is.Check == File) {
		return (!fileInfo.IsDir()), nil
	}

	switch(Cmd.Is.Check) {
	case ReadableFile:
		return isReadable(Cmd.Is.Path), nil
	case WritableFile:
		return isWritable(Cmd.Is.Path), nil
	case ExecutableFile:
		return isExecutable(Cmd.Is.Path), nil
	case ReadableDir:
		return isReadable(Cmd.Is.Path), nil
	case WritableDir:
		return isWritable(Cmd.Is.Path), nil
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
