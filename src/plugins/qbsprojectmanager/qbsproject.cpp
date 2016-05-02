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

#include "qbsproject.h"

#include "qbsbuildconfiguration.h"
#include "qbslogsink.h"
#include "qbspmlogging.h"
#include "qbsprojectfile.h"
#include "qbsprojectmanager.h"
#include "qbsprojectparser.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsnodes.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpartbuilder.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <qtsupport/qtkitinformation.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QVariantMap>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CPP_MODULE[] = "cpp";
static const char CONFIG_CXXFLAGS[] = "cxxFlags";
static const char CONFIG_CFLAGS[] = "cFlags";
static const char CONFIG_DEFINES[] = "defines";
static const char CONFIG_INCLUDEPATHS[] = "includePaths";
static const char CONFIG_SYSTEM_INCLUDEPATHS[] = "systemIncludePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "frameworkPaths";
static const char CONFIG_SYSTEM_FRAMEWORKPATHS[] = "systemFrameworkPaths";
static const char CONFIG_PRECOMPILEDHEADER[] = "precompiledHeader";

// --------------------------------------------------------------------
// QbsProject:
// --------------------------------------------------------------------

QbsProject::QbsProject(QbsManager *manager, const QString &fileName) :
    m_projectName(QFileInfo(fileName).completeBaseName()),
    m_qbsProjectParser(0),
    m_qbsUpdateFutureInterface(0),
    m_parsingScheduled(false),
    m_cancelStatus(CancelStatusNone),
    m_currentBc(0),
    m_extraCompilersPending(false)
{
    m_parsingDelay.setInterval(1000); // delay parsing by 1s.

    setId(Constants::PROJECT_ID);
    setProjectManager(manager);
    setDocument(new QbsProjectFile(this, fileName));
    DocumentManager::addDocument(document());
    setRootProjectNode(new QbsRootProjectNode(this));

    setProjectContext(Context(Constants::PROJECT_ID));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(changeActiveTarget(ProjectExplorer::Target*)));
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetWasAdded(ProjectExplorer::Target*)));
    connect(this, SIGNAL(environmentChanged()), this, SLOT(delayParsing()));

    connect(&m_parsingDelay, SIGNAL(timeout()), this, SLOT(startParsing()));
}

QbsProject::~QbsProject()
{
    m_codeModelFuture.cancel();
    delete m_qbsProjectParser;
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
        delete m_qbsUpdateFutureInterface;
        m_qbsUpdateFutureInterface = 0;
    }
    qDeleteAll(m_extraCompilers);
}

QString QbsProject::displayName() const
{
    return m_projectName;
}

QbsManager *QbsProject::projectManager() const
{
    return static_cast<QbsManager *>(Project::projectManager());
}

QbsRootProjectNode *QbsProject::rootProjectNode() const
{
    return static_cast<QbsRootProjectNode *>(Project::rootProjectNode());
}

void QbsProject::projectLoaded()
{
    m_parsingDelay.start(0);
}

static void collectFilesForProject(const qbs::ProjectData &project, QSet<QString> &result)
{
    result.insert(project.location().filePath());
    foreach (const qbs::ProductData &prd, project.products()) {
        foreach (const qbs::GroupData &grp, prd.groups()) {
            foreach (const QString &file, grp.allFilePaths())
                result.insert(file);
            result.insert(grp.location().filePath());
        }
        result.insert(prd.location().filePath());
    }
    foreach (const qbs::ProjectData &subProject, project.subProjects())
        collectFilesForProject(subProject, result);
}

QStringList QbsProject::files(Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    qCDebug(qbsPmLog) << Q_FUNC_INFO << m_qbsProject.isValid() << isParsing();
    if (!m_qbsProject.isValid() || isParsing())
        return QStringList();
    QSet<QString> result;
    collectFilesForProject(m_projectData, result);
    result.unite(m_qbsProject.buildSystemFiles());
    qCDebug(qbsPmLog) << "file count:" << result.count();
    return result.toList();
}

QStringList QbsProject::filesGeneratedFrom(const QString &sourceFile) const
{
    QStringList generated;
    foreach (const qbs::ProductData &data, m_projectData.allProducts())
         generated << m_qbsProject.generatedFiles(data, sourceFile, false);
    return generated;
}

bool QbsProject::isProjectEditable() const
{
    return m_qbsProject.isValid() && !isParsing() && !BuildManager::isBuilding();
}

class ChangeExpector
{
public:
    ChangeExpector(const QString &filePath, const QSet<IDocument *> &documents)
        : m_document(0)
    {
        foreach (IDocument * const doc, documents) {
            if (doc->filePath().toString() == filePath) {
                m_document = doc;
                break;
            }
        }
        QTC_ASSERT(m_document, return);
        DocumentManager::expectFileChange(filePath);
        m_wasInDocumentManager = DocumentManager::removeDocument(m_document);
        QTC_CHECK(m_wasInDocumentManager);
    }

    ~ChangeExpector()
    {
        QTC_ASSERT(m_document, return);
        DocumentManager::addDocument(m_document);
        DocumentManager::unexpectFileChange(m_document->filePath().toString());
    }

private:
    IDocument *m_document;
    bool m_wasInDocumentManager;
};

bool QbsProject::ensureWriteableQbsFile(const QString &file)
{
    // Ensure that the file is not read only
    QFileInfo fi(file);
    if (!fi.isWritable()) {
        // Try via vcs manager
        IVersionControl *versionControl =
            VcsManager::findVersionControlForDirectory(fi.absolutePath());
        if (!versionControl || !versionControl->vcsOpen(file)) {
            bool makeWritable = QFile::setPermissions(file, fi.permissions() | QFile::WriteUser);
            if (!makeWritable) {
                QMessageBox::warning(ICore::mainWindow(),
                                     tr("Failed!"),
                                     tr("Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

qbs::GroupData QbsProject::reRetrieveGroupData(const qbs::ProductData &oldProduct,
                                          const qbs::GroupData &oldGroup)
{
    qbs::GroupData newGroup;
    foreach (const qbs::ProductData &pd, m_projectData.allProducts()) {
        if (uniqueProductName(pd) == uniqueProductName(oldProduct)) {
            foreach (const qbs::GroupData &gd, pd.groups()) {
                if (gd.location() == oldGroup.location()) {
                    newGroup = gd;
                    break;
                }
            }
            break;
        }
    }
    QTC_CHECK(newGroup.isValid());
    return newGroup;
}

bool QbsProject::addFilesToProduct(QbsBaseProjectNode *node, const QStringList &filePaths,
        const qbs::ProductData &productData, const qbs::GroupData &groupData, QStringList *notAdded)
{
    QTC_ASSERT(m_qbsProject.isValid(), return false);
    QStringList allPaths = groupData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_qbsDocuments);
    ensureWriteableQbsFile(productFilePath);
    foreach (const QString &path, filePaths) {
        qbs::ErrorInfo err = m_qbsProject.addFiles(productData, groupData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notAdded += path;
        } else {
            allPaths += path;
        }
    }
    if (notAdded->count() != filePaths.count()) {
        m_projectData = m_qbsProject.projectData();
        QbsGroupNode::setupFiles(node, reRetrieveGroupData(productData, groupData),
                                 allPaths, QFileInfo(productFilePath).absolutePath(), true);
        rootProjectNode()->update();
        emit fileListChanged();
    }
    return notAdded->isEmpty();
}

bool QbsProject::removeFilesFromProduct(QbsBaseProjectNode *node, const QStringList &filePaths,
        const qbs::ProductData &productData, const qbs::GroupData &groupData,
        QStringList *notRemoved)
{
    QTC_ASSERT(m_qbsProject.isValid(), return false);
    QStringList allPaths = groupData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    ChangeExpector expector(productFilePath, m_qbsDocuments);
    ensureWriteableQbsFile(productFilePath);
    foreach (const QString &path, filePaths) {
        qbs::ErrorInfo err
                = m_qbsProject.removeFiles(productData, groupData, QStringList() << path);
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notRemoved += path;
        } else {
            allPaths.removeOne(path);
        }
    }
    if (notRemoved->count() != filePaths.count()) {
        m_projectData = m_qbsProject.projectData();
        QbsGroupNode::setupFiles(node, reRetrieveGroupData(productData, groupData), allPaths,
                                 QFileInfo(productFilePath).absolutePath(), true);
        rootProjectNode()->update();
        emit fileListChanged();
    }
    return notRemoved->isEmpty();
}

bool QbsProject::renameFileInProduct(QbsBaseProjectNode *node, const QString &oldPath,
        const QString &newPath, const qbs::ProductData &productData,
        const qbs::GroupData &groupData)
{
    if (newPath.isEmpty())
        return false;
    QStringList dummy;
    if (!removeFilesFromProduct(node, QStringList() << oldPath, productData, groupData, &dummy))
        return false;
    qbs::ProductData newProductData;
    foreach (const qbs::ProductData &p, m_projectData.allProducts()) {
        if (uniqueProductName(p) == uniqueProductName(productData)) {
            newProductData = p;
            break;
        }
    }
    if (!newProductData.isValid())
        return false;
    qbs::GroupData newGroupData;
    foreach (const qbs::GroupData &g, newProductData.groups()) {
        if (g.name() == groupData.name()) {
            newGroupData = g;
            break;
        }
    }
    if (!newGroupData.isValid())
        return false;

    return addFilesToProduct(node, QStringList() << newPath, newProductData, newGroupData, &dummy);
}

void QbsProject::invalidate()
{
    prepareForParsing();
}

qbs::BuildJob *QbsProject::build(const qbs::BuildOptions &opts, QStringList productNames,
                                 QString &error)
{
    QTC_ASSERT(qbsProject().isValid(), return 0);
    QTC_ASSERT(!isParsing(), return 0);

    if (productNames.isEmpty())
        return qbsProject().buildAllProducts(opts);

    QList<qbs::ProductData> products;
    foreach (const QString &productName, productNames) {
        bool found = false;
        foreach (const qbs::ProductData &data, qbsProjectData().allProducts()) {
            if (uniqueProductName(data) == productName) {
                found = true;
                products.append(data);
                break;
            }
        }
        if (!found) {
            error = tr("Cannot build: Selected products do not exist anymore.");
            return 0;
        }
    }

    return qbsProject().buildSomeProducts(products, opts);
}

qbs::CleanJob *QbsProject::clean(const qbs::CleanOptions &opts)
{
    if (!qbsProject().isValid())
        return 0;
    return qbsProject().cleanAllProducts(opts);
}

qbs::InstallJob *QbsProject::install(const qbs::InstallOptions &opts)
{
    if (!qbsProject().isValid())
        return 0;
    return qbsProject().installAllProducts(opts);
}

QString QbsProject::profileForTarget(const Target *t) const
{
    return projectManager()->profileForKit(t->kit());
}

bool QbsProject::isParsing() const
{
    return m_qbsUpdateFutureInterface;
}

bool QbsProject::hasParseResult() const
{
    return qbsProject().isValid();
}

qbs::Project QbsProject::qbsProject() const
{
    return m_qbsProject;
}

qbs::ProjectData QbsProject::qbsProjectData() const
{
    return m_projectData;
}

bool QbsProject::needsSpecialDeployment() const
{
    return true;
}

bool QbsProject::checkCancelStatus()
{
    const CancelStatus cancelStatus = m_cancelStatus;
    m_cancelStatus = CancelStatusNone;
    if (cancelStatus != CancelStatusCancelingForReparse)
        return false;
    qCDebug(qbsPmLog) << "Cancel request while parsing, starting re-parse";
    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = 0;
    parseCurrentBuildConfiguration();
    return true;
}

void QbsProject::updateAfterParse()
{
    qCDebug(qbsPmLog) << "Updating data after parse";
    rootProjectNode()->update();
    updateDocuments(m_qbsProject.buildSystemFiles());
    updateBuildTargetData();
    updateCppCodeModel();
    updateQmlJsCodeModel();
    emit fileListChanged();
}

void QbsProject::handleQbsParsingDone(bool success)
{
    QTC_ASSERT(m_qbsProjectParser, return);
    QTC_ASSERT(m_qbsUpdateFutureInterface, return);

    qCDebug(qbsPmLog) << "Parsing done, success:" << success;

    if (checkCancelStatus())
        return;

    generateErrors(m_qbsProjectParser->error());

    m_qbsProject = m_qbsProjectParser->qbsProject();
    bool dataChanged = false;
    if (success) {
        QTC_ASSERT(m_qbsProject.isValid(), return);
        const qbs::ProjectData &projectData = m_qbsProject.projectData();
        if (projectData != m_projectData) {
            m_projectData = projectData;
            dataChanged = true;
        }
    } else {
        m_qbsUpdateFutureInterface->reportCanceled();
    }

    bool hasTargetArtifacts = false;
    if (dataChanged) {
        qCDebug(qbsPmLog) << "Project data changed.";
        foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
            if (!product.targetArtifacts().isEmpty()) {
                hasTargetArtifacts = true;
                break;
            }
        }
        if (!hasTargetArtifacts) {
            qCDebug(qbsPmLog) << "No target artifacts present, executing rules";
            m_qbsProjectParser->startRuleExecution();
            return;
        }
    }

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = 0;
    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    if (dataChanged)
        updateAfterParse();
    emit projectParsingDone(success);
}

void QbsProject::handleRuleExecutionDone()
{
    qCDebug(qbsPmLog) << "Rule execution done";

    if (checkCancelStatus())
        return;

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = 0;
    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    QTC_ASSERT(m_qbsProject.isValid(), return);
    m_projectData = m_qbsProject.projectData();
    updateAfterParse();
    emit projectParsingDone(true);
}

void QbsProject::targetWasAdded(Target *t)
{
    connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(delayParsing()));
    connect(t, SIGNAL(buildDirectoryChanged()), this, SLOT(delayParsing()));
}

void QbsProject::changeActiveTarget(Target *t)
{
    BuildConfiguration *bc = 0;
    if (t && t->kit())
        bc = t->activeBuildConfiguration();
    buildConfigurationChanged(bc);
}

void QbsProject::buildConfigurationChanged(BuildConfiguration *bc)
{
    if (m_currentBc)
        disconnect(m_currentBc, SIGNAL(qbsConfigurationChanged()), this, SLOT(delayParsing()));

    m_currentBc = qobject_cast<QbsBuildConfiguration *>(bc);
    if (m_currentBc) {
        connect(m_currentBc, SIGNAL(qbsConfigurationChanged()), this, SLOT(delayParsing()));
        delayParsing();
    } else {
        invalidate();
    }
}

void QbsProject::startParsing()
{
    // Qbs does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(this)) {
        scheduleParsing();
        return;
    }

    parseCurrentBuildConfiguration();
}

void QbsProject::delayParsing()
{
    m_parsingDelay.start();
}

void QbsProject::parseCurrentBuildConfiguration()
{
    m_parsingScheduled = false;
    if (m_cancelStatus == CancelStatusCancelingForReparse)
        return;

    // The CancelStatusCancelingAltoghether type can only be set by a build job, during
    // which no other parse requests come through to this point (except by the build job itself,
    // but of course not while canceling is in progress).
    QTC_ASSERT(m_cancelStatus == CancelStatusNone, return);

    if (!activeTarget())
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    // New parse requests override old ones.
    // NOTE: We need to wait for the current operation to finish, since otherwise there could
    //       be a conflict. Consider the case where the old qbs::ProjectSetupJob is writing
    //       to the build graph file when the cancel request comes in. If we don't wait for
    //       acknowledgment, it might still be doing that when the new one already reads from the
    //       same file.
    if (m_qbsProjectParser) {
        m_cancelStatus = CancelStatusCancelingForReparse;
        m_qbsProjectParser->cancel();
        return;
    }

    parse(bc->qbsConfiguration(), bc->environment(), bc->buildDirectory().toString());
}

void QbsProject::cancelParsing()
{
    QTC_ASSERT(m_qbsProjectParser, return);
    m_cancelStatus = CancelStatusCancelingAltoghether;
    m_qbsProjectParser->cancel();
}

void QbsProject::updateAfterBuild()
{
    QTC_ASSERT(m_qbsProject.isValid(), return);
    m_projectData = m_qbsProject.projectData();
    updateBuildTargetData();
    updateCppCompilerCallData();
    if (m_extraCompilersPending) {
        m_extraCompilersPending = false;
        updateCppCodeModel();
    }
}

void QbsProject::registerQbsProjectParser(QbsProjectParser *p)
{
    m_parsingDelay.stop();

    if (m_qbsProjectParser) {
        m_qbsProjectParser->disconnect(this);
        m_qbsProjectParser->deleteLater();
    }

    m_qbsProjectParser = p;

    if (p) {
        connect(m_qbsProjectParser, &QbsProjectParser::ruleExecutionDone,
                this, &QbsProject::handleRuleExecutionDone);
        connect(m_qbsProjectParser, SIGNAL(done(bool)), this, SLOT(handleQbsParsingDone(bool)));
    }
}

Project::RestoreResult QbsProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;

    Kit *defaultKit = KitManager::defaultKit();
    if (!activeTarget() && defaultKit)
        addTarget(createTarget(defaultKit));

    return RestoreResult::Ok;
}

void QbsProject::generateErrors(const qbs::ErrorInfo &e)
{
    foreach (const qbs::ErrorItem &item, e.items())
        TaskHub::addTask(Task::Error, item.description(),
                         ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                         FileName::fromString(item.codeLocation().filePath()),
                         item.codeLocation().line());

}

QString QbsProject::productDisplayName(const qbs::Project &project,
                                       const qbs::ProductData &product)
{
    QString displayName = product.name();
    if (product.profile() != project.profile())
        displayName.append(QLatin1String(" [")).append(product.profile()).append(QLatin1Char(']'));
    return displayName;
}

QString QbsProject::uniqueProductName(const qbs::ProductData &product)
{
    return product.name() + QLatin1Char('.') + product.profile();
}

void QbsProject::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    prepareForParsing();
    QTC_ASSERT(!m_qbsProjectParser, return);

    registerQbsProjectParser(new QbsProjectParser(this, m_qbsUpdateFutureInterface));

    QbsManager::instance()->updateProfileIfNecessary(activeTarget()->kit());
    m_qbsProjectParser->parse(config, env, dir);
    emit projectParsingStarted();
}

void QbsProject::prepareForParsing()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
    }
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    m_qbsUpdateFutureInterface = new QFutureInterface<bool>();
    m_qbsUpdateFutureInterface->setProgressRange(0, 0);
    ProgressManager::addTask(m_qbsUpdateFutureInterface->future(),
        tr("Reading Project \"%1\"").arg(displayName()), "Qbs.QbsEvaluate");
    m_qbsUpdateFutureInterface->reportStarted();
}

void QbsProject::updateDocuments(const QSet<QString> &files)
{
    // Update documents:
    QSet<QString> newFiles = files;
    QTC_ASSERT(!newFiles.isEmpty(), newFiles << projectFilePath().toString());
    QSet<QString> oldFiles;
    foreach (IDocument *doc, m_qbsDocuments)
        oldFiles.insert(doc->filePath().toString());

    QSet<QString> filesToAdd = newFiles;
    filesToAdd.subtract(oldFiles);
    QSet<QString> filesToRemove = oldFiles;
    filesToRemove.subtract(newFiles);

    QSet<IDocument *> currentDocuments = m_qbsDocuments;
    foreach (IDocument *doc, currentDocuments) {
        if (filesToRemove.contains(doc->filePath().toString())) {
            m_qbsDocuments.remove(doc);
            delete doc;
        }
    }
    QSet<IDocument *> toAdd;
    foreach (const QString &f, filesToAdd)
        toAdd.insert(new QbsProjectFile(this, f));

    DocumentManager::addDocuments(toAdd.toList());
    m_qbsDocuments.unite(toAdd);
}

static CppTools::ProjectFile::Kind cppFileType(const qbs::SourceArtifact &sourceFile)
{
    if (sourceFile.fileTags().contains(QLatin1String("hpp")))
        return CppTools::ProjectFile::CXXHeader;
    if (sourceFile.fileTags().contains(QLatin1String("cpp")))
        return CppTools::ProjectFile::CXXSource;
    if (sourceFile.fileTags().contains(QLatin1String("c")))
        return CppTools::ProjectFile::CSource;
    if (sourceFile.fileTags().contains(QLatin1String("objc")))
        return CppTools::ProjectFile::ObjCSource;
    if (sourceFile.fileTags().contains(QLatin1String("objcpp")))
        return CppTools::ProjectFile::ObjCXXSource;
    return CppTools::ProjectFile::Unclassified;
}

static QString groupLocationToProjectFile(const qbs::CodeLocation &location)
{
    return QString::fromLatin1("%1:%2:%3")
                        .arg(location.filePath())
                        .arg(location.line())
                        .arg(location.column());
}

void QbsProject::updateCppCodeModel()
{
    if (!m_projectData.isValid())
        return;

    QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit());

    CppTools::CppModelManager *modelmanager = CppTools::CppModelManager::instance();
    CppTools::ProjectInfo pinfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pinfo);

    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            ppBuilder.setQtVersion(CppTools::ProjectPart::Qt4);
        else
            ppBuilder.setQtVersion(CppTools::ProjectPart::Qt5);
    } else {
        ppBuilder.setQtVersion(CppTools::ProjectPart::NoQt);
    }

    QList<ProjectExplorer::ExtraCompilerFactory *> factories =
            ProjectExplorer::ExtraCompilerFactory::extraCompilerFactories();
    const auto factoriesBegin = factories.constBegin();
    const auto factoriesEnd = factories.constEnd();

    qDeleteAll(m_extraCompilers);
    m_extraCompilers.clear();
    foreach (const qbs::ProductData &prd, m_projectData.allProducts()) {
        foreach (const qbs::GroupData &grp, prd.groups()) {
            const qbs::PropertyMap &props = grp.properties();

            ppBuilder.setCxxFlags(props.getModulePropertiesAsStringList(
                                      QLatin1String(CONFIG_CPP_MODULE),
                                      QLatin1String(CONFIG_CXXFLAGS)));
            ppBuilder.setCFlags(props.getModulePropertiesAsStringList(
                                    QLatin1String(CONFIG_CPP_MODULE),
                                    QLatin1String(CONFIG_CFLAGS)));

            QStringList list = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_DEFINES));
            QByteArray grpDefines;
            foreach (const QString &def, list) {
                QByteArray data = def.toUtf8();
                int pos = data.indexOf('=');
                if (pos >= 0)
                    data[pos] = ' ';
                else
                    data.append(" 1"); // cpp.defines: [ "FOO" ] is considered to be "FOO=1"
                grpDefines += (QByteArray("#define ") + data + '\n');
            }
            ppBuilder.setDefines(grpDefines);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_INCLUDEPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_INCLUDEPATHS)));
            CppTools::ProjectPartHeaderPaths grpHeaderPaths;
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPartHeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPartHeaderPath::IncludePath);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_FRAMEWORKPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_FRAMEWORKPATHS)));
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPartHeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPartHeaderPath::FrameworkPath);

            ppBuilder.setHeaderPaths(grpHeaderPaths);

            const QString pch = props.getModuleProperty(QLatin1String(CONFIG_CPP_MODULE),
                    QLatin1String(CONFIG_PRECOMPILEDHEADER)).toString();
            ppBuilder.setPreCompiledHeaders(QStringList() << pch);

            ppBuilder.setDisplayName(grp.name());
            ppBuilder.setProjectFile(groupLocationToProjectFile(grp.location()));

            QHash<QString, qbs::SourceArtifact> filePathToSourceArtifact;
            foreach (const qbs::SourceArtifact &source, grp.allSourceArtifacts()) {
                filePathToSourceArtifact.insert(source.filePath(), source);

                foreach (const QString &tag, source.fileTags()) {
                    for (auto i = factoriesBegin; i != factoriesEnd; ++i) {
                        if ((*i)->sourceTag() != tag)
                            continue;
                        QStringList generated = m_qbsProject.generatedFiles(prd, source.filePath(),
                                                                            false);
                        if (generated.isEmpty()) {
                            // We don't know the target files until we build for the first time.
                            m_extraCompilersPending = true;
                            continue;
                        }

                        const FileNameList fileNames = Utils::transform(generated,
                                                                        [](const QString &s) {
                            return Utils::FileName::fromString(s);
                        });
                        m_extraCompilers.append((*i)->create(
                                this, FileName::fromString(source.filePath()), fileNames));
                    }
                }
            }

            const QList<Id> languages = ppBuilder.createProjectPartsForFiles(
                grp.allFilePaths(),
                [filePathToSourceArtifact](const QString &filePath) {
                    return cppFileType(filePathToSourceArtifact.value(filePath));
            });

            foreach (Id language, languages)
                setProjectLanguage(language, true);
        }
    }

    pinfo.finish();

    CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);

    // Update the code model
    m_codeModelFuture.cancel();
    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);
    m_codeModelProjectInfo = modelmanager->projectInfo(this);
    QTC_CHECK(m_codeModelProjectInfo == pinfo);
}

void QbsProject::updateCppCompilerCallData()
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    QTC_ASSERT(m_codeModelProjectInfo == modelManager->projectInfo(this), return);

    CppTools::ProjectInfo::CompilerCallData data;
    foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
        if (!product.isEnabled())
            continue;

        foreach (const qbs::GroupData &group, product.groups()) {
            if (!group.isEnabled())
                continue;

            CppTools::ProjectInfo::CompilerCallGroup compilerCallGroup;
            compilerCallGroup.groupId = groupLocationToProjectFile(group.location());

            foreach (const qbs::SourceArtifact &file, group.allSourceArtifacts()) {
                const QString &filePath = file.filePath();
                if (!CppTools::ProjectFile::isSource(cppFileType(file)))
                    continue;

                qbs::ErrorInfo errorInfo;
                const qbs::RuleCommandList ruleCommands = m_qbsProject.ruleCommands(product,
                        filePath, QLatin1String("obj"), &errorInfo);
                if (errorInfo.hasError())
                    continue;

                QList<QStringList> calls;
                foreach (const qbs::RuleCommand &ruleCommand, ruleCommands) {
                    if (ruleCommand.type() == qbs::RuleCommand::ProcessCommandType)
                        calls << ruleCommand.arguments();
                }

                if (!calls.isEmpty())
                    compilerCallGroup.callsPerSourceFile.insert(filePath, calls);
            }

            if (!compilerCallGroup.callsPerSourceFile.isEmpty())
                data.append(compilerCallGroup);
        }
    }

    m_codeModelProjectInfo = modelManager->updateCompilerCallDataForProject(this, data);
}

void QbsProject::updateQmlJsCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);
    foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
        static const QString propertyName = QLatin1String("qmlImportPaths");
        foreach (const QString &path, product.properties().value(propertyName).toStringList()) {
            projectInfo.importPaths.maybeInsert(Utils::FileName::fromString(path),
                                                QmlJS::Dialect::Qml);
        }
    }

    setProjectLanguage(ProjectExplorer::Constants::LANG_QMLJS, !projectInfo.sourceFiles.isEmpty());
    modelManager->updateProjectInfo(projectInfo, this);
}

void QbsProject::updateApplicationTargets()
{
    BuildTargetInfoList applications;
    foreach (const qbs::ProductData &productData, m_projectData.allProducts()) {
        if (!productData.isEnabled() || !productData.isRunnable())
            continue;
        const QString displayName = productDisplayName(m_qbsProject, productData);
        if (productData.targetArtifacts().isEmpty()) { // No build yet.
            applications.list << BuildTargetInfo(displayName,
                    FileName(),
                    FileName::fromString(productData.location().filePath()));
            continue;
        }
        foreach (const qbs::TargetArtifact &ta, productData.targetArtifacts()) {
            QTC_ASSERT(ta.isValid(), continue);
            if (!ta.isExecutable())
                continue;
            applications.list << BuildTargetInfo(displayName,
                    FileName::fromString(ta.filePath()),
                    FileName::fromString(productData.location().filePath()));
        }
    }
    activeTarget()->setApplicationTargets(applications);
}

void QbsProject::updateDeploymentInfo()
{
    DeploymentData deploymentData;
    if (m_qbsProject.isValid()) {
        qbs::InstallOptions installOptions;
        installOptions.setInstallRoot(QLatin1String("/"));
        foreach (const qbs::InstallableFile &f, m_qbsProject
                     .installableFilesForProject(m_projectData, installOptions)) {
            deploymentData.addFile(f.sourceFilePath(), QFileInfo(f.targetFilePath()).path(),
                    f.isExecutable() ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
        }
    }
    activeTarget()->setDeploymentData(deploymentData);
}

void QbsProject::updateBuildTargetData()
{
    updateApplicationTargets();
    updateDeploymentInfo();
    if (activeTarget())
        activeTarget()->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace QbsProjectManager
