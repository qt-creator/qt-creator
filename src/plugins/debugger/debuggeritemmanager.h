/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef DEBUGGER_DEBUGGERITEMMANAGER_H
#define DEBUGGER_DEBUGGERITEMMANAGER_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QCoreApplication>

namespace Utils { class FileName; }

namespace Debugger {
class DebuggerItem;

// -----------------------------------------------------------------------
// DebuggerItemManager
// -----------------------------------------------------------------------

class DEBUGGER_EXPORT DebuggerItemManager : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerItemManager)
public:
    DebuggerItemManager();
    ~DebuggerItemManager();

    static QList<DebuggerItem> debuggers();

    static QVariant registerDebugger(const DebuggerItem &item);
    static void deregisterDebugger(const QVariant &id);

    static DebuggerItem *findByCommand(const Utils::FileName &command);
    static const DebuggerItem *findById(const QVariant &id);
    static const DebuggerItem *findByEngineType(DebuggerEngineType engineType);

    static void restoreDebuggers();
    static QString uniqueDisplayName(const QString &base);

    static void updateOrAddDebugger(const DebuggerItem &item);
    static void saveDebuggers();

private:
    static void autoDetectGdbOrLldbDebuggers();
    static void autoDetectCdbDebuggers();
    static void readLegacyDebuggers(const Utils::FileName &file);
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERITEMMANAGER_H
