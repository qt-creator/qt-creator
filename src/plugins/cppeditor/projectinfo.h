// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppcodemodelsettings.h"
#include "projectpart.h"

#include <projectexplorer/project.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/toolchain.h>

#include <utils/filepath.h>

#include <QHash>
#include <QList>
#include <QSet>
#include <QVector>

#include <memory>

namespace CppEditor {

class CPPEDITOR_EXPORT ProjectInfo
{
public:
    using ConstPtr = std::shared_ptr<const ProjectInfo>;
    static ConstPtr create(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                           const QVector<ProjectPart::ConstPtr> &projectParts);
    static ConstPtr cloneWithNewSettings(const ProjectInfo::ConstPtr &pi,
                                         const CppCodeModelSettings &settings);

    const QVector<ProjectPart::ConstPtr> &projectParts() const { return m_projectParts; }
    const QSet<Utils::FilePath> &sourceFiles() const { return m_sourceFiles; }
    QString projectName() const { return m_projectName; }
    Utils::FilePath projectFilePath() const { return m_projectFilePath; }
    ProjectExplorer::Project *project() const;
    Utils::FilePath projectRoot() const { return m_projectFilePath.parentDir(); }
    Utils::FilePath buildRoot() const { return m_buildRoot; }
    const CppCodeModelSettings &settings() const { return m_settings; }

    // Comparisons
    bool operator ==(const ProjectInfo &other) const;
    bool operator !=(const ProjectInfo &other) const;
    bool definesChanged(const ProjectInfo &other) const;
    bool configurationChanged(const ProjectInfo &other) const;
    bool configurationOrFilesChanged(const ProjectInfo &other) const;

private:
    ProjectInfo(const ProjectExplorer::ProjectUpdateInfo &updateInfo,
                const QVector<ProjectPart::ConstPtr> &projectParts);
    ProjectInfo(const ProjectInfo::ConstPtr &pi, const CppCodeModelSettings &settings);

    const QVector<ProjectPart::ConstPtr> m_projectParts;
    const QString m_projectName;
    const Utils::FilePath m_projectFilePath;
    const Utils::FilePath m_buildRoot;
    const ProjectExplorer::HeaderPaths m_headerPaths;
    const QSet<Utils::FilePath> m_sourceFiles;
    const ProjectExplorer::Macros m_defines;
    const CppCodeModelSettings m_settings;
};

using ProjectInfoList = QList<ProjectInfo::ConstPtr>;

} // namespace CppEditor
