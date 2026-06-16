# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Proof of concept for 'passive' mixed C++/QML stacks: extract the QML
# stack purely by reading inferior memory when stopped in a plain C++
# debug session. No qmldebug services, no inferior calls, and no
# -qmljsdebugger argument are involved. This works on anything the
# debugger can read memory from, including core dumps in principle.
#
# Requires debug info for libQt6Qml. The decoding mirrors private QV4
# structures (CppStackFrame, Function, CompiledData) and needs to be
# kept in sync with qtdeclarative.
#
# Usage:
#     qt-cmake -S . -B build && cmake --build build
#     QMLMIX_EXECUTABLE=build/qmlmixtest gdb -batch -x qmlmix-passive-stack.py

import os
import struct as pystruct

import gdb
from gdb.FrameDecorator import FrameDecorator


def read_mem(addr, size):
    return bytes(gdb.selected_inferior().read_memory(addr, size))


def le_int(value):
    # quint32_le and friends are QSpecialInteger wrappers around 'val'.
    try:
        return int(value)
    except gdb.error:
        return int(value['val'])


def string_at(unit, idx):
    # Mirrors CompiledData::Unit::stringAtInternal()
    base = int(unit)
    offset_table = base + le_int(unit['offsetToStringTable'])
    str_offset = pystruct.unpack('<I', read_mem(offset_table + 4 * idx, 4))[0]
    size = pystruct.unpack('<i', read_mem(base + str_offset, 4))[0]
    if size <= 0:
        return ''
    return read_mem(base + str_offset + 4, 2 * size).decode('utf-16-le')


def read_qstring(qstr):
    size = int(qstr['d']['size'])
    if size <= 0:
        return ''
    return read_mem(int(qstr['d']['ptr']), 2 * size).decode('utf-16-le')


def line_for_frame(compiled_function, instruction_pointer):
    # Mirrors CppStackFrame::lineNumber(). The line number table sits
    # behind the locals table; its offset is computed, not stored.
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


def decode_cpp_stack_frame(frame):
    # frame: gdb.Value of type QV4::CppStackFrame*
    fn = frame['v4Function']
    if int(fn) == 0:
        return None
    # Function::compilationUnit is a WriteBarrier::HeapObjectWrapper
    # holding the pointer as an opaque enum.
    ecu_addr = int(fn['compilationUnit']['wrapped'].cast(
        gdb.lookup_type('unsigned long')))
    ecu_type = gdb.lookup_type('QV4::ExecutableCompilationUnit').pointer()
    ecu = gdb.Value(ecu_addr).cast(ecu_type)
    cu = ecu['m_compilationUnit']['o']
    unit = cu['data']
    cf = fn['compiledFunction']
    name = string_at(unit, le_int(cf['nameIndex']))
    # For ahead-of-time compiled units the file name is only known at
    # load time and stored in the runtime CompilationUnit.
    source = read_qstring(cu['m_fileName'])
    if not source:
        source = string_at(unit, le_int(unit['sourceFileIndex']))
    line = line_for_frame(cf, int(frame['instructionPointer']))
    kind = 'js' if int(frame['kind']) == 0 else 'meta'
    return {'function': name, 'file': source, 'line': line, 'kind': kind}


def extract_js_stack(engine):
    # engine: gdb.Value of type QV4::ExecutionEngine*
    result = []
    frame = engine['currentStackFrame']
    while int(frame) != 0:
        decoded = decode_cpp_stack_frame(frame)
        if decoded is not None:
            result.append(decoded)
        frame = frame['parent']
    return result


def find_engine():
    frame = gdb.newest_frame()
    while frame is not None:
        name = frame.name()
        if name is not None and name.startswith('QV4::Moth::VME::exec'):
            return frame.read_var('engine')
        frame = frame.older()
    return None


# Frame filter: make 'bt' show the JS source position on interpreter frames.
class QmlFrameDecorator(FrameDecorator):
    def __init__(self, fobj, decoded):
        super(QmlFrameDecorator, self).__init__(fobj)
        self.decoded = decoded

    def function(self):
        return '[QML] %s' % (self.decoded['function'] or '<anonymous>')

    def filename(self):
        return self.decoded['file']

    def line(self):
        return self.decoded['line']


class QmlFrameFilter():
    def __init__(self):
        self.name = 'qml-frames'
        self.priority = 100
        self.enabled = True
        gdb.frame_filters[self.name] = self

    def filter(self, frame_iter):
        for fobj in frame_iter:
            frame = fobj.inferior_frame()
            name = frame.name()
            if name is not None and name.startswith('QV4::Moth::VME::exec'):
                try:
                    decoded = decode_cpp_stack_frame(frame.read_var('frame'))
                    if decoded is not None:
                        yield QmlFrameDecorator(fobj, decoded)
                        continue
                except Exception as error:
                    print('QML frame decoration failed: %s' % error)
            yield fobj


test_dir = os.path.dirname(os.path.abspath(__file__))


def line_of(marker):
    # Resolves a 'MARKER: <name>' comment in Main.qml to its line number.
    with open(os.path.join(test_dir, 'Main.qml')) as qml:
        for number, text in enumerate(qml, 1):
            if ('MARKER: ' + marker) in text:
                return number
    raise RuntimeError('marker not found in Main.qml: ' + marker)


failures = []

def check(name, cond):
    print('=== %s: %s ===' % ('PASS' if cond else 'FAIL', name))
    if not cond:
        failures.append(name)

executable = os.environ.get('QMLMIX_EXECUTABLE',
                            os.path.join(test_dir, 'build', 'qmlmixtest'))
if not os.path.isfile(executable):
    print('=== FAIL: executable not found: %s ===' % executable)
    print('=== Set QMLMIX_EXECUTABLE or build into ./build first ===')
    gdb.execute('quit 1')

gdb.execute('set confirm off')
gdb.execute('set breakpoint pending on')
try:
    gdb.execute('set debuginfod enabled off')
except gdb.error:
    pass
gdb.execute('file %s' % executable)
# Plain run, no -qmljsdebugger. QV4_FORCE_INTERPRETER prevents
# qmlcachegen's ahead-of-time compiled code from bypassing the
# interpreter; the native mixed setup sets it as well.
gdb.execute('set environment QV4_FORCE_INTERPRETER 1')
gdb.execute('set environment QT_QPA_PLATFORM offscreen')
gdb.execute('break QV4::QObjectWrapper::setProperty')
gdb.execute('run')

check('stopped at setProperty',
      gdb.newest_frame().name() == 'QV4::QObjectWrapper::setProperty')

# Property writes from startup code may hit first; continue until the
# write from the timer handler comes around.
engine = None
stack = []
for attempt in range(10):
    engine = find_engine()
    if engine is not None:
        stack = extract_js_stack(engine)
        if stack and stack[0]['function'].find('onTriggered') >= 0:
            break
    gdb.execute('continue')

check('found engine in interpreter frame', engine is not None)

print('=== PASSIVELY EXTRACTED QML STACK ===')
for f in stack:
    print('  %s [%s] at %s:%s' % (f['function'], f['kind'], f['file'], f['line']))
check('stack has frames', bool(stack))
if stack:
    top = stack[0]
    check('top frame is the executing QML statement',
          top['function'].find('onTriggered') >= 0
          and top['file'].endswith('Main.qml')
          and top['line'] == line_of('handler-write')
          and top['kind'] == 'js')

QmlFrameFilter()
bt = gdb.execute('bt', to_string=True)
check('frame filter decorates bt', bt.find('[QML] expression for onTriggered') >= 0
      and bt.find('Main.qml:%d' % line_of('handler-write')) >= 0)

gdb.execute('kill')

if failures:
    print('=== RESULT: FAIL (%s) ===' % ', '.join(failures))
    gdb.execute('quit 1')
print('=== RESULT: PASS ===')
