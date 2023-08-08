// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/extracompiler.h>
#include <utils/filepath.h>

namespace QtSupport {

class UicGeneratorFactory : public ProjectExplorer::ExtraCompilerFactory
{
public:
    UicGeneratorFactory() = default;

    ProjectExplorer::FileType sourceType() const override;

    QString sourceTag() const override;

    ProjectExplorer::ExtraCompiler *create(const ProjectExplorer::Project *project,
                                           const Utils::FilePath &source,
                                           const Utils::FilePaths &targets) override;
};

} // QtSupport
