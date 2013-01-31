/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qt4project.h"

#include "qt4projectmanager.h"
#include "makestep.h"
#include "qmakestep.h"
#include "qmakerunconfigurationfactory.h"
#include "qt4nodes.h"
#include "qt4projectconfigwidget.h"
#include "qt4projectmanagerconstants.h"
#include "qt4buildconfiguration.h"
#include "findqt4profiles.h"
#include "buildconfigurationinfo.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icontext.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/variablemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/ModelManagerInterface.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <utils/qtcassert.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qmldumptool.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/profilereader.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/QtConcurrentTools>
#include <utils/stringutils.h>

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QFileDialog>
#include <QInputDialog>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace ProjectExplorer;

enum { debug = 0 };

// -----------------------------------------------------------------------
// Helpers:
// -----------------------------------------------------------------------

namespace {

Qt4BuildConfiguration *enableActiveQt4BuildConfiguration(ProjectExplorer::Target *t, bool enabled)
{
    if (!t)
        return 0;
    Qt4BuildConfiguration *bc = static_cast<Qt4BuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return 0;
    bc->setEnabled(enabled);
    return bc;
}

} // namespace

namespace Qt4ProjectManager {
namespace Internal {

class Qt4ProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    Qt4ProjectFile(const QString &filePath, QObject *parent = 0);

    bool save(QString *errorString, const QString &fileName, bool autoSave);
    QString fileName() const;
    virtual void rename(const QString &newName);

    QString defaultPath() const;
    QString suggestedFileName() const;
    virtual QString mimeType() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    const QString m_mimeType;
    QString m_filePath;
};

/// Watches folders for Qt4PriFile nodes
/// use one file system watcher to watch all folders
/// such minimizing system ressouce usage

class CentralizedFolderWatcher : public QObject
{
    Q_OBJECT
public:
    CentralizedFolderWatcher(QObject *parent);
    ~CentralizedFolderWatcher();
    void watchFolders(const QList<QString> &folders, Qt4ProjectManager::Qt4PriFileNode *node);
    void unwatchFolders(const QList<QString> &folders, Qt4ProjectManager::Qt4PriFileNode *node);

private slots:
    void folderChanged(const QString &folder);
    void onTimer();
    void delayedFolderChanged(const QString &folder);

private:
    QSet<QString> recursiveDirs(const QString &folder);
    QFileSystemWatcher m_watcher;
    QMultiMap<QString, Qt4ProjectManager::Qt4PriFileNode *> m_map;

    QSet<QString> m_recursiveWatchedFolders;
    QTimer m_compressTimer;
    QSet<QString> m_changedFolders;
};

// Qt4ProjectFiles: Struct for (Cached) lists of files in a project
class Qt4ProjectFiles {
public:
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

// ----------- Qt4ProjectFile
namespace Internal {
Qt4ProjectFile::Qt4ProjectFile(const QString &filePath, QObject *parent)
    : Core::IDocument(parent),
      m_mimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)),
      m_filePath(filePath)
{
}

bool Qt4ProjectFile::save(QString *, const QString &, bool)
{
    // This is never used
    return false;
}

void Qt4ProjectFile::rename(const QString &newName)
{
    // Can't happen
    Q_UNUSED(newName);
    Q_ASSERT(false);
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

bool Qt4ProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior Qt4ProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool Qt4ProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

} // namespace Internal

/*!
  \class Qt4Project

  Qt4Project manages information about an individual Qt 4 (.pro) project file.
  */

Qt4Project::Qt4Project(Qt4Manager *manager, const QString& fileName) :
    m_manager(manager),
    m_rootProjectNode(0),
    m_nodesWatcher(new Internal::Qt4NodesWatcher(this)),
    m_fileInfo(new Qt4ProjectFile(fileName, this)),
    m_projectFiles(new Qt4ProjectFiles),
    m_qmakeGlobals(0),
    m_asyncUpdateFutureInterface(0),
    m_pendingEvaluateFuturesCount(0),
    m_asyncUpdateState(NoState),
    m_cancelEvaluate(false),
    m_centralizedFolderWatcher(0),
    m_activeTarget(0)
{
    setProjectContext(Core::Context(Qt4ProjectManager::Constants::PROJECT_ID));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(3000);
    connect(&m_asyncUpdateTimer, SIGNAL(timeout()), this, SLOT(asyncUpdate()));
}

Qt4Project::~Qt4Project()
{
    m_codeModelFuture.cancel();
    m_asyncUpdateState = ShuttingDown;
    m_manager->unregisterProject(this);
    delete m_projectFiles;
    m_cancelEvaluate = true;
    // Deleting the root node triggers a few things, make sure rootProjectNode
    // returns 0 already
    Qt4ProFileNode *root = m_rootProjectNode;
    m_rootProjectNode = 0;
    delete root;
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

bool Qt4Project::setupTarget(ProjectExplorer::Target *t)
{
    QList<BuildConfigurationInfo> infoList
            = Qt4BuildConfigurationFactory::availableBuildConfigurations(t->kit(), m_fileInfo->fileName());
    setupTarget(t, infoList);
    return true;
}

void Qt4Project::setupTarget(ProjectExplorer::Target *t, const QList<BuildConfigurationInfo> &infoList)
{
    // Build Configurations:
    foreach (const BuildConfigurationInfo &info, infoList) {
        QString name = info.buildConfig & QtSupport::BaseQtVersion::DebugBuild
                ? tr("Debug") : tr("Release");
        Qt4BuildConfiguration *bc
                = Qt4BuildConfiguration::setup(t, name, name,
                                               info.buildConfig, info.additionalArguments,
                                               info.directory, info.importing);
        t->addBuildConfiguration(bc);
    }

    // Deploy Configurations:
    t->updateDefaultDeployConfigurations();
    // Do not create Run Configurations: Those will be generated later anyway.
}

bool Qt4Project::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    // Prune targets without buildconfigurations:
    // This can happen esp. when updating from a old version of Qt Creator
    QList<Target *>ts = targets();
    foreach (Target *t, ts) {
        if (t->buildConfigurations().isEmpty()) {
            qWarning() << "Removing" << t->id().name() << "since it has no buildconfigurations!";
            removeTarget(t);
        }
    }

    m_manager->registerProject(this);

    m_rootProjectNode = new Qt4ProFileNode(this, m_fileInfo->fileName(), this);
    m_rootProjectNode->registerWatcher(m_nodesWatcher);

    update();
    updateFileList();
    // This might be incorrect, need a full update
    updateCodeModels();

    // We have the profile nodes now, so we know the runconfigs!
    connect(m_nodesWatcher, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)),
            this, SIGNAL(proFileUpdated(Qt4ProjectManager::Qt4ProFileNode*,bool,bool)));

    // Now we emit update once :)
    m_rootProjectNode->emitProFileUpdatedRecursive();

    // On active buildconfiguration changes, reevaluate the .pro files
    m_activeTarget = activeTarget();
    if (m_activeTarget)
        connect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(scheduleAsyncUpdate()));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(activeTargetWasChanged()));

    return true;
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
        if (*ait == CPlusPlus::CppModelManagerInterface::configurationFileName())
            ++ait;
        else if (*bit == CPlusPlus::CppModelManagerInterface::configurationFileName())
            ++bit;
        else if (*ait == *bit)
            ++ait, ++bit;
        else
           return false;
    }
    return (ait == aend && bit == bend);
}

void Qt4Project::updateCodeModels()
{
    if (debug)
        qDebug()<<"Qt4Project::updateCodeModel()";

    if (activeTarget() && !activeTarget()->activeBuildConfiguration())
        return;

    updateCppCodeModel();
    updateQmlJSCodeModel();
}

void Qt4Project::updateCppCodeModel()
{
    typedef CPlusPlus::CppModelManagerInterface::ProjectPart ProjectPart;

    Kit *k = 0;
    QtSupport::BaseQtVersion *qtVersion = 0;
    ToolChain *tc = 0;
    if (ProjectExplorer::Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::instance()->defaultKit();
    qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    tc = ToolChainKitInformation::toolChain(k);

    CPlusPlus::CppModelManagerInterface *modelmanager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (!modelmanager)
        return;

    FindQt4ProFiles findQt4ProFiles;
    QList<Qt4ProFileNode *> proFiles = findQt4ProFiles(rootProjectNode());

    CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
    pinfo.clearProjectParts();
    ProjectPart::QtVersion qtVersionForPart = ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = ProjectPart::Qt4;
        else
            qtVersionForPart = ProjectPart::Qt5;
    }

    QStringList allFiles;
    foreach (Qt4ProFileNode *pro, proFiles) {
        ProjectPart::Ptr part(new ProjectPart);

        if (pro->variableValue(ConfigVar).contains(QLatin1String("qt")))
            part->qtVersion = qtVersionForPart;
        else
            part->qtVersion = ProjectPart::NoQt;

        const QStringList cxxflags = pro->variableValue(CppFlagsVar);

        // part->defines
        if (tc)
            part->defines = tc->predefinedMacros(cxxflags);
        part->defines += pro->cxxDefines();

        // part->includePaths
        part->includePaths.append(pro->variableValue(IncludePathVar));

        QList<HeaderPath> headers;
        if (tc)
            headers = tc->systemHeaderPaths(cxxflags, SysRootKitInformation::sysRoot(k));
        if (qtVersion)
            headers.append(qtVersion->systemHeaderPathes(k));

        foreach (const HeaderPath &headerPath, headers) {
            if (headerPath.kind() == HeaderPath::FrameworkHeaderPath)
                part->frameworkPaths.append(headerPath.path());
            else
                part->includePaths.append(headerPath.path());
        }

        if (qtVersion) {
            if (!qtVersion->frameworkInstallPath().isEmpty())
                part->frameworkPaths.append(qtVersion->frameworkInstallPath());

        }
        if (Qt4ProFileNode *node = rootQt4ProjectNode())
            part->includePaths.append(node->resolvedMkspecPath());

        // part->precompiledHeaders
        part->precompiledHeaders.append(pro->variableValue(PrecompiledHeaderVar));

        // part->language
        if (tc)
            part->language = tc->compilerFlags(cxxflags) == ToolChain::STD_CXX11 ? ProjectPart::CXX11 : ProjectPart::CXX;
        else
            part->language = CPlusPlus::CppModelManagerInterface::ProjectPart::CXX11;

        part->sourceFiles = pro->variableValue(CppSourceVar);
        part->headerFiles += pro->variableValue(CppHeaderVar);
        part->headerFiles += pro->uiFiles();
        part->sourceFiles.prepend(CPlusPlus::CppModelManagerInterface::configurationFileName());
        part->objcSourceFiles = pro->variableValue(ObjCSourceVar);
        pinfo.appendProjectPart(part);

        allFiles += part->headerFiles;
        allFiles += part->sourceFiles;
        allFiles += part->objcSourceFiles;
    }

    modelmanager->updateProjectInfo(pinfo);
    m_codeModelFuture = modelmanager->updateSourceFiles(allFiles);
}

void Qt4Project::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager->projectInfo(this);
    projectInfo.sourceFiles = m_projectFiles->files[QMLType];

    FindQt4ProFiles findQt4ProFiles;
    QList<Qt4ProFileNode *> proFiles = findQt4ProFiles(rootProjectNode());

    projectInfo.importPaths.clear();
    foreach (Qt4ProFileNode *node, proFiles) {
        projectInfo.importPaths.append(node->variableValue(QmlImportPathVar));
    }

    bool preferDebugDump = false;
    projectInfo.tryQmlDump = false;

    ProjectExplorer::Target *t = activeTarget();
    ProjectExplorer::Kit *k = t ? t->kit() : ProjectExplorer::KitManager::instance()->defaultKit();
    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);

    if (t) {
        if (Qt4BuildConfiguration *bc = qobject_cast<Qt4BuildConfiguration *>(t->activeBuildConfiguration()))
            preferDebugDump = bc->qmakeBuildConfiguration() & QtSupport::BaseQtVersion::DebugBuild;
    } else {
        if (qtVersion)
            preferDebugDump = qtVersion->defaultBuildConfig() & QtSupport::BaseQtVersion::DebugBuild;
    }
    if (qtVersion && qtVersion->isValid()) {
        projectInfo.tryQmlDump = qtVersion->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)
                || qtVersion->type() == QLatin1String(QtSupport::Constants::SIMULATORQT);
        projectInfo.qtQmlPath = qtVersion->qmakeProperty("QT_INSTALL_QML");
        if (!projectInfo.qtQmlPath.isEmpty())
            projectInfo.importPaths += projectInfo.qtQmlPath;
        projectInfo.qtImportsPath = qtVersion->qmakeProperty("QT_INSTALL_IMPORTS");
        if (!projectInfo.qtImportsPath.isEmpty())
            projectInfo.importPaths += projectInfo.qtImportsPath;
        projectInfo.qtVersionString = qtVersion->qtVersionString();
    }
    projectInfo.importPaths.removeDuplicates();

    if (projectInfo.tryQmlDump) {
        QtSupport::QmlDumpTool::pathAndEnvironment(this, qtVersion,
                                                   ToolChainKitInformation::toolChain(k),
                                                   preferDebugDump, &projectInfo.qmlDumpPath,
                                                   &projectInfo.qmlDumpEnvironment);
    } else {
        projectInfo.qmlDumpPath.clear();
        projectInfo.qmlDumpEnvironment.clear();
    }

    modelManager->updateProjectInfo(projectInfo);
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
    enableActiveQt4BuildConfiguration(activeTarget(), true);
    updateBuildSystemData();
    updateRunConfigurations();
    emit proFilesEvaluated();
}

void Qt4Project::updateRunConfigurations()
{
    foreach (Target *t, targets())
        t->updateDefaultRunConfigurations();
}

void Qt4Project::scheduleAsyncUpdate(Qt4ProFileNode *node)
{
    if (m_asyncUpdateState == ShuttingDown)
        return;

    if (debug)
        qDebug()<<"schduleAsyncUpdate (node)"<<node->path();
    Q_ASSERT(m_asyncUpdateState != NoState);

    if (m_cancelEvaluate) {
        if (debug)
            qDebug()<<"  Already canceling, nothing to do";
        // A cancel is in progress
        // That implies that a full update is going to happen afterwards
        // So we don't need to do anything
        return;
    }

    enableActiveQt4BuildConfiguration(activeTarget(), false);

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

        QList<Qt4ProFileNode *>::iterator it;
        bool add = true;
        if (debug)
            qDebug()<<"scheduleAsyncUpdate();"<<m_partialEvaluate.size()<<"nodes";
        it = m_partialEvaluate.begin();
        while (it != m_partialEvaluate.end()) {
            if (*it == node) {
                add = false;
                break;
            } else if (node->isParent(*it)) { // We already have the parent in the list, nothing to do
                it = m_partialEvaluate.erase(it);
            } else if ((*it)->isParent(node)) { // The node is the parent of a child already in the list
                add = false;
                break;
            } else {
                ++it;
            }
        }

        if (add)
            m_partialEvaluate.append(node);
        // and start the timer anew
        m_asyncUpdateTimer.start();

        // Cancel running code model update
        m_codeModelFuture.cancel();
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
    if (m_asyncUpdateState == ShuttingDown)
        return;

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
        enableActiveQt4BuildConfiguration(activeTarget(), false);
        m_rootProjectNode->setParseInProgressRecursive(true);
        return;
    }

    if (debug)
        qDebug()<<"  starting timer for full update, setting state to full update pending";
    m_partialEvaluate.clear();
    enableActiveQt4BuildConfiguration(activeTarget(), false);
    m_rootProjectNode->setParseInProgressRecursive(true);
    m_asyncUpdateState = AsyncFullUpdatePending;
    m_asyncUpdateTimer.start();

    // Cancel running code model update
    m_codeModelFuture.cancel();
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
            // After being done, we need to call:
            m_asyncUpdateState = Base;
            enableActiveQt4BuildConfiguration(activeTarget(), true);
            updateFileList();
            updateCodeModels();
            updateBuildSystemData();
            updateRunConfigurations();
            emit proFilesEvaluated();
            if (debug)
                qDebug()<<"  Setting state to Base";
        }
    }
}

bool Qt4Project::wasEvaluateCanceled()
{
    return m_cancelEvaluate;
}

void Qt4Project::asyncUpdate()
{
    if (debug)
        qDebug()<<"async update, timer expired, doing now";
    Q_ASSERT(!m_asyncUpdateFutureInterface);
    m_asyncUpdateFutureInterface = new QFutureInterface<void>();

    Core::ProgressManager *progressManager = Core::ICore::progressManager();

    m_asyncUpdateFutureInterface->setProgressRange(0, 0);
    progressManager->addTask(m_asyncUpdateFutureInterface->future(), tr("Evaluating"),
                             QLatin1String(Constants::PROFILE_EVALUATE));
    if (debug)
        qDebug()<<"  adding task";

    m_asyncUpdateFutureInterface->reportStarted();

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        if (debug)
            qDebug()<<"  full update, starting with root node";
        m_rootProjectNode->asyncUpdate();
    } else {
        if (debug)
            qDebug()<<"  partial update,"<<m_partialEvaluate.size()<<"nodes to update";
        foreach (Qt4ProFileNode *node, m_partialEvaluate)
            node->asyncUpdate();
    }

    m_partialEvaluate.clear();
    if (debug)
        qDebug()<<"  Setting state to AsyncUpdateInProgress";
    m_asyncUpdateState = AsyncUpdateInProgress;
}

ProjectExplorer::IProjectManager *Qt4Project::projectManager() const
{
    return m_manager;
}

Qt4Manager *Qt4Project::qt4ProjectManager() const
{
    return m_manager;
}

bool Qt4Project::supportsKit(Kit *k, QString *errorMessage) const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version && errorMessage)
        *errorMessage = tr("No Qt version set in kit.");
    return version;
}

QString Qt4Project::displayName() const
{
    return QFileInfo(document()->fileName()).completeBaseName();
}

Core::Id Qt4Project::id() const
{
    return Core::Id(Constants::QT4PROJECT_ID);
}

Core::IDocument *Qt4Project::document() const
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
    foreach (FileNode *fn, in->fileNodes())
        if (fn->fileType() == fileType && fn->path() == fileName)
            return in;
    foreach (FolderNode *folder, in->subFolderNodes())
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

void Qt4Project::proFileParseError(const QString &errorMessage)
{
    Core::ICore::messageManager()->printToOutputPanePopup(errorMessage);
}

QtSupport::ProFileReader *Qt4Project::createProFileReader(const Qt4ProFileNode *qt4ProFileNode, Qt4BuildConfiguration *bc)
{
    if (!m_qmakeGlobals) {
        m_qmakeGlobals = new ProFileGlobals;
        m_qmakeGlobalsRefCnt = 0;

        Kit *k;
        Utils::Environment env = Utils::Environment::systemEnvironment();
        QStringList qmakeArgs;
        if (!bc)
            bc = activeTarget() ? static_cast<Qt4BuildConfiguration *>(activeTarget()->activeBuildConfiguration()) : 0;

        if (bc) {
            k = bc->target()->kit();
            env = bc->environment();
            if (bc->qmakeStep())
                qmakeArgs = bc->qmakeStep()->parserArguments();
            else
                qmakeArgs = bc->configCommandLineArguments();
        } else {
            k = KitManager::instance()->defaultKit();
        }

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
        QString systemRoot = SysRootKitInformation::hasSysRoot(k)
                ? SysRootKitInformation::sysRoot(k).toString() : QString();

        if (qtVersion && qtVersion->isValid()) {
            m_qmakeGlobals->qmake_abslocation = QDir::cleanPath(qtVersion->qmakeCommand().toString());
            m_qmakeGlobals->setProperties(qtVersion->versionInfo());
        }
        m_qmakeGlobals->setDirectories(m_rootProjectNode->sourceDir(), m_rootProjectNode->buildDir());
        m_qmakeGlobals->sysroot = systemRoot;

        Utils::Environment::const_iterator eit = env.constBegin(), eend = env.constEnd();
        for (; eit != eend; ++eit)
            m_qmakeGlobals->environment.insert(env.key(eit), env.value(eit));

        m_qmakeGlobals->setCommandLineArguments(m_rootProjectNode->buildDir(), qmakeArgs);

        QtSupport::ProFileCacheManager::instance()->incRefCount();
    }
    ++m_qmakeGlobalsRefCnt;

    QtSupport::ProFileReader *reader = new QtSupport::ProFileReader(m_qmakeGlobals);

    reader->setOutputDir(qt4ProFileNode->buildDir());

    return reader;
}

ProFileGlobals *Qt4Project::qmakeGlobals()
{
    return m_qmakeGlobals;
}

void Qt4Project::destroyProFileReader(QtSupport::ProFileReader *reader)
{
    delete reader;
    if (!--m_qmakeGlobalsRefCnt) {
        QString dir = QFileInfo(m_fileInfo->fileName()).absolutePath();
        if (!dir.endsWith(QLatin1Char('/')))
            dir += QLatin1Char('/');
        QtSupport::ProFileCacheManager::instance()->discardFiles(dir);
        QtSupport::ProFileCacheManager::instance()->decRefCount();

        delete m_qmakeGlobals;
        m_qmakeGlobals = 0;
    }
}

ProjectExplorer::ProjectNode *Qt4Project::rootProjectNode() const
{
    return m_rootProjectNode;
}

Qt4ProFileNode *Qt4Project::rootQt4ProjectNode() const
{
    return m_rootProjectNode;
}

bool Qt4Project::validParse(const QString &proFilePath) const
{
    if (!m_rootProjectNode)
        return false;
    const Qt4ProFileNode *node = m_rootProjectNode->findProFileFor(proFilePath);
    return node && node->validParse();
}

bool Qt4Project::parseInProgress(const QString &proFilePath) const
{
    if (!m_rootProjectNode)
        return false;
    const Qt4ProFileNode *node = m_rootProjectNode->findProFileFor(proFilePath);
    return node && node->parseInProgress();
}

void Qt4Project::collectAllfProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node)
{
    list.append(node);
    foreach (ProjectNode *n, node->subProjectNodes()) {
        Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(n);
        if (qt4ProFileNode)
            collectAllfProFiles(list, qt4ProFileNode);
    }
}

void Qt4Project::collectApplicationProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node)
{
    if (node->projectType() == ApplicationTemplate
        || node->projectType() == ScriptTemplate) {
        list.append(node);
    }
    foreach (ProjectNode *n, node->subProjectNodes()) {
        Qt4ProFileNode *qt4ProFileNode = qobject_cast<Qt4ProFileNode *>(n);
        if (qt4ProFileNode)
            collectApplicationProFiles(list, qt4ProFileNode);
    }
}

QList<Qt4ProFileNode *> Qt4Project::allProFiles() const
{
    QList<Qt4ProFileNode *> list;
    if (!rootProjectNode())
        return list;
    collectAllfProFiles(list, rootQt4ProjectNode());
    return list;
}

QList<Qt4ProFileNode *> Qt4Project::applicationProFiles() const
{
    QList<Qt4ProFileNode *> list;
    if (!rootProjectNode())
        return list;
    collectApplicationProFiles(list, rootQt4ProjectNode());
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
    if (m_activeTarget) {
        disconnect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(scheduleAsyncUpdate()));
    }

    m_activeTarget = activeTarget();

    if (!m_activeTarget)
        return;

    connect(m_activeTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(scheduleAsyncUpdate()));

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
        findProFile(name, rootQt4ProjectNode(), list);
        foreach (Qt4ProFileNode *node, list) {
            QtSupport::ProFileCacheManager::instance()->discardFile(name);
            node->update();
        }
        updateFileList();
    }
}

void Qt4Project::watchFolders(const QStringList &l, Qt4PriFileNode *node)
{
    if (l.isEmpty())
        return;
    if (!m_centralizedFolderWatcher)
        m_centralizedFolderWatcher = new Internal::CentralizedFolderWatcher(this);
    m_centralizedFolderWatcher->watchFolders(l, node);
}

void Qt4Project::unwatchFolders(const QStringList &l, Qt4PriFileNode *node)
{
    if (m_centralizedFolderWatcher && !l.isEmpty())
        m_centralizedFolderWatcher->unwatchFolders(l, node);
}

/////////////
/// Centralized Folder Watcher
////////////

// All the folder have a trailing slash!

namespace {
   bool debugCFW = false;
}

CentralizedFolderWatcher::CentralizedFolderWatcher(QObject *parent) : QObject(parent)
{
    m_compressTimer.setSingleShot(true);
    m_compressTimer.setInterval(200);
    connect(&m_compressTimer, SIGNAL(timeout()),
            this, SLOT(onTimer()));
    connect (&m_watcher, SIGNAL(directoryChanged(QString)),
             this, SLOT(folderChanged(QString)));
}

CentralizedFolderWatcher::~CentralizedFolderWatcher()
{

}

QSet<QString> CentralizedFolderWatcher::recursiveDirs(const QString &folder)
{
    QSet<QString> result;
    QDir dir(folder);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    foreach (const QString &f, list) {
        const QString a = folder + f + QLatin1Char('/');
        result.insert(a);
        result += recursiveDirs(a);
    }
    return result;
}

void CentralizedFolderWatcher::watchFolders(const QList<QString> &folders, Qt4ProjectManager::Qt4PriFileNode *node)
{
    if (debugCFW)
        qDebug()<<"CFW::watchFolders()"<<folders<<"for node"<<node->path();
    m_watcher.addPaths(folders);

    const QChar slash = QLatin1Char('/');
    foreach (const QString &f, folders) {
        QString folder = f;
        if (!folder.endsWith(slash))
            folder.append(slash);
        m_map.insert(folder, node);

        // Support for recursive watching
        // we add the recursive directories we find
        QSet<QString> tmp = recursiveDirs(folder);
        if (!tmp.isEmpty())
            m_watcher.addPaths(tmp.toList());
        m_recursiveWatchedFolders += tmp;

        if (debugCFW)
            qDebug()<<"adding recursive dirs for"<< folder<<":"<<tmp;
    }
}

void CentralizedFolderWatcher::unwatchFolders(const QList<QString> &folders, Qt4ProjectManager::Qt4PriFileNode *node)
{
    if (debugCFW)
        qDebug()<<"CFW::unwatchFolders()"<<folders<<"for node"<<node->path();
    const QChar slash = QLatin1Char('/');
    foreach (const QString &f, folders) {
        QString folder = f;
        if (!folder.endsWith(slash))
            folder.append(slash);
        m_map.remove(folder, node);
        if (!m_map.contains(folder))
            m_watcher.removePath(folder);

        // Figure out which recursive directories we can remove
        // this might not scale. I'm pretty sure it doesn't
        // A scaling implementation would need to save more information
        // where a given directory watcher actual comes from...

        QStringList toRemove;
        foreach (const QString &rwf, m_recursiveWatchedFolders) {
            if (rwf.startsWith(folder)) {
                // So the rwf is a subdirectory of a folder we aren't watching
                // but maybe someone else wants us to watch
                bool needToWatch = false;
                QMultiMap<QString, Qt4ProjectManager::Qt4PriFileNode *>::const_iterator it, end;
                end = m_map.constEnd();
                for (it = m_map.constEnd(); it != end; ++it) {
                    if (rwf.startsWith(it.key())) {
                        needToWatch = true;
                        break;
                    }
                }
                if (!needToWatch) {
                    m_watcher.removePath(rwf);
                    toRemove << rwf;
                }
            }
        }

        if (debugCFW)
            qDebug()<<"removing recursive dirs for"<<folder<<":"<<toRemove;

        foreach (const QString &tr, toRemove) {
            m_recursiveWatchedFolders.remove(tr);
        }
    }
}

void CentralizedFolderWatcher::folderChanged(const QString &folder)
{
    m_changedFolders.insert(folder);
    m_compressTimer.start();
}

void CentralizedFolderWatcher::onTimer()
{
    foreach (const QString &folder, m_changedFolders)
        delayedFolderChanged(folder);
    m_changedFolders.clear();
}

void CentralizedFolderWatcher::delayedFolderChanged(const QString &folder)
{
    if (debugCFW)
        qDebug()<<"CFW::folderChanged"<<folder;
    // Figure out whom to inform

    QString dir = folder;
    const QChar slash = QLatin1Char('/');
    while (true) {
        if (!dir.endsWith(slash))
            dir.append(slash);
        QList<Qt4ProjectManager::Qt4PriFileNode *> nodes = m_map.values(dir);
        foreach (Qt4ProjectManager::Qt4PriFileNode *node, nodes) {
            node->folderChanged(folder);
        }

        // Chop off last part, and break if there's nothing to chop off
        //
        if (dir.length() < 2)
            break;

        // We start before the last slash
        const int index = dir.lastIndexOf(slash, dir.length() - 2);
        if (index == -1)
            break;
        dir.truncate(index + 1);
    }


    QString folderWithSlash = folder;
    if (!folder.endsWith(slash))
        folderWithSlash.append(slash);

    // If a subdirectory was added, watch it too
    QSet<QString> tmp = recursiveDirs(folderWithSlash);
    if (!tmp.isEmpty()) {
        if (debugCFW)
            qDebug()<<"found new recursive dirs"<<tmp;

        QSet<QString> alreadyAdded = m_watcher.directories().toSet();
        tmp.subtract(alreadyAdded);
        if (!tmp.isEmpty())
            m_watcher.addPaths(tmp.toList());
        m_recursiveWatchedFolders += tmp;
    }
}

bool Qt4Project::needsConfiguration() const
{
    return targets().isEmpty();
}

void Qt4Project::configureAsExampleProject(const QStringList &platforms)
{
    QList<Kit *> kits = ProjectExplorer::KitManager::instance()->kits();
    foreach (Kit *k, kits) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        if (!version)
            continue;
        if (!platforms.isEmpty() && !platforms.contains(version->platformName()))
            continue;

        QList<BuildConfigurationInfo> infoList
                = Qt4BuildConfigurationFactory::availableBuildConfigurations(k, document()->fileName());
        if (infoList.isEmpty())
            continue;
        addTarget(createTarget(k, infoList));
    }
    ProjectExplorer::ProjectExplorerPlugin::instance()->requestProjectModeUpdate(this);
}

bool Qt4Project::supportsNoTargetPanel() const
{
    return true;
}

// All the Qt4 run configurations should share code.
// This is a rather suboptimal way to do that for disabledReason()
// but more pratical then duplicated the code everywhere
QString Qt4Project::disabledReasonForRunConfiguration(const QString &proFilePath)
{
    if (!QFileInfo(proFilePath).exists())
        return tr("The .pro file '%1' does not exist.")
                .arg(QFileInfo(proFilePath).fileName());

    if (!m_rootProjectNode) // Shutting down
        return QString();

    if (!m_rootProjectNode->findProFileFor(proFilePath))
        return tr("The .pro file '%1' is not part of the project.")
                .arg(QFileInfo(proFilePath).fileName());

    return tr("The .pro file '%1' could not be parsed.")
            .arg(QFileInfo(proFilePath).fileName());
}

QString Qt4Project::shadowBuildDirectory(const QString &proFilePath, const Kit *k, const QString &suffix)
{
    if (proFilePath.isEmpty())
        return QString();
    QFileInfo info(proFilePath);

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (version && !version->supportsShadowBuilds())
        return info.absolutePath();

    const QString projectName = QFileInfo(proFilePath).completeBaseName();
    ProjectExplorer::ProjectExpander expander(proFilePath, projectName, k, suffix);
    QDir projectDir = QDir(projectDirectory(proFilePath));
    QString buildPath = Utils::expandMacros(Core::DocumentManager::buildDirectory(), &expander);
    return QDir::cleanPath(projectDir.absoluteFilePath(buildPath));
}

QString Qt4Project::buildNameFor(const Kit *k)
{
    if (!k)
        return QLatin1String("unknown");

    return k->fileSystemFriendlyName();
}

Target *Qt4Project::createTarget(Kit *k, const QList<BuildConfigurationInfo> &infoList)
{
    if (target(k))
        return 0;

    Target *t = new Target(this, k);
    setupTarget(t, infoList);
    return t;
}

void Qt4Project::updateBuildSystemData()
{
    Target * const target = activeTarget();
    if (!target)
        return;
    const Qt4ProFileNode * const rootNode = rootQt4ProjectNode();
    if (!rootNode || rootNode->parseInProgress())
        return;

    DeploymentData deploymentData;
    collectData(rootNode, deploymentData);
    target->setDeploymentData(deploymentData);

    BuildTargetInfoList appTargetList;
    foreach (const Qt4ProFileNode * const node, applicationProFiles())
        appTargetList.list << BuildTargetInfo(node->targetInformation().executable, node->path());
    target->setApplicationTargets(appTargetList);
}

void Qt4Project::collectData(const Qt4ProFileNode *node, DeploymentData &deploymentData)
{
    if (!node->isSubProjectDeployable(node->path()))
        return;

    const InstallsList &installsList = node->installsList();
    foreach (const InstallsItem &item, installsList.items) {
        foreach (const QString &localFile, item.files)
            deploymentData.addFile(localFile, item.path);
    }

    switch (node->projectType()) {
    case ApplicationTemplate:
        if (!installsList.targetPath.isEmpty())
            deploymentData.addFile(node->targetInformation().executable, installsList.targetPath,
                                   DeployableFile::TypeExecutable);
        break;
    case LibraryTemplate:
        collectLibraryData(node, deploymentData);
        break;
    case SubDirsTemplate:
        foreach (const ProjectNode * const subProject, node->subProjectNodesExact()) {
            const Qt4ProFileNode * const qt4SubProject
                    = qobject_cast<const Qt4ProFileNode *>(subProject);
            if (!qt4SubProject)
                continue;
            collectData(qt4SubProject, deploymentData);
        }
        break;
    default:
        break;
    }
}

void Qt4Project::collectLibraryData(const Qt4ProFileNode *node, DeploymentData &deploymentData)
{
    const QString targetPath = node->installsList().targetPath;
    if (targetPath.isEmpty())
        return;
    const ProjectExplorer::Kit * const kit = activeTarget()->kit();
    const ProjectExplorer::ToolChain * const toolchain
            = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
    if (!toolchain)
        return;

    TargetInformation ti = node->targetInformation();
    QString targetFileName = ti.target;
    const QStringList config = node->variableValue(ConfigVar);
    const bool isStatic = config.contains(QLatin1String("static"));
    const bool isPlugin = config.contains(QLatin1String("plugin"));
    switch (toolchain->targetAbi().os()) {
    case ProjectExplorer::Abi::WindowsOS: {
        QString targetVersionExt = node->singleVariableValue(TargetVersionExtVar);
        if (targetVersionExt.isEmpty()) {
            const QString version = node->singleVariableValue(VersionVar);
            if (!version.isEmpty()) {
                targetVersionExt = version.left(version.indexOf(QLatin1Char('.')));
                if (targetVersionExt == QLatin1String("0"))
                    targetVersionExt.clear();
            }
        }
        targetFileName += targetVersionExt + QLatin1Char('.');
        targetFileName += QLatin1String(isStatic ? "lib" : "dll");
        deploymentData.addFile(ti.workingDir + QLatin1Char('/') + targetFileName, targetPath);
        break;
    }
    case ProjectExplorer::Abi::MacOS:
        if (config.contains(QLatin1String("lib_bundle"))) {
            ti.workingDir.append(QLatin1Char('/')).append(ti.target)
                    .append(QLatin1String(".framework"));
        } else {
            targetFileName.prepend(QLatin1String("lib"));
            if (!isPlugin) {
                targetFileName += QLatin1Char('.');
                const QString version = node->singleVariableValue(VersionVar);
                QString majorVersion = version.left(version.indexOf(QLatin1Char('.')));
                if (majorVersion.isEmpty())
                    majorVersion = QLatin1String("1");
                targetFileName += majorVersion;
            }
            targetFileName += QLatin1Char('.');
            targetFileName += node->singleVariableValue(isStatic
                    ? StaticLibExtensionVar : ShLibExtensionVar);
        }
        deploymentData.addFile(ti.workingDir + QLatin1Char('/') + targetFileName, targetPath);
        break;
    case ProjectExplorer::Abi::LinuxOS:
    case ProjectExplorer::Abi::BsdOS:
    case ProjectExplorer::Abi::UnixOS:
        targetFileName.prepend(QLatin1String("lib"));
        targetFileName += QLatin1Char('.');
        if (isStatic) {
            targetFileName += QLatin1Char('a');
        } else {
            targetFileName += QLatin1String("so");
            deploymentData.addFile(ti.workingDir + QLatin1Char('/') + targetFileName, targetPath);
            if (!isPlugin) {
                QString version = node->singleVariableValue(VersionVar);
                if (version.isEmpty())
                    version = QLatin1String("1.0.0");
                targetFileName += QLatin1Char('.');
                while (true) {
                    deploymentData.addFile(ti.workingDir + QLatin1Char('/')
                            + targetFileName + version, targetPath);
                    const QString tmpVersion = version.left(version.lastIndexOf(QLatin1Char('.')));
                    if (tmpVersion == version)
                        break;
                    version = tmpVersion;
                }
            }
        }
        break;
    default:
        break;
    }
}

void Qt4Project::emitBuildDirectoryInitialized()
{
    emit buildDirectoryInitialized();
}
} // namespace Qt4ProjectManager

#include "qt4project.moc"
