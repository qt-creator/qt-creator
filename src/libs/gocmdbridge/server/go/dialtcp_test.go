// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"io"
	"net"
	"testing"
	"time"

	"github.com/fxamacker/cbor/v2"
)

type packet struct {
	Type   string
	Id     int
	ConnId int
	Data   []byte
	Error  string
}

func nextPacket(t *testing.T, out <-chan []byte) packet {
	t.Helper()
	select {
	case data := <-out:
		var p packet
		if err := cbor.Unmarshal(data, &p); err != nil {
			t.Fatalf("decode packet: %v", err)
		}
		return p
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for a packet")
		return packet{}
	}
}

func expectPacket(t *testing.T, out <-chan []byte, typ string) packet {
	t.Helper()
	p := nextPacket(t, out)
	if p.Type != typ {
		t.Fatalf("expected %q, got %q (%s)", typ, p.Type, p.Error)
	}
	return p
}

func listenLocal(t *testing.T) net.Listener {
	t.Helper()
	l, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("listen: %v", err)
	}
	return l
}

// TestDialTCPRelaysBothWays dials a TCP server and checks that bytes flow in
// both directions with ConnId 0, and that closing the forward is acknowledged.
func TestDialTCPRelaysBothWays(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)
	cmd := command{Type: "dialtcp", Id: 9910, Path: l.Addr().String()}

	handler.processDialTCP(cmd, out)
	expectPacket(t, out, "dialtcpready")

	server, err := l.Accept()
	if err != nil {
		t.Fatalf("accept: %v", err)
	}
	defer server.Close()

	// app -> Creator
	if _, err := server.Write([]byte("hello")); err != nil {
		t.Fatal(err)
	}
	p := expectPacket(t, out, "socketdata")
	if p.Id != cmd.Id || p.ConnId != 0 || string(p.Data) != "hello" {
		t.Fatalf("unexpected data packet %+v", p)
	}

	// Creator -> app
	handler.processData(command{Type: "socketdata", Id: cmd.Id, ConnId: 0,
		SocketData: socketdatacmd{Data: []byte("world")}})
	buf := make([]byte, 5)
	server.SetReadDeadline(time.Now().Add(2 * time.Second))
	if _, err := io.ReadFull(server, buf); err != nil || string(buf) != "world" {
		t.Fatalf("read %q, err %v", buf, err)
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
	expectPacket(t, out, "forwardserverstopped")
}

// TestDialTCPFailureIsAnError checks that a refused connection is reported as
// an error for the job and leaves no forward behind.
func TestDialTCPFailureIsAnError(t *testing.T) {
	l := listenLocal(t)
	addr := l.Addr().String()
	l.Close() // nothing listens there any more

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 8)
	cmd := command{Type: "dialtcp", Id: 9911, Path: addr}

	handler.processDialTCP(cmd, out)
	p := expectPacket(t, out, "error")
	if p.Id != cmd.Id || p.Error == "" {
		t.Fatalf("unexpected error packet %+v", p)
	}

	handler.mutex.Lock()
	_, found := handler.forwards[cmd.Id]
	handler.mutex.Unlock()
	if found {
		t.Fatal("a failed dial left a forward registered")
	}
}

// TestDialTCPServerCloseIsReported checks that the server hanging up produces
// socketclose, and that the forward can still be stopped afterwards.
func TestDialTCPServerCloseIsReported(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)
	cmd := command{Type: "dialtcp", Id: 9912, Path: l.Addr().String()}

	handler.processDialTCP(cmd, out)
	expectPacket(t, out, "dialtcpready")
	server, err := l.Accept()
	if err != nil {
		t.Fatal(err)
	}
	server.Close()

	p := expectPacket(t, out, "socketclose")
	if p.Id != cmd.Id || p.ConnId != 0 {
		t.Fatalf("unexpected close packet %+v", p)
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
	expectPacket(t, out, "forwardserverstopped")
}

// TestDialTCPStopWhileOpen checks that stopping an open connection closes the
// socket towards the server.
func TestDialTCPStopWhileOpen(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)
	cmd := command{Type: "dialtcp", Id: 9913, Path: l.Addr().String()}

	handler.processDialTCP(cmd, out)
	expectPacket(t, out, "dialtcpready")
	server, err := l.Accept()
	if err != nil {
		t.Fatal(err)
	}
	defer server.Close()

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
	expectPacket(t, out, "forwardserverstopped")

	server.SetReadDeadline(time.Now().Add(2 * time.Second))
	if _, err := server.Read(make([]byte, 1)); err != io.EOF {
		t.Fatalf("expected EOF after stop, got %v", err)
	}
}

// TestSocketDataKeepsOrderThroughExecutor sends many chunks through the
// executor, which is how the bridge receives them, and checks that the server
// gets them in the order they were sent. Commands run on goroutines of their
// own would let neighbouring chunks overtake each other.
func TestSocketDataKeepsOrderThroughExecutor(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 4096)
	commands := make(chan command, 4096)
	go executor(nil, handler, commands, out)
	defer close(commands)

	const id = 9920
	commands <- command{Type: "dialtcp", Id: id, Path: l.Addr().String()}
	expectPacket(t, out, "dialtcpready")
	server, err := l.Accept()
	if err != nil {
		t.Fatal(err)
	}
	defer server.Close()

	const chunks = 2000
	const chunkSize = 1024
	go func() {
		for i := 0; i < chunks; i++ {
			chunk := make([]byte, chunkSize)
			for j := range chunk {
				chunk[j] = byte(i)
			}
			commands <- command{Type: "socketdata", Id: id, ConnId: 0,
				SocketData: socketdatacmd{Data: chunk}}
		}
	}()

	server.SetReadDeadline(time.Now().Add(10 * time.Second))
	buf := make([]byte, chunks*chunkSize)
	if _, err := io.ReadFull(server, buf); err != nil {
		t.Fatalf("read: %v", err)
	}
	for i := 0; i < chunks; i++ {
		if buf[i*chunkSize] != byte(i) || buf[(i+1)*chunkSize-1] != byte(i) {
			t.Fatalf("chunk %d arrived out of order (found %d)", i, buf[i*chunkSize])
		}
	}
}

// TestDialTCPStopKeepsQueuedData sends data and stops the forward straight
// away, as the C++ side does when the tool writes and hangs up. The data was
// queued before the stop, so the server must receive all of it.
func TestDialTCPStopKeepsQueuedData(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)

	// The stop and the data used to race for the connection's goroutine, so
	// a single round would only catch a regression about half of the time.
	for i := 0; i < 20; i++ {
		cmd := command{Type: "dialtcp", Id: 9930 + i, Path: l.Addr().String()}
		handler.processDialTCP(cmd, out)
		expectPacket(t, out, "dialtcpready")
		server, err := l.Accept()
		if err != nil {
			t.Fatal(err)
		}

		handler.processData(command{Type: "socketdata", Id: cmd.Id, ConnId: 0,
			SocketData: socketdatacmd{Data: []byte("bye")}})
		handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
		expectPacket(t, out, "forwardserverstopped")

		server.SetReadDeadline(time.Now().Add(2 * time.Second))
		got, err := io.ReadAll(server)
		server.Close()
		if err != nil || string(got) != "bye" {
			t.Fatalf("round %d: read %q, err %v", i, got, err)
		}
	}
}

// TestDialTCPServerHalfCloseClosesConnection checks that the bridge closes its
// end once the server has shut down its sending side, as "nc -l" does, which
// otherwise waits for that forever.
func TestDialTCPServerHalfCloseClosesConnection(t *testing.T) {
	l := listenLocal(t)
	defer l.Close()

	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)
	cmd := command{Type: "dialtcp", Id: 9950, Path: l.Addr().String()}

	handler.processDialTCP(cmd, out)
	expectPacket(t, out, "dialtcpready")
	server, err := l.Accept()
	if err != nil {
		t.Fatal(err)
	}
	defer server.Close()

	server.(*net.TCPConn).CloseWrite()
	expectPacket(t, out, "socketclose")

	server.SetReadDeadline(time.Now().Add(2 * time.Second))
	if _, err := server.Read(make([]byte, 1)); err != io.EOF {
		t.Fatalf("expected EOF after the half-close, got %v", err)
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
	expectPacket(t, out, "forwardserverstopped")
}
