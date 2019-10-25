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

#include "nimproject.h"
#include "nimbleproject.h"
#include "nimprojectnode.h"

#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QVariantMap>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char SETTINGS_KEY[] = "Nim.BuildSystem";
const char EXCLUDED_FILES_KEY[] = "ExcludedFiles";

NimBuildSystem::NimBuildSystem(Target *target)
    : BuildSystem(target)
{
    connect(target->project(), &Project::settingsLoaded, this, &NimBuildSystem::loadSettings);
    connect(target->project(), &Project::aboutToSaveSettings, this, &NimBuildSystem::saveSettings);

    connect(&m_scanner, &TreeScanner::finished, this, &NimBuildSystem::updateProject);
    m_scanner.setFilter([this](const Utils::MimeType &, const Utils::FilePath &fp) {
        const QString path = fp.toString();
        return excludedFiles().contains(path)
                || path.endsWith(".nimproject")
                || path.contains(".nimproject.user");
    });

    connect(&m_directoryWatcher, &FileSystemWatcher::directoryChanged, this, [this] {
        if (!isWaitingForParse())
            requestDelayedParse();
    });
}

bool NimBuildSystem::addFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filtered(excludedFiles(), [&](const QString & f) {
        return !filePaths.contains(f);
    }));
    requestParse();
    return true;
}

bool NimBuildSystem::removeFiles(const QStringList &filePaths)
{
    setExcludedFiles(Utils::filteredUnique(excludedFiles() + filePaths));
    requestParse();
    return true;
}

bool NimBuildSystem::renameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(filePath)
    QStringList files = excludedFiles();
    files.removeOne(newFilePath);
    setExcludedFiles(files);
    requestParse();
    return true;
}

void NimBuildSystem::setExcludedFiles(const QStringList &list)
{
    static_cast<NimProject *>(project())->setExcludedFiles(list);
}

QStringList NimBuildSystem::excludedFiles()
{
    return static_cast<NimProject *>(project())->excludedFiles();
}

void NimBuildSystem::triggerParsing()
{
    m_guard = guardParsingRun();
    m_scanner.asyncScanForFiles(projectDirectory());
}

const FilePathList NimBuildSystem::nimFiles() const
{
    return project()->files([](const Node *n) {
        return Project::AllFiles(n) && n->path().endsWith(".nim");
    });
}

void NimBuildSystem::loadSettings()
{
    QVariantMap settings = project()->namedSettings(SETTINGS_KEY).toMap();
    if (settings.contains(EXCLUDED_FILES_KEY))
        setExcludedFiles(settings.value(EXCLUDED_FILES_KEY, excludedFiles()).toStringList());

    requestParse();
}

void NimBuildSystem::saveSettings()
{
    QVariantMap settings;
    settings.insert(EXCLUDED_FILES_KEY, excludedFiles());
    project()->setNamedSettings(SETTINGS_KEY, settings);
}

void NimBuildSystem::updateProject()
{
    // Collect scanned nodes
    std::vector<std::unique_ptr<FileNode>> nodes;
    for (FileNode *node : m_scanner.release()) {
        if (!node->path().endsWith(".nim") && !node->path().endsWith(".nimble"))
            node->setEnabled(false); // Disable files that do not end in .nim
        nodes.emplace_back(node);
    }

    // Sync watched dirs
    const QSet<QString> fsDirs = Utils::transform<QSet>(nodes, &FileNode::directory);
    const QSet<QString> projectDirs = Utils::toSet(m_directoryWatcher.directories());
    m_directoryWatcher.addDirectories(Utils::toList(fsDirs - projectDirs), FileSystemWatcher::WatchAllChanges);
    m_directoryWatcher.removeDirectories(Utils::toList(projectDirs - fsDirs));

    // Sync project files
    const QSet<FilePath> fsFiles = Utils::transform<QSet>(nodes, &FileNode::filePath);
    const QSet<FilePath> projectFiles = Utils::toSet(project()->files([](const Node *n) { return Project::AllFiles(n); }));

    if (fsFiles != projectFiles) {
        auto projectNode = std::make_unique<ProjectNode>(project()->projectDirectory());
        projectNode->setDisplayName(project()->displayName());
        projectNode->addNestedNodes(std::move(nodes));
        project()->setRootProjectNode(std::move(projectNode));
    }

    // Complete scan
    m_guard.markAsSuccess();
    m_guard = {}; // Trigger destructor of previous object, emitting parsingFinished()
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

bool NimBuildSystem::addFiles(Node *, const QStringList &filePaths, QStringList *)
{
    return addFiles(filePaths);
}

RemovedFilesFromProject NimBuildSystem::removeFiles(Node *,
                                                    const QStringList &filePaths,
                                                    QStringList *)
{
    return removeFiles(filePaths) ? RemovedFilesFromProject::Ok
                                                 : RemovedFilesFromProject::Error;
}

bool NimBuildSystem::deleteFiles(Node *, const QStringList &)
{
    return true;
}

bool NimBuildSystem::renameFile(Node *, const QString &filePath, const QString &newFilePath)
{
    return renameFile(filePath, newFilePath);
}
} // namespace Nim
