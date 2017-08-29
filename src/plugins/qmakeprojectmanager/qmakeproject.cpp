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
#include "qmakenodetreebuilder.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakebuildconfiguration.h"

#include <utils/algorithm.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpprawprojectpart.h>
#include <cpptools/projectinfo.h>
#include <cpptools/projectpartheaderpath.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/cppmodelmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
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

const int UPDATE_INTERVAL = 3000;

/// Watches folders for QmakePriFile nodes
/// use one file system watcher to watch all folders
/// such minimizing system ressouce usage

class CentralizedFolderWatcher : public QObject
{
    Q_OBJECT
public:
    CentralizedFolderWatcher(QmakeProject *parent);

    void watchFolders(const QList<QString> &folders, QmakePriFile *file);
    void unwatchFolders(const QList<QString> &folders, QmakePriFile *file);

private:
    void folderChanged(const QString &folder);
    void onTimer();
    void delayedFolderChanged(const QString &folder);

    QmakeProject *m_project;
    QSet<QString> recursiveDirs(const QString &folder);
    QFileSystemWatcher m_watcher;
    QMultiMap<QString, QmakePriFile *> m_map;

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

static QList<QmakeProject *> s_projects;

} // namespace Internal

/*!
  \class QmakeProject

  QmakeProject manages information about an individual Qt 4 (.pro) project file.
  */

QmakeProject::QmakeProject(const FileName &fileName) :
    Project(QmakeProjectManager::Constants::PROFILE_MIMETYPE, fileName),
    m_qmakeVfs(new QMakeVfs),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater(this))
{
    s_projects.append(this);
    setId(Constants::QMAKEPROJECT_ID);
    setProjectContext(Core::Context(QmakeProjectManager::Constants::PROJECT_ID));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setRequiredKitPredicate(QtSupport::QtKitInformation::qtVersionPredicate());
    setDisplayName(fileName.toFileInfo().completeBaseName());

    const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
    m_qmakeVfs->setTextCodec(codec);

    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);
    connect(&m_asyncUpdateTimer, &QTimer::timeout, this, &QmakeProject::asyncUpdate);

    m_rootProFile = std::make_unique<QmakeProFile>(this, projectFilePath());

    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &QmakeProject::buildFinished);

    setPreferredKitPredicate([this](const Kit *kit) -> bool { return matchesKit(kit); });
}

QmakeProject::~QmakeProject()
{
    s_projects.removeOne(this);
    delete m_projectImporter;
    m_projectImporter = nullptr;
    delete m_cppCodeModelUpdater;
    m_cppCodeModelUpdater = nullptr;
    m_asyncUpdateState = ShuttingDown;

    // Make sure root node (and associated readers) are shut hown before proceeding
    setRootProjectNode(nullptr);
    m_rootProFile.reset();

    m_cancelEvaluate = true;
    Q_ASSERT(m_qmakeGlobalsRefCnt == 0);
    delete m_qmakeVfs;
}

QmakeProFile *QmakeProject::rootProFile() const
{
    return m_rootProFile.get();
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

    ToolChain *cToolChain
            = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);
    ToolChain *cxxToolChain
            = ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);

    m_cppCodeModelUpdater->cancel();

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    ProjectPart::QtVersion qtVersionForPart = ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() <= QtSupport::QtVersionNumber(4,8,6))
            qtVersionForPart = ProjectPart::Qt4_8_6AndOlder;
        else if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = ProjectPart::Qt4Latest;
        else
            qtVersionForPart = ProjectPart::Qt5;
    }

    const QList<QmakeProFile *> proFiles = rootProFile()->allProFiles();

    QList<ProjectExplorer::ExtraCompiler *> generators;
    CppTools::RawProjectParts rpps;
    for (const QmakeProFile *pro : proFiles) {
        warnOnToolChainMismatch(pro);

        CppTools::RawProjectPart rpp;
        rpp.setDisplayName(pro->displayName());
        rpp.setProjectFileLocation(pro->filePath().toString());
        rpp.setBuildSystemTarget(pro->targetInformation().target);
        const bool isExecutable = pro->projectType() == ProjectType::ApplicationTemplate;
        rpp.setBuildTargetType(isExecutable ? CppTools::ProjectPart::Executable
                                            : CppTools::ProjectPart::Library);

        // TODO: Handle QMAKE_CFLAGS
        rpp.setFlagsForCxx({cxxToolChain, pro->variableValue(Variable::CppFlags)});
        rpp.setMacros(ProjectExplorer::Macro::toMacros(pro->cxxDefines()));
        rpp.setPreCompiledHeaders(pro->variableValue(Variable::PrecompiledHeader));
        rpp.setSelectedForBuilding(pro->includedInExactParse());

        // Qt Version
        if (pro->variableValue(Variable::Config).contains(QLatin1String("qt")))
            rpp.setQtVersion(qtVersionForPart);
        else
            rpp.setQtVersion(ProjectPart::NoQt);

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
        rpp.setHeaderPaths(headerPaths);

        // Files and generators
        QStringList fileList = pro->variableValue(Variable::Source);
        QList<ProjectExplorer::ExtraCompiler *> proGenerators = pro->extraCompilers();
        foreach (ProjectExplorer::ExtraCompiler *ec, proGenerators) {
            ec->forEachTarget([&](const Utils::FileName &generatedFile) {
                fileList += generatedFile.toString();
            });
        }
        generators.append(proGenerators);
        fileList.prepend(CppTools::CppModelManager::configurationFileName());
        rpp.setFiles(fileList);

        rpps.append(rpp);
    }

    CppTools::GeneratedCodeModelSupport::update(generators);
    m_cppCodeModelUpdater->update({this, cToolChain, cxxToolChain, k, rpps});
}

void QmakeProject::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);

    const QList<QmakeProFile *> proFiles = rootProFile()->allProFiles();

    projectInfo.importPaths.clear();

    bool hasQmlLib = false;
    for (QmakeProFile *file : proFiles) {
        for (const QString &path : file->variableValue(Variable::QmlImportPath)) {
            projectInfo.importPaths.maybeInsert(FileName::fromString(path),
                                                QmlJS::Dialect::Qml);
        }
        const QStringList &exactResources = file->variableValue(Variable::ExactResource);
        const QStringList &cumulativeResources = file->variableValue(Variable::CumulativeResource);
        projectInfo.activeResourceFiles.append(exactResources);
        projectInfo.allResourceFiles.append(exactResources);
        projectInfo.allResourceFiles.append(cumulativeResources);
        QString errorMessage;
        foreach (const QString &rc, exactResources) {
            QString contents;
            if (m_qmakeVfs->readFile(rc, QMakeVfs::VfsExact, &contents, &errorMessage) == QMakeVfs::ReadOk)
                projectInfo.resourceFileContents[rc] = contents;
        }
        foreach (const QString &rc, cumulativeResources) {
            QString contents;
            if (m_qmakeVfs->readFile(rc, QMakeVfs::VfsCumulative, &contents, &errorMessage) == QMakeVfs::ReadOk)
                projectInfo.resourceFileContents[rc] = contents;
        }
        if (!hasQmlLib) {
            QStringList qtLibs = file->variableValue(Variable::Qt);
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

void QmakeProject::scheduleAsyncUpdate(QmakeProFile *file, QmakeProFile::AsyncUpdateDelay delay)
{
    if (m_asyncUpdateState == ShuttingDown)
        return;

    if (m_cancelEvaluate) {
        // A cancel is in progress
        // That implies that a full update is going to happen afterwards
        // So we don't need to do anything
        return;
    }

    emitParsingStarted();
    file->setParseInProgressRecursive(true);
    setAllBuildConfigurationsEnabled(false);

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        // Just postpone
        startAsyncTimer(delay);
    } else if (m_asyncUpdateState == AsyncPartialUpdatePending
               || m_asyncUpdateState == Base) {
        // Add the node
        m_asyncUpdateState = AsyncPartialUpdatePending;

        bool add = true;
        auto it = m_partialEvaluate.begin();
        while (it != m_partialEvaluate.end()) {
            if (*it == file) {
                add = false;
                break;
            } else if (file->isParent(*it)) { // We already have the parent in the list, nothing to do
                it = m_partialEvaluate.erase(it);
            } else if ((*it)->isParent(file)) { // The node is the parent of a child already in the list
                add = false;
                break;
            } else {
                ++it;
            }
        }

        if (add)
            m_partialEvaluate.append(file);

        // Cancel running code model update
        m_cppCodeModelUpdater->cancel();

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

    emitParsingStarted();
    rootProFile()->setParseInProgressRecursive(true);
    setAllBuildConfigurationsEnabled(false);

    if (m_asyncUpdateState == AsyncUpdateInProgress) {
        m_cancelEvaluate = true;
        m_asyncUpdateState = AsyncFullUpdatePending;
        return;
    }

    m_partialEvaluate.clear();
    m_asyncUpdateState = AsyncFullUpdatePending;

    // Cancel running code model update
    m_cppCodeModelUpdater->cancel();
    startAsyncTimer(delay);
}

void QmakeProject::startAsyncTimer(QmakeProFile::AsyncUpdateDelay delay)
{
    m_asyncUpdateTimer.stop();
    m_asyncUpdateTimer.setInterval(qMin(m_asyncUpdateTimer.interval(),
                                        delay == QmakeProFile::ParseLater ? UPDATE_INTERVAL : 0));
    m_asyncUpdateTimer.start();
}

void QmakeProject::incrementPendingEvaluateFutures()
{
    ++m_pendingEvaluateFuturesCount;
    if (m_pendingEvaluateFuturesCount == 1)
        m_totalEvaluationSuccess = true;
    m_asyncUpdateFutureInterface->setProgressRange(m_asyncUpdateFutureInterface->progressMinimum(),
                                                   m_asyncUpdateFutureInterface->progressMaximum() + 1);
}

void QmakeProject::decrementPendingEvaluateFutures(bool success)
{
    --m_pendingEvaluateFuturesCount;

    m_totalEvaluationSuccess = m_totalEvaluationSuccess && success;

    m_asyncUpdateFutureInterface->setProgressValue(m_asyncUpdateFutureInterface->progressValue() + 1);
    if (m_pendingEvaluateFuturesCount == 0) {
        // We are done!
        setRootProjectNode(QmakeNodeTreeBuilder::buildTree(this));

        if (!m_totalEvaluationSuccess)
            m_asyncUpdateFutureInterface->reportCanceled();

        m_asyncUpdateFutureInterface->reportFinished();
        delete m_asyncUpdateFutureInterface;
        m_asyncUpdateFutureInterface = nullptr;
        m_cancelEvaluate = false;

        // TODO clear the profile cache ?
        if (m_asyncUpdateState == AsyncFullUpdatePending || m_asyncUpdateState == AsyncPartialUpdatePending) {
            // Already parsing!
            rootProFile()->setParseInProgressRecursive(true);
            setAllBuildConfigurationsEnabled(false);
            startAsyncTimer(QmakeProFile::ParseLater);
        } else  if (m_asyncUpdateState != ShuttingDown){
            // After being done, we need to call:
            setAllBuildConfigurationsEnabled(true);

            m_asyncUpdateState = Base;
            updateCodeModels();
            updateBuildSystemData();
            if (activeTarget())
                activeTarget()->updateDefaultDeployConfigurations();
            updateRunConfigurations();
            emit proFilesEvaluated();
            emitParsingFinished(true); // Qmake always returns (some) data, even when it failed:-)
        }
    }
}

bool QmakeProject::wasEvaluateCanceled()
{
    return m_cancelEvaluate;
}

void QmakeProject::asyncUpdate()
{
    m_asyncUpdateTimer.setInterval(UPDATE_INTERVAL);

    m_qmakeVfs->invalidateCache();

    Q_ASSERT(!m_asyncUpdateFutureInterface);
    m_asyncUpdateFutureInterface = new QFutureInterface<void>();

    m_asyncUpdateFutureInterface->setProgressRange(0, 0);
    Core::ProgressManager::addTask(m_asyncUpdateFutureInterface->future(),
                                   tr("Reading Project \"%1\"").arg(displayName()),
                                   Constants::PROFILE_EVALUATE);

    m_asyncUpdateFutureInterface->reportStarted();

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        rootProFile()->asyncUpdate();
    } else {
        foreach (QmakeProFile *file, m_partialEvaluate)
            file->asyncUpdate();
    }

    m_partialEvaluate.clear();
    m_asyncUpdateState = AsyncUpdateInProgress;
}

void QmakeProject::buildFinished(bool success)
{
    if (success)
        m_qmakeVfs->invalidateContents();
}

bool QmakeProject::supportsKit(Kit *k, QString *errorMessage) const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
    if (!version && errorMessage)
        *errorMessage = tr("No Qt version set in kit.");
    return version;
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
    if (!rootProjectNode())
        return { };

    if (const FileNode *file = fileNodeOf(rootProjectNode(), FileName::fromString(input))) {
        const QmakeProFileNode *pro = static_cast<QmakeProFileNode *>(file->parentFolderNode());
        QTC_ASSERT(pro, return {});
        if (const QmakeProFile *proFile = pro->proFile())
            return Utils::transform(proFile->generatedFiles(FileName::fromString(pro->buildDir()),
                                                            file->filePath(), file->fileType()),
                                    &FileName::toString);
    }
    return { };
}

void QmakeProject::proFileParseError(const QString &errorMessage)
{
    Core::MessageManager::write(errorMessage);
}

QtSupport::ProFileReader *QmakeProject::createProFileReader(const QmakeProFile *qmakeProFile)
{
    if (!m_qmakeGlobals) {
        m_qmakeGlobals = new QMakeGlobals;
        m_qmakeGlobalsRefCnt = 0;

        Kit *k = KitManager::defaultKit();
        Environment env = Environment::systemEnvironment();
        QStringList qmakeArgs;

        if (Target *t = activeTarget()) {
            k = t->kit();
            if (auto bc = static_cast<QmakeBuildConfiguration *>(t->activeBuildConfiguration())) {
                env = bc->environment();
                if (QMakeStep *qs = bc->qmakeStep())
                    qmakeArgs = qs->parserArguments();
                else
                    qmakeArgs = bc->configCommandLineArguments();
            }
        }

        QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
        m_qmakeSysroot = SysRootKitInformation::hasSysRoot(k)
                ? SysRootKitInformation::sysRoot(k).toString() : QString();

        if (qtVersion && qtVersion->isValid()) {
            m_qmakeGlobals->qmake_abslocation = QDir::cleanPath(qtVersion->qmakeCommand().toString());
            qtVersion->applyProperties(m_qmakeGlobals);
        }
        m_qmakeGlobals->setDirectories(rootProFile()->sourceDir().toString(),
                                       rootProFile()->buildDir().toString());

        Environment::const_iterator eit = env.constBegin(), eend = env.constEnd();
        for (; eit != eend; ++eit)
            m_qmakeGlobals->environment.insert(env.key(eit), env.value(eit));

        m_qmakeGlobals->setCommandLineArguments(rootProFile()->buildDir().toString(), qmakeArgs);

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

    reader->setOutputDir(qmakeProFile->buildDir().toString());

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

QList<QmakeProFile *>
QmakeProject::collectAllProFiles(QmakeProFile *file, Parsing parse,
                                 const QList<ProjectType> &projectTypes)
{
    QList<QmakeProFile *> result;
    if (parse == ExactAndCumulativeParse || file->includedInExactParse())
        if (projectTypes.isEmpty() || projectTypes.contains(file->projectType()))
            result.append(file);

    for (QmakePriFile *f : file->children()) {
        auto qmakeProFileNode = dynamic_cast<QmakeProFile *>(f);
        if (qmakeProFileNode)
            result.append(collectAllProFiles(qmakeProFileNode, parse, projectTypes));
    }

    return result;
}

QList<QmakeProFile *> QmakeProject::applicationProFiles(Parsing parse) const
{
    return allProFiles({ProjectType::ApplicationTemplate, ProjectType::ScriptTemplate}, parse);
}

QList<QmakeProFile *> QmakeProject::allProFiles(const QList<ProjectType> &projectTypes, Parsing parse) const
{
    QList<QmakeProFile *> list;
    if (!rootProFile())
        return list;
    list = collectAllProFiles(rootProFile(), parse, projectTypes);
    return list;
}

bool QmakeProject::hasApplicationProFile(const FileName &path) const
{
    const QList<QmakeProFile *> list = applicationProFiles();
    return Utils::contains(list, Utils::equal(&QmakeProFile::filePath, path));
}

QList<Core::Id> QmakeProject::creationIds(Core::Id base,
                                          IRunConfigurationFactory::CreationMode mode,
                                          const QList<ProjectType> &projectTypes)
{
    QList<ProjectType> realTypes = projectTypes;
    if (realTypes.isEmpty())
        realTypes = {ProjectType::ApplicationTemplate, ProjectType::ScriptTemplate};
    QList<QmakeProFile *> files = allProFiles(realTypes);
    QList<QmakeProFile *> temp = files;

    if (mode == IRunConfigurationFactory::AutoCreate) {
        QList<QmakeProFile *> filtered = Utils::filtered(files, [](const QmakeProFile *f) {
            return f->isQtcRunnable();
        });
        temp = filtered.isEmpty() ? files : filtered;
    }

    return Utils::transform(temp, [&base](QmakeProFile *f) {
        return base.withSuffix(f->filePath().toString());
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

static void notifyChangedHelper(const FileName &fileName, QmakeProFile *file)
{
    if (file->filePath() == fileName) {
        QtSupport::ProFileCacheManager::instance()->discardFile(fileName.toString());
        file->scheduleUpdate(QmakeProFile::ParseNow);
    }

    for (QmakePriFile *fn : file->children()) {
        if (auto pro = dynamic_cast<QmakeProFile *>(fn))
            notifyChangedHelper(fileName, pro);
    }
}

void QmakeProject::notifyChanged(const FileName &name)
{
    for (QmakeProject *project : s_projects) {
        if (project->files(QmakeProject::SourceFiles).contains(name.toString()))
            notifyChangedHelper(name, project->rootProFile());
    }
}

void QmakeProject::watchFolders(const QStringList &l, QmakePriFile *file)
{
    if (l.isEmpty())
        return;
    if (!m_centralizedFolderWatcher)
        m_centralizedFolderWatcher = new Internal::CentralizedFolderWatcher(this);
    m_centralizedFolderWatcher->watchFolders(l, file);
}

void QmakeProject::unwatchFolders(const QStringList &l, QmakePriFile *file)
{
    if (m_centralizedFolderWatcher && !l.isEmpty())
        m_centralizedFolderWatcher->unwatchFolders(l, file);
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

void CentralizedFolderWatcher::watchFolders(const QList<QString> &folders, QmakePriFile *file)
{
    m_watcher.addPaths(folders);

    const QChar slash = QLatin1Char('/');
    foreach (const QString &f, folders) {
        QString folder = f;
        if (!folder.endsWith(slash))
            folder.append(slash);
        m_map.insert(folder, file);

        // Support for recursive watching
        // we add the recursive directories we find
        QSet<QString> tmp = recursiveDirs(folder);
        if (!tmp.isEmpty())
            m_watcher.addPaths(tmp.toList());
        m_recursiveWatchedFolders += tmp;
    }
}

void CentralizedFolderWatcher::unwatchFolders(const QList<QString> &folders, QmakePriFile *file)
{
    const QChar slash = QLatin1Char('/');
    foreach (const QString &f, folders) {
        QString folder = f;
        if (!folder.endsWith(slash))
            folder.append(slash);
        m_map.remove(folder, file);
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
        QList<QmakePriFile *> files = m_map.values(dir);
        if (!files.isEmpty()) {
            // Collect all the files
            QSet<FileName> newFiles;
            newFiles += QmakePriFile::recursiveEnumerate(folder);
            foreach (QmakePriFile *file, files)
                newOrRemovedFiles = newOrRemovedFiles || file->folderChanged(folder, newFiles);
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

    if (newOrRemovedFiles)
        m_project->updateCodeModels();
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
    const QmakeProFile *const file = rootProFile();
    if (!file || file->parseInProgress())
        return;

    DeploymentData deploymentData;
    collectData(file, deploymentData);
    target->setDeploymentData(deploymentData);

    BuildTargetInfoList appTargetList;
    for (const QmakeProFile * const file : applicationProFiles()) {
        appTargetList.list << BuildTargetInfo(file->targetInformation().target,
                                              FileName::fromString(executableFor(file)),
                                              file->filePath());
    }
    target->setApplicationTargets(appTargetList);
}

void QmakeProject::collectData(const QmakeProFile *file, DeploymentData &deploymentData)
{
    if (!file->isSubProjectDeployable(file->filePath()))
        return;

    const InstallsList &installsList = file->installsList();
    for (const InstallsItem &item : installsList.items) {
        if (!item.active)
            continue;
        foreach (const auto &localFile, item.files)
            deploymentData.addFile(localFile.fileName, item.path);
    }

    switch (file->projectType()) {
    case ProjectType::ApplicationTemplate:
        if (!installsList.targetPath.isEmpty())
            collectApplicationData(file, deploymentData);
        break;
    case ProjectType::SharedLibraryTemplate:
    case ProjectType::StaticLibraryTemplate:
        collectLibraryData(file, deploymentData);
        break;
    case ProjectType::SubDirsTemplate:
        for (const QmakePriFile *const subPriFile : file->subPriFilesExact()) {
            auto subProFile = dynamic_cast<const QmakeProFile *>(subPriFile);
            if (subProFile)
                collectData(subProFile, deploymentData);
        }
        break;
    default:
        break;
    }
}

void QmakeProject::collectApplicationData(const QmakeProFile *file, DeploymentData &deploymentData)
{
    QString executable = executableFor(file);
    if (!executable.isEmpty())
        deploymentData.addFile(executable, file->installsList().targetPath,
                               DeployableFile::TypeExecutable);
}

static FileName destDirFor(const TargetInformation &ti)
{
    if (ti.destDir.isEmpty())
        return ti.buildDir;
    if (QDir::isRelativePath(ti.destDir.toString()))
        return FileName::fromString(QDir::cleanPath(ti.buildDir.toString() + '/' + ti.destDir.toString()));
    return ti.destDir;
}

void QmakeProject::collectLibraryData(const QmakeProFile *file, DeploymentData &deploymentData)
{
    const QString targetPath = file->installsList().targetPath;
    if (targetPath.isEmpty())
        return;
    const Kit * const kit = activeTarget()->kit();
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!toolchain)
        return;

    TargetInformation ti = file->targetInformation();
    QString targetFileName = ti.target;
    const QStringList config = file->variableValue(Variable::Config);
    const bool isStatic = config.contains(QLatin1String("static"));
    const bool isPlugin = config.contains(QLatin1String("plugin"));
    switch (toolchain->targetAbi().os()) {
    case Abi::WindowsOS: {
        QString targetVersionExt = file->singleVariableValue(Variable::TargetVersionExt);
        if (targetVersionExt.isEmpty()) {
            const QString version = file->singleVariableValue(Variable::Version);
            if (!version.isEmpty()) {
                targetVersionExt = version.left(version.indexOf(QLatin1Char('.')));
                if (targetVersionExt == QLatin1String("0"))
                    targetVersionExt.clear();
            }
        }
        targetFileName += targetVersionExt + QLatin1Char('.');
        targetFileName += QLatin1String(isStatic ? "lib" : "dll");
        deploymentData.addFile(destDirFor(ti).toString() + '/' + targetFileName, targetPath);
        break;
    }
    case Abi::DarwinOS: {
        FileName destDir = destDirFor(ti);
        if (config.contains(QLatin1String("lib_bundle"))) {
            destDir.appendPath(ti.target + ".framework");
        } else {
            if (!(isPlugin && config.contains(QLatin1String("no_plugin_name_prefix"))))
                targetFileName.prepend(QLatin1String("lib"));

            if (!isPlugin) {
                targetFileName += QLatin1Char('.');
                const QString version = file->singleVariableValue(Variable::Version);
                QString majorVersion = version.left(version.indexOf(QLatin1Char('.')));
                if (majorVersion.isEmpty())
                    majorVersion = QLatin1String("1");
                targetFileName += majorVersion;
            }
            targetFileName += QLatin1Char('.');
            targetFileName += file->singleVariableValue(isStatic
                    ? Variable::StaticLibExtension : Variable::ShLibExtension);
        }
        deploymentData.addFile(destDir.toString() + '/' + targetFileName, targetPath);
        break;
    }
    case Abi::LinuxOS:
    case Abi::BsdOS:
    case Abi::QnxOS:
    case Abi::UnixOS:
        if (!(isPlugin && config.contains(QLatin1String("no_plugin_name_prefix"))))
            targetFileName.prepend(QLatin1String("lib"));

        targetFileName += QLatin1Char('.');
        if (isStatic) {
            targetFileName += QLatin1Char('a');
        } else {
            targetFileName += QLatin1String("so");
            deploymentData.addFile(destDirFor(ti).toString() + '/' + targetFileName, targetPath);
            if (!isPlugin) {
                QString version = file->singleVariableValue(Variable::Version);
                if (version.isEmpty())
                    version = QLatin1String("1.0.0");
                QStringList versionComponents = version.split('.');
                while (versionComponents.size() < 3)
                    versionComponents << QLatin1String("0");
                targetFileName += QLatin1Char('.');
                while (!versionComponents.isEmpty()) {
                    const QString versionString = versionComponents.join(QLatin1Char('.'));
                    deploymentData.addFile(destDirFor(ti).toString() + '/'
                            + targetFileName + versionString, targetPath);
                    versionComponents.removeLast();
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

static Utils::FileName getFullPathOf(const QmakeProFile *pro, Variable variable,
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

    Environment env = Environment::systemEnvironment();
    if (Target *t = activeTarget()) {
        if (BuildConfiguration *bc = t->activeBuildConfiguration())
            env = bc->environment();
        else
            t->kit()->addToEnvironment(env);
    }

    if (!env.isSameExecutable(path.toString(), expected.toString())) {
        const QPair<Utils::FileName, Utils::FileName> pair = qMakePair(expected, path);
        if (!m_toolChainWarnings.contains(pair)) {
            // Suppress warnings on Apple machines where compilers in /usr/bin point into Xcode.
            // This will suppress some valid warnings, but avoids annoying Apple users with
            // spurious warnings all the time!
            if (!pair.first.toString().startsWith("/usr/bin/")
                    || !pair.second.toString().contains("/Contents/Developer/Toolchains/")) {
                TaskHub::addTask(Task(Task::Warning,
                                      QCoreApplication::translate("QmakeProjectManager", "\"%1\" is used by qmake, but \"%2\" is configured in the kit.\n"
                                                                                         "Please update your kit or choose a mkspec for qmake that matches your target environment better.").
                                      arg(path.toUserOutput()).arg(expected.toUserOutput()),
                                      Utils::FileName(), -1, ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
                m_toolChainWarnings.insert(pair);
            }
        }
    }
}

void QmakeProject::warnOnToolChainMismatch(const QmakeProFile *pro) const
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

QString QmakeProject::executableFor(const QmakeProFile *file)
{
    const Kit *const kit = activeTarget() ? activeTarget()->kit() : nullptr;
    const ToolChain *const tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc)
        return QString();

    TargetInformation ti = file->targetInformation();
    QString target;

    if (tc->targetAbi().os() == Abi::DarwinOS
            && file->variableValue(Variable::Config).contains("app_bundle")) {
        target = ti.target + ".app/Contents/MacOS/" + ti.target;
    } else {
        QString extension = file->singleVariableValue(Variable::TargetExt);
        target = ti.target + extension;
    }
    return QDir(destDirFor(ti).toString()).absoluteFilePath(target);
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
    const QmakeProFile *pro = rootProFile()->findProFile(proFilePath);
    return pro ? pro->targetInformation().target : QString();
}

} // namespace QmakeProjectManager

#include "qmakeproject.moc"
