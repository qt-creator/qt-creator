/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "autotoolsproject.h"
#include "autotoolsbuildconfiguration.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsmanager.h"
#include "autotoolsprojectnode.h"
#include "autotoolsprojectfile.h"
#include "autotoolsopenprojectwizard.h"
#include "makestep.h"
#include "makefileparserthread.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/headerpath.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>
#include <utils/filesystemwatcher.h>

#include <QFileInfo>
#include <QTimer>
#include <QPointer>
#include <QApplication>
#include <QCursor>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

AutotoolsProject::AutotoolsProject(AutotoolsManager *manager, const QString &fileName) :
    m_manager(manager),
    m_fileName(fileName),
    m_files(),
    m_file(new AutotoolsProjectFile(this, m_fileName)),
    m_rootNode(new AutotoolsProjectNode(m_file->filePath())),
    m_fileWatcher(new Utils::FileSystemWatcher(this)),
    m_watchedFiles(),
    m_makefileParserThread(0)
{
    setId(Constants::AUTOTOOLS_PROJECT_ID);
    setProjectContext(Core::Context(Constants::PROJECT_CONTEXT));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    const QFileInfo fileInfo(m_fileName);
    m_projectName = fileInfo.absoluteDir().dirName();
    m_rootNode->setDisplayName(fileInfo.absoluteDir().dirName());
}

AutotoolsProject::~AutotoolsProject()
{
    // Although ProjectExplorer::ProjectNode is a QObject, the ctor
    // does not allow to specify the parent. Manually setting the
    // parent would be possible, but we use the same approach as in the
    // other project managers and delete the node manually (TBD).
    //
    delete m_rootNode;
    m_rootNode = 0;

    if (m_makefileParserThread != 0) {
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = 0;
    }
}

QString AutotoolsProject::displayName() const
{
    return m_projectName;
}

Core::IDocument *AutotoolsProject::document() const
{
    return m_file;
}

IProjectManager *AutotoolsProject::projectManager() const
{
    return m_manager;
}

QString AutotoolsProject::defaultBuildDirectory(const QString &projectPath)
{
    return QFileInfo(projectPath).absolutePath();
}

ProjectNode *AutotoolsProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList AutotoolsProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    return m_files;
}

// This function, is called at the very beginning, to
// restore the settings if there are some stored.
Project::RestoreResult AutotoolsProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    connect(m_fileWatcher, &Utils::FileSystemWatcher::fileChanged,
            this, &AutotoolsProject::onFileChanged);

    // Load the project tree structure.
    loadProjectTree();

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    return RestoreResult::Ok;
}

void AutotoolsProject::loadProjectTree()
{
    if (m_makefileParserThread != 0) {
        // The thread is still busy parsing a previus configuration.
        // Wait until the thread has been finished and delete it.
        // TODO: Discuss whether blocking is acceptable.
        disconnect(m_makefileParserThread, &QThread::finished,
                   this, &AutotoolsProject::makefileParsingFinished);
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = 0;
    }

    // Parse the makefile asynchronously in a thread
    m_makefileParserThread = new MakefileParserThread(m_fileName);

    connect(m_makefileParserThread, &MakefileParserThread::started,
            this, &AutotoolsProject::makefileParsingStarted);

    connect(m_makefileParserThread, &MakefileParserThread::finished,
            this, &AutotoolsProject::makefileParsingFinished);
    m_makefileParserThread->start();
}

void AutotoolsProject::makefileParsingStarted()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void AutotoolsProject::makefileParsingFinished()
{
    // The finished() signal is from a previous makefile-parser-thread
    // and can be skipped. This can happen, if the thread has emitted the
    // finished() signal during the execution of AutotoolsProject::loadProjectTree().
    // In this case the signal is in the message queue already and deleting
    // the thread of course does not remove the signal again.
    if (sender() != m_makefileParserThread)
        return;

    QApplication::restoreOverrideCursor();

    if (m_makefileParserThread->isCanceled()) {
        // The parsing has been cancelled by the user. Don't show any
        // project data at all.
        m_makefileParserThread->deleteLater();
        m_makefileParserThread = 0;
        return;
    }

    if (m_makefileParserThread->hasError())
        qWarning("Parsing of makefile contained errors.");

    // Remove file watches for the current project state.
    // The file watches will be added again after the parsing.
    foreach (const QString& watchedFile, m_watchedFiles)
        m_fileWatcher->removeFile(watchedFile);

    m_watchedFiles.clear();

    // Apply sources to m_files, which are returned at AutotoolsProject::files()
    const QFileInfo fileInfo(m_fileName);
    const QDir dir = fileInfo.absoluteDir();
    QStringList files = m_makefileParserThread->sources();
    foreach (const QString& file, files)
        m_files.append(dir.absoluteFilePath(file));

    // Watch for changes of Makefile.am files. If a Makefile.am file
    // has been changed, the project tree must be reparsed.
    const QStringList makefiles = m_makefileParserThread->makefiles();
    foreach (const QString &makefile, makefiles) {
        files.append(makefile);

        const QString watchedFile = dir.absoluteFilePath(makefile);
        m_fileWatcher->addFile(watchedFile, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(watchedFile);
    }

    // Add configure.ac file to project and watch for changes.
    const QLatin1String configureAc(QLatin1String("configure.ac"));
    const QFile configureAcFile(fileInfo.absolutePath() + QLatin1Char('/') + configureAc);
    if (configureAcFile.exists()) {
        files.append(configureAc);
        const QString configureAcFilePath = dir.absoluteFilePath(configureAc);
        m_fileWatcher->addFile(configureAcFilePath, Utils::FileSystemWatcher::WatchAllChanges);
        m_watchedFiles.append(configureAcFilePath);
    }

    buildFileNodeTree(dir, files);
    updateCppCodeModel();

    m_makefileParserThread->deleteLater();
    m_makefileParserThread = 0;
}

void AutotoolsProject::onFileChanged(const QString &file)
{
    Q_UNUSED(file);
    loadProjectTree();
}

QStringList AutotoolsProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

void AutotoolsProject::buildFileNodeTree(const QDir &directory,
                                         const QStringList &files)
{
    // Get all existing nodes and remember them in a hash table.
    // This allows to reuse existing nodes and to remove obsolete
    // nodes later.
    QHash<QString, Node *> nodeHash;
    foreach (Node * node, nodes(m_rootNode))
        nodeHash.insert(node->filePath().toString(), node);

    // Add the sources to the filenode project tree. Sources
    // inside the same directory are grouped into a folder-node.
    const QString baseDir = directory.absolutePath();

    QList<FileNode *> fileNodes;
    FolderNode *parentFolder = 0;
    FolderNode *oldParentFolder = 0;

    foreach (const QString &file, files) {
        if (file.endsWith(QLatin1String(".moc")))
            continue;

        QString subDir = baseDir + QLatin1Char('/') + file;
        const int lastSlashPos = subDir.lastIndexOf(QLatin1Char('/'));
        if (lastSlashPos != -1)
            subDir.truncate(lastSlashPos);

        // Add folder nodes, that are not already available
        oldParentFolder = parentFolder;
        parentFolder = 0;
        if (nodeHash.contains(subDir)) {
            QTC_ASSERT(nodeHash[subDir]->nodeType() == FolderNodeType, return);
            parentFolder = static_cast<FolderNode *>(nodeHash[subDir]);
        } else {
            parentFolder = insertFolderNode(QDir(subDir), nodeHash);
            if (parentFolder == 0) {
                // No node gets created for the root folder
                parentFolder = m_rootNode;
            }
        }
        QTC_ASSERT(parentFolder, return);
        if (oldParentFolder && (oldParentFolder != parentFolder) && !fileNodes.isEmpty()) {
            // AutotoolsProjectNode::addFileNodes() is a very expensive operation. It is
            // important to collect as much file nodes of the same parent folder as
            // possible before invoking it.
            oldParentFolder->addFileNodes(fileNodes);
            fileNodes.clear();
        }

        // Add file node
        const QString filePath = directory.absoluteFilePath(file);
        if (nodeHash.contains(filePath)) {
            nodeHash.remove(filePath);
        } else if (file == QLatin1String("Makefile.am") || file == QLatin1String("configure.ac")) {
            fileNodes.append(new FileNode(Utils::FileName::fromString(filePath),
                                          ProjectFileType, false));
        } else {
            fileNodes.append(new FileNode(Utils::FileName::fromString(filePath),
                                          ResourceType, false));
        }
    }

    if (parentFolder && !fileNodes.isEmpty())
        parentFolder->addFileNodes(fileNodes);

    // Remove unused file nodes and empty folder nodes
    QHash<QString, Node *>::const_iterator it = nodeHash.constBegin();
    while (it != nodeHash.constEnd()) {
        if ((*it)->nodeType() == FileNodeType) {
            FileNode *fileNode = static_cast<FileNode *>(*it);
            FolderNode* parent = fileNode->parentFolderNode();
            parent->removeFileNodes(QList<FileNode *>() << fileNode);

            // Remove all empty parent folders
            while (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty()) {
                FolderNode *grandParent = parent->parentFolderNode();
                grandParent->removeFolderNodes(QList<FolderNode *>() << parent);
                parent = grandParent;
                if (parent == m_rootNode)
                    break;
            }
        }
        ++it;
    }
}

FolderNode *AutotoolsProject::insertFolderNode(const QDir &nodeDir, QHash<QString, Node *> &nodes)
{
    const Utils::FileName nodePath = Utils::FileName::fromString(nodeDir.absolutePath());
    QFileInfo rootInfo = m_rootNode->filePath().toFileInfo();
    const Utils::FileName rootPath = Utils::FileName::fromString(rootInfo.absolutePath());

    // Do not create a folder for the root node
    if (rootPath == nodePath)
        return 0;

    FolderNode *folder = new FolderNode(nodePath);
    QDir dir(nodeDir);
    folder->setDisplayName(dir.dirName());

    // Get parent-folder. If it does not exist, create it recursively.
    // Take care that the m_rootNode is considered as top folder.
    FolderNode *parentFolder = m_rootNode;
    if ((rootPath != folder->filePath()) && dir.cdUp()) {
        const QString parentDir = dir.absolutePath();
        if (!nodes.contains(parentDir)) {
            FolderNode *insertedFolder = insertFolderNode(parentDir, nodes);
            if (insertedFolder != 0)
                parentFolder = insertedFolder;
        } else {
            QTC_ASSERT(nodes[parentDir]->nodeType() == FolderNodeType, return 0);
            parentFolder = static_cast<FolderNode *>(nodes[parentDir]);
        }
    }

    parentFolder->addFolderNodes(QList<FolderNode *>() << folder);
    nodes.insert(nodePath.toString(), folder);

    return folder;
}

QList<Node *> AutotoolsProject::nodes(FolderNode *parent) const
{
    QList<Node *> list;
    QTC_ASSERT(parent != 0, return list);

    foreach (FolderNode *folder, parent->subFolderNodes()) {
        list.append(nodes(folder));
        list.append(folder);
    }
    foreach (FileNode *file, parent->fileNodes())
        list.append(file);

    return list;
}

static QStringList filterIncludes(const QString &absSrc, const QString &absBuild,
                                  const QStringList &in)
{
    QStringList result;
    foreach (const QString i, in) {
        QString out = i;
        out.replace(QLatin1String("$(top_srcdir)"), absSrc);
        out.replace(QLatin1String("$(abs_top_srcdir)"), absSrc);

        out.replace(QLatin1String("$(top_builddir)"), absBuild);
        out.replace(QLatin1String("$(abs_top_builddir)"), absBuild);

        result << out;
    }

    return result;
}

void AutotoolsProject::updateCppCodeModel()
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    m_codeModelFuture.cancel();
    CppTools::ProjectInfo pInfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pInfo);

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit())) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    ppBuilder.setQtVersion(activeQtVersion);
    const QStringList cflags = m_makefileParserThread->cflags();
    QStringList cxxflags = m_makefileParserThread->cxxflags();
    if (cxxflags.isEmpty())
        cxxflags = cflags;
    ppBuilder.setCFlags(cflags);
    ppBuilder.setCxxFlags(cxxflags);

    const QString absSrc = projectDirectory().toString();
    const Target *target = activeTarget();
    const QString absBuild = (target && target->activeBuildConfiguration())
            ? target->activeBuildConfiguration()->buildDirectory().toString() : QString();

    ppBuilder.setIncludePaths(filterIncludes(absSrc, absBuild, m_makefileParserThread->includePaths()));
    ppBuilder.setDefines(m_makefileParserThread->defines());

    const QList<Core::Id> languages = ppBuilder.createProjectPartsForFiles(m_files);
    foreach (Core::Id language, languages)
        setProjectLanguage(language, true);

    pInfo.finish();
    m_codeModelFuture = modelManager->updateProjectInfo(pInfo);
}
