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

#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectexplorer_global.h>
#include <projectexplorer/projectmacro.h>

#include <coreplugin/id.h>

#include <utils/cpplanguage_details.h>

#include <cplusplus/Token.h>

#include <utils/cpplanguage_details.h>

#include <QString>
#include <QSharedPointer>

namespace ProjectExplorer {
class Project;
}

namespace CppTools {

class CPPTOOLS_EXPORT ProjectPart
{
public:
    enum QtVersion {
        UnknownQt = -1,
        NoQt,
        Qt4,
        Qt5
    };

    enum ToolChainWordWidth {
        WordWidth32Bit,
        WordWidth64Bit,
    };

    enum BuildTargetType {
        Unknown,
        Executable,
        Library
    };

    using Ptr = QSharedPointer<ProjectPart>;

public:
    QString id() const;
    QString projectFileLocation() const;

    Ptr copy() const;
    void updateLanguageFeatures();

    static QByteArray readProjectConfigFile(const Ptr &projectPart);

public:
    ProjectExplorer::Project *project = nullptr;

    QString displayName;

    QString projectFile;
    int projectFileLine = -1;
    int projectFileColumn = -1;
    QString callGroupId;

    // Versions, features and extensions
    ::Utils::Language language = ::Utils::Language::Cxx;
    ::Utils::LanguageVersion languageVersion = ::Utils::LanguageVersion::LatestCxx;
    ::Utils::LanguageExtensions languageExtensions = ::Utils::LanguageExtension::None;
    CPlusPlus::LanguageFeatures languageFeatures;
    QtVersion qtVersion = UnknownQt;

    // Files
    ProjectFiles files;
    QStringList precompiledHeaders;
    ProjectExplorer::HeaderPaths headerPaths;
    QString projectConfigFile; // Generic Project Manager only

    // Macros
    ProjectExplorer::Macros projectMacros;
    ProjectExplorer::Macros toolChainMacros;

    // Build system
    QString buildSystemTarget;
    BuildTargetType buildTargetType = Unknown;
    bool selectedForBuilding = true;

    // ToolChain
    Core::Id toolchainType;
    bool isMsvc2015Toolchain = false;
    QString toolChainTargetTriple;
    ToolChainWordWidth toolChainWordWidth = WordWidth32Bit;
    ProjectExplorer::WarningFlags warningFlags = ProjectExplorer::WarningFlags::Default;

    // Misc
    QStringList extraCodeModelFlags;
    QStringList compilerFlags;
};

} // namespace CppTools
