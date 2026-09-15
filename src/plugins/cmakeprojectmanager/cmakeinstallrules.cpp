// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeinstallrules.h"

#include "fileapiparser.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QHash>
#include <QList>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

using namespace FileApiDetails;

// The installers whose files are ones to deploy and can be looked up on the host. Of the
// types left over, install(SCRIPT) and install(CODE) run code CMake cannot look into, and
// install(IMPORTED_RUNTIME_ARTIFACTS) as well as a RUNTIME_DEPENDENCY_SET name libraries it
// resolves while installing - which is how a Qt application ships Qt, so a deployment
// leaving those out would miss the point. An install(DIRECTORY) names a directory whose
// files the rules do not: which of them a PATTERN or EXCLUDE option keeps is not reported,
// and the contents while the rules are read are not the ones an install copies anyway - a
// directory the build fills with QML modules holds nothing yet, and a file added to a
// directory of assets reconfigures nothing that would pick it up. A type a later CMake
// adds says as little, which is why this names the types to take rather than the ones to
// leave.
static bool installsDeployableFiles(const Installer &installer)
{
    return installer.type == "file" || installer.type == "target";
}

// The installers whose files belong to the projects using this one rather than to a target
// running it, so that a deployment without them is a complete one all the same.
// install(EXPORT) writes files CMake considers its own business, install(TARGETS
// ... FILE_SET) the headers a library has for its users and install(TARGETS
// ... CXX_MODULES_BMI) what a compiler reads.
static bool installsDevelopmentFiles(const Installer &installer)
{
    return installer.type == "export" || installer.type == "fileSet"
           || installer.type == "cxxModuleBmi";
}

// The paths of a target are written down against the build directory, the paths of
// everything else against the source directory. A path outside the directory it belongs
// to is absolute, which resolvePath() leaves alone.
static FilePath localPath(const Installer &installer,
                          const PathPair &path,
                          const FilePath &sourceDirectory,
                          const FilePath &buildDirectory)
{
    const FilePath &base = installer.type == "target" ? buildDirectory : sourceDirectory;
    return base.resolvePath(path.from);
}

// Where the file goes under the destination. The string form of a path says that too:
// what follows its last slash is the name the file gets.
static QString installName(const PathPair &path)
{
    if (!path.to.isEmpty())
        return path.to;
    const int lastSlash = path.from.lastIndexOf('/');
    return lastSlash == -1 ? path.from : path.from.mid(lastSlash + 1);
}

static QString withoutTrailingSlash(const QString &path)
{
    return path.endsWith('/') ? path.left(path.size() - 1) : path;
}

static QString pathAppended(const QString &directory, const QString &tail)
{
    if (tail.isEmpty() || tail == ".")
        return directory;
    return withoutTrailingSlash(directory) + '/' + tail;
}

// A drive of the host names nothing on the target, and an install with DESTDIR set, which
// is where the deploy steps have had their files from so far, leaves it out the same way:
// "C:/Program Files/App/bin" installs into "<DESTDIR>/Program Files/App/bin".
static QString withoutDriveLetter(const QString &path)
{
    if (path.size() > 1 && path.at(1) == ':' && path.at(0).isLetter())
        return path.mid(2);
    return path;
}

// The directory an install rule puts its files in, as an absolute path on the target, or
// empty where that is not a directory to deploy to. An absolute destination is where the
// files go whatever the install prefix says, which is how CMake reads one as well.
static QString installDirectory(const QString &destination, const QString &installPrefix)
{
    if (destination.isEmpty())
        return {};

    const bool isAbsolute = FilePath::fromUserInput(destination).isAbsolutePath();
    const QString directory = isAbsolute || installPrefix.isEmpty()
                                  ? destination
                                  : pathAppended(installPrefix, withoutTrailingSlash(destination));
    // A DeployableFile puts the slash between the directory and the file name itself, so
    // the root of the target has no spelling here: what is left of "/", or of a drive, is
    // nothing, and nothing is what says the destination is not one to deploy to.
    return withoutDriveLetter(withoutTrailingSlash(directory));
}

// The targets an install(TARGETS) installs, looked up by id rather than by the index the
// installer also carries: that one counts the targets of the whole codemodel, which is not
// the vector we are handed.
static QHash<QString, const TargetDetails *> targetsById(const std::vector<TargetDetails> &targets)
{
    QHash<QString, const TargetDetails *> result;
    for (const TargetDetails &t : targets)
        result.insert(t.id, &t);
    return result;
}

// Whether the rule names the directory the artifact of its target sits in, which is what an
// install(TARGETS) of a macOS application bundle or of a framework does. A DeployableFile
// holds a file, and what else is in such a directory is not in the rules.
static bool namesArtifactDirectory(const TargetDetails *target, const QString &from)
{
    if (!target)
        return false;
    const QString prefix = withoutTrailingSlash(from) + '/';
    return Utils::anyOf(target->artifacts, [&prefix](const FilePath &artifact) {
        return artifact.path().startsWith(prefix);
    });
}

InstallRuleDeployment deploymentFromInstallRules(
    const QFuture<void> &cancelFuture,
    const std::vector<DirectoryDetails> &directories,
    const std::vector<TargetDetails> &targets,
    const FilePath &sourceDirectory,
    const FilePath &buildDirectory,
    const QString &installPrefix)
{
    const QHash<QString, const TargetDetails *> targetOfId = targetsById(targets);

    QList<DeployableFile> files;
    QHash<DeployableFile, int> indexOfFile;

    // A DeployableFile compares by local path and remote directory alone, so a file two
    // rules name is one entry - as the executable it is if either of them says so.
    const auto add = [&files, &indexOfFile](const FilePath &localFilePath,
                                            const QString &remoteDirectory,
                                            DeployableFile::Type type) {
        const DeployableFile file(localFilePath, remoteDirectory, type);
        const auto index = indexOfFile.constFind(file);
        if (index == indexOfFile.constEnd()) {
            indexOfFile.insert(file, files.size());
            files.append(file);
        } else if (type == DeployableFile::TypeExecutable) {
            files[*index] = file;
        }
    };

    for (const DirectoryDetails &directory : directories) {
        if (cancelFuture.isCanceled())
            return {};

        for (const Installer &installer : directory.installers) {
            // An installer of a component that is not part of the default install is not
            // what "cmake --install" copies.
            if (installer.isExcludeFromAll)
                continue;

            if (installsDevelopmentFiles(installer))
                continue;

            // What is left over copies files that cannot be looked up, and an optional
            // installer copies its file only where the build produced one - the debug
            // symbols of a release build, say - while a deploy step treats a local file
            // that is missing as an error. Neither has a description to give, and the
            // rest of the deployment goes with it: a list silently missing files is worse
            // than none at all, which the header spells out.
            if (!installsDeployableFiles(installer) || installer.isOptional)
                return {};

            const QString destination = installDirectory(installer.destination, installPrefix);
            if (destination.isEmpty())
                return {};

            const TargetDetails *target = targetOfId.value(installer.targetId);

            for (const PathPair &path : installer.paths) {
                const FilePath local = localPath(installer, path, sourceDirectory, buildDirectory);
                const QString name = installName(path);

                // An install(TARGETS) naming the directory its artifact sits in installs
                // an application bundle or a framework, whose files are not in the rules.
                if (namesArtifactDirectory(target, path.from))
                    return {};

                // The directory an install name points into is something a DeployableFile
                // can hold, a name of its own for the file is not. So the file an
                // install(FILES ... RENAME) renames has to stay out: deploying it under
                // the name it has here would put the wrong file on the target.
                const int lastSlash = name.lastIndexOf('/');
                if (name.mid(lastSlash + 1) != local.fileName())
                    return {};

                const QString remoteDirectory
                    = lastSlash == -1 ? destination
                                      : pathAppended(destination, name.left(lastSlash));

                const bool isExecutable = installer.type == "target"
                                          && !installer.targetIsImportLibrary && target
                                          && target->type == "EXECUTABLE";
                add(local,
                    remoteDirectory,
                    isExecutable ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
            }
        }
    }

    if (cancelFuture.isCanceled() || files.isEmpty())
        return {};

    InstallRuleDeployment result;
    for (const DeployableFile &file : files)
        result.data.addFile(file);
    result.knowledge = DeploymentKnowledge::Perfect;
    return result;
}

} // CMakeProjectManager::Internal
