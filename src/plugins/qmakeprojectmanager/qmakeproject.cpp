/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "qmakeproject.h"

#include "qmakeprojectmanager.h"
#include "qmakeprojectimporter.h"
#include "qmakebuildinfo.h"
#include "qmakestep.h"
#include "qmakenodes.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakebuildconfiguration.h"
#include "findqmakeprofiles.h"

#include <utils/algorithm.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpartbuilder.h>
#include <cpptools/projectpartheaderpath.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <proparser/qmakevfs.h>
#include <qtsupport/profilereader.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <resourceeditor/resourcenode.h>
#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMessageBox>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class QmakeProjectFile : public Core::IDocument
{
public:
    QmakeProjectFile(const QString &filePath);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
};

/// Watches folders for QmakePriFile nodes
/// use one file system watcher to watch all folders
/// such minimizing system ressouce usage

class CentralizedFolderWatcher : public QObject
{
    Q_OBJECT
public:
    CentralizedFolderWatcher(QmakeProject *parent);

    void watchFolders(const QList<QString> &folders, QmakePriFileNode *node);
    void unwatchFolders(const QList<QString> &folders, QmakePriFileNode *node);

private:
    void folderChanged(const QString &folder);
    void onTimer();
    void delayedFolderChanged(const QString &folder);

    QmakeProject *m_project;
    QSet<QString> recursiveDirs(const QString &folder);
    QFileSystemWatcher m_watcher;
    QMultiMap<QString, QmakePriFileNode *> m_map;

    QSet<QString> m_recursiveWatchedFolders;
    QTimer m_compressTimer;
    QSet<QString> m_changedFolders;
};

// QmakeProjectFiles: Struct for (Cached) lists of files in a project
class QmakeProjectFiles {
public:
    void clear();
    bool equals(const QmakeProjectFiles &f) const;

    QStringList files[static_cast<int>(FileType::FileTypeSize)];
    QStringList generatedFiles[static_cast<int>(FileType::FileTypeSize)];
    QStringList proFiles;
};

void QmakeProjectFiles::clear()
{
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        files[i].clear();
        generatedFiles[i].clear();
    }
    proFiles.clear();
}

bool QmakeProjectFiles::equals(const QmakeProjectFiles &f) const
{
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i)
        if (files[i] != f.files[i] || generatedFiles[i] != f.generatedFiles[i])
            return false;
    if (proFiles != f.proFiles)
        return false;
    return true;
}

inline bool operator==(const QmakeProjectFiles &f1, const QmakeProjectFiles &f2)
{       return f1.equals(f2); }

inline bool operator!=(const QmakeProjectFiles &f1, const QmakeProjectFiles &f2)
{       return !f1.equals(f2); }

QDebug operator<<(QDebug d, const  QmakeProjectFiles &f)
{
    QDebug nsp = d.nospace();
    nsp << "QmakeProjectFiles: proFiles=" <<  f.proFiles << '\n';
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i)
        nsp << "Type " << i << " files=" << f.files[i] <<  " generated=" << f.generatedFiles[i] << '\n';
    return d;
}

// A visitor to collect all files of a project in a QmakeProjectFiles struct
class ProjectFilesVisitor : public NodesVisitor
{
    ProjectFilesVisitor(QmakeProjectFiles *files);

public:
    static void findProjectFiles(QmakeProFileNode *rootNode, QmakeProjectFiles *files);

    void visitFolderNode(FolderNode *folderNode) final;

private:
    QmakeProjectFiles *m_files;
};

ProjectFilesVisitor::ProjectFilesVisitor(QmakeProjectFiles *files) :
    m_files(files)
{
}

namespace {
// uses std::unique, so takes a sorted list
void unique(QStringList &list)
{
    list.erase(std::unique(list.begin(), list.end()), list.end());
}
}

void ProjectFilesVisitor::findProjectFiles(QmakeProFileNode *rootNode, QmakeProjectFiles *files)
{
    files->clear();
    ProjectFilesVisitor visitor(files);
    rootNode->accept(&visitor);
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        Utils::sort(files->files[i]);
        unique(files->files[i]);
        Utils::sort(files->generatedFiles[i]);
        unique(files->generatedFiles[i]);
    }
    Utils::sort(files->proFiles);
    unique(files->proFiles);
}

void ProjectFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    if (ProjectNode *projectNode = folderNode->asProjectNode())
        m_files->proFiles.append(projectNode->filePath().toString());
    if (dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(folderNode))
        m_files->files[static_cast<int>(FileType::Resource)].push_back(folderNode->filePath().toString());

    foreach (FileNode *fileNode, folderNode->fileNodes()) {
        const int type = static_cast<int>(fileNode->fileType());
        QStringList &targetList = fileNode->isGenerated() ? m_files->generatedFiles[type] : m_files->files[type];
        targetList.push_back(fileNode->filePath().toString());
    }
}

}

// ----------- QmakeProjectFile
namespace Internal {
QmakeProjectFile::QmakeProjectFile(const QString &filePath)
{
    setId("Qmake.ProFile");
    setMimeType(QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE));
    setFilePath(FileName::fromString(filePath));
}

Core::IDocument::ReloadBehavior QmakeProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool QmakeProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

} // namespace Internal

/*!
  \class QmakeProject

  QmakeProject manages information about an individual Qt 4 (.pro) project file.
  */

QmakeProject::QmakeProject(QmakeManager *manager, const QString &fileName) :
    m_projectFiles(new QmakeProjectFiles),
    m_qmakeVfs(new QMakeVfs)
{
    setId(Constants::QMAKEPROJECT_ID);
    setProjectManager(manager);
    setDocument(new QmakeProjectFile(fileName));
    setProjectContext(Core::Context(QmakeProjectManager::Constants::PROJECT_ID));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setRequiredKitPredicate(QtSupport::QtKitInformation::qtVersionPredicate());

    const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
    m_qmakeVfs->setTextCodec(codec);

    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(3000);
    connect(&m_asyncUpdateTimer, &QTimer::timeout, this, &QmakeProject::asyncUpdate);

    setRootProjectNode(new QmakeProFileNode(this, projectFilePath()));

    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &QmakeProject::buildFinished);

    setPreferredKitPredicate([this](const Kit *kit) -> bool { return matchesKit(kit); });
}

QmakeProject::~QmakeProject()
{
    delete m_projectImporter;
    m_projectImporter = nullptr;
    m_codeModelFuture.cancel();
    m_asyncUpdateState = ShuttingDown;

    // Make sure root node (and associated readers) are shut hown before proceeding
    setRootProjectNode(nullptr);

    projectManager()->unregisterProject(this);
    delete m_projectFiles;
    m_cancelEvaluate = true;
    Q_ASSERT(m_qmakeGlobalsRefCnt == 0);
    delete m_qmakeVfs;
}

void QmakeProject::updateFileList()
{
    QmakeProjectFiles newFiles;
    ProjectFilesVisitor::findProjectFiles(rootProjectNode(), &newFiles);
    if (newFiles != *m_projectFiles) {
        *m_projectFiles = newFiles;
        emit fileListChanged();
    }
}

Project::RestoreResult QmakeProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    // Prune targets without buildconfigurations:
    // This can happen esp. when updating from a old version of Qt Creator
    QList<Target *>ts = targets();
    foreach (Target *t, ts) {
        if (t->buildConfigurations().isEmpty()) {
            qWarning() << "Removing" << t->id().name() << "since it has no buildconfigurations!";
            removeTarget(t);
        }
    }

    projectManager()->registerProject(this);

    // On active buildconfiguration changes, reevaluate the .pro files
    m_activeTarget = activeTarget();
    if (m_activeTarget) {
        connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                this, &QmakeProject::scheduleAsyncUpdateLater);
    }

    connect(this, &Project::activeTargetChanged,
            this, &QmakeProject::activeTargetWasChanged);

    scheduleAsyncUpdate(QmakeProFile::ParseNow);
    return RestoreResult::Ok;
}

void QmakeProject::updateCodeModels()
{
    if (activeTarget() && !activeTarget()->activeBuildConfiguration())
        return;

    updateCppCodeModel();
    updateQmlJSCodeModel();
}

void QmakeProject::updateCppCodeModel()
{
    using ProjectPart = CppTools::ProjectPart;

    m_toolChainWarnings.clear();

    const Kit *k = nullptr;
    if (Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::defaultKit();
    QTC_ASSERT(k, return);

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    ProjectPart::QtVersion qtVersionForPart = ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = ProjectPart::Qt4;
        else
            qtVersionForPart = ProjectPart::Qt5;
    }

    FindQmakeProFiles findQmakeProFiles;
    const QList<QmakeProFileNode *> proFiles = findQmakeProFiles(rootProjectNode());
    CppTools::ProjectInfo projectInfo(this);
    CppTools::ProjectPartBuilder ppBuilder(projectInfo);

    QList<ProjectExplorer::ExtraCompiler *> generators;

    foreach (QmakeProFileNode *pro, proFiles) {
        warnOnToolChainMismatch(pro);

        ppBuilder.setDisplayName(pro->displayName());
        ppBuilder.setProjectFile(pro->filePath().toString());
        ppBuilder.setCxxFlags(pro->variableValue(Variable::CppFlags)); // TODO: Handle QMAKE_CFLAGS
        ppBuilder.setDefines(pro->cxxDefines());
        ppBuilder.setPreCompiledHeaders(pro->variableValue(Variable::PrecompiledHeader));
        ppBuilder.setSelectedForBuilding(pro->includedInExactParse());

        // Qt Version
        if (pro->variableValue(Variable::Config).contains(QLatin1String("qt")))
            ppBuilder.setQtVersion(qtVersionForPart);
        else
            ppBuilder.setQtVersion(ProjectPart::NoQt);

        // Header paths
        CppTools::ProjectPartHeaderPaths headerPaths;
        using CppToolsHeaderPath = CppTools::ProjectPartHeaderPath;
        foreach (const QString &inc, pro->variableValue(Variable::IncludePath)) {
            const auto headerPath = CppToolsHeaderPath(inc, CppToolsHeaderPath::IncludePath);
            if (!headerPaths.contains(headerPath))
                headerPaths += headerPath;
        }

        if (qtVersion && !qtVersion->frameworkInstallPath().isEmpty()) {
            headerPaths += CppToolsHeaderPath(qtVersion->frameworkInstallPath(),
                                              CppToolsHeaderPath::FrameworkPath);
        }
        ppBuilder.setHeaderPaths(headerPaths);

        // Files and generators
        QStringList fileList = pro->variableValue(Variable::Source);
        QList<ProjectExplorer::ExtraCompiler *> proGenerators = pro->extraCompilers();
        foreach (ProjectExplorer::ExtraCompiler *ec, proGenerators) {
            ec->forEachTarget([&](const Utils::FileName &generatedFile) {
                fileList += generatedFile.toString();
            });
        }
        generators.append(proGenerators);

        const QList<Core::Id> languages = ppBuilder.createProjectPartsForFiles(fileList);
        foreach (const Core::Id &language, languages)
            setProjectLanguage(language, true);
    }

    CppTools::GeneratedCodeModelSupport::update(generators);
    m_codeModelFuture = CppTools::CppModelManager::instance()->updateProjectInfo(projectInfo);
}

void QmakeProject::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);

    FindQmakeProFiles findQt4ProFiles;
    QList<QmakeProFileNode *> proFiles = findQt4ProFiles(rootProjectNode());

    projectInfo.importPaths.clear();

    bool hasQmlLib = false;
    foreach (QmakeProFileNode *node, proFiles) {
        foreach (const QString &path, node->variableValue(Variable::QmlImportPath))
            projectInfo.importPaths.maybeInsert(FileName::fromString(path),
                                                QmlJS::Dialect::Qml);
        const QStringList &exactResources = node->variableValue(Variable::ExactResource);
        const QStringList &cumulativeResources = node->variableValue(Variable::CumulativeResource);
        projectInfo.activeResourceFiles.append(exactResources);
        projectInfo.allResourceFiles.append(exactResources);
        projectInfo.allResourceFiles.append(cumulativeResources);
        foreach (const QString &rc, exactResources) {
            QString contents;
            if (m_qmakeVfs->readVirtualFile(rc, QMakeVfs::VfsExact, &contents))
                projectInfo.resourceFileContents[rc] = contents;
        }
        foreach (const QString &rc, cumulativeResources) {
            QString contents;
            if (m_qmakeVfs->readVirtualFile(rc, QMakeVfs::VfsCumulative, &contents))
                projectInfo.resourceFileContents[rc] = contents;
        }
        if (!hasQmlLib) {
            QStringList qtLibs = node->variableValue(Variable::Qt);
            hasQmlLib = qtLibs.contains(QLatin1String("declarative")) ||
                    qtLibs.contains(QLatin1String("qml")) ||
                    qtLibs.contains(QLatin1String("quick"));
        }
    }

    // If the project directory has a pro/pri file that includes a qml or quick or declarative
    // library then chances of the project being a QML project is quite high.
    // This assumption fails when there are no QDeclarativeEngine/QDeclarativeView (QtQuick 1)
    // or QQmlEngine/QQuickView (QtQuick 2) instances.
    if (hasQmlLib)
        addProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID);

    projectInfo.activeResourceFiles.removeDuplicates();
    projectInfo.allResourceFiles.removeDuplicates();

    modelManager->updateProjectInfo(projectInfo, this);
}

void QmakeProject::updateRunConfigurations()
{
    if (activeTarget())
        activeTarget()->updateDefaultRunConfigurations();
}

void QmakeProject::scheduleAsyncUpdate(QmakeProFileNode *node, QmakeProFile::AsyncUpdateDelay delay)
{
    if (m_asyncUpdateState == ShuttingDown)
        return;

    if (m_cancelEvaluate) {
        // A cancel is in progress
        // That implies that a full update is going to happen afterwards
        // So we don't need to do anything
        return;
    }

    node->setParseInProgressRecursive(true);
    setAllBuildConfigurationsEnabled(false);

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        // Just postpone
        startAsyncTimer(delay);
    } else if (m_asyncUpdateState == AsyncPartialUpdatePending
               || m_asyncUpdateState == Base) {
        // Add the node
        m_asyncUpdateState = AsyncPartialUpdatePending;

        QList<QmakeProFileNode *>::iterator it;
        bool add = true;
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

        // Cancel running code model update
        m_codeModelFuture.cancel();

        startAsyncTimer(delay);
    } else if (m_asyncUpdateState == AsyncUpdateInProgress) {
        // A update is in progress
        // And this slot only gets called if a file changed on disc
        // So we'll play it safe and schedule a complete evaluate
        // This might trigger if due to version control a few files
        // change a partial update gets in progress and then another
        // batch of changes come in, which triggers a full update
        // even if that's not really needed
        scheduleAsyncUpdate(delay);
    }
}

void QmakeProject::scheduleAsyncUpdate(QmakeProFile::AsyncUpdateDelay delay)
{
    if (m_asyncUpdateState == ShuttingDown)
        return;

    if (m_cancelEvaluate) { // we are in progress of canceling
                            // and will start the evaluation after that
        return;
    }

    rootProjectNode()->setParseInProgressRecursive(true);
    setAllBuildConfigurationsEnabled(false);

    if (m_asyncUpdateState == AsyncUpdateInProgress) {
        m_cancelEvaluate = true;
        m_asyncUpdateState = AsyncFullUpdatePending;
        return;
    }

    m_partialEvaluate.clear();
    m_asyncUpdateState = AsyncFullUpdatePending;

    // Cancel running code model update
    m_codeModelFuture.cancel();
    startAsyncTimer(delay);
}

void QmakeProject::startAsyncTimer(QmakeProFile::AsyncUpdateDelay delay)
{
    m_asyncUpdateTimer.stop();
    m_asyncUpdateTimer.setInterval(qMin(m_asyncUpdateTimer.interval(), delay == QmakeProFile::ParseLater ? 3000 : 0));
    m_asyncUpdateTimer.start();
}

void QmakeProject::incrementPendingEvaluateFutures()
{
    ++m_pendingEvaluateFuturesCount;
    m_asyncUpdateFutureInterface->setProgressRange(m_asyncUpdateFutureInterface->progressMinimum(),
                                                  m_asyncUpdateFutureInterface->progressMaximum() + 1);
}

void QmakeProject::decrementPendingEvaluateFutures()
{
    --m_pendingEvaluateFuturesCount;

    m_asyncUpdateFutureInterface->setProgressValue(m_asyncUpdateFutureInterface->progressValue() + 1);
    if (m_pendingEvaluateFuturesCount == 0) {
        // We are done!

        m_asyncUpdateFutureInterface->reportFinished();
        delete m_asyncUpdateFutureInterface;
        m_asyncUpdateFutureInterface = nullptr;
        m_cancelEvaluate = false;

        // TODO clear the profile cache ?
        if (m_asyncUpdateState == AsyncFullUpdatePending || m_asyncUpdateState == AsyncPartialUpdatePending) {
            rootProjectNode()->setParseInProgressRecursive(true);
            setAllBuildConfigurationsEnabled(false);
            startAsyncTimer(QmakeProFile::ParseLater);
        } else  if (m_asyncUpdateState != ShuttingDown){
            // After being done, we need to call:
            setAllBuildConfigurationsEnabled(true);

            m_asyncUpdateState = Base;
            updateFileList();
            updateCodeModels();
            updateBuildSystemData();
            if (activeTarget())
                activeTarget()->updateDefaultDeployConfigurations();
            updateRunConfigurations();
            emit proFilesEvaluated();
            emit parsingFinished();
        }
    }
}

bool QmakeProject::wasEvaluateCanceled()
{
    return m_cancelEvaluate;
}

void QmakeProject::asyncUpdate()
{
    m_asyncUpdateTimer.setInterval(3000);

    m_qmakeVfs->invalidateCache();

    Q_ASSERT(!m_asyncUpdateFutureInterface);
    m_asyncUpdateFutureInterface = new QFutureInterface<void>();

    m_asyncUpdateFutureInterface->setProgressRange(0, 0);
    Core::ProgressManager::addTask(m_asyncUpdateFutureInterface->future(),
                                   tr("Reading Project \"%1\"").arg(displayName()),
                                   Constants::PROFILE_EVALUATE);

    m_asyncUpdateFutureInterface->reportStarted();

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        rootProjectNode()->asyncUpdate();
    } else {
        foreach (QmakeProFileNode *node, m_partialEvaluate)
            node->asyncUpdate();
    }

    m_partialEvaluate.clear();
    m_asyncUpdateState = AsyncUpdateInProgress;
}

void QmakeProject::buildFinished(bool success)
{
    if (success)
        m_qmakeVfs->invalidateContents();
}

QmakeManager *QmakeProject::projectManager() const
{
    return static_cast<QmakeManager *>(Project::projectManager());
}

bool QmakeProject::supportsKit(Kit *k, QString *errorMessage) const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version && errorMessage)
        *errorMessage = tr("No Qt version set in kit.");
    return version;
}

QString QmakeProject::displayName() const
{
    return projectFilePath().toFileInfo().completeBaseName();
}

QStringList QmakeProject::files(FilesMode fileMode) const
{
    QStringList files;
    for (int i = 0; i < static_cast<int>(FileType::FileTypeSize); ++i) {
        if (fileMode & SourceFiles)
            files += m_projectFiles->files[i];
        if (fileMode & GeneratedFiles)
            files += m_projectFiles->generatedFiles[i];
    }

    files.removeDuplicates();

    return files;
}

// Find the folder that contains a file with a certain name (recurse down)
static FolderNode *folderOf(FolderNode *in, const FileName &fileName)
{
    foreach (FileNode *fn, in->fileNodes())
        if (fn->filePath() == fileName)
            return in;
    foreach (FolderNode *folder, in->folderNodes())
        if (FolderNode *pn = folderOf(folder, fileName))
            return pn;
    return nullptr;
}

// Find the QmakeProFileNode that contains a certain file.
// First recurse down to folder, then find the pro-file.
static FileNode *fileNodeOf(QmakeProFileNode *in, const FileName &fileName)
{
    for (FolderNode *folder = folderOf(in, fileName); folder; folder = folder->parentFolderNode()) {
        if (QmakeProFileNode *proFile = dynamic_cast<QmakeProFileNode *>(folder)) {
            foreach (FileNode *fileNode, proFile->fileNodes()) {
                if (fileNode->filePath() == fileName)
                    return fileNode;
            }
        }
    }
    return nullptr;
}

QStringList QmakeProject::filesGeneratedFrom(const QString &input) const
{
    // Look in sub-profiles as SessionManager::projectForFile returns
    // the top-level project only.
    if (!rootProjectNode())
        return QStringList();

    if (const FileNode *file = fileNodeOf(rootProjectNode(), FileName::fromString(input))) {
        QmakeProFileNode *pro = static_cast<QmakeProFileNode *>(file->parentFolderNode());
        return pro->generatedFiles(pro->buildDir(), file);
    } else {
        return QStringList();
    }
}

void QmakeProject::proFileParseError(const QString &errorMessage)
{
    Core::MessageManager::write(errorMessage);
}

QtSupport::ProFileReader *QmakeProject::createProFileReader(const QmakeProFileNode *qmakeProFileNode, QmakeBuildConfiguration *bc)
{
    if (!m_qmakeGlobals) {
        m_qmakeGlobals = new QMakeGlobals;
        m_qmakeGlobalsRefCnt = 0;

        Kit *k;
        Environment env = Environment::systemEnvironment();
        QStringList qmakeArgs;
        if (!bc)
            bc = activeTarget() ? static_cast<QmakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration()) : nullptr;

        if (bc) {
            k = bc->target()->kit();
            env = bc->environment();
            if (QMakeStep *qs = bc->qmakeStep())
                qmakeArgs = qs->parserArguments();
            else
                qmakeArgs = bc->configCommandLineArguments();
        } else {
            k = KitManager::defaultKit();
        }

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
        m_qmakeSysroot = SysRootKitInformation::hasSysRoot(k)
                ? SysRootKitInformation::sysRoot(k).toString() : QString();

        if (qtVersion && qtVersion->isValid()) {
            m_qmakeGlobals->qmake_abslocation = QDir::cleanPath(qtVersion->qmakeCommand().toString());
            qtVersion->applyProperties(m_qmakeGlobals);
        }
        m_qmakeGlobals->setDirectories(rootProjectNode()->sourceDir(), rootProjectNode()->buildDir());

        Environment::const_iterator eit = env.constBegin(), eend = env.constEnd();
        for (; eit != eend; ++eit)
            m_qmakeGlobals->environment.insert(env.key(eit), env.value(eit));

        m_qmakeGlobals->setCommandLineArguments(rootProjectNode()->buildDir(), qmakeArgs);

        QtSupport::ProFileCacheManager::instance()->incRefCount();

        // On ios, qmake is called recursively, and the second call with a different
        // spec.
        // macx-ios-clang just creates supporting makefiles, and to avoid being
        // slow does not evaluate everything, and contains misleading information
        // (that is never used).
        // macx-xcode correctly evaluates the variables and generates the xcodeproject
        // that is actually used to build the application.
        //
        // It is important to override the spec file only for the creator evaluator,
        // and not the qmake buildstep used to build the app (as we use the makefiles).
        const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // from Ios::Constants
        if (qtVersion && qtVersion->type() == QLatin1String(IOSQT))
            m_qmakeGlobals->xqmakespec = QLatin1String("macx-xcode");
    }
    ++m_qmakeGlobalsRefCnt;

    auto reader = new QtSupport::ProFileReader(m_qmakeGlobals, m_qmakeVfs);

    reader->setOutputDir(qmakeProFileNode->buildDir());

    return reader;
}

QMakeGlobals *QmakeProject::qmakeGlobals()
{
    return m_qmakeGlobals;
}

QMakeVfs *QmakeProject::qmakeVfs()
{
    return m_qmakeVfs;
}

QString QmakeProject::qmakeSysroot()
{
    return m_qmakeSysroot;
}

void QmakeProject::destroyProFileReader(QtSupport::ProFileReader *reader)
{
    delete reader;
    if (!--m_qmakeGlobalsRefCnt) {
        QString dir = projectFilePath().toString();
        if (!dir.endsWith(QLatin1Char('/')))
            dir += QLatin1Char('/');
        QtSupport::ProFileCacheManager::instance()->discardFiles(dir);
        QtSupport::ProFileCacheManager::instance()->decRefCount();

        delete m_qmakeGlobals;
        m_qmakeGlobals = nullptr;
    }
}

QmakeProFileNode *QmakeProject::rootProjectNode() const
{
    return static_cast<QmakeProFileNode *>(Project::rootProjectNode());
}

bool QmakeProject::validParse(const FileName &proFilePath) const
{
    if (!rootProjectNode())
        return false;
    const QmakeProFileNode *node = rootProjectNode()->findProFileFor(proFilePath);
    return node && node->validParse();
}

bool QmakeProject::parseInProgress(const FileName &proFilePath) const
{
    if (!rootProjectNode())
        return false;
    const QmakeProFileNode *node = rootProjectNode()->findProFileFor(proFilePath);
    return node && node->parseInProgress();
}

void QmakeProject::collectAllProFiles(QList<QmakeProFileNode *> &list, QmakeProFileNode *node, Parsing parse,
                                      const QList<ProjectType> &projectTypes)
{
    if (parse == ExactAndCumulativeParse || node->includedInExactParse())
        if (projectTypes.isEmpty() || projectTypes.contains(node->projectType()))
            list.append(node);
    foreach (ProjectNode *n, node->projectNodes()) {
        QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(n);
        if (qmakeProFileNode)
            collectAllProFiles(list, qmakeProFileNode, parse, projectTypes);
    }
}

QList<QmakeProFileNode *> QmakeProject::applicationProFiles(Parsing parse) const
{
    return allProFiles({ ProjectType::ApplicationTemplate, ProjectType::ScriptTemplate }, parse);
}

QList<QmakeProFileNode *> QmakeProject::allProFiles(const QList<ProjectType> &projectTypes, Parsing parse) const
{
    QList<QmakeProFileNode *> list;
    if (!rootProjectNode())
        return list;
    collectAllProFiles(list, rootProjectNode(), parse, projectTypes);
    return list;
}

bool QmakeProject::hasApplicationProFile(const FileName &path) const
{
    if (path.isEmpty())
        return false;

    QList<QmakeProFileNode *> list = applicationProFiles();
    foreach (QmakeProFileNode * node, list)
        if (node->filePath() == path)
            return true;
    return false;
}

QList<QmakeProFileNode *> QmakeProject::nodesWithQtcRunnable(QList<QmakeProFileNode *> nodes)
{
    std::function<bool (QmakeProFileNode *)> hasQtcRunnable = [](QmakeProFileNode *node) {
        return node->isQtcRunnable();
    };

    if (anyOf(nodes, hasQtcRunnable))
        erase(nodes, std::not1(hasQtcRunnable));
    return nodes;
}

QList<Core::Id> QmakeProject::idsForNodes(Core::Id base, const QList<QmakeProFileNode *> &nodes)
{
    return Utils::transform(nodes, [&base](QmakeProFileNode *node) {
        return base.withSuffix(node->filePath().toString());
    });
}

void QmakeProject::activeTargetWasChanged()
{
    if (m_activeTarget) {
        disconnect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                   this, &QmakeProject::scheduleAsyncUpdateLater);
    }

    m_activeTarget = activeTarget();

    if (!m_activeTarget)
        return;

    connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
            this, &QmakeProject::scheduleAsyncUpdateLater);

    scheduleAsyncUpdate();
}

void QmakeProject::setAllBuildConfigurationsEnabled(bool enabled)
{
    foreach (Target *t, targets()) {
        foreach (BuildConfiguration *bc, t->buildConfigurations()) {
            auto qmakeBc = qobject_cast<QmakeBuildConfiguration *>(bc);
            if (qmakeBc)
                qmakeBc->setEnabled(enabled);
        }
    }
}

bool QmakeProject::hasSubNode(QmakePriFileNode *root, const FileName &path)
{
    if (root->filePath() == path)
        return true;
    foreach (FolderNode *fn, root->folderNodes()) {
        if (dynamic_cast<QmakeProFileNode *>(fn)) {
            // we aren't interested in pro file nodes
        } else if (QmakePriFileNode *qt4prifilenode = dynamic_cast<QmakePriFileNode *>(fn)) {
            if (hasSubNode(qt4prifilenode, path))
                return true;
        }
    }
    return false;
}

void QmakeProject::findProFile(const FileName &fileName, QmakeProFileNode *root, QList<QmakeProFileNode *> &list)
{
    if (hasSubNode(root, fileName))
        list.append(root);

    foreach (FolderNode *fn, root->folderNodes())
        if (QmakeProFileNode *qt4proFileNode = dynamic_cast<QmakeProFileNode *>(fn))
            findProFile(fileName, qt4proFileNode, list);
}

void QmakeProject::notifyChanged(const FileName &name)
{
    if (files(QmakeProject::SourceFiles).contains(name.toString())) {
        QList<QmakeProFileNode *> list;
        findProFile(name, rootProjectNode(), list);
        foreach (QmakeProFileNode *node, list) {
            QtSupport::ProFileCacheManager::instance()->discardFile(name.toString());
            node->scheduleUpdate(QmakeProFile::ParseNow);
        }
    }
}

void QmakeProject::watchFolders(const QStringList &l, QmakePriFileNode *node)
{
    if (l.isEmpty())
        return;
    if (!m_centralizedFolderWatcher)
        m_centralizedFolderWatcher = new Internal::CentralizedFolderWatcher(this);
    m_centralizedFolderWatcher->watchFolders(l, node);
}

void QmakeProject::unwatchFolders(const QStringList &l, QmakePriFileNode *node)
{
    if (m_centralizedFolderWatcher && !l.isEmpty())
        m_centralizedFolderWatcher->unwatchFolders(l, node);
}

/////////////
/// Centralized Folder Watcher
////////////

// All the folder have a trailing slash!
CentralizedFolderWatcher::CentralizedFolderWatcher(QmakeProject *parent)
    : QObject(parent), m_project(parent)
{
    m_compressTimer.setSingleShot(true);
    m_compressTimer.setInterval(200);
    connect(&m_compressTimer, &QTimer::timeout, this, &CentralizedFolderWatcher::onTimer);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &CentralizedFolderWatcher::folderChanged);
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

void CentralizedFolderWatcher::watchFolders(const QList<QString> &folders, QmakePriFileNode *node)
{
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
    }
}

void CentralizedFolderWatcher::unwatchFolders(const QList<QString> &folders, QmakePriFileNode *node)
{
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
                auto end = m_map.constEnd();
                for (auto it = m_map.constBegin(); it != end; ++it) {
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

        foreach (const QString &tr, toRemove)
            m_recursiveWatchedFolders.remove(tr);
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
    // Figure out whom to inform
    QString dir = folder;
    const QChar slash = QLatin1Char('/');
    bool newOrRemovedFiles = false;
    while (true) {
        if (!dir.endsWith(slash))
            dir.append(slash);
        QList<QmakePriFileNode *> nodes = m_map.values(dir);
        if (!nodes.isEmpty()) {
            // Collect all the files
            QSet<FileName> newFiles;
            newFiles += QmakePriFileNode::recursiveEnumerate(folder);
            foreach (QmakePriFileNode *node, nodes) {
                newOrRemovedFiles = newOrRemovedFiles
                        || node->folderChanged(folder, newFiles);
            }
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
        QSet<QString> alreadyAdded = m_watcher.directories().toSet();
        tmp.subtract(alreadyAdded);
        if (!tmp.isEmpty())
            m_watcher.addPaths(tmp.toList());
        m_recursiveWatchedFolders += tmp;
    }

    if (newOrRemovedFiles) {
        m_project->updateFileList();
        m_project->updateCodeModels();
    }
}

bool QmakeProject::needsConfiguration() const
{
    return targets().isEmpty();
}

void QmakeProject::configureAsExampleProject(const QSet<Core::Id> &platforms)
{
    QList<const BuildInfo *> infoList;
    QList<Kit *> kits = KitManager::kits();
    foreach (Kit *k, kits) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        if (!version
                || (!platforms.isEmpty()
                    && !Utils::contains(version->targetDeviceTypes(), [platforms](Core::Id i) { return platforms.contains(i); })))
            continue;

        IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(k, projectFilePath().toString());
        if (!factory)
            continue;
        foreach (BuildInfo *info, factory->availableSetups(k, projectFilePath().toString()))
            infoList << info;
    }
    setup(infoList);
    qDeleteAll(infoList);
}

bool QmakeProject::requiresTargetPanel() const
{
    return !targets().isEmpty();
}

// All the Qmake run configurations should share code.
// This is a rather suboptimal way to do that for disabledReason()
// but more pratical then duplicated the code everywhere
QString QmakeProject::disabledReasonForRunConfiguration(const FileName &proFilePath)
{
    if (!proFilePath.exists())
        return tr("The .pro file \"%1\" does not exist.")
                .arg(proFilePath.fileName());

    if (!rootProjectNode()) // Shutting down
        return QString();

    if (!rootProjectNode()->findProFileFor(proFilePath))
        return tr("The .pro file \"%1\" is not part of the project.")
                .arg(proFilePath.fileName());

    return tr("The .pro file \"%1\" could not be parsed.")
            .arg(proFilePath.fileName());
}

QString QmakeProject::buildNameFor(const Kit *k)
{
    if (!k)
        return QLatin1String("unknown");

    return k->fileSystemFriendlyName();
}

void QmakeProject::updateBuildSystemData()
{
    Target *const target = activeTarget();
    if (!target)
        return;
    const QmakeProFileNode * const rootNode = rootProjectNode();
    if (!rootNode || rootNode->parseInProgress())
        return;

    DeploymentData deploymentData;
    collectData(rootNode, deploymentData);
    target->setDeploymentData(deploymentData);

    BuildTargetInfoList appTargetList;
    foreach (const QmakeProFileNode * const node, applicationProFiles()) {
        appTargetList.list << BuildTargetInfo(node->targetInformation().target,
                                              FileName::fromString(executableFor(node)),
                                              node->filePath());
    }
    target->setApplicationTargets(appTargetList);
}

void QmakeProject::collectData(const QmakeProFileNode *node, DeploymentData &deploymentData)
{
    if (!node->isSubProjectDeployable(node->filePath().toString()))
        return;

    const InstallsList &installsList = node->installsList();
    foreach (const InstallsItem &item, installsList.items) {
        if (!item.active)
            continue;
        foreach (const auto &localFile, item.files)
            deploymentData.addFile(localFile.fileName, item.path);
    }

    switch (node->projectType()) {
    case ProjectType::ApplicationTemplate:
        if (!installsList.targetPath.isEmpty())
            collectApplicationData(node, deploymentData);
        break;
    case ProjectType::SharedLibraryTemplate:
    case ProjectType::StaticLibraryTemplate:
        collectLibraryData(node, deploymentData);
        break;
    case ProjectType::SubDirsTemplate:
        foreach (const ProjectNode * const subProject, node->subProjectNodesExact()) {
            const QmakeProFileNode * const qt4SubProject
                    = dynamic_cast<const QmakeProFileNode *>(subProject);
            if (!qt4SubProject)
                continue;
            collectData(qt4SubProject, deploymentData);
        }
        break;
    default:
        break;
    }
}

void QmakeProject::collectApplicationData(const QmakeProFileNode *node, DeploymentData &deploymentData)
{
    QString executable = executableFor(node);
    if (!executable.isEmpty())
        deploymentData.addFile(executable, node->installsList().targetPath,
                               DeployableFile::TypeExecutable);
}

static QString destDirFor(const TargetInformation &ti)
{
    if (ti.destDir.isEmpty())
        return ti.buildDir;
    if (QDir::isRelativePath(ti.destDir))
        return QDir::cleanPath(ti.buildDir + QLatin1Char('/') + ti.destDir);
    return ti.destDir;
}

void QmakeProject::collectLibraryData(const QmakeProFileNode *node, DeploymentData &deploymentData)
{
    const QString targetPath = node->installsList().targetPath;
    if (targetPath.isEmpty())
        return;
    const Kit * const kit = activeTarget()->kit();
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        return;

    TargetInformation ti = node->targetInformation();
    QString targetFileName = ti.target;
    const QStringList config = node->variableValue(Variable::Config);
    const bool isStatic = config.contains(QLatin1String("static"));
    const bool isPlugin = config.contains(QLatin1String("plugin"));
    switch (toolchain->targetAbi().os()) {
    case Abi::WindowsOS: {
        QString targetVersionExt = node->singleVariableValue(Variable::TargetVersionExt);
        if (targetVersionExt.isEmpty()) {
            const QString version = node->singleVariableValue(Variable::Version);
            if (!version.isEmpty()) {
                targetVersionExt = version.left(version.indexOf(QLatin1Char('.')));
                if (targetVersionExt == QLatin1String("0"))
                    targetVersionExt.clear();
            }
        }
        targetFileName += targetVersionExt + QLatin1Char('.');
        targetFileName += QLatin1String(isStatic ? "lib" : "dll");
        deploymentData.addFile(destDirFor(ti) + QLatin1Char('/') + targetFileName, targetPath);
        break;
    }
    case Abi::DarwinOS: {
        QString destDir = destDirFor(ti);
        if (config.contains(QLatin1String("lib_bundle"))) {
            destDir.append(QLatin1Char('/')).append(ti.target)
                    .append(QLatin1String(".framework"));
        } else {
            if (!(isPlugin && config.contains(QLatin1String("no_plugin_name_prefix"))))
                targetFileName.prepend(QLatin1String("lib"));

            if (!isPlugin) {
                targetFileName += QLatin1Char('.');
                const QString version = node->singleVariableValue(Variable::Version);
                QString majorVersion = version.left(version.indexOf(QLatin1Char('.')));
                if (majorVersion.isEmpty())
                    majorVersion = QLatin1String("1");
                targetFileName += majorVersion;
            }
            targetFileName += QLatin1Char('.');
            targetFileName += node->singleVariableValue(isStatic
                    ? Variable::StaticLibExtension : Variable::ShLibExtension);
        }
        deploymentData.addFile(destDir + QLatin1Char('/') + targetFileName, targetPath);
        break;
    }
    case Abi::LinuxOS:
    case Abi::BsdOS:
    case Abi::UnixOS:
        if (!(isPlugin && config.contains(QLatin1String("no_plugin_name_prefix"))))
            targetFileName.prepend(QLatin1String("lib"));

        targetFileName += QLatin1Char('.');
        if (isStatic) {
            targetFileName += QLatin1Char('a');
        } else {
            targetFileName += QLatin1String("so");
            deploymentData.addFile(destDirFor(ti) + QLatin1Char('/') + targetFileName, targetPath);
            if (!isPlugin) {
                QString version = node->singleVariableValue(Variable::Version);
                if (version.isEmpty())
                    version = QLatin1String("1.0.0");
                targetFileName += QLatin1Char('.');
                while (true) {
                    deploymentData.addFile(destDirFor(ti) + QLatin1Char('/')
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

bool QmakeProject::matchesKit(const Kit *kit)
{
    FileName filePath = projectFilePath();
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);

    return QtSupport::QtVersionManager::version([&filePath, version](const QtSupport::BaseQtVersion *v) {
        return v->isValid() && v->isSubProject(filePath) && v == version;
    });
}

static Utils::FileName getFullPathOf(const QmakeProFileNode *pro, Variable variable,
                                     const BuildConfiguration *bc)
{
    // Take last non-flag value, to cover e.g. '@echo $< && $$QMAKE_CC' or 'ccache gcc'
    const QStringList values = Utils::filtered(pro->variableValue(variable),
                                               [](const QString &value) {
        return !value.startsWith('-');
    });
    if (values.isEmpty())
        return Utils::FileName();
    const QString exe = values.last();
    QTC_ASSERT(bc, return Utils::FileName::fromString(exe));
    QFileInfo fi(exe);
    if (fi.isAbsolute())
        return Utils::FileName::fromString(exe);

    return bc->environment().searchInPath(exe);
}

void QmakeProject::testToolChain(ToolChain *tc, const Utils::FileName &path) const
{
    if (!tc || path.isEmpty())
        return;

    const Utils::FileName expected = tc->compilerCommand();
    if (expected != path) {
        const QPair<Utils::FileName, Utils::FileName> pair = qMakePair(expected, path);
        if (!m_toolChainWarnings.contains(pair)) {
            TaskHub::addTask(Task(Task::Warning,
                                  QCoreApplication::translate("QmakeProjectManager", "\"%1\" is used by qmake, but \"%2\" is configured in the kit.\n"
                                                              "Please update your kit or choose a mkspec for qmake that matches your target environment better.").
                                  arg(path.toUserOutput()).arg(expected.toUserOutput()),
                                  Utils::FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
            m_toolChainWarnings.insert(pair);
        }
    }
}

void QmakeProject::warnOnToolChainMismatch(const QmakeProFileNode *pro) const
{
    const Target *t = activeTarget();
    const BuildConfiguration *bc = t ? t->activeBuildConfiguration() : nullptr;
    if (!bc)
        return;

    testToolChain(ToolChainKitInformation::toolChain(t->kit(), ProjectExplorer::Constants::C_LANGUAGE_ID),
                  getFullPathOf(pro, Variable::QmakeCc, bc));
    testToolChain(ToolChainKitInformation::toolChain(t->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID),
                  getFullPathOf(pro, Variable::QmakeCxx, bc));
}

QString QmakeProject::executableFor(const QmakeProFileNode *node)
{
    const Kit * const kit = activeTarget()->kit();
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        return QString();

    TargetInformation ti = node->targetInformation();
    QString target;

    switch (toolchain->targetAbi().os()) {
    case Abi::DarwinOS:
        if (node->variableValue(Variable::Config).contains(QLatin1String("app_bundle"))) {
            target = ti.target + QLatin1String(".app/Contents/MacOS/") + ti.target;
            break;
        }
        // else fall through
    default: {
        QString extension = node->singleVariableValue(Variable::TargetExt);
        target = ti.target + extension;
        break;
    }
    }
    return QDir(destDirFor(ti)).absoluteFilePath(target);
}

void QmakeProject::emitBuildDirectoryInitialized()
{
    emit buildDirectoryInitialized();
}

ProjectImporter *QmakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new QmakeProjectImporter(projectFilePath());
    return m_projectImporter;
}

QmakeProject::AsyncUpdateState QmakeProject::asyncUpdateState() const
{
    return m_asyncUpdateState;
}

QString QmakeProject::mapProFilePathToTarget(const FileName &proFilePath)
{
    const QmakeProjectManager::QmakeProFileNode *root = rootProjectNode();
    const QmakeProjectManager::QmakeProFileNode *node = root ? root->findProFileFor(proFilePath) : nullptr;
    return node ? node->targetInformation().target : QString();
}

} // namespace QmakeProjectManager

#include "qmakeproject.moc"
