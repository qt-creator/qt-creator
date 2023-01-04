// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ui_workspacedialog.h"

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace ADS {

class DockManager;

class WorkspaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorkspaceDialog(DockManager *manager, QWidget *parent = nullptr);

    void setAutoLoadWorkspace(bool);
    bool autoLoadWorkspace() const;

    DockManager *dockManager() const;

private:
    void updateActions(const QStringList &workspaces);

    Ui::WorkspaceDialog m_ui;

    DockManager *m_manager = nullptr;
};

class WorkspaceNameInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorkspaceNameInputDialog(DockManager *manager, QWidget *parent);

    void setActionText(const QString &actionText, const QString &openActionText);
    void setValue(const QString &value);
    QString value() const;
    bool isSwitchToRequested() const;

private:
    QLineEdit *m_newWorkspaceLineEdit = nullptr;
    QPushButton *m_switchToButton = nullptr;
    QPushButton *m_okButton = nullptr;
    bool m_usedSwitchTo = false;

    DockManager *m_manager;
};

} // namespace ADS
