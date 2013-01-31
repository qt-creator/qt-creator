/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#define QT_NO_CAST_FROM_ASCII

#include "lldbengineguest.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "breakpoint.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "threadshandler.h"

#include <utils/qtcassert.h>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QThread>
#include <QMutexLocker>

#include <lldb/API/LLDB.h>

#define DEBUG_FUNC_ENTER \
    showMessage(QString(QLatin1String("LLDB guest engine: %1 ")) \
    .arg(QLatin1String(Q_FUNC_INFO))); \
    qDebug("%s", Q_FUNC_INFO)

#define SYNC_INFERIOR_OR(x)  if (m_running) { x; }


namespace Debugger {
namespace Internal {

void LldbEventListener::listen(lldb::SBListener *listener)
{
    while (true) {
        lldb::SBEvent event;
        if (listener->WaitForEvent(1000, event))
            emit lldbEvent(&event);
    }
}

LldbEngineGuest::LldbEngineGuest()
    : IPCEngineGuest()
    , m_running (false)
    , m_worker  (new LldbEventListener)
    , m_lldb    (new lldb::SBDebugger)
    , m_target  (new lldb::SBTarget)
    , m_process (new lldb::SBProcess)
    , m_listener(new lldb::SBListener("bla"))
    , m_relistFrames (false)
#if defined(HAVE_LLDB_PRIVATE)
    , py (new PythonLLDBToGdbMiHack)
#endif
{
    qRegisterMetaType<lldb::SBListener *>("lldb::SBListener *");
    qRegisterMetaType<lldb::SBEvent *>("lldb::SBEvent *");

    m_worker->moveToThread(&m_wThread);
    connect(m_worker, SIGNAL(lldbEvent(lldb::SBEvent*)), this,
            SLOT(lldbEvent(lldb::SBEvent*)), Qt::BlockingQueuedConnection);
    m_wThread.start();
    setObjectName(QLatin1String("LLDBEngineGuest"));
}

LldbEngineGuest::~LldbEngineGuest()
{
    delete m_lldb;
    delete m_target;
    delete m_process;
    delete m_listener;
}


void LldbEngineGuest::nuke()
{
    ::exit(4);
}

void LldbEngineGuest::setupEngine()
{
    DEBUG_FUNC_ENTER;

    lldb::SBDebugger::Initialize();

    *m_lldb = lldb::SBDebugger::Create();
    m_lldb->Initialize();
    if (m_lldb->IsValid())
        notifyEngineSetupOk();
    else
        notifyEngineSetupFailed();

}

void LldbEngineGuest::setupInferior(const QString &executable,
        const QStringList &args, const QStringList &env)
{
    DEBUG_FUNC_ENTER;

    foreach (const QString &s, args) {
        m_arguments.append(s.toLocal8Bit());
    }
    foreach (const QString &s, env) {
        m_environment.append(s.toLocal8Bit());
    }

    qDebug("creating target for %s", executable.toLocal8Bit().data());
    showStatusMessage(QLatin1String("starting ") + executable);
    *m_target = m_lldb->CreateTarget(executable.toLocal8Bit().data());
    if (!m_target->IsValid()) {
        notifyInferiorSetupFailed();
        return;
    }
    DEBUG_FUNC_ENTER;

    const char **argp = new const char *[m_arguments.count() + 1];
    argp[m_arguments.count()] = 0;
    for (int i = 0; i < m_arguments.count(); i++) {
        argp[i] = m_arguments[i].data();
    }

    const char **envp = new const char *[m_environment.count() + 1];
    envp[m_environment.count()] = 0;
    for (int i = 0; i < m_environment.count(); i++) {
        envp[i] = m_environment[i].data();
    }
    lldb::SBError err;
    *m_process = m_target->Launch(argp, envp, NULL, NULL, true, err);

    if (!err.Success()) {
        showMessage(QString::fromLocal8Bit(err.GetCString()));
        qDebug() << err.GetCString();
        notifyInferiorSetupFailed();
    }

    /*
     * note, the actual string ptrs are still valid. They are in m_environment.
     * They probably leak. Considered the marvelous API, there is not much we can do
     */
    delete [] envp;

    if (!m_process->IsValid())
        notifyEngineRunFailed();
    QTC_ASSERT(m_listener->IsValid(), qDebug() << false);
    m_listener->StartListeningForEvents(m_process->GetBroadcaster(), UINT32_MAX);
    QMetaObject::invokeMethod(m_worker, "listen", Qt::QueuedConnection,
            Q_ARG(lldb::SBListener *, m_listener));
    notifyInferiorSetupOk();
}

void LldbEngineGuest::runEngine()
{
    DEBUG_FUNC_ENTER;
    m_process->Continue();
}

void LldbEngineGuest::shutdownInferior()
{
    DEBUG_FUNC_ENTER;
    m_process->Kill();
}

void LldbEngineGuest::shutdownEngine()
{
    DEBUG_FUNC_ENTER;
    m_currentFrame = lldb::SBFrame();
    m_currentThread = lldb::SBThread();
    m_breakpoints.clear();
    m_localesCache.clear();

    /*
     * this leaks. However, Terminate is broken and lldb leaks anyway
     * We should kill the engine guest process
     */

    *m_lldb = lldb::SBDebugger();
    // leakd.Terminate();
    notifyEngineShutdownOk();
}

void LldbEngineGuest::detachDebugger()
{
    DEBUG_FUNC_ENTER;
}

void LldbEngineGuest::executeStep()
{
    DEBUG_FUNC_ENTER;

    if (!m_currentThread.IsValid())
        return;
    m_currentThread.StepInto();
}

void LldbEngineGuest::executeStepOut()
{
    DEBUG_FUNC_ENTER;

    if (!m_currentThread.IsValid())
        return;
    m_currentThread.StepOut();
}

void LldbEngineGuest::executeNext()
{
    DEBUG_FUNC_ENTER;

    if (!m_currentThread.IsValid())
        return;
    m_currentThread.StepOver();
}

void LldbEngineGuest::executeStepI()
{
    DEBUG_FUNC_ENTER;

    if (!m_currentThread.IsValid())
        return;
    m_currentThread.StepInstruction(false);
}

void LldbEngineGuest::executeNextI()
{
    DEBUG_FUNC_ENTER;

    if (!m_currentThread.IsValid())
        return;
    m_currentThread.StepInstruction(true);
}

void LldbEngineGuest::continueInferior()
{
    DEBUG_FUNC_ENTER;

    notifyInferiorRunRequested();
    m_process->Continue();
    showStatusMessage(QLatin1String("resuming inferior"));
}
void LldbEngineGuest::interruptInferior()
{
    DEBUG_FUNC_ENTER;

    m_process->Stop();
    notifyInferiorStopOk();
    m_relistFrames = true;
    updateThreads();
}

void LldbEngineGuest::executeRunToLine(const ContextData &data);
{
    DEBUG_FUNC_ENTER;

    // TODO
    Q_UNUSED(data);
}

void LldbEngineGuest::executeRunToFunction(const QString &functionName)
{
    DEBUG_FUNC_ENTER;

    // TODO
    Q_UNUSED(functionName);
}
void LldbEngineGuest::executeJumpToLine(const ContextData &data);
{
    DEBUG_FUNC_ENTER;

    // TODO
    Q_UNUSED(data);
}

void LldbEngineGuest::activateFrame(qint64 token)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(showMessage(QLatin1String(
        "activateFrame called while inferior running")); return);

    currentFrameChanged(token);
    m_localesCache.clear();

    lldb::SBFrame fr = m_currentThread.GetFrameAtIndex(token);
    m_currentFrame = fr;
    lldb::SBSymbolContext context = fr.GetSymbolContext(lldb::eSymbolContextEverything);
    lldb::SBValueList values = fr.GetVariables(true, true, false, true);
    QList<WatchData> wd;
    QByteArray iname = "local";
    for (uint i = 0; i < values.GetSize(); i++) {
        lldb::SBValue v = values.GetValueAtIndex(i);
        if (!v.IsInScope(fr))
            continue;
        getWatchDataR(v, 1, iname, wd);
    }
    updateWatchData(true, wd);
}

void LldbEngineGuest::requestUpdateWatchData(const Internal::WatchData &data,
        const Internal::WatchUpdateFlags &)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(return);

    lldb::SBValue v = m_localesCache.value(QString::fromUtf8(data.iname));
    QList<WatchData> wd;
    for (uint j = 0; j < v.GetNumChildren(); j++) {
        lldb::SBValue vv = v.GetChildAtIndex(j);
        getWatchDataR(vv, 1, data.iname, wd);
    }
    updateWatchData(false, wd);
}

void LldbEngineGuest::getWatchDataR(lldb::SBValue v, int level,
        const QByteArray &p_iname, QList<WatchData> &wd)
{
    QByteArray iname = p_iname + '.' + QByteArray(v.GetName());
    m_localesCache.insert(QString::fromLocal8Bit(iname), v);

#if defined(HAVE_LLDB_PRIVATE)
    wd += py->expand(p_iname, v, m_currentFrame, *m_process);
#else
    WatchData d;
    d.name = QString::fromLocal8Bit(v.GetName());
    d.iname = iname;
    d.type = QByteArray(v.GetTypeName()).trimmed();
    d.value = (QString::fromLocal8Bit(v.GetValue(m_currentFrame)));
    d.hasChildren = v.GetNumChildren();
    d.state = WatchData::State(0);
    wd.append(d);
#endif

    if (--level > 0) {
        for (uint j = 0; j < v.GetNumChildren(); j++) {
            lldb::SBValue vv = v.GetChildAtIndex(j);
            getWatchDataR(vv, level, iname, wd);
        }
    }
}

void LldbEngineGuest::disassemble(quint64 pc)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(return);

    if (!m_currentThread.IsValid())
        return;
    for (uint j = 0; j < m_currentThread.GetNumFrames(); j++) {
        lldb::SBFrame fr = m_currentThread.GetFrameAtIndex(j);
        if (pc == fr.GetPCAddress().GetLoadAddress(*m_target)) {
            QString linesStr = QString::fromLocal8Bit(fr.Disassemble());
            DisassemblerLines lines;
            foreach (const QString &lineStr, linesStr.split(QLatin1Char('\n'))) {
                lines.appendLine(DisassemblerLine(lineStr));
            }
            disassembled(pc, lines);
        }
    }
}
void LldbEngineGuest::fetchFrameSource(qint64 frame)
{
    QFile f(m_frame_to_file.value(frame));
    f.open(QFile::ReadOnly);
    frameSourceFetched(frame, QFileInfo(m_frame_to_file.value(frame)).fileName()
            , QString::fromLocal8Bit(f.readAll()));
}

void LldbEngineGuest::addBreakpoint(BreakpointId id,
        const Internal::BreakpointParameters &bp_)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(notifyAddBreakpointFailed(id); return);

    Internal::BreakpointParameters bp(bp_);

    lldb::SBBreakpoint llbp = m_target->BreakpointCreateByLocation(
            bp.fileName.toLocal8Bit().constData(), bp.lineNumber);
    if (llbp.IsValid()) {
        m_breakpoints.insert(id, llbp);

        llbp.SetIgnoreCount(bp.ignoreCount);
        bp.ignoreCount = llbp.GetIgnoreCount();
        bp.enabled = llbp.IsEnabled();

        lldb::SBBreakpointLocation location = llbp.GetLocationAtIndex(0);
        if (location.IsValid()) {
            bp.address = location.GetLoadAddress();

            // FIXME get those from lldb
            bp.lineNumber = bp.lineNumber;
            bp.fileName = bp.fileName;
            notifyAddBreakpointOk(id);
            showMessage(QLatin1String("[BB] ok."));
            notifyBreakpointAdjusted(id, bp);
        } else {
            m_breakpoints.take(id);
            showMessage(QLatin1String("[BB] failed. cant resolve yet"));
//            notifyAddBreakpointFailed(id);
//            notifyAddBreakpointOk(id);
        }
    } else {
        showMessage(QLatin1String("[BB] failed. dunno."));
        notifyAddBreakpointFailed(id);
    }
}

void LldbEngineGuest::removeBreakpoint(BreakpointId id)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(notifyRemoveBreakpointFailed(id); return);

    lldb::SBBreakpoint llbp = m_breakpoints.take(id);
    llbp.SetEnabled(false);
    notifyRemoveBreakpointOk(id);
}

void LldbEngineGuest::changeBreakpoint(BreakpointId id,
        const Internal::BreakpointParameters &bp)
{
    DEBUG_FUNC_ENTER;

    // TODO
    Q_UNUSED(id);
    Q_UNUSED(bp);
}

void LldbEngineGuest::selectThread(qint64 token)
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(return);

    m_frame_to_file.clear();
    for (uint i = 0; i < m_process->GetNumThreads(); i++) {
        lldb::SBThread t = m_process->GetThreadAtIndex(i);
        if (t.GetThreadID() == token) {
            m_currentThread = t;
            StackFrames frames;
            int firstResolvableFrame = -1;
            for (uint j = 0; j < t.GetNumFrames(); j++) {
                lldb::SBFrame fr = t.GetFrameAtIndex(j);
                if (!fr.IsValid()) {
                    qDebug("warning: frame %i is garbage", j);
                    continue;
                }
                lldb::SBSymbolContext context =
                    fr.GetSymbolContext(lldb::eSymbolContextEverything);
                lldb::SBSymbol sym = fr.GetSymbol();
                lldb::SBFunction func = fr.GetFunction();
                lldb::SBCompileUnit tu = fr.GetCompileUnit();
                lldb::SBModule module = fr.GetModule();
                lldb::SBBlock block = fr.GetBlock();
                lldb::SBBlock fblock = fr.GetFrameBlock();
                lldb::SBLineEntry le = fr.GetLineEntry();
                lldb::SBValueList values = fr.GetVariables(true, true, true, false);
#if 0
                qDebug()<<"\tframe "<<fr.GetFrameID();
                qDebug() << "\t\tPC:     " << ("0x" + QByteArray::number(
                            fr.GetPCAddress().GetLoadAddress(*m_target), 16)).data();
                qDebug() << "\t\tFP:     " << ("0x" + QByteArray::number(fr.GetFP(), 16)).data();
                qDebug() << "\t\tSP:     " << ("0x" + QByteArray::number(fr.GetSP(), 16)).data();
                qDebug() << "\t\tsymbol:  " << sym.IsValid() << sym.GetName() << sym.GetMangledName();
                qDebug() << "\t\tfunction:" << func.IsValid();
                qDebug() << "\t\ttu:      " << tu.IsValid();
                if (tu.IsValid())

                qDebug() << "\t\tmodule:  " << module.IsValid() << module.GetFileSpec().IsValid()
                    << module.GetFileSpec().GetFilename();
                qDebug() << "\t\tblock:   " << block.IsValid() << block.GetInlinedName();
                qDebug() << "\t\tfblock:  " << block.IsValid() << block.GetInlinedName();
                qDebug() << "\t\tle:      " << le.IsValid() << le.GetLine()<<le.GetColumn();
                qDebug() << "\t\tvalues:  "<<values.IsValid() << values.GetSize();
                qDebug() << "\t\tcontext: " << context.IsValid();
                qDebug() << "\t\t\tmodule:   " << context.GetModule().IsValid();
                qDebug() << "\t\t\tfunction: " << context.GetFunction().IsValid();
                qDebug() << "\t\t\tblock:    " << context.GetBlock().IsValid();
                qDebug() << "\t\t\tle:       " << context.GetLineEntry().IsValid();
                qDebug() << "\t\t\tsymbol:   " << context.GetSymbol().IsValid();
//            qDebug() << "\t\tdisassemly -->\n" << fr.Disassemble() << "<--";
#endif

                QString sourceFile;
                QString sourceFilePath;
                int lineNumber = 0;
                if (le.IsValid()) {
                    lineNumber = le.GetLine();
                    if (le.GetFileSpec().IsValid()) {
                        sourceFile = QString::fromLocal8Bit(le.GetFileSpec().GetFilename());
                        sourceFilePath = QString::fromLocal8Bit(le.GetFileSpec().GetDirectory())
                            + QLatin1String("/") + sourceFile;
                        if (firstResolvableFrame < 0)
                            firstResolvableFrame = j;
                    }
                }
                sourceFilePath = QFileInfo(sourceFilePath).canonicalFilePath();

                QString functionName;
                if (func.IsValid())
                    functionName = QString::fromLocal8Bit(func.GetName());
                else
                    functionName = QString::fromLocal8Bit(sym.GetName());

                StackFrame frame;
                frame.level = fr.GetFrameID();
                if (func.IsValid())
                    frame.function = QString::fromLocal8Bit(func.GetName());
                else
                    frame.function = QString::fromLocal8Bit(sym.GetName());
                frame.from = QString::fromLocal8Bit(module.GetFileSpec().GetFilename());
                frame.address = fr.GetPCAddress().GetLoadAddress(*m_target);
                frame.line = lineNumber;
                frame.file = sourceFilePath;
                frame.usable = QFileInfo(frame.file).isReadable();
                frames.append(frame);
                m_frame_to_file.insert(j, frame.file);
            }
            currentThreadChanged(token);
            listFrames(frames);
            activateFrame(firstResolvableFrame > -1 ? firstResolvableFrame : 0);
            return;
        }
    }
}

void LldbEngineGuest::updateThreads()
{
    DEBUG_FUNC_ENTER;
    SYNC_INFERIOR_OR(return);

   /* There is no  way to find the StopReason of a _process_
     * We try to emulate gdb here, by assuming there must be exactly one 'guilty' thread.
     * However, if there are no threads at all, it must be that the process
     * no longer exists. Let's tear down the whole session.
     */
    if (m_process->GetNumThreads() < 1) {
        notifyEngineSpontaneousShutdown();
        m_process->Kill();
        m_process->Destroy();
    }

    Threads threads;
    for (uint i = 0; i < m_process->GetNumThreads(); i++) {
        lldb::SBThread t = m_process->GetThreadAtIndex(i);
        if (!t.IsValid()) {
                qDebug("warning: thread %i is garbage", i);
                continue;
        }
        ThreadData thread;
        thread.id = t.GetThreadID();
        thread.targetId = QString::number(t.GetThreadID());
        thread.core.clear();
        thread.state = QString::number(t.GetStopReason());

        switch (t.GetStopReason()) {
            case lldb::eStopReasonInvalid:
            case lldb::eStopReasonNone:
            case lldb::eStopReasonTrace:
                thread.state = QLatin1String("running");
                break;
            case lldb::eStopReasonBreakpoint:
            case lldb::eStopReasonWatchpoint:
                showStatusMessage(QLatin1String("hit breakpoint"));
                thread.state = QLatin1String("hit breakpoint");
                if (m_currentThread.GetThreadID() != t.GetThreadID()) {
                    m_currentThread = t;
                    currentThreadChanged(t.GetThreadID());
                    m_relistFrames = true;
                }
                break;
            case lldb::eStopReasonSignal:
                showStatusMessage(QLatin1String("stopped"));
                thread.state = QLatin1String("stopped");
                if (m_currentThread.GetThreadID() != t.GetThreadID()) {
                    m_currentThread = t;
                    currentThreadChanged(t.GetThreadID());
                    m_relistFrames = true;
                }
                break;
            case lldb::eStopReasonException:
                showStatusMessage(QLatin1String("application crashed."));
                thread.state = QLatin1String("crashed");
                if (m_currentThread.GetThreadID() != t.GetThreadID()) {
                    m_currentThread = t;
                    currentThreadChanged(t.GetThreadID());
                    m_relistFrames = true;
                }
                break;
            case lldb::eStopReasonPlanComplete:
                thread.state = QLatin1String("crazy things happened");
                break;
        };

        thread.lineNumber = 0;
        thread.name = QString::fromLocal8Bit(t.GetName());

        lldb::SBFrame fr = t.GetFrameAtIndex(0);
        if (!fr.IsValid()) {
            qDebug("warning: frame 0 is garbage");
            continue;
        }
        lldb::SBSymbolContext context = fr.GetSymbolContext(lldb::eSymbolContextEverything);
        lldb::SBSymbol sym = fr.GetSymbol();
        lldb::SBFunction func = fr.GetFunction();
        lldb::SBLineEntry le = fr.GetLineEntry();
        QString sourceFile;
        QString sourceFilePath;
        int lineNumber = 0;
        if (le.IsValid()) {
            lineNumber = le.GetLine();
            if (le.GetFileSpec().IsValid()) {
                sourceFile = QString::fromLocal8Bit(le.GetFileSpec().GetFilename());
                sourceFilePath = QString::fromLocal8Bit(le.GetFileSpec().GetDirectory())
                    + QLatin1String("/") + sourceFile;
            }
        }
        QString functionName;
        if (func.IsValid())
            functionName = QString::fromLocal8Bit(func.GetName());
        else
            functionName = QString::fromLocal8Bit(sym.GetName());

        lldb::SBValueList values = fr.GetVariables(true, true, false, false);
        thread.fileName = sourceFile;
        thread.function = functionName;
        thread.address = fr.GetPCAddress().GetLoadAddress(*m_target);
        thread.lineNumber = lineNumber;
        threads.append(thread);
    }
    listThreads(threads);
    if (m_relistFrames) {
        selectThread(m_currentThread.GetThreadID());
        m_relistFrames = false;
    }
}

void LldbEngineGuest::lldbEvent(lldb::SBEvent *ev)
{
    qDebug() << "lldbevent" << ev->GetType() <<
         m_process->GetState() << (int)state();

    uint32_t etype = ev->GetType();
    switch (etype) {
        // ProcessEvent
        case 1:
            switch (m_process->GetState()) {
                case lldb::eStateRunning: // 5
                    if (!m_running)
                        m_running = true;
                    notifyInferiorPid(m_process->GetProcessID());
                    switch (state()) {
                        case EngineRunRequested:
                            notifyEngineRunAndInferiorRunOk();
                            break;
                        case InferiorRunRequested:
                            notifyInferiorRunOk();
                            break;
                        case InferiorStopOk:
                            notifyInferiorRunRequested();
                            notifyInferiorRunOk();
                            break;
                        default:
                            break;
                    }
                    break;
                case lldb::eStateExited: // 9
                    if (m_running)
                        m_running = false;
                    switch (state()) {
                        case InferiorShutdownRequested:
                            notifyInferiorShutdownOk();
                            break;
                        case InferiorRunOk:
                            m_relistFrames = true;
                            updateThreads();
                            notifyEngineSpontaneousShutdown();
                            m_process->Kill();
                            m_process->Destroy();
                            break;
                        default:
                            updateThreads();
                            break;
                    }
                    break;
                case lldb::eStateStopped: // 4
                    if (m_running)
                        m_running = false;
                    switch (state()) {
                        case InferiorShutdownRequested:
                            notifyInferiorShutdownOk();
                            break;
                        case InferiorRunOk:
                            m_relistFrames = true;
                            updateThreads();
                            notifyInferiorSpontaneousStop();
                            // fall
                        default:
                            m_relistFrames = true;
                            updateThreads();
                            break;
                    }
                    break;
                case lldb::eStateCrashed: // 7
                    if (m_running)
                        m_running = false;
                    switch (state()) {
                        case InferiorShutdownRequested:
                            notifyInferiorShutdownOk();
                            break;
                        case InferiorRunOk:
                            m_relistFrames = true;
                            updateThreads();
                            notifyInferiorSpontaneousStop();
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    qDebug("unexpected ProcessEvent");
                    break;
            }
            break;;
        default:
            break;;
    };
}


} // namespace Internal
} // namespace Debugger
