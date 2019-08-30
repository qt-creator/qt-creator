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
#include "nimprojectnode.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QVariantMap>

#if 0
#include "nimbuildconfiguration.h"
#include "nimtoolchain.h"

#include "../nimconstants.h"

#include <coreplugin/icontext.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <texteditor/textdocument.h>

#include <utils/runextensions.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QFileInfo>
#include <QQueue>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char SETTINGS_KEY[] = "Nim.BuildSystem";
const char EXCLUDED_FILES_KEY[] = "ExcludedFiles";

NimBuildSystem::NimBuildSystem(Project *project)
    : BuildSystem(project)
{
    connect(project, &Project::settingsLoaded, this, &NimBuildSystem::loadSettings);
    connect(project, &Project::aboutToSaveSettings, this, &NimBuildSystem::saveSettings);

    connect(&m_scanner, &TreeScanner::finished, this, &NimBuildSystem::updateProject);
    m_scanner.setFilter([this](const Utils::MimeType &, const Utils::FilePath &fp) {
        const QString path = fp.toString();
        return excludedFiles().contains(path) || path.endsWith(".nimproject")
               || path.contains(".nimproject.user");
    });

    connect(&m_directoryWatcher, &FileSystemWatcher::directoryChanged, this, [this]() {
        requestParse();
    });
}

bool NimBuildSystem::addFiles(const QStringList &filePaths)
{
    m_excludedFiles = Utils::filtered(m_excludedFiles, [&](const QString & f) {
        return !filePaths.contains(f);
    });
    requestParse();
    return true;
}

bool NimBuildSystem::removeFiles(const QStringList &filePaths)
{
    m_excludedFiles.append(filePaths);
    m_excludedFiles = Utils::filteredUnique(m_excludedFiles);
    requestParse();
    return true;
}

bool NimBuildSystem::renameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(filePath)
    m_excludedFiles.removeOne(newFilePath);
    requestParse();
    return true;
}

void NimBuildSystem::setExcludedFiles(const QStringList &list)
{
    m_excludedFiles = list;
}

QStringList NimBuildSystem::excludedFiles()
{
    return m_excludedFiles;
}

void NimBuildSystem::parseProject(BuildSystem::ParsingContext &&ctx)
{
    QTC_ASSERT(!m_currentContext.project, return );
    m_currentContext = std::move(ctx);
    QTC_CHECK(m_currentContext.project);

    m_scanner.asyncScanForFiles(m_currentContext.project->projectDirectory());
}

const FilePathList NimBuildSystem::nimFiles() const
{
    return project()->files(
        [](const Node *n) { return Project::AllFiles(n) && n->path().endsWith(".nim"); });
}

void NimBuildSystem::loadSettings()
{
    QVariantMap settings = project()->namedSettings(SETTINGS_KEY).toMap();
    if (settings.contains(EXCLUDED_FILES_KEY))
        m_excludedFiles = settings.value(EXCLUDED_FILES_KEY, m_excludedFiles).toStringList();

    requestParse();
}

void NimBuildSystem::saveSettings()
{
    QVariantMap settings;
    settings.insert(EXCLUDED_FILES_KEY, m_excludedFiles);
    project()->setNamedSettings(SETTINGS_KEY, settings);
}

void NimBuildSystem::updateProject()
{
    auto newRoot = std::make_unique<NimProjectNode>(project()->projectDirectory());

    QSet<QString> directories;
    for (FileNode *node : m_scanner.release()) {
        if (!node->path().endsWith(".nim"))
            node->setEnabled(false); // Disable files that do not end in .nim
        directories.insert(node->directory());
        newRoot->addNestedNode(std::unique_ptr<FileNode>(node));
    }

    newRoot->setDisplayName(project()->displayName());
    project()->setRootProjectNode(std::move(newRoot));

    m_directoryWatcher.addDirectories(Utils::toList(directories), FileSystemWatcher::WatchAllChanges);

    m_currentContext.guard.markAsSuccess();

    m_currentContext = {};
}

} // namespace Nim
