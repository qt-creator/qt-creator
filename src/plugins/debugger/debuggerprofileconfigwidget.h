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

#ifndef DEBUGGER_DEBUGGERPROFILECONFIGWIDGET_H
#define DEBUGGER_DEBUGGERPROFILECONFIGWIDGET_H

#include <projectexplorer/profileconfigwidget.h>

#include <QLabel>
#include <debuggerprofileinformation.h>

namespace ProjectExplorer { class Profile; }
namespace Utils { class PathChooser; }

namespace Debugger {
class DebuggerProfileInformation;

namespace Internal {
// -----------------------------------------------------------------------
// DebuggerProfileConfigWidget:
// -----------------------------------------------------------------------

class DebuggerProfileConfigWidget : public ProjectExplorer::ProfileConfigWidget
{
    Q_OBJECT

public:
    DebuggerProfileConfigWidget(ProjectExplorer::Profile *p,
                               const DebuggerProfileInformation *pi,
                               QWidget *parent = 0);

    QString displayName() const;

    void makeReadOnly();

    void apply();
    void discard();
    bool isDirty() const;

private slots:
    void autoDetectDebugger();

private:
    ProjectExplorer::Profile *m_profile;
    const DebuggerProfileInformation *m_info;
    Utils::PathChooser *m_chooser;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERPROFILEINFORMATION_H
