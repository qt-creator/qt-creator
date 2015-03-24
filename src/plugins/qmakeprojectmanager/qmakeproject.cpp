/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "qmakeproject.h"

#include "qmakeprojectmanager.h"
#include "qmakeprojectimporter.h"
#include "qmakebuildinfo.h"
#include "qmakestep.h"
#include "qmakenodes.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakebuildconfiguration.h"
#include "findqmakeprofiles.h"
#include "wizards/abstractmobileapp.h"
#include "wizards/qtquickapp.h"

#include <utils/algorithm.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <proparser/qmakevfs.h>
#include <qtsupport/profilereader.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/uicodemodelsupport.h>
#include <resourceeditor/resourcenode.h>

#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMessageBox>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;
using namespace Utils;

enum { debug = 0 };

// -----------------------------------------------------------------------
// Helpers:
// -----------------------------------------------------------------------

namespace {

QmakeBuildConfiguration *enableActiveQmakeBuildConfiguration(Target *t, bool enabled)
{
    if (!t)
        return 0;
    QmakeBuildConfiguration *bc = static_cast<QmakeBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return 0;
    bc->setEnabled(enabled);
    return bc;
}

void updateBoilerPlateCodeFiles(const AbstractMobileApp *app, const QString &proFile)
{
    const QList<AbstractGeneratedFileInfo> updates = app->fileUpdates(proFile);
    const QString nativeProFile = QDir::toNativeSeparators(proFile);
    if (!updates.empty()) {
        const QString title = QmakeManager::tr("Update of Generated Files");
        QStringList fileNames;
        foreach (const AbstractGeneratedFileInfo &info, updates)
            fileNames.append(QDir::toNativeSeparators(info.fileInfo.fileName()));
        const QString message =
                QmakeManager::tr("In project<br><br>%1<br><br>The following files are either "
                               "outdated or have been modified:<br><br>%2<br><br>Do you want "
                               "Qt Creator to update the files? Any changes will be lost.")
                .arg(nativeProFile, fileNames.join(QLatin1String(", ")));
        if (QMessageBox::question(Core::ICore::dialogParent(), title, message, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            QString error;
            if (!app->updateFiles(updates, error))
                QMessageBox::critical(0, title, error);
        }
    }
}

} // namespace

namespace QmakeProjectManager {
namespace Internal {

class QmakeProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    QmakeProjectFile(const QString &filePath, QObject *parent = 0);

    bool save(QString *errorString, const QString &fileName, bool autoSave);

    QString defaultPath() const;
    QString suggestedFileName() const;

    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
};

/// Watches folders for QmakePriFile nodes
/// use one file system watcher to watch all folders
/// such minimizing system ressouce usage

class CentralizedFolderWatcher : public QObject
{
    Q_OBJECT
public:
    CentralizedFolderWatcher(QmakeProject *parent);
    ~CentralizedFolderWatcher();
    void watchFolders(const QList<QString> &folders, QmakePriFileNode *node);
    void unwatchFolders(const QList<QString> &folders, QmakePriFileNode *node);

private slots:
    void folderChanged(const QString &folder);
    void onTimer();
    void delayedFolderChanged(const QString &folder);

private:
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

    QStringList files[FileTypeSize];
    QStringList generatedFiles[FileTypeSize];
    QStringList proFiles;
};

void QmakeProjectFiles::clear()
{
    for (int i = 0; i < FileTypeSize; ++i) {
        files[i].clear();
        generatedFiles[i].clear();
    }
    proFiles.clear();
}

bool QmakeProjectFiles::equals(const QmakeProjectFiles &f) const
{
    for (int i = 0; i < FileTypeSize; ++i)
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
    for (int i = 0; i < FileTypeSize; ++i)
        nsp << "Type " << i << " files=" << f.files[i] <<  " generated=" << f.generatedFiles[i] << '\n';
    return d;
}

// A visitor to collect all files of a project in a QmakeProjectFiles struct
class ProjectFilesVisitor : public NodesVisitor
{
    ProjectFilesVisitor(QmakeProjectFiles *files);

public:

    static void findProjectFiles(QmakeProFileNode *rootNode, QmakeProjectFiles *files);

    void visitProjectNode(ProjectNode *projectNode);
    void visitFolderNode(FolderNode *folderNode);

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
    for (int i = 0; i < FileTypeSize; ++i) {
        Utils::sort(files->files[i]);
        unique(files->files[i]);
        Utils::sort(files->generatedFiles[i]);
        unique(files->generatedFiles[i]);
    }
    Utils::sort(files->proFiles);
    unique(files->proFiles);
}

void ProjectFilesVisitor::visitProjectNode(ProjectNode *projectNode)
{
    m_files->proFiles.append(projectNode->path().toString());
    visitFolderNode(projectNode);
}

void ProjectFilesVisitor::visitFolderNode(FolderNode *folderNode)
{
    if (dynamic_cast<ResourceEditor::ResourceTopLevelNode *>(folderNode))
        m_files->files[ResourceType].push_back(folderNode->path().toString());

    foreach (FileNode *fileNode, folderNode->fileNodes()) {
        const int type = fileNode->fileType();
        QStringList &targetList = fileNode->isGenerated() ? m_files->generatedFiles[type] : m_files->files[type];
        targetList.push_back(fileNode->path().toString());
    }
}

}

// ----------- QmakeProjectFile
namespace Internal {
QmakeProjectFile::QmakeProjectFile(const QString &filePath, QObject *parent)
    : Core::IDocument(parent)
{
    setId("Qmake.ProFile");
    setMimeType(QLatin1String(QmakeProjectManager::Constants::PROFILE_MIMETYPE));
    setFilePath(FileName::fromString(filePath));
}

bool QmakeProjectFile::save(QString *, const QString &, bool)
{
    // This is never used
    return false;
}

QString QmakeProjectFile::defaultPath() const
{
    return QString();
}

QString QmakeProjectFile::suggestedFileName() const
{
    return QString();
}

bool QmakeProjectFile::isModified() const
{
    return false; // we save after changing anyway
}

bool QmakeProjectFile::isSaveAsAllowed() const
{
    return false;
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
    m_manager(manager),
    m_rootProjectNode(0),
    m_fileInfo(new QmakeProjectFile(fileName, this)),
    m_projectFiles(new QmakeProjectFiles),
    m_qmakeVfs(new QMakeVfs),
    m_qmakeGlobals(0),
    m_qmakeGlobalsRefCnt(0),
    m_asyncUpdateFutureInterface(0),
    m_pendingEvaluateFuturesCount(0),
    m_asyncUpdateState(Base),
    m_cancelEvaluate(false),
    m_centralizedFolderWatcher(0),
    m_activeTarget(0),
    m_checkForTemplateUpdate(true)
{
    setId(Constants::QMAKEPROJECT_ID);
    setProjectContext(Core::Context(QmakeProjectManager::Constants::PROJECT_ID));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));
    setRequiredKitMatcher(QtSupport::QtKitInformation::qtVersionMatcher());

    m_asyncUpdateTimer.setSingleShot(true);
    m_asyncUpdateTimer.setInterval(3000);
    connect(&m_asyncUpdateTimer, SIGNAL(timeout()), this, SLOT(asyncUpdate()));

    connect(BuildManager::instance(), SIGNAL(buildQueueFinished(bool)),
            SLOT(buildFinished(bool)));

    setPreferredKitMatcher(KitMatcher([this](const Kit *kit) -> bool {
                               return matchesKit(kit);
                           }));
}

QmakeProject::~QmakeProject()
{
    m_codeModelFuture.cancel();
    m_asyncUpdateState = ShuttingDown;
    m_manager->unregisterProject(this);
    delete m_projectFiles;
    m_cancelEvaluate = true;
    // Deleting the root node triggers a few things, make sure rootProjectNode
    // returns 0 already
    QmakeProFileNode *root = m_rootProjectNode;
    m_rootProjectNode = 0;
    delete root;
    Q_ASSERT(m_qmakeGlobalsRefCnt == 0);
    delete m_qmakeVfs;
}

void QmakeProject::updateFileList()
{
    QmakeProjectFiles newFiles;
    ProjectFilesVisitor::findProjectFiles(m_rootProjectNode, &newFiles);
    if (newFiles != *m_projectFiles) {
        *m_projectFiles = newFiles;
        emit fileListChanged();
        if (debug)
            qDebug() << Q_FUNC_INFO << *m_projectFiles;
    }
}

bool QmakeProject::fromMap(const QVariantMap &map)
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

    m_rootProjectNode = new QmakeProFileNode(this, m_fileInfo->filePath());

    // On active buildconfiguration changes, reevaluate the .pro files
    m_activeTarget = activeTarget();
    if (m_activeTarget) {
        connect(m_activeTarget, &Target::activeBuildConfigurationChanged,
                this, &QmakeProject::scheduleAsyncUpdateLater);
    }

    connect(this, &Project::activeTargetChanged,
            this, &QmakeProject::activeTargetWasChanged);

    scheduleAsyncUpdate(QmakeProFileNode::ParseNow);
    return true;
}

/// equalFileList compares two file lists ignoring
/// <configuration> without generating temporary lists

bool QmakeProject::equalFileList(const QStringList &a, const QStringList &b)
{
    if (abs(a.length() - b.length()) > 1)
        return false;
    QStringList::const_iterator ait = a.constBegin();
    QStringList::const_iterator bit = b.constBegin();
    QStringList::const_iterator aend = a.constEnd();
    QStringList::const_iterator bend = b.constEnd();

    while (ait != aend && bit != bend) {
        if (*ait == CppTools::CppModelManager::configurationFileName())
            ++ait;
        else if (*bit == CppTools::CppModelManager::configurationFileName())
            ++bit;
        else if (*ait == *bit)
            ++ait, ++bit;
        else
           return false;
    }
    return (ait == aend && bit == bend);
}

void QmakeProject::updateCodeModels()
{
    if (debug)
        qDebug() << "QmakeProject::updateCodeModel()";

    if (activeTarget() && !activeTarget()->activeBuildConfiguration())
        return;

    updateCppCodeModel();
    updateQmlJSCodeModel();
}

void QmakeProject::updateCppCodeModel()
{
    typedef CppTools::ProjectPart ProjectPart;
    typedef CppTools::ProjectFile ProjectFile;

    Kit *k = 0;
    if (Target *target = activeTarget())
        k = target->kit();
    else
        k = KitManager::defaultKit();

    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    FindQmakeProFiles findQmakeProFiles;
    QList<QmakeProFileNode *> proFiles = findQmakeProFiles(rootProjectNode());

    CppTools::ProjectInfo pinfo(this);

    QtSupport::BaseQtVersion *qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    ProjectPart::QtVersion qtVersionForPart = ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = ProjectPart::Qt4;
        else
            qtVersionForPart = ProjectPart::Qt5;
    }

    QHash<QString, QString> uiCodeModelData;
    QStringList allFiles;
    foreach (QmakeProFileNode *pro, proFiles) {
        ProjectPart::Ptr templatePart(new ProjectPart);
        templatePart->project = this;
        templatePart->displayName = pro->displayName();
        templatePart->projectFile = pro->path().toString();
        templatePart->selectedForBuilding = pro->includedInExactParse();

        if (pro->variableValue(ConfigVar).contains(QLatin1String("qt")))
            templatePart->qtVersion = qtVersionForPart;
        else
            templatePart->qtVersion = ProjectPart::NoQt;

        // part->defines
        templatePart->projectDefines += pro->cxxDefines();

        // part->headerPaths
        if (qtVersion) {
            foreach (const HeaderPath &header, qtVersion->systemHeaderPathes(k)) {
                ProjectPart::HeaderPath::Type type = ProjectPart::HeaderPath::IncludePath;
                if (header.kind() == HeaderPath::FrameworkHeaderPath)
                    type = ProjectPart::HeaderPath::FrameworkPath;
                templatePart->headerPaths += ProjectPart::HeaderPath(header.path(), type);
            }
        }

        foreach (const QString &inc, pro->variableValue(IncludePathVar)) {
            const auto headerPath
                = ProjectPart::HeaderPath(inc, ProjectPart::HeaderPath::IncludePath);
            // We've added header paths from qtVersion->systemHeaderPathes() above,
            // which also contains the mkspecs dir. However, it's also part of
            // the pro->variableValue(IncludePathVar), so check for duplicates.
            if (!templatePart->headerPaths.contains(headerPath))
                templatePart->headerPaths += headerPath;
        }

        if (qtVersion) {
            if (!qtVersion->frameworkInstallPath().isEmpty()) {
                templatePart->headerPaths += ProjectPart::HeaderPath(
                            qtVersion->frameworkInstallPath(),
                            ProjectPart::HeaderPath::FrameworkPath);
            }
        }

        // part->precompiledHeaders
        templatePart->precompiledHeaders.append(pro->variableValue(PrecompiledHeaderVar));

        templatePart->updateLanguageFeatures();

        ProjectPart::Ptr cppPart = templatePart->copy();
        { // C++ files:
            // part->files
            foreach (const QString &file, pro->variableValue(CppSourceVar)) {
                allFiles << file;
                cppPart->files << ProjectFile(file, ProjectFile::CXXSource);
            }
            foreach (const QString &file, pro->variableValue(CppHeaderVar)) {
                allFiles << file;
                cppPart->files << ProjectFile(file, ProjectFile::CXXHeader);
            }

            // Ui Files:
            QHash<QString, QString> uiData = pro->uiFiles();
            for (QHash<QString, QString>::const_iterator i = uiData.constBegin(); i != uiData.constEnd(); ++i) {
                allFiles << i.value();
                cppPart->files << ProjectFile(i.value(), ProjectFile::CXXHeader);
            }
            uiCodeModelData.unite(uiData);

            cppPart->files.prepend(ProjectFile(CppTools::CppModelManager::configurationFileName(),
                                            ProjectFile::CXXSource));
            const QStringList cxxflags = pro->variableValue(CppFlagsVar);
            cppPart->evaluateToolchain(ToolChainKitInformation::toolChain(k),
                                       cxxflags,
                                       SysRootKitInformation::sysRoot(k));

            if (!cppPart->files.isEmpty()) {
                pinfo.appendProjectPart(cppPart);
                setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, true);
            }
        }

        ProjectPart::Ptr objcppPart = templatePart->copy();
        { // ObjC++ files:
            foreach (const QString &file, pro->variableValue(ObjCSourceVar)) {
                allFiles << file;
                // Although the enum constant is called ObjCSourceVar, it actually is ObjC++ source
                // code, as qmake does not handle C (and ObjC).
                objcppPart->files << ProjectFile(file, ProjectFile::ObjCXXSource);
            }
            foreach (const QString &file, pro->variableValue(ObjCHeaderVar)) {
                allFiles << file;
                objcppPart->files << ProjectFile(file, ProjectFile::ObjCXXHeader);
            }

            const QStringList cxxflags = pro->variableValue(CppFlagsVar);
            objcppPart->evaluateToolchain(ToolChainKitInformation::toolChain(k),
                                          cxxflags,
                                          SysRootKitInformation::sysRoot(k));

            if (!objcppPart->files.isEmpty()) {
                pinfo.appendProjectPart(objcppPart);
                // TODO: there is no LANG_OBJCXX, so:
                setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, true);
            }
        }

        if (!objcppPart->files.isEmpty())
            cppPart->displayName += QLatin1String(" (C++)");
        if (!cppPart->files.isEmpty())
            objcppPart->displayName += QLatin1String(" (ObjC++)");
    }
    pinfo.finish();

    // Also update Ui Code Model Support:
    QtSupport::UiCodeModelManager::update(this, uiCodeModelData);

    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);
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
        foreach (const QString &path, node->variableValue(QmlImportPathVar))
            projectInfo.importPaths.maybeInsert(FileName::fromString(path),
                                                QmlJS::Dialect::Qml);
        projectInfo.activeResourceFiles.append(node->variableValue(ExactResourceVar));
        projectInfo.allResourceFiles.append(node->variableValue(ResourceVar));
        if (!hasQmlLib) {
            QStringList qtLibs = node->variableValue(QtVar);
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
        addProjectLanguage(ProjectExplorer::Constants::LANG_QMLJS);

    projectInfo.activeResourceFiles.removeDuplicates();
    projectInfo.allResourceFiles.removeDuplicates();

    modelManager->updateProjectInfo(projectInfo, this);
}

void QmakeProject::updateRunConfigurations()
{
    if (activeTarget())
        activeTarget()->updateDefaultRunConfigurations();
}

void QmakeProject::scheduleAsyncUpdate(QmakeProFileNode *node, QmakeProFileNode::AsyncUpdateDelay delay)
{
    if (m_asyncUpdateState == ShuttingDown)
        return;

    if (debug)
        qDebug()<<"schduleAsyncUpdate (node)"<<node->path();

    if (m_cancelEvaluate) {
        if (debug)
            qDebug()<<"  Already canceling, nothing to do";
        // A cancel is in progress
        // That implies that a full update is going to happen afterwards
        // So we don't need to do anything
        return;
    }

    enableActiveQmakeBuildConfiguration(activeTarget(), false);

    if (m_asyncUpdateState == AsyncFullUpdatePending) {
        // Just postpone
        if (debug)
            qDebug()<<"  full update pending, restarting timer";
        startAsyncTimer(delay);
    } else if (m_asyncUpdateState == AsyncPartialUpdatePending
               || m_asyncUpdateState == Base) {
        if (debug)
            qDebug()<<"  adding node to async update list, setting state to AsyncPartialUpdatePending";
        // Add the node
        m_asyncUpdateState = AsyncPartialUpdatePending;

        QList<QmakeProFileNode *>::iterator it;
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
        if (debug)
            qDebug()<<"  Async update in progress, scheduling new one afterwards";
        scheduleAsyncUpdate(delay);
    }
}

void QmakeProject::scheduleAsyncUpdate(QmakeProFileNode::AsyncUpdateDelay delay)
{
    if (debug)
        qDebug()<<"scheduleAsyncUpdate";
    if (m_asyncUpdateState == ShuttingDown)
        return;

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
        enableActiveQmakeBuildConfiguration(activeTarget(), false);
        m_rootProjectNode->setParseInProgressRecursive(true);
        return;
    }

    if (debug)
        qDebug()<<"  starting timer for full update, setting state to full update pending";
    m_partialEvaluate.clear();
    enableActiveQmakeBuildConfiguration(activeTarget(), false);
    m_rootProjectNode->setParseInProgressRecursive(true);
    m_asyncUpdateState = AsyncFullUpdatePending;

    // Cancel running code model update
    m_codeModelFuture.cancel();
    startAsyncTimer(delay);
}

void QmakeProject::startAsyncTimer(QmakeProFileNode::AsyncUpdateDelay delay)
{
    m_asyncUpdateTimer.stop();
    m_asyncUpdateTimer.setInterval(qMin(m_asyncUpdateTimer.interval(), delay == QmakeProFileNode::ParseLater ? 3000 : 0));
    m_asyncUpdateTimer.start();
}

void QmakeProject::incrementPendingEvaluateFutures()
{
    ++m_pendingEvaluateFuturesCount;
    if (debug)
        qDebug()<<"incrementPendingEvaluateFutures to"<<m_pendingEvaluateFuturesCount;

    m_asyncUpdateFutureInterface->setProgressRange(m_asyncUpdateFutureInterface->progressMinimum(),
                                                  m_asyncUpdateFutureInterface->progressMaximum() + 1);
}

void QmakeProject::decrementPendingEvaluateFutures()
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
            startAsyncTimer(QmakeProFileNode::ParseLater);
        } else  if (m_asyncUpdateState != ShuttingDown){
            // After being done, we need to call:
            m_asyncUpdateState = Base;
            enableActiveQmakeBuildConfiguration(activeTarget(), true);
            updateFileList();
            updateCodeModels();
            updateBuildSystemData();
            if (activeTarget())
                activeTarget()->updateDefaultDeployConfigurations();
            updateRunConfigurations();
            emit proFilesEvaluated();
            if (debug)
                qDebug()<<"  Setting state to Base";
        }

        if (m_checkForTemplateUpdate) {
            // Update boiler plate code for subprojects.
            QtQuickApp qtQuickApp;

            foreach (QmakeProFileNode *node, applicationProFiles(QmakeProject::ExactAndCumulativeParse)) {
                const QString path = node->path().toString();

                foreach (TemplateInfo info, QtQuickApp::templateInfos()) {
                    qtQuickApp.setTemplateInfo(info);
                    updateBoilerPlateCodeFiles(&qtQuickApp, path);
                }
            }
            m_checkForTemplateUpdate = false;
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
    if (debug)
        qDebug()<<"async update, timer expired, doing now";

    m_qmakeVfs->invalidateCache();

    Q_ASSERT(!m_asyncUpdateFutureInterface);
    m_asyncUpdateFutureInterface = new QFutureInterface<void>();

    m_asyncUpdateFutureInterface->setProgressRange(0, 0);
    Core::ProgressManager::addTask(m_asyncUpdateFutureInterface->future(),
                                   tr("Reading Project \"%1\"").arg(displayName()),
                                   Constants::PROFILE_EVALUATE);
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
        foreach (QmakeProFileNode *node, m_partialEvaluate)
            node->asyncUpdate();
    }

    m_partialEvaluate.clear();
    if (debug)
        qDebug()<<"  Setting state to AsyncUpdateInProgress";
    m_asyncUpdateState = AsyncUpdateInProgress;
}

void QmakeProject::buildFinished(bool success)
{
    if (success)
        m_qmakeVfs->invalidateContents();
}

IProjectManager *QmakeProject::projectManager() const
{
    return m_manager;
}

QmakeManager *QmakeProject::qmakeProjectManager() const
{
    return m_manager;
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

Core::IDocument *QmakeProject::document() const
{
    return m_fileInfo;
}

QStringList QmakeProject::files(FilesMode fileMode) const
{
    QStringList files;
    for (int i = 0; i < FileTypeSize; ++i) {
        files += m_projectFiles->files[i];
        if (fileMode == AllFiles)
            files += m_projectFiles->generatedFiles[i];
    }

    files.removeDuplicates();

    return files;
}

// Find the folder that contains a file a certain type (recurse down)
static FolderNode *folderOf(FolderNode *in, FileType fileType, const FileName &fileName)
{
    foreach (FileNode *fn, in->fileNodes())
        if (fn->fileType() == fileType && fn->path() == fileName)
            return in;
    foreach (FolderNode *folder, in->subFolderNodes())
        if (FolderNode *pn = folderOf(folder, fileType, fileName))
            return pn;
    return 0;
}

// Find the QmakeProFileNode that contains a file of a certain type.
// First recurse down to folder, then find the pro-file.
static QmakeProFileNode *proFileNodeOf(QmakeProFileNode *in, FileType fileType, const FileName &fileName)
{
    for (FolderNode *folder =  folderOf(in, fileType, fileName); folder; folder = folder->parentFolderNode())
        if (QmakeProFileNode *proFile = dynamic_cast<QmakeProFileNode *>(folder))
            return proFile;
    return 0;
}

QString QmakeProject::generatedUiHeader(const FileName &formFile) const
{
    // Look in sub-profiles as SessionManager::projectForFile returns
    // the top-level project only.
    if (m_rootProjectNode)
        if (const QmakeProFileNode *pro = proFileNodeOf(m_rootProjectNode, FormType, formFile))
            return QmakeProFileNode::uiHeaderFile(pro->uiDirectory(pro->buildDir()), formFile);
    return QString();
}

void QmakeProject::proFileParseError(const QString &errorMessage)
{
    Core::MessageManager::write(errorMessage);
}

QtSupport::ProFileReader *QmakeProject::createProFileReader(const QmakeProFileNode *qmakeProFileNode, QmakeBuildConfiguration *bc)
{
    if (!m_qmakeGlobals) {
        m_qmakeGlobals = new ProFileGlobals;
        m_qmakeGlobalsRefCnt = 0;

        Kit *k;
        Environment env = Environment::systemEnvironment();
        QStringList qmakeArgs;
        if (!bc)
            bc = activeTarget() ? static_cast<QmakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration()) : 0;

        if (bc) {
            k = bc->target()->kit();
            env = bc->environment();
            if (bc->qmakeStep())
                qmakeArgs = bc->qmakeStep()->parserArguments();
            else
                qmakeArgs = bc->configCommandLineArguments();
        } else {
            k = KitManager::defaultKit();
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

        Environment::const_iterator eit = env.constBegin(), eend = env.constEnd();
        for (; eit != eend; ++eit)
            m_qmakeGlobals->environment.insert(env.key(eit), env.value(eit));

        m_qmakeGlobals->setCommandLineArguments(m_rootProjectNode->buildDir(), qmakeArgs);

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

    QtSupport::ProFileReader *reader = new QtSupport::ProFileReader(m_qmakeGlobals, m_qmakeVfs);

    reader->setOutputDir(qmakeProFileNode->buildDir());

    return reader;
}

ProFileGlobals *QmakeProject::qmakeGlobals()
{
    return m_qmakeGlobals;
}

QMakeVfs *QmakeProject::qmakeVfs()
{
    return m_qmakeVfs;
}

void QmakeProject::destroyProFileReader(QtSupport::ProFileReader *reader)
{
    delete reader;
    if (!--m_qmakeGlobalsRefCnt) {
        QString dir = m_fileInfo->filePath().toFileInfo().absolutePath();
        if (!dir.endsWith(QLatin1Char('/')))
            dir += QLatin1Char('/');
        QtSupport::ProFileCacheManager::instance()->discardFiles(dir);
        QtSupport::ProFileCacheManager::instance()->decRefCount();

        delete m_qmakeGlobals;
        m_qmakeGlobals = 0;
    }
}

ProjectNode *QmakeProject::rootProjectNode() const
{
    return m_rootProjectNode;
}

QmakeProFileNode *QmakeProject::rootQmakeProjectNode() const
{
    return m_rootProjectNode;
}

bool QmakeProject::validParse(const FileName &proFilePath) const
{
    if (!m_rootProjectNode)
        return false;
    const QmakeProFileNode *node = m_rootProjectNode->findProFileFor(proFilePath);
    return node && node->validParse();
}

bool QmakeProject::parseInProgress(const FileName &proFilePath) const
{
    if (!m_rootProjectNode)
        return false;
    const QmakeProFileNode *node = m_rootProjectNode->findProFileFor(proFilePath);
    return node && node->parseInProgress();
}

void QmakeProject::collectAllProFiles(QList<QmakeProFileNode *> &list, QmakeProFileNode *node, Parsing parse,
                                      const QList<QmakeProjectType> &projectTypes)
{
    if (parse == ExactAndCumulativeParse || node->includedInExactParse())
        if (projectTypes.isEmpty() || projectTypes.contains(node->projectType()))
            list.append(node);
    foreach (ProjectNode *n, node->subProjectNodes()) {
        QmakeProFileNode *qmakeProFileNode = dynamic_cast<QmakeProFileNode *>(n);
        if (qmakeProFileNode)
            collectAllProFiles(list, qmakeProFileNode, parse, projectTypes);
    }
}

QList<QmakeProFileNode *> QmakeProject::applicationProFiles(Parsing parse) const
{
    return allProFiles(QList<QmakeProjectType>() << ApplicationTemplate << ScriptTemplate, parse);
}

QList<QmakeProFileNode *> QmakeProject::allProFiles(const QList<QmakeProjectType> &projectTypes, Parsing parse) const
{
    QList<QmakeProFileNode *> list;
    if (!rootProjectNode())
        return list;
    collectAllProFiles(list, rootQmakeProjectNode(), parse, projectTypes);
    return list;
}

bool QmakeProject::hasApplicationProFile(const FileName &path) const
{
    if (path.isEmpty())
        return false;

    QList<QmakeProFileNode *> list = applicationProFiles();
    foreach (QmakeProFileNode * node, list)
        if (node->path() == path)
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
        return base.withSuffix(node->path().toString());
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

bool QmakeProject::hasSubNode(QmakePriFileNode *root, const FileName &path)
{
    if (root->path() == path)
        return true;
    foreach (FolderNode *fn, root->subFolderNodes()) {
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

    foreach (FolderNode *fn, root->subFolderNodes())
        if (QmakeProFileNode *qt4proFileNode = dynamic_cast<QmakeProFileNode *>(fn))
            findProFile(fileName, qt4proFileNode, list);
}

void QmakeProject::notifyChanged(const FileName &name)
{
    if (files(QmakeProject::ExcludeGeneratedFiles).contains(name.toString())) {
        QList<QmakeProFileNode *> list;
        findProFile(name, rootQmakeProjectNode(), list);
        foreach (QmakeProFileNode *node, list) {
            QtSupport::ProFileCacheManager::instance()->discardFile(name.toString());
            node->scheduleUpdate(QmakeProFileNode::ParseNow);
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

namespace {
    bool debugCFW = false;
}

CentralizedFolderWatcher::CentralizedFolderWatcher(QmakeProject *parent)
    : QObject(parent), m_project(parent)
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

void CentralizedFolderWatcher::watchFolders(const QList<QString> &folders, QmakePriFileNode *node)
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

void CentralizedFolderWatcher::unwatchFolders(const QList<QString> &folders, QmakePriFileNode *node)
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
                QMultiMap<QString, QmakePriFileNode *>::const_iterator it, end;
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
        if (debugCFW)
            qDebug()<<"found new recursive dirs"<<tmp;

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

void QmakeProject::configureAsExampleProject(const QStringList &platforms)
{
    QList<const BuildInfo *> infoList;
    QList<Kit *> kits = KitManager::kits();
    foreach (Kit *k, kits) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(k);
        if (!version)
            continue;
        if (!platforms.isEmpty() && !platforms.contains(version->platformName()))
            continue;

        IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(k, projectFilePath().toString());
        if (!factory)
            continue;
        foreach (BuildInfo *info, factory->availableSetups(k, projectFilePath().toString()))
            infoList << info;
    }
    setup(infoList);
    qDeleteAll(infoList);
    ProjectExplorerPlugin::requestProjectModeUpdate(this);
}

bool QmakeProject::requiresTargetPanel() const
{
    return false;
}

// All the Qmake run configurations should share code.
// This is a rather suboptimal way to do that for disabledReason()
// but more pratical then duplicated the code everywhere
QString QmakeProject::disabledReasonForRunConfiguration(const FileName &proFilePath)
{
    if (!proFilePath.exists())
        return tr("The .pro file \"%1\" does not exist.")
                .arg(proFilePath.fileName());

    if (!m_rootProjectNode) // Shutting down
        return QString();

    if (!m_rootProjectNode->findProFileFor(proFilePath))
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
    Target * const target = activeTarget();
    if (!target)
        return;
    const QmakeProFileNode * const rootNode = rootQmakeProjectNode();
    if (!rootNode || rootNode->parseInProgress())
        return;

    DeploymentData deploymentData;
    collectData(rootNode, deploymentData);
    target->setDeploymentData(deploymentData);

    BuildTargetInfoList appTargetList;
    foreach (const QmakeProFileNode * const node, applicationProFiles()) {
        appTargetList.list << BuildTargetInfo(node->targetInformation().target,
                                              FileName::fromString(executableFor(node)),
                                              node->path());
    }
    target->setApplicationTargets(appTargetList);
}

void QmakeProject::collectData(const QmakeProFileNode *node, DeploymentData &deploymentData)
{
    if (!node->isSubProjectDeployable(node->path().toString()))
        return;

    const InstallsList &installsList = node->installsList();
    foreach (const InstallsItem &item, installsList.items) {
        foreach (const QString &localFile, item.files)
            deploymentData.addFile(localFile, item.path);
    }

    switch (node->projectType()) {
    case ApplicationTemplate:
        if (!installsList.targetPath.isEmpty())
            collectApplicationData(node, deploymentData);
        break;
    case SharedLibraryTemplate:
    case StaticLibraryTemplate:
        collectLibraryData(node, deploymentData);
        break;
    case SubDirsTemplate:
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
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit);
    if (!toolchain)
        return;

    TargetInformation ti = node->targetInformation();
    QString targetFileName = ti.target;
    const QStringList config = node->variableValue(ConfigVar);
    const bool isStatic = config.contains(QLatin1String("static"));
    const bool isPlugin = config.contains(QLatin1String("plugin"));
    switch (toolchain->targetAbi().os()) {
    case Abi::WindowsOS: {
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
        deploymentData.addFile(destDirFor(ti) + QLatin1Char('/') + targetFileName, targetPath);
        break;
    }
    case Abi::MacOS: {
        QString destDir = destDirFor(ti);
        if (config.contains(QLatin1String("lib_bundle"))) {
            destDir.append(QLatin1Char('/')).append(ti.target)
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
        deploymentData.addFile(destDir + QLatin1Char('/') + targetFileName, targetPath);
        break;
    }
    case Abi::LinuxOS:
    case Abi::BsdOS:
    case Abi::UnixOS:
        targetFileName.prepend(QLatin1String("lib"));
        targetFileName += QLatin1Char('.');
        if (isStatic) {
            targetFileName += QLatin1Char('a');
        } else {
            targetFileName += QLatin1String("so");
            deploymentData.addFile(destDirFor(ti) + QLatin1Char('/') + targetFileName, targetPath);
            if (!isPlugin) {
                QString version = node->singleVariableValue(VersionVar);
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
    QList<QtSupport::BaseQtVersion *> parentQts;
    FileName filePath = projectFilePath();
    foreach (QtSupport::BaseQtVersion *version, QtSupport::QtVersionManager::validVersions()) {
        if (version->isSubProject(filePath))
            parentQts.append(version);
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(kit);
    if (!parentQts.isEmpty())
        return parentQts.contains(version);
    return false;
}

QString QmakeProject::executableFor(const QmakeProFileNode *node)
{
    const Kit * const kit = activeTarget()->kit();
    const ToolChain * const toolchain = ToolChainKitInformation::toolChain(kit);
    if (!toolchain)
        return QString();

    TargetInformation ti = node->targetInformation();
    QString target;

    switch (toolchain->targetAbi().os()) {
    case Abi::MacOS:
        if (node->variableValue(ConfigVar).contains(QLatin1String("app_bundle"))) {
            target = ti.target + QLatin1String(".app/Contents/MacOS/") + ti.target;
            break;
        }
        // else fall through
    default: {
        QString extension = node->singleVariableValue(TargetExtVar);
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

ProjectImporter *QmakeProject::createProjectImporter() const
{
    return new QmakeProjectImporter(projectFilePath().toString());
}

QmakeProject::AsyncUpdateState QmakeProject::asyncUpdateState() const
{
    return m_asyncUpdateState;
}

} // namespace QmakeProjectManager

#include "qmakeproject.moc"
