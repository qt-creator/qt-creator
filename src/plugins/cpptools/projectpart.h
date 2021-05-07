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

#include "cppprojectfile.h"

#include <projectexplorer/buildtargettype.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/rawprojectpart.h>

#include <cplusplus/Token.h>

#include <utils/cpplanguage_details.h>
#include <utils/fileutils.h>
#include <utils/id.h>

#include <QString>
#include <QSharedPointer>

namespace ProjectExplorer { class Project; }

namespace CppTools {

class CPPTOOLS_EXPORT ProjectPart
{
public:
    enum ToolChainWordWidth {
        WordWidth32Bit,
        WordWidth64Bit,
    };

    using Ptr = QSharedPointer<ProjectPart>;

public:
    static Ptr create(const Utils::FilePath &topLevelProject,
                      const ProjectExplorer::RawProjectPart &rpp = {},
                      const QString &displayName = {},
                      const ProjectFiles &files = {},
                      Utils::Language language = Utils::Language::Cxx,
                      Utils::LanguageExtensions languageExtensions = {},
                      const ProjectExplorer::RawProjectPartFlags &flags = {},
                      const ProjectExplorer::ToolChainInfo &tcInfo = {})
    {
        return Ptr(new ProjectPart(topLevelProject, rpp, displayName, files, language,
                                   languageExtensions, flags, tcInfo));
    }

    QString id() const;
    QString projectFileLocation() const;
    bool hasProject() const { return !topLevelProject.isEmpty(); }
    bool belongsToProject(const ProjectExplorer::Project *project) const;

    static QByteArray readProjectConfigFile(const QString &projectConfigFile);

public:
    const Utils::FilePath topLevelProject;
    const QString displayName;
    const QString projectFile;
    const QString projectConfigFile; // Generic Project Manager only

    const int projectFileLine = -1;
    const int projectFileColumn = -1;
    const QString callGroupId;

    // Versions, features and extensions
    const Utils::Language language = Utils::Language::Cxx;
    const Utils::LanguageVersion &languageVersion = m_macroReport.languageVersion;
    const Utils::LanguageExtensions languageExtensions = Utils::LanguageExtension::None;
    const Utils::QtVersion qtVersion = Utils::QtVersion::Unknown;

    // Files
    const ProjectFiles files;
    const QStringList includedFiles;
    const QStringList precompiledHeaders;
    const ProjectExplorer::HeaderPaths headerPaths;

    // Macros
    const ProjectExplorer::Macros projectMacros;
    const ProjectExplorer::Macros &toolChainMacros = m_macroReport.macros;

    // Build system
    const QString buildSystemTarget;
    const ProjectExplorer::BuildTargetType buildTargetType
        = ProjectExplorer::BuildTargetType::Unknown;
    const bool selectedForBuilding = true;

    // ToolChain
    const Utils::Id toolchainType;
    const bool isMsvc2015Toolchain = false;
    const QString toolChainTargetTriple;
    const ToolChainWordWidth toolChainWordWidth = WordWidth32Bit;
    const Utils::FilePath toolChainInstallDir;
    const Utils::FilePath compilerFilePath;
    const Utils::WarningFlags warningFlags = Utils::WarningFlags::Default;

    // Misc
    const QStringList extraCodeModelFlags;
    const QStringList compilerFlags;

private:
    ProjectPart(const Utils::FilePath &topLevelProject,
                const ProjectExplorer::RawProjectPart &rpp,
                const QString &displayName,
                const ProjectFiles &files,
                Utils::Language language,
                Utils::LanguageExtensions languageExtensions,
                const ProjectExplorer::RawProjectPartFlags &flags,
                const ProjectExplorer::ToolChainInfo &tcInfo);

    CPlusPlus::LanguageFeatures deriveLanguageFeatures() const;

    const ProjectExplorer::ToolChain::MacroInspectionReport m_macroReport;

public:
    // Must come last due to initialization order.
    const CPlusPlus::LanguageFeatures languageFeatures;
};

} // namespace CppTools
