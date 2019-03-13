/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "builddependency.h"

#include <compilermacro.h>
#include <filepath.h>
#include <includesearchpath.h>
#include <projectpartid.h>

#include <utils/smallstringvector.h>
#include <utils/cpplanguage_details.h>

namespace ClangBackEnd {

class PchTask
{
public:
    PchTask(ProjectPartId projectPartId,
            FilePathIds &&includes,
            FilePathIds &&sources,
            CompilerMacros &&compilerMacros,
            Utils::SmallStringVector &&usedMacros, // TODO remove
            Utils::SmallStringVector toolChainArguments,
            IncludeSearchPaths systemIncludeSearchPaths,
            IncludeSearchPaths projectIncludeSearchPaths,
            Utils::Language language = Utils::Language::Cxx,
            Utils::LanguageVersion languageVersion = Utils::LanguageVersion::CXX98,
            Utils::LanguageExtension languageExtension = Utils::LanguageExtension::None)
        : projectPartIds({projectPartId})
        , includes(includes)
        , sources(sources)
        , compilerMacros(compilerMacros)
        , systemIncludeSearchPaths(std::move(systemIncludeSearchPaths))
        , projectIncludeSearchPaths(std::move(projectIncludeSearchPaths))
        , toolChainArguments(std::move(toolChainArguments))
        , language(language)
        , languageVersion(languageVersion)
        , languageExtension(languageExtension)
    {}

    PchTask(ProjectPartIds &&projectPartIds,
            FilePathIds &&includes,
            FilePathIds &&sources,
            CompilerMacros &&compilerMacros,
            Utils::SmallStringVector &&usedMacros, // TODO remove
            Utils::SmallStringVector toolChainArguments,
            IncludeSearchPaths systemIncludeSearchPaths,
            IncludeSearchPaths projectIncludeSearchPaths,
            Utils::Language language = Utils::Language::Cxx,
            Utils::LanguageVersion languageVersion = Utils::LanguageVersion::CXX98,
            Utils::LanguageExtension languageExtension = Utils::LanguageExtension::None)
        : projectPartIds(std::move(projectPartIds))
        , includes(includes)
        , sources(sources)
        , compilerMacros(compilerMacros)
        , systemIncludeSearchPaths(std::move(systemIncludeSearchPaths))
        , projectIncludeSearchPaths(std::move(projectIncludeSearchPaths))
        , toolChainArguments(std::move(toolChainArguments))
        , language(language)
        , languageVersion(languageVersion)
        , languageExtension(languageExtension)
    {}

    friend bool operator==(const PchTask &first, const PchTask &second)
    {
        return first.systemPchPath == second.systemPchPath
               && first.projectPartIds == second.projectPartIds && first.includes == second.includes
               && first.compilerMacros == second.compilerMacros
               && first.systemIncludeSearchPaths == second.systemIncludeSearchPaths
               && first.projectIncludeSearchPaths == second.projectIncludeSearchPaths
               && first.toolChainArguments == second.toolChainArguments
               && first.language == second.language
               && first.languageVersion == second.languageVersion
               && first.languageExtension == second.languageExtension;
    }

    ProjectPartId projectPartId() const { return projectPartIds.front(); }

public:
    FilePath systemPchPath;
    ProjectPartIds projectPartIds;
    FilePathIds includes;
    FilePathIds sources;
    CompilerMacros compilerMacros;
    IncludeSearchPaths systemIncludeSearchPaths;
    IncludeSearchPaths projectIncludeSearchPaths;
    Utils::SmallStringVector toolChainArguments;
    Utils::Language language = Utils::Language::Cxx;
    Utils::LanguageVersion languageVersion = Utils::LanguageVersion::CXX98;
    Utils::LanguageExtension languageExtension = Utils::LanguageExtension::None;
    bool isMerged = false;
};

class PchTaskSet
{
public:
    PchTaskSet(PchTask &&system, PchTask &&project)
        : system(std::move(system))
        , project(std::move(project))
    {}

public:
    PchTask system;
    PchTask project;
};

using PchTasks = std::vector<PchTask>;
using PchTaskSets = std::vector<PchTaskSet>;
} // namespace ClangBackEnd
