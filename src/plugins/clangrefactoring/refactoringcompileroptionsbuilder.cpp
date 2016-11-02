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

#include "refactoringcompileroptionsbuilder.h"

namespace ClangRefactoring {

namespace {

QString getCreatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::instance()->resourcePath();
#else
    return QString();
#endif
}

QString getClangIncludeDirectory()
{
    QDir dir(getCreatorResourcePath() + QLatin1String("/cplusplus/clang/") +
             QLatin1String(CLANG_VERSION) + QLatin1String("/include"));
    if (!dir.exists() || !QFileInfo(dir, QLatin1String("stdint.h")).exists())
        dir = QDir(QLatin1String(CLANG_RESOURCE_DIR));
    return dir.canonicalPath();
}

}

RefactoringCompilerOptionsBuilder::RefactoringCompilerOptionsBuilder(CppTools::ProjectPart *projectPart)
    : CompilerOptionsBuilder(*projectPart)
{
}

bool RefactoringCompilerOptionsBuilder::excludeHeaderPath(const QString &path) const
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        if (path.contains(QLatin1String("lib/gcc/i686-apple-darwin")))
            return true;
    }

    return CompilerOptionsBuilder::excludeHeaderPath(path);
}

void RefactoringCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptions()
{
    if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        addPredefinedMacrosAndHeaderPathsOptionsForMsvc();
    else
        addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc();
}

void RefactoringCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptionsForMsvc()
{
    add(QLatin1String("-nostdinc"));
    add(QLatin1String("-undef"));
}

void RefactoringCompilerOptionsBuilder::addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc()
{
    static const QString resourceDir = getClangIncludeDirectory();
    if (!resourceDir.isEmpty()) {
        add(QLatin1String("-nostdlibinc"));
        add(QLatin1String("-I") + resourceDir);
        add(QLatin1String("-undef"));
    }
}

void RefactoringCompilerOptionsBuilder::addWrappedQtHeadersIncludePath()
{
    static const QString wrappedQtHeaders = getCreatorResourcePath()
            + QStringLiteral("/cplusplus/wrappedQtHeaders");

    if (m_projectPart.qtVersion != CppTools::ProjectPart::NoQt) {
        add(QLatin1String("-I") + wrappedQtHeaders);
        add(QLatin1String("-I") + wrappedQtHeaders + QLatin1String("/QtCore"));
    }
}

void RefactoringCompilerOptionsBuilder::addProjectConfigFileInclude()
{
    if (!m_projectPart.projectConfigFile.isEmpty()) {
        add(QLatin1String("-include"));
        add(m_projectPart.projectConfigFile);
    }
}

void RefactoringCompilerOptionsBuilder::addExtraOptions()
{
    add(QLatin1String("-fmessage-length=0"));
    add(QLatin1String("-fdiagnostics-show-note-include-stack"));
    add(QLatin1String("-fmacro-backtrace-limit=0"));
    add(QLatin1String("-fretain-comments-from-system-headers"));
    add(QLatin1String("-ferror-limit=1000"));
}

Utils::SmallStringVector RefactoringCompilerOptionsBuilder::build(CppTools::ProjectPart *projectPart,
                                                                  CppTools::ProjectFile::Kind fileKind,
                                                                  PchUsage pchUsage)
{
    if (projectPart == nullptr)
        return Utils::SmallStringVector();

    RefactoringCompilerOptionsBuilder optionsBuilder(projectPart);

    optionsBuilder.addWordWidth();
    optionsBuilder.addTargetTriple();
    optionsBuilder.addLanguageOption(fileKind);
    optionsBuilder.addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
    optionsBuilder.enableExceptions();

    optionsBuilder.addDefineFloat128ForMingw();
    optionsBuilder.addDefineToAvoidIncludingGccOrMinGwIntrinsics();
    optionsBuilder.addToolchainAndProjectDefines();
    optionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();

    optionsBuilder.addPredefinedMacrosAndHeaderPathsOptions();
    optionsBuilder.addWrappedQtHeadersIncludePath();
    optionsBuilder.addHeaderPathOptions();
    optionsBuilder.addPrecompiledHeaderOptions(pchUsage);
    optionsBuilder.addProjectConfigFileInclude();

    optionsBuilder.addMsvcCompatibilityVersion();

    optionsBuilder.addExtraOptions();

    return Utils::SmallStringVector(optionsBuilder.options());
}

} // namespace ClangRefactoring
