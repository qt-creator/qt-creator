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

#ifndef MAEMORUNCONFIGURATIONWIDGET_H
#define MAEMORUNCONFIGURATIONWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QTableView;
class QToolButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace RemoteLinux {
class RemoteLinuxRunConfigurationWidget;
}

namespace Madde {
namespace Internal {
class MaemoRunConfiguration;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaemoRunConfigurationWidget(MaemoRunConfiguration *runConfiguration,
        QWidget *parent = 0);

private slots:
    void addMount();
    void removeMount();
    void changeLocalMountDir(const QModelIndex &index);
    void enableOrDisableRemoveMountSpecButton();
    void handleRemoteMountsChanged();
    void updateMountWarning();
    void runConfigurationEnabledChange();

private:
    void addMountWidgets(QVBoxLayout *mainLayout);

    QWidget *m_subWidget;
    QLabel *m_mountWarningLabel;
    QTableView *m_mountView;
    QToolButton *m_removeMountButton;
    Utils::DetailsWidget *m_mountDetailsContainer;
    RemoteLinux::RemoteLinuxRunConfigurationWidget *m_remoteLinuxRunConfigWidget;
    MaemoRunConfiguration *m_runConfiguration;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMORUNCONFIGURATIONWIDGET_H
