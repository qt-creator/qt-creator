// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/project.h>

namespace ProjectExplorer { class Kit; }

namespace CompilationDatabaseProjectManager::Internal {

class CompilationDatabaseProject final : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CompilationDatabaseProject(const Utils::FilePath &filename);

private:
    void configureAsExampleProject(ProjectExplorer::Kit *kit) final;
};

void setupCompilationDatabaseEditor();
void setupCompilationDatabaseBuildConfiguration();

} // CompilationDatabaseProjectManager::Internal
