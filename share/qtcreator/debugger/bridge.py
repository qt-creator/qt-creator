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

import json
import os
import sys
import types

import gdb


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

        # Native qtc/ breakpoints: gdb.Breakpoint by its (stringified) number,
        # and the arguments each was created for.
        self.breakpointById = {}
        self.breakpointArgsById = {}

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
        data = json.dumps(obj).encode('utf-8')
        message = ('Content-Length: %d\r\n\r\n' % len(data)).encode('ascii') + data
        written = 0
        while written < len(message):
            written += os.write(1, message[written:])

    def _nextSeq(self):
        self.seq += 1
        return self.seq

    def sendResponse(self, request, body=None, success=True, message=None):
        response = {
            'type': 'response',
            'seq': self._nextSeq(),
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
            modelid = request.get('arguments', {}).get('modelid')
            if modelid is not None:
                body = {'modelid': modelid}
        if body is not None:
            response['body'] = body
        self._send(response)

    def sendEvent(self, event, body=None):
        message = {'type': 'event', 'seq': self._nextSeq(), 'event': event}
        if body is not None:
            message['body'] = body
        self._send(message)

    #######################################################################
    # Main loop
    #######################################################################

    def run(self):
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
        program = arguments.get('program', '')
        if program:
            gdb.execute('file "%s"' % program, to_string=True)
        programArgs = arguments.get('args', '')
        if programArgs:
            gdb.execute('set args %s' % programArgs, to_string=True)
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

    def cmd_pause(self, request):
        # TODO: not supported by the synchronous transport yet - while the
        # inferior runs we are blocked in gdb.execute() and not reading stdin.
        self.sendResponse(request, success=False,
                          message='pause is not supported yet')

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
    BP_WATCH_ADDRESS = 10
    BP_WATCH_EXPRESSION = 11

    def _createGdbBreakpoint(self, args):
        bptype = args.get('type', self.BP_BY_FILE_AND_LINE)
        temporary = bool(args.get('oneshot'))
        if bptype == self.BP_BY_FUNCTION:
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

        condition = args.get('condition', '')
        if condition:
            bp.condition = self.dumper.hexdecode(condition)
        ignore = args.get('ignorecount', 0)
        if ignore:
            bp.ignore_count = int(ignore)
        if not args.get('enabled', True):
            bp.enabled = False
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

    def _breakpointToMi(self, bp):
        result = {
            'number': bp.number,
            'enabled': 'y' if bp.enabled else 'n',
            'disp': 'del' if bp.temporary else 'keep',
            'type': 'breakpoint',
        }
        if bp.condition:
            result['cond'] = bp.condition
        try:
            locations = list(bp.locations)
        except AttributeError:
            # gdb.Breakpoint.locations arrived in gdb 13. Without it, resolve
            # the requested location instead of reporting every breakpoint as
            # pending.
            locations = self._decodedLocations(bp)
        except Exception:
            locations = []
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
        try:
            bp = self._createGdbBreakpoint(args)
            self.breakpointById[str(bp.number)] = bp
            self.breakpointArgsById[str(bp.number)] = args
            body['bkpt'] = self._breakpointToMi(bp)
        except Exception as error:
            warn('insertBreakpoint failed: %s' % error)
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
            try:
                bp.delete()
            except (gdb.error, RuntimeError):
                pass
            self.breakpointById.pop(key, None)
            self.breakpointArgsById.pop(key, None)
            # The insert reply carries the new breakpoint in the same shape,
            # and the C++ side feeds both through the same handler.
            self.cmd_qtc_insertBreakpoint(request)
            return

        bp.condition = self.dumper.hexdecode(args.get('condition', '')) \
            if args.get('condition') else ''
        bp.enabled = bool(args.get('enabled', True))
        bp.ignore_count = int(args.get('ignorecount', 0))
        self.breakpointArgsById[key] = args
        self.sendResponse(request, body={'modelid': args.get('modelid'),
                                         'bkpt': self._breakpointToMi(bp)})

    @staticmethod
    def _locationOf(args):
        return (args.get('type'), args.get('file'), args.get('line'),
                args.get('function'), args.get('address'), args.get('expression'))

    def cmd_qtc_removeBreakpoint(self, request):
        args = request.get('arguments', {})
        key = str(args.get('id'))
        self.breakpointArgsById.pop(key, None)
        bp = self.breakpointById.pop(key, None)
        if bp is not None:
            try:
                bp.delete()
            except gdb.error:
                pass
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
        self.variableReferences = {}
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
