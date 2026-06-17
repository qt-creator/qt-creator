# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Orchestration driver for tst_nativemixed. Runs a native combined
# (C++ + QML in one engine) debugging session against the qmlmix fixture
# and emits one machine-parseable line per check:
#
#     nmcheck={name="<name>",hook="0|1",ok="0|1"}
#
# followed by nmdone={engine="gdb|lldb"}. "hook" marks checks that need
# the qtdeclarative qt_v4AboutToCallNativeMethodHook (Qt >= 6.12); the C++
# side gates those by Qt version. The actual stepping/stack logic mirrors
# the validated manual harnesses (qmlmix-gdb-test.py and the lldb ones).
#
# Invoked inside the debugger's own Python. Configuration via environment:
#   QTC_NM_EXE     - absolute path to the built fixture executable
#   QTC_NM_QMLDIR  - directory containing Main.qml (for MARKER lookup)

import os
import sys

EXE = os.environ['QTC_NM_EXE']
QMLDIR = os.environ['QTC_NM_QMLDIR']
QML = os.path.join(QMLDIR, 'Main.qml')


def emit(name, ok, hook=False):
    sys.__stdout__.write('nmcheck={name="%s",hook="%d",ok="%d"}\n'
                         % (name, 1 if hook else 0, 1 if ok else 0))
    sys.__stdout__.flush()


def line_of(marker):
    for n, text in enumerate(open(QML), 1):
        if ('MARKER: ' + marker) in text:
            return n
    return -1


# --------------------------------------------------------------------------
# GDB
# --------------------------------------------------------------------------
def run_gdb():
    import gdb
    import gdbbridge

    d = gdbbridge.theDumper
    d.nativeMixed = 1
    gdb.execute('set confirm off')
    gdb.execute('set breakpoint pending on')
    try:
        gdb.execute('set debuginfod enabled off')
    except gdb.error:
        pass
    gdb.execute('file %s' % EXE)
    gdb.execute('set args -qmljsdebugger=native,services:NativeQmlDebugger')
    gdb.execute('set environment QV4_FORCE_INTERPRETER 1')
    gdb.execute('set environment QT_QPA_PLATFORM offscreen')

    d.insertInterpreterBreakpoint({'file': QML, 'line': line_of('qml-to-cpp'),
                                   'type': 'breakpoint', 'token': 1})
    gdb.execute('run')

    frames = d.extractInterpreterStack().get('frames', [{}])
    emit('stopped at the QML statement calling C++',
         frames and frames[0].get('function') == 'compute'
         and frames[0].get('line') == line_of('qml-to-cpp'))

    d.executeStep({'token': 50})
    landed = gdb.newest_frame().name()
    emit('step into from QML lands in the C++ method',
         landed == 'Backend::process', hook=True)

    reports = []
    saved = d.reportResult
    d.reportResult = lambda result, args: reports.append(result)
    d.fetchStack({'limit': 40, 'nativemixed': 1, 'token': 52,
                  'allowinferiorcalls': 1})
    d.reportResult = saved
    stack = reports[0] if reports else ''
    emit('C++ stack splices in the QML caller',
         stack.find('function="compute"') >= 0
         and stack.find('language="js"') >= 0, hook=True)
    emit('C++ stack dims the metacall machinery',
         stack.find('qt_static_metacall') >= 0
         and stack.find('machinery="1"') >= 0, hook=True)

    d.executeNativeMixedStepOut({'token': 51})
    out = d.extractInterpreterStack().get('frames', [{}])
    emit('step out from C++ returns to the QML caller',
         gdb.newest_frame().name() == 'qt_qmlDebugMessageAvailable'
         and out and out[0].get('function') == 'compute', hook=True)

    sys.__stdout__.write('nmdone={engine="gdb"}\n')
    sys.__stdout__.flush()
    gdb.execute('quit')


# --------------------------------------------------------------------------
# LLDB
# --------------------------------------------------------------------------
def run_lldb():
    import time
    import lldb
    import lldbbridge

    d = lldbbridge.Dumper()
    d.debugger.SetAsync(True)
    d.setVariableFetchingOptions({'nativemixed': 1, 'fancy': 1,
                                  'allowinferiorcalls': 1, 'passexceptions': 1,
                                  'qobjectnames': 0})
    d.nativeMixed = 1
    d.isShuttingDown_ = False
    d.isInterrupting_ = False
    d.firstStop_ = False
    d.useTerminal_ = False
    d.platform_ = ''
    d.eventState = lldb.eStateInvalid
    err = lldb.SBError()
    d.target = d.debugger.CreateTarget(EXE, None, '', True, err)
    listener = d.debugger.GetListener()
    bc = d.target.GetBroadcaster()
    bc.AddListener(listener, lldb.SBProcess.eBroadcastBitStateChanged)
    listener.StartListeningForEvents(bc, lldb.SBProcess.eBroadcastBitStateChanged)
    d.target.BreakpointCreateByName('qt_qmlDebugConnectorOpen')
    d.interpreterEventBreakpoint = \
        d.target.BreakpointCreateByName('qt_qmlDebugMessageAvailable')

    def resolver():
        d.parseAndEvaluate('qt_qmlDebugEnableService("NativeQmlDebugger")')
        d.sendInterpreterRequest('setbreakpoint',
                                 {'file': QML, 'line': line_of('qml-to-cpp'),
                                  'condition': ''})
    d.interpreterBreakpointResolvers = [resolver]

    launch = lldb.SBLaunchInfo(['-qmljsdebugger=native,services:NativeQmlDebugger'])
    launch.SetWorkingDirectory(os.getcwd())
    launch.SetEnvironmentEntries(
        ['QV4_FORCE_INTERPRETER=1', 'QT_QPA_PLATFORM=offscreen'], True)
    launch.SetListener(listener)
    d.process = d.target.Launch(launch, err)

    ev = lldb.SBEvent()
    phase = 'run-to-qml'
    deadline = time.time() + 120
    while time.time() < deadline:
        if not listener.WaitForEvent(1, ev):
            if d.process.GetState() == lldb.eStateExited:
                break
            continue
        d.handleEvent(ev)
        if d.process.GetState() != lldb.eStateStopped:
            continue
        thread = d.firstStoppedThread() or d.process.GetSelectedThread()
        fn = thread.GetFrameAtIndex(0).GetFunctionName() or ''
        if phase == 'run-to-qml' and 'qt_qmlDebugMessageAvailable' in fn:
            frames = d.extractInterpreterStack().get('frames', [{}])
            emit('stopped at the QML statement calling C++',
                 frames and frames[0].get('function') == 'compute'
                 and frames[0].get('line') == line_of('qml-to-cpp'))
            # All modules are loaded now, so the hook symbol resolves if
            # the Qt carries it (Qt >= 6.12).
            if not d.nativeCallHookAvailable():
                break
            phase = 'descending'
            d.executeStep({'token': 50})
        elif phase == 'descending' and fn == 'Backend::process(int)':
            emit('step into from QML lands in the C++ method', True, hook=True)
            reports = []
            saved = d.reportResult
            d.reportResult = lambda result, args: reports.append(result)
            d.fetchStack({'nativemixed': 1, 'stacklimit': -1, 'token': 52,
                          'extraqml': 0})
            d.reportResult = saved
            stack = reports[0] if reports else ''
            emit('C++ stack splices in the QML caller',
                 'function="compute"' in stack and 'language="js"' in stack,
                 hook=True)
            emit('C++ stack dims the metacall machinery',
                 'qt_static_metacall' in stack and 'machinery="1"' in stack,
                 hook=True)
            emit('atNativeToQmlBoundary in the C++ method',
                 d.atNativeToQmlBoundary(), hook=True)
            phase = 'stepping-out'
            d.executeStepOut({'token': 51})
        elif phase == 'stepping-out' and 'qt_qmlDebugMessageAvailable' in fn:
            frames = d.extractInterpreterStack().get('frames', [{}])
            emit('step out from C++ returns to the QML caller',
                 frames and frames[0].get('function') == 'compute', hook=True)
            break

    sys.__stdout__.write('nmdone={engine="lldb"}\n')
    sys.__stdout__.flush()
    if d.process:
        d.process.Kill()


if 'gdb' in sys.modules or os.environ.get('QTC_NM_ENGINE') == 'gdb':
    run_gdb()
else:
    run_lldb()
