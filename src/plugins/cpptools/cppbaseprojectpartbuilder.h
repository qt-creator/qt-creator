/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"

#include "cppprojectinterface.h"
#include "projectpart.h"

#include <functional>
#include <memory>

namespace CppTools {

class ProjectInfo;

class CPPTOOLS_EXPORT BaseProjectPartBuilder
{
public:
    BaseProjectPartBuilder(ProjectInterface *project, ProjectInfo &projectInfo);

    void setDisplayName(const QString &displayName);

    void setProjectFile(const QString &projectFile);
    void setConfigFileName(const QString &configFileName);

    void setQtVersion(ProjectPart::QtVersion qtVersion);

    void setCFlags(const QStringList &flags);
    void setCxxFlags(const QStringList &flags);

    void setDefines(const QByteArray &defines);
    void setHeaderPaths(const ProjectPartHeaderPaths &headerPaths);
    void setIncludePaths(const QStringList &includePaths);

    void setPreCompiledHeaders(const QStringList &preCompiledHeaders);

    void setSelectedForBuilding(bool yesno);

    using FileClassifier = std::function<ProjectFile::Kind (const QString &filePath)>;
    QList<Core::Id> createProjectPartsForFiles(const QStringList &filePaths,
                                               FileClassifier fileClassifier = FileClassifier());

private:
    void createProjectPart(const ProjectFiles &projectFiles,
                           const QString &partName,
                           ProjectPart::LanguageVersion languageVersion,
                           ProjectPart::LanguageExtensions languageExtensions);
    ToolChainInterfacePtr selectToolChain(ProjectPart::LanguageVersion languageVersion);

private:
    std::unique_ptr<ProjectInterface> m_project;
    ProjectInfo &m_projectInfo;

    ProjectPart::Ptr m_templatePart;
    QStringList m_cFlags;
    QStringList m_cxxFlags;
};

} // namespace CppTools
