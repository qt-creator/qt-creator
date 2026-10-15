// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javatargets.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Utils;

namespace CMakeProjectManager::Internal {

/*!
    Returns what \c cmake-helper/java-targets.cmake recorded about the
    \c add_jar() targets of the build in \a buildDirectory, with what it
    wrote of the tree in \a sourceDirectory resolved against that directory.

    The CMake file API reports such a target as a utility with neither sources
    nor artifacts, because \c UseJava.cmake builds the archive from a custom
    command. The archive and the \c .java files it compiles are therefore
    only reachable through the file the helper writes.

    A path of either tree is written relative to the top of that tree, the way
    the file API reports its own, and is a path of the tree as this side knows
    it once resolved. What CMake was handed may be another name for the same
    tree - on Windows the directories are configured through a junction - and
    files under that name are not the files of the project.
*/
QList<CMakeJavaTarget> readJavaTargets(const FilePath &sourceDirectory,
                                       const FilePath &buildDirectory)
{
    const FilePath file = buildDirectory / ProjectExplorer::Constants::PROJECT_QTC_DIR
                          / "java-targets.json";
    const Result<QByteArray> contents = file.fileContents();
    if (!contents)
        return {};

    // CMake knows nothing of devices and writes plain paths. They are on the
    // one the build runs on, so they need its scheme to stay reachable, which
    // resolving them against a directory of that device gives them.
    const auto inSourceTree = [&sourceDirectory](const QString &path) {
        return path.isEmpty() ? FilePath() : sourceDirectory.resolvePath(path);
    };
    const auto inBuildTree = [&buildDirectory](const QString &path) {
        return path.isEmpty() ? FilePath() : buildDirectory.resolvePath(path);
    };

    const QJsonArray targets = QJsonDocument::fromJson(*contents)["targets"].toArray();
    QList<CMakeJavaTarget> result;
    for (const QJsonValue &value : targets) {
        const QJsonObject object = value.toObject();
        CMakeJavaTarget target;
        target.name = object["name"].toString();
        target.jar = inBuildTree(object["jar"].toString());
        target.sourceDirectory = inSourceTree(object["sourceDirectory"].toString());
        const QJsonArray sources = object["sources"].toArray();
        for (const QJsonValue &source : sources)
            target.sources.append(inSourceTree(source.toString()));
        const QJsonArray classPath = object["classPath"].toArray();
        for (const QJsonValue &entry : classPath)
            target.classPath.append(inSourceTree(entry.toString()));
        if (!target.name.isEmpty() && !target.jar.isEmpty())
            result.append(target);
    }
    return result;
}

} // namespace CMakeProjectManager::Internal
