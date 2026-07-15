// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"net"
	"runtime"
	"sync"
	"testing"
	"time"

	"github.com/fxamacker/cbor/v2"
)

// waitReady drains the forwardlocalsocketserverready message from out.
func waitReady(t *testing.T, out <-chan []byte) {
	t.Helper()
	select {
	case <-out:
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for forwardlocalsocketserverready")
	}
}

// waitConnect drains one socketconnect message and returns it.
func waitConnect(t *testing.T, out <-chan []byte) socketconnectresult {
	t.Helper()
	select {
	case data := <-out:
		var r socketconnectresult
		if err := cbor.Unmarshal(data, &r); err != nil {
			t.Fatalf("decode socketconnectresult: %v", err)
		}
		if r.Type != "socketconnect" {
			t.Fatalf("expected socketconnect, got %q", r.Type)
		}
		return r
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for socketconnect")
		return socketconnectresult{}
	}
}

// dialAndConnect dials the server at path and waits for the socketconnect
// message, returning both the local connection and the assigned ConnId.
func dialAndConnect(t *testing.T, path string, out <-chan []byte) (net.Conn, int) {
	t.Helper()
	conn, err := net.Dial("unix", path)
	if err != nil {
		t.Fatalf("dial %s: %v", path, err)
	}
	r := waitConnect(t, out)
	return conn, r.ConnId
}

// TestListenerCloseExitsCleanly verifies that when the listener is closed
// externally (not via processStopForward), both the accept and main forwarding
// goroutines exit without leaking.
func TestListenerCloseExitsCleanly(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9900}

	goroutinesBefore := runtime.NumGoroutine()

	handler.processForward(cmd, out)
	waitReady(t, out)

	handler.mutex.Lock()
	fwd := handler.forwards[cmd.Id]
	handler.mutex.Unlock()
	if fwd == nil {
		t.Fatal("forward state not found")
	}

	// Close the listener from outside (simulating an OS-level close or error).
	fwd.listener.Close()

	select {
	case <-fwd.done:
	case <-time.After(2 * time.Second):
		t.Fatal("forwarding goroutine did not exit after listener close")
	}

	time.Sleep(100 * time.Millisecond)

	goroutinesAfter := runtime.NumGoroutine()
	if goroutinesAfter > goroutinesBefore+1 {
		t.Errorf("goroutine leak: before=%d after=%d", goroutinesBefore, goroutinesAfter)
	}
}

// TestStopForwardThenDataDoesNotBlock verifies that calling processData after
// processStopForward does not deadlock, even with a ConnId that was valid
// before the stop.
func TestStopForwardThenDataDoesNotBlock(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9901}
	handler.processForward(cmd, out)
	waitReady(t, out)

	// Connect a client to obtain a valid connId.
	conn, connId := dialAndConnect(t, socketServerPath(cmd.Id), out)
	defer conn.Close()

	// Grab a reference to fwd before processStopForward removes it —
	// simulating the race window where processData has already looked up fwd.
	handler.mutex.Lock()
	fwd := handler.forwards[cmd.Id]
	handler.mutex.Unlock()
	if fwd == nil {
		t.Fatal("forward state not found")
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)

	// Wait for teardown so cs.done is closed.
	select {
	case <-fwd.done:
	case <-time.After(2 * time.Second):
		t.Fatal("forward did not shut down in time")
	}

	// Late processData calls must return without blocking.
	done := make(chan struct{})
	go func() {
		defer close(done)
		for i := 0; i < 100; i++ {
			handler.processData(command{
				Type:       "socketdata",
				Id:         cmd.Id,
				ConnId:     connId,
				SocketData: socketdatacmd{Data: []byte("x")},
			})
		}
	}()

	select {
	case <-done:
	case <-time.After(5 * time.Second):
		t.Fatal("test timed out — late processData calls blocked")
	}
}

// TestProcessStopForwardCleanup verifies that processStopForward correctly
// tears down the listener, closes active connections, and removes the forward
// from the handler map.
func TestProcessStopForwardCleanup(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 128)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9902}
	handler.processForward(cmd, out)
	waitReady(t, out)

	conn, _ := dialAndConnect(t, socketServerPath(cmd.Id), out)

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)

	// The connected client should get disconnected.
	buf := make([]byte, 1)
	conn.SetReadDeadline(time.Now().Add(2 * time.Second))
	_, readErr := conn.Read(buf)
	if readErr == nil {
		t.Error("expected connection to be closed after stopforward")
	}
	conn.Close()

	// The forward should be removed from the map.
	handler.mutex.Lock()
	_, exists := handler.forwards[cmd.Id]
	handler.mutex.Unlock()
	if exists {
		t.Error("forward entry should be removed from map after stop")
	}
}

// TestStopForwardClosesQueuedConnections verifies that connections accepted by
// the listener before processStopForward is called are explicitly closed,
// rather than being silently leaked.
func TestStopForwardClosesQueuedConnections(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 256)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9904}
	handler.processForward(cmd, out)
	waitReady(t, out)

	serverPath := socketServerPath(cmd.Id)

	const numClients = 5
	conns := make([]net.Conn, 0, numClients)
	for i := 0; i < numClients; i++ {
		c, err := net.Dial("unix", serverPath)
		if err != nil {
			t.Fatalf("dial %d: %v", i, err)
		}
		conns = append(conns, c)
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)

	for i, c := range conns {
		c.SetReadDeadline(time.Now().Add(2 * time.Second))
		buf := make([]byte, 1)
		_, readErr := c.Read(buf)
		if readErr == nil {
			t.Errorf("client %d: expected closed connection, got no error", i)
		}
		c.Close()
	}
}

// TestConcurrentDataAndStop hammers processData and processStopForward
// concurrently to detect deadlocks.
func TestConcurrentDataAndStop(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 1024)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9903}
	handler.processForward(cmd, out)
	waitReady(t, out)

	stopDrain := make(chan struct{})
	go func() {
		for {
			select {
			case <-out:
			case <-stopDrain:
				return
			}
		}
	}()

	var wg sync.WaitGroup

	for i := 0; i < 50; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			// ConnId 0 will not be found (no active connection) so this returns
			// immediately — the test exercises the no-deadlock path.
			handler.processData(command{
				Type:       "socketdata",
				Id:         cmd.Id,
				ConnId:     0,
				SocketData: socketdatacmd{Data: []byte("test")},
			})
		}()
	}

	wg.Add(1)
	go func() {
		defer wg.Done()
		handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
	}()

	done := make(chan struct{})
	go func() {
		wg.Wait()
		close(done)
	}()

	select {
	case <-done:
	case <-time.After(10 * time.Second):
		t.Fatal("test timed out — likely deadlock from concurrent data + stop")
	}
	close(stopDrain)
}

// TestMultipleSimultaneousConnections verifies that several clients can connect
// concurrently and each receives a distinct ConnId.
func TestMultipleSimultaneousConnections(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 256)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9910}
	handler.processForward(cmd, out)
	waitReady(t, out)

	serverPath := socketServerPath(cmd.Id)

	const numConns = 3
	conns := make([]net.Conn, numConns)
	connIds := make(map[int]bool)

	for i := range conns {
		c, err := net.Dial("unix", serverPath)
		if err != nil {
			t.Fatalf("dial %d: %v", i, err)
		}
		conns[i] = c
		r := waitConnect(t, out)
		if connIds[r.ConnId] {
			t.Errorf("duplicate ConnId %d on connection %d", r.ConnId, i)
		}
		connIds[r.ConnId] = true
	}

	if len(connIds) != numConns {
		t.Errorf("expected %d unique ConnIds, got %d", numConns, len(connIds))
	}

	for _, c := range conns {
		c.Close()
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
}

// TestDataExchangeMultipleConnections verifies that data is routed to the
// correct connection and does not bleed into others.
func TestDataExchangeMultipleConnections(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 256)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9911}
	handler.processForward(cmd, out)
	waitReady(t, out)

	serverPath := socketServerPath(cmd.Id)

	// Dial sequentially so we know which socketconnect belongs to which conn.
	connA, connIdA := dialAndConnect(t, serverPath, out)
	defer connA.Close()
	connB, connIdB := dialAndConnect(t, serverPath, out)
	defer connB.Close()

	// --- Go → remote: send to A, verify A receives and B does not ---
	handler.processData(command{
		Type:       "socketdata",
		Id:         cmd.Id,
		ConnId:     connIdA,
		SocketData: socketdatacmd{Data: []byte("hello-A")},
	})

	buf := make([]byte, 64)
	connA.SetReadDeadline(time.Now().Add(2 * time.Second))
	n, err := connA.Read(buf)
	if err != nil {
		t.Fatalf("connA read: %v", err)
	}
	if got := string(buf[:n]); got != "hello-A" {
		t.Errorf("connA: got %q, want %q", got, "hello-A")
	}

	// B should not have received anything within a short window.
	connB.SetReadDeadline(time.Now().Add(50 * time.Millisecond))
	nB, errB := connB.Read(buf)
	if errB == nil {
		t.Errorf("connB unexpectedly received %d bytes: %q", nB, string(buf[:nB]))
	}
	connB.SetReadDeadline(time.Time{})

	// --- remote → Go: data from A arrives tagged with connIdA ---
	if _, werr := connA.Write([]byte("from-A")); werr != nil {
		t.Fatalf("connA write: %v", werr)
	}

	select {
	case data := <-out:
		var r socketdataresult
		if err := cbor.Unmarshal(data, &r); err != nil {
			t.Fatalf("decode socketdataresult: %v", err)
		}
		if r.Type != "socketdata" {
			t.Fatalf("expected socketdata, got %q", r.Type)
		}
		if r.ConnId != connIdA {
			t.Errorf("expected ConnId %d, got %d", connIdA, r.ConnId)
		}
		if string(r.Data) != "from-A" {
			t.Errorf("expected %q, got %q", "from-A", string(r.Data))
		}
	case <-time.After(2 * time.Second):
		t.Fatal("timed out waiting for socketdata from A")
	}

	// --- Go → remote: now send to B ---
	handler.processData(command{
		Type:       "socketdata",
		Id:         cmd.Id,
		ConnId:     connIdB,
		SocketData: socketdatacmd{Data: []byte("hello-B")},
	})

	connB.SetReadDeadline(time.Now().Add(2 * time.Second))
	n, err = connB.Read(buf)
	if err != nil {
		t.Fatalf("connB read: %v", err)
	}
	if got := string(buf[:n]); got != "hello-B" {
		t.Errorf("connB: got %q, want %q", got, "hello-B")
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
}

// TestCloseOneConnectionKeepsOthers verifies that closing one connection via
// processClose does not affect other active connections on the same forward.
func TestCloseOneConnectionKeepsOthers(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 256)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9912}
	handler.processForward(cmd, out)
	waitReady(t, out)

	serverPath := socketServerPath(cmd.Id)

	connA, connIdA := dialAndConnect(t, serverPath, out)
	defer connA.Close()
	connB, connIdB := dialAndConnect(t, serverPath, out)
	defer connB.Close()

	// Close connection A from the C++ side.
	handler.processClose(command{Type: "socketclose", Id: cmd.Id, ConnId: connIdA})

	// connA should be closed.
	buf := make([]byte, 1)
	connA.SetReadDeadline(time.Now().Add(2 * time.Second))
	if _, err := connA.Read(buf); err == nil {
		t.Error("expected connA to be closed, but read succeeded")
	}

	// connB should still be alive: send data to it and verify receipt.
	handler.processData(command{
		Type:       "socketdata",
		Id:         cmd.Id,
		ConnId:     connIdB,
		SocketData: socketdatacmd{Data: []byte("still-alive")},
	})

	dataBuf := make([]byte, 64)
	connB.SetReadDeadline(time.Now().Add(2 * time.Second))
	n, err := connB.Read(dataBuf)
	if err != nil {
		t.Fatalf("connB read failed: %v", err)
	}
	if got := string(dataBuf[:n]); got != "still-alive" {
		t.Errorf("connB: got %q, want %q", got, "still-alive")
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)
}

// TestStopWithMultipleActiveConnections verifies that processStopForward closes
// all concurrently active connections.
func TestStopWithMultipleActiveConnections(t *testing.T) {
	handler := NewSocketForwardHandler()
	out := make(chan []byte, 256)

	cmd := command{Type: "forwardlocalsocketserver", Id: 9913}
	handler.processForward(cmd, out)
	waitReady(t, out)

	serverPath := socketServerPath(cmd.Id)

	const numConns = 3
	conns := make([]net.Conn, numConns)
	for i := range conns {
		var err error
		conns[i], err = net.Dial("unix", serverPath)
		if err != nil {
			t.Fatalf("dial %d: %v", i, err)
		}
		waitConnect(t, out)
	}

	handler.processStopForward(command{Type: "stopforwardserver", Id: cmd.Id}, out)

	// Every client must see its connection closed.
	for i, c := range conns {
		c.SetReadDeadline(time.Now().Add(2 * time.Second))
		buf := make([]byte, 1)
		if _, err := c.Read(buf); err == nil {
			t.Errorf("conn %d: expected closed connection, got no error", i)
		}
		c.Close()
	}
}
