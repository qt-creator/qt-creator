# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# A DAP-shaped server hosted inside gdb's Python, talking to Qt Creator's
# BridgeEngine.
#
# This is a first, deliberately simple version:
#   - It owns the gdb process stdio and speaks Content-Length-framed JSON,
#     the same framing the C++ DapClient parses.
#   - It is synchronous and single-threaded: runDapServer() blocks the gdb
#     main thread in a read loop and drives gdb via gdb.execute(). Execution
#     commands (run/continue/step) block until the inferior stops; the stop is
#     then reported as a DAP 'stopped' event.
#   - Consequence/limitation: asynchronous 'pause' while the inferior runs is
#     not supported yet (we are not reading stdin while blocked in a run), and
#     variables use gdb's own formatting, not the Qt dumpers. Both are follow-up
#     steps (native qtc/ variables, async transport).

import base64
import json
import os
import sys
import threading
import types

try:
    import termios
except ImportError:  # not POSIX
    termios = None

import gdb


def gdbLineArgument(value, what):
    # Rest-of-line gdb commands take everything to the end of the line and
    # cannot be escaped, so a newline would end the command and run the rest as
    # the next one.
    text = str(value)
    if '\n' in text or '\r' in text:
        warn('%s contains a newline, ignoring it: %r' % (what, text))
        return None
    return text


def quoteArgument(argument):
    # Double quotes with backslash escapes are understood both by the shell gdb
    # starts the inferior through and by gdb's own argument splitting, so an
    # argument containing spaces survives either way.
    return '"%s"' % argument.replace('\\', '\\\\').replace('"', '\\"')


def warn(message):
    # Diagnostics must not go to stdout: that is the protocol stream. The C++
    # side reads stderr separately and shows it in the debugger log.
    sys.stderr.write('bridge: %s\n' % message)
    sys.stderr.flush()


class DapServer():
    def __init__(self, dumper):
        self.dumper = dumper
        self.seq = 0
        self.running = False
        self.attachMode = False
        self.dumperSetup = ''

        # The protocol stream. Set up by _claimStdio() before anything is
        # written; the inferior output pump uses it from its own thread.
        self.protocolFd = 1
        self.sendLock = threading.RLock()
        self.inferiorTty = None

        # Native qtc/ breakpoints: gdb.Breakpoint by its (stringified) number,
        # and the arguments each was created for.
        self.breakpointById = {}
        self.breakpointArgsById = {}
        # Extra gdb breakpoints belonging to one of ours (catch fork/vfork).
        self.companionBreakpoints = {}

        # Rebuilt on every stop: maps a DAP frame id to a gdb.Frame.
        self.frameForId = {}

        # Recorded by the gdb event handlers during a blocking execution
        # command, consumed right after it returns.
        self.lastStopEvent = None
        self.lastExitCode = None
        self.inferiorExited = False

        gdb.events.stop.connect(self._onStop)
        gdb.events.exited.connect(self._onExited)

    #######################################################################
    # Transport
    #######################################################################

    def _readExactly(self, count):
        data = b''
        while len(data) < count:
            chunk = os.read(0, count - len(data))
            if not chunk:
                return None
            data += chunk
        return data

    def _readMessage(self):
        header = b''
        while not header.endswith(b'\r\n\r\n'):
            byte = os.read(0, 1)
            if not byte:
                return None
            header += byte

        length = 0
        for line in header.split(b'\r\n'):
            if line.lower().startswith(b'content-length:'):
                length = int(line.split(b':', 1)[1].strip())
        if length <= 0:
            return {}

        body = self._readExactly(length)
        if body is None:
            return None
        return json.loads(body.decode('utf-8'))

    def _send(self, obj):
        with self.sendLock:
            # Numbered under the same lock that writes, or two threads can take
            # their numbers in one order and write in the other.
            self.seq += 1
            obj['seq'] = self.seq
            data = json.dumps(obj).encode('utf-8')
            message = ('Content-Length: %d\r\n\r\n' % len(data)).encode('ascii') + data
            written = 0
            while written < len(message):
                written += os.write(self.protocolFd, message[written:])

    def sendResponse(self, request, body=None, success=True, message=None):
        response = {
            'type': 'response',
            'request_seq': request.get('seq', 0),
            'command': request.get('command', ''),
            'success': success,
        }
        if message is not None:
            response['message'] = message
        if body is None:
            # A failure carries no payload of its own, but the modelid still has
            # to travel back: it is how the C++ side finds the breakpoint a
            # failed request belonged to. Harmless for other requests, which do
            # not send one.
            arguments = request.get('arguments', {})
            for key in ('modelid', 'token'):
                if arguments.get(key) is not None:
                    body = body or {}
                    body[key] = arguments[key]
        if body is not None:
            response['body'] = body
        self._send(response)

    def sendEvent(self, event, body=None):
        message = {'type': 'event', 'event': event}
        if body is not None:
            message['body'] = body
        self._send(message)

    #######################################################################
    # Main loop
    #######################################################################

    def _claimStdio(self):
        # The protocol gets a private copy of the original stdout, and fd 1 is
        # pointed at stderr. Anything that still writes to stdout - gdb's own
        # console output, a stray dumper print - then lands in the debugger log
        # instead of corrupting the framing.
        self.protocolFd = os.dup(1)
        os.dup2(2, 1)

    def _takeOverStopEvents(self):
        # A session can only have one stop handler. gdbbridge connects an
        # interpreter (native mixed) handler when it is imported, which runs
        # before ours and, for a breakpoint carrying an interpreter handler,
        # resumes the inferior on its own - we would then report a stop that no
        # longer holds. The bridge does not do native mixed debugging, so take
        # that handler out for as long as we own the process. Reviving it is
        # part of the native mixed work, which needs its own requests anyway.
        try:
            self.dumper.disableInterpreterStopHandler()
        except Exception as error:
            warn('cannot take over the stop events: %s' % error)

    def _setupInferiorTty(self):
        # The inferior must not share our stdout either. Give it a pty and
        # forward what it writes as DAP 'output' events, which is what puts it
        # in the Application Output pane.
        if termios is None:
            return
        try:
            master, slave = os.openpty()
            attrs = termios.tcgetattr(slave)
            attrs[1] &= ~termios.ONLCR  # no \n -> \r\n on the way out
            attrs[3] &= ~termios.ECHO
            termios.tcsetattr(slave, termios.TCSANOW, attrs)
            gdb.execute('set inferior-tty %s' % os.ttyname(slave), to_string=True)
        except (OSError, gdb.error) as error:
            warn('no inferior tty: %s' % error)
            return

        # Keep the slave open, so reading the master blocks between runs
        # instead of failing once the inferior is gone.
        self.inferiorTty = (master, slave)
        thread = threading.Thread(target=self._pumpInferiorOutput, args=(master,))
        thread.daemon = True
        thread.start()

    def _pumpInferiorOutput(self, master):
        while True:
            try:
                data = os.read(master, 4096)
            except OSError:
                return
            if not data:
                return
            self.sendEvent('output', {'category': 'stdout',
                                      'output': data.decode('utf-8', 'replace')})

    def run(self):
        self._claimStdio()
        self._takeOverStopEvents()

        # Keep gdb quiet and non-interactive; this loop owns stdio.
        for command in ['set pagination off', 'set confirm off',
                        'set width 0', 'set height 0',
                        'set breakpoint pending on']:
            try:
                gdb.execute(command, to_string=True)
            except gdb.error:
                pass

        # Register the Qt/stdlib type dumpers so qtc/fetchVariables produces
        # Qt-aware output (the C++ engine no longer sends a separate
        # loadDumpers command).
        try:
            # The return value lists the types the dumpers know and the display
            # formats they offer; the C++ side needs it for the format menus.
            self.dumperSetup = self.dumper.setupDumpers()
        except Exception as error:
            warn('setupDumpers failed: %s' % error)

        self._setupInferiorTty()

        self.running = True
        while self.running:
            try:
                message = self._readMessage()
            except Exception as error:
                warn('DAP read error: %s' % error)
                break
            if message is None:
                break
            if message.get('type') == 'request':
                self._dispatch(message)

    def _dispatch(self, request):
        command = request.get('command', '')
        # qtc/ extension requests map to cmd_qtc_<name> handlers.
        handler = getattr(self, 'cmd_' + command.replace('/', '_'), None)
        if handler is None:
            self.sendResponse(request, success=False,
                              message='unhandled request: %s' % command)
            return
        try:
            handler(request)
        except Exception as error:
            warn('DAP handler %s failed: %s' % (command, error))
            self.sendResponse(request, success=False, message=str(error))

    #######################################################################
    # gdb event handlers (record only; emitted after the execute returns)
    #######################################################################

    def _onStop(self, event):
        self.lastStopEvent = event

    def _onExited(self, event):
        self.inferiorExited = True
        self.lastExitCode = getattr(event, 'exit_code', None)

    #######################################################################
    # Execution helper
    #######################################################################

    def _execute(self, command):
        # Run a command that resumes the inferior and blocks until it stops or
        # exits, then report the resulting state as a DAP event.
        self.lastStopEvent = None
        self.inferiorExited = False
        self.lastExitCode = None
        try:
            gdb.execute(command, to_string=True)
        except gdb.error as error:
            warn('DAP execute %r failed: %s' % (command, error))
        except KeyboardInterrupt:
            # How an interrupt arrives while the inferior runs: it is not a
            # gdb.error, and not even an Exception, so letting it through would
            # take the read loop - and the session - with it. The inferior is
            # stopped now, which is what was asked for; report it below.
            pass

        self._reportInferiorState()

    def _reportInferiorState(self):
        # Whatever the inferior did while a command was running: gone, or
        # stopped somewhere.
        inferior = gdb.selected_inferior()
        if self.inferiorExited or not inferior.threads():
            code = self.lastExitCode if self.lastExitCode is not None else 0
            # No 'terminated': the C++ side has no mapping for it, so it would
            # only show up as an unknown event. 'exited' drives the shutdown.
            self.sendEvent('exited', {'exitCode': code})
            return

        self._reportStopped()

    def _reportStopped(self):
        reason = 'step'
        hitBreakpointIds = []
        event = self.lastStopEvent
        if event is not None and hasattr(event, 'breakpoints'):
            for bp in event.breakpoints:
                # A temporary breakpoint is deleted the moment it is hit, and
                # reading it then raises, so there is no id to report for it.
                try:
                    hitBreakpointIds.append(bp.number)
                except RuntimeError:
                    pass
            if hitBreakpointIds:
                reason = 'breakpoint'
        elif event is not None and hasattr(event, 'stop_signal'):
            reason = 'exception'

        self._dropDeadBreakpoints()

        thread = gdb.selected_thread()
        threadId = thread.global_num if thread is not None else 0
        self.sendEvent('stopped', {
            'reason': reason,
            'threadId': threadId,
            'allThreadsStopped': True,
            'hitBreakpointIds': hitBreakpointIds,
        })

    #######################################################################
    # Lifecycle requests
    #######################################################################

    def cmd_initialize(self, request):
        self.sendResponse(request, body={
            'supportsConfigurationDoneRequest': True,
            'supportsFunctionBreakpoints': True,
            'supportsConditionalBreakpoints': True,
            'supportsEvaluateForHovers': True,
            # What the dumpers registered, verbatim from setupDumpers().
            'qtcDumpers': self.dumperSetup,
        })
        self.sendEvent('initialized')

    def cmd_launch(self, request):
        self.attachMode = False
        arguments = request.get('arguments', {})
        program = gdbLineArgument(arguments.get('program') or '', 'the program path')
        if program:
            gdb.execute('file %s' % quoteArgument(program), to_string=True)
        quoted = []
        for argument in arguments.get('args') or []:
            text = gdbLineArgument(argument, 'an inferior argument')
            if text is None:
                self.sendResponse(request, success=False,
                                  message='An inferior argument contains a newline: %r'
                                          % argument)
                return
            quoted.append(quoteArgument(text))
        if quoted:
            gdb.execute('set args %s' % ' '.join(quoted), to_string=True)
        # The debuggee runs where and how the run configuration says, not
        # wherever gdb happens to sit. No quoting for these: 'set cwd' and
        # 'set environment' take the rest of the line, so a path with spaces
        # works as is while quotes would become part of it.
        cwd = gdbLineArgument(arguments.get('cwd') or '', 'the working directory')
        if cwd:
            gdb.execute('set cwd %s' % cwd, to_string=True)
        for item in arguments.get('env', []):
            name = gdbLineArgument(item.get('name') or '', 'an environment variable name')
            if not name:
                continue
            if item.get('unset'):
                gdb.execute('unset environment %s' % name, to_string=True)
                continue
            value = gdbLineArgument(item.get('value', ''), 'the value of %s' % name)
            if value is None:
                continue
            gdb.execute('set environment %s=%s' % (name, value), to_string=True)
        # Do not run yet; wait for configurationDone so breakpoints are set.
        self.sendResponse(request)

    def cmd_attach(self, request):
        self.attachMode = True
        pid = request.get('arguments', {}).get('pid', 0)
        try:
            gdb.execute('attach %d' % int(pid), to_string=True)
        except gdb.error as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request)

    def cmd_configurationDone(self, request):
        self.sendResponse(request)
        if self.attachMode:
            # Attaching already stopped the target; report the current state.
            self._reportStopped()
        else:
            # The inferior starts now and runs to the first stop.
            self._execute('run')

    def _shutdown(self, request, terminateDebuggee=True):
        try:
            gdb.execute('kill' if terminateDebuggee else 'detach', to_string=True)
        except gdb.error:
            pass
        self.sendResponse(request)
        self.running = False
        # Leaving the loop only ends the -ex command; gdb would then read the
        # remaining protocol stream as gdb commands. Quit for good.
        gdb.execute('quit', to_string=True)

    def cmd_disconnect(self, request):
        arguments = request.get('arguments', {})
        self._shutdown(request, arguments.get('terminateDebuggee', True))

    def cmd_terminate(self, request):
        self._shutdown(request)

    #######################################################################
    # Execution requests
    #######################################################################

    def cmd_continue(self, request):
        self.sendResponse(request, body={'allThreadsContinued': True})
        self._execute('continue')

    def cmd_next(self, request):
        self.sendResponse(request)
        self._execute('next')

    def cmd_stepIn(self, request):
        self.sendResponse(request)
        self._execute('step')

    def cmd_stepOut(self, request):
        self.sendResponse(request)
        self._execute('finish')

    def cmd_qtc_jumpToLine(self, request):
        # Move the execution point without resuming: set $pc to the target
        # address. The C++ side stays stopped and refreshes the views.
        args = request.get('arguments', {})
        address = args.get('address')
        try:
            if address:
                pc = int(str(address), 0)
            else:
                # Resolve via a throwaway breakpoint rather than a linespec,
                # so a source path containing spaces still resolves.
                probe = gdb.Breakpoint(source=args.get('file', ''),
                                       line=int(args.get('line', 0)),
                                       temporary=True)
                try:
                    locations = self._locationsOf(probe)
                    pc = int(locations[0].address) if locations else 0
                finally:
                    probe.delete()  # a probe left behind would stop the inferior
                if not pc:
                    raise gdb.error('cannot resolve the target location')
            gdb.execute('set $pc = 0x%x' % pc, to_string=True)
        except gdb.error as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request)

    def cmd_qtc_runToFunction(self, request):
        # Temporary breakpoint at the function, then continue until it is hit.
        func = request.get('arguments', {}).get('function', '')
        try:
            gdb.Breakpoint(function=func, temporary=True)
        except (gdb.error, RuntimeError) as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request)
        self._execute('continue')

    def cmd_qtc_runToLine(self, request):
        # Temporary breakpoint at the location, then continue until it is hit.
        args = request.get('arguments', {})
        address = args.get('address')
        try:
            if address:
                gdb.Breakpoint('*%s' % address, gdb.BP_BREAKPOINT, temporary=True)
            else:
                gdb.Breakpoint(source=args.get('file', ''),
                               line=int(args.get('line', 0)), temporary=True)
        except (gdb.error, RuntimeError) as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request)
        self._execute('continue')

    #######################################################################
    # Breakpoint requests
    #######################################################################

    # Native, incremental, tree-shaped breakpoints. Each request maps 1:1 to
    # Creator's insert/update/remove and returns the breakpoint in the MI
    # 'bkpt' shape (plus sub-location list), which the C++ side feeds to the
    # shared updateFromGdbOutput()/handleBkpt() logic.
    # Correlation is by the stable modelid echoed back, never by file:line.

    # BreakpointType enum values shared with breakpoint.h.
    BP_BY_FILE_AND_LINE = 1
    BP_BY_FUNCTION = 2
    BP_BY_ADDRESS = 3
    BP_AT_THROW = 4
    BP_AT_CATCH = 5
    BP_AT_MAIN = 6
    BP_AT_FORK = 7
    BP_AT_EXEC = 8
    BP_AT_SYSCALL = 9
    BP_WATCH_ADDRESS = 10
    BP_WATCH_EXPRESSION = 11

    # Throwing and catching are breaks in the C++ runtime, as in GdbEngine.
    FUNCTION_FOR_TYPE = {BP_AT_THROW: '__cxa_throw',
                         BP_AT_CATCH: '__cxa_begin_catch'}
    # The rest are catchpoints, which gdb only creates from the CLI. A fork
    # breakpoint means both flavours, again as in GdbEngine.
    CATCH_KINDS = {BP_AT_FORK: ('fork', 'vfork'), BP_AT_EXEC: ('exec',),
                   BP_AT_SYSCALL: ('syscall',)}

    def _createCatchpoint(self, bptype):
        # gdb has no Python constructor for catchpoints; make them with the CLI
        # command and pick up what appeared.
        created = []
        for kind in self.CATCH_KINDS[bptype]:
            known = {bp.number for bp in (gdb.breakpoints() or ())}
            gdb.execute('catch %s' % kind, to_string=True)
            created += [bp for bp in (gdb.breakpoints() or ())
                        if bp.number not in known]
        if not created:
            raise gdb.error('gdb created no catchpoint')
        if len(created) > 1:
            # Only the first one is reported; the others go with it.
            self.companionBreakpoints[str(created[0].number)] = created[1:]
        return created[0]

    def _applyBreakpointArgs(self, bp, args):
        condition = args.get('condition', '')
        bp.condition = self.dumper.hexdecode(condition) if condition else ''
        bp.ignore_count = int(args.get('ignorecount', 0) or 0)
        bp.enabled = bool(args.get('enabled', True))

    def _applyBreakpointArgsToAll(self, bp, args):
        # A companion carries the same settings: a disabled fork breakpoint
        # that leaves its vfork twin armed still stops the inferior.
        self._applyBreakpointArgs(bp, args)
        for companion in self.companionBreakpoints.get(str(bp.number), ()):
            self._applyBreakpointArgs(companion, args)

    def _createGdbBreakpoint(self, args):
        bptype = args.get('type', self.BP_BY_FILE_AND_LINE)
        temporary = bool(args.get('oneshot'))
        if bptype in self.CATCH_KINDS:
            bp = self._createCatchpoint(bptype)
        elif bptype in self.FUNCTION_FOR_TYPE:
            bp = gdb.Breakpoint(function=self.FUNCTION_FOR_TYPE[bptype],
                                temporary=temporary)
        elif bptype == self.BP_AT_MAIN:
            bp = gdb.Breakpoint(function=args.get('function') or 'main',
                                temporary=temporary)
        elif bptype == self.BP_BY_FUNCTION:
            bp = gdb.Breakpoint(function=args.get('function', ''),
                                temporary=temporary)
        elif bptype == self.BP_BY_ADDRESS:
            bp = gdb.Breakpoint('*0x%x' % args.get('address', 0), gdb.BP_BREAKPOINT,
                                temporary=temporary)
        elif bptype in (self.BP_WATCH_ADDRESS, self.BP_WATCH_EXPRESSION):
            expr = args.get('expression') or ('*0x%x' % args.get('address', 0))
            bp = gdb.Breakpoint(expr, gdb.BP_WATCHPOINT)
        else:
            # Keyword form, so a source path containing spaces still resolves.
            bp = gdb.Breakpoint(source=args.get('file', ''),
                                line=int(args.get('line', 0)),
                                temporary=temporary)

        self._applyBreakpointArgsToAll(bp, args)
        return bp

    def _fillLocationDict(self, target, location):
        if location.address is not None:
            target['addr'] = '0x%x' % location.address
        if location.function:
            target['func'] = location.function
        source = location.source
        if source:
            target['file'] = source[0]
            target['line'] = source[1]
        if location.fullname:
            target['fullname'] = location.fullname

    def _dropDeadBreakpoints(self):
        # gdb deletes a temporary breakpoint as it is hit. Keeping the object
        # would hold it alive with every access raising, so let it go.
        for key, bp in list(self.breakpointById.items()):
            if not bp.is_valid():
                del self.breakpointById[key]
                self.breakpointArgsById.pop(key, None)

    def _functionAt(self, pc):
        # gdb.Breakpoint.locations knows the function; without it (gdb < 13) the
        # block at the address does.
        try:
            block = gdb.block_for_pc(int(pc))
        except (gdb.error, RuntimeError):
            return None
        while block is not None:
            if block.function is not None:
                return block.function.name
            block = block.superblock
        return None

    def _locationsOf(self, bp):
        try:
            return list(bp.locations)
        except AttributeError:
            # gdb.Breakpoint.locations arrived in gdb 13. Without it, resolve
            # the requested location instead of reporting every breakpoint as
            # pending.
            return self._decodedLocations(bp)
        except Exception:
            return []

    def _decodedLocations(self, bp):
        # A stand-in for gdb.Breakpoint.locations on gdb < 13: ask gdb to
        # resolve the breakpoint's own location string.
        if not bp.location:
            return []
        try:
            sals = gdb.decode_line(bp.location)[1] or []
        except gdb.error:
            return []
        decoded = []
        for sal in sals:
            source = None
            if sal.symtab is not None:
                source = (sal.symtab.filename, sal.line)
            decoded.append(types.SimpleNamespace(
                address=int(sal.pc), function=self._functionAt(sal.pc), source=source,
                fullname=sal.symtab.fullname() if sal.symtab is not None else None,
                enabled=bp.enabled))
        return decoded

    def _isWatchpoint(self, bp, requested):
        if requested is not None:
            return requested in (self.BP_WATCH_ADDRESS, self.BP_WATCH_EXPRESSION)
        try:
            # Not one of ours. gdb refuses to map a type it does not know
            # rather than returning it, so this can raise.
            return bp.type != gdb.BP_BREAKPOINT
        except Exception:
            return False

    def _breakpointToMi(self, bp):
        args = self.breakpointArgsById.get(str(bp.number), {})
        requested = args.get('type')
        result = {
            'number': bp.number,
            'enabled': 'y' if bp.enabled else 'n',
            'disp': 'del' if bp.temporary else 'keep',
        }
        if bp.condition:
            result['cond'] = bp.condition

        if self._isWatchpoint(bp, requested):
            # A watchpoint has no code location, and must not be asked for one:
            # gdb.Breakpoint.locations dies on it (gdb 17.1: internal-error,
            # gdbarch_addr_bit assertion). Report what it watches instead, which
            # is also what keeps it from being displayed as forever pending.
            result['type'] = 'hw watchpoint'
            result['what'] = args.get('expression') or bp.location or ''
            return self.dumper.resultToMi(result)

        if requested in self.CATCH_KINDS:
            # The shape gdb's MI uses for a catchpoint, which is what
            # BreakpointParameters::updateFromGdbOutput() reads: a breakpoint
            # carrying a catch-type. Reporting it as a watchpoint instead would
            # replace the type the user asked for. It has no code location
            # either, and none reported would show it as forever pending.
            result['type'] = 'breakpoint'
            result['catch-type'] = self.CATCH_KINDS[requested][0]
            result['what'] = self.CATCH_KINDS[requested][0]
            return self.dumper.resultToMi(result)

        result['type'] = 'breakpoint'
        locations = self._locationsOf(bp)
        if len(locations) == 1:
            self._fillLocationDict(result, locations[0])
        elif len(locations) > 1:
            result['addr'] = '<MULTIPLE>'
            subs = []
            for index, location in enumerate(locations, start=1):
                sub = {'number': '%d.%d' % (bp.number, index),
                       'enabled': 'y' if location.enabled else 'n'}
                self._fillLocationDict(sub, location)
                subs.append(sub)
            result['locations'] = subs
        else:
            result['pending'] = bp.location or ''
        return self.dumper.resultToMi(result)

    def cmd_qtc_insertBreakpoint(self, request):
        args = request.get('arguments', {})
        body = {'modelid': args.get('modelid')}
        bp = None
        try:
            bp = self._createGdbBreakpoint(args)
            self.breakpointById[str(bp.number)] = bp
            self.breakpointArgsById[str(bp.number)] = args
            body['bkpt'] = self._breakpointToMi(bp)
        except Exception as error:
            warn('insertBreakpoint failed: %s' % error)
            # Whatever gdb created before the failure has to go with it: the
            # C++ side takes an empty bkpt as "there is no breakpoint" and
            # would never remove or disable one that stayed behind.
            if bp is not None:
                self._forgetBreakpoint(str(bp.number))
            body['bkpt'] = ''
            body['error'] = str(error)
        self.sendResponse(request, body=body)

    def cmd_qtc_updateBreakpoint(self, request):
        args = request.get('arguments', {})
        key = str(args.get('id'))
        bp = self.breakpointById.get(key)
        if bp is None:
            self.sendResponse(request, success=False, message='no such breakpoint')
            return

        # gdb changes a condition, an ignore count and the enabled state in
        # place, but it cannot move a breakpoint: for that it has to be
        # recreated, or we would acknowledge a move that never happened.
        if self._locationOf(args) != self._locationOf(self.breakpointArgsById.get(key, {})):
            self._forgetBreakpoint(key)
            # The insert reply carries the new breakpoint in the same shape,
            # and the C++ side feeds both through the same handler.
            self.cmd_qtc_insertBreakpoint(request)
            return

        self._applyBreakpointArgsToAll(bp, args)
        self.breakpointArgsById[key] = args
        self.sendResponse(request, body={'modelid': args.get('modelid'),
                                         'bkpt': self._breakpointToMi(bp)})

    @staticmethod
    def _locationOf(args):
        return (args.get('type'), args.get('file'), args.get('line'),
                args.get('function'), args.get('address'), args.get('expression'))

    def _forgetBreakpoint(self, key):
        self.breakpointArgsById.pop(key, None)
        bp = self.breakpointById.pop(key, None)
        for breakpoint in self.companionBreakpoints.pop(key, []) + ([bp] if bp else []):
            try:
                breakpoint.delete()
            except (gdb.error, RuntimeError):
                pass

    def cmd_qtc_removeBreakpoint(self, request):
        args = request.get('arguments', {})
        self._forgetBreakpoint(str(args.get('id')))
        self.sendResponse(request, body={'modelid': args.get('modelid')})

    #######################################################################
    # Inspection requests
    #######################################################################

    def cmd_threads(self, request):
        threads = []
        for thread in gdb.selected_inferior().threads():
            name = thread.name or ('Thread %d' % thread.global_num)
            threads.append({'id': thread.global_num, 'name': name})
        self.sendResponse(request, body={'threads': threads})

    def cmd_stackTrace(self, request):
        arguments = request.get('arguments', {})
        threadId = arguments.get('threadId', 0)
        self._selectThread(threadId)

        self.frameForId = {}
        frames = []
        frameId = 1
        try:
            frame = gdb.newest_frame()
        except gdb.error:
            frame = None
        while frame is not None and frame.is_valid():
            self.frameForId[frameId] = frame
            entry = {'id': frameId, 'name': frame.name() or '??', 'line': 0,
                     'column': 0}
            sal = frame.find_sal()
            if sal is not None and sal.symtab is not None:
                entry['line'] = sal.line
                entry['source'] = {'path': sal.symtab.fullname()}
            entry['instructionPointerReference'] = int(frame.pc())
            frames.append(entry)
            frameId += 1
            frame = frame.older()

        self.sendResponse(request, body={'stackFrames': frames,
                                         'totalFrames': len(frames)})

    def cmd_qtc_fetchVariables(self, request):
        # Native, dumper-aware variables: run the real Qt dumpers for the
        # requested frame and return their structured output verbatim; the C++
        # side parses it with GdbMi and feeds the shared updateLocalsView(). We
        # capture the dumper's report rather than let it print to the
        # (DAP-owned) stdout.
        args = request.get('arguments', {})

        frame = self.frameForId.get(args.get('frameid'))
        if frame is not None and frame.is_valid():
            frame.select()

        captured = {}
        original = self.dumper.reportResult

        def capture(result, unused):
            captured['result'] = result

        self.dumper.reportResult = capture
        try:
            self.dumper.fetchVariables(dict(args))
        finally:
            self.dumper.reportResult = original

        self.sendResponse(request, body={'dumperResult': captured.get('result', '')})

    def cmd_qtc_configureTarget(self, request):
        # Tell gdb where the target's sources and libraries are, before the
        # program is loaded.
        args = request.get('arguments', {})
        # 'set substitute-path' and 'directory' split their arguments like a
        # shell, so a path with spaces needs quoting. 'set sysroot' takes the
        # rest of the line and would keep the quotes.
        for mapping in args.get('sourcePathMap', []):
            source = gdbLineArgument(mapping.get('from', ''), 'source path (from)')
            target = gdbLineArgument(mapping.get('to', ''), 'source path (to)')
            if source is not None and target is not None:
                self._executeQuietly('set substitute-path %s %s'
                                     % (quoteArgument(source), quoteArgument(target)))
        for directory in args.get('sourceDirectories', []):
            directory = gdbLineArgument(directory, 'source directory')
            if directory is not None:
                self._executeQuietly('directory %s' % quoteArgument(directory))
        sysroot = gdbLineArgument(args.get('sysroot', ''), 'sysroot')
        if sysroot:
            self._executeQuietly('set sysroot %s' % sysroot)
            # A sysroot alone does not locate the sources; relocate the most
            # likely place for them too, as GdbEngine does.
            self._executeQuietly('set substitute-path /usr/src %s'
                                 % quoteArgument(sysroot + '/usr/src'))
        self.sendResponse(request)

    def _executeQuietly(self, command):
        try:
            gdb.execute(command, to_string=True)
        except gdb.error as error:
            warn('%r failed: %s' % (command, error))

    def cmd_qtc_fetchModules(self, request):
        # gdb's Python API lists the objfiles but not where they are loaded or
        # whether their symbols are in, so 'info sharedlibrary' fills that in.
        # The two sources can spell one module differently - a symlink, a
        # sysroot prefix, a '..' - so they are matched on the resolved path
        # while the reported one stays as it came. Resolving rather than
        # comparing file names keeps two same-named libraries apart.
        modules = {}
        for objfile in gdb.objfiles():
            path = objfile.filename
            if not path or getattr(objfile, 'owner', None) is not None:
                continue  # an owner means separate debug info for that entry
            modules[os.path.realpath(path)] = {'path': path, 'symbolsRead': True,
                                               'startAddress': 0, 'endAddress': 0}
        try:
            listing = gdb.execute('info sharedlibrary', to_string=True)
        except gdb.error:
            listing = ''
        for line in listing.splitlines():
            fields = line.split()
            if len(fields) < 4 or not fields[0].startswith('0x'):
                continue
            rest = fields[3:]
            if rest[0] == '(*)':  # symbols not fully read yet
                rest = rest[1:]
            path = ' '.join(rest)  # may contain spaces
            entry = modules.setdefault(os.path.realpath(path), {'path': path})
            entry['startAddress'] = int(fields[0], 16)
            entry['endAddress'] = int(fields[1], 16)
            entry['symbolsRead'] = fields[2] == 'Yes'
        self.sendResponse(request, body={'modules': list(modules.values())})

    def cmd_qtc_executeCommand(self, request):
        # The debugger console. Capture what gdb prints: its stdout is the
        # protocol stream.
        command = request.get('arguments', {}).get('command', '')
        self.lastStopEvent = None
        self.inferiorExited = False
        self.lastExitCode = None
        try:
            body = {'output': gdb.execute(command, to_string=True) or ''}
        except KeyboardInterrupt:
            # An interrupt that arrived while the command ran the inferior; the
            # state it left behind is reported below.
            body = {'output': ''}
        except Exception as error:
            # Not only gdb.error: a command can raise anything, and letting it
            # out would answer with a bare protocol failure that the console
            # cannot show.
            body = {'error': str(error)}

        # The console can resume the inferior - continue, next, finish, run -
        # and gdb returns only once it stopped again. Nobody else reports that,
        # so the views would keep showing the old location. 'continued' first:
        # a stop can only be reported from a running state.
        resumed = self.lastStopEvent is not None or self.inferiorExited
        if resumed:
            thread = gdb.selected_thread()
            self.sendEvent('continued',
                           {'threadId': thread.global_num if thread is not None else 0,
                            'allThreadsContinued': True})
        self.sendResponse(request, body=body)
        if resumed:
            self._reportInferiorState()

    def cmd_qtc_fetchRegisters(self, request):
        # Enumerate the selected frame's registers via gdb's Python API and
        # return name/value(hex)/size. The C++ side (handleFetchRegistersResponse)
        # feeds them into the RegisterHandler.
        registers = []
        try:
            frame = gdb.selected_frame()
        except gdb.error:
            self.sendResponse(request, body={'registers': registers})
            return

        try:
            descriptors = list(frame.architecture().registers())
        except Exception:
            descriptors = []

        for desc in descriptors:
            try:
                value = frame.read_register(desc)
            except Exception:
                continue
            size = 0
            try:
                size = int(value.type.sizeof)
            except Exception:
                pass
            try:
                text = '0x%x' % (int(value) & ((1 << (size * 8)) - 1) if size else int(value))
            except Exception:
                try:
                    text = str(value)
                except Exception:
                    text = ''
            registers.append({'name': desc.name, 'value': text, 'size': size})

        self.sendResponse(request, body={'registers': registers})

    def cmd_qtc_readMemory(self, request):
        # Read inferior memory and return it base64-encoded. The token echoes
        # back so the C++ side can route the data to the requesting MemoryAgent.
        args = request.get('arguments', {})
        token = args.get('token', 0)
        address = int(str(args.get('address', '0')), 0)
        length = int(args.get('length', 0))
        try:
            mem = gdb.selected_inferior().read_memory(address, length)
            data = base64.b64encode(bytes(mem)).decode('ascii')
        except Exception as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request, body={'token': token,
                                         'address': '0x%x' % address,
                                         'data': data})

    def cmd_qtc_writeMemory(self, request):
        args = request.get('arguments', {})
        address = int(str(args.get('address', '0')), 0)
        try:
            raw = base64.b64decode(args.get('data', ''))
            gdb.selected_inferior().write_memory(address, raw)
        except Exception as error:
            self.sendResponse(request, success=False, message=str(error))
            return
        self.sendResponse(request)

    def cmd_qtc_disassemble(self, request):
        # Disassemble the enclosing function (or a window) around the address
        # via gdb's Python API, returning address/bytes/asm per instruction.
        args = request.get('arguments', {})
        token = args.get('token', 0)
        address = int(str(args.get('address', '0')), 0)
        lines = []
        try:
            arch = gdb.selected_frame().architecture()
        except gdb.error:
            self.sendResponse(request, body={'token': token, 'lines': lines})
            return

        start, end, func = address, address + 64, ''
        try:
            block = gdb.block_for_pc(address)
            while block and not block.function:
                block = block.superblock
            if block and block.function:
                func = block.function.name
                start, end = int(block.start), int(block.end)
        except Exception:
            pass

        try:
            insns = arch.disassemble(start, end - 1)
        except Exception as error:
            self.sendResponse(request, success=False, message=str(error))
            return

        inferior = gdb.selected_inferior()
        for insn in insns:
            addr = int(insn['addr'])
            length = int(insn.get('length', 0))
            rawbytes = ''
            try:
                mem = inferior.read_memory(addr, length)
                rawbytes = ' '.join('%02x' % b for b in bytes(mem))
            except Exception:
                pass
            lines.append({'address': '0x%x' % addr,
                          'bytes': rawbytes,
                          'data': insn.get('asm', ''),
                          'function': func,
                          'offset': addr - start if func else 0})

        self.sendResponse(request, body={'token': token, 'lines': lines})

    def cmd_evaluate(self, request):
        arguments = request.get('arguments', {})
        expression = arguments.get('expression', '')
        try:
            value = gdb.parse_and_eval(expression)
            self.sendResponse(request, body={'result': str(value),
                                             'variablesReference': 0})
        except gdb.error as error:
            self.sendResponse(request, success=False, message=str(error))

    #######################################################################
    # Helpers
    #######################################################################

    def _selectThread(self, threadId):
        for thread in gdb.selected_inferior().threads():
            if thread.global_num == threadId:
                thread.switch()
                return
