// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectinfo.h"

QT_BEGIN_NAMESPACE
template <class T>
class QPromise;
QT_END_NAMESPACE

namespace CppEditor::Internal {

class ProjectInfoGenerator
{
public:
    ProjectInfoGenerator(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo);

    ProjectInfo::ConstPtr generate(const QPromise<ProjectInfo::ConstPtr> &promise);

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
    const ProjectExplorer::ProjectUpdateInfo &m_projectUpdateInfo;
    bool m_cToolchainMissing = false;
    bool m_cxxToolchainMissing = false;
};

} // namespace CppEditor::Internal
