// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compileroptionsbuilder.h"
#include "projectpart.h"

#include <utils/filepath.h>

namespace CppEditor::Internal {

class HeaderPathFilter
{
public:
    HeaderPathFilter(const ProjectPart &projectPart,
                     UseTweakedHeaderPaths useTweakedHeaderPaths = UseTweakedHeaderPaths::Yes,
                     const Utils::FilePath &clangIncludeDirectory = {},
                     const QString &projectDirectory = {},
                     const QString &buildDirectory = {})
        : projectPart{projectPart}
        , clangIncludeDirectory{clangIncludeDirectory}
        , projectDirectory(ensurePathWithSlashEnding(projectDirectory))
        , buildDirectory(ensurePathWithSlashEnding(buildDirectory))
        , useTweakedHeaderPaths{useTweakedHeaderPaths}
    {}

    void process();

private:
    void filterHeaderPath(const ProjectExplorer::HeaderPath &headerPath);

    void tweakHeaderPaths();

    void addPreIncludesPath();

    bool isProjectHeaderPath(const QString &path) const;

    void removeGccInternalIncludePaths();

    static QString ensurePathWithSlashEnding(const QString &path);

public:
    ProjectExplorer::HeaderPaths builtInHeaderPaths;
    ProjectExplorer::HeaderPaths systemHeaderPaths;
    ProjectExplorer::HeaderPaths userHeaderPaths;
    const ProjectPart &projectPart;
    const Utils::FilePath clangIncludeDirectory;
    const QString projectDirectory;
    const QString buildDirectory;
    const UseTweakedHeaderPaths useTweakedHeaderPaths;
};

} // namespace CppEditor::Internal
