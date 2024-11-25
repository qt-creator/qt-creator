package main

import (
	"io"
	"os"

	"github.com/fxamacker/cbor/v2"
)

type readfile struct {
	Path string
	Limit int64
	Offset int64
}

type readfiledata struct {
	Type string
	Id int
	Contents []byte
}

type readfiledone struct {
	Type string
	Id int
}

func processReadFile(cmd command, out chan<- []byte) {
	file, seekErr := os.Open(cmd.ReadFile.Path)
	if seekErr != nil {
		sendError(out, cmd, seekErr)
		return
	}

	size, seekErr := file.Seek(0, io.SeekEnd)
	if seekErr != nil {
		sendError(out, cmd, seekErr)
		return
	}

	_, seekErr = file.Seek(cmd.ReadFile.Offset, 0)
	if seekErr != nil {
		sendError(out, cmd, seekErr)
		return
	}

	readSize := cmd.ReadFile.Limit
	if readSize == -1 {
		readSize = size - cmd.ReadFile.Offset
	}

	for {
		toRead := min(readSize, 4096)
		contents := make([]byte, toRead)
		bytesRead, readErr := file.Read(contents)

		if readErr != nil && readErr != io.EOF {
			sendError(out, cmd, readErr)
			return
		}

		readSize -= int64(bytesRead)

		result, _ := cbor.Marshal(readfiledata{
			Type:     "readfiledata",
			Id:       cmd.Id,
			Contents: contents,
		})
		out <- result

		if readErr == io.EOF || readSize == 0 {
			result, _:= cbor.Marshal(readfiledone{
				Type: "readfiledone",
				Id: cmd.Id,
			})
			out <- result
			return
		}
	}
}
