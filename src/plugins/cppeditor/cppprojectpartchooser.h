// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cpptoolsreuse.h"
#include "projectpart.h"

#include <functional>

namespace ProjectExplorer { class Project; }

namespace CppEditor::Internal {

class ProjectPartChooser
{
public:
    using FallBackProjectPart = std::function<ProjectPart::ConstPtr()>;
    using ProjectPartsForFile = std::function<QList<ProjectPart::ConstPtr>(const QString &filePath)>;
    using ProjectPartsFromDependenciesForFile
        = std::function<QList<ProjectPart::ConstPtr>(const QString &filePath)>;

public:
    void setFallbackProjectPart(const FallBackProjectPart &getter);
    void setProjectPartsForFile(const ProjectPartsForFile &getter);
    void setProjectPartsFromDependenciesForFile(const ProjectPartsFromDependenciesForFile &getter);

    ProjectPartInfo choose(const QString &filePath,
            const ProjectPartInfo &currentProjectPartInfo,
            const QString &preferredProjectPartId,
            const Utils::FilePath &activeProject,
            Utils::Language languagePreference,
            bool projectsUpdated) const;

private:
    FallBackProjectPart m_fallbackProjectPart;
    ProjectPartsForFile m_projectPartsForFile;
    ProjectPartsFromDependenciesForFile m_projectPartsFromDependenciesForFile;
};

} // namespace CppEditor::Internal
