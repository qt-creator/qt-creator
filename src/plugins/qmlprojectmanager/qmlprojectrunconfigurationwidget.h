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

#ifndef QMLPROJECTRUNCONFIGURATIONWIDGET_H
#define QMLPROJECTRUNCONFIGURATIONWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QStandardItemModel);

namespace ProjectExplorer {

class EnvironmentWidget;

} // namespace Qt4ProjectManager

namespace QmlProjectManager {

class QmlProjectRunConfiguration;

namespace Internal {

const char * const CURRENT_FILE  = QT_TRANSLATE_NOOP("QmlManager", "<Current File>");

class QmlProjectRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc);

public slots:
    void updateQtVersionComboBox();
    void userEnvironmentChangesChanged();
    void updateFileComboBox();

private slots:

    void setMainScript(int index);
    void onQtVersionSelectionChanged();
    void onViewerArgsChanged();
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);
    void qmlDebugServerPortChanged(uint port);

    void userChangesChanged();

    void manageQtVersions();

private:
    QmlProjectRunConfiguration *m_runConfiguration;

    QComboBox *m_qtVersionComboBox;
    QComboBox *m_fileListCombo;
    QStandardItemModel *m_fileListModel;

    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONFIGURATIONWIDGET_H
