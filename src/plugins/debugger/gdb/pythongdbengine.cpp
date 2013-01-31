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

#include "gdbengine.h"

#include "debuggerprotocol.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "watchhandler.h"
#include "stackhandler.h"

#include <utils/qtcassert.h>

#define PRECONDITION QTC_CHECK(hasPython())
#define CB(callback) &GdbEngine::callback, STRINGIFY(callback)


namespace Debugger {
namespace Internal {

void GdbEngine::updateLocalsPython(const UpdateParameters &params)
{
    PRECONDITION;
    //m_pendingWatchRequests = 0;
    m_pendingBreakpointRequests = 0;
    m_processedNames.clear();

    WatchHandler *handler = watchHandler();
    QByteArray expanded = "expanded:" + handler->expansionRequests() + ' ';
    expanded += "typeformats:" + handler->typeFormatRequests() + ' ';
    expanded += "formats:" + handler->individualFormatRequests();

    QByteArray watchers;
    const QString fileName = stackHandler()->currentFrame().file;
    const QString function = stackHandler()->currentFrame().function;
    if (!fileName.isEmpty()) {
        typedef DebuggerToolTipManager::ExpressionInamePair ExpressionInamePair;
        typedef DebuggerToolTipManager::ExpressionInamePairs ExpressionInamePairs;

        // Re-create tooltip items that are not filters on existing local variables in
        // the tooltip model.
        ExpressionInamePairs toolTips = DebuggerToolTipManager::instance()
            ->treeWidgetExpressions(fileName, objectName(), function);

        const QString currentExpression = tooltipExpression();
        if (!currentExpression.isEmpty()) {
            int currentIndex = -1;
            for (int i = 0; i < toolTips.size(); ++i) {
                if (toolTips.at(i).first == currentExpression) {
                    currentIndex = i;
                    break;
                }
            }
            if (currentIndex < 0)
                toolTips.push_back(ExpressionInamePair(currentExpression, tooltipIName(currentExpression)));
        }

        foreach (const ExpressionInamePair &p, toolTips) {
            if (p.second.startsWith("tooltip")) {
                if (!watchers.isEmpty())
                    watchers += "##";
                watchers += p.first.toLatin1();
                watchers += '#';
                watchers += p.second;
            }
        }
    }

    QHash<QByteArray, int> watcherNames = handler->watcherNames();
    QHashIterator<QByteArray, int> it(watcherNames);
    while (it.hasNext()) {
        it.next();
        if (!watchers.isEmpty())
            watchers += "##";
        watchers += it.key() + "#watch." + QByteArray::number(it.value());
    }

    const static bool alwaysVerbose = !qgetenv("QTC_DEBUGGER_PYTHON_VERBOSE").isEmpty();
    QByteArray options;
    if (alwaysVerbose)
        options += "pe,";
    if (debuggerCore()->boolSetting(UseDebuggingHelpers))
        options += "fancy,";
    if (debuggerCore()->boolSetting(AutoDerefPointers))
        options += "autoderef,";
    if (debuggerCore()->boolSetting(UseDynamicType))
        options += "dyntype,";
    if (options.isEmpty())
        options += "defaults,";
    if (params.tryPartial)
        options += "partial,";
    if (params.tooltipOnly)
        options += "tooltiponly,";
    if (isAutoTestRunning())
        options += "autotest,";
    options.chop(1);

    QByteArray resultVar;
    if (!m_resultVarName.isEmpty())
        resultVar = "resultvarname:" + m_resultVarName + ' ';

    postCommand("bb options:" + options + " vars:" + params.varList + ' '
            + resultVar + expanded + " watchers:" + watchers.toHex(),
        Discardable, CB(handleStackFramePython), QVariant(params.tryPartial));
}

void GdbEngine::handleStackFramePython(const GdbResponse &response)
{
    PRECONDITION;
    if (response.resultClass == GdbResultDone) {
        const bool partial = response.cookie.toBool();
        QByteArray out = response.consoleStreamOutput;
        while (out.endsWith(' ') || out.endsWith('\n'))
            out.chop(1);
        int pos = out.indexOf("data=");
        if (pos != 0) {
            showMessage(_("DISCARDING JUNK AT BEGIN OF RESPONSE: "
                + out.left(pos)));
            out = out.mid(pos);
        }
        GdbMi all;
        all.fromStringMultiple(out);
        GdbMi data = all.findChild("data");

        WatchHandler *handler = watchHandler();
        QList<WatchData> list;

        if (!partial) {
            list.append(*handler->findData("local"));
            list.append(*handler->findData("watch"));
            list.append(*handler->findData("return"));
        }

        foreach (const GdbMi &child, data.children()) {
            WatchData dummy;
            dummy.iname = child.findChild("iname").data();
            GdbMi wname = child.findChild("wname");
            if (wname.isValid()) {
                // Happens (only) for watched expressions. They are encoded as
                // base64 encoded 8 bit data, without quotes
                dummy.name = decodeData(wname.data(), Base64Encoded8Bit);
                dummy.exp = dummy.name.toUtf8();
            } else {
                dummy.name = _(child.findChild("name").data());
            }
            parseWatchData(handler->expandedINames(), dummy, child, &list);
        }
        const GdbMi typeInfo = all.findChild("typeinfo");
        if (typeInfo.type() == GdbMi::List) {
            foreach (const GdbMi &s, typeInfo.children()) {
                const GdbMi name = s.findChild("name");
                const GdbMi size = s.findChild("size");
                if (name.isValid() && size.isValid())
                    m_typeInfoCache.insert(QByteArray::fromBase64(name.data()),
                                           TypeInfo(size.data().toUInt()));
            }
        }
        for (int i = 0; i != list.size(); ++i) {
            const TypeInfo ti = m_typeInfoCache.value(list.at(i).type);
            if (ti.size)
                list[i].size = ti.size;
        }

        handler->insertData(list);

        //PENDING_DEBUG("AFTER handleStackFrame()");
        // FIXME: This should only be used when updateLocals() was
        // triggered by expanding an item in the view.
        //if (m_pendingWatchRequests <= 0) {
            //PENDING_DEBUG("\n\n ....  AND TRIGGERS MODEL UPDATE\n");
            rebuildWatchModel();
        //}
        if (!partial)
            emit stackFrameCompleted();
    } else {
        showMessage(_("DUMPER FAILED: " + response.toString()));
    }
}

// Called from CoreAdapter and AttachAdapter
void GdbEngine::updateAllPython()
{
    PRECONDITION;
    //PENDING_DEBUG("UPDATING ALL\n");
    QTC_CHECK(state() == InferiorUnrunnable || state() == InferiorStopOk);
    reloadModulesInternal();
    postCommand("-stack-list-frames", CB(handleStackListFrames),
        QVariant::fromValue<StackCookie>(StackCookie(false, true)));
    stackHandler()->setCurrentIndex(0);
    postCommand("-thread-info", CB(handleThreadInfo), 0);
    reloadRegisters();
    updateLocals();
}

} // namespace Internal
} // namespace Debugger
