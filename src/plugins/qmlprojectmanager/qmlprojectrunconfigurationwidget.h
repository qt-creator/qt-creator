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

#ifndef QMLPROJECTRUNCONFIGURATIONWIDGET_H
#define QMLPROJECTRUNCONFIGURATIONWIDGET_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QStandardItemModel)

namespace ProjectExplorer {

class EnvironmentWidget;

} // namespace Qt4ProjectManager

namespace QmlProjectManager {

class QmlProjectRunConfiguration;

namespace Internal {

const char CURRENT_FILE[]  = QT_TRANSLATE_NOOP("QmlManager", "<Current File>");

class QmlProjectRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProjectRunConfigurationWidget(QmlProjectRunConfiguration *rc);

public slots:
    void userEnvironmentChangesChanged();
    void updateFileComboBox();

private slots:
    void setMainScript(int index);
    void onViewerArgsChanged();
    void userChangesChanged();

private:
    QmlProjectRunConfiguration *m_runConfiguration;

    QComboBox *m_fileListCombo;
    QStandardItemModel *m_fileListModel;

    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONFIGURATIONWIDGET_H
