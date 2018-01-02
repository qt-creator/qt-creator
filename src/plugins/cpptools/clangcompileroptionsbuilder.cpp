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

#include "clangcompileroptionsbuilder.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QDir>

namespace CppTools {

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::instance()->resourcePath();
#else
    return QString();
#endif
}

static QString creatorLibexecPath()
{
#ifndef UNIT_TESTS
    return Core::ICore::instance()->libexecPath();
#else
    return QString();
#endif
}

QStringList ClangCompilerOptionsBuilder::build(CppTools::ProjectFile::Kind fileKind,
                                               PchUsage pchUsage)
{
    addWordWidth();
    addTargetTriple();
    addLanguageOption(fileKind);
    addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
    enableExceptions();

    addDefineFloat128ForMingw();
    addToolchainAndProjectMacros();
    undefineClangVersionMacrosForMsvc();
    undefineCppLanguageFeatureMacrosForMsvc2015();
    addDefineFunctionMacrosMsvc();

    addPredefinedHeaderPathsOptions();
    addWrappedQtHeadersIncludePath();
    addPrecompiledHeaderOptions(pchUsage);
    addHeaderPathOptions();
    addProjectConfigFileInclude();

    addMsvcCompatibilityVersion();

    addExtraOptions();

    return options();
}

ClangCompilerOptionsBuilder::ClangCompilerOptionsBuilder(const CppTools::ProjectPart &projectPart,
                                                         const QString &clangVersion,
                                                         const QString &clangResourceDirectory)
    : CompilerOptionsBuilder(projectPart),
      m_clangVersion(clangVersion),
      m_clangResourceDirectory(clangResourceDirectory)
{
}

void ClangCompilerOptionsBuilder::addPredefinedHeaderPathsOptions()
{
    add("-undef");
    add("-nostdinc");
    add("-nostdlibinc");

    if (m_projectPart.toolchainType != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        add(includeDirOption() + clangIncludeDirectory());
}

void ClangCompilerOptionsBuilder::addWrappedQtHeadersIncludePath()
{
    static const QString resourcePath = creatorResourcePath();
    static QString wrappedQtHeadersPath = resourcePath + "/cplusplus/wrappedQtHeaders";
    QDir dir(wrappedQtHeadersPath);
    QTC_ASSERT(QDir(wrappedQtHeadersPath).exists(), return;);

    if (m_projectPart.qtVersion != CppTools::ProjectPart::NoQt) {
        const QString wrappedQtCoreHeaderPath = wrappedQtHeadersPath + "/QtCore";
        add(includeDirOption() + QDir::toNativeSeparators(wrappedQtHeadersPath));
        add(includeDirOption() + QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
    }
}

void ClangCompilerOptionsBuilder::addProjectConfigFileInclude()
{
    if (!m_projectPart.projectConfigFile.isEmpty()) {
        add("-include");
        add(QDir::toNativeSeparators(m_projectPart.projectConfigFile));
    }
}

void ClangCompilerOptionsBuilder::addExtraOptions()
{
    add("-fmessage-length=0");
    add("-fdiagnostics-show-note-include-stack");
    add("-fmacro-backtrace-limit=0");
    add("-fretain-comments-from-system-headers");
    add("-ferror-limit=1000");
}

QString ClangCompilerOptionsBuilder::clangIncludeDirectory() const
{
    QDir dir(creatorLibexecPath() + "/clang/lib/clang/" + m_clangVersion + "/include");
    if (!dir.exists() || !QFileInfo(dir, "stdint.h").exists())
        dir = QDir(m_clangResourceDirectory);
    return QDir::toNativeSeparators(dir.canonicalPath());
}

void ClangCompilerOptionsBuilder::undefineClangVersionMacrosForMsvc()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        static QStringList macroNames {
            "__clang__",
            "__clang_major__",
            "__clang_minor__",
            "__clang_patchlevel__",
            "__clang_version__"
        };

        foreach (const QString &macroName, macroNames)
            add(undefineOption() + macroName);
    }
}

} // namespace CppTools
