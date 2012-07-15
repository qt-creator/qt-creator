/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_DEBUGGERPROFILEINFORMATION_H
#define DEBUGGER_DEBUGGERPROFILEINFORMATION_H

#include "debugger_global.h"

#include <projectexplorer/profileinformation.h>

namespace Debugger {

class DEBUGGER_EXPORT DebuggerProfileInformation : public ProjectExplorer::ProfileInformation
{
    Q_OBJECT

public:
    DebuggerProfileInformation();

    Core::Id dataId() const;

    unsigned int priority() const; // the higher the closer to the top.

    QVariant defaultValue(ProjectExplorer::Profile *p) const;

    QList<ProjectExplorer::Task> validate(ProjectExplorer::Profile *p) const;

    ProjectExplorer::ProfileConfigWidget *createConfigWidget(ProjectExplorer::Profile *p) const;

    ItemList toUserOutput(ProjectExplorer::Profile *p) const;

    static Utils::FileName debuggerCommand(const ProjectExplorer::Profile *p);
    static void setDebuggerCommand(ProjectExplorer::Profile *p, const Utils::FileName &command);
};

} // namespace Debugger

#endif // DEBUGGER_DEBUGGERPROFILEINFORMATION_H
