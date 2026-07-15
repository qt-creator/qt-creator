package main

import (
	"os"

	"github.com/fxamacker/cbor/v2"
)

type writefile struct {
	Path     string
	Contents []byte
}

type writefileresult struct {
	Type string
	Id   int
	WrittenBytes uint64
}

func processWriteFile(cmd command, out chan<- []byte) {
	err := os.WriteFile(cmd.WriteFile.Path, cmd.WriteFile.Contents, 0644)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(writefileresult{
		Type: "writefileresult",
		Id:   cmd.Id,
		WrittenBytes: uint64(len(cmd.WriteFile.Contents)),
	})
	out <- result
}
