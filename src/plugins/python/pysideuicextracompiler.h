// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/extracompiler.h>

namespace Python::Internal {

class PySideUicExtraCompiler : public ProjectExplorer::ProcessExtraCompiler
{
public:
    PySideUicExtraCompiler(const Utils::FilePath &pySideUic,
                           const ProjectExplorer::Project *project,
                           const Utils::FilePath &source,
                           const Utils::FilePaths &targets,
                           QObject *parent = nullptr);

    Utils::FilePath pySideUicPath() const;

private:
    Utils::FilePath command() const override;
    ProjectExplorer::FileNameToContentsHash handleProcessFinished(
        Utils::Process *process) override;

    Utils::FilePath m_pySideUic;
};

} // Python::Internal
