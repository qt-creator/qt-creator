# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Drives the native combined (C++ + QML) debugging machinery through
# GDB without a Qt Creator frontend, mimicking the commands the GDB
# engine would send.
#
# Usage:
#     qt-cmake -S . -B build && cmake --build build
#     QMLMIX_EXECUTABLE=build/qmlmixtest gdb -batch -x qmlmix-gdb-test.py
#
# The Qt used to build the application needs the qmldbg_native and
# qmldbg_nativedebugger plugins.

import os
import sys

test_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.normpath(
    os.path.join(test_dir, '..', '..', '..', '..', 'share', 'qtcreator', 'debugger')))

import gdbbridge


def line_of(marker):
    # Resolves a 'MARKER: <name>' comment in Main.qml to its line number,
    # so the test does not hardcode line numbers of the test app.
    path = os.path.join(test_dir, 'Main.qml')
    with open(path) as qml:
        for number, text in enumerate(qml, 1):
            if ('MARKER: ' + marker) in text:
                return number
    raise RuntimeError('marker not found in Main.qml: ' + marker)


# First and second executable statement of the timer handler. The first
# is JS only, the second writes a property.
HANDLER_JS = line_of('handler-js')
HANDLER_WRITE = line_of('handler-write')

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

d = gdbbridge.theDumper
d.nativeMixed = 1  # This is a native combined debugging session.

gdb.execute('set confirm off')
gdb.execute('set breakpoint pending on')
try:
    gdb.execute('set debuginfod enabled off')
except gdb.error:
    pass
gdb.execute('file %s' % executable)
gdb.execute('set args -qmljsdebugger=native,services:NativeQmlDebugger')
gdb.execute('set environment QV4_FORCE_INTERPRETER 1')
gdb.execute('set environment QT_QPA_PLATFORM offscreen')

# Capture the asynchronous breakpoint resolution reports; they carry
# the service breakpoint ids needed for removal.
asyncReports = []
savedReportAsync = d.reportInterpreterAsync
d.reportInterpreterAsync = lambda resdict, asyncclass: asyncReports.append(dict(resdict))

def serviceId(token):
    for report in asyncReports:
        if report.get('token') == token:
            return report.get('number', -1)
    return -1


# Insertion before the program runs is expected to fail directly and to
# create one pending resolver breakpoint on qt_qmlDebugConnectorOpen()
# per interpreter breakpoint. All must get resolved on the single stop
# there. Breakpoint 3 sits in the nested square() function.
d.insertInterpreterBreakpoint({
    'file': os.path.join(test_dir, 'Main.qml'),
    'line': HANDLER_JS,
    'type': 'breakpoint',
    'token': 1,
})
d.insertInterpreterBreakpoint({
    'file': os.path.join(test_dir, 'Main.qml'),
    'line': HANDLER_WRITE,
    'type': 'breakpoint',
    'token': 2,
})
d.insertInterpreterBreakpoint({
    'file': os.path.join(test_dir, 'Main.qml'),
    'line': line_of('deep-js'),
    'type': 'breakpoint',
    'token': 3,
})
d.insertInterpreterBreakpoint({
    'file': os.path.join(test_dir, 'Main.qml'),
    'line': line_of('qml-to-cpp'),
    'type': 'breakpoint',
    'token': 4,
})
check('pending resolvers created', len(d.interpreterBreakpointResolvers) == 4)

gdb.execute('run')

check('all breakpoints were resolved',
      len([r for r in asyncReports
           if not r.get('pending') and r.get('number', -1) != -1]) == 4)

# Component.onCompleted runs first and calls compute(3), so the first
# stop is in the nested square() function.
mframes = d.extractInterpreterStack().get('frames', [])
mfuncs = [f.get('function', '') for f in mframes]
check('multi-frame JS stack: square <- compute <- onCompleted',
      len(mframes) >= 3
      and mfuncs[0] == 'square'
      and mfuncs[1] == 'compute'
      and mfuncs[2].find('onCompleted') >= 0)

# Remove the square breakpoint (it would hit thrice) and run to the
# statement that calls a C++ method.
d.removeInterpreterBreakpoint({'id': serviceId(3)})
gdb.execute('continue')
check('stopped at the QML statement calling C++',
      d.extractInterpreterStack().get('frames', [{}])[0].get('line')
      == line_of('qml-to-cpp'))
d.removeInterpreterBreakpoint({'id': serviceId(4)})


def hookAvailable():
    try:
        return gdb.lookup_global_symbol('qt_v4AboutToCallNativeMethodHook') is not None
    except Exception:
        return False


# Drive the production Step-Into. With the native call hook it descends
# into the C++ method; without it the step falls back to a step over the
# call and lands at the next JS statement.
d.executeStep({'token': 50})
if hookAvailable():
    check('step into from QML lands in the C++ method',
          gdb.newest_frame().name() == 'Backend::process')
    # The stack while in the C++ method splices in the QML caller chain.
    cppStackReports = []
    saved = d.reportResult
    d.reportResult = lambda result, args: cppStackReports.append(result)
    d.fetchStack({'limit': 40, 'nativemixed': 1, 'token': 52,
                  'allowinferiorcalls': 1})
    d.reportResult = saved
    cppStack = cppStackReports[0] if cppStackReports else ''
    check('C++ stack shows the QML caller',
          cppStack.find('function="compute"') >= 0
          and cppStack.find('language="js"') >= 0)
    # Step out of the C++ method back to the QML caller.
    d.executeNativeMixedStepOut({'token': 51})
    outFrames = d.extractInterpreterStack().get('frames', [{}])
    check('step out from C++ returns to the QML caller',
          gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable'
          and outFrames and outFrames[0].get('function') == 'compute')
else:
    check('step over the C++ call stays in QML',
          gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable')

# Resynchronize with the timer handler for the remaining checks.
gdb.execute('continue')

frame = gdb.newest_frame()
check('stopped at qt_qmlDebugMessageAvailable',
      frame.name() == 'qt_qmlDebugMessageAvailable')

stack = d.extractInterpreterStack()
frames = stack.get('frames', [])
check('interpreter stack has frames', bool(frames))

ctx = ''
if frames:
    top = frames[0]
    ctx = top.get('context', '')
    check('top frame is the onTriggered expression',
          top.get('language') == 'js'
          and top.get('line') == HANDLER_JS
          and top.get('function', '').find('onTriggered') >= 0)
    check('top frame has a context', bool(ctx))

if ctx:
    (ok, res) = d.tryFetchInterpreterVariables({
        'nativemixed': 1,
        'context': ctx,
        'expanded': {'local.this': 100, 'local.this.parent': 100},
    })
    check('interpreter variables fetched', ok)
    check('message is undefined before assignment',
          res.find('name="message"') >= 0
          and res.find('valueencoded="undefined"') >= 0)
    check('this expands to Timer properties',
          res.find('iname="local.this.interval"') >= 0)
    check('this.parent expands one level deeper',
          res.find('iname="local.this.parent.children"') >= 0)

# The full mixed stack report must de-emphasize the debugger machinery
# frames between the QML frame and the application code.
reports = []
savedReportResult = d.reportResult
d.reportResult = lambda result, args: reports.append(result)
d.fetchStack({'limit': 40, 'nativemixed': 1, 'token': 99,
              'allowinferiorcalls': 1})
d.reportResult = savedReportResult
stackFrames = (reports[0] if reports else '').split('frame={')[1:]

def frames_matching(needle):
    return [f for f in stackFrames if f.find(needle) >= 0]

check('machinery frames are marked',
      all(f.find('usable="0",machinery="1"') >= 0
          for f in frames_matching('qt_qmlDebug')
                 + frames_matching('NativeDebugger::')
                 + frames_matching('QV4::Moth::'))
      and len(frames_matching('machinery="1"')) >= 3)
check('QML frame is not marked',
      all(f.find('machinery="1"') < 0 for f in frames_matching('language="js"'))
      and len(frames_matching('language="js"')) >= 1)
check('application frames are not marked',
      len(frames_matching('QQmlTimer')) >= 1
      and all(f.find('machinery="1"') < 0 for f in frames_matching('QQmlTimer')))

# The second breakpoint, on the next line, must be hit as well.
gdb.execute('continue')
check('stopped at second breakpoint',
      gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable')
frames = d.extractInterpreterStack().get('frames', [])
check('second breakpoint is on the next line',
      bool(frames) and frames[0].get('line') == HANDLER_WRITE)

# Stepping: back at the first breakpoint, remove the second breakpoint,
# then a single step must stop on its line nevertheless.
gdb.execute('continue')
frames = d.extractInterpreterStack().get('frames', [])
check('first breakpoint hits again',
      bool(frames) and frames[0].get('line') == HANDLER_JS)

bp2 = serviceId(2)
check('second breakpoint has a service id', bp2 != -1)
res = d.removeInterpreterBreakpoint({'id': bp2})
check('breakpoint removal acknowledged', res.get('id') == bp2)

d.sendInterpreterRequest('stepin', {})
d.interpreterStepArmed = True  # As executeStep() would do.
gdb.execute('continue')
frames = d.extractInterpreterStack().get('frames', [])
check('step lands on the next line',
      gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable'
      and bool(frames) and frames[0].get('line') == HANDLER_WRITE)

# C++ to QML cross stepping: remove the remaining breakpoint, stop in
# C++ right before the timer fires the handler, arm and do a native
# step. The step must pass through the machinery and stop on the JS
# statement.
bp1 = serviceId(1)
res = d.removeInterpreterBreakpoint({'id': bp1})
check('first breakpoint removed', res.get('id') == bp1)

cppbp = gdb.Breakpoint('QQmlTimer::triggered')
gdb.execute('continue')
check('stopped in C++ before the handler',
      (gdb.newest_frame().name() or '').find('QQmlTimer::triggered') >= 0)
cppbp.delete()

d.armInterpreterStepIn({'token': 77})
gdb.execute('step')
frames = d.extractInterpreterStack().get('frames', [])
# The first statement boundary of the handler is attributed to the
# 'onTriggered:' line itself.
check('cross step from C++ lands on the JS statement',
      gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable'
      and bool(frames)
      and frames[0].get('line') in (HANDLER_JS - 1, HANDLER_JS)
      and frames[0].get('language') == 'js')

# Prototype for QML to C++ step-into: race the service's 'next JS
# statement' request against native breakpoints on the JS-to-C++
# transition choke points. Whichever fires first defines what 'into'
# means for the current statement. The stop handler disarms the
# service stepping when the native side wins.
def qmlStepInto():
    d.sendInterpreterRequest('stepin', {})
    d.interpreterStepArmed = True
    chokepoints = []
    for spec in ('QV4::QObjectMethod::callInternal',
                 'QV4::QObjectWrapper::setProperty'):
        try:
            chokepoints.append(gdb.Breakpoint(spec))
        except Exception:
            pass
    gdb.execute('continue')
    for bp in chokepoints:
        bp.delete()
    name = gdb.newest_frame().name() or ''
    if name == 'qt_qmlDebugMessageAvailable':
        frames = d.extractInterpreterStack().get('frames', [{}])
        return ('js', frames[0].get('line', -1))
    return ('cpp', name)

jsLines = []
cppLanding = None
for attempt in range(4):
    (kind, where) = qmlStepInto()
    if kind == 'cpp':
        cppLanding = where
        break
    jsLines.append(where)

check('step into walks the JS statements first',
      jsLines and all(HANDLER_JS - 1 <= line <= HANDLER_WRITE
                      for line in jsLines))
check('step into a statement with a C++ transition lands in C++',
      cppLanding is not None and cppLanding.find('setProperty') >= 0)
check('interpreter stepping is disarmed after the C++ landing',
      not d.interpreterStepArmed)

d.reportInterpreterAsync = savedReportAsync
gdb.execute('kill')

if failures:
    print('=== RESULT: FAIL (%s) ===' % ', '.join(failures))
    gdb.execute('quit 1')
print('=== RESULT: PASS ===')
