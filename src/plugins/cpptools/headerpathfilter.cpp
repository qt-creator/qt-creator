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

#include "headerpathfilter.h"

#ifndef UNIT_TESTS
#include <coreplugin/icore.h>
#endif

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QRegularExpression>

#include <utils/algorithm.h>

namespace CppTools {

using ProjectExplorer::HeaderPath;
using ProjectExplorer::HeaderPaths;
using ProjectExplorer::HeaderPathType;

void HeaderPathFilter::process()
{
    const HeaderPaths &headerPaths = projectPart.headerPaths;

    addPreIncludesPath();

    for (const HeaderPath &headerPath : headerPaths)
        filterHeaderPath(headerPath);

    if (useTweakedHeaderPaths == UseTweakedHeaderPaths::Yes)
        tweakHeaderPaths();
}

bool HeaderPathFilter::isProjectHeaderPath(const QString &path) const
{
    return path.startsWith(projectDirectory) || path.startsWith(buildDirectory);
}

void HeaderPathFilter::removeGccInternalIncludePaths()
{
    if (projectPart.toolchainType != ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
        && projectPart.toolchainType != ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
        return;
    }

    if (projectPart.toolChainInstallDir.isEmpty())
        return;

    const Utils::FilePath gccInstallDir = projectPart.toolChainInstallDir;
    auto isGccInternalInclude = [gccInstallDir](const HeaderPath &headerPath) {
        const auto filePath = Utils::FilePath::fromString(headerPath.path);
        return filePath == gccInstallDir.pathAppended("include")
               || filePath == gccInstallDir.pathAppended("include-fixed");
    };

    Utils::erase(builtInHeaderPaths, isGccInternalInclude);
}

void HeaderPathFilter::filterHeaderPath(const ProjectExplorer::HeaderPath &headerPath)
{
    if (headerPath.path.isEmpty())
        return;

    switch (headerPath.type) {
    case HeaderPathType::BuiltIn:
        builtInHeaderPaths.push_back(headerPath);
        break;
    case HeaderPathType::System:
    case HeaderPathType::Framework:
        systemHeaderPaths.push_back(headerPath);
        break;
    case HeaderPathType::User:
        if (isProjectHeaderPath(headerPath.path))
            userHeaderPaths.push_back(headerPath);
        else
            systemHeaderPaths.push_back(headerPath);
        break;
    }
}

namespace {

QString clangIncludeDirectory(const QString &clangVersion, const QString &clangResourceDirectory)
{
#ifndef UNIT_TESTS
    return Core::ICore::clangIncludeDirectory(clangVersion, clangResourceDirectory);
#else
    Q_UNUSED(clangVersion)
    Q_UNUSED(clangResourceDirectory)
    return {CLANG_RESOURCE_DIR};
#endif
}

HeaderPaths::iterator resourceIterator(HeaderPaths &headerPaths)
{
    // include/c++, include/g++, libc++\include and libc++abi\include
    static const QString cppIncludes = R"((.*/include/.*(g\+\+|c\+\+).*))"
                                       R"(|(.*libc\+\+/include))"
                                       R"(|(.*libc\+\+abi/include))"
                                       R"(|(/usr/local/include))";
    static const QRegularExpression includeRegExp("\\A(" + cppIncludes + ")\\z");

    return std::stable_partition(headerPaths.begin(),
                                 headerPaths.end(),
                                 [&](const HeaderPath &headerPath) {
                                     return includeRegExp.match(headerPath.path).hasMatch();
                                 });
}

bool isClangSystemHeaderPath(const HeaderPath &headerPath)
{
    // Always exclude clang system includes (including intrinsics) which do not come with libclang
    // that Qt Creator uses for code model.
    // For example GCC on macOS uses system clang include path which makes clang code model
    // include incorrect system headers.
    static const QRegularExpression clangIncludeDir(
        R"(\A.*/lib\d*/clang/\d+\.\d+(\.\d+)?/include\z)");
    return clangIncludeDir.match(headerPath.path).hasMatch();
}

void removeClangSystemHeaderPaths(HeaderPaths &headerPaths)
{
    auto newEnd = std::remove_if(headerPaths.begin(), headerPaths.end(), isClangSystemHeaderPath);
    headerPaths.erase(newEnd, headerPaths.end());
}

} // namespace

void HeaderPathFilter::tweakHeaderPaths()
{
    removeClangSystemHeaderPaths(builtInHeaderPaths);
    removeGccInternalIncludePaths();

    auto split = resourceIterator(builtInHeaderPaths);

    if (!clangVersion.isEmpty()) {
        const QString clangIncludePath = clangIncludeDirectory(clangVersion, clangResourceDirectory);
        builtInHeaderPaths.insert(split, HeaderPath{clangIncludePath, HeaderPathType::BuiltIn});
    }
}

void HeaderPathFilter::addPreIncludesPath()
{
    if (!projectDirectory.isEmpty()) {
        const Utils::FilePath rootProjectDirectory = Utils::FilePath::fromString(projectDirectory)
                .pathAppended(".pre_includes");

        systemHeaderPaths.push_back(
            {rootProjectDirectory.toString(), ProjectExplorer::HeaderPathType::System});
    }
}

QString HeaderPathFilter::ensurePathWithSlashEnding(const QString &path)
{
    QString pathWithSlashEnding = path;
    if (!pathWithSlashEnding.isEmpty() && *pathWithSlashEnding.rbegin() != '/')
        pathWithSlashEnding.push_back('/');

    return pathWithSlashEnding;
}

} // namespace CppTools
