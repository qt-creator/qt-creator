# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Protocol checks for the debugger bridge (bridge.py), invoked by tst_bridge.
# bridge.py normally runs inside gdb, so a fake gdb module is installed before
# importing it and the server is driven request by request over a pipe. The
# location of bridge.py comes from the DUMPERDIR environment variable (or the
# second argument); the check to run is the first argument.
#
# Each check exits 0 on success, 1 with an explanation on failure, and 3 with a
# "SKIP: <reason>" line where the platform cannot run it. The skip is the exit
# code, not the line: stdout and stderr are read as one, so anything the bridge
# warns about would sit in front of it.

import importlib.util
import json
import os
import select
import sys
import types

check = sys.argv[1] if len(sys.argv) > 1 else ""
dumperDir = os.environ.get("DUMPERDIR") or (sys.argv[2] if len(sys.argv) > 2 else "")


#######################################################################
# A fake gdb module, recording what the bridge asks of it
#######################################################################

class GdbError(Exception):
    pass


class Skip(Exception):
    pass


class FakeLocation():
    def __init__(self, source):
        self.address = 0x400123
        self.function = "add"
        self.source = (source, 42) if source else None
        self.fullname = source
        self.enabled = True


class FakeWatchpointLocations():
    # gdb 17.1 aborts with an internal error when a watchpoint is asked for its
    # locations, so touching this at all is the bug.
    def __iter__(self):
        raise AssertionError("a watchpoint must not be asked for its locations")


class FakeBreakpoint():
    # gdb deletes a temporary breakpoint as it is hit; reading it afterwards
    # raises RuntimeError. The bridge has to cope with that.
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.temporary = kwargs.get("temporary", False)
        self.enabled = kwargs.get("enabled", True)
        self.condition = None
        self.ignore_count = 0
        self.hit_count = 0
        self.location = kwargs.get("source") or (args[0] if args else "")
        if args[1:2] == (gdb.BP_WATCHPOINT,):
            self.type = gdb.BP_WATCHPOINT
            self.locations = FakeWatchpointLocations()
        else:
            self.type = gdb.BP_BREAKPOINT
            self.locations = [FakeLocation(kwargs.get("source"))]
        self.deleted = False
        self._number = gdb.nextBreakpointNumber
        gdb.nextBreakpointNumber += 1
        gdb.breakpointObjects.append(self)

    @property
    def number(self):
        if self.deleted:
            raise RuntimeError("Breakpoint %d is invalid." % self._number)
        return self._number

    @property
    def condition(self):
        # Standing in for any attribute gdb refuses to hand out - bp.type on an
        # older gdb that cannot map the type is the real case.
        if getattr(gdb, "raiseOnReport", False):
            raise GdbError("cannot read the breakpoint")
        return self._condition

    @condition.setter
    def condition(self, value):
        self._condition = value

    def is_valid(self):
        return not self.deleted

    def delete(self):
        if getattr(self, "raiseOnDelete", False):
            # As gdb does for a breakpoint it has already dropped: not gdb.error,
            # so the handler does not catch it and the dispatcher answers.
            raise RuntimeError("Breakpoint %d is invalid." % self._number)
        self.deleted = True


class FakeThread():
    def __init__(self, num):
        self.global_num = num
        self.name = "inferior"

    def switch(self):
        pass


class FakeInferior():
    def threads(self):
        return [FakeThread(1)]


class FakeEventRegistry():
    def __init__(self):
        self.handlers = []

    def connect(self, handler):
        self.handlers.append(handler)

    def disconnect(self, handler):
        # As gdb does: removing something that is not connected is an error.
        self.handlers.remove(handler)


def makeFakeGdb():
    module = types.ModuleType("gdb")
    module.error = GdbError
    module.Breakpoint = FakeBreakpoint
    module.BP_BREAKPOINT = 1
    module.BP_WATCHPOINT = 2
    module.commands = []
    module.objfileList = []
    module.sharedLibraryListing = ""
    module.breakpointObjects = []
    module.nextBreakpointNumber = 1
    module.onExecute = None
    module.miErrorRecord = None

    def execute(command, to_string=False):
        module.commands.append(command)
        if command == "info sharedlibrary":
            return module.sharedLibraryListing
        if command.startswith("maint print msymbols"):
            path = command.rsplit(" -- ", 1)[1].strip('"')
            with open(path, "w") as listing:
                listing.write("Object file /lib/libc.so.6:\n"
                              "[ 0] A 0x16bd64 _DYNAMIC  moc_qudpsocket.cpp\n"
                              "[12] S 0xe94680 _ZN4myns5QFileC1Ev section .plt"
                              "  myns::QFile::QFile()\n")
        if command.startswith("interpreter-exec mi"):
            if module.miErrorRecord:
                return module.miErrorRecord
            return '^done,threads=[{id="1"}],current-thread-id="1"\n'
        if command.startswith("catch "):
            FakeBreakpoint(command)  # gdb creates one, we only see it in the list
        if module.onExecute:
            module.onExecute(command)
        return ""

    module.execute = execute
    module.events = types.SimpleNamespace(stop=FakeEventRegistry(),
                                          exited=FakeEventRegistry(),
                                          breakpoint_modified=FakeEventRegistry(),
                                          new_objfile=FakeEventRegistry(),
                                          free_objfile=FakeEventRegistry())
    module.breakpoints = lambda: tuple(module.breakpointObjects)
    module.objfiles = lambda: list(module.objfileList)
    module.selected_inferior = lambda: FakeInferior()
    module.decode_line = lambda spec: (None, [])
    module.selected_thread = lambda: FakeThread(1)
    module.newest_frame = lambda: None
    module.parse_and_eval = lambda expression: 0
    return module


gdb = makeFakeGdb()
sys.modules["gdb"] = gdb


def interpreterStopHandler(event):
    # Stand-in for gdbbridge's native-mixed handler, which the real gdbbridge
    # connects when it is imported - that is, before the server is built.
    pass


fakeGdbbridge = types.ModuleType("gdbbridge")
fakeGdbbridge.interpreterStopHandler = interpreterStopHandler
sys.modules["gdbbridge"] = fakeGdbbridge
gdb.events.stop.connect(interpreterStopHandler)


#######################################################################
# The server under test, talking to us over a pipe
#######################################################################

def realDumperBase():
    # dumper.py is backend-agnostic and imports cleanly, so the checks can use
    # the real serializer instead of a stand-in that could drift from it.
    sys.path.insert(0, dumperDir)
    spec = importlib.util.spec_from_file_location("dumper_under_test",
                                                  os.path.join(dumperDir, "dumper.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.DumperBase


class FakeDumper():
    def __init__(self):
        self.addedModules = []
        self.calls = []
        self.reported = []

    def addDumperModule(self, args):
        self.addedModules.append(args['path'])

    def setupDumpers(self):
        # The real one returns what the dumpers registered; the C++ side needs
        # it for the per-type display format menus.
        return 'dumpers=[{type="QString",editable="true",formats="2,3"}],python="31200"'

    def resultToMi(self, value):
        # The real one from dumper.py: this is what the C++ GdbMi reader parses,
        # so a stand-in here would only test itself.
        return realDumperBase().resultToMi(self, value)

    def disableInterpreterStopHandler(self):
        # What gdbbridge's own method does: drop the handler it connected on
        # import. The bridge has to ask for it; if it stops asking, the handler
        # stays connected and the check below fails.
        gdb.events.stop.disconnect(interpreterStopHandler)

    def fetchStack(self, args):
        self.calls.append("fetchStack")
        self.reportResult('stack={frames=[{level="0",function="main"}]}', args)

    def assignValue(self, args):
        self.calls.append("assignValue")
        self.reportResult("", args)

    def reportResult(self, result, args):
        # Replaced by the server while it captures; a real dumper prints here.
        self.reported.append(result)

    def hexdecode(self, s):
        return bytes.fromhex(s).decode("utf-8")

    def hexencode(self, s):
        return s.encode("utf-8").hex()


def loadBridge():
    path = os.path.join(dumperDir, "bridge.py")
    spec = importlib.util.spec_from_file_location("bridge_under_test", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


if sys.platform == "win32":
    import ctypes
    import ctypes.wintypes
    import msvcrt
    import time

    _peekNamedPipe = ctypes.WinDLL("kernel32", use_last_error=True).PeekNamedPipe
    _peekNamedPipe.argtypes = [ctypes.wintypes.HANDLE, ctypes.c_void_p,
                               ctypes.wintypes.DWORD, ctypes.wintypes.LPDWORD,
                               ctypes.wintypes.LPDWORD, ctypes.wintypes.LPDWORD]
    _peekNamedPipe.restype = ctypes.wintypes.BOOL

    def waitReadable(fd, timeout):
        # select() takes sockets only on Windows, so ask the pipe itself. A
        # peek that fails means the write end is gone; let the read see the EOF.
        handle = msvcrt.get_osfhandle(fd)
        available = ctypes.wintypes.DWORD()
        deadline = time.monotonic() + timeout
        while True:
            if not _peekNamedPipe(handle, None, 0, None,
                                  ctypes.byref(available), None):
                return True
            if available.value:
                return True
            if time.monotonic() >= deadline:
                return False
            time.sleep(0.005)
else:
    def waitReadable(fd, timeout):
        return bool(select.select([fd], [], [], timeout)[0])


class Peer():
    # Holds the server and the read end of its protocol stream.
    def __init__(self, bridge):
        self.readFd, writeFd = os.pipe()
        self.server = bridge.DapServer(FakeDumper())
        self.server.protocolFd = writeFd
        self.buffer = b""
        self.seq = 0

    def request(self, command, arguments=None):
        self.seq += 1
        self.server._dispatch({"type": "request", "seq": self.seq,
                               "command": command,
                               "arguments": arguments or {}})
        return self.seq

    def _readSome(self, timeout):
        # Whether anything more arrived. The timeout only bounds a message that
        # never comes; it never decides that one has all of them.
        if not waitReadable(self.readFd, timeout):
            return False
        chunk = os.read(self.readFd, 65536)
        if not chunk:
            return False  # EOF
        self.buffer += chunk
        return True

    def raw(self, timeout=5.0):
        while self._readSome(timeout):
            timeout = 0  # the rest is already in the pipe or not coming
        return self.buffer

    def _frames(self):
        # The complete messages in the buffer, and the tail that is not one yet.
        data = self.buffer
        messages = []
        while data:
            header, separator, rest = data.partition(b"\r\n\r\n")
            if not separator:
                break
            fields = dict(line.split(b": ", 1)
                          for line in header.split(b"\r\n") if b": " in line)
            if b"Content-Length" not in fields:
                raise AssertionError("no Content-Length in header: %r" % header)
            length = int(fields[b"Content-Length"])
            if len(rest) < length:
                break
            messages.append(json.loads(rest[:length].decode("utf-8")))
            data = rest[length:]
        return messages, data

    def messages(self, expected=None, timeout=5.0):
        # Parse the Content-Length framing strictly: anything else in the
        # stream (a stray print, say) makes this fail. A handler answers before
        # request() returns, so only what another thread writes can still be on
        # its way; expected says how many messages to read for, so that a
        # loaded machine cannot cut the stream short.
        while True:
            self.raw(timeout)
            messages, rest = self._frames()
            if expected is None or len(messages) >= expected:
                break
            if not self._readSome(timeout):
                break  # nothing more is coming; let the check report what it got
        if rest:
            raise AssertionError("unframed trailing bytes: %r" % rest[:200])
        return messages


def eventsOf(messages, name):
    return [m for m in messages if m.get("type") == "event" and m.get("event") == name]


def responsesOf(messages, command):
    return [m for m in messages
            if m.get("type") == "response" and m.get("command") == command]


#######################################################################
# The checks
#######################################################################

def check_framing(bridge):
    peer = Peer(bridge)
    seq = peer.request("threads")
    messages = peer.messages()
    assert len(messages) == 1, "expected one message, got %d" % len(messages)
    response = messages[0]
    assert response["type"] == "response", response
    assert response["command"] == "threads", response
    assert response["request_seq"] == seq, response
    assert response["success"] is True, response
    assert response["body"]["threads"] == [{"id": 1, "name": "inferior"}], response


def check_stdout_cannot_corrupt_the_protocol(bridge):
    # The protocol must not share a descriptor with anything that prints: gdb's
    # console output, a stray dumper print, the inferior. _claimStdio() keeps a
    # private copy for the protocol and points fd 1 elsewhere, so a print can
    # no longer land between two frames.
    protocol = os.pipe()
    savedOut, savedErr = os.dup(1), os.dup(2)
    stderrPipe = os.pipe()
    try:
        os.dup2(protocol[1], 1)   # as if gdb had been started on the pipe
        os.dup2(stderrPipe[1], 2)
        peer = Peer(bridge)
        peer.server._claimStdio()
        print("a stray print", flush=True)
        bridge.warn("and a diagnostic")
        peer.request("threads")
    finally:
        os.dup2(savedOut, 1)
        os.dup2(savedErr, 2)
        os.close(stderrPipe[1])

    peer.readFd = protocol[0]
    messages = peer.messages()
    assert len(messages) == 1, \
        "the protocol stream is not clean: %r" % (peer.buffer[:300],)
    assert messages[0]["command"] == "threads", messages[0]

    complaints = os.read(stderrPipe[0], 65536)
    assert b"a stray print" in complaints, "the print vanished: %r" % complaints
    assert b"and a diagnostic" in complaints, "the diagnostic vanished: %r" % complaints


def check_temporary_breakpoint_stop(bridge):
    # Running to a function sets a temporary breakpoint; gdb deletes it as it
    # is hit, so reading its number afterwards raises. The stop must still be
    # reported, exactly once, and no second (failure) response may follow.
    peer = Peer(bridge)

    def onExecute(command):
        if command == "continue":
            hit = gdb.breakpointObjects[-1]
            hit.delete()  # as gdb does for a temporary breakpoint
            for handler in gdb.events.stop.handlers:
                handler(types.SimpleNamespace(breakpoints=[hit]))

    gdb.onExecute = onExecute
    try:
        peer.request("qtc/runToFunction", {"function": "add"})
    finally:
        gdb.onExecute = None

    messages = peer.messages()
    responses = responsesOf(messages, "qtc/runToFunction")
    assert len(responses) == 1, "expected one response, got %d: %s" % (
        len(responses), json.dumps(messages))
    assert responses[0]["success"] is True, responses[0]
    stopped = eventsOf(messages, "stopped")
    assert len(stopped) == 1, "expected one stopped event, got %d: %s" % (
        len(stopped), json.dumps(messages))
    assert stopped[0]["body"]["hitBreakpointIds"] == [], stopped[0]


def check_shutdown_quits_gdb(bridge):
    # Leaving the read loop is not enough: gdb has to be told to quit, or it
    # stays alive on our stdin.
    peer = Peer(bridge)
    peer.request("terminate")
    assert "kill" in gdb.commands, gdb.commands
    assert "quit" in gdb.commands, "gdb was not asked to quit: %s" % gdb.commands
    assert peer.server.running is False

    del gdb.commands[:]
    peer = Peer(bridge)
    peer.request("disconnect", {"terminateDebuggee": False})
    assert "detach" in gdb.commands, "expected a detach: %s" % gdb.commands
    assert "kill" not in gdb.commands, "killed the debuggee anyway: %s" % gdb.commands
    assert "quit" in gdb.commands, "gdb was not asked to quit: %s" % gdb.commands


def check_breakpoint_source_with_spaces(bridge):
    # A source path containing spaces must survive, so the location cannot be
    # built as a "file:line" linespec string.
    peer = Peer(bridge)
    path = "/tmp/a directory with spaces/main.cpp"
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 7, "type": 1, "file": path, "line": 42,
                  "enabled": True})
    messages = peer.messages()
    responses = responsesOf(messages, "qtc/insertBreakpoint")
    assert len(responses) == 1, json.dumps(messages)
    assert responses[0]["success"] is True, responses[0]
    body = responses[0]["body"]
    assert body["modelid"] == 7, body
    assert body["bkpt"], "no breakpoint payload: %s" % body
    assert path in body["bkpt"], "the location did not survive: %s" % body["bkpt"]
    assert gdb.breakpointObjects, "no breakpoint was created"
    created = gdb.breakpointObjects[-1]
    assert created.kwargs.get("source") == path, \
        "source not passed as a keyword: args=%r kwargs=%r" % (created.args,
                                                              created.kwargs)
    assert created.kwargs.get("line") == 42, created.kwargs
    for argument in created.args:
        assert path not in str(argument), \
            "location built as a linespec: %r" % (created.args,)


def check_one_module_is_not_reported_twice(bridge):
    # The object file list and 'info sharedlibrary' can spell one module
    # differently; keyed on the spelling, it turned into two rows, one of them
    # with no addresses.
    peer = Peer(bridge)
    gdb.objfileList = [types.SimpleNamespace(filename="/lib/libc.so.6", owner=None)]
    gdb.sharedLibraryListing = (
        "From                To                  Syms Read   Shared Object Library\n"
        "0x00001000  0x00002000  Yes         /lib/../lib/libc.so.6\n")
    try:
        peer.request("qtc/fetchModules", {})
    finally:
        gdb.objfileList = []
        gdb.sharedLibraryListing = ""

    modules = responsesOf(peer.messages(), "qtc/fetchModules")[0]["body"]["modules"]
    assert len(modules) == 1, modules
    assert modules[0]["path"] == "/lib/libc.so.6", modules
    assert modules[0]["startAddress"] == 0x1000, modules
    assert modules[0]["endAddress"] == 0x2000, modules
    assert modules[0]["symbolsRead"] is True, modules


def check_a_resuming_console_command_reports_the_stop(bridge):
    # The console can run continue/next/finish/kill. gdb returns only once the
    # inferior stopped again, and nobody else would report that, so the views
    # would keep showing the old location.
    peer = Peer(bridge)

    def onExecute(command):
        if command == "continue":
            for handler in gdb.events.stop.handlers:
                handler(types.SimpleNamespace(stop_signal="SIGINT"))

    gdb.onExecute = onExecute
    try:
        peer.request("qtc/executeCommand", {"command": "continue"})
    finally:
        gdb.onExecute = None

    messages = peer.messages()
    order = [m.get("event") or ("response:" + m.get("command", "")) for m in messages]
    assert order == ["continued", "response:qtc/executeCommand", "stopped"], order

    # A command that does not resume anything says nothing extra.
    plain = Peer(bridge)
    plain.request("qtc/executeCommand", {"command": "print x"})
    assert [m.get("event") for m in plain.messages() if m.get("type") == "event"] == [], \
        plain.messages()


def check_a_failing_console_command_answers_with_an_error(bridge):
    # Anything the command raises has to come back as console output; a bare
    # protocol failure leaves the user with nothing to look at.
    peer = Peer(bridge)

    def onExecute(command):
        raise ValueError("not a gdb.error")

    gdb.onExecute = onExecute
    try:
        peer.request("qtc/executeCommand", {"command": "whatever"})
    finally:
        gdb.onExecute = None

    responses = responsesOf(peer.messages(), "qtc/executeCommand")
    assert len(responses) == 1, responses
    assert responses[0]["success"] is True, responses[0]
    assert responses[0]["body"]["error"] == "not a gdb.error", responses[0]


def check_inferior_output_event(bridge):
    # The inferior gets its own tty; what it writes must arrive as an 'output'
    # event rather than in the middle of the protocol stream.
    if bridge.termios is None:
        raise Skip("no termios on this platform")
    peer = Peer(bridge)
    peer.server._setupInferiorTty()
    assert peer.server.inferiorTty, "no inferior tty was set up"
    assert any(c.startswith("set inferior-tty") for c in gdb.commands), gdb.commands
    os.write(peer.server.inferiorTty[1], b"hello from the inferior\n")
    messages = peer.messages(expected=1)
    output = eventsOf(messages, "output")
    assert len(output) == 1, "expected one output event, got %s" % json.dumps(messages)
    assert output[0]["body"]["category"] == "stdout", output[0]
    assert output[0]["body"]["output"] == "hello from the inferior\n", output[0]


def check_initialize_reports_dumpers(bridge):
    # What setupDumpers() reported has to reach the client, or Creator never
    # learns which display formats the dumpers offer. Go through run(), which is
    # what picks the report up - stdin is at EOF, so the read loop returns at
    # once and we can ask for the capabilities afterwards.
    protocol = os.pipe()
    savedIn, savedOut = os.dup(0), os.dup(1)
    devnull = os.open(os.devnull, os.O_RDONLY)
    try:
        os.dup2(devnull, 0)
        os.dup2(protocol[1], 1)
        peer = Peer(bridge)
        peer.server.run()
        peer.request("initialize")
    finally:
        os.dup2(savedIn, 0)
        os.dup2(savedOut, 1)
        os.close(devnull)
    peer.readFd = protocol[0]
    messages = peer.messages()
    responses = responsesOf(messages, "initialize")
    assert len(responses) == 1, json.dumps(messages)
    reported = responses[0]["body"].get("qtcDumpers", "")
    assert "QString" in reported, "the dumper report did not travel: %r" % reported
    assert 'formats="2,3"' in reported, reported


def check_attach_failure_is_reported(bridge):
    # If gdb cannot attach, the request has to fail: the C++ side turns that
    # into a failed engine instead of a session with nothing behind it.
    peer = Peer(bridge)

    def onExecute(command):
        if command.startswith("attach"):
            raise GdbError("ptrace: Operation not permitted.")

    gdb.onExecute = onExecute
    try:
        peer.request("attach", {"pid": 4711})
    finally:
        gdb.onExecute = None

    messages = peer.messages()
    responses = responsesOf(messages, "attach")
    assert len(responses) == 1, json.dumps(messages)
    assert responses[0]["success"] is False, "the failure was swallowed: %s" % responses[0]
    assert "not permitted" in responses[0].get("message", ""), responses[0]
    assert "attach 4711" in gdb.commands, gdb.commands


def check_server_owns_the_stop_events(bridge):
    # Two stop handlers means the other one can resume the inferior behind the
    # server's back, so the server has to take the session over.
    assert interpreterStopHandler in gdb.events.stop.handlers, "test setup"
    protocol = os.pipe()
    savedIn, savedOut = os.dup(0), os.dup(1)
    devnull = os.open(os.devnull, os.O_RDONLY)
    try:
        os.dup2(devnull, 0)
        os.dup2(protocol[1], 1)
        peer = Peer(bridge)
        peer.server.run()
    finally:
        os.dup2(savedIn, 0)
        os.dup2(savedOut, 1)
        os.close(devnull)
    assert interpreterStopHandler not in gdb.events.stop.handlers, \
        "gdbbridge's interpreter handler is still connected"
    assert peer.server._onStop in gdb.events.stop.handlers, \
        "the server stopped listening for stops"


def check_extra_dumpers_are_loaded(bridge):
    # The user's own dumper module has to be added before the dumpers are set
    # up, so it travels with initialize - a later request would be too late.
    peer = Peer(bridge)
    peer.request("initialize", {"qtcDumperFile": "/home/me/mydumpers.py",
                                "qtcDumperCommands": "python print(1)\nset confirm off"})
    assert peer.server.dumper.addedModules == ["/home/me/mydumpers.py"], \
        peer.server.dumper.addedModules
    assert "python print(1)" in gdb.commands, gdb.commands
    assert "set confirm off" in gdb.commands, gdb.commands
    # They are the user's own commands, so what they do belongs in the log
    # rather than being swallowed, as GdbEngine's normal channel has it.
    echoed = [event["body"]["output"] for event in eventsOf(peer.messages(), "output")
              if event["body"]["category"] == "console"]
    assert echoed == ["python print(1)\n", "set confirm off\n"], echoed
    # And the report still comes back, i.e. the setup ran after the additions.
    body = responsesOf(peer.messages(), "initialize")[0]["body"]
    assert "QString" in body.get("qtcDumpers", ""), body


def check_target_configuration_reaches_gdb(bridge):
    # Without these, gdb looks for sources and libraries at the paths in the
    # debug info, which are the build machine's.
    peer = Peer(bridge)
    peer.request("qtc/configureTarget",
                 {"sourcePathMap": [{"from": "/build/qt", "to": "/home/me/qt"}],
                  "sourceDirectories": ["/home/me/extra sources"],
                  "sysroot": "/opt/sysroot"})
    assert 'set substitute-path "/build/qt" "/home/me/qt"' in gdb.commands, gdb.commands
    assert 'directory "/home/me/extra sources"' in gdb.commands, gdb.commands
    assert "set sysroot /opt/sysroot" in gdb.commands, gdb.commands
    # A sysroot alone does not locate the sources.
    assert 'set substitute-path /usr/src "/opt/sysroot/usr/src"' in gdb.commands, gdb.commands
    assert len(responsesOf(peer.messages(), "qtc/configureTarget")) == 1


def check_catchpoints_are_created(bridge):
    # The catch types have no gdb.Breakpoint constructor, so they are made with
    # the CLI command; a fork breakpoint means fork and vfork, and removing it
    # has to take both away.
    peer = Peer(bridge)
    peer.request("qtc/insertBreakpoint", {"modelid": 9, "type": 7, "enabled": True})
    assert "catch fork" in gdb.commands, gdb.commands
    assert "catch vfork" in gdb.commands, gdb.commands
    body = responsesOf(peer.messages(), "qtc/insertBreakpoint")[0]["body"]
    assert body["bkpt"], "no breakpoint reported for the catchpoint"
    created = [bp for bp in gdb.breakpointObjects if str(bp.args[0]).startswith("catch")]
    assert len(created) == 2, created

    peer.buffer = b""
    peer.request("qtc/removeBreakpoint",
                 {"id": created[0]._number, "modelid": 9})
    assert all(bp.deleted for bp in created), \
        "the companion catchpoint outlived the breakpoint: %s" % [bp.deleted for bp in created]

    # Throw/catch and main are ordinary function breakpoints.
    for bptype, function in ((4, "__cxa_throw"), (5, "__cxa_begin_catch")):
        peer.request("qtc/insertBreakpoint",
                     {"modelid": 10 + bptype, "type": bptype, "enabled": True})
        assert gdb.breakpointObjects[-1].kwargs.get("function") == function, \
            gdb.breakpointObjects[-1].kwargs
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 20, "type": 6, "function": "qMain", "enabled": True})
    assert gdb.breakpointObjects[-1].kwargs.get("function") == "qMain", \
        gdb.breakpointObjects[-1].kwargs


def check_a_catchpoint_keeps_its_settings(bridge):
    # A catchpoint is created by a different route than the other breakpoints,
    # which must not cost it the condition, the ignore count and the enabled
    # state - and the companion has to carry them too, or a disabled fork
    # breakpoint still stops the inferior on vfork.
    peer = Peer(bridge)
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 30, "type": 7, "enabled": False,
                  "condition": "78203e2030", "ignorecount": 3})
    created = [bp for bp in gdb.breakpointObjects if str(bp.args[0]).startswith("catch")]
    assert len(created) == 2, created
    for bp in created:
        assert bp.condition == "x > 0", (bp.args, bp.condition)
        assert bp.ignore_count == 3, (bp.args, bp.ignore_count)
        assert bp.enabled is False, (bp.args, bp.enabled)

    # The same for an update; the location is unchanged, so it stays in place.
    peer.buffer = b""
    peer.request("qtc/updateBreakpoint",
                 {"id": created[0]._number, "modelid": 30, "type": 7,
                  "enabled": True, "ignorecount": 0})
    for bp in created:
        assert bp.enabled is True, (bp.args, bp.enabled)
        assert bp.condition == "", (bp.args, bp.condition)


def check_a_catchpoint_reports_its_catch_type(bridge):
    # The C++ side reads a catchpoint as a breakpoint carrying a catch-type
    # (BreakpointParameters::updateFromGdbOutput). Reported as a watchpoint it
    # would replace the type the user asked for.
    peer = Peer(bridge)
    peer.request("qtc/insertBreakpoint", {"modelid": 31, "type": 7, "enabled": True})
    payload = responsesOf(peer.messages(), "qtc/insertBreakpoint")[0]["body"]["bkpt"]
    assert payload, "no breakpoint reported for the catchpoint"
    assert 'catch-type="fork"' in payload, payload
    assert 'type="breakpoint"' in payload, payload
    assert "watchpoint" not in payload, payload
    assert "pending" not in payload, payload


def check_a_breakpoint_that_cannot_be_reported_is_not_left_behind(bridge):
    # gdb has created the breakpoint by the time the reply is assembled. If
    # that fails, the C++ side sees "no breakpoint" and can never remove it, so
    # nothing may stay armed in the inferior.
    peer = Peer(bridge)
    gdb.raiseOnReport = True
    try:
        peer.request("qtc/insertBreakpoint",
                     {"modelid": 32, "type": 1, "file": "/tmp/main.cpp",
                      "line": 10, "enabled": True})
    finally:
        gdb.raiseOnReport = False
    body = responsesOf(peer.messages(), "qtc/insertBreakpoint")[0]["body"]
    assert not body["bkpt"], "the failure was reported as a breakpoint: %s" % body
    assert body.get("error"), "no error reported: %s" % body
    assert gdb.breakpointObjects[-1].deleted, \
        "the breakpoint gdb created stayed behind"


def check_windows_paths_survive_the_payload(bridge):
    # The breakpoint payload is MI text parsed by the C++ GdbMi reader, so a
    # Windows path must not leave a stray escape sequence in it.
    peer = Peer(bridge)
    path = r"C:\Users\dummy\main.cpp"
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 3, "type": 1, "file": path, "line": 7, "enabled": True})
    body = responsesOf(peer.messages(), "qtc/insertBreakpoint")[0]["body"]
    payload = body["bkpt"]
    assert r"C:\\Users\\dummy\\main.cpp" in payload, \
        "backslashes not escaped: %s" % payload


def check_watchpoint_is_not_asked_for_locations(bridge):
    # A watchpoint has no code location. Asking gdb for one aborts it, and
    # reporting "pending" instead leaves it displayed as unresolved forever.
    peer = Peer(bridge)
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 21, "type": 11, "expression": "counter",
                  "enabled": True})
    body = responsesOf(peer.messages(), "qtc/insertBreakpoint")[0]["body"]
    payload = body["bkpt"]
    assert payload, "no breakpoint reported: %s" % body
    assert "pending" not in payload, "reported as pending: %s" % payload
    assert 'what="counter"' in payload, payload
    assert 'type="hw watchpoint"' in payload, payload


def check_interrupt_does_not_end_the_session(bridge):
    # An interrupt reaches the bridge as KeyboardInterrupt, which is not an
    # Exception: unhandled, it would take the read loop with it.
    peer = Peer(bridge)

    def onExecute(command):
        if command == "continue":
            raise KeyboardInterrupt()

    peer.server.running = True  # as the read loop has it
    gdb.onExecute = onExecute
    try:
        peer.request("continue")
    finally:
        gdb.onExecute = None

    messages = peer.messages()
    assert len(responsesOf(messages, "continue")) == 1, json.dumps(messages)
    assert len(eventsOf(messages, "stopped")) == 1, \
        "the interrupt was not reported as a stop: %s" % json.dumps(messages)
    assert peer.server.running, "the server gave up on the session"
    assert not eventsOf(messages, "exited"), "the session was ended: %s" % json.dumps(messages)


def check_failed_breakpoint_request_carries_the_modelid(bridge):
    # A failed request has no payload, but without the modelid the C++ side
    # cannot tell which breakpoint failed - and one left in its proceeding state
    # cannot even be removed afterwards.
    peer = Peer(bridge)

    def onExecute(command):
        raise GdbError("no symbol table is loaded")

    gdb.onExecute = onExecute
    try:
        peer.request("qtc/insertBreakpoint",
                     {"modelid": 12, "type": 2, "function": "nowhere",
                      "enabled": True})
        # No such breakpoint: a handler that answers the failure itself.
        peer.request("qtc/updateBreakpoint", {"modelid": 13, "id": 999})
        # And the two failures the dispatcher answers on a handler's behalf: an
        # unknown request, and a handler that raises.
        peer.request("qtc/nonsense", {"modelid": 14})
        raising = FakeBreakpoint("raising")
        raising.raiseOnDelete = True
        peer.server.breakpointById["4711"] = raising
        peer.request("qtc/removeBreakpoint", {"modelid": 15, "id": 4711})
    finally:
        gdb.onExecute = None

    messages = peer.messages()  # once: a second drain would only idle out
    for command, modelid in (("qtc/insertBreakpoint", 12), ("qtc/updateBreakpoint", 13),
                             ("qtc/nonsense", 14), ("qtc/removeBreakpoint", 15)):
        answers = responsesOf(messages, command)
        assert len(answers) == 1, json.dumps(answers)
        body = answers[0].get("body") or {}
        assert body.get("modelid") == modelid, \
            "%s answered without the modelid: %s" % (command, answers[0])


def check_moving_a_breakpoint_recreates_it(bridge):
    # gdb cannot move a breakpoint, so an update that changes the location has
    # to delete and recreate it instead of acknowledging a move that never
    # happened.
    peer = Peer(bridge)
    peer.request("qtc/insertBreakpoint",
                 {"modelid": 5, "type": 1, "file": "/tmp/main.cpp", "line": 10,
                  "enabled": True})
    inserted = gdb.breakpointObjects[-1]
    peer.buffer = b""

    # Same location, only the condition differs: kept in place.
    peer.request("qtc/updateBreakpoint",
                 {"id": inserted._number, "modelid": 5, "type": 1,
                  "file": "/tmp/main.cpp", "line": 10, "enabled": True,
                  "condition": "78203e2030"})
    assert not inserted.deleted, "the breakpoint was recreated for a condition change"
    assert inserted.condition == "x > 0", inserted.condition

    # Different line: recreated.
    peer.buffer = b""
    peer.request("qtc/updateBreakpoint",
                 {"id": inserted._number, "modelid": 5, "type": 1,
                  "file": "/tmp/main.cpp", "line": 20, "enabled": True})
    assert inserted.deleted, "the moved breakpoint was not recreated"
    fresh = gdb.breakpointObjects[-1]
    assert fresh is not inserted, "no new breakpoint was created"
    assert fresh.kwargs.get("line") == 20, fresh.kwargs
    body = responsesOf(peer.messages(), "qtc/updateBreakpoint")[0]["body"]
    assert body["bkpt"], "the reply carries no breakpoint"


def check_launch_passes_cwd_and_environment(bridge):
    # The debuggee must not silently inherit gdb's working directory and
    # environment; a Qt application then typically fails to start.
    peer = Peer(bridge)
    peer.request("launch", {"program": "/tmp/app",
                            "args": ["--plain", "two words", 'has"quote'],
                            "cwd": "/tmp/a directory with spaces",
                            "env": [{"name": "LD_LIBRARY_PATH", "value": "/opt/qt/lib",
                                     "unset": False},
                                    {"name": "LC_ALL", "value": "", "unset": True}]})
    assert 'file "/tmp/app"' in gdb.commands, \
        "the program was not quoted like the arguments: %s" % gdb.commands
    assert "set cwd /tmp/a directory with spaces" in gdb.commands, \
        "working directory not applied, or quoted: %s" % gdb.commands
    assert "set environment LD_LIBRARY_PATH=/opt/qt/lib" in gdb.commands, gdb.commands
    assert "unset environment LC_ALL" in gdb.commands, gdb.commands
    # One argument per entry: an argument with spaces must not become two.
    assert 'set args "--plain" "two words" "has\\"quote"' in gdb.commands, \
        "arguments not quoted individually: %s" % gdb.commands


def check_data_requests_use_the_dumpers(bridge):
    # The interface's data plane is the dumpers' own output, and a reply has to
    # carry the token back or the caller cannot tell what it answers.
    peer = Peer(bridge)
    peer.request("qtc/fetchStack", {"token": 7, "limit": -1})
    responses = responsesOf(peer.messages(), "qtc/fetchStack")
    assert len(responses) == 1, responses
    body = responses[0]["body"]
    assert body["dumperResult"] == 'stack={frames=[{level="0",function="main"}]}', body
    assert body["token"] == 7, "the token did not travel back: %s" % body
    assert peer.server.dumper.calls == ["fetchStack"], peer.server.dumper.calls
    # Captured, not printed: a dumper writing to the protocol stream would have
    # broken the framing above.
    assert peer.server.dumper.reported == [], peer.server.dumper.reported

    threads = Peer(bridge)
    threads.request("qtc/fetchThreads", {"token": 8})
    body = responsesOf(threads.messages(), "qtc/fetchThreads")[0]["body"]
    assert body["dumperResult"].startswith("threads="), body
    assert body["token"] == 8, body

    # An error record has the shape of a result one; passed on as a dumper
    # result the C++ side would parse the message as threads.
    gdb.miErrorRecord = '^error,msg="No threads."\n'
    try:
        failing = Peer(bridge)
        failing.request("qtc/fetchThreads", {"token": 9})
        answer = responsesOf(failing.messages(), "qtc/fetchThreads")[0]
    finally:
        gdb.miErrorRecord = None
    assert answer["success"] is False, answer
    assert "No threads." in answer.get("message", ""), answer


def check_startup_commands_reach_gdb(bridge):
    # The user's "Additional Startup Commands" and the script that can replace
    # them: without these the setting is silently ignored.
    peer = Peer(bridge)
    peer.request("qtc/runStartupCommands",
                 {"commands": "set print pretty on\n\nset listsize 20"})
    sent = [c for c in gdb.commands if c.startswith("set ")]
    assert sent == ["set print pretty on", "set listsize 20"], sent

    del gdb.commands[:]
    peer.request("qtc/runStartupCommands", {"script": "/tmp/a dir/init.gdb"})
    assert "source /tmp/a dir/init.gdb" in gdb.commands, gdb.commands

    del gdb.commands[:]
    peer.request("qtc/runStartupCommands", {"script": "/tmp/init.gdb\nkill"})
    assert not any(c.startswith("source") for c in gdb.commands), \
        "a script path with a newline was sourced anyway: %s" % gdb.commands


def check_an_unusable_program_fails_the_launch(bridge):
    # Loading no executable and answering 'success' anyway makes the session
    # start and exit again with the reason only in the log.
    peer = Peer(bridge)
    peer.request("launch", {"program": "/tmp/app\nkill"})
    responses = responsesOf(peer.messages(), "launch")
    assert len(responses) == 1, responses
    assert responses[0]["success"] is False, responses[0]
    assert not any(c.startswith("file ") for c in gdb.commands), \
        "an executable was loaded anyway: %s" % gdb.commands


def check_module_symbols_are_fetched(bridge):
    # ShowModuleSymbolsCapability is claimed, so "Show Symbols" has to answer
    # with something; gdb only writes msymbols to a file.
    peer = Peer(bridge)
    peer.request("qtc/fetchSymbols", {"module": "/lib/a dir/libc.so.6"})
    body = responsesOf(peer.messages(), "qtc/fetchSymbols")[0]["body"]
    assert body["module"] == "/lib/a dir/libc.so.6", body
    assert len(body["symbols"]) == 2, body
    assert body["symbols"][0] == {"state": "A", "address": "0x16bd64",
                                  "name": "_DYNAMIC", "section": "",
                                  "demangled": "moc_qudpsocket.cpp"}, body
    assert body["symbols"][1]["section"] == ".plt", body
    assert body["symbols"][1]["demangled"] == "myns::QFile::QFile()", body
    # The listing went through a file, which has to be gone again.
    written = [c for c in gdb.commands if c.startswith("maint print msymbols")]
    assert len(written) == 1, gdb.commands
    path = written[0].rsplit(" -- ", 1)[1].strip('"')
    assert not os.path.exists(path), "the temporary listing was left behind: " + path


def check_symbols_are_loaded_for_a_module(bridge):
    peer = Peer(bridge)
    peer.request("qtc/loadSymbols", {"module": "/usr/lib/a dir/libfoo.so.1"})
    # 'sharedlibrary' takes a regular expression, so the separators become the
    # wildcard, as GdbEngine does it.
    assert "sharedlibrary .usr.lib.a.dir.libfoo.so.1" in gdb.commands, gdb.commands

    del gdb.commands[:]
    peer.request("qtc/loadSymbols", {"all": True})
    assert "sharedlibrary .*" in gdb.commands, gdb.commands


def check_a_loaded_library_is_reported(bridge):
    # Without this the modules view stays empty until the user asks for it;
    # MI reports the same thing as =library-loaded.
    peer = Peer(bridge)
    objfile = types.SimpleNamespace(filename="/usr/lib/libc.so.6", owner=None)
    for handler in gdb.events.new_objfile.handlers:
        handler(types.SimpleNamespace(new_objfile=objfile))
    events = eventsOf(peer.messages(), "qtc/library")
    assert len(events) == 1, events
    assert events[0]["body"] == {"reason": "loaded", "path": "/usr/lib/libc.so.6"}, events[0]

    # Separate debug info belongs to an entry that was reported already.
    owned = types.SimpleNamespace(filename="/usr/lib/debug/libc.so.6.debug",
                                  owner=objfile)
    for handler in gdb.events.new_objfile.handlers:
        handler(types.SimpleNamespace(new_objfile=owned))
    for handler in gdb.events.free_objfile.handlers:
        handler(types.SimpleNamespace(objfile=objfile))
    reasons = [e["body"]["reason"] for e in eventsOf(peer.messages(), "qtc/library")]
    assert reasons == ["loaded", "unloaded"], reasons


def check_a_rejected_argument_fails_the_launch(bridge):
    # Dropping the whole argument list would run the debuggee differently
    # instead of saying that it cannot be run as asked.
    peer = Peer(bridge)
    peer.request("launch", {"program": "/tmp/app",
                            "args": ["--fine", "two\nlines"]})
    responses = responsesOf(peer.messages(), "launch")
    assert len(responses) == 1, responses
    assert responses[0]["success"] is False, responses[0]
    assert not any(c.startswith("set args") for c in gdb.commands), \
        "arguments were applied anyway: %s" % gdb.commands


def check_a_newline_cannot_smuggle_a_gdb_command(bridge):
    # 'set cwd' and 'set environment' take the rest of the line and have no
    # escaping, so a newline in a value would end the command and gdb would run
    # what follows as the next one.
    peer = Peer(bridge)
    peer.request("launch", {"program": "/tmp/app",
                            "cwd": "/tmp/app\nkill",
                            "env": [{"name": "LD_PRELOAD",
                                     "value": "/opt/lib\nshell rm -rf /",
                                     "unset": False}]})
    for command in gdb.commands:
        assert "\n" not in command, "command spans lines: %r" % command
    assert not any(c.startswith("set cwd") for c in gdb.commands), \
        "working directory with a newline was applied: %s" % gdb.commands
    assert not any(c.startswith("set environment LD_PRELOAD") for c in gdb.commands), \
        "environment value with a newline was applied: %s" % gdb.commands


checks = {
    "framing": check_framing,
    "initialize-reports-dumpers": check_initialize_reports_dumpers,
    "launch-passes-cwd-and-environment": check_launch_passes_cwd_and_environment,
    "server-owns-the-stop-events": check_server_owns_the_stop_events,
    "attach-failure-is-reported": check_attach_failure_is_reported,
    "catchpoints-are-created": check_catchpoints_are_created,
    "a-catchpoint-keeps-its-settings": check_a_catchpoint_keeps_its_settings,
    "a-catchpoint-reports-its-catch-type": check_a_catchpoint_reports_its_catch_type,
    "a-breakpoint-that-cannot-be-reported-is-not-left-behind":
        check_a_breakpoint_that_cannot_be_reported_is_not_left_behind,
    "extra-dumpers-are-loaded": check_extra_dumpers_are_loaded,
    "target-configuration-reaches-gdb": check_target_configuration_reaches_gdb,
    "windows-paths-survive-the-payload": check_windows_paths_survive_the_payload,
    "moving-a-breakpoint-recreates-it": check_moving_a_breakpoint_recreates_it,
    "watchpoint-is-not-asked-for-locations": check_watchpoint_is_not_asked_for_locations,
    "interrupt-does-not-end-the-session": check_interrupt_does_not_end_the_session,
    "failed-breakpoint-request-carries-the-modelid":
        check_failed_breakpoint_request_carries_the_modelid,
    "stdout-cannot-corrupt-the-protocol": check_stdout_cannot_corrupt_the_protocol,
    "temporary-breakpoint-stop": check_temporary_breakpoint_stop,
    "shutdown-quits-gdb": check_shutdown_quits_gdb,
    "breakpoint-source-with-spaces": check_breakpoint_source_with_spaces,
    "inferior-output-event": check_inferior_output_event,
    "a-resuming-console-command-reports-the-stop":
        check_a_resuming_console_command_reports_the_stop,
    "a-failing-console-command-answers-with-an-error":
        check_a_failing_console_command_answers_with_an_error,
    "one-module-is-not-reported-twice": check_one_module_is_not_reported_twice,
    "a-newline-cannot-smuggle-a-gdb-command":
        check_a_newline_cannot_smuggle_a_gdb_command,
    "an-unusable-program-fails-the-launch":
        check_an_unusable_program_fails_the_launch,
    "a-rejected-argument-fails-the-launch":
        check_a_rejected_argument_fails_the_launch,
    "a-loaded-library-is-reported": check_a_loaded_library_is_reported,
    "module-symbols-are-fetched": check_module_symbols_are_fetched,
    "symbols-are-loaded-for-a-module": check_symbols_are_loaded_for_a_module,
    "startup-commands-reach-gdb": check_startup_commands_reach_gdb,
    "data-requests-use-the-dumpers": check_data_requests_use_the_dumpers,
}

if check not in checks:
    sys.stderr.write("unknown check %r; known: %s\n"
                     % (check, ", ".join(sorted(checks))))
    sys.exit(2)

try:
    checks[check](loadBridge())
except Skip as reason:
    sys.stderr.write("SKIP: %s\n" % reason)
    sys.exit(3)
except AssertionError as error:
    sys.stderr.write("%s failed: %s\n" % (check, error))
    sys.exit(1)
except Exception as error:
    import traceback
    traceback.print_exc()
    sys.stderr.write("%s raised: %s\n" % (check, error))
    sys.exit(1)
