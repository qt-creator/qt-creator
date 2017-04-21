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
    return Core::ICore::instance()->libexecPath();
#else
    return QString();
#endif
}

QStringList ClangCompilerOptionsBuilder::build(const CppTools::ProjectPart *projectPart,
                                               CppTools::ProjectFile::Kind fileKind,
                                               PchUsage pchUsage,
                                               const QString &clangVersion,
                                               const QString &clangResourceDirectory)
{
    if (projectPart) {
        ClangCompilerOptionsBuilder builder(*projectPart, clangVersion, clangResourceDirectory);

        builder.addWordWidth();
        builder.addTargetTriple();
        builder.addLanguageOption(fileKind);
        builder.addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
        builder.enableExceptions();

        builder.addDefineToAvoidIncludingGccOrMinGwIntrinsics();
        builder.addDefineFloat128ForMingw();
        builder.addToolchainAndProjectDefines();
        builder.undefineCppLanguageFeatureMacrosForMsvc2015();

        builder.addPredefinedMacrosAndHeaderPathsOptions();
        builder.addWrappedQtHeadersIncludePath();
        builder.addPrecompiledHeaderOptions(pchUsage);
        builder.addHeaderPathOptions();
        builder.addProjectConfigFileInclude();

        builder.addMsvcCompatibilityVersion();

        builder.addExtraOptions();

        return builder.options();
    }

    return QStringList();
}

ClangCompilerOptionsBuilder::ClangCompilerOptionsBuilder(const CppTools::ProjectPart &projectPart,
                                                         const QString &clangVersion,
                                                         const QString &clangResourceDirectory)
    : CompilerOptionsBuilder(projectPart),
      m_clangVersion(clangVersion),
      m_clangResourceDirectory(clangResourceDirectory)
{
}

bool ClangCompilerOptionsBuilder::excludeHeaderPath(const QString &path) const
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        if (path.contains("lib/gcc/i686-apple-darwin"))
            return true;
    }

    return CompilerOptionsBuilder::excludeHeaderPath(path);
}

void ClangCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptions()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        addPredefinedMacrosAndHeaderPathsOptionsForMsvc();
    else
        addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc();
}

void ClangCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptionsForMsvc()
{
    add("-nostdinc");
    add("-undef");
}

void ClangCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc()
{
    static const QString resourceDir = clangIncludeDirectory();
    if (QTC_GUARD(!resourceDir.isEmpty())) {
        add("-nostdlibinc");
        add("-I" + QDir::toNativeSeparators(resourceDir));
        add("-undef");
    }
}

void ClangCompilerOptionsBuilder::addWrappedQtHeadersIncludePath()
{
    static const QString wrappedQtHeadersPath = creatorResourcePath()
            + "/cplusplus/wrappedQtHeaders";

    if (m_projectPart.qtVersion != CppTools::ProjectPart::NoQt) {
        const QString wrappedQtCoreHeaderPath = wrappedQtHeadersPath + "/QtCore";
        add("-I" + QDir::toNativeSeparators(wrappedQtHeadersPath));
        add("-I" + QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
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
    QDir dir(creatorResourcePath() + "/clang/lib/clang/" + m_clangVersion + "/include");

    if (!dir.exists() || !QFileInfo(dir, "stdint.h").exists())
        dir = QDir(m_clangResourceDirectory);

    return dir.canonicalPath();
}

} // namespace CppTools
