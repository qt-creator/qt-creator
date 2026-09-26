// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaclasspath.h"

#include "javaconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/algorithm.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java {

QString javaPackageOf(const QString &contents)
{
    // A package declaration is the first statement of a compilation unit, and
    // what may come before it is comments and blank lines.
    static const QRegularExpression declaration(R"(^[ \t]*package[ \t]+([\w.]+)[ \t]*;)",
                                                QRegularExpression::MultilineOption);
    const QRegularExpressionMatch match = declaration.match(contents);
    return match.hasMatch() ? match.captured(1) : QString();
}

// The directory the package hierarchy of that file starts at, which is what a
// language server is told about rather than the directory the file lies in: a
// file declaring "package com.example" has its root two directories up. One
// that lies somewhere else than its package says is its own root, since
// nothing else about it can be relied on.
static FilePath sourceRootOf(const FilePath &file, const QString &packageName)
{
    FilePath directory = file.parentDir();
    const QStringList parts = packageName.split('.', Qt::SkipEmptyParts);
    for (auto part = parts.rbegin(); part != parts.rend(); ++part) {
        if (directory.fileName() != *part)
            return file.parentDir();
        directory = directory.parentDir();
    }
    return directory;
}

FilePaths javaSourceRoots(const FilePaths &files)
{
    // The files of one directory share a package in every layout that is not
    // broken already, so one of them answers for all of them - and asking all
    // of them would read every Java file of the project on every parse.
    FilePaths roots;
    QSet<FilePath> directoriesAsked;
    for (const FilePath &file : files) {
        const FilePath directory = file.parentDir();
        if (directoriesAsked.contains(directory))
            continue;
        directoriesAsked.insert(directory);

        const Result<QByteArray> contents = file.fileContents();
        const FilePath root = contents
                                  ? sourceRootOf(file, javaPackageOf(QString::fromUtf8(*contents)))
                                  : directory;
        if (!root.isEmpty() && !roots.contains(root))
            roots << root;
    }
    return roots;
}

// What the build system of that configuration says about the Java of its
// targets. A build system that has none says nothing, and a project that no
// build system speaks for is one the language server reads by itself.
static std::optional<JavaClassPath> classPathFromBuildSystem(BuildConfiguration *bc)
{
    BuildSystem *buildSystem = bc->buildSystem();
    ProjectNode *rootNode = bc->project()->rootProjectNode();
    if (!buildSystem || !rootNode)
        return {};

    FilePaths files;
    JavaClassPath classPath;
    rootNode->forEachProjectNode([&](const ProjectNode *node) {
        const QString buildKey = node->buildKey();
        files += FilePaths::fromStrings(
            buildSystem->extraData(buildKey, Constants::JavaSourceFiles).toStringList());
        classPath.libraries += FilePaths::fromStrings(
            buildSystem->extraData(buildKey, Constants::JavaClassPath).toStringList());
    });

    if (files.isEmpty() && classPath.libraries.isEmpty())
        return {};

    classPath.sourceDirectories = javaSourceRoots(files);
    classPath.libraries = Utils::filteredUnique(classPath.libraries);
    return classPath;
}

QList<JavaClassPathProvider> &javaClassPathProviders()
{
    static QList<JavaClassPathProvider> theProviders;
    return theProviders;
}

std::optional<JavaClassPath> javaClassPath(ProjectExplorer::BuildConfiguration *bc)
{
    if (!bc)
        return {};

    // What the build system says about the Java of a project holds whoever
    // builds it, and a plugin that knows the build configuration adds what it
    // knows on top: an Android application compiles against the platform of
    // the SDK it targets as well as against whatever add_jar() was told about.
    std::optional<JavaClassPath> classPath = classPathFromBuildSystem(bc);
    for (const JavaClassPathProvider &provider : std::as_const(javaClassPathProviders()))
        classPath = mergedClassPath(classPath, provider(bc));
    return classPath;
}

std::optional<JavaClassPath> mergedClassPath(const std::optional<JavaClassPath> &first,
                                             const std::optional<JavaClassPath> &second)
{
    if (!first)
        return second;
    if (!second)
        return first;

    JavaClassPath merged;
    merged.sourceDirectories = Utils::filteredUnique(first->sourceDirectories
                                                     + second->sourceDirectories);
    merged.libraries = Utils::filteredUnique(first->libraries + second->libraries);
    return merged;
}

} // namespace Java
