package main

import (
	"errors"
	"os/exec"
	"sync"

	"github.com/fxamacker/cbor/v2"
)

type execute struct {
	Args  []string
	Env   []string
	Stdin []byte
}

type execresult struct {
	Type   string
	Id     int
	Code   int
}

type execdata struct {
	Type string
	Id int
	Stdout []byte
	Stderr []byte
}

func processExec(cmd command, out chan<- []byte) {
	process := exec.Command(cmd.Exec.Args[0], cmd.Exec.Args[1:]...)
	process.Env = cmd.Exec.Env

    in, _ := process.StdinPipe()
	stdout, _ := process.StdoutPipe()
	stderr, _ := process.StderrPipe()
	process.Start()
	if len(cmd.Exec.Stdin) > 0 {
		in.Write([]byte(cmd.Exec.Stdin))
	}
    in.Close()

	var wg sync.WaitGroup
	wg.Add(2)
	go func() {
		defer wg.Done()
		for {
			buf := make([]byte, 1024)
			n, err := stdout.Read(buf)
			if n > 0 {
				result, _ := cbor.Marshal(execdata{
					Type:   "execdata",
					Id:     cmd.Id,
					Stdout: buf[:n],
				})
				out <- result
			}
			if err != nil {
				break
			}
		}
	}()
	go func() {
		defer wg.Done()
		for {
			buf := make([]byte, 1024)
			n, err := stderr.Read(buf)
			if n > 0 {
				result, _ := cbor.Marshal(execdata{
					Type:   "execdata",
					Id:     cmd.Id,
					Stderr: buf[:n],
				})
				out <- result
			}
			if err != nil {
				break
			}
		}
	}()

    err := process.Wait()
	wg.Wait()

    exitCode := 0
	if err != nil {
		exitCode = -1
		var exitError *exec.ExitError
		if errors.As(err, &exitError) {
			exitCode = exitError.ExitCode()
		}
	}

	result, _ := cbor.Marshal(execresult{
		Type:   "execresult",
		Id:     cmd.Id,
		Code:   exitCode,
	})

	out <- result
}
