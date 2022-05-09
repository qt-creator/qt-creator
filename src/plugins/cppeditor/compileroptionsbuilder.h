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

#include "cppeditor_global.h"

#include "projectpart.h"

namespace CppEditor {

enum class UsePrecompiledHeaders : char { Yes, No };
enum class UseSystemHeader : char { Yes, No };
enum class UseTweakedHeaderPaths : char { Yes, Tools, No };
enum class UseToolchainMacros : char { Yes, No };
enum class UseLanguageDefines : char { Yes, No };
enum class UseBuildSystemWarnings : char { Yes, No };

CPPEDITOR_EXPORT QStringList XclangArgs(const QStringList &args);
CPPEDITOR_EXPORT QStringList clangArgsForCl(const QStringList &args);
CPPEDITOR_EXPORT QStringList createLanguageOptionGcc(ProjectFile::Kind fileKind, bool objcExt);

class CPPEDITOR_EXPORT CompilerOptionsBuilder
{
public:
    CompilerOptionsBuilder(const ProjectPart &projectPart,
        UseSystemHeader useSystemHeader = UseSystemHeader::No,
        UseTweakedHeaderPaths useTweakedHeaderPaths = UseTweakedHeaderPaths::No,
        UseLanguageDefines useLanguageDefines = UseLanguageDefines::No,
        UseBuildSystemWarnings useBuildSystemWarnings = UseBuildSystemWarnings::No,
        const Utils::FilePath &clangIncludeDirectory = {});

    QStringList build(ProjectFile::Kind fileKind, UsePrecompiledHeaders usePrecompiledHeaders);
    QStringList options() const { return m_options; }

    // Add options based on project part
    void provideAdditionalMacros(const ProjectExplorer::Macros &macros);
    void addProjectMacros();
    void addSyntaxOnly();
    void addWordWidth();
    void addHeaderPathOptions();
    void addPrecompiledHeaderOptions(UsePrecompiledHeaders usePrecompiledHeaders);
    void addIncludedFiles(const QStringList &files);
    void addMacros(const ProjectExplorer::Macros &macros);

    void addTargetTriple();
    void addExtraCodeModelFlags();
    void addPicIfCompilerFlagsContainsIt();
    void addCompilerFlags();
    void addMsvcExceptions();
    void enableExceptions();
    void insertWrappedQtHeaders();
    void insertWrappedMingwHeaders();
    void addLanguageVersionAndExtensions();
    void updateFileLanguage(ProjectFile::Kind fileKind);

    void addMsvcCompatibilityVersion();
    void undefineCppLanguageFeatureMacrosForMsvc2015();
    void addDefineFunctionMacrosMsvc();

    void addProjectConfigFileInclude();
    void undefineClangVersionMacrosForMsvc();

    void addDefineFunctionMacrosQnx();

    // Add custom options
    void add(const QString &arg, bool gccOnlyOption = false);
    void prepend(const QString &arg);
    void add(const QStringList &args, bool gccOnlyOptions = false);

    static UseToolchainMacros useToolChainMacros();
    void reset();

    void evaluateCompilerFlags();
    bool isClStyle() const;

    const ProjectPart &projectPart() const { return m_projectPart; }

private:
    void addIncludeDirOptionForPath(const ProjectExplorer::HeaderPath &path);
    bool excludeDefineDirective(const ProjectExplorer::Macro &macro) const;
    void insertWrappedHeaders(const QStringList &paths);
    QStringList wrappedQtHeadersIncludePath() const;
    QStringList wrappedMingwHeadersIncludePath() const;
    QByteArray msvcVersion() const;
    void addIncludeFile(const QString &file);

private:
    const ProjectPart &m_projectPart;

    const UseSystemHeader m_useSystemHeader;
    const UseTweakedHeaderPaths m_useTweakedHeaderPaths;
    const UseLanguageDefines m_useLanguageDefines;
    const UseBuildSystemWarnings m_useBuildSystemWarnings;

    const Utils::FilePath m_clangIncludeDirectory;

    ProjectExplorer::Macros m_additionalMacros;

    struct {
        QStringList flags;
        bool isLanguageVersionSpecified = false;
    } m_compilerFlags;

    QStringList m_options;
    QString m_explicitTarget;
    bool m_clStyle = false;
};

} // namespace CppEditor
