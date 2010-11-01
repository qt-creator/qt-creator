/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CUSTOMEXECUTABLECONFIGURATIONWIDGET_H
#define CUSTOMEXECUTABLECONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
class DebuggerLanguageChooser;
}

namespace ProjectExplorer {
class CustomExecutableRunConfiguration;
class EnvironmentWidget;

namespace Internal {

class CustomExecutableConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc);

private slots:
    void changed();

    void executableEdited();
    void argumentsEdited(const QString &arguments);
    void workingDirectoryEdited();
    void termToggled(bool);

    void userChangesChanged();
    void baseEnvironmentChanged();
    void userEnvironmentChangesChanged();
    void baseEnvironmentSelected(int index);
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);
    void qmlDebugServerPortChanged(uint port);

private:
    bool m_ignoreChange;
    CustomExecutableRunConfiguration *m_runConfiguration;
    Utils::PathChooser *m_executableChooser;
    QLineEdit *m_userName;
    QLineEdit *m_commandLineArgumentsLineEdit;
    Utils::PathChooser *m_workingDirectory;
    QCheckBox *m_useTerminalCheck;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    QComboBox *m_baseEnvironmentComboBox;
    Utils::DetailsWidget *m_detailsContainer;
    Utils::DebuggerLanguageChooser *m_debuggerLanguageChooser;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMEXECUTABLECONFIGURATIONWIDGET_H
