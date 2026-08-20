// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package main

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/fxamacker/cbor/v2"
)

// sendErrorResult runs err through sendError and decodes the packet it
// produces.
func sendErrorResult(t *testing.T, err error) errorresult {
	t.Helper()
	out := make(chan []byte, 1)
	sendError(out, command{Id: 1}, err)
	var r errorresult
	if unmarshalErr := cbor.Unmarshal(<-out, &r); unmarshalErr != nil {
		t.Fatalf("decode errorresult: %v", unmarshalErr)
	}
	return r
}

// The client only special-cases the literal "Errno" type, decoded through
// std::generic_category() (see cmdbridgeclient.cpp) - so a caller-visible
// errno has to be one of the cases sendError() classifies, or it never
// reaches the client at all. These pin the wire contract for the
// conditions callers actually rely on.

func TestSendErrorNotExist(t *testing.T) {
	dir := t.TempDir()
	_, err := os.Stat(filepath.Join(dir, "missing"))

	r := sendErrorResult(t, err)
	if r.ErrorType != "Errno" || r.Errno != posixENOENT {
		t.Fatalf("Stat on a missing file: got ErrorType=%q Errno=%d, want Errno/%d",
			r.ErrorType, r.Errno, posixENOENT)
	}
}

// Regression test: fs.ErrInvalid never matches a real syscall.Errno (see
// isErrno's doc comment), so classifying EINVAL through it, as an earlier
// version of this file did, silently never fires. bridgedfileaccess.cpp's
// symLinkTarget() depends on seeing EINVAL here to report "not a symlink"
// instead of a generic failure.
func TestSendErrorInvalidNotASymlink(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "regular-file")
	if err := os.WriteFile(path, []byte("x"), 0600); err != nil {
		t.Fatal(err)
	}

	_, err := os.Readlink(path)
	if err == nil {
		t.Fatal("Readlink on a regular file unexpectedly succeeded")
	}

	r := sendErrorResult(t, err)
	if r.ErrorType != "Errno" || r.Errno != posixEINVAL {
		t.Fatalf("Readlink on a regular file: got ErrorType=%q Errno=%d, want Errno/%d",
			r.ErrorType, r.Errno, posixEINVAL)
	}
}

// Regression test: fs.ErrExist also matches ENOTEMPTY (see isErrno's
// callers in sendError), so removing a non-empty directory has to be
// checked ahead of the generic fs.ErrExist case, or it is misreported as
// EEXIST instead.
func TestSendErrorNotEmpty(t *testing.T) {
	dir := t.TempDir()
	sub := filepath.Join(dir, "sub")
	if err := os.Mkdir(sub, 0700); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(sub, "f"), []byte("x"), 0600); err != nil {
		t.Fatal(err)
	}

	err := os.Remove(sub)
	if err == nil {
		t.Fatal("Remove of a non-empty directory unexpectedly succeeded")
	}

	r := sendErrorResult(t, err)
	if r.ErrorType != "Errno" || r.Errno != posixENOTEMPTY {
		t.Fatalf("Remove of a non-empty directory: got ErrorType=%q Errno=%d, want Errno/%d",
			r.ErrorType, r.Errno, posixENOTEMPTY)
	}
}
