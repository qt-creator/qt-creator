/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
* version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEBUGGER_DEBUGGERKITINFORMATION_H
#define DEBUGGER_DEBUGGERKITINFORMATION_H

#include "debugger_global.h"
#include "debuggerconstants.h"

#include <projectexplorer/kitinformation.h>

namespace Debugger {

class DEBUGGER_EXPORT DebuggerKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT

public:
    class DEBUGGER_EXPORT DebuggerItem {
    public:
        DebuggerItem();
        DebuggerItem(DebuggerEngineType engineType, const Utils::FileName &fn);
        bool equals(const DebuggerItem &rhs) const { return engineType == rhs.engineType && binary == rhs.binary; }

        DebuggerEngineType engineType;
        Utils::FileName binary;
    };

    DebuggerKitInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    static DebuggerItem autoDetectItem(const ProjectExplorer::Kit *k);
    QVariant defaultValue(ProjectExplorer::Kit *k) const
        { return DebuggerKitInformation::itemToVariant(DebuggerKitInformation::autoDetectItem(k)); }

    QList<ProjectExplorer::Task> validate(ProjectExplorer::Kit *k) const
        { return DebuggerKitInformation::validateDebugger(k); }

    static QList<ProjectExplorer::Task> validateDebugger(const ProjectExplorer::Kit *p);
    static bool isValidDebugger(const ProjectExplorer::Kit *p);

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const;

    ItemList toUserOutput(ProjectExplorer::Kit *k) const;
    static QString userOutput(const DebuggerItem &item);

    static DebuggerItem debuggerItem(const ProjectExplorer::Kit *p);
    static void setDebuggerItem(ProjectExplorer::Kit *p, const DebuggerItem &item);

    static Utils::FileName debuggerCommand(const ProjectExplorer::Kit *p)
        { return debuggerItem(p).binary; }

    static void setDebuggerCommand(ProjectExplorer::Kit *p, const Utils::FileName &command);

    static DebuggerEngineType engineType(const ProjectExplorer::Kit *p)
        { return debuggerItem(p).engineType; }

    static void setEngineType(ProjectExplorer::Kit *p, DebuggerEngineType type);

    static QString debuggerEngineName(DebuggerEngineType t);

private:
    static DebuggerItem variantToItem(const QVariant &v);
    static QVariant itemToVariant(const DebuggerItem &i);
};

inline bool operator==(const DebuggerKitInformation::DebuggerItem &i1, const DebuggerKitInformation::DebuggerItem &i2)
    { return i1.equals(i2); }
inline bool operator!=(const DebuggerKitInformation::DebuggerItem &i1, const DebuggerKitInformation::DebuggerItem &i2)
    { return !i1.equals(i2); }

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERKITINFORMATION_H
