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

    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *k) const
        { return DebuggerKitInformation::validateDebugger(k); }

    void setup(ProjectExplorer::Kit *k);

    static QList<ProjectExplorer::Task> validateDebugger(const ProjectExplorer::Kit *k);
    static bool isValidDebugger(const ProjectExplorer::Kit *k);

    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *k) const;

    ItemList toUserOutput(ProjectExplorer::Kit *k) const;
    static QString userOutput(const DebuggerItem &item);

    static DebuggerItem debuggerItem(const ProjectExplorer::Kit *k);
    static void setDebuggerItem(ProjectExplorer::Kit *k, const DebuggerItem &item);

    static Utils::FileName debuggerCommand(const ProjectExplorer::Kit *k)
        { return debuggerItem(k).binary; }

    static void setDebuggerCommand(ProjectExplorer::Kit *k, const Utils::FileName &command);

    static DebuggerEngineType engineType(const ProjectExplorer::Kit *k)
        { return debuggerItem(k).engineType; }

    static void setEngineType(ProjectExplorer::Kit *k, DebuggerEngineType type);

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
