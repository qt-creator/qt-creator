/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "workspacemodel.h"

#include <utils/itemviews.h>

#include <QAbstractTableModel>

namespace ADS {

class DockManager;
class WorkspaceDialog;

class WorkspaceView : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit WorkspaceView(QWidget *parent = nullptr);

    void createNewWorkspace();
    void deleteSelectedWorkspaces();
    void cloneCurrentWorkspace();
    void renameCurrentWorkspace();
    void resetCurrentWorkspace();
    void switchToCurrentWorkspace();

    void importWorkspace();
    void exportCurrentWorkspace();

    QString currentWorkspace();
    WorkspaceModel *workspaceModel();
    void selectActiveWorkspace();
    void selectWorkspace(const QString &workspaceName);

    QStringList selectedWorkspaces() const;

signals:
    void activated(const QString &workspace);
    void selected(const QStringList &workspaces);
    void workspaceSwitched();

private:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void deleteWorkspaces(const QStringList &workspaces);

    static WorkspaceDialog *castToWorkspaceDialog(QWidget *widget);

    DockManager *m_manager;
    WorkspaceModel m_workspaceModel;
};

} // namespace ADS
