// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"bytes"
	"testing"
)

// The marker is what tells the launcher that the exec of this binary succeeded,
// so its format is a contract with Utils::takePidMarker(): the template with
// the pid in place of its %1, one newline, nothing else.
func TestWritePidMarker(t *testing.T) {
	var buf bytes.Buffer
	if err := writePidMarker(&buf, "__qtc%1qtc__", 4711); err != nil {
		t.Fatal(err)
	}
	if got, want := buf.String(), "__qtc4711qtc__\n"; got != want {
		t.Fatalf("got %q, want %q", got, want)
	}
}

// No marker asked for, nothing written: a launcher that does not parse one
// would see the line as output of the command it started.
func TestWritePidMarkerEmpty(t *testing.T) {
	var buf bytes.Buffer
	if err := writePidMarker(&buf, "", 4711); err != nil {
		t.Fatal(err)
	}
	if got := buf.String(); got != "" {
		t.Fatalf("got %q, want nothing", got)
	}
}

// A template with nowhere to put the pid is the caller's bug, and stdout is not
// the place to report it: whatever went there would be read as the started
// command's output.
func TestWritePidMarkerWithoutHole(t *testing.T) {
	var buf bytes.Buffer
	if err := writePidMarker(&buf, "__qtc", 4711); err == nil {
		t.Fatal("no error for a template without %1")
	}
	if got := buf.String(); got != "" {
		t.Fatalf("got %q, want nothing", got)
	}
}
