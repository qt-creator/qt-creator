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
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef DEBUGGER_DEBUGGERKITCONFIGWIDGET_H
#define DEBUGGER_DEBUGGERKITCONFIGWIDGET_H

#include <projectexplorer/kitconfigwidget.h>

#include <QLabel>
#include <debuggerkitinformation.h>

namespace ProjectExplorer { class Kit; }
namespace Utils { class PathChooser; }

namespace Debugger {
class DebuggerKitInformation;

namespace Internal {
// -----------------------------------------------------------------------
// DebuggerKitConfigWidget:
// -----------------------------------------------------------------------

class DebuggerKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    DebuggerKitConfigWidget(ProjectExplorer::Kit *p,
                            const DebuggerKitInformation *ki,
                            QWidget *parent = 0);

    QString displayName() const;

    void makeReadOnly();

    void apply();
    void discard();
    bool isDirty() const;
    QWidget *buttonWidget() const;

private slots:
    void autoDetectDebugger();

private:
    ProjectExplorer::Kit *m_kit;
    const DebuggerKitInformation *m_info;
    Utils::PathChooser *m_chooser;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGGERKITINFORMATION_H
