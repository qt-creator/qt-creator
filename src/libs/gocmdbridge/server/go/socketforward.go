// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"fmt"
	"net"
	"os"
	"path/filepath"
	"sync"
	"sync/atomic"

	"github.com/fxamacker/cbor/v2"
)

// socketForwardCmd is sent on the per-connection channel to serialise all
// operations that touch that connection (data writes, close).
type socketForwardCmd struct {
	kind string // "data", "close"
	data []byte // only set for "data"
}

// connState is the per-connection goroutine state.
type connState struct {
	cmdCh chan socketForwardCmd
	done  chan struct{} // closed when the per-connection goroutine exits
}

// socketForwardState is the per-forward-server state.
type socketForwardState struct {
	done     chan struct{} // closed after the accept goroutine and all connections exit
	stopCh   chan struct{} // closed by the accept goroutine when the listener closes
	listener net.Listener
	mu       sync.Mutex
	conns    map[int]*connState
	nextConn int
}

// SocketForwardHandler creates local Unix socket servers on the remote device
// and forwards each accepted client connection through the cmdbridge protocol
// to the C++ side, which in turn connects to the real socket server running
// on the local machine.
//
// Flow:
//
//	remote app  <-->  Go socket server  <-->  cmdbridge (stdin/stderr)
//	                       <-->  C++ QLocalSocket  <-->  local App socket server
//
// Multiple simultaneous client connections are supported. Each accepted
// connection receives a unique ConnId that is included in all protocol
// messages (socketconnect, socketdata, socketclose) so the C++ side can
// demultiplex them.
type SocketForwardHandler struct {
	mutex    sync.Mutex
	forwards map[int]*socketForwardState
}

func NewSocketForwardHandler() *SocketForwardHandler {
	return &SocketForwardHandler{
		forwards: make(map[int]*socketForwardState),
	}
}

func socketServerPath(id int) string {
	return filepath.Join(os.TempDir(), fmt.Sprintf("cmdbridge-server-%d.sock", id))
}

// Result types sent from Go to C++.

type forwardlocalsocketserverreadyresult struct {
	Type string
	Id   int
	Path string // Path of the newly created socket server on the remote device.
}

type socketconnectresult struct {
	Type   string
	Id     int
	ConnId int // unique ID for this connection within the forward
}

type socketdataresult struct {
	Type   string
	Id     int
	ConnId int
	Data   []byte
}

type socketcloseresult struct {
	Type   string
	Id     int
	ConnId int
}

type forwardserverStoppedResult struct {
	Type string
	Id   int
}

// processForward creates a Unix socket server on the remote, sends its path
// back to C++, then accepts multiple simultaneous client connections. For each
// accepted client it:
//  1. Sends socketconnect (with a unique ConnId) so C++ can connect to the
//     local App.
//  2. Reads data from the remote client and forwards it as socketdata packets
//     tagged with ConnId.
//  3. Sends socketclose (with ConnId) when the remote client disconnects.
//
// Each accepted connection is handled by its own goroutine so multiple clients
// can be active simultaneously.
func (h *SocketForwardHandler) processForward(cmd command, out chan<- []byte) {
	serverPath := socketServerPath(cmd.Id)

	// Remove stale socket file from a previous run if present.
	os.Remove(serverPath)

	listener, err := net.Listen("unix", serverPath)
	if err != nil {
		sendError(out, cmd, err)
		return
	}

	fwd := &socketForwardState{
		done:     make(chan struct{}),
		stopCh:   make(chan struct{}),
		listener: listener,
		conns:    make(map[int]*connState),
	}

	h.mutex.Lock()
	h.forwards[cmd.Id] = fwd
	h.mutex.Unlock()

	result, _ := cbor.Marshal(forwardlocalsocketserverreadyresult{
		Type: "forwardlocalsocketserverready",
		Id:   cmd.Id,
		Path: serverPath,
	})
	out <- result

	globalWaitGroup.Add(1)
	go func() {
		defer globalWaitGroup.Done()
		defer close(fwd.done)
		defer os.Remove(serverPath)

		var connWG sync.WaitGroup
		// Defers run LIFO: close stopCh first (signals per-conn goroutines),
		// then wait for all of them to exit.
		defer connWG.Wait()
		defer close(fwd.stopCh)

		for {
			conn, err := listener.Accept()
			if err != nil {
				return
			}

			fwd.mu.Lock()
			connId := fwd.nextConn
			fwd.nextConn++
			cs := &connState{
				cmdCh: make(chan socketForwardCmd, 64),
				done:  make(chan struct{}),
			}
			fwd.conns[connId] = cs
			fwd.mu.Unlock()

			// Notify C++ that a new remote client connected.
			connectData, _ := cbor.Marshal(socketconnectresult{
				Type:   "socketconnect",
				Id:     cmd.Id,
				ConnId: connId,
			})
			out <- connectData

			connWG.Add(1)
			globalWaitGroup.Add(1)
			go func(connId int, conn net.Conn, cs *connState) {
				defer globalWaitGroup.Done()
				defer connWG.Done()
				defer close(cs.done)
				defer func() {
					fwd.mu.Lock()
					delete(fwd.conns, connId)
					fwd.mu.Unlock()
				}()

				// closedByCpp is set before closing conn when the close was
				// initiated by the C++ side, to suppress the redundant
				// socketclose that the read goroutine would otherwise send.
				var closedByCpp atomic.Bool

				readDone := make(chan struct{})
				globalWaitGroup.Add(1)
				go func() {
					defer globalWaitGroup.Done()
					defer close(readDone)
					buf := make([]byte, 32768)
					for {
						n, readErr := conn.Read(buf)
						if n > 0 {
							data, _ := cbor.Marshal(socketdataresult{
								Type:   "socketdata",
								Id:     cmd.Id,
								ConnId: connId,
								Data:   append([]byte(nil), buf[:n]...),
							})
							out <- data
						}
						if readErr != nil {
							if !closedByCpp.Load() {
								closeData, _ := cbor.Marshal(socketcloseresult{
									Type:   "socketclose",
									Id:     cmd.Id,
									ConnId: connId,
								})
								out <- closeData
							}
							return
						}
					}
				}()

				closeConn := func() {
					conn.Close()
					<-readDone
				}

				for {
					select {
					case fc, ok := <-cs.cmdCh:
						if !ok {
							closeConn()
							return
						}
						switch fc.kind {
						case "data":
							_, writeErr := conn.Write(fc.data)
							if writeErr != nil {
								// Write failed: the connection is broken. Set closedByCpp to
								// suppress the read goroutine's duplicate notification.
								closedByCpp.Store(true)
								closeConn()
								closeData, _ := cbor.Marshal(socketcloseresult{
									Type:   "socketclose",
									Id:     cmd.Id,
									ConnId: connId,
								})
								out <- closeData
								return
							}
						case "close":
							closedByCpp.Store(true)
							closeConn()
							return
						}
					case <-readDone:
						return
					case <-fwd.stopCh:
						closedByCpp.Store(true)
						closeConn()
						return
					}
				}
			}(connId, conn, cs)
		}
	}()
}

// processData sends bytes received from the C++ side to the per-connection
// goroutine identified by cmd.ConnId within the forward identified by cmd.Id.
func (h *SocketForwardHandler) processData(cmd command) {
	h.mutex.Lock()
	fwd, ok := h.forwards[cmd.Id]
	h.mutex.Unlock()
	if !ok {
		return
	}

	fwd.mu.Lock()
	cs, ok := fwd.conns[cmd.ConnId]
	fwd.mu.Unlock()
	if !ok {
		return
	}

	select {
	case cs.cmdCh <- socketForwardCmd{kind: "data", data: cmd.SocketData.Data}:
	case <-cs.done:
	}
}

// processClose tells the per-connection goroutine identified by cmd.ConnId to
// close that remote client connection (not the listener). The listener keeps
// running so it can accept further clients.
func (h *SocketForwardHandler) processClose(cmd command) {
	h.mutex.Lock()
	fwd, ok := h.forwards[cmd.Id]
	h.mutex.Unlock()
	if !ok {
		return
	}

	fwd.mu.Lock()
	cs, ok := fwd.conns[cmd.ConnId]
	fwd.mu.Unlock()
	if !ok {
		return
	}

	select {
	case cs.cmdCh <- socketForwardCmd{kind: "close"}:
	case <-cs.done:
	}
}

// processStopForward tears down the entire forward server: closes the listener
// and signals all active connections to close. It launches a goroutine that
// waits for all connection goroutines to finish and then sends a
// "forwardserverstopped" packet on out. This guarantees that every in-flight
// socketdata/socketclose packet written to out before the stop is delivered to
// the C++ side before it receives the stopped notification.
func (h *SocketForwardHandler) processStopForward(cmd command, out chan<- []byte) {
	h.mutex.Lock()
	fwd, ok := h.forwards[cmd.Id]
	if ok {
		delete(h.forwards, cmd.Id)
	}
	h.mutex.Unlock()
	if !ok {
		return
	}

	// Closing the listener unblocks Accept in the accept goroutine. The accept
	// goroutine then closes fwd.stopCh, which signals all per-connection
	// goroutines to shut down.
	fwd.listener.Close()

	// Wait for all connection goroutines to finish (and flush their packets to
	// out) before notifying C++ that the forward has fully stopped.
	globalWaitGroup.Add(1)
	go func() {
		defer globalWaitGroup.Done()
		<-fwd.done
		result, _ := cbor.Marshal(forwardserverStoppedResult{
			Type: "forwardserverstopped",
			Id:   cmd.Id,
		})
		out <- result
	}()
}
