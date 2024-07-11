package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"reflect"
	"runtime"
	"sync"
	"syscall"

	"github.com/fxamacker/cbor/v2"
)

// Can be changed from CMakeLists.txt (-X main.MagicPacketMarker=...)
var MagicPacketMarker = "-magic-packet-marker-"

var globalWaitGroup sync.WaitGroup

type command struct {
	Type string
	Id   int

	Stat stat
	Exec execute
	Find find
	Is   is
	ReadFile readfile
	WriteFile writefile

	Path string

	Error string

	CopyFile copyfile
	RenameFile renamefile
	SetPermissions setpermissions

	Signal signal
}

type errorresult struct {
	Type  string
	Id    int
	Error string
	ErrorType string
}

type readlinkresult struct {
	Type   string
	Id     int
	Target string
}

type fileidresult struct {
	Type string
	Id   int
	FileId string
}

type freespaceresult struct {
	Type string
	Id   int
	FreeSpace uint64
}

type voidresult struct {
	Type string
	Id   int
}

type environment struct {
	Type string
	Id int
	OsType string
	Env  []string
}

type copyfile struct {
	Source string
	Target string
}

type renamefile struct {
	Source string
	Target string
}

type setpermissions struct {
	Path string
	Mode uint32
}

type createtempfileresult struct {
	Type string
	Id   int
	Path string
}

type signal struct {
	Type string
	Id int
	Pid int
	Signal string
}

type signalsuccess struct {
	Type string
	Id int
}

func readPacket(decoder *cbor.Decoder) (*command, error) {
	cmd := new(command)
	err := decoder.Decode(&cmd)
	return cmd, err
}

func sendError(out chan<- []byte, cmd command, err error) {
	errMsg := err.Error()
	errType := reflect.TypeOf(err).Elem().Name()
	if e, ok := err.(*os.PathError); ok {
		errMsg = e.Err.Error()
		errType = reflect.TypeOf(e.Err).Name()

		if erno, ok := e.Err.(syscall.Errno); ok {
			if erno == syscall.ENOENT {
				errType = "ENOENT"
			}
		}
	}
	result, _ := cbor.Marshal(errorresult{
		Type:  "error",
		Id:    cmd.Id,
		Error: errMsg,
		ErrorType: errType,
	})
	out <- result
}

func processReadLink(cmd command, out chan<- []byte) {
	linkTarget, err := os.Readlink(cmd.Path)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(readlinkresult{
		Type:   "readlinkresult",
		Id:     cmd.Id,
		Target: linkTarget,
	})
	out <- result
}

func processFileId(cmd command, out chan<- []byte) {
	result, _ := cbor.Marshal(fileidresult{
		Type:         "fileidresult",
		Id:           cmd.Id,
		FileId:       fileId(cmd.Path),
	})
	out <- result
}

func processFreespace(cmd command, out chan<- []byte) {
	result, _ := cbor.Marshal(freespaceresult{
		Type:         "freespaceresult",
		Id:           cmd.Id,
		FreeSpace:    freeSpace(cmd.Path),
	})
	out <- result
}

func processRemove(cmd command, out chan<- []byte) {
	err := os.Remove(cmd.Path)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "removeresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processRemoveAll(cmd command, out chan<- []byte) {
	err := os.RemoveAll(cmd.Path)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "removeallresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processEnsureExistingFile(cmd command, out chan<- []byte) {
	_, err := os.Stat(cmd.Path)
	if err != nil {
		file, err := os.Create(cmd.Path)
		if err != nil {
			sendError(out, cmd, err)
			return
		}
		file.Close()
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "ensureexistingfileresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processCreateDir(cmd command, out chan<- []byte) {
	err := os.MkdirAll(cmd.Path, 0755)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "createdirresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processCopyFile(cmd command, out chan<- []byte) {
	src, err := os.Open(cmd.CopyFile.Source)
	if err != nil {
		sendError(out, cmd, err)
		return
	}
	defer src.Close()

	dst, err := os.Create(cmd.CopyFile.Target)
	if err != nil {
		sendError(out, cmd, err)
		return
	}
	defer dst.Close()

	_, err = io.Copy(dst, src)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "copyfileresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processRenameFile(cmd command, out chan<- []byte) {
	err := os.Rename(cmd.RenameFile.Source, cmd.RenameFile.Target)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "renamefileresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processCreateTempFile(cmd command, out chan<- []byte) {
	dir := cmd.Path
	template := ""

	if _, err := os.Stat(cmd.Path); os.IsNotExist(err) {
		dir = filepath.Dir(cmd.Path)
		template = filepath.Base(cmd.Path)
	}

	file, err := os.CreateTemp(dir, template)
	if err != nil {
		sendError(out, cmd, err)
		return
	}
	file.Close()

	result, _ := cbor.Marshal(createtempfileresult{
		Type: "createtempfileresult",
		Id:   cmd.Id,
		Path: file.Name(),
	})
	out <- result
}

func processSetPermissions(cmd command, out chan<- []byte) {
	err := os.Chmod(cmd.SetPermissions.Path, os.FileMode(cmd.SetPermissions.Mode))
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	result, _ := cbor.Marshal(voidresult{
		Type: "setpermissionsresult",
		Id:   cmd.Id,
	})
	out <- result
}

func processSignal(cmd command, out chan<- []byte) {
	proc, err := os.FindProcess(cmd.Signal.Pid)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	switch(cmd.Signal.Signal) {
	case "terminate":
		err = proc.Signal(syscall.SIGTERM)
	case "kill":
		err = proc.Kill()
	case "interrupt":
		err = proc.Signal(syscall.SIGINT)
	}

	if err != nil {
		sendError(out, cmd, err)
		return
	}

	data, _ := cbor.Marshal(signalsuccess{
		Type: "signalsuccess",
		Id: cmd.Id,
	})
	out <- data
}


func processCommand(watcher *WatcherHandler, cmd command, out chan<- []byte) {
	defer globalWaitGroup.Done()

	switch cmd.Type {
	case "copyfile":
		processCopyFile(cmd, out)
	case "createdir":
		processCreateDir(cmd, out)
	case "createtempfile":
		processCreateTempFile(cmd, out)
	case "ensureexistingfile":
		processEnsureExistingFile(cmd, out)
	case "exec":
		processExec(cmd, out)
	case "fileid":
		processFileId(cmd, out)
	case "find":
		processFind(cmd, out)
	case "freespace":
		processFreespace(cmd, out)
	case "is":
		processIs(cmd, out)
	case "signal":
		processSignal(cmd, out)
	case "readfile":
		processReadFile(cmd, out)
	case "readlink":
		processReadLink(cmd, out)
	case "remove":
		processRemove(cmd, out)
	case "removeall":
		processRemoveAll(cmd, out)
	case "renamefile":
		processRenameFile(cmd, out)
	case "setpermissions":
		processSetPermissions(cmd, out)
	case "stat":
		processStat(cmd, out)
	case "stopwatch":
		watcher.processStop(cmd, out)
	case "watch":
		watcher.processAdd(cmd, out)
	case "writefile":
		processWriteFile(cmd, out)
	case "error":
		result, _ := cbor.Marshal(errorresult{
			Type:  "error",
			Id:    cmd.Id,
			Error: cmd.Error,
		})
		out <- result
	case "exit":
		os.Exit(0)
	}
}


func sendEnvironment(out chan<- []byte) {
	env := os.Environ()
	result, _ := cbor.Marshal(environment{
		Type: "environment",
		Id: -1,
		OsType: runtime.GOOS,
		Env:  env,
	})
	out <- result
}

func executor(watcher *WatcherHandler, commands <-chan command, out chan<- []byte) {
	sendEnvironment(out)

	for cmd := range commands {
		globalWaitGroup.Add(1)
		go processCommand(watcher, cmd, out)
	}
}

func outputter(out <-chan []byte) {
	writer := bufio.NewWriter(os.Stderr)
	for data := range out {
		if len(data) == 0 {
			break
		}
		writer.Write([]byte(MagicPacketMarker))
		binary.Write(writer, binary.BigEndian, uint32(len(data)))
		writer.Write(data)
		writer.Flush()
	}
}

func prettyOutputter(out <-chan []byte) {
	for data := range out {
		if len(data) == 0 {
			break
		}

		reader := bytes.NewReader(data)
		// As long as reader has data ...
		decoder := cbor.NewDecoder(reader)

		for reader.Len() > 0 {
			var result errorresult;
			err := decoder.Decode(&result)
			if err != nil {
				fmt.Fprint(os.Stderr, err)
				continue
			}
			fmt.Fprint(os.Stderr, result.Type, " ", result.Error," ",reader.Len(), "\n")
		}
	}
}

func writeMain(out *bufio.Writer) {
	// This can be used to quickly test commands. Start with "--test" flag.
	encoder := cbor.NewEncoder(out)

	//encoder.Encode(command{
	//	Id:   1,
	//	Type: "find",
	//	Find: find{Directory: "/tmp/"},
	//})

	encoder.Encode(command{
		Id:   1,
		Type: "stat",
		Stat: stat{Path: "/Users/Users"},
	})

	// /Users/mtillmanns/tmp/untitled/CMakeLists.txt.user.df20d15
	encoder.Encode(command{
		Id:   1,
		Type: "readfile",
		ReadFile: readfile{Path: "/Users/mtillmanns/tmp/untitled/CMakeLists.txt.user.df20d15", Offset: 0, Limit: -1},
	})
	// encoder.Encode(command{
	// 	Id: 1,
	// 	Type: "watch",
	// 	Path: "/private/tmp",
	// })

	out.Flush()
}

func readMain(test bool) {
	commandChannel := make(chan command)
	outputChannel := make(chan []byte)
	watcher := NewWatcherHandler()

	var outputWG sync.WaitGroup
	outputWG.Add(1)
	go func() {
		defer outputWG.Done()
		if test {
			prettyOutputter(outputChannel)
		} else {
			outputter(outputChannel)
		}
	}()

	globalWaitGroup.Add(1)
	go func() {
		defer globalWaitGroup.Done()
		executor(watcher, commandChannel, outputChannel)
	}()

	globalWaitGroup.Add(1)
	go func() {
		defer globalWaitGroup.Done()
		watcher.start(outputChannel)
	}()

	var in *bufio.Reader
	if test {
		var buf bytes.Buffer
		{
			out := bufio.NewWriter(&buf)
			writeMain(out)
		}
		in = bufio.NewReader(&buf)
	} else {
		in = bufio.NewReader(os.Stdin)
	}
	decoder := cbor.NewDecoder(in)
	for {
		cmd, err := readPacket(decoder)

		if err == io.EOF {
			close(commandChannel)
			break
		} else if err != nil {
			commandChannel <- command{Type: "error", Id: cmd.Id, Error: err.Error()}
		} else {
			commandChannel <- *cmd
		}
	}

	globalWaitGroup.Wait()

	// Tells the outputter to finish sending.
	outputChannel <- []byte{}

	outputWG.Wait()
}

func main() {
	test := flag.Bool("test", false, "test instead of read from stdin")
	write := flag.Bool("write", false, "write instead of read data")
	flag.Parse()

	if *write {
		writeMain(bufio.NewWriter(os.Stdout))
	} else {
		readMain(*test)
	}
}
