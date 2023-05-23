// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace ADS {

class DockManager;
class WorkspaceView;

class WorkspaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorkspaceDialog(DockManager *manager, QWidget *parent = nullptr);

    void setAutoLoadWorkspace(bool);
    bool autoLoadWorkspace() const;

    DockManager *dockManager() const;

private:
    void updateActions(const QStringList &fileNames);

    DockManager *m_manager = nullptr;

    WorkspaceView *m_workspaceView = nullptr;
    QPushButton *m_btCreateNew = nullptr;
    QPushButton *m_btRename = nullptr;
    QPushButton *m_btClone = nullptr;
    QPushButton *m_btDelete = nullptr;
    QPushButton *m_btReset = nullptr;
    QPushButton *m_btSwitch = nullptr;
    QPushButton *m_btImport = nullptr;
    QPushButton *m_btExport = nullptr;
    QPushButton *m_btUp = nullptr;
    QPushButton *m_btDown = nullptr;
    QCheckBox *m_autoLoadCheckBox = nullptr;
};

} // namespace ADS
