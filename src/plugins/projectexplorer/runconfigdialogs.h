// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "runconfiguration.h"

#include <QDialog>

namespace Utils { class TreeView; }

namespace ProjectExplorer::Internal {

class AddRunConfigDialog : public QDialog
{
    Q_OBJECT
public:
    AddRunConfigDialog(BuildConfiguration *bc, QWidget *parent);

    RunConfigurationCreationInfo creationInfo() const { return m_creationInfo; }

private:
    void accept() override;

    Utils::TreeView * const m_view;
    RunConfigurationCreationInfo m_creationInfo;
};

class RemoveRunConfigsDialog : public QDialog
{
    Q_OBJECT
public:
    RemoveRunConfigsDialog(BuildConfiguration *bc, QWidget *parent);

    QList<RunConfiguration *> runConfigsToRemove() const { return m_runConfigs; }

private:
    void accept() override;

    Utils::TreeView * const m_view;
    QList<RunConfiguration *> m_runConfigs;
};

} // namespace ProjectExplorer::Internal
