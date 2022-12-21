// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectinfo.h"

#include <QFutureInterface>

namespace CppEditor::Internal {

class ProjectInfoGenerator
{
public:
    ProjectInfoGenerator(const QFutureInterface<ProjectInfo::ConstPtr> &futureInterface,
                         const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo);

    ProjectInfo::ConstPtr generate();

private:
    const QVector<ProjectPart::ConstPtr> createProjectParts(
            const ProjectExplorer::RawProjectPart &rawProjectPart,
            const Utils::FilePath &projectFilePath);
    ProjectPart::ConstPtr createProjectPart(const Utils::FilePath &projectFilePath,
                                       const ProjectExplorer::RawProjectPart &rawProjectPart,
                                       const ProjectFiles &projectFiles,
                                       const QString &partName,
                                       Utils::Language language,
                                       Utils::LanguageExtensions languageExtensions);

private:
    const QFutureInterface<ProjectInfo::ConstPtr> m_futureInterface;
    const ProjectExplorer::ProjectUpdateInfo &m_projectUpdateInfo;
    bool m_cToolchainMissing = false;
    bool m_cxxToolchainMissing = false;
};

} // namespace CppEditor::Internal
