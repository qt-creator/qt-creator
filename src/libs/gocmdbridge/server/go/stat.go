package main

import (
	"os"
	"time"

	"github.com/fxamacker/cbor/v2"
)

type stat struct {
	Path string
}

type statresult struct {
	Type         string
	Id           int
	Size         int64
	Mode         uint32
	UserMode     uint32
	ModTime      time.Time
	IsDir        bool
	NumHardLinks int
}

func processStat(cmd command, out chan<- []byte) {
	stat, err := os.Lstat(cmd.Stat.Path)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	var usermode uint32
	if isReadable(cmd.Stat.Path) {
		usermode |= 0x400
	}
	if isWritable(cmd.Stat.Path) {
		usermode |= 0x200
	}
	if isExecutable(cmd.Stat.Path) {
		usermode |= 0x100
	}


	data, _ := cbor.Marshal(statresult{
		Type:         "statresult",
		Id:           cmd.Id,
		Size:         stat.Size(),
		Mode:         uint32(stat.Mode()),
		UserMode:     usermode,
		ModTime:      stat.ModTime(),
		IsDir:        stat.IsDir(),
		NumHardLinks: numberOfHardLinks(cmd.Stat.Path),
	})

	out <- data
}
