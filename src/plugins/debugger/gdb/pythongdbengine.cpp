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

#include "watchhandler.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>

#define PRECONDITION QTC_ASSERT(hasPython(), /**/)
#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)


namespace Debugger {
namespace Internal {

void GdbEngine::updateLocalsPython(const QByteArray &varList)
{
    PRECONDITION;
    if (m_gdbAdapter->isTrkAdapter()) {
        postCommand("-stack-list-locals 0",
            WatchUpdate, CB(handleStackListLocalsPython));
        return;
    }
    m_processedNames.clear();
    manager()->watchHandler()->beginCycle();
    //m_toolTipExpression.clear();
    WatchHandler *handler = m_manager->watchHandler();

    QByteArray expanded;
    QSet<QByteArray> expandedINames = handler->expandedINames();
    QSetIterator<QByteArray> jt(expandedINames);
    while (jt.hasNext()) {
        expanded.append(jt.next());
        expanded.append(',');
    }
    if (expanded.isEmpty())
        expanded.append("defaults,");
    expanded.chop(1);

    QByteArray watchers;
    if (!m_toolTipExpression.isEmpty())
        watchers += m_toolTipExpression.toLatin1()
            + "#" + tooltipINameForExpression(m_toolTipExpression.toLatin1());

    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        if (it.key() == WatchHandler::watcherEditPlaceHolder().toLatin1())
            watchers += "<Edit>#watch." + QByteArray::number(it.value());
        else
            watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    QByteArray options;
    if (theDebuggerBoolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (theDebuggerBoolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (options.isEmpty())
        options += "defaults,";
    options.chop(1);

    postCommand("bb " + options + " @" + varList + ' '
            + expanded + ' ' + watchers.toHex(),
        WatchUpdate, CB(handleStackFramePython));
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
        updateLocalsPython(varList);
    }
}

void GdbEngine::handleStackFramePython(const GdbResponse &response)
{
    PRECONDITION;
    if (response.resultClass == GdbResultDone) {
        QByteArray out = response.data.findChild("consolestreamoutput").data();
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        //qDebug() << "SECOND CHUNK: " << out;
        int pos = out.indexOf("data=");
        if (pos != 0) {
            qDebug() << "DISCARDING JUNK AT BEGIN OF RESPONSE: "
                << out.left(pos);
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
            handleChildren(dummy, child, &list);
        }
        manager()->watchHandler()->insertBulkData(list);
        //for (int i = 0; i != list.size(); ++i)
        //    qDebug() << "LOCAL: " << list.at(i).toString();

        //PENDING_DEBUG("AFTER handleStackFrame()");
        // FIXME: This should only be used when updateLocals() was
        // triggered by expanding an item in the view.
        if (m_pendingRequests <= 0) {
            //PENDING_DEBUG("\n\n ....  AND TRIGGERS MODEL UPDATE\n");
            rebuildModel();
        }
    } else {
        debugMessage(_("DUMPER FAILED: " + response.toString()));
    }
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAllPython()
{
    PRECONDITION;
    //PENDING_DEBUG("UPDATING ALL\n");
    QTC_ASSERT(state() == InferiorUnrunnable || state() == InferiorStopped, /**/);
    reloadModulesInternal();
    postCommand("-stack-list-frames", WatchUpdate, CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    manager()->stackHandler()->setCurrentIndex(0);
    if (supportsThreads())
        postCommand("-thread-list-ids", WatchUpdate, CB(handleStackListThreads), 0);
    manager()->reloadRegisters();
    updateLocals(); 
}

} // namespace Internal
} // namespace Debugger
