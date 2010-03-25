/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qt4project.h"

#include "profilereader.h"
#include "qt4projectmanager.h"
#include "makestep.h"
#include "qmakestep.h"
#include "qt4runconfiguration.h"
#include "qt4nodes.h"
#include "qt4projectconfigwidget.h"
#include "qt4projectmanagerconstants.h"
#include "projectloadwizard.h"
#include "qt4buildconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/project.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

enum { debug = 0 };

namespace Qt4ProjectManager {
namespace Internal {

// Qt4ProjectFiles: Struct for (Cached) lists of files in a project
struct Qt4ProjectFiles {
    void clear();
    bool equals(const Qt4ProjectFiles &f) const;

    QStringList files[ProjectExplorer::FileTypeSize];
    QStringList generatedFiles[ProjectExplorer::FileTypeSize];
    QStringList proFiles;
};

void Qt4ProjectFiles::clear()
{
    for (int i = 0; i < FileTypeSize; ++i) {
        files[i].clear();
        generatedFiles[i].clear();
    }
    proFiles.clear();
}

bool Qt4ProjectFiles::equals(const Qt4ProjectFiles &f) const
{
    for (int i = 0; i < FileTypeSize; ++i)
        if (files[i] != f.files[i] || generatedFiles[i] != f.generatedFiles[i])
            return false;
    if (proFiles != f.proFiles)
        return false;
    return true;
}

inline bool operator==(const Qt4ProjectFiles &f1, const Qt4ProjectFiles &f2)
{       return f1.equals(f2); }

inline bool operator!=(const Qt4ProjectFiles &f1, const Qt4ProjectFiles &f2)
{       return !f1.equals(f2); }

QDebug operator<<(QDebug d, const  Qt4ProjectFiles &f)
{
    QDebug nsp = d.nospace();
    nsp << "Qt4ProjectFiles: proFiles=" <<  f.proFiles << '\n';
    for (int i = 0; i < FileTypeSize; ++i)
        nsp << "Type " << i << " files=" << f.files[i] <<  " generated=" << f.generatedFiles[i] << '\n';
    return d;
}

// A visitor to collect all files of a project in a Qt4ProjectFiles struct
class ProjectFilesVisitor : public ProjectExplorer::NodesVisitor
{
    Q_DISABLE_COPY(ProjectFilesVisitor)
    ProjectFilesVisitor(Qt4ProjectFiles *files);
public:

    static void findProjectFiles(Qt4ProFileNode *rootNode, Qt4ProjectFiles *files);

    void visitProjectNode(ProjectNode *projectNode);
    void visitFolderNode(FolderNode *folderNode);

private:
    Qt4ProjectFiles *m_files;
};

ProjectFilesVisitor::ProjectFilesVisitor(Qt4ProjectFiles *files) :
    m_files(files)
{
}

void ProjectFilesVisitor::findProjectFiles(Qt4ProFileNode *rootNode, Qt4ProjectFiles *files)
{
    files->clear();
    ProjectFilesVisitor visitor(files);
    rootNode->accept(&visitor);
    for (int i = 0; i < FileTypeSize; ++i) {
        qSort(files->files[i]);
        qSort(files->generatedFiles[i]);
    }
    qSort(files->proFiles);
}

void ProjectFilesVisitor::visitProjectNode(ProjectNode *projectNode)
{
    const QString path = projectNode->path();
    if (!m_files->proFiles.contains(path))
        m_files->proFiles.append(path);
    visitFolderNode(projectNode);
}

void ProjectFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    foreach (FileNode *fileNode, folderNode->fileNodes()) {
        const QString path = fileNode->path();
        const int type = fileNode->fileType();
        QStringList &targetList = fileNode->isGenerated() ? m_files->generatedFiles[type] : m_files->files[type];
        if (!targetList.contains(path))
            targetList.push_back(path);
    }
}

}
}

// ----------- Qt4ProjectFile
Qt4ProjectFile::Qt4ProjectFile(Qt4Project *project, const QString &filePath, QObject *parent)
    : Core::IFile(parent),
      m_mimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)),
      m_project(project),
      m_filePath(filePath)
{
}

bool Qt4ProjectFile::save(const QString &)
{
    // This is never used
    return false;
}

QString Qt4ProjectFile::fileName() const
{
    return m_filePath;
}

QString Qt4ProjectFile::defaultPath() const
{
    return QString();
}

QString Qt4ProjectFile::suggestedFileName() const
{
    return QString();
}

QString Qt4ProjectFile::mimeType() const
{
    return m_mimeType;
}

bool Qt4ProjectFile::isModified() const
{
    return false; // we save after changing anyway
}

bool Qt4ProjectFile::isReadOnly() const
{
    QFileInfo fi(m_filePath);
    return !fi.isWritable();
}

bool Qt4ProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IFile::ReloadBehavior Qt4ProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

void Qt4ProjectFile::reload(ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag)
    Q_UNUSED(type)
}

/*!
  \class Qt4Project

  Qt4Project manages information about an individual Qt 4 (.pro) project file.
  */

Qt4Project::Qt4Project(Qt4Manager *manager, const QString& fileName) :
    m_manager(manager),
    m_rootProjectNode(0),
    m_nodesWatcher(new Internal::Qt4NodesWatcher(this)),
    m_targetFactory(new Qt4TargetFactory(this)),
    m_fileInfo(new Qt4ProjectFile(this, fileName, this)),
    m_isApplication(true),
    m_projectFiles(new Qt4ProjectFiles),
    m_proFileOption(0),
    m_asyncUpdateFutureInterface(0),
    m_pendingEvaluateFuturesCount(0),
    m_asyncUpdateState(NoState),
    m_cancelEvaluate(false)
{
    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(3000);
    connect(&m_asyncUpdateTimer, SIGNAL(timeout()), this, SLOT(asyncUpdate()));

    setSupportedTargetIds(QtVersionManager::instance()->supportedTargetIds());
}

Qt4Project::~Qt4Project()
{
    m_asyncUpdateState = ShuttingDown;
    m_manager->unregisterProject(this);
    delete m_projectFiles;
    m_cancelEvaluate = true;
    delete m_rootProjectNode;
}

void Qt4Project::updateFileList()
{
    Qt4ProjectFiles newFiles;
    ProjectFilesVisitor::findProjectFiles(m_rootProjectNode, &newFiles);
    if (newFiles != *m_projectFiles) {
        *m_projectFiles = newFiles;
        emit fileListChanged();
        if (debug)
            qDebug() << Q_FUNC_INFO << *m_projectFiles;
    }
}

bool Qt4Project::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Prune targets without buildconfigurations:
    // This can happen esp. when updating from a old version of Qt Creator
    QList<Target *>ts(targets());
    foreach (Target *t, ts) {
        if (t->buildConfigurations().isEmpty()) {
            qWarning() << "Removing" << t->id() << "since it has no buildconfigurations!";
            removeTarget(t);
        }
    }

    // Add buildconfigurations so we can parse the pro-files.
    if (targets().isEmpty())
        addDefaultBuild();

    if (targets().isEmpty()) {
        qWarning() << "Unable to create targets!";
        return false;
    }

    Q_ASSERT(activeTarget());
    Q_ASSERT(activeTarget()->activeBuildConfiguration());

    m_manager->registerProject(this);

    m_rootProjectNode = new Qt4ProFileNode(this, m_fileInfo->fileName(), this);
    m_rootProjectNode->registerWatcher(m_nodesWatcher);

    update();
    updateFileList();
    // This might be incorrect, need a full update
    updateCodeModel();

    checkForNewApplicationProjects();
    checkForDeletedApplicationProjects();

    foreach (Target *t, targets())
        onAddedTarget(t);

    setSupportedTargetIds(QtVersionManager::instance()->supportedTargetIds());

    // Setup Qt versions supported (== possible targets).
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(onAddedTarget(ProjectExplorer::Target*)));

    connect(QtVersionManager::instance(), SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(qtVersionsChanged()));

    connect(m_nodesWatcher, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *)),
            this, SIGNAL(proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *)));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetWasChanged()));

    return true;
}

Qt4TargetFactory *Qt4Project::targetFactory() const
{
    return m_targetFactory;
}

Qt4Target *Qt4Project::activeTarget() const
{
    return static_cast<Qt4Target *>(Project::activeTarget());
}

namespace {
    class FindQt4ProFiles: protected ProjectExplorer::NodesVisitor {
        QList<Qt4ProFileNode *> m_proFiles;

    public:
        QList<Qt4ProFileNode *> operator()(ProjectNode *root)
        {
            m_proFiles.clear();
            root->accept(this);
            return m_proFiles;
        }

    protected:
        virtual void visitProjectNode(ProjectNode *projectNode)
        {
            if (Qt4ProFileNode *pro = qobject_cast<Qt4ProFileNode *>(projectNode))
                m_proFiles.append(pro);
        }
    };
}

void Qt4Project::onAddedTarget(ProjectExplorer::Target *t)
{
    Q_ASSERT(t);
    Qt4Target *qt4target = qobject_cast<Qt4Target *>(t);
    Q_ASSERT(qt4target);
    connect(qt4target, SIGNAL(buildDirectoryInitialized()),
            this, SIGNAL(buildDirectoryInitialized()));
    connect(qt4target, SIGNAL(proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4Target*)),
            this, SLOT(proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4Target*)));
}

void Qt4Project::proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4Target *target)
{
    if (activeTarget() == target)
        scheduleAsyncUpdate();
}

/// equalFileList compares two file lists ignoring
/// <configuration> without generating temporary lists

bool Qt4Project::equalFileList(const QStringList &a, const QStringList &b)
{
    if (abs(a.length() - b.length()) > 1)
        return false;
    QStringList::const_iterator ait = a.constBegin();
    QStringList::const_iterator bit = b.constBegin();
    QStringList::const_iterator aend = a.constEnd();
    QStringList::const_iterator bend = b.constEnd();

    while (ait != aend && bit != bend) {
        if (*ait == QLatin1String("<configuration>"))
            ++ait;
        else if (*bit == QLatin1String("<configuration>"))
            ++bit;
        else if (*ait == *bit)
            ++ait, ++bit;
        else
           return false;
    }
    return (ait == aend && bit == bend);
}

void Qt4Project::updateCodeModel()
{
    if (debug)
        qDebug()<<"Qt4Project::updateCodeModel()";

    m_codeModelFuture.cancel();

    if (!activeTarget() || !activeTarget()->activeBuildConfiguration())
        return;

    Qt4BuildConfiguration *activeBC = activeTarget()->activeBuildConfiguration();

    CppTools::CppModelManagerInterface *modelmanager =
        ExtensionSystem::PluginManager::instance()
            ->getObject<CppTools::CppModelManagerInterface>();

    if (!modelmanager)
        return;

    // Collect global headers/defines
    QStringList predefinedIncludePaths;
    QStringList predefinedFrameworkPaths;
    QByteArray predefinedMacros;

    ToolChain *tc = activeBC->toolChain();
    if (tc) {
        predefinedMacros = tc->predefinedMacros();
        //qDebug()<<"Predefined Macros";
        //qDebug()<<tc->predefinedMacros();
        //qDebug()<<"";
        //qDebug()<<"System Header Paths";
        //foreach(const HeaderPath &hp, tc->systemHeaderPaths())
        //    qDebug()<<hp.path();

        foreach (const HeaderPath &headerPath, tc->systemHeaderPaths()) {
            if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                predefinedFrameworkPaths.append(headerPath.path());
            else
                predefinedIncludePaths.append(headerPath.path());
        }
    }

    FindQt4ProFiles findQt4ProFiles;
    QList<Qt4ProFileNode *> proFiles = findQt4ProFiles(rootProjectNode());
    QByteArray allDefinedMacros = predefinedMacros;
    QStringList allIncludePaths;
    QStringList allFrameworkPaths = predefinedFrameworkPaths;
    QStringList allPrecompileHeaders;

    // Collect per .pro file information
    m_codeModelInfo.clear();
    foreach (Qt4ProFileNode *pro, proFiles) {
        Internal::CodeModelInfo info;
        info.defines = predefinedMacros;
        info.frameworkPaths = predefinedFrameworkPaths;

        info.precompiledHeader = pro->variableValue(PrecompiledHeaderVar);

        allPrecompileHeaders.append(info.precompiledHeader);

        // Add custom defines

        foreach (const QString &def, pro->variableValue(DefinesVar)) {
            allDefinedMacros += "#define ";
            info.defines += "#define ";
            const int index = def.indexOf(QLatin1Char('='));
            if (index == -1) {
                allDefinedMacros += def.toLatin1();
                allDefinedMacros += " 1\n";
                info.defines += def.toLatin1();
                info.defines += " 1\n";
            } else {
                const QString name = def.left(index);
                const QString value = def.mid(index + 1);
                allDefinedMacros += name.toLatin1();
                allDefinedMacros += ' ';
                allDefinedMacros += value.toLocal8Bit();
                allDefinedMacros += '\n';
                info.defines += name.toLatin1();
                info.defines += ' ';
                info.defines += value.toLocal8Bit();
                info.defines += '\n';
            }
        }

        const QStringList proIncludePaths = pro->variableValue(IncludePathVar);
        foreach (const QString &includePath, proIncludePaths) {
            if (!allIncludePaths.contains(includePath))
                allIncludePaths.append(includePath);
            if (!info.includes.contains(includePath))
                info.includes.append(includePath);
        }

#if 0 // Experimental PKGCONFIG support
        { // Pkg Config support
            QStringList pkgConfig = pro->variableValue(PkgConfigVar);
            if (!pkgConfig.isEmpty()) {
                pkgConfig.prepend("--cflags-only-I");
                QProcess process;
                process.start("pkg-config", pkgConfig);
                process.waitForFinished();
                QString result = process.readAllStandardOutput();
                foreach(const QString &part, result.trimmed().split(' ', QString::SkipEmptyParts)) {
                    info.includes.append(part.mid(2)); // Chop off "-I"
                }
            }
        }
#endif

        // Add mkspec directory
        info.includes.append(activeBC->qtVersion()->mkspecPath());
        info.includes.append(predefinedIncludePaths);

//        qDebug()<<"Dumping code model information";
//        qDebug()<<"for .pro file"<< pro->path();
//        qDebug()<<info.defines;
//        qDebug()<<info.includes;
//        qDebug()<<info.frameworkPaths;
//        qDebug()<<"\n";

#if 0
        //Disable for now, we need better .pro file parsing first, and code model
        //support to access this information

        // TODO this is wastefull
        // only save it per .pro file, and on being asked
        // search for the .pro file that has that file
        foreach (FileNode *fileNode, pro->fileNodes()) {
            const QString path = fileNode->path();
            const int type = fileNode->fileType();
            if (type == HeaderType || type == SourceType) {
                m_codeModelInfo.insert(path, info);
            }
        }
#endif
    }

    // Add mkspec directory
    allIncludePaths.append(activeBC->qtVersion()->mkspecPath());

    allIncludePaths.append(predefinedIncludePaths);

    QStringList files;
    files += m_projectFiles->files[HeaderType];
    files += m_projectFiles->generatedFiles[HeaderType];
    files += m_projectFiles->files[SourceType];
    files += m_projectFiles->generatedFiles[SourceType];

    CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);

    //qDebug()<<"Using precompiled header"<<allPrecompileHeaders;

    bool fileList = equalFileList(pinfo.sourceFiles, files);

    if (pinfo.defines == allDefinedMacros
        && pinfo.includePaths == allIncludePaths
        && pinfo.frameworkPaths == allFrameworkPaths
        && fileList
        && pinfo.precompiledHeaders == allPrecompileHeaders) {
        // Nothing to update...
    } else {
        pinfo.sourceFiles.clear();
        if (pinfo.defines != allDefinedMacros
            || pinfo.includePaths != allIncludePaths
            || pinfo.frameworkPaths != allFrameworkPaths
            || pinfo.precompiledHeaders != allPrecompileHeaders)
        {
            pinfo.sourceFiles.append(QLatin1String("<configuration>"));
        }

        //pinfo.defines = predefinedMacros;
        pinfo.defines = allDefinedMacros;
        pinfo.includePaths = allIncludePaths;
        pinfo.frameworkPaths = allFrameworkPaths;
        pinfo.sourceFiles += files;
        pinfo.precompiledHeaders = allPrecompileHeaders;

        modelmanager->updateProjectInfo(pinfo);
        m_codeModelFuture = modelmanager->updateSourceFiles(pinfo.sourceFiles);
    }
}

void Qt4Project::qtVersionsChanged()
{
    setSupportedTargetIds(QtVersionManager::instance()->supportedTargetIds());
}

QByteArray Qt4Project::predefinedMacros(const QString &fileName) const
{
    QMap<QString, CodeModelInfo>::const_iterator it = m_codeModelInfo.constFind(fileName);
    if (it == m_codeModelInfo.constEnd())
        return QByteArray();
    else
        return (*it).defines;
}

QStringList Qt4Project::includePaths(const QString &fileName) const
{
    QMap<QString, CodeModelInfo>::const_iterator it = m_codeModelInfo.constFind(fileName);
    if (it == m_codeModelInfo.constEnd())
        return QStringList();
    else
        return (*it).includes;
}

QStringList Qt4Project::frameworkPaths(const QString &fileName) const
{
    QMap<QString, CodeModelInfo>::const_iterator it = m_codeModelInfo.constFind(fileName);
    if (it == m_codeModelInfo.constEnd())
        return QStringList();
    else
        return (*it).frameworkPaths;
}

///*!
//  Updates complete project
//  */
void Qt4Project::update()
{
    if (debug)
        qDebug()<<"Doing sync update";
    m_rootProjectNode->update();
    if (debug)
        qDebug()<<"State is now Base";
    m_asyncUpdateState = Base;
}

void Qt4Project::scheduleAsyncUpdate(Qt4ProFileNode *node)
{
    if (debug)
        qDebug()<<"schduleAsyncUpdate (node)";
    Q_ASSERT(m_asyncUpdateState != NoState);

    if (m_cancelEvaluate) {
        if (debug)
            qDebug()<<"  Already canceling, nothing to do";
        // A cancel is in progress
        // That implies that a full update is going to happen afterwards
        // So we don't need to do anything
        return;
    }

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        // Just postpone
        if (debug)
            qDebug()<<"  full update pending, restarting timer";
        m_asyncUpdateTimer.start();
    } else if (m_asyncUpdateState == AsyncPartialUpdatePending
               || m_asyncUpdateState == Base) {
        if (debug)
            qDebug()<<"  adding node to async update list, setting state to AsyncPartialUpdatePending";
        // Add the node
        m_asyncUpdateState = AsyncPartialUpdatePending;

        QList<Internal::Qt4ProFileNode *>::iterator it;
        bool add = true;
        if (debug)
            qDebug()<<"scheduleAsyncUpdate();"<<m_partialEvaluate.size()<<"nodes";
        it = m_partialEvaluate.begin();
        while (it != m_partialEvaluate.end()) {
            if (*it == node) {
                add = false;
                break;
            } else if (node->isParent(*it)) { // We already have the parent in the list, nothing to do
                add = false;
                break;
            } else if ((*it)->isParent(node)) { // The node is the parent of a child already in the list
                it = m_partialEvaluate.erase(it);
            } else {
                ++it;
            }
        }

        if (add)
            m_partialEvaluate.append(node);
        // and start the timer anew
        m_asyncUpdateTimer.start();
    } else if (m_asyncUpdateState == AsyncUpdateInProgress) {
        // A update is in progress
        // And this slot only gets called if a file changed on disc
        // So we'll play it safe and schedule a complete evaluate
        // This might trigger if due to version control a few files
        // change a partial update gets in progress and then another
        // batch of changes come in, which triggers a full update
        // even if that's not really needed
        if (debug)
            qDebug()<<"  Async update in progress, scheduling new one afterwards";
        scheduleAsyncUpdate();
    }
}

void Qt4Project::scheduleAsyncUpdate()
{
    if (debug)
        qDebug()<<"scheduleAsyncUpdate";
    Q_ASSERT(m_asyncUpdateState != NoState);
    if (m_cancelEvaluate) { // we are in progress of canceling
                            // and will start the evaluation after that
        if (debug)
            qDebug()<<"  canceling is in progress, doing nothing";
        return;
    }
    if (m_asyncUpdateState == AsyncUpdateInProgress) {
        if (debug)
            qDebug()<<"  update in progress, canceling and setting state to full update pending";
        m_cancelEvaluate = true;
        m_asyncUpdateState = AsyncFullUpdatePending;
        return;
    }

    if (debug)
        qDebug()<<"  starting timer for full update, setting state to full update pending";
    m_partialEvaluate.clear();
    m_asyncUpdateState = AsyncFullUpdatePending;
    m_asyncUpdateTimer.start();
}


void Qt4Project::incrementPendingEvaluateFutures()
{
    ++m_pendingEvaluateFuturesCount;
    if (debug)
        qDebug()<<"incrementPendingEvaluateFutures to"<<m_pendingEvaluateFuturesCount;

    m_asyncUpdateFutureInterface->setProgressRange(m_asyncUpdateFutureInterface->progressMinimum(),
                                                  m_asyncUpdateFutureInterface->progressMaximum() + 1);
}

void Qt4Project::decrementPendingEvaluateFutures()
{
    --m_pendingEvaluateFuturesCount;

    if (debug)
        qDebug()<<"decrementPendingEvaluateFutures to"<<m_pendingEvaluateFuturesCount;

    m_asyncUpdateFutureInterface->setProgressValue(m_asyncUpdateFutureInterface->progressValue() + 1);
    if (m_pendingEvaluateFuturesCount == 0) {
        if (debug)
            qDebug()<<"  WOHOO, no pending futures, cleaning up";
        // We are done!
        if (debug)
            qDebug()<<"  reporting finished";
        m_asyncUpdateFutureInterface->reportFinished();
        delete m_asyncUpdateFutureInterface;
        m_asyncUpdateFutureInterface = 0;
        m_cancelEvaluate = false;

        // TODO clear the profile cache ?
        if (m_asyncUpdateState == AsyncFullUpdatePending || m_asyncUpdateState == AsyncPartialUpdatePending) {
            if (debug)
                qDebug()<<"  Oh update is pending start the timer";
            m_asyncUpdateTimer.start();
        } else  if (m_asyncUpdateState != ShuttingDown){
            // After beeing done, we need to call:
            updateFileList();
            updateCodeModel();
            checkForNewApplicationProjects();
            checkForDeletedApplicationProjects();
            if (debug)
                qDebug()<<"  Setting state to Base";
            m_asyncUpdateState = Base;
        }
    }
}

bool Qt4Project::wasEvaluateCanceled()
{
    return m_cancelEvaluate;
}

QString Qt4Project::defaultTopLevelBuildDirectory() const
{
    QFileInfo info(file()->fileName());
    return projectDirectory() + QChar('/') + info.baseName() + QLatin1String("-build");
}

void Qt4Project::asyncUpdate()
{
    if (debug)
        qDebug()<<"async update, timer expired, doing now";
    Q_ASSERT(!m_asyncUpdateFutureInterface);
    m_asyncUpdateFutureInterface = new QFutureInterface<void>();

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    progressManager->addTask(m_asyncUpdateFutureInterface->future(), tr("Evaluate"), Constants::PROFILE_EVALUATE);
    if (debug)
        qDebug()<<"  adding task";

    m_asyncUpdateFutureInterface->setProgressRange(0, 0);
    m_asyncUpdateFutureInterface->reportStarted();

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        if (debug)
            qDebug()<<"  full update, starting with root node";
        m_rootProjectNode->asyncUpdate();
    } else {
        if (debug)
            qDebug()<<"  partial update,"<<m_partialEvaluate.size()<<"nodes to update";
        foreach(Qt4ProFileNode *node, m_partialEvaluate)
            node->asyncUpdate();
    }

    m_partialEvaluate.clear();
    if (debug)
        qDebug()<<"  Setting state to AsyncUpdateInProgress";
    m_asyncUpdateState = AsyncUpdateInProgress;
}

/*!
  Returns whether the project is an application, or has an application as a subproject.
 */
bool Qt4Project::isApplication() const
{
    return m_isApplication;
}

ProjectExplorer::IProjectManager *Qt4Project::projectManager() const
{
    return m_manager;
}

Qt4Manager *Qt4Project::qt4ProjectManager() const
{
    return m_manager;
}

QString Qt4Project::displayName() const
{
    return QFileInfo(file()->fileName()).completeBaseName();
}

QString Qt4Project::id() const
{
    return QLatin1String("Qt4ProjectManager.Qt4Project");
}

Core::IFile *Qt4Project::file() const
{
    return m_fileInfo;
}

QStringList Qt4Project::files(FilesMode fileMode) const
{
    QStringList files;
    for (int i = 0; i < FileTypeSize; ++i) {
        files += m_projectFiles->files[i];
        if (fileMode == AllFiles)
            files += m_projectFiles->generatedFiles[i];
    }
    return files;
}

// Find the folder that contains a file a certain type (recurse down)
static FolderNode *folderOf(FolderNode *in, FileType fileType, const QString &fileName)
{
    foreach(FileNode *fn, in->fileNodes())
        if (fn->fileType() == fileType && fn->path() == fileName)
            return in;
    foreach(FolderNode *folder, in->subFolderNodes())
        if (FolderNode *pn = folderOf(folder, fileType, fileName))
            return pn;
    return 0;
}

// Find the Qt4ProFileNode that contains a file of a certain type.
// First recurse down to folder, then find the pro-file.
static Qt4ProFileNode *proFileNodeOf(Qt4ProFileNode *in, FileType fileType, const QString &fileName)
{
    for (FolderNode *folder =  folderOf(in, fileType, fileName); folder; folder = folder->parentFolderNode())
        if (Qt4ProFileNode *proFile = qobject_cast<Qt4ProFileNode *>(folder))
            return proFile;
    return 0;
}

QString Qt4Project::generatedUiHeader(const QString &formFile) const
{
    // Look in sub-profiles as SessionManager::projectForFile returns
    // the top-level project only.
    if (m_rootProjectNode)
        if (const Qt4ProFileNode *pro = proFileNodeOf(m_rootProjectNode, FormType, formFile))
            return Qt4ProFileNode::uiHeaderFile(pro->uiDirectory(), formFile);
    return QString();
}

QList<ProjectExplorer::Project*> Qt4Project::dependsOn()
{
    // NBS implement dependsOn
    return QList<Project *>();
}

void Qt4Project::addDefaultBuild()
{
    // TODO this could probably refactored
    // That is the ProjectLoadWizard divided into useful bits
    // and this code then called here, instead of that strange forwarding
    // to a wizard, which doesn't even show up
    ProjectLoadWizard wizard(this);
    wizard.execDialog();
}

void Qt4Project::proFileParseError(const QString &errorMessage)
{
    Core::ICore::instance()->messageManager()->printToOutputPane(errorMessage);
}

ProFileReader *Qt4Project::createProFileReader(Qt4ProFileNode *qt4ProFileNode)
{
    if (!m_proFileOption) {
        m_proFileOption = new ProFileOption;
        m_proFileOptionRefCnt = 0;

        if (activeTarget() &&
            activeTarget()->activeBuildConfiguration()) {
            QtVersion *version = activeTarget()->activeBuildConfiguration()->qtVersion();
            if (version->isValid())
                m_proFileOption->properties = version->versionInfo();
        }

        ProFileCacheManager::instance()->incRefCount();
        m_proFileOption->cache = ProFileCacheManager::instance()->cache();
    }
    ++m_proFileOptionRefCnt;

    ProFileReader *reader = new ProFileReader(m_proFileOption);
    connect(reader, SIGNAL(errorFound(QString)),
            this, SLOT(proFileParseError(QString)));

    reader->setOutputDir(qt4ProFileNode->buildDir());

    return reader;
}

void Qt4Project::destroyProFileReader(ProFileReader *reader)
{
    delete reader;
    if (!--m_proFileOptionRefCnt) {
        QString dir = QFileInfo(m_fileInfo->fileName()).absolutePath();
        if (!dir.endsWith(QLatin1Char('/')))
            dir += QLatin1Char('/');
        m_proFileOption->cache->discardFiles(dir);

        delete m_proFileOption;
        m_proFileOption = 0;
        ProFileCacheManager::instance()->decRefCount();
    }
}

Qt4ProFileNode *Qt4Project::rootProjectNode() const
{
    return m_rootProjectNode;
}

BuildConfigWidget *Qt4Project::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(this);
}

QList<BuildConfigWidget*> Qt4Project::subConfigWidgets()
{
    QList<BuildConfigWidget*> subWidgets;
    subWidgets << new BuildEnvironmentWidget;
    return subWidgets;
}

void Qt4Project::collectApplicationProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node)
{
    if (node->projectType() == Internal::ApplicationTemplate
        || node->projectType() == Internal::ScriptTemplate) {
        list.append(node);
    }
    foreach (ProjectNode *n, node->subProjectNodes()) {
        Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(n);
        if (qt4ProFileNode)
            collectApplicationProFiles(list, qt4ProFileNode);
    }
}

void Qt4Project::checkForNewApplicationProjects()
{
    // Check all new project nodes
    // against all runConfigurations in all targets.

    foreach (Qt4ProFileNode *qt4proFile, applicationProFiles()) {
        foreach (Target *target, targets()) {
            Qt4Target *qt4Target = static_cast<Qt4Target *>(target);
            bool found = false;
            foreach (RunConfiguration *rc, target->runConfigurations()) {
                Qt4RunConfiguration *qtrc = qobject_cast<Qt4RunConfiguration *>(rc);
                if (qtrc && qtrc->proFilePath() == qt4proFile->path()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                qt4Target->addRunConfigurationForPath(qt4proFile->path());
                m_isApplication = true;
            }
        }
    }
}

void Qt4Project::checkForDeletedApplicationProjects()
{
    QStringList paths;
    foreach (Qt4ProFileNode * node, applicationProFiles())
        paths.append(node->path());

//    qDebug()<<"Still existing paths :"<<paths;

    QList<Qt4RunConfiguration *> removeList;
    foreach (Target *target, targets()) {
        foreach (RunConfiguration *rc, target->runConfigurations()) {
            if (Qt4RunConfiguration *qt4rc = qobject_cast<Qt4RunConfiguration *>(rc)) {
                if (!paths.contains(qt4rc->proFilePath())) {
                    removeList.append(qt4rc);
                }
            }
        }
        foreach (Qt4RunConfiguration *qt4rc, removeList)
            target->removeRunConfiguration(qt4rc);

        if (target->runConfigurations().isEmpty()) {
            target->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(target));
            m_isApplication = false;
        }
    }
}

QList<Qt4ProFileNode *> Qt4Project::applicationProFiles() const
{
    QList<Qt4ProFileNode *> list;
    if (!rootProjectNode())
        return list;
    collectApplicationProFiles(list, rootProjectNode());
    return list;
}

bool Qt4Project::hasApplicationProFile(const QString &path) const
{
    if (path.isEmpty())
        return false;

    QList<Qt4ProFileNode *> list = applicationProFiles();
    foreach (Qt4ProFileNode * node, list)
        if (node->path() == path)
            return true;
    return false;
}

QStringList Qt4Project::applicationProFilePathes(const QString &prepend) const
{
    QStringList proFiles;
    foreach (Qt4ProFileNode *node, applicationProFiles())
        proFiles.append(prepend + node->path());
    return proFiles;
}

void Qt4Project::activeTargetWasChanged()
{
    if (!activeTarget())
        return;

    scheduleAsyncUpdate();
}

bool Qt4Project::hasSubNode(Qt4PriFileNode *root, const QString &path)
{
    if (root->path() == path)
        return true;
    foreach (FolderNode *fn, root->subFolderNodes()) {
        if (qobject_cast<Qt4ProFileNode *>(fn)) {
            // we aren't interested in pro file nodes
        } else if (Qt4PriFileNode *qt4prifilenode = qobject_cast<Qt4PriFileNode *>(fn)) {
            if (hasSubNode(qt4prifilenode, path))
                return true;
        }
    }
    return false;
}

void Qt4Project::findProFile(const QString& fileName, Qt4ProFileNode *root, QList<Qt4ProFileNode *> &list)
{
    if (hasSubNode(root, fileName))
        list.append(root);

    foreach (FolderNode *fn, root->subFolderNodes())
        if (Qt4ProFileNode *qt4proFileNode =  qobject_cast<Qt4ProFileNode *>(fn))
            findProFile(fileName, qt4proFileNode, list);
}

void Qt4Project::notifyChanged(const QString &name)
{
    if (files(Qt4Project::ExcludeGeneratedFiles).contains(name)) {
        QList<Qt4ProFileNode *> list;
        findProFile(name, rootProjectNode(), list);
        foreach(Qt4ProFileNode *node, list) {
            ProFileCacheManager::instance()->discardFile(name);
            node->update();
        }
    }
}

/*!
  Handle special case were a subproject of the qt directory is opened, and
  qt was configured to be built as a shadow build -> also build in the sub-
  project in the correct shadow build directory.
  */

// TODO this function should be called on project first load
// and it should check against all configured qt versions ?
//void Qt4Project::detectQtShadowBuild(const QString &buildConfiguration) const
//{
//    if (project()->activeBuildConfiguration() == buildConfiguration)
//        return;
//
//    const QString currentQtDir = static_cast<Qt4Project *>(project())->qtDir(buildConfiguration);
//    const QString qtSourceDir = static_cast<Qt4Project *>(project())->qtVersion(buildConfiguration)->sourcePath();
//
//    // if the project is a sub-project of Qt and Qt was shadow-built then automatically
//    // adjust the build directory of the sub-project.
//    if (project()->file()->fileName().startsWith(qtSourceDir) && qtSourceDir != currentQtDir) {
//        project()->setValue(buildConfiguration, "useShadowBuild", true);
//        QString buildDir = project()->projectDirectory();
//        buildDir.replace(qtSourceDir, currentQtDir);
//        project()->setValue(buildConfiguration, "buildDirectory", buildDir);
//        project()->setValue(buildConfiguration, "autoShadowBuild", true);
//    }
//}


