/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gdbengine.h"

#include "abstractgdbadapter.h"
#include "debuggeractions.h"
#include "debuggerstringutils.h"

#include "stackhandler.h"
#include "watchhandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>


#define PRECONDITION QTC_ASSERT(!hasPython(), /**/)
#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)


//#define DEBUG_PENDING  1
//#define DEBUG_SUBITEM  1

#if DEBUG_PENDING
#   define PENDING_DEBUG(s) qDebug() << s
#else
#   define PENDING_DEBUG(s)
#endif
#define PENDING_DEBUGX(s) qDebug() << s

namespace Debugger {
namespace Internal {

// reads a MI-encoded item frome the consolestream
static bool parseConsoleStream(const GdbResponse &response, GdbMi *contents)
{
    GdbMi output = response.data.findChild("consolestreamoutput");
    QByteArray out = output.data();

    int markerPos = out.indexOf('"') + 1; // position of 'success marker'
    if (markerPos == 0 || out.at(markerPos) == 'f') {  // 't' or 'f'
        // custom dumper produced no output
        return false;
    }

    out = out.mid(markerPos +  1);
    out = out.left(out.lastIndexOf('"'));
    // optimization: dumper output never needs real C unquoting
    out.replace('\\', "");

    contents->fromStringMultiple(out);
    //qDebug() << "CONTENTS" << contents->toString(true);
    return contents->isValid();
}

static double getDumperVersion(const GdbMi &contents)
{
    const GdbMi dumperVersionG = contents.findChild("dumperversion");
    if (dumperVersionG.type() != GdbMi::Invalid) {
        bool ok;
        const double v = QString::fromAscii(dumperVersionG.data()).toDouble(&ok);
        if (ok)
            return v;
    }
    return 1.0;
}


void GdbEngine::updateLocalsClassic(const QVariant &cookie)
{
    PRECONDITION;
    m_processedNames.clear();

    //PENDING_DEBUG("\nRESET PENDING");
    //m_toolTipCache.clear();
    m_toolTipExpression.clear();
    manager()->watchHandler()->beginCycle();

    // Asynchronous load of injected library, initialize in first stop
    if (m_dumperInjectionLoad && m_debuggingHelperState == DebuggingHelperLoadTried
            && m_dumperHelper.typeCount() == 0
            && inferiorPid() > 0)
        tryQueryDebuggingHelpersClassic();

    QByteArray level = QByteArray::number(currentFrame());
    // '2' is 'list with type and value'
    QByteArray cmd = "-stack-list-arguments 2 " + level + ' ' + level;
    postCommand(cmd, WatchUpdate,
        CB(handleStackListArgumentsClassic));
    // '2' is 'list with type and value'
    postCommand("-stack-list-locals 2", WatchUpdate,
        CB(handleStackListLocalsClassic), cookie); // stage 2/2
}

static inline QString msgRetrievingWatchData(int pending)
{
    return GdbEngine::tr("Retrieving data for watch view (%n requests pending)...", 0, pending);
}

void GdbEngine::runDirectDebuggingHelperClassic(const WatchData &data, bool dumpChildren)
{
    Q_UNUSED(dumpChildren)
    QByteArray type = data.type.toLatin1();
    QByteArray cmd;

    if (type == "QString" || type.endsWith("::QString"))
        cmd = "qdumpqstring (&(" + data.exp + "))";
    else if (type == "QStringList" || type.endsWith("::QStringList"))
        cmd = "qdumpqstringlist (&(" + data.exp + "))";

    QVariant var;
    var.setValue(data);
    postCommand(cmd, WatchUpdate, CB(handleDebuggingHelperValue3Classic), var);

    showStatusMessage(msgRetrievingWatchData(m_pendingRequests + 1), 10000);
}

void GdbEngine::runDebuggingHelperClassic(const WatchData &data0, bool dumpChildren)
{
    PRECONDITION;
    if (m_debuggingHelperState != DebuggingHelperAvailable) {
        runDirectDebuggingHelperClassic(data0, dumpChildren);
        return;
    }
    WatchData data = data0;

    // Avoid endless loops created by faulty dumpers.
    QByteArray processedName = QByteArray::number(dumpChildren) + '-' + data.iname;
    if (m_processedNames.contains(processedName)) {
        gdbInputAvailable(LogStatus,
            _("<Breaking endless loop for " + data.iname + ">"));
        data.setAllUnneeded();
        data.setValue(_("<unavailable>"));
        data.setHasChildren(false);
        insertData(data);
        return; 
    }
    m_processedNames.insert(processedName);

    QByteArray params;
    QStringList extraArgs;
    const QtDumperHelper::TypeData td = m_dumperHelper.typeData(data0.type);
    m_dumperHelper.evaluationParameters(data, td, QtDumperHelper::GdbDebugger, &params, &extraArgs);

    //int protocol = (data.iname.startsWith("watch") && data.type == "QImage") ? 3 : 2;
    //int protocol = data.iname.startsWith("watch") ? 3 : 2;
    const int protocol = 2;
    //int protocol = isDisplayedIName(data.iname) ? 3 : 2;

    QByteArray addr;
    if (data.addr.startsWith("0x"))
        addr = "(void*)" + data.addr;
    else if (data.exp.isEmpty()) // happens e.g. for QAbstractItem
        addr = "0";
    else
        addr = "&(" + data.exp + ')';

    sendWatchParameters(params);

    QString cmd;
    QTextStream(&cmd) << "call " << "(void*)qDumpObjectData440(" <<
            protocol << ",0," <<  addr << ',' << (dumpChildren ? "1" : "0")
            << ',' << extraArgs.join(QString(_c(','))) <<  ')';

    postCommand(cmd.toLatin1(), WatchUpdate | NonCriticalResponse);

    showStatusMessage(msgRetrievingWatchData(m_pendingRequests + 1), 10000);

    // retrieve response
    postCommand("p (char*)&qDumpOutBuffer", WatchUpdate,
        CB(handleDebuggingHelperValue2Classic), qVariantFromValue(data));
}

void GdbEngine::createGdbVariableClassic(const WatchData &data)
{
    PRECONDITION;
    if (data.iname == "local.flist.0") {
        int i = 1;
        Q_UNUSED(i);
    }
    postCommand("-var-delete \"" + data.iname + '"', WatchUpdate);
    QByteArray exp = data.exp;
    if (exp.isEmpty() && data.addr.startsWith("0x"))
        exp = "*(" + gdbQuoteTypes(data.type).toLatin1() + "*)" + data.addr;
    QVariant val = QVariant::fromValue<WatchData>(data);
    postCommand("-var-create \"" + data.iname + "\" * \"" + exp + '"',
        WatchUpdate, CB(handleVarCreate), val);
}

void GdbEngine::updateSubItemClassic(const WatchData &data0)
{
    PRECONDITION;
    WatchData data = data0;
    #if DEBUG_SUBITEM
    qDebug() << "UPDATE SUBITEM:" << data.toString();
    #endif
    QTC_ASSERT(data.isValid(), return);

    // in any case we need the type first
    if (data.isTypeNeeded()) {
        // This should only happen if we don't have a variable yet.
        // Let's play safe, though.
        if (!data.variable.isEmpty()) {
            // Update: It does so for out-of-scope watchers.
            #if 1
            qDebug() << "FIXME: GdbEngine::updateSubItem:"
                 << data.toString() << "should not happen";
            #else
            data.setType(WatchData::msgNotInScope());
            data.setValue(WatchData::msgNotInScope());
            data.setHasChildren(false);
            insertData(data);
            return;
            #endif
        }
        // The WatchVarCreate handler will receive type information
        // and re-insert a WatchData item with correct type, so
        // we will not re-enter this bit.
        // FIXME: Concurrency issues?
        createGdbVariableClassic(data);
        return;
    }

    // we should have a type now. this is relied upon further below
    QTC_ASSERT(!data.type.isEmpty(), return);

    // a common case that can be easily solved
    if (data.isChildrenNeeded() && isPointerType(data.type)
        && !hasDebuggingHelperForType(data.type)) {
        // We sometimes know what kind of children pointers have
        #if DEBUG_SUBITEM
        qDebug() << "IT'S A POINTER";
        #endif
    
        if (theDebuggerBoolSetting(AutoDerefPointers)) {
            // Try automatic dereferentiation
            data.exp = "(*(" + data.exp + "))";
            data.type = data.type + _("."); // FIXME: fragile HACK to avoid recursion
            insertData(data);
        } else {
            data.setChildrenUnneeded();
            insertData(data);
            WatchData data1;
            data1.iname = data.iname + ".*";
            data1.name = QLatin1Char('*') + data.name;
            data1.exp = "(*(" + data.exp + "))";
            data1.type = stripPointerType(data.type);
            data1.setValueNeeded();
            data1.setChildrenUnneeded();
            insertData(data1);
        }
        return;
    }

    if (data.isValueNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE";
        #endif
        runDebuggingHelperClassic(data,
            manager()->watchHandler()->isExpandedIName(data.iname));
        return;
    }

/*
    if (data.isValueNeeded() && data.exp.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: NO EXPRESSION?";
        #endif
        data.setError("<no expression given>");
        insertData(data);
        return;
    }
*/

    if (data.isValueNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR VALUE";
        #endif
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with valueNeeded() set.
        return;
    }

    if (data.isValueNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VALUE";
        #endif
        QByteArray cmd = "-var-evaluate-expression \"" + data.iname + '"';
        postCommand(cmd, WatchUpdate, CB(handleEvaluateExpression),
            QVariant::fromValue(data));
        return;
    }

    if (data.isChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runDebuggingHelperClassic(data, true);
        return;
    }

    if (data.isChildrenNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDREN";
        #endif
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }

    if (data.isChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        postCommand(cmd, WatchUpdate, CB(handleVarListChildren), QVariant::fromValue(data));
        return;
    }

    if (data.isHasChildrenNeeded() && hasDebuggingHelperForType(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runDebuggingHelperClassic(data,
            manager()->watchHandler()->isExpandedIName(data.iname));
        return;
    }

//#if !X
    if (data.isHasChildrenNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDCOUNT";
        #endif
        createGdbVariableClassic(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }
//#endif

    if (data.isHasChildrenNeeded()) {
        QTC_ASSERT(!data.variable.isEmpty(), return); // tested above
        QByteArray cmd = "-var-list-children --all-values \"" + data.variable + '"';
        postCommand(cmd, Discardable,
            CB(handleVarListChildren), QVariant::fromValue(data));
        return;
    }

    qDebug() << "FIXME: UPDATE SUBITEM:" << data.toString();
    QTC_ASSERT(false, return);
}

void GdbEngine::handleDebuggingHelperValue2Classic(const GdbResponse &response)
{
    PRECONDITION;
    WatchData data = response.cookie.value<WatchData>();
    QTC_ASSERT(data.isValid(), return);

    // The real dumper might have aborted without giving any answers.
    // Remove traces of the question, too.
    if (m_cookieForToken.contains(response.token - 1)) {
        m_cookieForToken.remove(response.token - 1);
        debugMessage(_("DETECTING LOST COMMAND %1").arg(response.token - 1));
        --m_pendingRequests;
        data.setError(WatchData::msgNotInScope());
        insertData(data);
        return;
    }

    //qDebug() << "CUSTOM VALUE RESULT:" << response.toString();
    //qDebug() << "FOR DATA:" << data.toString() << response.resultClass;
    if (response.resultClass != GdbResultDone) {
        qDebug() << "STRANGE CUSTOM DUMPER RESULT DATA:" << data.toString();
        return;
    }

    GdbMi contents;
    if (!parseConsoleStream(response, &contents)) {
        data.setError(WatchData::msgNotInScope());
        insertData(data);
        return;
    }

    setWatchDataType(data, response.data.findChild("type"));
    setWatchDataDisplayedType(data, response.data.findChild("displaytype"));
    QList<WatchData> list;
    handleChildren(data, contents, &list);
    //for (int i = 0; i != list.size(); ++i)
    //    qDebug() << "READ: " << list.at(i).toString();
    manager()->watchHandler()->insertBulkData(list);
}

void GdbEngine::handleDebuggingHelperValue3Classic(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        WatchData data = response.cookie.value<WatchData>();
        QByteArray out = response.data.findChild("consolestreamoutput").data();
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        QList<QByteArray> list = out.split(' ');
        //qDebug() << "RECEIVED" << response.toString() << "FOR" << data0.toString()
        //    <<  " STREAM:" << out;
        if (list.isEmpty()) {
            //: Value for variable
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == __("QString")
                || data.type.endsWith(__("::QString"))) {
            QList<QByteArray> list = out.split(' ');
            QString str;
            int l = out.isEmpty() ? 0 : list.size();
            for (int i = 0; i < l; ++i)
                 str.append(list.at(i).toInt());
            data.setValue(_c('"') + str + _c('"'));
            data.setHasChildren(false);
            data.setAllUnneeded();
            insertData(data);
        } else if (data.type == __("QStringList")
                || data.type.endsWith(__("::QStringList"))) {
            if (out.isEmpty()) {
                data.setValue(tr("<0 items>"));
                data.setHasChildren(false);
                data.setAllUnneeded();
                insertData(data);
            } else {
                int l = list.size();
                //: In string list
                data.setValue(tr("<%n items>", 0, l));
                data.setHasChildren(!list.empty());
                data.setAllUnneeded();
                insertData(data);
                for (int i = 0; i < l; ++i) {
                    WatchData data1;
                    data1.name = _("[%1]").arg(i);
                    data1.type = data.type.left(data.type.size() - 4);
                    data1.iname = data.iname + '.' + QByteArray::number(i);
                    data1.addr = list.at(i);
                    data1.exp = "((" + gdbQuoteTypes(data1.type).toLatin1() + "*)" + data1.addr + ")";
                    data1.setHasChildren(false);
                    data1.setValueNeeded();
                    QByteArray cmd = "qdumpqstring (" + data1.exp + ')';
                    QVariant var;
                    var.setValue(data1);
                    postCommand(cmd, WatchUpdate,
                        CB(handleDebuggingHelperValue3Classic), var);
                }
            }
        } else {
            //: Value for variable
            data.setError(WatchData::msgNotInScope());
            data.setAllUnneeded();
            insertData(data);
        }
    } else {
        WatchData data = response.cookie.value<WatchData>();
        data.setError(WatchData::msgNotInScope());
        data.setAllUnneeded();
        insertData(data);
    }
}

void GdbEngine::tryLoadDebuggingHelpersClassic()
{
    PRECONDITION;
    if (isSynchroneous())
        return;
    switch (m_debuggingHelperState) {
    case DebuggingHelperUninitialized:
        break;
    case DebuggingHelperLoadTried:
        tryQueryDebuggingHelpersClassic();
        return;
    case DebuggingHelperAvailable:
    case DebuggingHelperUnavailable:
        return;
    }

    if (m_gdbAdapter->dumperHandling() == AbstractGdbAdapter::DumperNotAvailable) {
        // Load at least gdb macro based dumpers.
        QFile file(_(":/gdb/gdbmacros.txt"));
        file.open(QIODevice::ReadOnly);
        QByteArray contents = file.readAll(); 
        m_debuggingHelperState = DebuggingHelperLoadTried;
        postCommand(contents);
        return;
    }
    if (m_dumperInjectionLoad && inferiorPid() <= 0) // Need PID to inject
        return;

    PENDING_DEBUG("TRY LOAD CUSTOM DUMPERS");
    m_debuggingHelperState = DebuggingHelperUnavailable;
    if (!checkDebuggingHelpersClassic())
        return;

    m_debuggingHelperState = DebuggingHelperLoadTried;
    QByteArray dlopenLib;
    if (startParameters().startMode == StartRemote)
        dlopenLib = startParameters().remoteDumperLib.toLocal8Bit();
    else
        dlopenLib = manager()->qtDumperLibraryName().toLocal8Bit();
#if defined(Q_OS_WIN)
    if (m_dumperInjectionLoad) {
        /// Launch asynchronous remote thread to load.
        SharedLibraryInjector injector(inferiorPid());
        QString errorMessage;
        const QString dlopenLibString = _(dlopenLib);
        if (injector.remoteInject(dlopenLibString, false, &errorMessage)) {
            debugMessage(_("Dumper injection loading triggered (%1)...").
                         arg(dlopenLibString));
        } else {
            debugMessage(_("Dumper loading (%1) failed: %2").
                         arg(dlopenLibString, errorMessage));
            debugMessage(errorMessage);
            manager()->showQtDumperLibraryWarning(errorMessage);
            m_debuggingHelperState = DebuggingHelperUnavailable;
            return;
        }
    } else {
        debugMessage(_("Loading dumpers via debugger call (%1)...").
                     arg(_(dlopenLib)));
        postCommand("sharedlibrary .*"); // for LoadLibraryA
        //postCommand("handle SIGSEGV pass stop print");
        //postCommand("set unwindonsignal off");
        postCommand("call LoadLibraryA(\"" + GdbMi::escapeCString(dlopenLib) + "\")",
                    CB(handleDebuggingHelperSetup));
        postCommand("sharedlibrary " + dotEscape(dlopenLib));
    }
#elif defined(Q_OS_MAC)
    //postCommand("sharedlibrary libc"); // for malloc
    //postCommand("sharedlibrary libdl"); // for dlopen
    postCommand("call (void)dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " STRINGIFY(RTLD_NOW) ")",
        CB(handleDebuggingHelperSetup));
    //postCommand("sharedlibrary " + dotEscape(dlopenLib));
#else
    //postCommand("p dlopen");
    postCommand("sharedlibrary libc"); // for malloc
    postCommand("sharedlibrary libdl"); // for dlopen
    postCommand("call (void*)dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " STRINGIFY(RTLD_NOW) ")",
        CB(handleDebuggingHelperSetup));
    // some older systems like CentOS 4.6 prefer this:
    postCommand("call (void*)__dlopen(\"" + GdbMi::escapeCString(dlopenLib)
                + "\", " STRINGIFY(RTLD_NOW) ")",
        CB(handleDebuggingHelperSetup));
    postCommand("sharedlibrary " + dotEscape(dlopenLib));
#endif
    if (!m_dumperInjectionLoad)
        tryQueryDebuggingHelpersClassic();
}

void GdbEngine::tryQueryDebuggingHelpersClassic()
{
    PRECONDITION;
    // retrieve list of dumpable classes
    postCommand("call (void*)qDumpObjectData440(1,0,0,0,0,0,0,0)");
    postCommand("p (char*)&qDumpOutBuffer",
        CB(handleQueryDebuggingHelperClassic));
}

void GdbEngine::recheckDebuggingHelperAvailabilityClassic()
{
    PRECONDITION;
    if (m_gdbAdapter->dumperHandling() != AbstractGdbAdapter::DumperNotAvailable) {
        // retrieve list of dumpable classes
        postCommand("call (void*)qDumpObjectData440(1,0,0,0,0,0,0,0)");
        postCommand("p (char*)&qDumpOutBuffer",
            CB(handleQueryDebuggingHelperClassic));
    }
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAllClassic()
{
    PRECONDITION;
    PENDING_DEBUG("UPDATING ALL\n");
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopped, /**/);
    tryLoadDebuggingHelpersClassic();
    reloadModulesInternal();
    postCommand("-stack-list-frames", WatchUpdate,
        CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    manager()->stackHandler()->setCurrentIndex(0);
    if (supportsThreads())
        postCommand("-thread-list-ids", WatchUpdate, CB(handleStackListThreads), 0);
    manager()->reloadRegisters();
    updateLocals(); 
}

void GdbEngine::setDebugDebuggingHelpersClassic(const QVariant &on)
{
    PRECONDITION;
    if (on.toBool()) {
        debugMessage(_("SWITCHING ON DUMPER DEBUGGING"));
        postCommand("set unwindonsignal off");
        m_manager->breakByFunction(_("qDumpObjectData440"));
        //updateLocals();
    } else {
        debugMessage(_("SWITCHING OFF DUMPER DEBUGGING"));
        postCommand("set unwindonsignal on");
    }
}

void GdbEngine::setDebuggingHelperStateClassic(DebuggingHelperState s)
{
    PRECONDITION;
    m_debuggingHelperState = s;
}


void GdbEngine::handleStackListArgumentsClassic(const GdbResponse &response)
{
    PRECONDITION;
    // stage 1/2

    // Linux:
    // 12^done,stack-args=
    //   [frame={level="0",args=[
    //     {name="argc",type="int",value="1"},
    //     {name="argv",type="char **",value="(char **) 0x7..."}]}]
    // Mac:
    // 78^done,stack-args=
    //    {frame={level="0",args={
    //      varobj=
    //        {exp="this",value="0x38a2fab0",name="var21",numchild="3",
    //             type="CurrentDocumentFind *  const",typecode="PTR",
    //             dynamic_type="",in_scope="true",block_start_addr="0x3938e946",
    //             block_end_addr="0x3938eb2d"},
    //      varobj=
    //         {exp="before",value="@0xbfffb9f8: {d = 0x3a7f2a70}",
    //              name="var22",numchild="1",type="const QString  ...} }}}
    //
    // In both cases, iterating over the children of stack-args/frame/args
    // is ok.
    m_currentFunctionArgs.clear();
    if (response.resultClass == GdbResultDone) {
        const GdbMi list = response.data.findChild("stack-args");
        const GdbMi frame = list.findChild("frame");
        const GdbMi args = frame.findChild("args");
        m_currentFunctionArgs = args.children();
    } else {
        qDebug() << "FIXME: GdbEngine::handleStackListArguments: should not happen"
            << response.toString();
    }
}

void GdbEngine::handleStackListLocalsClassic(const GdbResponse &response)
{
    PRECONDITION;
    // stage 2/2

    // There could be shadowed variables
    QList<GdbMi> locals = response.data.findChild("locals").children();
    locals += m_currentFunctionArgs;
    QMap<QByteArray, int> seen;
    // If desired, retrieve list of uninitialized variables looking at
    // the current frame. This is invoked first time after a stop from
    // handleStop1, which passes on the frame as cookie. The whole stack
    // is not known at this point.
    QStringList uninitializedVariables;
    if (theDebuggerAction(UseCodeModel)->isChecked()) {
        const StackFrame frame = qVariantCanConvert<Debugger::Internal::StackFrame>(response.cookie) ?
                                 qVariantValue<Debugger::Internal::StackFrame>(response.cookie) :
                                 m_manager->stackHandler()->currentFrame();
        if (frame.isUsable())
            getUninitializedVariables(m_manager->cppCodeModelSnapshot(),
                                      frame.function, frame.file, frame.line,
                                      &uninitializedVariables);
    }
    QList<WatchData> list;
    foreach (const GdbMi &item, locals) {
        const WatchData data = localVariable(item, uninitializedVariables, &seen);
        if (data.isValid())
            list.push_back(data);
    }
    manager()->watchHandler()->insertBulkData(list);
    manager()->watchHandler()->updateWatchers();
}

bool GdbEngine::checkDebuggingHelpersClassic()
{
    PRECONDITION;
    if (!manager()->qtDumperLibraryEnabled())
        return false;
    const QString lib = qtDumperLibraryName();
    //qDebug() << "DUMPERLIB:" << lib;
    const QFileInfo fi(lib);
    if (!fi.exists()) {
        const QStringList &locations = manager()->qtDumperLibraryLocations();
        const QString loc = locations.join(QLatin1String(", "));
        const QString msg = tr("The debugging helper library was not found at %1.").arg(loc);
        debugMessage(msg);
        manager()->showQtDumperLibraryWarning(msg);
        return false;
    }
    return true;
}

void GdbEngine::handleQueryDebuggingHelperClassic(const GdbResponse &response)
{
    const double dumperVersionRequired = 1.0;
    //qDebug() << "DATA DUMPER TRIAL:" << response.toString();

    GdbMi contents;
    QTC_ASSERT(parseConsoleStream(response, &contents), qDebug() << response.toString());
    const bool ok = m_dumperHelper.parseQuery(contents, QtDumperHelper::GdbDebugger)
        && m_dumperHelper.typeCount();
    if (ok) {
        // Get version and sizes from dumpers. Expression cache
        // currently causes errors.
        const double dumperVersion = getDumperVersion(contents);
        if (dumperVersion < dumperVersionRequired) {
            manager()->showQtDumperLibraryWarning(
                QtDumperHelper::msgDumperOutdated(dumperVersionRequired, dumperVersion));
            m_debuggingHelperState = DebuggingHelperUnavailable;
            return;
        }
        m_debuggingHelperState = DebuggingHelperAvailable;
        const QString successMsg = tr("Dumper version %1, %n custom dumpers found.",
            0, m_dumperHelper.typeCount()).arg(dumperVersion);
        showStatusMessage(successMsg);
    } else {
        if (!m_dumperInjectionLoad) // Retry if thread has not terminated yet.
            m_debuggingHelperState = DebuggingHelperUnavailable;
        showStatusMessage(tr("Debugging helpers not found."));
    }
    //qDebug() << m_dumperHelper.toString(true);
    //qDebug() << m_availableSimpleDebuggingHelpers << "DATA DUMPERS AVAILABLE";
}
} // namespace Internal
} // namespace Debugger
