// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "workspaceinputdialog.h"
#include "workspacemodel.h"

#include <utils/expected.h>
#include <utils/itemviews.h>

namespace ADS {

class DockManager;
class WorkspaceDialog;

class WorkspaceView : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit WorkspaceView(DockManager *manager, QWidget *parent = nullptr);

    void createNewWorkspace();
    void cloneCurrentWorkspace();
    void renameCurrentWorkspace();
    void resetCurrentWorkspace();
    void switchToCurrentWorkspace();
    void deleteSelectedWorkspaces();

    void importWorkspace();
    void exportCurrentWorkspace();

    void moveWorkspaceUp();
    void moveWorkspaceDown();

    QString currentWorkspace();
    WorkspaceModel *workspaceModel();
    void selectActiveWorkspace();
    void selectWorkspace(const QString &fileName);

    QStringList selectedWorkspaces() const;

signals:
    void workspaceActivated(const QString &fileName);
    void workspacesSelected(const QStringList &fileNames);
    void workspaceSwitched();

private:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void deleteWorkspaces(const QStringList &fileNames);

    bool confirmWorkspaceDelete(const QStringList &fileNames);

    void runWorkspaceNameInputDialog(
        WorkspaceNameInputDialog *workspaceInputDialog,
        std::function<Utils::expected_str<QString>(const QString &)> callback);

    DockManager *m_manager;
    WorkspaceModel m_workspaceModel;
};

} // namespace ADS
