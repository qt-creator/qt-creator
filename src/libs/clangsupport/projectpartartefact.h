/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangsupport_global.h"
#include "clangsupportexceptions.h"
#include "projectpartid.h"

#include <utils/cpplanguage_details.h>
#include <utils/smallstringvector.h>

#include <compilermacro.h>
#include <includesearchpath.h>

QT_FORWARD_DECLARE_CLASS(QJsonDocument)
QT_FORWARD_DECLARE_STRUCT(QJsonParseError)

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT ProjectPartArtefact
{
public:
    ProjectPartArtefact() = default;
    ProjectPartArtefact(ProjectPartId projectPartId,
                        Utils::SmallStringVector &&toolChainArguments,
                        CompilerMacros &&compilerMacros,
                        IncludeSearchPaths &&systemIncludeSearchPaths,
                        IncludeSearchPaths &&projectIncludeSearchPaths,
                        Utils::Language language,
                        Utils::LanguageVersion languageVersion,
                        Utils::LanguageExtension languageExtension)
        : projectPartId(projectPartId)
        , toolChainArguments(std::move(toolChainArguments))
        , compilerMacros(std::move(compilerMacros))
        , systemIncludeSearchPaths(std::move(systemIncludeSearchPaths))
        , projectIncludeSearchPaths(std::move(projectIncludeSearchPaths))
        , language(language)
        , languageVersion(languageVersion)
        , languageExtension(languageExtension)
    {}

    ProjectPartArtefact(Utils::SmallStringView compilerArgumentsText,
                        Utils::SmallStringView compilerMacrosText,
                        Utils::SmallStringView systemIncludeSearchPathsText,
                        Utils::SmallStringView projectIncludeSearchPathsText,
                        int projectPartId,
                        int language,
                        int languageVersion,
                        int languageExtension)
        : projectPartId(projectPartId)
        , toolChainArguments(toStringVector(compilerArgumentsText))
        , compilerMacros(toCompilerMacros(compilerMacrosText))
        , systemIncludeSearchPaths(toIncludeSearchPaths(systemIncludeSearchPathsText))
        , projectIncludeSearchPaths(toIncludeSearchPaths(projectIncludeSearchPathsText))
        , language(static_cast<Utils::Language>(language))
        , languageVersion(static_cast<Utils::LanguageVersion>(languageVersion))
        , languageExtension(static_cast<Utils::LanguageExtension>(languageExtension))
    {}

    ProjectPartArtefact(Utils::SmallStringView compilerArgumentsText,
                        Utils::SmallStringView compilerMacrosText,
                        Utils::SmallStringView systemIncludeSearchPathsText,
                        Utils::SmallStringView projectIncludeSearchPathsText,
                        int projectPartId,
                        Utils::Language language,
                        Utils::LanguageVersion languageVersion,
                        Utils::LanguageExtension languageExtension)
        : projectPartId(projectPartId)
        , toolChainArguments(toStringVector(compilerArgumentsText))
        , compilerMacros(toCompilerMacros(compilerMacrosText))
        , systemIncludeSearchPaths(toIncludeSearchPaths(systemIncludeSearchPathsText))
        , projectIncludeSearchPaths(toIncludeSearchPaths(projectIncludeSearchPathsText))
        , language(language)
        , languageVersion(languageVersion)
        , languageExtension(languageExtension)
    {}

    static Utils::SmallStringVector toStringVector(Utils::SmallStringView jsonText);
    static CompilerMacros createCompilerMacrosFromDocument(const QJsonDocument &document);
    static IncludeSearchPaths createIncludeSearchPathsFromDocument(const QJsonDocument &document);
    static CompilerMacros toCompilerMacros(Utils::SmallStringView jsonText);
    static QJsonDocument createJsonDocument(Utils::SmallStringView jsonText, const char *whatError);
    static IncludeSearchPaths toIncludeSearchPaths(Utils::SmallStringView jsonText);
    static void checkError(const char *whatError, const QJsonParseError &error);
    friend bool operator==(const ProjectPartArtefact &first, const ProjectPartArtefact &second);

public:
    ProjectPartId projectPartId;
    Utils::SmallStringVector toolChainArguments;
    CompilerMacros compilerMacros;
    IncludeSearchPaths systemIncludeSearchPaths;
    IncludeSearchPaths projectIncludeSearchPaths;
    Utils::Language language = Utils::Language::Cxx;
    Utils::LanguageVersion languageVersion = Utils::LanguageVersion::CXX98;
    Utils::LanguageExtension languageExtension = Utils::LanguageExtension::None;
};

using ProjectPartArtefacts = std::vector<ProjectPartArtefact>;
} // namespace ClangBackEnd
