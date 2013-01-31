/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/ModelManagerInterface.h>
#include <coreplugin/icore.h>
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
    m_rootNode(new AutotoolsProjectNode(this, m_file)),
    m_fileWatcher(new Utils::FileSystemWatcher(this)),
    m_watchedFiles(),
    m_makefileParserThread(0)
{
    setProjectContext(Core::Context(Constants::PROJECT_CONTEXT));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

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

Core::Id AutotoolsProject::id() const
{
    return Core::Id(Constants::AUTOTOOLS_PROJECT_ID);
}

Core::IDocument *AutotoolsProject::document() const
{
    return m_file;
}

IProjectManager *AutotoolsProject::projectManager() const
{
    return m_manager;
}

QString AutotoolsProject::defaultBuildDirectory() const
{
    return projectDirectory();
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
bool AutotoolsProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(onFileChanged(QString)));

    // Load the project tree structure.
    loadProjectTree();

    Kit *defaultKit = KitManager::instance()->defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    return true;
}

void AutotoolsProject::loadProjectTree()
{
    if (m_makefileParserThread != 0) {
        // The thread is still busy parsing a previus configuration.
        // Wait until the thread has been finished and delete it.
        // TODO: Discuss whether blocking is acceptable.
        disconnect(m_makefileParserThread, SIGNAL(finished()),
                   this, SLOT(makefileParsingFinished()));
        m_makefileParserThread->wait();
        delete m_makefileParserThread;
        m_makefileParserThread = 0;
    }

    // Parse the makefile asynchronously in a thread
    m_makefileParserThread = new MakefileParserThread(m_fileName);

    connect(m_makefileParserThread, SIGNAL(started()),
            this, SLOT(makefileParsingStarted()));

    connect(m_makefileParserThread, SIGNAL(finished()),
            this, SLOT(makefileParsingFinished()));
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
        nodeHash.insert(node->path(), node);

    // Add the sources to the filenode project tree. Sources
    // inside the same directory are grouped into a folder-node.
    const QString baseDir = directory.absolutePath();

    QList<FileNode *> fileNodes;
    FolderNode *parentFolder = 0;
    FolderNode *oldParentFolder = 0;

    foreach (const QString& file, files) {
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
        QTC_ASSERT(parentFolder != 0, return);
        if ((oldParentFolder != parentFolder) && !fileNodes.isEmpty()) {
            // AutotoolsProjectNode::addFileNodes() is a very expensive operation. It is
            // important to collect as much file nodes of the same parent folder as
            // possible before invoking it.
            m_rootNode->addFileNodes(fileNodes, oldParentFolder);
            fileNodes.clear();
        }

        // Add file node
        const QString filePath = directory.absoluteFilePath(file);
        if (nodeHash.contains(filePath))
            nodeHash.remove(filePath);
        else
            fileNodes.append(new FileNode(filePath, ResourceType, false));
    }

    if (!fileNodes.isEmpty())
        m_rootNode->addFileNodes(fileNodes, parentFolder);

    // Remove unused file nodes and empty folder nodes
    QHash<QString, Node *>::const_iterator it = nodeHash.constBegin();
    while (it != nodeHash.constEnd()) {
        if ((*it)->nodeType() == FileNodeType) {
            FileNode *fileNode = static_cast<FileNode *>(*it);
            FolderNode* parent = fileNode->parentFolderNode();
            m_rootNode->removeFileNodes(QList<FileNode *>() << fileNode, parent);

            // Remove all empty parent folders
            while (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty()) {
                FolderNode *grandParent = parent->parentFolderNode();
                m_rootNode->removeFolderNodes(QList<FolderNode *>() << parent, grandParent);
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
    const QString nodePath = nodeDir.absolutePath();
    QFileInfo rootInfo(m_rootNode->path());
    const QString rootPath = rootInfo.absolutePath();

    // Do not create a folder for the root node
    if (rootPath == nodePath)
        return 0;

    FolderNode *folder = new FolderNode(nodePath);
    QDir dir(nodeDir);
    folder->setDisplayName(dir.dirName());

    // Get parent-folder. If it does not exist, create it recursively.
    // Take care that the m_rootNode is considered as top folder.
    FolderNode *parentFolder = m_rootNode;
    if ((rootPath != folder->path()) && dir.cdUp()) {
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

    m_rootNode->addFolderNodes(QList<FolderNode *>() << folder, parentFolder);
    nodes.insert(nodePath, folder);

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

void AutotoolsProject::updateCppCodeModel()
{
    CPlusPlus::CppModelManagerInterface *modelManager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (!modelManager)
        return;

    QStringList allIncludePaths = m_makefileParserThread->includePaths();
    QStringList allFrameworkPaths;
    QByteArray macros;

    QStringList cxxflags; // FIXME: Autotools should be able to do better than this!

    if (activeTarget()) {
        ProjectExplorer::Kit *k = activeTarget()->kit();
        ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
        if (tc) {
            const QList<HeaderPath> allHeaderPaths = tc->systemHeaderPaths(cxxflags,
                                                                           SysRootKitInformation::sysRoot(k));
            foreach (const HeaderPath &headerPath, allHeaderPaths) {
                if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                    allFrameworkPaths.append(headerPath.path());
                else
                    allIncludePaths.append(headerPath.path());
            }
            macros = tc->predefinedMacros(cxxflags);
            macros += '\n';
        }
    }

    CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);

    const bool update = (pinfo.includePaths() != allIncludePaths)
            || (pinfo.sourceFiles() != m_files)
            || (pinfo.defines() != macros)
            || (pinfo.frameworkPaths() != allFrameworkPaths);
    if (update) {
        pinfo.clearProjectParts();
        CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(
                    new CPlusPlus::CppModelManagerInterface::ProjectPart);
        part->includePaths = allIncludePaths;
        part->sourceFiles = m_files;
        part->defines = macros;
        part->frameworkPaths = allFrameworkPaths;
        part->language = CPlusPlus::CppModelManagerInterface::ProjectPart::CXX11;
        pinfo.appendProjectPart(part);

        modelManager->updateProjectInfo(pinfo);
        modelManager->updateSourceFiles(m_files);
    }
}
