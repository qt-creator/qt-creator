/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"
#include "autotoolsmanager.h"
#include "autotoolsprojectnode.h"
#include "autotoolsprojectfile.h"
#include "autotoolsopenprojectwizard.h"
#include "makestep.h"
#include "makefileparserthread.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/ModelManagerInterface.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

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

const char TOOLCHAIN_KEY[] = "AutotoolsProjectManager.AutotoolsProject.Toolchain";

AutotoolsProject::AutotoolsProject(AutotoolsManager *manager, const QString &fileName) :
    m_manager(manager),
    m_fileName(fileName),
    m_files(),
    m_file(new AutotoolsProjectFile(this, m_fileName)),
    m_rootNode(new AutotoolsProjectNode(this, m_file)),
    m_fileWatcher(new Utils::FileSystemWatcher(this)),
    m_watchedFiles(),
    m_makefileParserThread(0),
    m_toolChain(0)
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

void AutotoolsProject::setToolChain(ToolChain *tc)
{
    if (m_toolChain == tc)
        return;

    m_toolChain = tc;

    foreach (Target *t, targets()) {
        foreach (BuildConfiguration *bc, t->buildConfigurations())
            bc->setToolChain(tc);
    }

    emit toolChainChanged(m_toolChain);
}

ToolChain *AutotoolsProject::toolChain() const
{
    return m_toolChain;
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

AutotoolsTarget *AutotoolsProject::activeTarget() const
{
    return static_cast<AutotoolsTarget *>(Project::activeTarget());
}

QString AutotoolsProject::defaultBuildDirectory() const
{
    return projectDirectory();
}

QList<BuildConfigWidget *> AutotoolsProject::subConfigWidgets()
{
    return QList<BuildConfigWidget *>() << new BuildEnvironmentWidget;
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

QVariantMap AutotoolsProject::toMap() const
{
    QVariantMap map = Project::toMap();
    map.insert(QLatin1String(TOOLCHAIN_KEY), m_toolChain ? m_toolChain->id() : QString());
    return map;
}

// This function, is called at the very beginning, to
// restore the settings if there are some stored.
bool AutotoolsProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Check if this project was already loaded by checking
    // if there already exists a .user file.
    bool hasUserFile = activeTarget();
    if (!hasUserFile) {
        AutotoolsTargetFactory *factory =
                ExtensionSystem::PluginManager::getObject<AutotoolsTargetFactory>();
        AutotoolsTarget *t = factory->create(this, Core::Id(Constants::DEFAULT_AUTOTOOLS_TARGET_ID));

        QTC_ASSERT(t, return false);
        QTC_ASSERT(t->activeBuildConfiguration(), return false);

        // Ask the user for where he/she wants to build it.
        QPointer<AutotoolsOpenProjectWizard> wizard = new AutotoolsOpenProjectWizard(m_manager, projectDirectory());
        if (!wizard->exec() == QDialog::Accepted)
            return false;

        AutotoolsBuildConfiguration *bc =
                static_cast<AutotoolsBuildConfiguration *>(t->buildConfigurations().at(0));
        if (!wizard->buildDirectory().isEmpty())
            bc->setBuildDirectory(wizard->buildDirectory());

        addTarget(t);
    }

    // Toolchain
    QString id = map.value(QLatin1String(TOOLCHAIN_KEY)).toString();
    const ToolChainManager *toolChainManager = ToolChainManager::instance();

    if (!id.isNull()) {
        setToolChain(toolChainManager->findToolChain(id));
    } else {
        Abi abi = Abi::hostAbi();
        abi = Abi(abi.architecture(), abi.os(), Abi::UnknownFlavor,
                               abi.binaryFormat(), abi.wordWidth() == 32 ? 32 : 0);
        QList<ToolChain *> tcs = toolChainManager->findToolChains(abi);
        if (tcs.isEmpty())
            tcs = toolChainManager->toolChains();
        if (!tcs.isEmpty())
            setToolChain(tcs.at(0));
    }

    connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(onFileChanged(QString)));

    // Load the project tree structure.
    loadProjectTree();

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
        if (file.contains(QLatin1String(".moc")))
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
        if (nodeHash.contains(filePath)) {
            nodeHash.remove(filePath);
        } else {
            fileNodes.append(new FileNode(filePath, ResourceType, false));
        }
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

    if (m_toolChain) {
        const QList<HeaderPath> allHeaderPaths = m_toolChain->systemHeaderPaths();
        foreach (const HeaderPath &headerPath, allHeaderPaths) {
            if (headerPath.kind() == HeaderPath::FrameworkHeaderPath) {
                allFrameworkPaths.append(headerPath.path());
            } else {
                allIncludePaths.append(headerPath.path());
            }
        }
    }

    CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelManager->projectInfo(this);

    const bool update = (pinfo.includePaths() != allIncludePaths)
            || (pinfo.sourceFiles() != m_files)
            || (pinfo.defines() != m_toolChain->predefinedMacros(QStringList()))
            || (pinfo.frameworkPaths() != allFrameworkPaths);
    if (update) {
        pinfo.clearProjectParts();
        CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(
                    new CPlusPlus::CppModelManagerInterface::ProjectPart);
        part->includePaths = allIncludePaths;
        part->sourceFiles = m_files;
        if (m_toolChain)
            part->defines = m_toolChain->predefinedMacros(QStringList());
        part->frameworkPaths = allFrameworkPaths;
        part->language = CPlusPlus::CppModelManagerInterface::CXX;
        pinfo.appendProjectPart(part);

        modelManager->updateProjectInfo(pinfo);
        modelManager->updateSourceFiles(m_files);
    }
}
