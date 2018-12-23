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

#include "nimproject.h"
#include "nimbuildconfiguration.h"
#include "nimprojectnode.h"
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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QFileInfo>
#include <QQueue>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const int MIN_TIME_BETWEEN_PROJECT_SCANS = 4500;

NimProject::NimProject(const FileName &fileName) : Project(Constants::C_NIM_MIMETYPE, fileName)
{
    setId(Constants::C_NIMPROJECT_ID);
    setDisplayName(fileName.toFileInfo().completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    m_projectScanTimer.setSingleShot(true);
    connect(&m_projectScanTimer, &QTimer::timeout, this, &NimProject::collectProjectFiles);
    connect(this, &Project::settingsLoaded, this, &NimProject::collectProjectFiles);

    connect(&m_futureWatcher, &QFutureWatcher<QList<FileNode *>>::finished, this,
            &NimProject::updateProject);
}

void NimProject::scheduleProjectScan()
{
    auto elapsedTime = m_lastProjectScan.elapsed();
    if (elapsedTime < MIN_TIME_BETWEEN_PROJECT_SCANS) {
        if (!m_projectScanTimer.isActive()) {
            m_projectScanTimer.setInterval(MIN_TIME_BETWEEN_PROJECT_SCANS - elapsedTime);
            m_projectScanTimer.start();
        }
    } else {
        collectProjectFiles();
    }
}

bool NimProject::addFiles(const QStringList &filePaths)
{
    m_excludedFiles = Utils::filtered(m_excludedFiles, [&](const QString & f) {
        return !filePaths.contains(f);
    });
    scheduleProjectScan();
    return true;
}

bool NimProject::removeFiles(const QStringList &filePaths)
{
    m_excludedFiles.append(filePaths);
    m_excludedFiles = Utils::filteredUnique(m_excludedFiles);
    scheduleProjectScan();
    return true;
}

bool NimProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(filePath)
    m_excludedFiles.removeOne(newFilePath);
    scheduleProjectScan();
    return true;
}

void NimProject::collectProjectFiles()
{
    m_lastProjectScan.start();
    QTC_ASSERT(!m_futureWatcher.future().isRunning(), return);
    FileName prjDir = projectDirectory();
    QFuture<QList<ProjectExplorer::FileNode *>> future = Utils::runAsync([prjDir,
    excluded = m_excludedFiles] {
        return FileNode::scanForFiles(prjDir, [excluded](const FileName & fn) -> FileNode * {
            const QString fileName = fn.fileName();
            if (excluded.contains(fn.toString())
                    || fileName.endsWith(".nimproject", HostOsInfo::fileNameCaseSensitivity())
                    || fileName.contains(".nimproject.user", HostOsInfo::fileNameCaseSensitivity()))
                return nullptr;
            return new FileNode(fn, FileType::Source, false);
        });
    });
    m_futureWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Scanning for Nim files"), "Nim.Project.Scan");
}

void NimProject::updateProject()
{
    emitParsingStarted();

    auto newRoot = std::make_unique<NimProjectNode>(*this, projectDirectory());

    QList<FileNode *> files = m_futureWatcher.future().result();

    for (FileNode *node : files)
        newRoot->addNestedNode(std::unique_ptr<FileNode>(node));

    newRoot->setDisplayName(displayName());
    setRootProjectNode(std::move(newRoot));

    emitParsingFinished(true);
}

QList<Task> NimProject::projectIssues(const Kit *k) const
{
    QList<Task> result = Project::projectIssues(k);
    auto tc = dynamic_cast<NimToolChain *>(ToolChainKitInformation::toolChain(k,
                                                                              Constants::C_NIMLANGUAGE_ID));
    if (!tc) {
        result.append(createProjectTask(Task::TaskType::Error, tr("No Nim compiler set.")));
        return result;
    }
    if (!tc->compilerCommand().exists())
        result.append(createProjectTask(Task::TaskType::Error, tr("Nim compiler does not exist.")));

    return result;
}

FileNameList NimProject::nimFiles() const
{
    return files([](const ProjectExplorer::Node *n) {
        return AllFiles(n) && n->filePath().endsWith(".nim");
    });
}

QVariantMap NimProject::toMap() const
{
    QVariantMap result = Project::toMap();
    result[Constants::C_NIMPROJECT_EXCLUDEDFILES] = m_excludedFiles;
    return result;
}

Project::RestoreResult NimProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    m_excludedFiles = map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList();
    return Project::fromMap(map, errorMessage);
}

} // namespace Nim
