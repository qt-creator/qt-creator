// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoitemsscanner.h"

namespace ProjectExplorer { class Project; }

namespace Todo {
namespace Internal {

// Scans qmake and CMake project files for todo keywords. Such files have no
// code model, so the model-based scanners never look at them.

class ProjectFileTodoItemsScanner : public TodoItemsScanner
{
    Q_OBJECT

public:
    explicit ProjectFileTodoItemsScanner(const KeywordList &keywordList, QObject *parent = nullptr);

protected:
    void scannerParamsChanged() override;

private:
    void scanAllProjects();
    void scanProject(ProjectExplorer::Project *project);
    void processFile(const Utils::FilePath &filePath);
    static bool isProjectFile(const Utils::FilePath &filePath);
};

} // namespace Internal
} // namespace Todo
