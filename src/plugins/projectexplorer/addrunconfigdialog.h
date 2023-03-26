// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "runconfiguration.h"

#include <QDialog>

namespace Utils { class TreeView; }

namespace ProjectExplorer {
class Target;

namespace Internal {

class AddRunConfigDialog : public QDialog
{
    Q_OBJECT
public:
    AddRunConfigDialog(Target *target, QWidget *parent);

    RunConfigurationCreationInfo creationInfo() const { return m_creationInfo; }

private:
    void accept() override;

    Utils::TreeView * const m_view;
    RunConfigurationCreationInfo m_creationInfo;
};

} // namespace Internal
} // namespace ProjectExplorer
