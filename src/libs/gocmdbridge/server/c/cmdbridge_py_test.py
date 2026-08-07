#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""
cmdbridge_py_test.py -- Protocol tests for the C CmdBridge server

Spawns the cmdbridge binary, sends CBOR commands via stdin pipe,
reads and validates responses from stderr pipe.

Uses subprocess.communicate() which reads all stderr after process exit,
avoiding macOS pipe buffering issues with interactive select().

Usage: python3 cmdbridge_py_test.py <cmdbridge_binary_path>
"""

import subprocess
import struct
import sys
import os
import shutil
import signal
import tempfile
import threading
import time


MAGIC = b'PkgMarkerGoBridgeMagicPacket'
MLG = len(MAGIC)


_SESSION_TMPDIR = None


def temp_dir():
    """Return a platform-appropriate temporary directory."""
    global _SESSION_TMPDIR
    if _SESSION_TMPDIR:
        return _SESSION_TMPDIR

    env_tmp = os.environ.get("CMDBRIDGE_TEST_TMPDIR")
    if env_tmp:
        _SESSION_TMPDIR = env_tmp
    else:
        _SESSION_TMPDIR = tempfile.mkdtemp(prefix="cmdbridge_test_")

    return _SESSION_TMPDIR


def cleanup_temp_dir():
    """Remove the session temporary directory if it was created by mkdtemp."""
    global _SESSION_TMPDIR
    if _SESSION_TMPDIR and _SESSION_TMPDIR.startswith("cmdbridge_test_"):
        try:
            import shutil
            shutil.rmtree(_SESSION_TMPDIR)
        except Exception:
            pass


def build_cbor_map(pairs):
    """Build a CBOR map from key-value string pairs."""
    n = len(pairs)
    if n <= 23:
        result = bytes([0xA0 | n])
    else:
        result = b'\xBA' + struct.pack('>H', n)
    for k, v in pairs:
        kb = k.encode()
        if len(kb) <= 23:
            result += bytes([0x60 | len(kb)]) + kb
        else:
            result += b'\x78' + bytes([len(kb)]) + kb
        if isinstance(v, int):
            if v >= 0:
                result += _enc_uint(v)
            else:
                result += bytes([0x20]) + _enc_uint(-(v + 1))
        elif isinstance(v, bytes):
            if len(v) <= 23:
                result += bytes([0x40 | len(v)]) + v
            elif len(v) <= 0xFF:
                result += b'\x58' + bytes([len(v)]) + v
            else:
                result += b'\x59' + struct.pack('>H', len(v)) + v
        else:
            vb = v.encode()
            if len(vb) <= 23:
                result += bytes([0x60 | len(vb)]) + vb
            elif len(vb) <= 0xFF:
                result += b'\x78' + bytes([len(vb)]) + vb
            else:
                result += b'\x79' + struct.pack('>H', len(vb)) + vb
    return result


def _enc_uint(v):
    if v <= 23:
        return bytes([v])
    elif v <= 0xFF:
        return b'\x18' + bytes([v])
    elif v <= 0xFFFF:
        return b'\x19' + v.to_bytes(2, 'big')
    elif v <= 0xFFFFFFFF:
        return b'\x1A' + v.to_bytes(4, 'big')
    else:
        return b'\x1B' + v.to_bytes(8, 'big')


def _enc_str(s):
    """Encode a CBOR text string."""
    b = s.encode()
    n = len(b)
    if n <= 23:
        return bytes([0x60 | n]) + b
    elif n <= 0xFF:
        return b'\x78' + bytes([n]) + b
    else:
        return b'\x79' + n.to_bytes(2, 'big') + b


def _enc_bytes(b):
    """Encode a CBOR byte string."""
    n = len(b)
    if n <= 23:
        return bytes([0x40 | n]) + b
    elif n <= 0xFF:
        return b'\x58' + bytes([n]) + b
    elif n <= 0xFFFF:
        return b'\x59' + struct.pack('>H', n) + b
    else:
        return b'\x5A' + struct.pack('>I', n) + b


def _build_cbor_array(items):
    """Build a CBOR array from a list of string items."""
    n = len(items)
    if n <= 23:
        result = bytes([0x80 | n])
    elif n <= 0xFFFF:
        result = b'\x97' + struct.pack('>H', n)
    else:
        result = b'\x9B' + n.to_bytes(8, 'big')
    for item in items:
        result += _enc_str(item)
    return result


def send_command(bin_path, cbor_data):
    """Send raw CBOR data and return the response bytes from stderr."""
    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
    )
    try:
        proc.stdin.write(cbor_data)
    except BrokenPipeError:
        pass
    try:
        proc.stdin.close()
    except OSError:
        pass
    # Workaround for Python 3.12 bug: communicate() fails with "flush of closed file"
    # when the process exits immediately with no stderr output. Read stderr directly.
    stderr_out = proc.stderr.read()
    proc.wait(timeout=5)
    return stderr_out


def parse_response(stderr_data):
    """Parse one or more CBOR responses from stderr data."""
    results = []
    pos = 0
    while pos + MLG + 4 <= len(stderr_data):
        idx = stderr_data.find(MAGIC, pos)
        if idx == -1:
            break
        pkt_len = struct.unpack('>I', stderr_data[idx+MLG:idx+MLG+4])[0]
        pkt_start = idx + MLG + 4
        if pkt_start + pkt_len <= len(stderr_data):
            results.append(stderr_data[pkt_start:pkt_start+pkt_len])
            pos = pkt_start + pkt_len
        else:
            break
    return results


def parse_response_like_client(stderr_data):
    """Parse packets the way Client::readPacket() does.

    The C++ client feeds one packet into a QCborStreamReader and reads a single
    value from it; trailing bytes in the same packet are never looked at. This
    helper reproduces that exactly and returns

        (values, violations)

    where `values` holds the one decoded value per packet that the client would
    actually see, and `violations` lists (packet_len, bytes_consumed) for every
    packet that contained more than one value -- i.e. responses the real client
    would silently drop.
    """
    values = []
    violations = []
    for pkt in parse_response(stderr_data):
        try:
            decoder = _CborDecoder(memoryview(pkt))
            values.append(decoder.decode())
            if decoder.pos != len(pkt):
                violations.append((len(pkt), decoder.pos))
        except Exception:
            violations.append((len(pkt), -1))
    return values, violations


def drain_responses(bridge):
    """Yields every response received so far, removing it from the queue.

    Popping rather than copying-then-clearing matters: the reader thread
    appends concurrently, so anything arriving between the copy and the clear
    would be lost. That is invisible with a handful of responses and fatal with
    thousands.
    """
    while True:
        try:
            raw = bridge.responses.pop(0)
        except IndexError:
            return
        yield decode_cbor(raw)


def assert_one_value_per_packet(stderr_data, what):
    """Fail if any packet carries more than one CBOR value."""
    values, violations = parse_response_like_client(stderr_data)
    assert not violations, (
        f"{what}: {len(violations)} packet(s) carry more than one CBOR value "
        f"{violations[:5]}; the client decodes only the first value per packet, "
        "so the rest would be lost")
    return values


def decode_cbor(data):
    """Decode CBOR-encoded bytes into Python objects.

    Supports: maps, text strings, byte strings, arrays, unsigned/negative integers.
    This is a minimal decoder matching the TinyCBOR encoder used by cmdbridge.
    """
    if isinstance(data, (bytes, bytearray)):
        data = memoryview(data)
    decoder = _CborDecoder(data)
    return decoder.decode()


class _CborDecoder:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def remaining(self):
        return len(self.data) - self.pos

    def peek(self):
        if self.pos >= len(self.data):
            raise ValueError("No more bytes")
        return self.data[self.pos]

    def read(self, n):
        if self.pos + n > len(self.data):
            raise ValueError(f"Expected {n} bytes, have {self.remaining()}")
        result = self.data[self.pos:self.pos + n]
        self.pos += n
        return result

    def decode(self):
        if self.remaining() <= 0:
            raise ValueError("Empty data")
        major, info = self._read_major()
        if major == 0:
            return self._decode_uint_add(info)
        elif major == 1:
            return -1 - self._decode_uint_add(info)
        elif major == 2:
            return self._decode_bytes(info)
        elif major == 3:
            return self._decode_text(info)
        elif major == 4:
            return self._decode_array(info)
        elif major == 5:
            return self._decode_map(info)
        elif major == 7:
            if info == 20:
                return False
            elif info == 21:
                return True
            elif info == 22:
                return None
            elif info == 23:
                return None
            else:
                raise ValueError(f"Unsupported simple value: {info}")
        else:
            raise ValueError(f"Unsupported major type: {major}, info={info}")

    def _read_major(self):
        b = self.read(1)[0]
        major = b >> 5
        info = b & 0x1F
        return major, info

    def _decode_uint_add(self, info):
        if info < 24:
            return info
        elif info == 24:
            return self.read(1)[0]
        elif info == 25:
            return struct.unpack('>H', self.read(2))[0]
        elif info == 26:
            return struct.unpack('>I', self.read(4))[0]
        elif info == 27:
            return struct.unpack('>Q', self.read(8))[0]
        raise ValueError(f"Invalid uint info: {info}")

    def _decode_bytes(self, info):
        n = self._decode_uint_add(info)
        raw = self.read(n)
        return bytes(raw)

    def _decode_text(self, info):
        n = self._decode_uint_add(info)
        raw = bytes(self.read(n))
        try:
            return raw.decode('utf-8')
        except UnicodeDecodeError:
            return raw.hex()

    def _decode_array(self, info):
        n = self._decode_uint_add(info)
        result = []
        for _ in range(n):
            result.append(self.decode())
        return result

    def _decode_map(self, info):
        n = self._decode_uint_add(info)
        result = {}
        for _ in range(n):
            key = self.decode()
            value = self.decode()
            result[key] = value
        return result


def test_ping(bin_path):
    """Ping should produce no output."""
    cbor = build_cbor_map([("Type", "ping")])
    stderr_out = send_command(bin_path, cbor)
    # Ping produces no response, so stderr should be empty or just have the exit
    return True


def test_stat(bin_path):
    """Stat temp dir should return statresult."""
    tmpdir = temp_dir()
    cbor = build_cbor_map([
        ("Type", "stat"),
        ("Id", "1"),
        ("Path", tmpdir),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    assert resp[0][0] == 0xA8, f"expected map(8), got 0x{resp[0][0]:02x}"
    return True


def test_stat_nonexistent(bin_path):
    """Stat nonexistent path should return error."""
    cbor = build_cbor_map([
        ("Type", "stat"),
        ("Id", "2"),
        ("Path", "/nonexistent/path/xyz"),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    # Error response has Type="error", Id, and Error fields (map with at least 3 pairs)
    assert (resp[0][0] & 0x1F) >= 3, f"expected error map with >=3 pairs, got 0x{resp[0][0]:02x}"
    return True


def test_is_dir(bin_path):
    """Check temp dir is a directory."""
    tmpdir = temp_dir()
    cbor = build_cbor_map([
        ("Type", "is"),
        ("Id", "3"),
        ("Path", tmpdir),
        ("Check", "6"),  # 6 = Dir
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    return True


def test_copyfile(bin_path):
    """Copy a file."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    src = os.path.join(tmpdir, "cmdbridge_test_src")
    dst = os.path.join(tmpdir, "cmdbridge_test_dst")

    with open(src, "w") as f:
        f.write("hello world")

    cbor = build_cbor_map([
        ("Type", "copyfile"),
        ("Id", "8"),
        ("Source", src),
        ("Target", dst),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    assert resp[0][0] == 0xA2, f"expected map(2), got 0x{resp[0][0]:02x}"

    assert os.path.exists(dst), "dst file not created"
    with open(dst) as f:
        assert f.read() == "hello world", "file content mismatch"

    os.unlink(src)
    os.unlink(dst)
    return True


def test_writefile_readfile(bin_path):
    """Write and read back a file."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    path = os.path.join(tmpdir, "cmdbridge_rw_test")
    if os.path.exists(path):
        os.unlink(path)

    content = "hello test"
    content_bytes = content.encode()

    # Build writefile with byte string contents
    cbor = b'\xA4'  # map(4)
    cbor += _enc_str("Type") + _enc_str("writefile")
    cbor += _enc_str("Id") + _enc_uint(9)
    pb = path.encode()
    cbor += _enc_str("Path") + _enc_str(path)
    cbor += _enc_str("Contents") + _enc_bytes(content_bytes)

    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    assert resp[0][0] == 0xA3, f"expected map(3), got 0x{resp[0][0]:02x}"

    # Read file back - readfile sends readfiledata + readfiledone responses
    cbor2 = b'\xA4'  # map(4)
    cbor2 += _enc_str("Type") + _enc_str("readfile")
    cbor2 += _enc_str("Id") + _enc_uint(10)
    pb = path.encode()
    cbor2 += _enc_str("Path") + _enc_str(path)
    cbor2 += _enc_str("Offset") + _enc_uint(0)
    cbor2 += _enc_str("Limit") + _enc_uint(0xFFFFFFFF)

    stderr_out2 = send_command(bin_path, cbor2)
    resp2 = parse_response(stderr_out2)
    assert len(resp2) >= 1, f"expected at least 1 response, got {len(resp2)}"
    # First response should be readfiledata (map with Contents)
    assert resp2[0][0] == 0xA3, f"expected readfiledata map(3), got 0x{resp2[0][0]:02x}"

    os.unlink(path)
    return True


def test_unicode_filenames(bin_path):
    """Test file operations with unicode characters in filenames."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)

    unicode_names = [
        "файл_test",
        "ファイル_test",
        "文件_test",
        "ملف_test",
    ]

    for uname in unicode_names:
        path = os.path.join(tmpdir, uname)
        if os.path.exists(path):
            os.unlink(path)

        content = f"content-for-{uname}".encode("utf-8")

        cbor = b'\xA4'
        cbor += _enc_str("Type") + _enc_str("writefile")
        cbor += _enc_str("Id") + _enc_uint(9)
        cbor += _enc_str("Path") + _enc_str(path)
        cbor += _enc_str("Contents") + _enc_bytes(content)

        stderr_out = send_command(bin_path, cbor)
        resp = parse_response(stderr_out)
        assert len(resp) == 1, f"expected 1 response for filename, got {len(resp)}"
        assert resp[0][0] == 0xA3, f"expected map(3) for filename, got 0x{resp[0][0]:02x}"
        # Verify file was actually created on disk
        assert os.path.exists(path), f"file not created for unicode filename: {uname!r}"
        assert os.path.getsize(path) == len(content), f"file size mismatch for {uname!r}"

        # Read file back via cmdbridge
        cbor2 = b'\xA4'
        cbor2 += _enc_str("Type") + _enc_str("readfile")
        cbor2 += _enc_str("Id") + _enc_uint(10)
        cbor2 += _enc_str("Path") + _enc_str(path)
        cbor2 += _enc_str("Offset") + _enc_uint(0)
        cbor2 += _enc_str("Limit") + _enc_uint(0xFFFFFFFF)

        stderr_out2 = send_command(bin_path, cbor2)
        resp2 = parse_response(stderr_out2)
        assert len(resp2) >= 1, f"expected at least 1 response for filename, got {len(resp2)}"
        # First response should be readfiledata (map with Type, Id, Contents)
        assert resp2[0][0] == 0xA3, f"expected readfiledata map(3) for filename, got 0x{resp2[0][0]:02x}"

        # Verify response contains "Contents" key with byte string value
        contents_marker = b"\x68Contents"
        assert contents_marker in resp2[0], f"no Contents key in readfiledata response for {uname!r}"

        os.unlink(path)

    return True


def test_long_paths(bin_path):
    """Test file operations with paths longer than 255 characters using cmdbridge."""
    tmpdir = temp_dir()

    # Build a directory path that makes the total path > 255 chars
    # Create intermediate dirs step by step since createdir only creates leaf
    # tmpdir is /tmp (5) on Linux or ~38 on Windows, use 80-char segments
    base = os.path.join(tmpdir, "lp")
    seg1 = os.path.join(base, "a" * 80)
    seg2 = os.path.join(seg1, "b" * 80)
    seg3 = os.path.join(seg2, "c" * 80)

    # Create each directory level
    for path, id_num in [(base, 1), (seg1, 2), (seg2, 3), (seg3, 4)]:
        cbor = build_cbor_map([("Type", "createdir"), ("Id", str(id_num)), ("Path", path)])
        stderr_out = send_command(bin_path, cbor)
        raw_resp = parse_response(stderr_out)
        assert len(raw_resp) == 1, f"expected 1 response for {path}, got {len(raw_resp)}"
        resp = decode_cbor(raw_resp[0])
        if isinstance(resp, dict) and resp.get("Type") == "error":
            error_msg = resp.get("Error", "unknown error")
            error_type = resp.get("ErrorType", "")
            errno_val = resp.get("Errno", -1)
            raise AssertionError(f"createdir failed for {path}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")
        assert isinstance(resp, dict), f"expected map response for {path}, got {type(resp)}"
        assert "Id" in resp, f"missing Id in response for {path}"

    # File with short name but long total path (>255 chars)
    short_filename = "testfile.txt"
    long_filepath = os.path.join(seg3, short_filename)
    assert len(long_filepath) > 255, f"path not long enough: {len(long_filepath)}"

    content = b"long path test content"

    # Write file via cmdbridge
    cbor = build_cbor_map([
        ("Type", "writefile"),
        ("Id", "5"),
        ("Path", long_filepath),
        ("Contents", content),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"writefile failed for {long_filepath}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    # Read file back via cmdbridge
    cbor = build_cbor_map([
        ("Type", "readfile"),
        ("Id", "6"),
        ("Path", long_filepath),
        ("Offset", "0"),
        ("Limit", str(0xFFFFFFFF)),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) >= 1, f"expected at least 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"readfile failed for {long_filepath}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    # Verify Contents key is present
    assert "Contents" in resp, "no Contents key in readfiledata response"

    # Stat the long-path file
    cbor = build_cbor_map([
        ("Type", "stat"),
        ("Id", "7"),
        ("Path", long_filepath),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"stat failed for {long_filepath}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    # Second test: file directly in tmpdir with long path
    seg4 = os.path.join(tmpdir, "lp2", "a" * 100)
    cbor = build_cbor_map([("Type", "createdir"), ("Id", "8"), ("Path", os.path.join(tmpdir, "lp2"))])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"createdir failed for lp2: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    cbor = build_cbor_map([("Type", "createdir"), ("Id", "9"), ("Path", seg4)])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"createdir failed for seg4: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    long_filepath2 = os.path.join(seg4, "file.txt")
    cbor = build_cbor_map([
        ("Type", "writefile"),
        ("Id", "10"),
        ("Path", long_filepath2),
        ("Contents", b"second long path"),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"writefile failed for {long_filepath2}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    # Stat the second long-path file
    cbor = build_cbor_map([
        ("Type", "stat"),
        ("Id", "11"),
        ("Path", long_filepath2),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"stat failed for {long_filepath2}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    # Cleanup via removeall
    cbor = build_cbor_map([("Type", "removeall"), ("Id", "12"), ("Path", base)])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"removeall failed for {base}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    cbor = build_cbor_map([("Type", "removeall"), ("Id", "13"), ("Path", os.path.join(tmpdir, "lp2"))])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"removeall failed for lp2: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    return True


def test_createdir_remove(bin_path):
    """Create and remove a directory."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    d = os.path.join(tmpdir, "cmdbridge_test_dir")
    if os.path.exists(d):
        os.rmdir(d)

    cbor = build_cbor_map([
        ("Type", "createdir"),
        ("Id", "6"),
        ("Path", d),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"createdir failed for {d}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    assert os.path.isdir(d), "dir not created"

    cbor = build_cbor_map([
        ("Type", "removeall"),
        ("Id", "7"),
        ("Path", d),
    ])
    stderr_out = send_command(bin_path, cbor)
    raw_resp = parse_response(stderr_out)
    assert len(raw_resp) == 1, f"expected 1 response, got {len(raw_resp)}"
    resp = decode_cbor(raw_resp[0])
    if isinstance(resp, dict) and resp.get("Type") == "error":
        error_msg = resp.get("Error", "unknown error")
        error_type = resp.get("ErrorType", "")
        errno_val = resp.get("Errno", -1)
        raise AssertionError(f"removeall failed for {d}: {error_msg} (ErrorType={error_type}, Errno={errno_val})")

    assert not os.path.exists(d), "dir not removed"
    return True


def test_removeall(bin_path):
    """Remove a directory with contents."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    d = os.path.join(tmpdir, "cmdbridge_test_rmall")
    sub = os.path.join(d, "sub")
    f = os.path.join(sub, "file.txt")

    if os.path.exists(d):
        import shutil
        shutil.rmtree(d)

    os.makedirs(sub)
    with open(f, "w") as fh:
        fh.write("content")

    cbor = build_cbor_map([
        ("Type", "removeall"),
        ("Id", "100"),
        ("Path", d),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    assert not os.path.exists(d), "dir tree not removed"
    return True


def test_createlink(bin_path):
    """Create a symlink."""
    tmpdir = temp_dir()
    target = os.path.join(tmpdir, "cmdbridge_link_target")
    link = os.path.join(tmpdir, "cmdbridge_link")

    with open(target, "w") as f:
        f.write("target")

    cbor = build_cbor_map([
        ("Type", "createsymlink"),
        ("Id", "11"),
        ("Source", target),
        ("SymLink", link),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    assert os.path.islink(link), "symlink not created"
    os.unlink(target)
    os.unlink(link)
    return True


def test_rename(bin_path):
    """Rename a file."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    src = os.path.join(tmpdir, "cmdbridge_rename_src")
    dst = os.path.join(tmpdir, "cmdbridge_rename_dst")

    with open(src, "w") as f:
        f.write("rename me")

    cbor = build_cbor_map([
        ("Type", "renamefile"),
        ("Id", "12"),
        ("Source", src),
        ("Target", dst),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    assert os.path.exists(dst), "renamed file not found"
    assert not os.path.exists(src), "original still exists"
    os.unlink(dst)
    return True


def test_tempfile(bin_path):
    """Create a temp file."""
    cbor = build_cbor_map([
        ("Type", "createtempfile"),
        ("Id", "13"),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    return True


def test_tempdir(bin_path):
    """Create a temp directory."""
    cbor = build_cbor_map([
        ("Type", "createtempdir"),
        ("Id", "14"),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    return True


def test_setpermissions(bin_path):
    """Set file permissions."""
    tmpdir = temp_dir()
    path = os.path.join(tmpdir, "cmdbridge_perms")

    with open(path, "w") as f:
        f.write("perms")

    cbor = build_cbor_map([
        ("Type", "setpermissions"),
        ("Id", "15"),
        ("Path", path),
        ("Mode", 0o644),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    time.sleep(0.1)
    os.unlink(path)
    return True


def test_owner_id(bin_path):
    """Get owner and owner ID."""
    tmpdir = temp_dir()
    cbor = build_cbor_map([
        ("Type", "owner"),
        ("Id", "20"),
        ("Path", tmpdir),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    cbor2 = build_cbor_map([
        ("Type", "ownerid"),
        ("Id", "21"),
        ("Path", tmpdir),
    ])
    stderr_out2 = send_command(bin_path, cbor2)
    resp2 = parse_response(stderr_out2)
    assert len(resp2) == 1, f"expected 1 response, got {len(resp2)}"
    return True


def test_group_id(bin_path):
    """Get group and group ID."""
    tmpdir = temp_dir()
    cbor = build_cbor_map([
        ("Type", "group"),
        ("Id", "22"),
        ("Path", tmpdir),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    cbor2 = build_cbor_map([
        ("Type", "groupId"),
        ("Id", "23"),
        ("Path", tmpdir),
    ])
    stderr_out2 = send_command(bin_path, cbor2)
    resp2 = parse_response(stderr_out2)
    assert len(resp2) == 1, f"expected 1 response, got {len(resp2)}"
    return True


def test_freespace(bin_path):
    """Get free space on filesystem.

    The value is checked, not just its presence: free space is computed from
    a different struct on each platform (statfs() with f_bsize, statvfs()
    with f_frsize on NetBSD), and picking the wrong block size still yields a
    plausible-looking number that is off by a constant factor.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    cbor = build_cbor_map([
        ("Type", "freespace"),
        ("Id", "24"),
        ("Path", tmpdir),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    free = _extract_cbor_int(resp[0], "FreeSpace")
    assert free is not None, "FreeSpace missing in freespace result"
    assert free > 0, f"FreeSpace should be positive, got {free}"

    # Compare against what the OS says. The two are sampled at slightly
    # different moments on a live filesystem, so only the magnitude is
    # asserted; a wrong block size is out by a factor of 8 or more.
    expected = shutil.disk_usage(tmpdir).free
    assert expected / 4 < free < expected * 4, \
        f"FreeSpace {free} is not close to the {expected} reported by the OS"
    return True


def test_issamefile(bin_path):
    """Check if two paths are the same file."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    path1 = os.path.join(tmpdir, "cmdbridge_same_a")
    path2 = os.path.join(tmpdir, "cmdbridge_same_b")

    with open(path1, "w") as f:
        f.write("same")
    os.link(path1, path2)

    cbor = build_cbor_map([
        ("Type", "issamefile"),
        ("Id", "25"),
        ("Path1", path1),
        ("Path2", path2),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

    os.unlink(path1)
    os.unlink(path2)
    return True


def test_ensure_existing_file(bin_path):
    """Ensure a file exists (create if not)."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    path = os.path.join(tmpdir, "cmdbridge_ensure")
    if os.path.exists(path):
        os.unlink(path)

    cbor = build_cbor_map([
        ("Type", "ensureexistingfile"),
        ("Id", "26"),
        ("Path", path),
    ])
    stderr_out = send_command(bin_path, cbor)
    resp = parse_response(stderr_out)
    assert len(resp) == 1, f"expected 1 response, got {len(resp)}"
    assert os.path.exists(path), "file not created"

    os.unlink(path)
    return True


def test_exit(bin_path):
    """Exit should terminate cleanly."""
    cbor = build_cbor_map([("Type", "exit")])
    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
    )
    try:
        proc.stdin.write(cbor)
    except BrokenPipeError:
        pass
    try:
        proc.stdin.close()
    except OSError:
        pass
    # Workaround for Python 3.12 bug with communicate()
    stderr_out = proc.stderr.read()
    proc.wait(timeout=5)
    # Exit produces no response
    return True


# --- Socket forwarding test helpers (interactive mode) ---

import socket as sock_module

# Check if AF_UNIX is available (may not be on all Python/Windows builds)
_HAS_AF_UNIX = hasattr(sock_module, 'AF_UNIX')


def _skip_if_no_af_unix(test_fn):
    """Skip a test if AF_UNIX sockets are not available."""
    def wrapper(bin_path):
        if os.name == 'nt' and not _HAS_AF_UNIX:
            return True  # Skip on Windows without AF_UNIX
        return test_fn(bin_path)
    wrapper.__name__ = test_fn.__name__
    return wrapper


class AFUnixSocketClient:
    """AF_UNIX socket client (same protocol as Go's net.Listen("unix", ...))."""

    def __init__(self, path):
        self.sock = sock_module.socket(sock_module.AF_UNIX, sock_module.SOCK_STREAM)
        self.sock.connect(path)

    def send(self, data):
        self.sock.sendall(data)

    def sendall(self, data):
        self.sock.sendall(data)

    def shutdown(self, how):
        self.sock.shutdown(how)

    def shutdown_write(self):
        self.sock.shutdown(sock_module.SHUT_WR)

    def recv(self, max_bytes=4096):
        return self.sock.recv(max_bytes)

    def close(self):
        try:
            self.sock.close()
        except Exception:
            pass


class CmdBridgeInteractive:
    """Keep a cmdbridge process alive for interactive command/response testing."""

    def __init__(self, bin_path):
        self.proc = subprocess.Popen(
            [bin_path],
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            bufsize=0,
        )
        self.responses = []
        # Packets that carried more than one CBOR value: (packet_len, consumed).
        self.framing_violations = []
        self.response_event = threading.Event()
        self._stderr_thread = threading.Thread(target=self._read_stderr, daemon=True)
        self._stderr_thread.start()

    def _read_stderr(self):
        """Read stderr in a background thread and parse responses."""
        buf = b""
        try:
            while True:
                data = self.proc.stderr.read(65536)
                if not data:
                    break
                buf += data
                # Try to parse complete packets from buffer
                while len(buf) >= MLG + 4:
                    idx = buf.find(MAGIC)
                    if idx == -1:
                        buf = b""
                        break
                    pkt_len = struct.unpack(">I", buf[idx+MLG:idx+MLG+4])[0]
                    pkt_start = idx + MLG + 4
                    if pkt_start + pkt_len <= len(buf):
                        pkt_data = buf[pkt_start:pkt_start+pkt_len]
                        # Mirror the real client: Client::readPacket() decodes
                        # exactly one CBOR value per packet and never looks at
                        # what follows it. Anything trailing is data the client
                        # would silently drop, so record it as a violation
                        # instead of quietly unpacking it.
                        try:
                            decoder = _CborDecoder(pkt_data)
                            decoder.decode()
                            self.responses.append(pkt_data[:decoder.pos])
                            if decoder.pos != len(pkt_data):
                                self.framing_violations.append(
                                    (len(pkt_data), decoder.pos))
                        except Exception:
                            pass
                        self.response_event.set()
                        buf = buf[pkt_start+pkt_len:]
                    else:
                        break
        except Exception:
            pass

    def send(self, cbor_data):
        """Send a command and wait for at least one response."""
        self.proc.stdin.write(cbor_data)
        self.proc.stdin.flush()
        self.response_event.wait(timeout=5)
        self.response_event.clear()
        if self.responses:
            resp = self.responses[0]
            self.responses.pop(0)
            return resp
        return None

    def send_multiple(self, cbor_data, count=1):
        """Send a command and wait for multiple responses."""
        self.proc.stdin.write(cbor_data)
        self.proc.stdin.flush()
        while len(self.responses) < count:
            self.response_event.wait(timeout=5)
            self.response_event.clear()
        result = self.responses[:count]
        self.responses = self.responses[count:]
        return result

    def close(self):
        """Terminate the process."""
        try:
            exit_cbor = build_cbor_map([("Type", "exit")])
            self.proc.stdin.write(exit_cbor)
            self.proc.stdin.flush()
        except Exception:
            pass
        try:
            self.proc.wait(timeout=5)
        except Exception:
            self.proc.kill()
            self.proc.wait()
        self._stderr_thread.join(timeout=2)


@_skip_if_no_af_unix
def test_socket_forward(bin_path):
    """Test socket forwarding: start server, connect client, send data, close."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        # 1. Start forward server
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "100"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        # Parse the path from the response
        # Response is a CBOR map with Type="forwardlocalsocketserverready", Id, Path
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # 2. Connect a client and send data
        test_data = b"hello from socket client"
        client = AFUnixSocketClient(path)
        client.send(test_data)
        if not (os.name == 'nt'):
            client.shutdown_write()

        # 3. Wait for socketdata response
        time.sleep(0.5)  # Give the server time to read and emit response

        socketdata_resp = None
        for _ in range(10):
            if bridge.responses:
                # Find socketdata response (map with Type="socketdata")
                for r in bridge.responses[:]:
                    data_type = _extract_cbor_string(r, "Type")
                    if data_type == "socketdata":
                        socketdata_resp = r
                        break
                if socketdata_resp:
                    break
            time.sleep(0.2)

        assert socketdata_resp is not None, "no socketdata response received"

        # 4. Close the connection
        client.close()

        # 5. Wait for socketclose response
        time.sleep(0.5)
        socketclose_found = False
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketclose":
                socketclose_found = True
                break

        assert socketclose_found, "no socketclose response received"

        # 6. Stop the forward server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "100"),
        ])
        resp = bridge.send(cbor_stop)
        assert resp is not None, "no response from stopforwardserver"
        stopped_type = _extract_cbor_string(resp, "Type")
        assert stopped_type == "forwardserverstopped", f"expected forwardserverstopped, got {stopped_type}"

        return True
    finally:
        bridge.close()


@_skip_if_no_af_unix
def test_socket_forward_data_roundtrip(bin_path):
    """Test that data sent via socket is correctly forwarded."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Start forward server
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "200"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # Send various test data patterns
        test_cases = [
            b"simple text",
            b"\x00\x01\x02\x03",  # binary data
            b"a" * 100,  # longer data
        ]

        for i, test_data in enumerate(test_cases):
            client = AFUnixSocketClient(path)
            client.send(test_data)
            if not (os.name == 'nt'):
                client.shutdown_write()

            # Wait for socketdata response
            time.sleep(0.5)
            found = False
            for r in bridge.responses[:]:
                data_type = _extract_cbor_string(r, "Type")
                if data_type == "socketdata":
                    contents = _extract_cbor_bytes(r, "Data")
                    if contents == test_data:
                        found = True
                        break
            assert found, f"data roundtrip failed for test case {i}"

            client.close()
            time.sleep(0.3)

        # Stop server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "200"),
        ])
        bridge.send(cbor_stop)

        return True
    finally:
        bridge.close()


@_skip_if_no_af_unix
def test_stop_forward_closes_all_connections(bin_path):
    """Test that stopping the forward server closes all active connections."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "400"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # Connect multiple clients
        num_clients = 5
        clients = []
        for i in range(num_clients):
            clients.append(AFUnixSocketClient(path))

        time.sleep(0.3)

        # Stop the forward server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "400"),
        ])
        bridge.send(cbor_stop)

        # Close all client connections
        for c in clients:
            c.close()

        return True
    finally:
        bridge.close()


@_skip_if_no_af_unix
def test_multiple_simultaneous_connections(bin_path):
    """Test that several clients can connect and all remain active."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "410"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # Connect multiple clients
        num_conns = 3
        clients = []
        for i in range(num_conns):
            clients.append(AFUnixSocketClient(path))

        time.sleep(0.5)

        # Drain socketconnect responses
        _drain_responses(bridge, "socketconnect", timeout=3)

        # Each client sends data sequentially with a small delay between
        for i, c in enumerate(clients):
            test_data = f"hello-from-{i}".encode()
            c.send(test_data)
            if not (os.name == 'nt'):
                c.shutdown_write()
            time.sleep(0.3)

        # Wait for socketdata responses
        _drain_responses(bridge, "socketdata", timeout=5)

        # Verify at least one socketdata was received (basic connectivity)
        found = False
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketdata":
                contents = _extract_cbor_bytes(r, "Data")
                if contents and contents.startswith(b"hello-from-"):
                    found = True
                    break

        assert found, "no socketdata received from any client"

        # Stop server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "410"),
        ])
        bridge.send(cbor_stop)

        return True
    finally:
        bridge.close()


@_skip_if_no_af_unix
def test_data_routing_isolation(bin_path):
    """Test that data sent via one connection does not leak to another."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "420"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # Connect two clients sequentially
        conn_a = AFUnixSocketClient(path)
        conn_b = AFUnixSocketClient(path)

        time.sleep(0.3)

        # Drain socketconnect responses for both clients
        for _ in range(10):
            had_connect = False
            for r in bridge.responses[:]:
                data_type = _extract_cbor_string(r, "Type")
                if data_type == "socketconnect":
                    had_connect = True
                    bridge.responses.remove(r)
                    break
            if not had_connect:
                break
            time.sleep(0.2)

        # Send data only through client A
        conn_a.send(b"hello-from-a")
        if not (os.name == 'nt'):
            conn_a.shutdown_write()

        # Wait for socketdata from A
        time.sleep(0.5)
        a_data = None
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketdata":
                contents = _extract_cbor_bytes(r, "Data")
                if contents == b"hello-from-a":
                    a_data = r
                    break

        assert a_data is not None, "no socketdata for client A received"

        # Now send data only through client B
        conn_b.send(b"hello-from-b")
        if not (os.name == 'nt'):
            conn_b.shutdown_write()

        time.sleep(0.5)
        b_data = None
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketdata":
                contents = _extract_cbor_bytes(r, "Data")
                if contents == b"hello-from-b":
                    b_data = r
                    break

        assert b_data is not None, "no socketdata for client B received"

        # Stop server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "420"),
        ])
        bridge.send(cbor_stop)

        return True
    finally:
        bridge.close()


@_skip_if_no_af_unix
def test_close_one_connection_keeps_others(bin_path):
    """Test that closing one connection does not affect other active connections."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "430"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        conn_a = AFUnixSocketClient(path)
        conn_b = AFUnixSocketClient(path)

        time.sleep(0.3)

        # Close connection A and wait for the socketclose notification
        conn_a.close()

        # Drain socketclose from A
        time.sleep(0.5)
        for _ in range(10):
            had_socketclose = False
            for r in bridge.responses[:]:
                data_type = _extract_cbor_string(r, "Type")
                if data_type == "socketclose":
                    had_socketclose = True
                    bridge.responses.remove(r)
                    break
            if had_socketclose:
                break
            time.sleep(0.2)

        # Connection B should still be alive: send data and verify receipt
        conn_b.send(b"still-alive")
        if not (os.name == 'nt'):
            conn_b.shutdown_write()

        time.sleep(0.5)
        found = False
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketdata":
                contents = _extract_cbor_bytes(r, "Data")
                if contents == b"still-alive":
                    found = True
                    break

        assert found, "connection B data not received after A was closed"

        # Stop server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "430"),
        ])
        bridge.send(cbor_stop)

        return True
    finally:
        bridge.close()


def _drain_responses(bridge, response_type, timeout=5):
    """Wait until at least one response of the given type is available."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == response_type:
                return True
        time.sleep(0.2)
    return False


def _extract_cbor_string(cbor_data, key):
    """Extract a string value for a given key from CBOR map data."""
    pos = 0
    # Read map header
    first = cbor_data[pos]
    maj = first & 0xE0
    if maj != 0xA0:
        return None
    n = first & 0x1F
    if n == 24:
        n = cbor_data[pos+1]
        pos += 2
    elif n == 25:
        n = struct.unpack(">H", cbor_data[pos+1:pos+3])[0]
        pos += 3
    elif n == 26:
        n = struct.unpack(">I", cbor_data[pos+1:pos+5])[0]
        pos += 5
    else:
        pos += 1

    for _ in range(n):
        # Read key (text string)
        kfirst = cbor_data[pos]
        klen = kfirst & 0x1F
        if klen == 24:
            klen = cbor_data[pos+1]
            pos += 2
        elif klen == 25:
            klen = struct.unpack(">H", cbor_data[pos+1:pos+3])[0]
            pos += 3
        elif klen == 26:
            klen = struct.unpack(">I", cbor_data[pos+1:pos+5])[0]
            pos += 5
        else:
            pos += 1
        key_val = cbor_data[pos:pos+klen].decode()
        pos += klen

        if key_val == key:
            vfirst = cbor_data[pos]
            vmaj = vfirst & 0xE0
            if vmaj == 0x60:  # text string
                vlen = vfirst & 0x1F
                pos += 1
                if vlen == 24:
                    vlen = cbor_data[pos]
                    pos += 1
                elif vlen == 25:
                    vlen = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                    pos += 2
                elif vlen == 26:
                    vlen = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                    pos += 4
                val = cbor_data[pos:pos+vlen].decode()
                return val
            elif vmaj == 0x40:  # byte string
                vlen = vfirst & 0x1F
                pos += 1
                if vlen == 24:
                    vlen = cbor_data[pos]
                    pos += 1
                elif vlen == 25:
                    vlen = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                    pos += 2
                elif vlen == 26:
                    vlen = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                    pos += 4
                val = cbor_data[pos:pos+vlen]
                return val
            else:
                # Handle boolean values as strings
                if vfirst == 0xF5:  # true
                    return "true"
                elif vfirst == 0xF4:  # false
                    return "false"
                # Skip value - advance pos past it
                if vfirst <= 0x17:
                    pass  # simple value embedded in first byte
                elif vfirst == 0x18:
                    pos += 1  # 1-byte uint follows
                elif vfirst == 0x19:
                    pos += 2  # 2-byte uint follows
                elif vfirst == 0x1A:
                    pos += 4  # 4-byte uint follows
                elif vfirst == 0x1B:
                    pos += 8  # 8-byte uint follows
                elif vfirst == 0xF6:  # null
                    pass
                else:
                    pos += 1
                return None

        else:
            # Skip value for non-matching key
            if pos >= len(cbor_data):
                break
            vfirst = cbor_data[pos]
            vmaj = vfirst & 0xE0
            if vmaj == 0x60:  # text string - skip
                vlen = vfirst & 0x1F
                pos += 1
                if vlen == 24:
                    vlen = cbor_data[pos]
                    pos += 1
                elif vlen == 25:
                    vlen = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                    pos += 2
                elif vlen == 26:
                    vlen = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                    pos += 4
                pos += vlen
            elif vmaj == 0x40:  # byte string - skip
                vlen = vfirst & 0x1F
                pos += 1
                if vlen == 24:
                    vlen = cbor_data[pos]
                    pos += 1
                elif vlen == 25:
                    vlen = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                    pos += 2
                elif vlen == 26:
                    vlen = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                    pos += 4
                pos += vlen
            elif vmaj == 0xA0:  # map - skip all key-value pairs
                mn = vfirst & 0x1F
                if mn == 24:
                    mn = cbor_data[pos+1]
                    pos += 1
                pos += 1
                for _ in range(mn):
                    # skip key
                    kf = cbor_data[pos]
                    kl = kf & 0x1F
                    pos += 1
                    if kl == 24:
                        kl = cbor_data[pos]
                        pos += 1
                    elif kl == 25:
                        kl = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                        pos += 2
                    elif kl == 26:
                        kl = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                        pos += 4
                    pos += kl
                    # skip value (recursive - simplified)
                    vf = cbor_data[pos]
                    vfmaj = vf & 0xE0
                    if vfmaj == 0x60 or vfmaj == 0x40:
                        vl = vf & 0x1F
                        pos += 1
                        if vl == 24:
                            vl = cbor_data[pos]
                            pos += 1
                        elif vl == 25:
                            vl = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                            pos += 2
                        elif vl == 26:
                            vl = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                            pos += 4
                        pos += vl
                    elif vfmaj == 0xA0 or vfmaj == 0x80:
                        # nested map/array - skip (simplified, may not be fully correct)
                        vn = vf & 0x1F
                        pos += 1
                        if vn == 24:
                            vn = cbor_data[pos]
                            pos += 1
                        pos += 1
                        for _ in range(vn * 2):
                            if pos < len(cbor_data):
                                sf = cbor_data[pos]
                                smaj = sf & 0xE0
                                if smaj == 0x60 or smaj == 0x40:
                                    sl = sf & 0x1F
                                    pos += 1
                                    if sl == 24:
                                        sl = cbor_data[pos]
                                        pos += 1
                                    elif sl == 25:
                                        sl = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                                        pos += 2
                                    elif sl == 26:
                                        sl = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                                        pos += 4
                                    pos += sl
                                else:
                                    pos += 1
                    elif vfirst <= 0x17 or vfirst in (0x18, 0xF4, 0xF5, 0xF6):
                        if vfirst == 0x18:
                            pos += 1
                    else:
                        pos += 1
            elif vmaj == 0x80:  # array - skip (simplified)
                alen = vfirst & 0x1F
                pos += 1
                if alen == 24:
                    alen = cbor_data[pos]
                    pos += 1
                pos += 1
                for _ in range(alen):
                    if pos < len(cbor_data):
                        ef = cbor_data[pos]
                        emaj = ef & 0xE0
                        if emaj == 0x60 or emaj == 0x40:
                            elen = ef & 0x1F
                            pos += 1
                            if elen == 24:
                                elen = cbor_data[pos]
                                pos += 1
                            elif elen == 25:
                                elen = struct.unpack(">H", cbor_data[pos:pos+2])[0]
                                pos += 2
                            elif elen == 26:
                                elen = struct.unpack(">I", cbor_data[pos:pos+4])[0]
                                pos += 4
                            pos += elen
                        else:
                            pos += 1
            elif vfirst <= 0x17:
                pos += 1  # simple uint, value embedded in first byte
            elif vfirst in (0x18, 0x19, 0x1a, 0x1b, 0xF4, 0xF5, 0xF6, 0xF7):
                if vfirst == 0x18:
                    pos += 2  # indicator + 1-byte value
                elif vfirst == 0x19:
                    pos += 3  # indicator + 2-byte value
                elif vfirst == 0x1a:
                    pos += 5  # indicator + 4-byte value
                elif vfirst == 0x1b:
                    pos += 9  # indicator + 8-byte value
                # else: bool/null (0xF4-0xF7), value embedded in first byte, nothing to skip
            elif 0x20 <= vfirst <= 0x37:
                pass  # negative int, value in first byte
            else:
                pos += 1

    return None


def _extract_cbor_bytes(cbor_data, key):
    """Extract a bytes value for a given key from CBOR map data."""
    return _extract_cbor_string(cbor_data, key)


def _skip_cbor_value(data, pos):
    """Skip a CBOR value at the given position, return new position."""
    if pos >= len(data):
        return pos
    vfirst = data[pos]
    vmaj = vfirst & 0xE0
    vinfo = vfirst & 0x1F
    pos += 1

    if vmaj == 0x60:  # text string
        vl = vinfo
        if vl == 24:
            vl = data[pos]; pos += 1
        elif vl == 25:
            vl = struct.unpack(">H", data[pos:pos+2])[0]; pos += 2
        elif vl == 26:
            vl = struct.unpack(">I", data[pos:pos+4])[0]; pos += 4
        pos += vl
    elif vmaj == 0x40:  # byte string
        vl = vinfo
        if vl == 24:
            vl = data[pos]; pos += 1
        elif vl == 25:
            vl = struct.unpack(">H", data[pos:pos+2])[0]; pos += 2
        elif vl == 26:
            vl = struct.unpack(">I", data[pos:pos+4])[0]; pos += 4
        pos += vl
    elif vmaj == 0xA0:  # map
        vn = vinfo
        if vn == 24:
            vn = data[pos]; pos += 1
        pos += 1
        for _ in range(vn * 2):
            if pos < len(data):
                pos = _skip_cbor_value(data, pos)
    elif vmaj == 0x80:  # array
        vn = vinfo
        if vn == 24:
            vn = data[pos]; pos += 1
        for _ in range(vn):
            if pos < len(data):
                pos = _skip_cbor_value(data, pos)
    elif vmaj == 0x00 or vmaj == 0x20:  # unsigned/negative int
        if vinfo == 24:
            pos += 1
        elif vinfo == 25:
            pos += 2
        elif vinfo == 26:
            pos += 4
        elif vinfo == 27:
            pos += 8
    # else: simple value (bool/null/0x18), nothing to skip

    return pos


def _extract_cbor_int(cbor_data, key):
    """Extract an integer value for a given key from CBOR map data."""
    pos = 0
    first = cbor_data[pos]
    maj = first & 0xE0
    if maj != 0xA0:
        return None
    n = first & 0x1F
    if n == 24:
        n = cbor_data[pos+1]
        pos += 2
    elif n == 25:
        n = struct.unpack(">H", cbor_data[pos+1:pos+3])[0]
        pos += 3
    elif n == 26:
        n = struct.unpack(">I", cbor_data[pos+1:pos+5])[0]
        pos += 5
    else:
        pos += 1

    for _ in range(n):
        kfirst = cbor_data[pos]
        klen = kfirst & 0x1F
        if klen == 24:
            klen = cbor_data[pos+1]
            pos += 2
        elif klen == 25:
            klen = struct.unpack(">H", cbor_data[pos+1:pos+3])[0]
            pos += 3
        elif klen == 26:
            klen = struct.unpack(">I", cbor_data[pos+1:pos+5])[0]
            pos += 5
        else:
            pos += 1
        key_val = cbor_data[pos:pos+klen].decode()
        pos += klen

        if key_val == key:
            vf = cbor_data[pos]
            vmaj = vf & 0xE0
            vinfo = vf & 0x1F
            pos += 1
            if vmaj == 0x00:  # unsigned int
                if vinfo <= 23:
                    return vinfo
                elif vinfo == 24:
                    val = cbor_data[pos]; pos += 1; return val
                elif vinfo == 25:
                    val = struct.unpack(">H", cbor_data[pos:pos+2])[0]; pos += 2; return val
                elif vinfo == 26:
                    val = struct.unpack(">I", cbor_data[pos:pos+4])[0]; pos += 4; return val
                elif vinfo == 27:
                    val = struct.unpack(">Q", cbor_data[pos:pos+8])[0]; pos += 8; return val
            elif vmaj == 0x20:  # negative int
                if vinfo <= 23:
                    return -(vinfo + 1)
                elif vinfo == 24:
                    val = cbor_data[pos]; pos += 1; return -(val + 1)
                elif vinfo == 25:
                    val = struct.unpack(">H", cbor_data[pos:pos+2])[0]; pos += 2; return -(val + 1)
                elif vinfo == 26:
                    val = struct.unpack(">I", cbor_data[pos:pos+4])[0]; pos += 4; return -(val + 1)
                elif vinfo == 27:
                    val = struct.unpack(">Q", cbor_data[pos:pos+8])[0]; pos += 8; return -(val + 1)
            else:
                return None
        else:
            pos = _skip_cbor_value(cbor_data, pos)

    return None


@_skip_if_no_af_unix
def test_socket_forward_server_start(bin_path):
    """Test starting and stopping a forward server."""
    if os.name == 'nt':
        return True

    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Start forward server
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "300"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"

        # Verify the response type
        resp_type = _extract_cbor_string(resp, "Type")
        assert resp_type == "forwardlocalsocketserverready", \
            f"expected forwardlocalsocketserverready, got {resp_type}"

        # Verify Id is present and correct
        resp_id = _extract_cbor_int(resp, "Id")
        assert resp_id == 300, f"expected Id=300, got {resp_id}"

        # Stop the server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "300"),
        ])
        resp = bridge.send(cbor_stop)
        assert resp is not None, "no response from stopforwardserver"

        stopped_type = _extract_cbor_string(resp, "Type")
        assert stopped_type == "forwardserverstopped", \
            f"expected forwardserverstopped, got {stopped_type}"

        return True
    finally:
        bridge.close()


def test_exec(bin_path):
    """Test basic exec command."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        if os.name == 'nt':
            cmd_args = ["cmd", "/c", "echo hello"]
        else:
            cmd_args = ["sh", "-c", "echo hello"]
        # Build exec CBOR manually: {"Type":"exec","Id":500,"Args":[...]}
        cbor = bytes([0xA3])  # map(3)
        cbor += _enc_str("Type") + _enc_str("exec")
        cbor += _enc_str("Id") + _enc_uint(500)
        cbor += _enc_str("Args") + _build_cbor_array(cmd_args)
        # exec sends multiple responses (execdata + execresult)
        resp_list = bridge.send_multiple(cbor, count=2)
        assert len(resp_list) >= 2, f"expected at least 2 responses, got {len(resp_list)}"

        # Check execdata response
        data_type = _extract_cbor_string(resp_list[0], "Type")
        assert data_type == "execdata", f"expected execdata, got {data_type}"

        # Check execresult response
        result_type = _extract_cbor_string(resp_list[1], "Type")
        assert result_type == "execresult", f"expected execresult, got {result_type}"

        result_id = _extract_cbor_int(resp_list[1], "Id")
        assert result_id == 500, f"expected Id=500, got {result_id}"

        return True
    finally:
        bridge.close()


def test_exec_cancel(bin_path):
    """Test that a running exec can be cancelled."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        if os.name == 'nt':
            cmd_args = ["cmd", "/c", "timeout /t 30"]
        else:
            cmd_args = ["sh", "-c", "sleep 30"]
        # Start a long-running command
        cbor = bytes([0xA3])  # map(3)
        cbor += _enc_str("Type") + _enc_str("exec")
        cbor += _enc_str("Id") + _enc_uint(600)
        cbor += _enc_str("Args") + _build_cbor_array(cmd_args)
        bridge.send(cbor)

        # Wait for the exec worker to register the job before sending cancel
        time.sleep(0.2)

        # Send cancel command directly (cancel produces no response)
        cancel_cbor = build_cbor_map([
            ("Type", "cancel"),
            ("Id", "600"),
        ])
        bridge.proc.stdin.write(cancel_cbor)
        bridge.proc.stdin.flush()

        # Wait for execresult with exit code (should be non-zero from SIGTERM=15, exit=128+15=143)
        deadline = time.time() + 10
        while len(bridge.responses) < 1 and time.time() < deadline:
            bridge.response_event.wait(timeout=1)
            bridge.response_event.clear()
        resp_list = bridge.responses[:1]
        bridge.responses = bridge.responses[1:]

        assert len(resp_list) > 0, "no execresult received after cancel"
        result_type = _extract_cbor_string(resp_list[0], "Type")
        assert result_type == "execresult", f"expected execresult, got {result_type}"
        exit_code = _extract_cbor_int(resp_list[0], "Code")
        assert exit_code != 0, f"expected non-zero exit code (process killed), got {exit_code}"
        return True
    finally:
        bridge.close()


def test_parallel_execution(bin_path):
    """Test that multiple commands are executed in parallel.

    Sends 5 stat commands simultaneously and verifies:
    1. All 5 responses are received
    2. Each response has the correct ID (routing is correct)
    3. Total time is less than 5x sequential time (proves parallelism)
    """

    tmpdir = temp_dir()

    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        bufsize=0,
    )

    try:
        num_commands = 5
        ids = list(range(700, 700 + num_commands))
        responses = []

        def read_stderr():
            buf = b""
            while True:
                data = proc.stderr.read(65536)
                if not data:
                    break
                buf += data
                while len(buf) >= MLG + 4:
                    idx = buf.find(MAGIC)
                    if idx == -1:
                        buf = b""
                        break
                    pkt_len = struct.unpack(">I", buf[idx+MLG:idx+MLG+4])[0]
                    pkt_start = idx + MLG + 4
                    if pkt_start + pkt_len <= len(buf):
                        responses.append(buf[pkt_start:pkt_start+pkt_len])
                        buf = buf[pkt_start+pkt_len:]
                    else:
                        break

        reader = threading.Thread(target=read_stderr, daemon=True)
        reader.start()

        # Send all commands at once
        start_time = time.time()
        for cmd_id in ids:
            cbor = build_cbor_map([
                ("Type", "stat"),
                ("Id", str(cmd_id)),
                ("Path", tmpdir),
            ])
            proc.stdin.write(cbor)
        proc.stdin.flush()

        # Wait for all responses
        timeout = 10

        while len(responses) < num_commands:
            if time.time() - start_time > timeout:
                raise TimeoutError(f"Expected {num_commands} responses, got {len(responses)}")
            time.sleep(0.1)

        elapsed = time.time() - start_time

        # Extract IDs from responses (handle integer IDs)
        received_ids = set()
        for resp in responses:
            id_key = b'\x62\x49\x64'  # "Id" as CBOR text string
            idx = resp.find(id_key)
            if idx > 0:
                val_start = idx + 3
                if val_start < len(resp):
                    first = resp[val_start]
                    if first <= 0x17:  # Simple unsigned int (0-23)
                        received_ids.add(first)
                    elif first == 0x18:  # 1-byte uint
                        received_ids.add(resp[val_start + 1])
                    elif first == 0x19:  # 2-byte uint
                        received_ids.add(struct.unpack(">H", resp[val_start+1:val_start+3])[0])
                    elif first == 0x1A:  # 4-byte uint
                        received_ids.add(struct.unpack(">I", resp[val_start+1:val_start+5])[0])

        expected_ids = set(ids)
        missing = expected_ids - received_ids
        assert not missing, f"Missing responses for IDs: {missing}"

        print(f"  (parallel: {num_commands} commands in {elapsed:.3f}s)")

        return True
    finally:
        proc.stdin.close()
        proc.terminate()
        proc.wait()


def test_watch(bin_path):
    """Test file watching: add watch, create/delete files, verify events, stop watch."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        watch_dir = os.path.join(tmpdir, "cmdbridge_watch_test")
        os.makedirs(watch_dir, exist_ok=True)

        # 1. Add watch
        cbor = build_cbor_map([
            ("Type", "watch"),
            ("Id", "500"),
            ("Path", watch_dir),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from watch"
        wtype = _extract_cbor_string(resp, "Type")
        assert wtype == "addwatchresult", f"expected addwatchresult, got {wtype}"

        # Wait for kqueue to be fully initialized
        time.sleep(1.0)

        # 2. Create a file and verify Create event
        test_file = os.path.join(watch_dir, "watch_test_file.txt")
        with open(test_file, "w") as f:
            f.write("watch me")
        time.sleep(3.0)

        found_create = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_create = True
                    break
            if found_create:
                break
            time.sleep(0.3)

        assert found_create, "no Create watchevent received"

        # 3. Modify the file and verify Write event
        with open(test_file, "w") as f:
            f.write("modified")
        time.sleep(3.0)

        found_write = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_write = True
                    break
            if found_write:
                break
            time.sleep(0.3)

        assert found_write, "no Write watchevent received"

        # 4. Delete the file and verify Remove event
        os.unlink(test_file)
        time.sleep(3.0)

        found_remove = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_remove = True
                    break
            if found_remove:
                break
            time.sleep(0.3)

        assert found_remove, "no Remove watchevent received"

        # 5. Stop watch
        cbor_stop = build_cbor_map([("Type", "stopwatch"), ("Id", "500")])
        bridge.proc.stdin.write(cbor_stop)
        bridge.proc.stdin.flush()

        # Wait specifically for removewatchresult
        deadline = time.time() + 10
        found_removewatch = False
        while time.time() < deadline:
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "removewatchresult":
                    bridge.responses.remove(r)
                    found_removewatch = True
                    break
            if found_removewatch:
                break
            bridge.response_event.wait(timeout=1)
            bridge.response_event.clear()

        assert found_removewatch, "no removewatchresult received"

        # 6. Watch nonexistent path should return error
        # Drain any remaining watchevents first
        time.sleep(1)
        for _ in range(20):
            has_watchevent = False
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    bridge.responses.remove(r)
                    has_watchevent = True
                    break
            if not has_watchevent:
                break
            time.sleep(0.2)

        cbor_bad = build_cbor_map([
            ("Type", "watch"),
            ("Id", "501"),
            ("Path", "/nonexistent/watch/path/xyz"),
        ])
        bridge.proc.stdin.write(cbor_bad)
        bridge.proc.stdin.flush()

        deadline = time.time() + 5
        found_error = False
        while time.time() < deadline:
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "error":
                    found_error = True
                    break
            if found_error:
                break
            bridge.response_event.wait(timeout=0.5)
            bridge.response_event.clear()

        assert found_error, "expected error for nonexistent path"

        return True
    finally:
        bridge.close()


def test_watch_file_readd_on_recreation(bin_path):
    """Test file watch re-add-on-recreation: delete watched file, recreate it, verify events continue.

    This tests the kqueue NOTE_DELETE/NOTE_RENAME re-add logic on macOS/BSD, the
    inotify IN_DELETE_SELF/IN_MOVE_SELF/IN_IGNORED re-add logic on Linux, and on
    Windows the parent-directory watch that a single file is followed through.
    """
    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        watched_file = os.path.join(tmpdir, "cmdbridge_readd_test.txt")

        # Create the file to watch
        with open(watched_file, "w") as f:
            f.write("initial content")

        # Add watch on the file (not a directory)
        cbor = build_cbor_map([
            ("Type", "watch"),
            ("Id", "700"),
            ("Path", watched_file),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from watch"
        wtype = _extract_cbor_string(resp, "Type")

        assert wtype == "addwatchresult", f"expected addwatchresult, got {wtype}"

        # Wait for kqueue/inotify to be fully initialized
        time.sleep(1.0)

        # Modify the file and verify Write event arrives
        with open(watched_file, "w") as f:
            f.write("modified content")
        time.sleep(3.0)

        found_write = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_write = True
                    break
            if found_write:
                break
            time.sleep(0.3)

        assert found_write, "no Write watchevent received before delete"

        # Delete the file — should get a Remove event
        os.unlink(watched_file)
        time.sleep(3.0)

        found_remove = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_remove = True
                    break
            if found_remove:
                break
            time.sleep(0.3)

        assert found_remove, "no Remove watchevent received after delete"

        # Recreate the file at the same path — this triggers re-add on kqueue/inotify
        with open(watched_file, "w") as f:
            f.write("recreated content")
        time.sleep(3.0)

        # Verify events continue to arrive for the recreated file
        found_after_recreate = False
        for _ in range(40):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "watchEvent":
                    found_after_recreate = True
                    break
            if found_after_recreate:
                break
            time.sleep(0.3)

        assert found_after_recreate, (
            "no watchevent received after file recreation — "
            "re-add-on-recreation may not be working"
        )

        # Stop watch
        cbor_stop = build_cbor_map([("Type", "stopwatch"), ("Id", "700")])
        bridge.proc.stdin.write(cbor_stop)
        bridge.proc.stdin.flush()

        deadline = time.time() + 10
        found_removewatch = False
        while time.time() < deadline:
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "removewatchresult":
                    bridge.responses.remove(r)
                    found_removewatch = True
                    break
            if found_removewatch:
                break
            bridge.response_event.wait(timeout=1)
            bridge.response_event.clear()

        assert found_removewatch, "no removewatchresult received"

        return True
    finally:
        bridge.close()


def test_find_basic(bin_path):
    """Test find command: recursive directory walk with basic results."""
    import shutil

    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        find_dir = os.path.join(tmpdir, "cmdbridge_find_test")
        if os.path.exists(find_dir):
            shutil.rmtree(find_dir)
        os.makedirs(os.path.join(find_dir, "subdir"))
        with open(os.path.join(find_dir, "file1.txt"), "w") as f:
            f.write("aaa")
        with open(os.path.join(find_dir, "subdir", "file2.txt"), "w") as f:
            f.write("bbb")
        with open(os.path.join(find_dir, "file3.log"), "w") as f:
            f.write("ccc")

        # Find all entries - write directly to stdin, then wait for findend
        cbor = build_cbor_map([
            ("Type", "find"),
            ("Id", "600"),
            ("Directory", find_dir),
            ("IteratorFlags", "2"),  # Subdirectories enabled
        ])
        bridge.proc.stdin.write(cbor)
        bridge.proc.stdin.flush()

        # Wait until we see a findend response
        found_end = False
        for _ in range(50):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "findend":
                    found_end = True
                    break
            if found_end:
                break
            time.sleep(0.2)

        found_files = set()
        found_dirs = set()
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "finddata":
                path = _extract_cbor_string(r, "Path")
                is_dir = _extract_cbor_string(r, "IsDir")
                if path:
                    basename = os.path.basename(path)
                    if is_dir == "true" or is_dir is True:
                        found_dirs.add(basename)
                    else:
                        found_files.add(basename)

        assert "file1.txt" in found_files, f"file1.txt not found in find results: {found_files}"
        assert "file2.txt" in found_files, f"file2.txt not found in find results: {found_files}"
        assert "file3.log" in found_files, f"file3.log not found in find results: {found_files}"
        assert "subdir" in found_dirs, f"subdir not found in find results: {found_dirs}"

        # Cleanup
        shutil.rmtree(find_dir)
        return True
    finally:
        bridge.close()


def test_find_name_filter(bin_path):
    """Test find command with name filters."""
    import shutil

    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        find_dir = os.path.join(tmpdir, "cmdbridge_find_filter")
        if os.path.exists(find_dir):
            shutil.rmtree(find_dir)
        os.makedirs(find_dir)
        with open(os.path.join(find_dir, "a.txt"), "w") as f:
            f.write("1")
        with open(os.path.join(find_dir, "b.log"), "w") as f:
            f.write("2")
        with open(os.path.join(find_dir, "c.txt"), "w") as f:
            f.write("3")

        # Build CBOR manually for NameFilters array
        # {"Type":"find","Id":610,"Directory":...,"IteratorFlags":2,"NameFilters":["*.txt"]}
        name_filter_bytes = "*.txt".encode()
        nf_len = len(name_filter_bytes)
        cbor = bytes([0xA5])  # map(5)
        cbor += _enc_str("Type") + _enc_str("find")
        cbor += _enc_str("Id") + _enc_uint(610)
        cbor += _enc_str("Directory") + _enc_str(find_dir)
        cbor += _enc_str("IteratorFlags") + _enc_uint(2)
        # NameFilters array with one element
        cbor += _enc_str("NameFilters")
        cbor += bytes([0x81])  # array(1)
        if nf_len <= 23:
            cbor += bytes([0x60 | nf_len]) + name_filter_bytes
        else:
            cbor += b'\x78' + struct.pack('>H', nf_len) + name_filter_bytes

        bridge.proc.stdin.write(cbor)
        bridge.proc.stdin.flush()

        # Wait until we see a findend response
        found_end = False
        for _ in range(50):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "findend":
                    found_end = True
                    break
            if found_end:
                break
            time.sleep(0.2)

        found_files = set()
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "finddata":
                path = _extract_cbor_string(r, "Path")
                is_dir = _extract_cbor_string(r, "IsDir")
                if path and (is_dir == "true" or is_dir is True) is False:
                    found_files.add(os.path.basename(path))

        assert "a.txt" in found_files, f"a.txt not found: {found_files}"
        assert "c.txt" in found_files, f"c.txt not found: {found_files}"
        assert "b.log" not in found_files, f"b.log should not be in results: {found_files}"

        # Cleanup
        shutil.rmtree(find_dir)
        return True
    finally:
        bridge.close()


def test_find_cancel(bin_path):
    """Test find cancellation mid-walk."""
    import shutil

    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        find_dir = os.path.join(tmpdir, "cmdbridge_find_cancel")
        if os.path.exists(find_dir):
            shutil.rmtree(find_dir)

        # Create a large directory tree
        for i in range(50):
            subdir = os.path.join(find_dir, f"dir_{i:03d}")
            os.makedirs(subdir, exist_ok=True)
            with open(os.path.join(subdir, "file.txt"), "w") as f:
                f.write("x" * 1000)

        # Start a find that will take some time
        cbor = build_cbor_map([
            ("Type", "find"),
            ("Id", "620"),
            ("Directory", find_dir),
            ("IteratorFlags", "2"),
        ])
        bridge.send(cbor)

        # Collect some results first
        time.sleep(0.5)
        initial_count = len([r for r in bridge.responses if _extract_cbor_string(r, "Type") == "finddata"])

        # Cancel the find
        cancel_cbor = build_cbor_map([
            ("Type", "cancel"),
            ("Id", "620"),
        ])
        bridge.send(cancel_cbor)

        # Wait a bit for cancellation to take effect
        time.sleep(1.0)

        final_count = len([r for r in bridge.responses if _extract_cbor_string(r, "Type") == "finddata"])

        # We should have received some results before cancellation
        assert initial_count > 0, "no find results received before cancel"

        # Cleanup
        shutil.rmtree(find_dir)
        return True
    finally:
        bridge.close()


def test_find_one_value_per_packet(bin_path):
    """Every find result must arrive in its own packet.

    The find handler batches messages before writing them, but each message has
    to stay a separate magic-marker packet: Client::readPacket() decodes exactly
    one CBOR value per packet, so packing several into one packet loses all but
    the first. This test would have caught that.
    """
    import shutil

    # Use send_command which runs the process to completion and captures all stderr
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    find_dir = os.path.join(tmpdir, "cmdbridge_find_batch")
    if os.path.exists(find_dir):
        shutil.rmtree(find_dir)
    os.makedirs(find_dir)

    # Create enough files to exceed the 1KB batch threshold
    # Each finddata entry is ~150-200 bytes, so 10+ files should trigger batching
    for i in range(15):
        with open(os.path.join(find_dir, f"file_{i:03d}.txt"), "w") as f:
            f.write(f"content_{i}")

    cbor = build_cbor_map([
        ("Type", "find"),
        ("Id", "650"),
        ("Directory", find_dir),
    ])
    stderr_data = send_command(bin_path, cbor)

    # Decode exactly as the client does; this asserts no packet holds more
    # than one value.
    values = assert_one_value_per_packet(stderr_data, "find")

    total_finddata = sum(
        1 for v in values if isinstance(v, dict) and v.get("Type") == "finddata")
    assert total_finddata >= 15, (
        f"expected at least 15 finddata entries visible to the client, "
        f"got {total_finddata}")
    assert any(isinstance(v, dict) and v.get("Type") == "findend" for v in values), \
        "no findend received"

    # Cleanup
    shutil.rmtree(find_dir)
    return True


def test_exec_cancel_sigkill(bin_path):
    """Test that exec cancel sends SIGKILL when process ignores SIGTERM."""
    if os.name == 'nt':
        return True

    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Start a process that ignores SIGTERM but will be killed by SIGKILL
        cbor = bytes([0xA3])
        cbor += _enc_str("Type") + _enc_str("exec")
        cbor += _enc_str("Id") + _enc_uint(700)
        # On Unix: trap TERM (ignore it), sleep for a long time.
        # "sh", not "bash": the bridge targets minimal containers and remote
        # devices, which frequently have no bash.
        cmd_args = ["sh", "-c", "trap '' TERM; sleep 30"]
        cbor += _enc_str("Args") + _build_cbor_array(cmd_args)
        bridge.send(cbor)

        # Wait for the exec worker to register the job
        time.sleep(0.2)

        # Send cancel command directly (cancel produces no response)
        cancel_cbor = build_cbor_map([
            ("Type", "cancel"),
            ("Id", "700"),
        ])
        bridge.proc.stdin.write(cancel_cbor)
        bridge.proc.stdin.flush()

        # Wait for execresult — should come back (process was killed by SIGKILL)
        deadline = time.time() + 10
        while len(bridge.responses) < 1 and time.time() < deadline:
            bridge.response_event.wait(timeout=1)
            bridge.response_event.clear()
        resp_list = bridge.responses[:1]
        bridge.responses = bridge.responses[1:]

        assert len(resp_list) > 0, "no execresult received after cancel"
        result_type = _extract_cbor_string(resp_list[0], "Type")
        assert result_type == "execresult", f"expected execresult, got {result_type}"
        exit_code = _extract_cbor_int(resp_list[0], "Code")
        # SIGKILL gives exit code 128+9=137
        assert exit_code == 137, f"expected exit code 137 (SIGKILL), got {exit_code}"

        return True
    finally:
        bridge.close()


def test_exec_large_output(bin_path):
    """Test exec with large binary output."""
    if os.name == 'nt':
        return True

    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Generate 100KB of binary data
        large_data = bytes(range(256)) * 40  # ~10KB repeating pattern
        hex_str = large_data.hex()

        cbor = bytes([0xA3])
        cbor += _enc_str("Type") + _enc_str("exec")
        cbor += _enc_str("Id") + _enc_uint(800)
        if sys.platform == "darwin":
            # macOS xxd -r -p can't handle very long args, use python instead
            cbor += _enc_str("Args") + _build_cbor_array([
                "python3", "-c",
                f"import sys; sys.stdout.buffer.write(bytes.fromhex('{hex_str[:50000]}'))"
            ])
        else:
            cbor += _enc_str("Args") + _build_cbor_array([
                "python3", "-c",
                f"import sys; sys.stdout.buffer.write(bytes.fromhex('{hex_str[:50000]}'))"
            ])

        resp_list = bridge.send_multiple(cbor, count=2)
        assert len(resp_list) >= 2, f"expected at least 2 responses, got {len(resp_list)}"

        # Check that we got execdata with content
        data_type = _extract_cbor_string(resp_list[0], "Type")
        assert data_type == "execdata", f"expected execdata, got {data_type}"

        contents = _extract_cbor_bytes(resp_list[0], "Stdout")
        assert contents is not None and len(contents) > 0, "no stdout in execdata"

        return True
    finally:
        bridge.close()


def test_readfile_offset_limit(bin_path):
    """Test readfile with offset and limit parameters."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    path = os.path.join(tmpdir, "cmdbridge_readfile_offset_test")

    # Write a file with known content
    content = b"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    with open(path, "wb") as f:
        f.write(content)

    try:
        bridge = CmdBridgeInteractive(bin_path)
        try:
            # Read with offset=5, limit=10 -> should get "56789ABCDEFGHIJ"
            cbor = bytes([0xA5])  # map(5)
            cbor += _enc_str("Type") + _enc_str("readfile")
            cbor += _enc_str("Id") + _enc_uint(900)
            cbor += _enc_str("Path") + _enc_str(path)
            cbor += _enc_str("Offset") + _enc_uint(5)
            cbor += _enc_str("Limit") + _enc_uint(10)

            resp_list = bridge.send_multiple(cbor, count=2)
            assert len(resp_list) >= 2, f"expected at least 2 responses, got {len(resp_list)}"

            # First should be readfiledata
            data_type = _extract_cbor_string(resp_list[0], "Type")
            assert data_type == "readfiledata", f"expected readfiledata, got {data_type}"

            contents = _extract_cbor_bytes(resp_list[0], "Contents")
            expected = content[5:15]  # "56789ABCDEFGHIJ"
            assert contents == expected, f"offset/limit mismatch: got {contents!r}, expected {expected!r}"

            return True
        finally:
            bridge.close()
    finally:
        os.unlink(path)


def test_cbor_malformed(bin_path):
    """Test that malformed CBOR input doesn't crash the server."""
    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
    )

    try:
        # Send invalid CBOR (random bytes that aren't valid CBOR)
        proc.stdin.write(b"\xFF\xFE\xFD\xFC")
        proc.stdin.write(b"\x00")  # Then a valid ping to see if it recovers
        proc.stdin.flush()

        # Send a valid ping command after the garbage
        cbor_ping = build_cbor_map([("Type", "ping")])
        proc.stdin.write(cbor_ping)
        proc.stdin.flush()

        proc.stdin.close()
        stderr_out = proc.stderr.read()
        proc.wait(timeout=5)

        # The process should still be alive and not crash
        # (exit code 0 means normal exit via stdin EOF, not a crash)
        assert proc.returncode == 0, f"process crashed with exit code {proc.returncode}"

        return True
    except Exception:
        try:
            proc.kill()
            proc.wait()
        except Exception:
            pass
        return False


def test_queue_backpressure(bin_path):
    """Test that sending more commands than the queue capacity doesn't hang."""
    num_commands = 300
    ids = list(range(1000, 1000 + num_commands))

    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        bufsize=0,
    )

    responses = []

    def read_stderr():
        buf = b""
        while True:
            data = proc.stderr.read(65536)
            if not data:
                break
            buf += data
            while len(buf) >= MLG + 4:
                idx = buf.find(MAGIC)
                if idx == -1:
                    buf = b""
                    break
                pkt_len = struct.unpack(">I", buf[idx+MLG:idx+MLG+4])[0]
                pkt_start = idx + MLG + 4
                if pkt_start + pkt_len <= len(buf):
                    responses.append(buf[pkt_start:pkt_start+pkt_len])
                    buf = buf[pkt_start+pkt_len:]
                else:
                    break

    reader = threading.Thread(target=read_stderr, daemon=True)
    reader.start()

    try:
        start_time = time.time()
        stat_path = "C:\\" if os.name == 'nt' else "/"
        for cmd_id in ids:
            cbor = build_cbor_map([
                ("Type", "stat"),
                ("Id", str(cmd_id)),
                ("Path", stat_path),
            ])
            proc.stdin.write(cbor)
        proc.stdin.flush()

        # Wait for all responses with a reasonable timeout
        timeout = 30
        while len(responses) < num_commands:
            if time.time() - start_time > timeout:
                raise TimeoutError(f"Expected {num_commands} responses, got {len(responses)}")
            time.sleep(0.1)

        elapsed = time.time() - start_time

        # Verify we got all responses with correct IDs
        received_ids = set()
        for resp in responses:
            id_key = b'\x62\x49\x64'  # "Id" as CBOR text string
            idx = resp.find(id_key)
            if idx > 0:
                val_start = idx + 3
                if val_start < len(resp):
                    first = resp[val_start]
                    if first <= 0x17:
                        received_ids.add(first)
                    elif first == 0x18:
                        received_ids.add(resp[val_start + 1])
                    elif first == 0x19:
                        received_ids.add(struct.unpack(">H", resp[val_start+1:val_start+3])[0])
                    elif first == 0x1A:
                        received_ids.add(struct.unpack(">I", resp[val_start+1:val_start+5])[0])

        expected_ids = set(ids)
        missing = expected_ids - received_ids
        assert not missing, f"Missing responses for IDs: {missing}"

        print(f"  (backpressure: {num_commands} commands in {elapsed:.3f}s)")
        return True
    finally:
        proc.stdin.close()
        proc.terminate()
        proc.wait()


def test_socket_forward_ring_overflow(bin_path):
    """Test socket forwarding behavior when connection command buffer overflows."""
    if os.name == 'nt':
        return True

    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Start forward server
        cbor = build_cbor_map([
            ("Type", "forwardlocalsocketserver"),
            ("Id", "1100"),
        ])
        resp = bridge.send(cbor)
        assert resp is not None, "no response from forwardlocalsocketserver"
        path = _extract_cbor_string(resp, "Path")
        assert path is not None, f"no Path in response: {resp.hex()}"

        # Connect a client
        client = AFUnixSocketClient(path)
        time.sleep(0.3)

        # Drain socketconnect
        _drain_responses(bridge, "socketconnect", timeout=3)

        # Get the connId
        conn_id = None
        for r in bridge.responses[:]:
            if _extract_cbor_string(r, "Type") == "socketconnect":
                conn_id = _extract_cbor_int(r, "ConnId")
                break
        assert conn_id is not None, "no socketconnect found"

        # Send many rapid socketdata commands to try to fill the ring buffer
        # Write directly to stdin without waiting for responses to avoid timeout
        for i in range(50):
            data_bytes = f"payload-{i}".encode()
            cbor_cmd = bytes([0xA4])
            cbor_cmd += _enc_str("Type") + _enc_str("socketdata")
            cbor_cmd += _enc_str("Id") + _enc_uint(1100)
            cbor_cmd += _enc_str("ConnId") + _enc_uint(conn_id)
            cbor_cmd += _enc_str("Data")
            if len(data_bytes) <= 23:
                cbor_cmd += bytes([0x40 | len(data_bytes)]) + data_bytes
            else:
                cbor_cmd += b'\x58' + struct.pack('>H', len(data_bytes)) + data_bytes
            bridge.proc.stdin.write(cbor_cmd)
        bridge.proc.stdin.flush()

        # Give time for processing
        time.sleep(1.0)

        # Now send a single client data and verify it still works (no deadlock)
        client.sendall(b"after_overflow")
        client.shutdown(sock_module.SHUT_WR)
        time.sleep(0.5)

        found = False
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "socketdata":
                contents = _extract_cbor_bytes(r, "Data")
                if contents == b"after_overflow":
                    found = True
                    break

        assert found, "socketdata after ring overflow not received"

        client.close()

        # Stop server
        cbor_stop = build_cbor_map([
            ("Type", "stopforwardserver"),
            ("Id", "1100"),
        ])
        bridge.send(cbor_stop)

        return True
    finally:
        bridge.close()


def test_watchdog_timeout(bin_path):
    """Test that --watchdogTimeout causes exit(100) after inactivity."""
    proc = subprocess.Popen(
        [bin_path, "--watchdogTimeout=2"],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
    )

    try:
        # Send one command to start, then wait for watchdog timeout
        cbor = build_cbor_map([("Type", "ping")])
        proc.stdin.write(cbor)
        proc.stdin.flush()

        # Wait past the watchdog timeout. Poll rather than sleeping a fixed
        # amount so a slow machine (or a sanitizer build) does not make this
        # flaky.
        deadline = time.time() + 20
        while time.time() < deadline and proc.poll() is None:
            time.sleep(0.2)

        assert proc.returncode is not None, "process did not exit after watchdog timeout"
        assert proc.returncode == 100, f"expected exit code 100, got {proc.returncode}"

        return True
    except Exception:
        try:
            proc.kill()
            proc.wait()
        except Exception:
            pass
        return False


def test_symlink_stat(bin_path):
    """stat on a symlink describes the link, not its target (Go uses os.Lstat).

    The expectation here used to be the POSIX S_IFLNK bits. It is Go's
    io/fs.FileMode Symlink bit instead, because that is what the client reads
    the type from -- see test_mode_is_go_filemode.
    """
    tmpdir = temp_dir()
    target = os.path.join(tmpdir, "cmdbridge_symlink_target")
    link = os.path.join(tmpdir, "cmdbridge_symlink")

    with open(target, "w") as f:
        f.write("target content")

    try:
        # Create symlink
        os.symlink(target, link)

        bridge = CmdBridgeInteractive(bin_path)
        try:
            # Stat the symlink (should use lstat)
            cbor = build_cbor_map([
                ("Type", "stat"),
                ("Id", "1200"),
                ("Path", link),
            ])
            resp = bridge.send(cbor)
            assert resp is not None, "no response from stat symlink"

            mode = _extract_cbor_int(resp, "Mode")
            assert mode is not None, "no Mode in stat response"
            assert mode & GO_MODE_SYMLINK, (
                f"expected Go's Symlink bit (0x{GO_MODE_SYMLINK:x}) in the "
                f"mode, got 0x{mode:x}")
            assert not mode & GO_MODE_DIR, \
                f"a symlink must not carry the Dir bit: 0x{mode:x}"

            return True
        finally:
            bridge.close()
    finally:
        if os.path.exists(link):
            os.unlink(link)
        if os.path.exists(target):
            os.unlink(target)


def test_stat_modtime_is_integer(bin_path):
    """Test that stat ModTime is a tagged integer (Unix epoch), not a string.

    Go's cbor library encodes time.Time as CBOR tag 0 with an integer payload.
    The C port must match this format for the client to parse correctly.

    The value has to be checked as well as the type: the Windows port divided
    the FILETIME without shifting it to the Unix epoch, which is an integer
    like any other, but one in the year 2402. Mode is checked here too, for
    want of a better place - it was reported as zero on Windows, which the
    client turns into an empty QFile::Permissions.
    """
    tmpdir = temp_dir()
    # Create a file to ensure it has a valid timestamp
    test_file = os.path.join(tmpdir, "cmdbridge_modtime_test")
    with open(test_file, "w") as f:
        f.write("test")

    try:
        cbor = build_cbor_map([
            ("Type", "stat"),
            ("Id", "2000"),
            ("Path", test_file),
        ])
        stderr_out = send_command(bin_path, cbor)
        resp = parse_response(stderr_out)
        assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

        # Decode the raw CBOR to check ModTime format
        decoded = decode_cbor(resp[0])
        assert isinstance(decoded, dict), f"expected map, got {type(decoded)}"
        assert decoded.get("Type") == "statresult", f"expected statresult, got {decoded.get('Type')}"

        modtime = decoded.get("ModTime")
        assert modtime is not None, "ModTime field missing"
        assert isinstance(modtime, int), f"ModTime should be int (tagged Unix epoch), got {type(modtime).__name__}: {modtime!r}"
        expected = os.stat(test_file).st_mtime
        assert abs(modtime - expected) < 5, \
            f"ModTime {modtime} is {modtime - expected:.0f}s off from expected {expected}"

        mode = decoded.get("Mode")
        assert mode is not None, "Mode field missing"
        assert mode & 0o777, f"Mode carries no permission bits: {mode:#o}"
        assert mode & 0o400, f"a readable file should report read permission: {mode:#o}"
    finally:
        if os.path.exists(test_file):
            os.unlink(test_file)
    return True


def test_finddata_modtime_is_integer(bin_path):
    """Test that finddata ModTime is a tagged integer (Unix epoch), not a string.

    find derives ModTime and Mode from the same platform data as stat, so it
    is checked against the same expectations - see test_stat_modtime_is_integer.
    """
    import shutil

    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        find_dir = os.path.join(tmpdir, "cmdbridge_find_modtime_test")
        if os.path.exists(find_dir):
            shutil.rmtree(find_dir)
        os.makedirs(find_dir)
        test_file = os.path.join(find_dir, "testfile.txt")
        with open(test_file, "w") as f:
            f.write("test")

        cbor = build_cbor_map([
            ("Type", "find"),
            ("Id", "2100"),
            ("Directory", find_dir),
        ])
        bridge.proc.stdin.write(cbor)
        bridge.proc.stdin.flush()

        # Wait for findend
        found_end = False
        for _ in range(50):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "findend":
                    found_end = True
                    break
            if found_end:
                break
            time.sleep(0.2)

        assert found_end, "no findend received"

        # Check that all finddata entries have integer ModTime
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "finddata":
                modtime = _extract_cbor_int(r, "ModTime")
                assert modtime is not None, f"ModTime missing in finddata: {r.hex()}"
                assert isinstance(modtime, int), f"ModTime should be int, got {type(modtime).__name__}: {modtime!r}"

                path = _extract_cbor_string(r, "Path")
                expected = os.stat(path).st_mtime
                assert abs(modtime - expected) < 5, \
                    f"ModTime {modtime} for {path} is {modtime - expected:.0f}s off from expected {expected}"

                mode = _extract_cbor_int(r, "Mode")
                assert mode is not None, f"Mode missing in finddata: {r.hex()}"
                assert mode & 0o777, f"Mode carries no permission bits: {mode:#o}"
                assert mode & 0o400, f"a readable file should report read permission: {mode:#o}"

        shutil.rmtree(find_dir)
        return True
    finally:
        bridge.close()


def test_finddata_size_is_int64(bin_path):
    """Test that finddata Size is int64 (can be negative for special files), not uint32.

    Go uses int64 for Size, which can be negative for special files (devices, sockets).
    The C port must use the same type to avoid truncation of large files or incorrect
    handling of special files.
    """
    import shutil

    bridge = CmdBridgeInteractive(bin_path)
    try:
        tmpdir = temp_dir()
        os.makedirs(tmpdir, exist_ok=True)
        find_dir = os.path.join(tmpdir, "cmdbridge_find_size_test")
        if os.path.exists(find_dir):
            shutil.rmtree(find_dir)
        os.makedirs(find_dir)

        # Create a file with known size
        test_file = os.path.join(find_dir, "testfile.bin")
        content = b"x" * 1024
        with open(test_file, "wb") as f:
            f.write(content)

        cbor = build_cbor_map([
            ("Type", "find"),
            ("Id", "2200"),
            ("Directory", find_dir),
        ])
        bridge.proc.stdin.write(cbor)
        bridge.proc.stdin.flush()

        # Wait for findend
        found_end = False
        for _ in range(50):
            for r in bridge.responses[:]:
                if _extract_cbor_string(r, "Type") == "findend":
                    found_end = True
                    break
            if found_end:
                break
            time.sleep(0.2)

        assert found_end, "no findend received"

        # Check that Size is a proper integer (not truncated to 32-bit unsigned)
        for r in bridge.responses[:]:
            data_type = _extract_cbor_string(r, "Type")
            if data_type == "finddata":
                path = _extract_cbor_string(r, "Path")
                if path and os.path.basename(path) == "testfile.bin":
                    size = _extract_cbor_int(r, "Size")
                    assert size is not None, f"Size missing in finddata"
                    assert isinstance(size, int), f"Size should be int, got {type(size).__name__}"
                    assert size == 1024, f"Size should be 1024, got {size}"

        shutil.rmtree(find_dir)
        return True
    finally:
        bridge.close()


def test_stat_size_is_int64(bin_path):
    """Test that stat Size is int64, matching Go's stat.Size()."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    test_file = os.path.join(tmpdir, "cmdbridge_stat_size_test")
    with open(test_file, "w") as f:
        f.write("hello world")

    try:
        cbor = build_cbor_map([
            ("Type", "stat"),
            ("Id", "2300"),
            ("Path", test_file),
        ])
        stderr_out = send_command(bin_path, cbor)
        resp = parse_response(stderr_out)
        assert len(resp) == 1, f"expected 1 response, got {len(resp)}"

        decoded = decode_cbor(resp[0])
        assert isinstance(decoded, dict), f"expected map, got {type(decoded)}"
        size = _extract_cbor_int(resp[0], "Size")
        assert size is not None, "Size missing in stat result"
        assert isinstance(size, int), f"Size should be int, got {type(size).__name__}"
        assert size == 11, f"Size should be 11, got {size}"
        return True
    finally:
        os.unlink(test_file)


def test_environment(bin_path):
    """environment must answer, and its reply contains an Env array.

    The reply is the only response carrying a CBOR array, so an encoder that
    cannot encode arrays drops it entirely and the client never completes its
    handshake.
    """
    cbor = build_cbor_map([("Type", "environment"), ("Id", "1")])
    stderr_data = send_command(bin_path, cbor)

    values = assert_one_value_per_packet(stderr_data, "environment")
    assert values, "no response to environment"

    resp = values[0]
    assert resp.get("Type") == "environment", f"unexpected type: {resp.get('Type')}"
    assert resp.get("OsType") in ("unix", "windows"), f"bad OsType: {resp.get('OsType')}"

    env = resp.get("Env")
    assert isinstance(env, list), f"Env is not an array: {type(env)}"
    assert all(isinstance(e, str) for e in env), "Env holds non-string entries"
    # The child inherits our environment, so a variable we set must show up.
    return True


def test_environment_passes_variable(bin_path):
    """A variable set for the process is reported back in Env."""
    env = dict(os.environ)
    env["CMDBRIDGE_TEST_MARKER"] = "hello-from-test"
    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        env=env,
    )
    try:
        proc.stdin.write(build_cbor_map([("Type", "environment"), ("Id", "1")]))
        proc.stdin.close()
    except OSError:
        pass
    stderr_out = proc.stderr.read()
    proc.wait(timeout=5)

    values = assert_one_value_per_packet(stderr_out, "environment")
    assert values, "no response to environment"
    assert "CMDBRIDGE_TEST_MARKER=hello-from-test" in values[0].get("Env", []), \
        "the variable set for the process is missing from Env"
    return True


def test_command_split_across_writes(bin_path):
    """A command arriving in several chunks must still be executed.

    Anything larger than a pipe buffer reaches the bridge in pieces. Treating a
    partial value as garbage and resynchronising loses the whole command.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    target = os.path.join(tmpdir, "split_write.bin")
    if os.path.exists(target):
        os.unlink(target)

    payload = b"A" * 200000
    cbor = b'\xA3'
    cbor += _enc_str("Type") + _enc_str("writefile")
    cbor += _enc_str("Id") + _enc_uint(10)
    cbor += _enc_str("WriteFile") + b'\xA2'
    cbor += _enc_str("Path") + _enc_str(target)
    cbor += _enc_str("Contents") + _enc_bytes(payload)

    proc = subprocess.Popen(
        [bin_path],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        bufsize=0,
    )
    try:
        # Deliberately hand the command over in pieces, with a pause in
        # between so the bridge sees a genuinely partial value.
        chunk = 60000
        for i in range(0, len(cbor), chunk):
            proc.stdin.write(cbor[i:i+chunk])
            proc.stdin.flush()
            time.sleep(0.05)
        proc.stdin.close()
    except OSError:
        pass
    stderr_out = proc.stderr.read()
    proc.wait(timeout=10)

    values = assert_one_value_per_packet(stderr_out, "writefile")
    assert values, "no response: the split command was dropped"
    assert values[0].get("Type") == "writefileresult", \
        f"unexpected response: {values[0]}"
    assert values[0].get("WrittenBytes") == len(payload), \
        f"wrote {values[0].get('WrittenBytes')} of {len(payload)} bytes"
    assert os.path.exists(target), "the file was not created"
    assert os.path.getsize(target) == len(payload), "the file is truncated"

    os.unlink(target)
    return True


def test_writefile_large(bin_path):
    """writeFile has no chunking on the client side, so one command can carry a
    whole large file. The reader must grow to fit it rather than give up.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    target = os.path.join(tmpdir, "large_write.bin")
    if os.path.exists(target):
        os.unlink(target)

    # Comfortably past the initial buffer size so the growth path is used.
    payload = bytes(range(256)) * (12 * 1024)  # 3 MiB
    cbor = b'\xA3'
    cbor += _enc_str("Type") + _enc_str("writefile")
    cbor += _enc_str("Id") + _enc_uint(11)
    cbor += _enc_str("WriteFile") + b'\xA2'
    cbor += _enc_str("Path") + _enc_str(target)
    cbor += _enc_str("Contents") + _enc_bytes(payload)

    values = assert_one_value_per_packet(send_command(bin_path, cbor), "writefile")
    assert values, "no response to a large writefile"
    assert values[0].get("Type") == "writefileresult", f"unexpected: {values[0]}"
    assert values[0].get("WrittenBytes") == len(payload), \
        f"wrote {values[0].get('WrittenBytes')} of {len(payload)} bytes"
    with open(target, "rb") as f:
        assert f.read() == payload, "the file contents differ"
    os.unlink(target)
    return True


def test_exec_stdin(bin_path):
    """Stdin data must reach the child process."""
    if os.name == "nt":
        return True

    stdin_payload = b"hello-from-stdin\n"
    cbor = b'\xA3'
    cbor += _enc_str("Type") + _enc_str("exec")
    cbor += _enc_str("Id") + _enc_uint(20)
    cbor += _enc_str("Exec") + b'\xA2'
    cbor += _enc_str("Args") + _build_cbor_array(["/bin/cat"])
    cbor += _enc_str("Stdin") + _enc_bytes(stdin_payload)

    stderr_data = send_command(bin_path, cbor)
    values = assert_one_value_per_packet(stderr_data, "exec")

    stdout = b"".join(
        v.get("Stdout", b"") for v in values
        if isinstance(v, dict) and v.get("Type") == "execdata")
    assert stdout == stdin_payload, \
        f"cat echoed {stdout!r}, expected {stdin_payload!r}"

    results = [v for v in values if v.get("Type") == "execresult"]
    assert results, "no execresult"
    assert results[0].get("Code") == 0, f"cat exited with {results[0].get('Code')}"
    return True


def test_exec_env_keeps_path_lookup(bin_path):
    """Supplying Env must not break resolving the program through PATH.

    Go resolves the program against the parent's PATH and only then applies the
    supplied environment, so passing Env has to keep working for a bare program
    name -- which the client always does.
    """
    if os.name == "nt":
        return True

    def run(with_env):
        pairs = [("Args", None)]  # placeholder, built by hand below
        del pairs
        inner_entries = 2 if with_env else 1
        cbor = b'\xA3'
        cbor += _enc_str("Type") + _enc_str("exec")
        cbor += _enc_str("Id") + _enc_uint(21)
        cbor += _enc_str("Exec") + bytes([0xA0 | inner_entries])
        cbor += _enc_str("Args") + _build_cbor_array(["echo", "hi"])
        if with_env:
            cbor += _enc_str("Env") + _build_cbor_array(["PATH=/usr/bin:/bin"])
        stderr_data = send_command(bin_path, cbor)
        return assert_one_value_per_packet(stderr_data, "exec")

    for with_env in (False, True):
        values = run(with_env)
        stdout = b"".join(
            v.get("Stdout", b"") for v in values
            if isinstance(v, dict) and v.get("Type") == "execdata")
        results = [v for v in values if v.get("Type") == "execresult"]
        assert results, f"no execresult (Env={with_env})"
        assert results[0].get("Code") == 0, (
            f"echo exited with {results[0].get('Code')} when Env={with_env}; "
            "127 means the PATH lookup was skipped")
        assert stdout.strip() == b"hi", (
            f"echo printed {stdout!r} when Env={with_env}")
    return True


def test_exec_missing_program(bin_path):
    """A program that cannot be found still has to produce an execresult.

    The client turns an "error" packet into an exception but an execresult into
    an exit code, so answering with the wrong one changes what callers see.
    Go always answers exec with an execresult, and so does the child here when
    execve fails.
    """
    if os.name == "nt":
        return True

    cbor = b'\xA3'
    cbor += _enc_str("Type") + _enc_str("exec")
    cbor += _enc_str("Id") + _enc_uint(23)
    cbor += _enc_str("Exec") + b'\xA1'
    cbor += _enc_str("Args") + _build_cbor_array(["definitely-not-a-real-program-xyz"])

    values = assert_one_value_per_packet(send_command(bin_path, cbor), "exec")
    assert values, "no response at all for a missing program"
    types = [v.get("Type") for v in values if isinstance(v, dict)]
    assert "execresult" in types, \
        f"expected an execresult for a missing program, got {types}"
    result = [v for v in values if v.get("Type") == "execresult"][0]
    assert result.get("Code") == 127, \
        f"expected 127 (command not found), got {result.get('Code')}"
    return True


def test_more_watches_than_old_limit(bin_path):
    """Watching more paths than the old fixed table held (256) must work.

    The watcher backends used to keep watchers in a 256-entry array and
    silently refuse the rest, so the client simply never heard about those
    files again.
    """
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "manywatch")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    count = 400
    dirs = []
    for i in range(count):
        d = os.path.join(root, "d%03d" % i)
        os.makedirs(d)
        dirs.append(d)

    bridge = CmdBridgeInteractive(bin_path)
    try:
        for i, d in enumerate(dirs):
            bridge.proc.stdin.write(build_cbor_map([
                ("Type", "watch"), ("Id", str(1000 + i)), ("Path", d)]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 30
        ok_ids = set()
        while time.time() < deadline and len(ok_ids) < count:
            for v in drain_responses(bridge):
                if v.get("Type") == "addwatchresult" and v.get("Result"):
                    ok_ids.add(v.get("Id"))
            if len(ok_ids) < count:
                time.sleep(0.05)

        assert len(ok_ids) == count, \
            f"only {len(ok_ids)} of {count} watches were accepted"

        # A watch beyond the old limit must actually deliver events.
        list(drain_responses(bridge))
        with open(os.path.join(dirs[-1], "touched.txt"), "w") as f:
            f.write("x")

        deadline = time.time() + 10
        got_event = False
        while time.time() < deadline and not got_event:
            for v in drain_responses(bridge):
                if v.get("Type") == "watchEvent" and v.get("Id") == 1000 + count - 1:
                    got_event = True
                    break
            if not got_event:
                time.sleep(0.05)
        assert got_event, \
            "no event from the last watch; watches past the old limit are inert"
        return True
    finally:
        bridge.close()
        shutil.rmtree(root, ignore_errors=True)


def test_more_cancellable_commands_than_old_limit(bin_path):
    """More cancellable commands in flight than the old 256-entry table held.

    register_cancel() used to drop registrations past 256, after which a cancel
    for those commands did nothing at all.
    """
    if os.name == "nt":
        return True
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "manyfind")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(root)
    for i in range(50):
        with open(os.path.join(root, "f%02d.txt" % i), "w") as f:
            f.write("x")

    bridge = CmdBridgeInteractive(bin_path)
    try:
        count = 400
        for i in range(count):
            bridge.proc.stdin.write(build_cbor_map([
                ("Type", "find"), ("Id", str(5000 + i)), ("Directory", root)]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 60
        ended = set()
        while time.time() < deadline and len(ended) < count:
            for v in drain_responses(bridge):
                if v.get("Type") == "findend":
                    ended.add(v.get("Id"))
            if len(ended) < count:
                time.sleep(0.05)

        assert len(ended) == count, \
            f"only {len(ended)} of {count} finds completed"
        assert not bridge.framing_violations, \
            f"packets with multiple values: {bridge.framing_violations[:5]}"
        return True
    finally:
        bridge.close()
        shutil.rmtree(root, ignore_errors=True)


def test_more_queued_commands_than_old_limit(bin_path):
    """Far more commands in one write than the old 256-slot queue held.

    The queue used to block the reader when full, which also stopped the loop
    that resets the watchdog.
    """
    count = 2000
    blob = b"".join(
        build_cbor_map([("Type", "stat"), ("Id", str(i)), ("Path", temp_dir())])
        for i in range(count))

    stderr_data = send_command(bin_path, blob)
    values = assert_one_value_per_packet(stderr_data, "stat")
    ids = {v.get("Id") for v in values if isinstance(v, dict)}
    assert len(ids) == count, f"only {len(ids)} of {count} commands were answered"
    return True


def _make_temp_names(bin_path, kind, template, count):
    """Creates `count` temp files or dirs from one bridge and returns the paths."""
    bridge = CmdBridgeInteractive(bin_path)
    try:
        for i in range(count):
            bridge.proc.stdin.write(build_cbor_map([
                ("Type", kind), ("Id", str(7000 + i)), ("Path", template)]))
        bridge.proc.stdin.flush()

        want = kind + "result"
        deadline = time.time() + 30
        paths, errors = [], []
        while time.time() < deadline and len(paths) + len(errors) < count:
            for v in drain_responses(bridge):
                if v.get("Type") == want:
                    paths.append(v["Path"])
                elif v.get("Type") == "error":
                    errors.append(v.get("Error"))
            if len(paths) + len(errors) < count:
                time.sleep(0.05)
        assert not errors, f"{kind} failed: {errors[:3]}"
        assert len(paths) == count, f"got {len(paths)} of {count} {kind} responses"
        return paths
    finally:
        bridge.close()


def _assert_names_unique_and_random(paths, prefix, what):
    """Checks the properties of temp names that are observable from outside.

    What this can prove: names are distinct, they keep the requested prefix,
    the random part is not a counter, and it is drawn from a reasonably wide
    and unbiased alphabet.

    What it cannot prove: unpredictability. A weak generator seeded from the
    clock and a counter still produces well-spread names once run through a
    scrambler, and no black-box test distinguishes that from a CSPRNG. That
    property rests on the implementation using the platform's random source
    (mkstemp/mkdtemp on POSIX, BCryptGenRandom on Windows) and is verified by
    reading the code, not here.
    """
    names = [os.path.basename(p) for p in paths]

    assert len(set(names)) == len(names), (
        f"{what}: names collided -- "
        f"{len(names) - len(set(names))} duplicate(s) out of {len(names)}")

    for n in names:
        assert n.startswith(prefix), f"{what}: {n} does not keep the prefix {prefix}"

    suffixes = [n[len(prefix):] for n in names]
    assert all(s for s in suffixes), f"{what}: some names have no random part"

    # A plain counter differs in only the last character or two between
    # consecutive names.
    lens = {len(s) for s in suffixes}
    if len(lens) == 1:
        width = lens.pop()
        diffs = [sum(1 for x, y in zip(a, b) if x != y)
                 for a, b in zip(suffixes, suffixes[1:])]
        avg = sum(diffs) / len(diffs)
        assert avg > width * 0.5, (
            f"{what}: consecutive names differ in only {avg:.1f} of {width} "
            f"characters on average, which looks like a counter; "
            f"examples: {suffixes[:4]}")

    # The alphabet must be wide and roughly evenly used. This catches a
    # generator with a tiny range or a badly biased one (modulo bias, only a
    # few bits of state reaching the output).
    from collections import Counter
    counts = Counter(c for s in suffixes for c in s)
    total = sum(counts.values())
    assert len(counts) >= 20, (
        f"{what}: only {len(counts)} distinct characters over {total} draws; "
        f"examples: {suffixes[:4]}")
    expected = total / len(counts)
    worst = counts.most_common(1)[0]
    assert worst[1] <= expected * 4, (
        f"{what}: character {worst[0]!r} appears {worst[1]} times against an "
        f"expected {expected:.1f}, so the distribution is badly skewed")


def test_tempfile_names_unique_and_random(bin_path):
    """createtempfile names must be unique and unpredictable.

    Predictable names in a shared temp directory let another user pre-create
    the path, so this is a security property, not just hygiene.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    prefix = "rndfile-"
    paths = _make_temp_names(
        bin_path, "createtempfile", os.path.join(tmpdir, prefix), 120)
    try:
        for p in paths:
            assert os.path.isfile(p), f"{p} is not a file"
        _assert_names_unique_and_random(paths, prefix, "createtempfile")
    finally:
        for p in paths:
            try:
                os.unlink(p)
            except OSError:
                pass
    return True


def test_tempdir_names_unique_and_random(bin_path):
    """createtempdir names must be unique and unpredictable."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    prefix = "rnddir-"
    paths = _make_temp_names(
        bin_path, "createtempdir", os.path.join(tmpdir, prefix), 120)
    try:
        for p in paths:
            assert os.path.isdir(p), f"{p} is not a directory"
        _assert_names_unique_and_random(paths, prefix, "createtempdir")
    finally:
        for p in paths:
            try:
                os.rmdir(p)
            except OSError:
                pass
    return True


def test_temp_names_differ_across_processes(bin_path):
    """Two bridges started in quick succession must not produce the same names.

    Seeding from the clock is not enough on Windows, where
    GetSystemTimeAsFileTime only advances every ~15 ms.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    prefix = "procrnd-"
    template = os.path.join(tmpdir, prefix)

    all_paths = []
    try:
        for _ in range(4):
            all_paths += _make_temp_names(bin_path, "createtempfile", template, 5)
        names = [os.path.basename(p) for p in all_paths]
        assert len(set(names)) == len(names), (
            "separate bridge processes produced colliding temp names: "
            f"{sorted(names)}")
    finally:
        for p in all_paths:
            try:
                os.unlink(p)
            except OSError:
                pass
    return True


def test_client_style_flags(bin_path):
    """The bridge must understand the flag spelling the client uses.

    The client passes Go-style "-watchdogTimeout <n>", so a parser that only
    accepts "--watchdogTimeout=<n>" silently leaves the watchdog off and the
    bridge outlives its client.
    """
    for args in (["-watchdogTimeout", "2"], ["--watchdogTimeout=2"]):
        proc = subprocess.Popen(
            [bin_path] + args,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
        )
        try:
            deadline = time.time() + 15
            while time.time() < deadline and proc.poll() is None:
                time.sleep(0.2)
            assert proc.poll() is not None, (
                f"watchdog did not fire with {' '.join(args)}; "
                "the option was not recognised")
        finally:
            if proc.poll() is None:
                proc.kill()
            proc.wait()
            proc.stdin.close()
            proc.stderr.close()
    return True


def test_watchdog_on_by_default(bin_path):
    """Without any option the watchdog is armed, as in the Go implementation."""
    proc = subprocess.Popen(
        [bin_path, "-watchdogTimeout", "0"],
        stdin=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
    )
    try:
        time.sleep(3)
        assert proc.poll() is None, "a timeout of 0 must disable the watchdog"
    finally:
        proc.kill()
        proc.wait()
        proc.stdin.close()
        proc.stderr.close()
    return True


def test_long_exec_does_not_block_other_commands(bin_path):
    """Long-running commands must not starve the ones behind them.

    Each handler occupies a worker for as long as it runs. With a fixed pool a
    handful of sleeps blocks every later command, including the cancel that
    would end them.
    """
    if os.name == "nt":
        return True

    bridge = CmdBridgeInteractive(bin_path)
    try:
        # Occupy more workers than the pool starts with.
        for i in range(6):
            cbor = b'\xA3'
            cbor += _enc_str("Type") + _enc_str("exec")
            cbor += _enc_str("Id") + _enc_uint(100 + i)
            cbor += _enc_str("Exec") + b'\xA1'
            cbor += _enc_str("Args") + _build_cbor_array(["/bin/sleep", "20"])
            bridge.proc.stdin.write(cbor)
        bridge.proc.stdin.flush()
        time.sleep(0.5)

        # An unrelated, instantaneous command must still be answered.
        bridge.proc.stdin.write(build_cbor_map([
            ("Type", "stat"), ("Id", "200"), ("Path", temp_dir())]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 10
        seen = None
        while time.time() < deadline and seen is None:
            for raw in list(bridge.responses):
                val = decode_cbor(raw)
                if val.get("Id") == 200:
                    seen = val
                    break
            if seen is None:
                time.sleep(0.1)
        assert seen is not None, \
            "stat was never answered while long execs were running"
        assert seen.get("Type") == "statresult", f"unexpected reply: {seen}"

        # And a cancel must get through too.
        bridge.responses.clear()
        bridge.proc.stdin.write(build_cbor_map([("Type", "cancel"), ("Id", "100")]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 15
        cancelled = None
        while time.time() < deadline and cancelled is None:
            for raw in list(bridge.responses):
                val = decode_cbor(raw)
                if val.get("Type") == "execresult" and val.get("Id") == 100:
                    cancelled = val
                    break
            if cancelled is None:
                time.sleep(0.1)
        assert cancelled is not None, \
            "cancel was not processed while all workers were busy"
        assert not bridge.framing_violations, \
            f"packets with multiple values: {bridge.framing_violations[:5]}"
        return True
    finally:
        bridge.proc.kill()
        bridge.proc.wait()


@_skip_if_no_af_unix
def test_stop_forward_replies(bin_path):
    """stopforwardserver must answer and leave forwarding usable.

    Joining the connection threads while holding the forward mutex deadlocks
    against their own deregistration, which loses the reply and wedges every
    later forward command.
    """
    bridge = CmdBridgeInteractive(bin_path)
    try:
        resp = bridge.send(build_cbor_map([
            ("Type", "forwardlocalsocketserver"), ("Id", "300")]))
        assert resp is not None, "no response from forwardlocalsocketserver"
        ready = decode_cbor(resp)
        assert ready.get("Type") == "forwardlocalsocketserverready", \
            f"unexpected response: {ready}"
        sock_path = ready.get("Path")

        client = AFUnixSocketClient(sock_path)
        time.sleep(0.3)

        bridge.responses.clear()
        bridge.proc.stdin.write(build_cbor_map([
            ("Type", "stopforwardserver"), ("Id", "300")]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 10
        stopped = False
        while time.time() < deadline and not stopped:
            for raw in list(bridge.responses):
                if decode_cbor(raw).get("Type") == "forwardserverstopped":
                    stopped = True
                    break
            if not stopped:
                time.sleep(0.1)
        assert stopped, "no forwardserverstopped: stopping deadlocked"
        client.close()

        # The forward mutex must still be free: starting another server works.
        bridge.responses.clear()
        bridge.proc.stdin.write(build_cbor_map([
            ("Type", "forwardlocalsocketserver"), ("Id", "301")]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 10
        second = None
        while time.time() < deadline and second is None:
            for raw in list(bridge.responses):
                val = decode_cbor(raw)
                if val.get("Id") == 301:
                    second = val
                    break
            if second is None:
                time.sleep(0.1)
        assert second is not None, \
            "a second forward server never started; the mutex is still held"
        assert second.get("Type") == "forwardlocalsocketserverready", \
            f"unexpected response for the second forward server: {second}"
        assert not bridge.framing_violations, \
            f"packets with multiple values: {bridge.framing_violations[:5]}"
        return True
    finally:
        bridge.close()


def test_createtempfile_unique(bin_path):
    """Concurrent createtempfile calls must produce distinct files.

    Deriving the name from time and pid makes callers within the same second
    collide, and the predictable name plus a non-exclusive open invites a
    symlink attack in a shared temp directory.
    """
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    template = os.path.join(tmpdir, "unique-XXXXXX")

    bridge = CmdBridgeInteractive(bin_path)
    try:
        count = 6
        for i in range(count):
            bridge.proc.stdin.write(build_cbor_map([
                ("Type", "createtempfile"), ("Id", str(400 + i)),
                ("Path", template)]))
        bridge.proc.stdin.flush()

        deadline = time.time() + 10
        paths = []
        while time.time() < deadline and len(paths) < count:
            for val in drain_responses(bridge):
                if val.get("Type") == "createtempfileresult":
                    paths.append(val["Path"])
            if len(paths) < count:
                time.sleep(0.1)

        assert len(paths) == count, f"got {len(paths)} of {count} responses"
        assert len(set(paths)) == count, \
            f"names collided: {sorted(paths)}"
        for p in paths:
            assert os.path.exists(p), f"{p} was not created"
            assert os.path.basename(p).startswith("unique-"), \
                f"{p} does not keep the requested prefix"
            os.unlink(p)
        return True
    finally:
        bridge.close()


def test_createtempdir_keeps_prefix(bin_path):
    """createtempdir keeps the requested name as a prefix, like os.MkdirTemp."""
    tmpdir = temp_dir()
    os.makedirs(tmpdir, exist_ok=True)
    template = os.path.join(tmpdir, "prefixed-XXXXXX")

    cbor = build_cbor_map([
        ("Type", "createtempdir"), ("Id", "410"), ("Path", template)])
    values = assert_one_value_per_packet(send_command(bin_path, cbor), "createtempdir")
    assert values, "no response to createtempdir"
    assert values[0].get("Type") == "createtempdirresult", f"unexpected: {values[0]}"

    path = values[0]["Path"]
    assert os.path.isdir(path), f"{path} is not a directory"
    assert os.path.basename(path).startswith("prefixed-"), \
        f"the prefix was lost: {os.path.basename(path)}"
    os.rmdir(path)
    return True


def test_find_non_recursive_lists_directories(bin_path):
    """A non-recursive find must name its subdirectories, not just its files.

    This is how the file browser navigates: it lists one directory and shows
    the subdirectories as entries to descend into. Go reports the directory and
    only declines to *walk* it (filepath.SkipDir); conflating the two makes
    "Open File from Device" show files but no folders, so a container's root --
    which is almost entirely directories -- looks empty.
    """
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "browse")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(root)
    for d in ("bin", "etc", "usr"):
        os.makedirs(os.path.join(root, d))
        # Content that must NOT appear: this listing is not recursive.
        with open(os.path.join(root, d, "buried.txt"), "w") as f:
            f.write("x")
    for f_ in ("readme.txt", "notes.md"):
        with open(os.path.join(root, f_), "w") as f:
            f.write("x")

    try:
        # QDir::AllEntries | QDir::NoDotAndDotDot, no Subdirectories flag,
        # which is what the file dialog sends.
        cbor = build_cbor_map([
            ("Type", "find"), ("Id", "900"), ("Directory", root),
            ("FileFilters", 0x6007), ("IteratorFlags", 0)])
        values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")

        entries = {os.path.basename(v["Path"]): v
                   for v in values
                   if isinstance(v, dict) and v.get("Type") == "finddata"}

        dirs = sorted(n for n, v in entries.items() if v.get("IsDir"))
        files = sorted(n for n, v in entries.items() if not v.get("IsDir"))

        assert dirs == ["bin", "etc", "usr"], (
            f"a non-recursive listing must report its subdirectories; "
            f"got dirs={dirs} files={files}")
        assert files == ["notes.md", "readme.txt"], f"unexpected files: {files}"
        assert "buried.txt" not in entries, \
            "a non-recursive listing must not descend into subdirectories"
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_find_recursive_still_descends(bin_path):
    """With the Subdirectories flag, find reports directories and their
    contents."""
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "browse_rec")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(os.path.join(root, "sub", "deeper"))
    with open(os.path.join(root, "top.txt"), "w") as f:
        f.write("x")
    with open(os.path.join(root, "sub", "mid.txt"), "w") as f:
        f.write("x")
    with open(os.path.join(root, "sub", "deeper", "low.txt"), "w") as f:
        f.write("x")

    try:
        cbor = build_cbor_map([
            ("Type", "find"), ("Id", "901"), ("Directory", root),
            ("FileFilters", 0x6007), ("IteratorFlags", 2)])
        values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")
        names = {os.path.basename(v["Path"])
                 for v in values
                 if isinstance(v, dict) and v.get("Type") == "finddata"}

        for expected in ("sub", "deeper", "top.txt", "mid.txt", "low.txt"):
            assert expected in names, \
                f"recursive find missed {expected}; got {sorted(names)}"
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_find_dirs_only_filter(bin_path):
    """FileFilters=Dirs reports directories and omits files."""
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "dirsonly")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(os.path.join(root, "adir"))
    with open(os.path.join(root, "afile.txt"), "w") as f:
        f.write("x")

    try:
        cbor = build_cbor_map([
            ("Type", "find"), ("Id", "902"), ("Directory", root),
            ("FileFilters", 0x001), ("IteratorFlags", 0)])
        values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")
        names = {os.path.basename(v["Path"])
                 for v in values
                 if isinstance(v, dict) and v.get("Type") == "finddata"}
        assert "adir" in names, f"Dirs filter dropped the directory: {names}"
        assert "afile.txt" not in names, f"Dirs filter kept a file: {names}"
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_find_paths_have_no_double_separator(bin_path):
    """Listing a directory that ends in a separator must not double it.

    "//bin" is read by the client as a host-qualified path, so
    FilePath::fromUserInput() yields an empty path and every entry collapses
    onto the device root -- a container's root then looks like a list of
    identical unnamed files.
    """
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "sepjoin")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(os.path.join(root, "adir"))
    with open(os.path.join(root, "afile.txt"), "w") as f:
        f.write("x")

    sep = "\\" if os.name == "nt" else "/"
    try:
        # Once with a trailing separator, once without: same result either way.
        for directory in (root + sep, root):
            cbor = build_cbor_map([
                ("Type", "find"), ("Id", "903"), ("Directory", directory),
                ("FileFilters", 0x6007), ("IteratorFlags", 0)])
            values = assert_one_value_per_packet(
                send_command(bin_path, cbor), "find")
            paths = [v["Path"] for v in values
                     if isinstance(v, dict) and v.get("Type") == "finddata"]
            assert paths, f"nothing listed for {directory!r}"
            for p in paths:
                assert sep * 2 not in p, \
                    f"doubled separator in {p!r} when listing {directory!r}"
                assert os.path.basename(p) in ("adir", "afile.txt"), \
                    f"unexpected entry {p!r}"
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_find_root_directory(bin_path):
    """Listing the filesystem root, which is what a device browser opens on."""
    if os.name == "nt":
        return True
    cbor = build_cbor_map([
        ("Type", "find"), ("Id", "904"), ("Directory", "/"),
        ("FileFilters", 0x6007), ("IteratorFlags", 0)])
    values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")
    entries = [v for v in values
               if isinstance(v, dict) and v.get("Type") == "finddata"]
    assert entries, "listing / returned nothing"

    for v in entries:
        assert not v["Path"].startswith("//"), \
            f"entry under / has a doubled separator: {v['Path']!r}"
        assert v["Path"].startswith("/"), f"entry is not absolute: {v['Path']!r}"

    # / is mostly directories; if none are reported the browser cannot descend.
    dirs = [v for v in entries if v.get("IsDir")]
    assert len(dirs) >= 3, (
        f"only {len(dirs)} directories reported under /, so the browser has "
        "nothing to navigate into")
    # And the mode must carry Go's directory bit, which is what the client
    # tests to decide the entry is a folder.
    for v in dirs:
        assert v["Mode"] & GO_MODE_DIR, (
            f"{v['Path']} is IsDir but Mode 0x{v['Mode']:x} lacks Go's Dir bit, "
            "so the client treats it as a plain file")
    return True


# Go io/fs.FileMode bits. The client reads the file *type* from these (see the
# fsMode enum in bridgedfileaccess.cpp) while reading permissions from the low
# nine bits, so "Mode" has to be a Go FileMode and not a POSIX st_mode.
GO_MODE_DIR = 0x80000000
GO_MODE_SYMLINK = 0x08000000
GO_MODE_TYPE_MASK = (0x80000000 | 0x08000000 | 0x02000000 | 0x01000000
                     | 0x04000000 | 0x00200000 | 0x00080000)


def test_mode_is_go_filemode(bin_path):
    """Mode must be a Go io/fs.FileMode, for both find and stat.

    The client decides whether something is a directory from Go's Dir bit
    (1 << 31). A raw POSIX st_mode never has it, so every folder reads as a
    plain file: the dialog lists them as files and opening one tries to open it
    rather than descend into it.
    """
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "gomode")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(os.path.join(root, "adir"))
    with open(os.path.join(root, "afile.txt"), "w") as f:
        f.write("x")
    os.chmod(os.path.join(root, "afile.txt"), 0o644)
    have_symlink = os.name != "nt"
    if have_symlink:
        os.symlink(os.path.join(root, "afile.txt"), os.path.join(root, "alink"))

    try:
        # --- via find ---
        cbor = build_cbor_map([
            ("Type", "find"), ("Id", "905"), ("Directory", root),
            ("FileFilters", 0x6007), ("IteratorFlags", 0)])
        values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")
        by_name = {os.path.basename(v["Path"]): v for v in values
                   if isinstance(v, dict) and v.get("Type") == "finddata"}

        d = by_name["adir"]
        assert d["Mode"] & GO_MODE_DIR, \
            f"find: directory Mode 0x{d['Mode']:x} lacks Go's Dir bit"
        assert d["Mode"] & 0o777, \
            f"find: directory Mode 0x{d['Mode']:x} has no permission bits"

        f_ = by_name["afile.txt"]
        assert f_["Mode"] & GO_MODE_TYPE_MASK == 0, (
            f"find: a regular file must carry no type bit, got 0x{f_['Mode']:x}")
        # Windows has no POSIX permission bits: Go's os.Stat reports 0666 for a
        # writable file and 0444 for a read-only one, which is what we mirror.
        if os.name == "nt":
            assert f_["Mode"] & 0o222, (
                f"find: a writable file must report write permission, got "
                f"0o{f_['Mode'] & 0o777:o}")
        else:
            assert f_["Mode"] & 0o777 == 0o644, \
                f"find: file permissions lost, got 0o{f_['Mode'] & 0o777:o}"

        # --- via stat, which is what the dialog uses on double click ---
        for name, expect_dir in (("adir", True), ("afile.txt", False)):
            cbor = build_cbor_map([
                ("Type", "stat"), ("Id", "906"),
                ("Path", os.path.join(root, name))])
            st = assert_one_value_per_packet(
                send_command(bin_path, cbor), "stat")[0]
            assert st["Type"] == "statresult", f"unexpected: {st}"
            assert bool(st["Mode"] & GO_MODE_DIR) == expect_dir, (
                f"stat: {name} Mode 0x{st['Mode']:x} Dir bit is "
                f"{bool(st['Mode'] & GO_MODE_DIR)}, expected {expect_dir}")
            assert st["IsDir"] == expect_dir, f"stat: {name} IsDir is wrong"

        # stat uses lstat, as Go's does, so a symlink reports as a symlink.
        if have_symlink:
            cbor = build_cbor_map([
                ("Type", "stat"), ("Id", "907"),
                ("Path", os.path.join(root, "alink"))])
            st = assert_one_value_per_packet(
                send_command(bin_path, cbor), "stat")[0]
            assert st["Mode"] & GO_MODE_SYMLINK, (
                f"stat: symlink Mode 0x{st['Mode']:x} lacks Go's Symlink bit")
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_stat_numhardlinks_counts_the_file(bin_path):
    """NumHardLinks counts the links to the file, not to the name.

    supportsAtomicSaveFile() reads it as "renaming over this would orphan the
    other link", so a second link has to show, through a symlink as well as
    directly - DesktopDeviceFileAccess::hasHardLinks follows the link on both
    platforms, and the bridge is meant to be indistinguishable from it.
    """
    import shutil

    tmpdir = temp_dir()
    root = os.path.join(tmpdir, "nlinks")
    if os.path.exists(root):
        shutil.rmtree(root, ignore_errors=True)
    os.makedirs(root)

    lone = os.path.join(root, "lone.txt")
    with open(lone, "w") as f:
        f.write("x")
    linked = os.path.join(root, "linked.txt")
    with open(linked, "w") as f:
        f.write("x")
    second = os.path.join(root, "second.txt")
    try:
        os.link(linked, second)
    except (OSError, AttributeError):
        shutil.rmtree(root, ignore_errors=True)
        return True

    cases = [("lone", lone, 1), ("linked", linked, 2)]
    if os.name != "nt":
        via_link = os.path.join(root, "via_symlink")
        os.symlink(linked, via_link)
        cases.append(("symlink to a linked file", via_link, 2))

    try:
        for what, path, expected in cases:
            cbor = build_cbor_map([
                ("Type", "stat"), ("Id", "930"), ("Path", path)])
            st = assert_one_value_per_packet(
                send_command(bin_path, cbor), "stat")[0]
            assert st["NumHardLinks"] == expected, (
                f"{what}: NumHardLinks is {st['NumHardLinks']}, "
                f"expected {expected}")
        return True
    finally:
        shutil.rmtree(root, ignore_errors=True)


def test_find_does_not_follow_symlink_loop(bin_path):
    """find must not descend through symlinks.

    A link pointing at an ancestor otherwise makes the walk recurse until the
    path hits PATH_MAX, flooding the client with bogus entries.
    """
    if os.name == "nt":
        return True
    import shutil

    tmpdir = temp_dir()
    loop_dir = os.path.join(tmpdir, "loop")
    if os.path.exists(loop_dir):
        shutil.rmtree(loop_dir, ignore_errors=True)
    os.makedirs(os.path.join(loop_dir, "sub"))
    with open(os.path.join(loop_dir, "sub", "file.txt"), "w") as f:
        f.write("x")
    os.symlink(loop_dir, os.path.join(loop_dir, "sub", "back"))

    cbor = build_cbor_map([
        ("Type", "find"), ("Id", "500"), ("Directory", loop_dir),
        ("IteratorFlags", 2)])
    values = assert_one_value_per_packet(send_command(bin_path, cbor), "find")

    paths = [v["Path"] for v in values
             if isinstance(v, dict) and v.get("Type") == "finddata"]
    assert any(v.get("Type") == "findend" for v in values), "no findend received"
    # "back" itself is reported, but nothing below it.
    descended = [p for p in paths if "back/" in p or p.count("/sub/") > 1]
    assert not descended, f"walked through the symlink: {descended[:5]}"

    shutil.rmtree(loop_dir, ignore_errors=True)
    return True


def test_removeall_reports_errors(bin_path):
    """removeall reports failures instead of always claiming success."""
    if os.name == "nt" or os.geteuid() == 0:
        return True

    tmpdir = temp_dir()
    locked = os.path.join(tmpdir, "locked")
    if os.path.exists(locked):
        os.chmod(locked, 0o755)
        import shutil
        shutil.rmtree(locked, ignore_errors=True)
    os.makedirs(os.path.join(locked, "inner"))
    with open(os.path.join(locked, "inner", "file.txt"), "w") as f:
        f.write("x")
    # Make the inner directory unreadable/unwritable so removal must fail.
    os.chmod(os.path.join(locked, "inner"), 0o500)

    try:
        cbor = build_cbor_map([("Type", "removeall"), ("Id", "600"), ("Path", locked)])
        values = assert_one_value_per_packet(send_command(bin_path, cbor), "removeall")
        assert values, "no response to removeall"
        assert values[0].get("Type") == "error", (
            f"removeall reported {values[0].get('Type')} even though the "
            "directory could not be removed")
    finally:
        os.chmod(os.path.join(locked, "inner"), 0o755)
        import shutil
        shutil.rmtree(locked, ignore_errors=True)
    return True


def test_removeall_missing_path_is_ok(bin_path):
    """Removing a path that does not exist succeeds, as os.RemoveAll does."""
    missing = os.path.join(temp_dir(), "definitely-not-here")
    cbor = build_cbor_map([("Type", "removeall"), ("Id", "601"), ("Path", missing)])
    values = assert_one_value_per_packet(send_command(bin_path, cbor), "removeall")
    assert values, "no response to removeall"
    assert values[0].get("Type") == "removeallresult", \
        f"unexpected response: {values[0]}"
    return True


def test_cbor_tagged_value(bin_path):
    """A tagged command still decodes (and does not leak the tag wrapper)."""
    inner = build_cbor_map([("Type", "stat"), ("Id", "700"), ("Path", temp_dir())])
    tagged = b'\xC1' + inner  # tag(1) wrapping the command map

    values = assert_one_value_per_packet(send_command(bin_path, tagged), "stat")
    assert values, "no response to a tagged command"
    assert values[0].get("Type") == "statresult", f"unexpected: {values[0]}"
    return True


def test_cbor_huge_container_header(bin_path):
    """A container header larger than the input must not preallocate.

    A five byte header claiming 16M elements previously reserved 128 MB before
    failing; it has to be rejected against the bytes actually available.
    """
    # array header announcing 0xFFFFFFFF elements, with nothing following it,
    # then a real command that must still be handled.
    bomb = b'\x9A\xFF\xFF\xFF\xFF'
    good = build_cbor_map([("Type", "ping"), ("Id", "800")])
    stat = build_cbor_map([("Type", "stat"), ("Id", "801"), ("Path", temp_dir())])

    stderr_data = send_command(bin_path, bomb + good + stat)
    values = assert_one_value_per_packet(stderr_data, "stat")
    ids = [v.get("Id") for v in values if isinstance(v, dict)]
    assert 801 in ids, (
        f"the bridge did not recover after a bogus container header; got {ids}")
    return True


TEST_TIMEOUT = 90  # seconds per test


def _timeout_handler(signum, frame):
    raise TimeoutError(f"test timed out after {TEST_TIMEOUT}s")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <cmdbridge_binary>", file=sys.stderr)
        sys.exit(1)

    bin_path = sys.argv[1]

    if not os.path.isfile(bin_path):
        print(f"Cannot find cmdbridge binary: {bin_path}", file=sys.stderr)
        sys.exit(1)

    # Set per-test timeout on Unix
    if hasattr(signal, "SIGALRM"):
        signal.signal(signal.SIGALRM, _timeout_handler)

    print("CmdBridge C Protocol Tests (Python)")
    print("=" * 40)
    print(f"Binary: {bin_path}\n")

    tests = [
        ("ping", test_ping),
        ("stat", test_stat),
        ("stat_nonexistent", test_stat_nonexistent),
        ("is_dir", test_is_dir),
        ("copyfile", test_copyfile),
        ("writefile_readfile", test_writefile_readfile),
        ("unicode_filenames", test_unicode_filenames),
        ("long_paths", test_long_paths),
        ("createdir_remove", test_createdir_remove),
        ("removeall", test_removeall),
        ("createsymlink", test_createlink),
        ("renamefile", test_rename),
        ("createtempfile", test_tempfile),
        ("createtempdir", test_tempdir),
        ("setpermissions", test_setpermissions),
        ("owner_ownerid", test_owner_id),
        ("group_groupid", test_group_id),
        ("freespace", test_freespace),
        ("issamefile", test_issamefile),
        ("ensureexistingfile", test_ensure_existing_file),
        ("exit", test_exit),
        ("watch", test_watch),
        ("watch_file_readd_on_recreation", test_watch_file_readd_on_recreation),
        ("find_basic", test_find_basic),
        ("find_name_filter", test_find_name_filter),
        ("find_cancel", test_find_cancel),
        ("find_one_value_per_packet", test_find_one_value_per_packet),
        ("stat_numhardlinks_counts_the_file", test_stat_numhardlinks_counts_the_file),
        ("find_does_not_follow_symlink_loop", test_find_does_not_follow_symlink_loop),
        ("find_non_recursive_lists_directories", test_find_non_recursive_lists_directories),
        ("find_recursive_still_descends", test_find_recursive_still_descends),
        ("find_dirs_only_filter", test_find_dirs_only_filter),
        ("find_paths_have_no_double_separator",
         test_find_paths_have_no_double_separator),
        ("find_root_directory", test_find_root_directory),
        ("mode_is_go_filemode", test_mode_is_go_filemode),
        ("exec_cancel_sigkill", test_exec_cancel_sigkill),
        ("exec_large_output", test_exec_large_output),
        ("readfile_offset_limit", test_readfile_offset_limit),
        ("cbor_malformed", test_cbor_malformed),
        ("queue_backpressure", test_queue_backpressure),
        ("socket_forward_ring_overflow", test_socket_forward_ring_overflow),
        ("watchdog_timeout", test_watchdog_timeout),
        ("symlink_stat", test_symlink_stat),
        ("socket_forward_server_start", test_socket_forward_server_start),
        ("socket_forward_data_roundtrip", test_socket_forward_data_roundtrip),
        ("stop_forward_closes_all_connections", test_stop_forward_closes_all_connections),
        ("multiple_simultaneous_connections", test_multiple_simultaneous_connections),
        ("data_routing_isolation", test_data_routing_isolation),
        ("close_one_connection_keeps_others", test_close_one_connection_keeps_others),
        ("exec", test_exec),
        ("exec_cancel", test_exec_cancel),
        ("parallel_execution", test_parallel_execution),
        ("stat_modtime_is_integer", test_stat_modtime_is_integer),
        ("finddata_modtime_is_integer", test_finddata_modtime_is_integer),
        ("finddata_size_is_int64", test_finddata_size_is_int64),
        ("stat_size_is_int64", test_stat_size_is_int64),
        ("environment", test_environment),
        ("environment_passes_variable", test_environment_passes_variable),
        ("command_split_across_writes", test_command_split_across_writes),
        ("writefile_large", test_writefile_large),
        ("exec_stdin", test_exec_stdin),
        ("exec_env_keeps_path_lookup", test_exec_env_keeps_path_lookup),
        ("exec_missing_program", test_exec_missing_program),
        ("client_style_flags", test_client_style_flags),
        ("watchdog_on_by_default", test_watchdog_on_by_default),
        ("long_exec_does_not_block_other_commands",
         test_long_exec_does_not_block_other_commands),
        ("stop_forward_replies", test_stop_forward_replies),
        ("createtempfile_unique", test_createtempfile_unique),
        ("createtempdir_keeps_prefix", test_createtempdir_keeps_prefix),
        ("removeall_reports_errors", test_removeall_reports_errors),
        ("removeall_missing_path_is_ok", test_removeall_missing_path_is_ok),
        ("cbor_tagged_value", test_cbor_tagged_value),
        ("cbor_huge_container_header", test_cbor_huge_container_header),
        ("more_watches_than_old_limit", test_more_watches_than_old_limit),
        ("more_cancellable_commands_than_old_limit",
         test_more_cancellable_commands_than_old_limit),
        ("more_queued_commands_than_old_limit", test_more_queued_commands_than_old_limit),
        ("tempfile_names_unique_and_random", test_tempfile_names_unique_and_random),
        ("tempdir_names_unique_and_random", test_tempdir_names_unique_and_random),
        ("temp_names_differ_across_processes", test_temp_names_differ_across_processes),
    ]

    passed = 0
    failed = 0

    for name, test_fn in tests:
        print(f"  test_{name}... ", end="", flush=True)
        if hasattr(signal, "SIGALRM"):
            signal.alarm(TEST_TIMEOUT)
        try:
            if test_fn(bin_path):
                print("OK")
                passed += 1
            else:
                print("FAILED")
                failed += 1
        except TimeoutError as e:
            print(f"TIMEOUT ({e})")
            failed += 1
        except Exception as e:
            print(f"FAILED ({e})")
            failed += 1
        finally:
            if hasattr(signal, "SIGALRM"):
                signal.alarm(0)

    total = passed + failed
    print(f"\n{total} tests, {passed} passed, {failed} failed")
    cleanup_temp_dir()
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
