// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
CPPEDITOR_EXPORT QStringList createLanguageOptionGcc(Utils::Language language, ProjectFile::Kind fileKind, bool objcExt);

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
    void addQtMacros();

    // Add custom options
    void add(const QString &arg, bool gccOnlyOption = false);
    void prepend(const QString &arg);
    void add(const QStringList &args, bool gccOnlyOptions = false);

    static UseToolchainMacros useToolChainMacros();
    void reset();

    void evaluateCompilerFlags();
    bool isClStyle() const;
    void setClStyle(bool clStyle) { m_clStyle = clStyle; }
    void setNativeMode() { m_nativeMode = true; }

    const ProjectPart &projectPart() const { return m_projectPart; }

private:
    void addIncludeDirOptionForPath(const ProjectExplorer::HeaderPath &path);
    bool excludeDefineDirective(const ProjectExplorer::Macro &macro) const;
    void insertWrappedHeaders(const QStringList &paths);
    QStringList wrappedQtHeadersIncludePath() const;
    QStringList wrappedMingwHeadersIncludePath() const;
    QByteArray msvcVersion() const;
    void addIncludeFile(const QString &file);
    void removeUnsupportedCpuFlags();

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
    bool m_nativeMode = false;
};

} // namespace CppEditor
