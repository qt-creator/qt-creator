// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/extracompiler.h>
#include <utils/fileutils.h>

namespace QtSupport {

class UicGenerator : public ProjectExplorer::ProcessExtraCompiler
{
    Q_OBJECT
public:
    UicGenerator(const ProjectExplorer::Project *project, const Utils::FilePath &source,
                 const Utils::FilePaths &targets, QObject *parent = nullptr);

protected:
    Utils::FilePath command() const override;
    QStringList arguments() const override;
    ProjectExplorer::FileNameToContentsHash handleProcessFinished(Utils::QtcProcess *process) override;
};

class UicGeneratorFactory : public ProjectExplorer::ExtraCompilerFactory
{
    Q_OBJECT
public:
    UicGeneratorFactory() = default;

    ProjectExplorer::FileType sourceType() const override;

    QString sourceTag() const override;

    ProjectExplorer::ExtraCompiler *create(const ProjectExplorer::Project *project,
                                           const Utils::FilePath &source,
                                           const Utils::FilePaths &targets) override;
};

} // QtSupport
