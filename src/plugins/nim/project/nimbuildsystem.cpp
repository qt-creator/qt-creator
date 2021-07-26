/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimbuildsystem.h"

#include "nimconstants.h"
#include "nimproject.h"
#include "nimbleproject.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char SETTINGS_KEY[] = "Nim.BuildSystem";
const char EXCLUDED_FILES_KEY[] = "ExcludedFiles";

NimProjectScanner::NimProjectScanner(Project *project)
    : m_project(project)
{
    m_scanner.setFilter([this](const Utils::MimeType &, const FilePath &fp) {
        const QString path = fp.toString();
        return excludedFiles().contains(path)
                || path.endsWith(".nimproject")
                || path.contains(".nimproject.user")
                || path.contains(".nimble.user");
    });

    connect(&m_directoryWatcher, &FileSystemWatcher::directoryChanged,
            this, &NimProjectScanner::directoryChanged);
    connect(&m_directoryWatcher, &FileSystemWatcher::fileChanged,
            this, &NimProjectScanner::fileChanged);

    connect(m_project, &Project::settingsLoaded, this, &NimProjectScanner::loadSettings);
    connect(m_project, &Project::aboutToSaveSettings, this, &NimProjectScanner::saveSettings);

    connect(&m_scanner, &TreeScanner::finished, this, [this] {
        // Collect scanned nodes
        std::vector<std::unique_ptr<FileNode>> nodes;
        const TreeScanner::Result scanResult = m_scanner.release();
        for (FileNode *node : scanResult.allFiles) {
            if (!node->path().endsWith(".nim") && !node->path().endsWith(".nimble"))
                node->setEnabled(false); // Disable files that do not end in .nim
            nodes.emplace_back(node);
        }

        // Sync watched dirs
        const QSet<QString> fsDirs = Utils::transform<QSet>(nodes,
            [](const std::unique_ptr<FileNode> &fn) { return fn->directory().toString(); });
        const QSet<QString> projectDirs = Utils::toSet(m_directoryWatcher.directories());
        m_directoryWatcher.addDirectories(Utils::toList(fsDirs - projectDirs), FileSystemWatcher::WatchAllChanges);
        m_directoryWatcher.removeDirectories(Utils::toList(projectDirs - fsDirs));

        // Sync project files
        const QSet<FilePath> fsFiles = Utils::transform<QSet>(nodes, &FileNode::filePath);
        const QSet<FilePath> projectFiles = Utils::toSet(m_project->files([](const Node *n) { return Project::AllFiles(n); }));

        if (fsFiles != projectFiles) {
            auto projectNode = std::make_unique<ProjectNode>(m_project->projectDirectory());
            projectNode->setDisplayName(m_project->displayName());
            projectNode->addNestedNodes(std::move(nodes));
            m_project->setRootProjectNode(std::move(projectNode));
        }

        emit finished();
    });
}

void NimProjectScanner::loadSettings()
{
    QVariantMap settings = m_project->namedSettings(SETTINGS_KEY).toMap();
    if (settings.contains(EXCLUDED_FILES_KEY))
        setExcludedFiles(settings.value(EXCLUDED_FILES_KEY, excludedFiles()).toStringList());

    emit requestReparse();
}

void NimProjectScanner::saveSettings()
{
    QVariantMap settings;
    settings.insert(EXCLUDED_FILES_KEY, excludedFiles());
    m_project->setNamedSettings(SETTINGS_KEY, settings);
}

void NimProjectScanner::startScan()
{
    m_scanner.asyncScanForFiles(m_project->projectDirectory());
}

void NimProjectScanner::watchProjectFilePath()
{
    m_directoryWatcher.addFile(m_project->projectFilePath().toString(), FileSystemWatcher::WatchModifiedDate);
}

void NimProjectScanner::setExcludedFiles(const QStringList &list)
{
    static_cast<NimbleProject *>(m_project)->setExcludedFiles(list);
}

QStringList NimProjectScanner::excludedFiles() const
{
    return static_cast<NimbleProject *>(m_project)->excludedFiles();
}

bool NimProjectScanner::addFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filtered(excludedFiles(), [&](const QString & f) {
        return !filePaths.contains(f);
    }));

    requestReparse();

    return true;
}

RemovedFilesFromProject NimProjectScanner::removeFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filteredUnique(excludedFiles() + filePaths));

    requestReparse();

    return RemovedFilesFromProject::Ok;
}

bool NimProjectScanner::renameFile(const QString &, const QString &to)
{
    QStringList files = excludedFiles();
    files.removeOne(to);
    setExcludedFiles(files);

    requestReparse();

    return true;
}

NimBuildSystem::NimBuildSystem(Target *target)
    : BuildSystem(target), m_projectScanner(target->project())
{
    connect(&m_projectScanner, &NimProjectScanner::finished, this, [this] {
        m_guard.markAsSuccess();
        m_guard = {}; // Trigger destructor of previous object, emitting parsingFinished()

        emitBuildSystemUpdated();
    });

    connect(&m_projectScanner, &NimProjectScanner::requestReparse,
            this, &NimBuildSystem::requestDelayedParse);

    connect(&m_projectScanner, &NimProjectScanner::directoryChanged, this, [this] {
        if (!isWaitingForParse())
            requestDelayedParse();
    });

    requestDelayedParse();
}

void NimBuildSystem::triggerParsing()
{
    m_guard = guardParsingRun();
    m_projectScanner.startScan();
}

FilePath nimPathFromKit(Kit *kit)
{
    auto tc = ToolChainKitAspect::toolChain(kit, Constants::C_NIMLANGUAGE_ID);
    QTC_ASSERT(tc, return {});
    const FilePath command = tc->compilerCommand();
    return command.isEmpty() ? FilePath() : command.absolutePath();
}

FilePath nimblePathFromKit(Kit *kit)
{
    // There's no extra setting for "nimble", derive it from the "nim" path.
    const QString nimbleFromPath = QStandardPaths::findExecutable("nimble");
    const FilePath nimPath = nimPathFromKit(kit);
    const FilePath nimbleFromKit = nimPath.pathAppended(HostOsInfo::withExecutableSuffix("nimble"));
    return nimbleFromKit.exists() ? nimbleFromKit.canonicalPath() : FilePath::fromString(nimbleFromPath);
}

bool NimBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (node->asFileNode()) {
        return action == ProjectAction::Rename
            || action == ProjectAction::RemoveFile;
    }
    if (node->isFolderNodeType() || node->isProjectNodeType()) {
        return action == ProjectAction::AddNewFile
            || action == ProjectAction::RemoveFile
            || action == ProjectAction::AddExistingFile;
    }
    return BuildSystem::supportsAction(context, action, node);
}

bool NimBuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    return m_projectScanner.addFiles(Utils::transform(filePaths, &FilePath::toString));
}

RemovedFilesFromProject NimBuildSystem::removeFiles(Node *,
                                                    const FilePaths &filePaths,
                                                    FilePaths *)
{
    return m_projectScanner.removeFiles(Utils::transform(filePaths, &FilePath::toString));
}

bool NimBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    return true;
}

bool NimBuildSystem::renameFile(Node *, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    return m_projectScanner.renameFile(oldFilePath.toString(), newFilePath.toString());
}

} // namespace Nim
