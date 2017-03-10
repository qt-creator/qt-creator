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

#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFileInfo>
#include <QQueue>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const int MIN_TIME_BETWEEN_PROJECT_SCANS = 4500;

NimProject::NimProject(const FileName &fileName)
{
    setId(Constants::C_NIMPROJECT_ID);
    setDocument(new TextEditor::TextDocument);
    document()->setFilePath(fileName);
    QFileInfo fi = fileName.toFileInfo();
    QDir dir = fi.dir();
    setRootProjectNode(new NimProjectNode(*this, FileName::fromString(dir.absolutePath())));
    rootProjectNode()->setDisplayName(dir.dirName());

    m_projectScanTimer.setSingleShot(true);
    connect(&m_projectScanTimer, &QTimer::timeout, this, &NimProject::collectProjectFiles);

    connect(&m_futureWatcher, &QFutureWatcher<QList<FileNode *>>::finished, this, &NimProject::updateProject);

    collectProjectFiles();
}

QString NimProject::displayName() const
{
    return rootProjectNode()->displayName();
}

QStringList NimProject::files(FilesMode) const
{
    return m_files;
}

bool NimProject::needsConfiguration() const
{
    return targets().empty();
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
    m_excludedFiles = Utils::filtered(m_excludedFiles, [&](const QString &f) { return !filePaths.contains(f); });
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
    QFuture<QList<ProjectExplorer::FileNode *>> future = Utils::runAsync([prjDir] {
        return FileNode::scanForFiles(prjDir, [](const FileName &fn) { return new FileNode(fn, FileType::Source, false); });
    });
    m_futureWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Scanning for Nim files"), "Nim.Project.Scan");
}

void NimProject::updateProject()
{
    QStringList oldFiles = m_files;
    m_files.clear();

    QList<FileNode *> fileNodes = Utils::filtered(m_futureWatcher.future().result(),
                                                  [&](const FileNode *fn) {
        const FileName path = fn->filePath();
        const QString fileName = path.fileName();
        const bool keep = !m_excludedFiles.contains(path.toString())
                && !fileName.endsWith(".nimproject", HostOsInfo::fileNameCaseSensitivity())
                && !fileName.contains(".nimproject.user", HostOsInfo::fileNameCaseSensitivity());
        if (!keep)
            delete fn;
        return keep;
    });

    m_files = Utils::transform(fileNodes, [](const FileNode *fn) { return fn->filePath().toString(); });
    Utils::sort(m_files, [](const QString &a, const QString &b) { return a < b; });

    if (oldFiles == m_files)
        return;

    rootProjectNode()->makeEmpty();
    rootProjectNode()->buildTree(fileNodes);

    emit fileListChanged();

    emit parsingFinished();
}

bool NimProject::supportsKit(Kit *k, QString *errorMessage) const
{
    auto tc = dynamic_cast<NimToolChain*>(ToolChainKitInformation::toolChain(k, Constants::C_NIMLANGUAGE_ID));
    if (!tc) {
        if (errorMessage)
            *errorMessage = tr("No nim compiler set.");
        return false;
    }
    if (!tc->compilerCommand().exists()) {
        if (errorMessage)
            *errorMessage = tr("Nim compiler doesn't exist");
        return false;
    }
    return true;
}

FileNameList NimProject::nimFiles() const
{
    FileNameList result;
    rootProjectNode()->forEachNode([&](FileNode *file) {
        if (file->displayName().endsWith(QLatin1String(".nim")))
            result.append(file->filePath());
    });
    return result;
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

}
