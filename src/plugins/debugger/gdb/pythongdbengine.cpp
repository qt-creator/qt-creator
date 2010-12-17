/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gdbengine.h"
#include "gdbmi.h"
#include "abstractgdbadapter.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include "breakhandler.h"
#include "watchhandler.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>

#define PRECONDITION QTC_ASSERT(hasPython(), /**/)
#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)


namespace Debugger {
namespace Internal {

void GdbEngine::updateLocalsPython(bool tryPartial, const QByteArray &varList)
{
    PRECONDITION;
    m_processedNames.clear();
    watchHandler()->beginCycle(!tryPartial);
    //m_toolTipExpression.clear();
    WatchHandler *handler = watchHandler();

    QByteArray expanded = "expanded:" + handler->expansionRequests() + ' ';
    expanded += "typeformats:" + handler->typeFormatRequests() + ' ';
    expanded += "formats:" + handler->individualFormatRequests();

    QByteArray watchers;
    if (!m_toolTipExpression.isEmpty()) {
        watchers += m_toolTipExpression.toLatin1();
        watchers += '#';
        watchers += tooltipIName(m_toolTipExpression);
    }

    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    QByteArray options;
    if (debuggerCore()->boolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (debuggerCore()->boolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (!qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty())
        options += "pe,";
    if (options.isEmpty())
        options += "defaults,";
    if (tryPartial)
        options += "partial,";
    options.chop(1);

    QByteArray resultVar;
    if (!m_resultVarName.isEmpty())
        resultVar = "resultvarname:" + m_resultVarName + ' ';

    postCommand("bb options:" + options + " vars:" + varList + ' '
            + resultVar + expanded + " watchers:" + watchers.toHex(),
        WatchUpdate, CB(handleStackFramePython), QVariant(tryPartial));
}

void GdbEngine::handleStackListLocalsPython(const GdbResponse &response)
{
    PRECONDITION;
    if (response.resultClass == GdbResultDone) {
        // 44^done,data={locals=[name="model",name="backString",...]}
        QByteArray varList = "vars"; // Dummy entry, will be stripped by dumper.
        foreach (const GdbMi &child, response.data.findChild("locals").children()) {
            varList.append(',');
            varList.append(child.data());
        }
        updateLocalsPython(false, varList);
    }
}

void GdbEngine::handleStackFramePython(const GdbResponse &response)
{
    PRECONDITION;
    if (response.resultClass == GdbResultDone) {
        const bool partial = response.cookie.toBool();
        Q_UNUSED(partial);
        //qDebug() << "READING " << (partial ? "PARTIAL" : "FULL");
        QByteArray out = response.data.findChild("consolestreamoutput").data();
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        //qDebug() << "SECOND CHUNK: " << out;
        int pos = out.indexOf("data=");
        if (pos != 0) {
            showMessage(_("DISCARDING JUNK AT BEGIN OF RESPONSE: "
                + out.left(pos)));
            out = out.mid(pos);
        }
        GdbMi all;
        all.fromStringMultiple(out);
        //qDebug() << "ALL: " << all.toString();

        GdbMi data = all.findChild("data");
        QList<WatchData> list;
        foreach (const GdbMi &child, data.children()) {
            WatchData dummy;
            dummy.iname = child.findChild("iname").data();
            dummy.name = _(child.findChild("name").data());
            //qDebug() << "CHILD: " << child.toString();
            parseWatchData(watchHandler()->expandedINames(), dummy, child, &list);
        }
        watchHandler()->insertBulkData(list);
        //for (int i = 0; i != list.size(); ++i)
        //    qDebug() << "LOCAL: " << list.at(i).toString();

#if 0
        data = all.findChild("bkpts");
        if (data.isValid()) {
            BreakHandler *handler = breakHandler();
            foreach (const GdbMi &child, data.children()) {
                int bpNumber = child.findChild("number").data().toInt();
                int found = handler->findBreakpoint(bpNumber);
                if (found != -1) {
                    BreakpointData *bp = handler->at(found);
                    GdbMi addr = child.findChild("addr");
                    if (addr.isValid()) {
                        bp->bpAddress = child.findChild("addr").data();
                        bp->pending = false;
                    } else {
                        bp->bpAddress = "<PENDING>";
                        bp->pending = true;
                    }
                    bp->bpFuncName = child.findChild("func").data();
                    bp->bpLineNumber = child.findChild("line").data();
                    bp->bpFileName = child.findChild("file").data();
                    bp->markerLineNumber = bp->bpLineNumber.toInt();
                    bp->markerFileName = bp->bpFileName;
                    // Happens with moved/symlinked sources.
                    if (!bp->fileName.isEmpty()
                            && !bp->bpFileName.isEmpty()
                            && bp->fileName !=  bp->bpFileName)
                        bp->markerFileName = bp->fileName;
                } else {
                    QTC_ASSERT(false, qDebug() << child.toString() << bpNumber);
                    //bp->bpNumber = "<unavailable>";
                }
            }
            handler->updateMarkers();
        }
#endif

        //PENDING_DEBUG("AFTER handleStackFrame()");
        // FIXME: This should only be used when updateLocals() was
        // triggered by expanding an item in the view.
        if (m_pendingWatchRequests <= 0) {
            //PENDING_DEBUG("\n\n ....  AND TRIGGERS MODEL UPDATE\n");
            rebuildWatchModel();
        }
    } else {
        showMessage(_("DUMPER FAILED: " + response.toString()));
    }
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAllPython()
{
    PRECONDITION;
    //PENDING_DEBUG("UPDATING ALL\n");
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopOk, /**/);
    reloadModulesInternal();
    postCommand("-stack-list-frames", CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    stackHandler()->setCurrentIndex(0);
    if (m_gdbAdapter->isTrkAdapter())
        m_gdbAdapter->trkReloadThreads();
    else
        postCommand("-thread-list-ids", CB(handleThreadListIds), 0);
    reloadRegisters();
    updateLocals();
}

} // namespace Internal
} // namespace Debugger
