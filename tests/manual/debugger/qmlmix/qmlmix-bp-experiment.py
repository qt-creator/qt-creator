# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Breakpoint cost experiment for passive QML debugging.
#
# Measures the wall time of a 20000 iteration QML loop (delimited by
# two property writes serving as markers) under different breakpoint
# strategies:
#
#   baseline       debug instructions active, nothing else
#   gdbtrap_count  gdb python breakpoint on debug_slowPath, counting only
#   gdbtrap_decode ... plus passive file/line decode and compare per hit
#   qt_v4_miss     inferior-side bp list via qt_v4DebuggerHook, no match
#   qt_v4_hit      qt_v4 breakpoint on the loop body line, expect a stop
#
# Usage:
#     EXP_MODE=<mode> QMLMIX_EXECUTABLE=build/qmlmixtest \
#         gdb -batch -x qmlmix-bp-experiment.py

import os
import struct as pystruct
import time

import gdb

test_dir = os.path.dirname(os.path.abspath(__file__))
EXECUTABLE = os.environ.get('QMLMIX_EXECUTABLE',
                            os.path.join(test_dir, 'build', 'qmlmixtest'))
ENGINE_NAME = 'qrc:/qt/qml/MixTest/Main.qml'


def line_of(marker):
    # Resolves a 'MARKER: <name>' comment in Main.qml to its line number.
    with open(os.path.join(test_dir, 'Main.qml')) as qml:
        for number, text in enumerate(qml, 1):
            if ('MARKER: ' + marker) in text:
                return number
    raise RuntimeError('marker not found in Main.qml: ' + marker)


LOOP_BODY_LINE = line_of('hot-loop-body')

mode = os.environ.get('EXP_MODE', 'baseline')


def read_mem(addr, size):
    return bytes(gdb.selected_inferior().read_memory(addr, size))


def le_int(value):
    try:
        return int(value)
    except gdb.error:
        return int(value['val'])


def line_for_frame(compiled_function, instruction_pointer):
    if instruction_pointer <= 0:
        return -1
    base = int(compiled_function)
    table = base + le_int(compiled_function['localsOffset']) \
                 + 4 * le_int(compiled_function['nLocals'])
    count = le_int(compiled_function['nLineAndStatementNumbers'])
    line = -1
    for i in range(count):
        (code_offset, entry_line, _) = pystruct.unpack('<IiI',
                                                       read_mem(table + 12 * i, 12))
        if code_offset >= instruction_pointer:
            break
        line = entry_line
    return line


def read_qstring(qstr):
    size = int(qstr['d']['size'])
    if size <= 0:
        return ''
    return read_mem(int(qstr['d']['ptr']), 2 * size).decode('utf-16-le')


def decode_position(frame):
    # frame: gdb.Value of QV4::CppStackFrame*
    fn = frame['v4Function']
    if int(fn) == 0:
        return ('', -1)
    ecu_addr = int(fn['compilationUnit']['wrapped'].cast(
        gdb.lookup_type('unsigned long')))
    ecu = gdb.Value(ecu_addr).cast(
        gdb.lookup_type('QV4::ExecutableCompilationUnit').pointer())
    source = read_qstring(ecu['m_compilationUnit']['o']['m_fileName'])
    line = line_for_frame(fn['compiledFunction'], int(frame['instructionPointer']))
    return (source, line)


trap_count = [0]

class CountingTrap(gdb.Breakpoint):
    def stop(self):
        trap_count[0] += 1
        return False


lines_seen = set()

class DecodingTrap(gdb.Breakpoint):
    def stop(self):
        trap_count[0] += 1
        try:
            engine = gdb.newest_frame().read_var('engine')
            (source, line) = decode_position(engine['currentStackFrame'])
            lines_seen.add((source.rsplit('/', 1)[-1], line))
            if line == 9999 and source.endswith('Main.qml'):
                return True
        except Exception as error:
            print('DECODE FAILED: %s' % error)
        return False


def insert_qt_v4_breakpoint(line):
    json = ('{"command":"insertBreakpoint","version":"1","lineNumber":"%d",'
            '"engineName":"%s","fullName":"%s","condition":""}'
            % (line, ENGINE_NAME, ENGINE_NAME))
    expr = 'qt_v4DebuggerHook("%s")' % json.replace('"', '\\"')
    res = gdb.parse_and_eval(expr)
    print('qt_v4DebuggerHook returned: %s' % res)


gdb.execute('set confirm off')
gdb.execute('set breakpoint pending on')
try:
    gdb.execute('set debuginfod enabled off')
except gdb.error:
    pass
gdb.execute('file %s' % EXECUTABLE)
gdb.execute('set args -qmljsdebugger=native,services:NativeQmlDebugger')
gdb.execute('set environment QV4_FORCE_INTERPRETER 1')
gdb.execute('set environment QT_QPA_PLATFORM offscreen')

# The NativeQmlDebugger service only installs the per-engine debugger
# (which makes the compiler emit Debug instructions) when enabled. Do
# that before any engine exists.
hook_bp = gdb.Breakpoint('qt_qmlDebugConnectorOpen')
gdb.execute('run')
gdb.parse_and_eval('qt_qmlDebugEnableService("NativeQmlDebugger")')
hook_bp.delete()

marker = gdb.Breakpoint('QV4::QObjectWrapper::setProperty')

gdb.execute('continue')  # Stops at the hotStart write, before the loop.

if mode == 'gdbtrap_count':
    CountingTrap('debug_slowPath')
elif mode == 'gdbtrap_decode':
    DecodingTrap('debug_slowPath')
elif mode == 'qt_v4_miss':
    insert_qt_v4_breakpoint(9999)
elif mode == 'qt_v4_hit':
    insert_qt_v4_breakpoint(LOOP_BODY_LINE)
    gdb.Breakpoint('qt_v4TriggeredBreakpointHook')

t0 = time.monotonic()
gdb.execute('continue')  # Expect: hotResult write (or qt_v4 hook hit).
t1 = time.monotonic()

stopped_at = gdb.newest_frame().name()

if mode == 'qt_v4_hit':
    print('STOPPED AT: %s' % stopped_at)
    if stopped_at == 'qt_v4TriggeredBreakpointHook':
        # Find the interpreter frame and decode the position passively.
        frame = gdb.newest_frame()
        while frame is not None and \
                not (frame.name() or '').startswith('QV4::Moth::VME::exec'):
            frame = frame.older()
        if frame is not None:
            engine = frame.read_var('engine')
            (source, line) = decode_position(engine['currentStackFrame'])
            print('QT_V4 BREAK AT: %s:%s' % (source, line))
    print('EXPRESULT mode=%s functional=%s'
          % (mode, stopped_at == 'qt_v4TriggeredBreakpointHook'))
else:
    print('EXPRESULT mode=%s elapsed=%.3fs traps=%d stop=%s'
          % (mode, t1 - t0, trap_count[0], stopped_at))
    if lines_seen:
        print('LINES SEEN: %s' % sorted(lines_seen))

gdb.execute('kill')
