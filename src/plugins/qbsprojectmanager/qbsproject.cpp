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
#include "qbsbuildstep.h"
#include "qbsinstallstep.h"
#include "qbslogsink.h"
#include "qbspmlogging.h"
#include "qbsprojectimporter.h"
#include "qbsprojectparser.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsnodes.h"
#include "qbsnodetreebuilder.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljsmodelmanager.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

#include <qbs.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMessageBox>
#include <QSet>
#include <QVariantMap>

#include <algorithm>
#include <type_traits>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CPP_MODULE[] = "cpp";
static const char CONFIG_DEFINES[] = "defines";
static const char CONFIG_INCLUDEPATHS[] = "includePaths";
static const char CONFIG_SYSTEM_INCLUDEPATHS[] = "systemIncludePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "frameworkPaths";
static const char CONFIG_SYSTEM_FRAMEWORKPATHS[] = "systemFrameworkPaths";

class OpTimer
{
public:
    OpTimer(const char *name) : m_name(name)
    {
        m_timer.start();
    }
    ~OpTimer()
    {
        if (qEnvironmentVariableIsSet(Constants::QBS_PROFILING_ENV)) {
            MessageManager::write(QString("operation %1 took %2ms")
                                  .arg(QLatin1String(m_name)).arg(m_timer.elapsed()));
        }
    }

private:
    QElapsedTimer m_timer;
    const char * const m_name;
};

// --------------------------------------------------------------------
// QbsProject:
// --------------------------------------------------------------------

QbsProject::QbsProject(const FilePath &fileName)
    : Project(Constants::MIME_TYPE, fileName)
{
    setId(Constants::PROJECT_ID);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setCanBuildProducts();
    setDisplayName(fileName.toFileInfo().completeBaseName());
}

QbsProject::~QbsProject()
{
    delete m_importer;
}

ProjectImporter *QbsProject::projectImporter() const
{
    if (!m_importer)
        m_importer = new QbsProjectImporter(projectFilePath());
    return m_importer;
}

// --------------------------------------------------------------------
// QbsBuildSystem:
// --------------------------------------------------------------------

static const QbsProjectNode *parentQbsProjectNode(const ProjectExplorer::Node *node)
{
    for (const ProjectExplorer::FolderNode *pn = node->managingProject(); pn; pn = pn->parentProjectNode()) {
        const auto prjNode = dynamic_cast<const QbsProjectNode *>(pn);
        if (prjNode)
            return prjNode;
    }
    return nullptr;
}

static const QbsProductNode *parentQbsProductNode(const ProjectExplorer::Node *node)
{
    for (; node; node = node->parentFolderNode()) {
        const auto prdNode = dynamic_cast<const QbsProductNode *>(node);
        if (prdNode)
            return prdNode;
    }
    return nullptr;
}

static bool supportsNodeAction(ProjectAction action, const Node *node)
{
    const Project * const project = parentQbsProjectNode(node)->project();
    Target *t = project ? project->activeTarget() : nullptr;
    QbsBuildSystem *bs = t ? static_cast<QbsBuildSystem *>(t->buildSystem()) : nullptr;
    if (!bs)
        return false;
    if (!bs->isProjectEditable())
        return false;
    if (action == RemoveFile || action == Rename)
        return node->asFileNode();
    return false;
}

static qbs::GroupData findMainQbsGroup(const qbs::ProductData &productData)
{
    foreach (const qbs::GroupData &grp, productData.groups()) {
        if (grp.name() == productData.name() && grp.location() == productData.location())
            return grp;
    }
    return qbs::GroupData();
}

QbsBuildSystem::QbsBuildSystem(QbsBuildConfiguration *bc)
    : BuildSystem(bc->target()),
      m_cppCodeModelUpdater(new CppTools::CppProjectUpdater),
      m_buildConfiguration(bc)

{
    m_parsingDelay.setInterval(1000); // delay parsing by 1s.
    delayParsing();

    connect(bc->project(), &Project::activeTargetChanged,
            this, &QbsBuildSystem::changeActiveTarget);

    connect(bc->target(), &Target::activeBuildConfigurationChanged,
            this, &QbsBuildSystem::delayParsing);

    connect(&m_parsingDelay, &QTimer::timeout, this, &QbsBuildSystem::triggerParsing);

    connect(bc->project(), &Project::projectFileIsDirty, this, &QbsBuildSystem::delayParsing);

    rebuildProjectTree();
}

QbsBuildSystem::~QbsBuildSystem()
{
    delete m_cppCodeModelUpdater;
    delete m_qbsProjectParser;
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
        delete m_qbsUpdateFutureInterface;
        m_qbsUpdateFutureInterface = nullptr;
    }
    qDeleteAll(m_extraCompilers);
}

bool QbsBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (dynamic_cast<QbsGroupNode *>(context)) {
        if (action == AddNewFile || action == AddExistingFile)
            return true;
    }

    if (dynamic_cast<QbsProductNode *>(context)) {
        if (action == AddNewFile || action == AddExistingFile)
            return true;
    }

    return supportsNodeAction(action, node);
}

bool QbsBuildSystem::addFiles(Node *context, const QStringList &filePaths, QStringList *notAdded)
{
    if (auto n = dynamic_cast<QbsGroupNode *>(context)) {
        QStringList notAddedDummy;
        if (!notAdded)
            notAdded = &notAddedDummy;

        const QbsProjectNode *prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid()) {
            *notAdded += filePaths;
            return false;
        }

        const QbsProductNode *prdNode = parentQbsProductNode(n);
        if (!prdNode || !prdNode->qbsProductData().isValid()) {
            *notAdded += filePaths;
            return false;
        }

        return addFilesToProduct(filePaths, prdNode->qbsProductData(), n->m_qbsGroupData, notAdded);
    }

    if (auto n = dynamic_cast<QbsProductNode *>(context)) {
        QStringList notAddedDummy;
        if (!notAdded)
            notAdded = &notAddedDummy;

        const QbsProjectNode *prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid()) {
            *notAdded += filePaths;
            return false;
        }

        qbs::GroupData grp = findMainQbsGroup(n->qbsProductData());
        if (grp.isValid())
            return addFilesToProduct(filePaths, n->qbsProductData(), grp, notAdded);

        QTC_ASSERT(false, return false);
    }

    return BuildSystem::addFiles(context, filePaths, notAdded);
}

RemovedFilesFromProject QbsBuildSystem::removeFiles(Node *context, const QStringList &filePaths,
                                                    QStringList *notRemoved)
{
    if (auto n = dynamic_cast<QbsGroupNode *>(context)) {
        QStringList notRemovedDummy;
        if (!notRemoved)
            notRemoved = &notRemovedDummy;

        const QbsProjectNode *prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid()) {
            *notRemoved += filePaths;
            return RemovedFilesFromProject::Error;
        }

        const QbsProductNode *prdNode = parentQbsProductNode(n);
        if (!prdNode || !prdNode->qbsProductData().isValid()) {
            *notRemoved += filePaths;
            return RemovedFilesFromProject::Error;
        }

        return removeFilesFromProduct(filePaths, prdNode->qbsProductData(),
                                      n->m_qbsGroupData, notRemoved);
    }

    if (auto n = dynamic_cast<QbsProductNode *>(context)) {
        QStringList notRemovedDummy;
        if (!notRemoved)
            notRemoved = &notRemovedDummy;

        const QbsProjectNode *prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid()) {
            *notRemoved += filePaths;
            return RemovedFilesFromProject::Error;
        }

        qbs::GroupData grp = findMainQbsGroup(n->qbsProductData());
        if (grp.isValid()) {
            return removeFilesFromProduct(filePaths, n->qbsProductData(), grp, notRemoved);
        }

        QTC_ASSERT(false, return RemovedFilesFromProject::Error);
    }

    return BuildSystem::removeFiles(context, filePaths, notRemoved);
}

bool QbsBuildSystem::renameFile(Node *context, const QString &filePath, const QString &newFilePath)
{
    if (auto *n = dynamic_cast<QbsGroupNode *>(context)) {
        const QbsProjectNode *prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid())
            return false;
        const QbsProductNode *prdNode = parentQbsProductNode(n);
        if (!prdNode || !prdNode->qbsProductData().isValid())
            return false;

        return renameFileInProduct(filePath, newFilePath,
                                   prdNode->qbsProductData(), n->m_qbsGroupData);
    }

    if (auto *n = dynamic_cast<QbsProductNode *>(context)) {
        const QbsProjectNode * prjNode = parentQbsProjectNode(n);
        if (!prjNode || !qbsProject().isValid())
            return false;
        const qbs::GroupData grp = findMainQbsGroup(n->qbsProductData());
        QTC_ASSERT(grp.isValid(), return false);
        return renameFileInProduct(filePath, newFilePath, n->qbsProductData(), grp);
    }

    return BuildSystem::renameFile(context, filePath, newFilePath);
}

QVariant QbsBuildSystem::additionalData(Id id) const
{
    if (id == "QmlDesignerImportPath") {
        const qbs::ProjectData projectData = m_qbsProject.isValid()
                ? m_qbsProject.projectData() : qbs::ProjectData();
        QStringList designerImportPaths;
        foreach (const qbs::ProductData &product, projectData.allProducts()) {
            designerImportPaths << product.properties()
                                   .value("qmlDesignerImportPaths").toStringList();
        }
        return designerImportPaths;
    }
    return BuildSystem::additionalData(id);
}

ProjectExplorer::DeploymentKnowledge QbsProject::deploymentKnowledge() const
{
    return DeploymentKnowledge::Perfect;
}

QStringList QbsBuildSystem::filesGeneratedFrom(const QString &sourceFile) const
{
    QStringList generated;
    foreach (const qbs::ProductData &data, m_projectData.allProducts())
         generated << m_qbsProject.generatedFiles(data, sourceFile, false);
    return generated;
}

bool QbsBuildSystem::isProjectEditable() const
{
    return m_qbsProject.isValid() && !isParsing() && !BuildManager::isBuilding();
}

bool QbsBuildSystem::ensureWriteableQbsFile(const QString &file)
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
                                     tr("Failed"),
                                     tr("Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

bool QbsBuildSystem::addFilesToProduct(const QStringList &filePaths,
                                       const qbs::ProductData productData,
                                       const qbs::GroupData groupData, QStringList *notAdded)
{
    QTC_ASSERT(m_qbsProject.isValid(), return false);
    QStringList allPaths = groupData.allFilePaths();
    const QString productFilePath = productData.location().filePath();
    Core::FileChangeBlocker expector(productFilePath);
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
        delayedUpdateAfterParse();
    }
    return notAdded->isEmpty();
}

RemovedFilesFromProject QbsBuildSystem::removeFilesFromProduct(const QStringList &filePaths,
                                                               const qbs::ProductData &productData,
                                                               const qbs::GroupData &groupData,
                                                               QStringList *notRemoved)
{
    QTC_ASSERT(m_qbsProject.isValid(), return RemovedFilesFromProject::Error);

    const QList<qbs::ArtifactData> allWildcardsInGroup = groupData.sourceArtifactsFromWildcards();
    QStringList wildcardFiles;
    QStringList nonWildcardFiles;
    for (const QString &filePath : filePaths) {
        if (contains(allWildcardsInGroup, [filePath](const qbs::ArtifactData &artifact) {
                     return artifact.filePath() == filePath; })) {
            wildcardFiles << filePath;
        } else {
            nonWildcardFiles << filePath;
        }
    }
    const QString productFilePath = productData.location().filePath();
    Core::FileChangeBlocker expector(productFilePath);
    ensureWriteableQbsFile(productFilePath);
    for (const QString &path : qAsConst(nonWildcardFiles)) {
        const qbs::ErrorInfo err = m_qbsProject.removeFiles(productData, groupData, {path});
        if (err.hasError()) {
            MessageManager::write(err.toString());
            *notRemoved += path;
        }
    }

    if (notRemoved->count() != filePaths.count()) {
        m_projectData = m_qbsProject.projectData();
        delayedUpdateAfterParse();
    }

    const bool success = notRemoved->isEmpty();
    if (!wildcardFiles.isEmpty()) {
        *notRemoved += wildcardFiles;
        delayParsing();
    }
    if (!success)
        return RemovedFilesFromProject::Error;
    if (!wildcardFiles.isEmpty())
        return RemovedFilesFromProject::Wildcard;
    return RemovedFilesFromProject::Ok;
}

bool QbsBuildSystem::renameFileInProduct(const QString &oldPath, const QString &newPath,
                                         const qbs::ProductData productData,
                                         const qbs::GroupData groupData)
{
    if (newPath.isEmpty())
        return false;
    QStringList dummy;
    if (removeFilesFromProduct(QStringList(oldPath), productData, groupData, &dummy)
            != RemovedFilesFromProject::Ok) {
        return false;
    }
    qbs::ProductData newProductData;
    foreach (const qbs::ProductData &p, m_projectData.allProducts()) {
        if (QbsProject::uniqueProductName(p) == QbsProject::uniqueProductName(productData)) {
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

    return addFilesToProduct(QStringList() << newPath, newProductData, newGroupData, &dummy);
}

static qbs::AbstractJob *doBuildOrClean(const qbs::Project &project,
                                        const QList<qbs::ProductData> &products,
                                        const qbs::BuildOptions &options)
{
    if (products.isEmpty())
        return project.buildAllProducts(options);
    return project.buildSomeProducts(products, options);
}

static qbs::AbstractJob *doBuildOrClean(const qbs::Project &project,
                                        const QList<qbs::ProductData> &products,
                                        const qbs::CleanOptions &options)
{
    if (products.isEmpty())
        return project.cleanAllProducts(options);
    return project.cleanSomeProducts(products, options);
}

template<typename Options>
qbs::AbstractJob *QbsBuildSystem::buildOrClean(const Options &opts, const QStringList &productNames,
                                               QString &error)
{
    QTC_ASSERT(qbsProject().isValid(), return nullptr);
    QTC_ASSERT(!isParsing(), return nullptr);

    QList<qbs::ProductData> products;
    foreach (const QString &productName, productNames) {
        bool found = false;
        foreach (const qbs::ProductData &data, qbsProjectData().allProducts()) {
            if (QbsProject::uniqueProductName(data) == productName) {
                found = true;
                products.append(data);
                break;
            }
        }
        if (!found) {
            const bool cleaningRequested = std::is_same<Options, qbs::CleanOptions>::value;
            error = tr("%1: Selected products do not exist anymore.")
                    .arg(cleaningRequested ? tr("Cannot clean") : tr("Cannot build"));
            return nullptr;
        }
    }
    return doBuildOrClean(qbsProject(), products, opts);
}

qbs::BuildJob *QbsBuildSystem::build(const qbs::BuildOptions &opts, QStringList productNames,
                                     QString &error)
{
    return static_cast<qbs::BuildJob *>(buildOrClean(opts, productNames, error));
}

qbs::CleanJob *QbsBuildSystem::clean(const qbs::CleanOptions &opts, const QStringList &productNames,
                                     QString &error)
{
    return static_cast<qbs::CleanJob *>(buildOrClean(opts, productNames, error));
}

qbs::InstallJob *QbsBuildSystem::install(const qbs::InstallOptions &opts)
{
    if (!qbsProject().isValid())
        return nullptr;
    return qbsProject().installAllProducts(opts);
}

QString QbsBuildSystem::profile() const
{
    return QbsManager::profileForKit(target()->kit());
}

qbs::Project QbsBuildSystem::qbsProject() const
{
    return m_qbsProject;
}

qbs::ProjectData QbsBuildSystem::qbsProjectData() const
{
    return m_projectData;
}

bool QbsBuildSystem::checkCancelStatus()
{
    const CancelStatus cancelStatus = m_cancelStatus;
    m_cancelStatus = CancelStatusNone;
    if (cancelStatus != CancelStatusCancelingForReparse)
        return false;
    qCDebug(qbsPmLog) << "Cancel request while parsing, starting re-parse";
    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = nullptr;
    m_guard = {};
    parseCurrentBuildConfiguration();
    return true;
}

void QbsBuildSystem::updateAfterParse()
{
    qCDebug(qbsPmLog) << "Updating data after parse";
    OpTimer opTimer("updateAfterParse");
    updateProjectNodes();
    updateDocuments(m_qbsProject.buildSystemFiles());
    updateBuildTargetData();
    updateCppCodeModel();
    updateQmlJsCodeModel();
    emit project()->fileListChanged();
    m_envCache.clear();
}

void QbsBuildSystem::delayedUpdateAfterParse()
{
    QTimer::singleShot(0, this, &QbsBuildSystem::updateAfterParse);
}

void QbsBuildSystem::updateProjectNodes()
{
    OpTimer opTimer("updateProjectNodes");
    rebuildProjectTree();
}

FilePath QbsBuildSystem::installRoot()
{
    const auto dc = target()->activeDeployConfiguration();
    if (dc) {
        const QList<BuildStep *> steps = dc->stepList()->steps();
        for (const BuildStep * const step : steps) {
            if (!step->enabled())
                continue;
            if (const auto qbsInstallStep = qobject_cast<const QbsInstallStep *>(step))
                return FilePath::fromString(qbsInstallStep->installRoot());
        }
    }
    const QbsBuildStep * const buildStep = m_buildConfiguration->qbsStep();
    return buildStep && buildStep->install() ? buildStep->installRoot() : FilePath();
}

void QbsBuildSystem::handleQbsParsingDone(bool success)
{
    QTC_ASSERT(m_qbsProjectParser, return);
    QTC_ASSERT(m_qbsUpdateFutureInterface, return);

    qCDebug(qbsPmLog) << "Parsing done, success:" << success;

    if (checkCancelStatus())
        return;

    generateErrors(m_qbsProjectParser->error());

    m_qbsProject = m_qbsProjectParser->qbsProject();
    bool dataChanged = false;
    bool envChanged = m_lastParseEnv != m_qbsProjectParser->environment();
    m_lastParseEnv = m_qbsProjectParser->environment();
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

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = nullptr;
    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = nullptr;

    if (dataChanged)
        updateAfterParse();
    else if (envChanged)
        updateCppCodeModel();
    m_guard.markAsSuccess();
    m_guard = {};
}

void QbsBuildSystem::rebuildProjectTree()
{
    std::unique_ptr<QbsRootProjectNode> newRoot = Internal::QbsNodeTreeBuilder::buildTree(this);
    project()->setDisplayName(newRoot ? newRoot->displayName() : projectFilePath().toFileInfo().completeBaseName());
    project()->setRootProjectNode(std::move(newRoot));
}

void QbsBuildSystem::handleRuleExecutionDone()
{
    qCDebug(qbsPmLog) << "Rule execution done";

    if (checkCancelStatus())
        return;

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = nullptr;
    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = nullptr;

    QTC_ASSERT(m_qbsProject.isValid(), return);
    m_projectData = m_qbsProject.projectData();
    updateAfterParse();
}

void QbsBuildSystem::changeActiveTarget(Target *t)
{
    if (t)
        delayParsing();
}

void QbsBuildSystem::triggerParsing()
{
    // Qbs does update the build graph during the build. So we cannot
    // start to parse while a build is running or we will lose information.
    if (BuildManager::isBuilding(project())) {
        scheduleParsing();
        return;
    }

    parseCurrentBuildConfiguration();
}

void QbsBuildSystem::delayParsing()
{
    if (m_buildConfiguration->isActive())
        m_parsingDelay.start();
}

void QbsBuildSystem::parseCurrentBuildConfiguration()
{
    m_parsingScheduled = false;
    if (m_cancelStatus == CancelStatusCancelingForReparse)
        return;

    // The CancelStatusCancelingAltoghether type can only be set by a build job, during
    // which no other parse requests come through to this point (except by the build job itself,
    // but of course not while canceling is in progress).
    QTC_ASSERT(m_cancelStatus == CancelStatusNone, return);

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

    QVariantMap config = m_buildConfiguration->qbsConfiguration();
    Environment env = m_buildConfiguration->environment();
    QString dir = m_buildConfiguration->buildDirectory().toString();

    m_guard = guardParsingRun();

    prepareForParsing();

    m_parsingDelay.stop();

    QTC_ASSERT(!m_qbsProjectParser, return);
    m_qbsProjectParser = new QbsProjectParser(this, m_qbsUpdateFutureInterface);

    connect(m_qbsProjectParser, &QbsProjectParser::ruleExecutionDone,
            this, &QbsBuildSystem::handleRuleExecutionDone);
    connect(m_qbsProjectParser, &QbsProjectParser::done,
            this, &QbsBuildSystem::handleQbsParsingDone);

    QbsManager::updateProfileIfNecessary(target()->kit());
    m_qbsProjectParser->parse(config, env, dir, m_buildConfiguration->configurationName());
}

void QbsBuildSystem::cancelParsing()
{
    QTC_ASSERT(m_qbsProjectParser, return);
    m_cancelStatus = CancelStatusCancelingAltoghether;
    m_qbsProjectParser->cancel();
}

void QbsBuildSystem::updateAfterBuild()
{
    OpTimer opTimer("updateAfterBuild");
    QTC_ASSERT(m_qbsProject.isValid(), return);
    const qbs::ProjectData &projectData = m_qbsProject.projectData();
    if (projectData == m_projectData) {
        DeploymentData deploymentDataTmp = deploymentData();
        deploymentDataTmp.setLocalInstallRoot(installRoot());
        setDeploymentData(deploymentDataTmp);
        return;
    }
    qCDebug(qbsPmLog) << "Updating data after build";
    m_projectData = projectData;
    updateProjectNodes();
    updateBuildTargetData();
    if (m_extraCompilersPending) {
        m_extraCompilersPending = false;
        updateCppCodeModel();
    }
    m_envCache.clear();
}

void QbsBuildSystem::generateErrors(const qbs::ErrorInfo &e)
{
    foreach (const qbs::ErrorItem &item, e.items())
        TaskHub::addTask(Task::Error, item.description(),
                         ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                         FilePath::fromString(item.codeLocation().filePath()),
                         item.codeLocation().line());

}

QString QbsProject::uniqueProductName(const qbs::ProductData &product)
{
    return product.name() + QLatin1Char('.') + product.multiplexConfigurationId();
}

void QbsProject::configureAsExampleProject()
{
    QList<BuildInfo> infoList;
    const QList<Kit *> kits = KitManager::kits();
    for (Kit *k : kits) {
        if (QtSupport::QtKitAspect::qtVersion(k) != nullptr) {
            if (auto factory = BuildConfigurationFactory::find(k, projectFilePath()))
                infoList << factory->allAvailableSetups(k, projectFilePath());
        }
    }
    setup(infoList);
    if (activeTarget())
        static_cast<QbsBuildSystem *>(activeTarget()->buildSystem())->prepareForParsing();
}

void QbsBuildSystem::prepareForParsing()
{
    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
    }
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = nullptr;

    m_qbsUpdateFutureInterface = new QFutureInterface<bool>();
    m_qbsUpdateFutureInterface->setProgressRange(0, 0);
    ProgressManager::addTask(m_qbsUpdateFutureInterface->future(),
        tr("Reading Project \"%1\"").arg(project()->displayName()), "Qbs.QbsEvaluate");
    m_qbsUpdateFutureInterface->reportStarted();
}

void QbsBuildSystem::updateDocuments(const std::set<QString> &files)
{
    OpTimer opTimer("updateDocuments");

    const QVector<FilePath> filePaths = transform<QVector>(files, &FilePath::fromString);

    const FilePath buildDir = FilePath::fromString(m_projectData.buildDirectory());
    const QVector<FilePath> nonBuildDirFilePaths = filtered(filePaths,
                                                            [buildDir](const FilePath &p) {
                                                                return !p.isChildOf(buildDir);
                                                            });
    project()->setExtraProjectFiles(nonBuildDirFilePaths);
}

static QString getMimeType(const qbs::ArtifactData &sourceFile)
{
    if (sourceFile.fileTags().contains("hpp")) {
        if (CppTools::ProjectFile::isAmbiguousHeader(sourceFile.filePath()))
            return QString(CppTools::Constants::AMBIGUOUS_HEADER_MIMETYPE);
        return QString(CppTools::Constants::CPP_HEADER_MIMETYPE);
    }
    if (sourceFile.fileTags().contains("cpp"))
        return QString(CppTools::Constants::CPP_SOURCE_MIMETYPE);
    if (sourceFile.fileTags().contains("c"))
        return QString(CppTools::Constants::C_SOURCE_MIMETYPE);
    if (sourceFile.fileTags().contains("objc"))
        return QString(CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE);
    if (sourceFile.fileTags().contains("objcpp"))
        return QString(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE);
    return {};
}

static QString groupLocationToCallGroupId(const qbs::CodeLocation &location)
{
    return QString::fromLatin1("%1:%2:%3")
                        .arg(location.filePath())
                        .arg(location.line())
                        .arg(location.column());
}

// TODO: Receive the values from qbs when QBS-1030 is resolved.
static void getExpandedCompilerFlags(QStringList &cFlags, QStringList &cxxFlags,
                                     const qbs::PropertyMap &properties)
{
    const auto getCppProp = [properties](const char *propertyName) {
        return properties.getModuleProperty("cpp", QLatin1String(propertyName));
    };
    const QVariant &enableExceptions = getCppProp("enableExceptions");
    const QVariant &enableRtti = getCppProp("enableRtti");
    QStringList commonFlags = getCppProp("platformCommonCompilerFlags").toStringList();
    commonFlags << getCppProp("commonCompilerFlags").toStringList()
                << getCppProp("platformDriverFlags").toStringList()
                << getCppProp("driverFlags").toStringList();
    const QStringList toolchain = properties.getModulePropertiesAsStringList("qbs", "toolchain");
    if (toolchain.contains("gcc")) {
        bool hasTargetOption = false;
        if (toolchain.contains("clang")) {
            const int majorVersion = getCppProp("compilerVersionMajor").toInt();
            const int minorVersion = getCppProp("compilerVersionMinor").toInt();
            if (majorVersion > 3 || (majorVersion == 3 && minorVersion >= 1))
                hasTargetOption = true;
        }
        if (hasTargetOption) {
            commonFlags << "-target" << getCppProp("target").toString();
        } else {
            const QString targetArch = getCppProp("targetArch").toString();
            if (targetArch == "x86_64")
                commonFlags << "-m64";
            else if (targetArch == "i386")
                commonFlags << "-m32";
            const QString machineType = getCppProp("machineType").toString();
            if (!machineType.isEmpty())
                commonFlags << ("-march=" + machineType);
        }
        const QStringList targetOS = properties.getModulePropertiesAsStringList(
                    "qbs", "targetOS");
        if (targetOS.contains("unix")) {
            const QVariant positionIndependentCode = getCppProp("positionIndependentCode");
            if (!positionIndependentCode.isValid() || positionIndependentCode.toBool())
                commonFlags << "-fPIC";
        }
        cFlags = cxxFlags = commonFlags;

        const auto cxxLanguageVersion = getCppProp("cxxLanguageVersion").toStringList();
        if (cxxLanguageVersion.contains("c++17"))
            cxxFlags << "-std=c++17";
        else if (cxxLanguageVersion.contains("c++14"))
            cxxFlags << "-std=c++14";
        else if (cxxLanguageVersion.contains("c++11"))
            cxxFlags << "-std=c++11";
        else if (!cxxLanguageVersion.isEmpty())
            cxxFlags << ("-std=" + cxxLanguageVersion.first());
        const QString cxxStandardLibrary = getCppProp("cxxStandardLibrary").toString();
        if (!cxxStandardLibrary.isEmpty() && toolchain.contains("clang"))
            cxxFlags << ("-stdlib=" + cxxStandardLibrary);
        if (enableExceptions.isValid()) {
            cxxFlags << QLatin1String(enableExceptions.toBool()
                                      ? "-fexceptions" : "-fno-exceptions");
        }
        if (enableRtti.isValid())
            cxxFlags << QLatin1String(enableRtti.toBool() ? "-frtti" : "-fno-rtti");

        const auto cLanguageVersion = getCppProp("cLanguageVersion").toStringList();
        if (cLanguageVersion.contains("c11"))
            cFlags << "-std=c11";
        else if (cLanguageVersion.contains("c99"))
            cFlags << "-std=c99";
        else if (!cLanguageVersion.isEmpty())
            cFlags << ("-std=" + cLanguageVersion.first());

        if (targetOS.contains("darwin")) {
            const auto darwinVersion = getCppProp("minimumDarwinVersion").toString();
            if (!darwinVersion.isEmpty()) {
                const auto darwinVersionFlag = getCppProp("minimumDarwinVersionCompilerFlag")
                        .toString();
                if (!darwinVersionFlag.isEmpty())
                    cxxFlags << (darwinVersionFlag + '=' + darwinVersion);
            }
        }
    } else if (toolchain.contains("msvc")) {
        if (enableExceptions.toBool()) {
            const QString exceptionModel = getCppProp("exceptionHandlingModel").toString();
            if (exceptionModel == "default")
                commonFlags << "/EHsc";
            else if (exceptionModel == "seh")
                commonFlags << "/EHa";
            else if (exceptionModel == "externc")
                commonFlags << "/EHs";
        }
        cFlags = cxxFlags = commonFlags;
        cFlags << "/TC";
        cxxFlags << "/TP";
        if (enableRtti.isValid())
            cxxFlags << QLatin1String(enableRtti.toBool() ? "/GR" : "/GR-");
        if (getCppProp("cxxLanguageVersion").toStringList().contains("c++17"))
            cxxFlags << "/std:c++17";
    }
}

void QbsBuildSystem::updateCppCodeModel()
{
    OpTimer optimer("updateCppCodeModel");
    if (!m_projectData.isValid())
        return;

    const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();
    const auto factoriesBegin = factories.constBegin();
    const auto factoriesEnd = factories.constEnd();

    qDeleteAll(m_extraCompilers);
    m_extraCompilers.clear();

    QtSupport::CppKitInfo kitInfo(project());
    QTC_ASSERT(kitInfo.isValid(), return);

    RawProjectParts rpps;
    foreach (const qbs::ProductData &prd, m_projectData.allProducts()) {
        QString cPch;
        QString cxxPch;
        QString objcPch;
        QString objcxxPch;
        const auto &pchFinder = [&cPch, &cxxPch, &objcPch, &objcxxPch](const qbs::ArtifactData &a) {
            const QStringList fileTags = a.fileTags();
            if (fileTags.contains("c_pch_src"))
                cPch = a.filePath();
            if (fileTags.contains("cpp_pch_src"))
                cxxPch = a.filePath();
            if (fileTags.contains("objc_pch_src"))
                objcPch = a.filePath();
            if (fileTags.contains("objcpp_pch_src"))
                objcxxPch = a.filePath();
        };
        const QList<qbs::ArtifactData> &generatedArtifacts = prd.generatedArtifacts();
        std::for_each(generatedArtifacts.cbegin(), generatedArtifacts.cend(), pchFinder);
        foreach (const qbs::GroupData &grp, prd.groups()) {
            const QList<qbs::ArtifactData> &sourceArtifacts = grp.allSourceArtifacts();
            std::for_each(sourceArtifacts.cbegin(), sourceArtifacts.cend(), pchFinder);
        }

        const Utils::QtVersion qtVersionForPart
            = prd.moduleProperties().getModuleProperty("Qt.core", "version").isValid()
                  ? kitInfo.projectPartQtVersion
                  : Utils::QtVersion::None;

        foreach (const qbs::GroupData &grp, prd.groups()) {
            RawProjectPart rpp;
            rpp.setQtVersion(qtVersionForPart);
            const qbs::PropertyMap &props = grp.properties();
            rpp.setCallGroupId(groupLocationToCallGroupId(grp.location()));

            QStringList cFlags;
            QStringList cxxFlags;
            getExpandedCompilerFlags(cFlags, cxxFlags, props);
            rpp.setFlagsForC({kitInfo.cToolChain, cFlags});
            rpp.setFlagsForCxx({kitInfo.cxxToolChain, cxxFlags});

            QStringList list = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_DEFINES));
            rpp.setMacros(Utils::transform<QVector>(list, [](const QString &s) { return ProjectExplorer::Macro::fromKeyValue(s); }));

            ProjectExplorer::HeaderPaths grpHeaderPaths;
            list = props.getModulePropertiesAsStringList(CONFIG_CPP_MODULE, CONFIG_INCLUDEPATHS);
            list.removeDuplicates();
            for (const QString &p : qAsConst(list))
                grpHeaderPaths += {FilePath::fromUserInput(p).toString(),  HeaderPathType::User};

            list = props.getModulePropertiesAsStringList(CONFIG_CPP_MODULE,
                                                         CONFIG_SYSTEM_INCLUDEPATHS);

            list.removeDuplicates();
            for (const QString &p : qAsConst(list))
                grpHeaderPaths += {FilePath::fromUserInput(p).toString(),  HeaderPathType::System};

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_FRAMEWORKPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_FRAMEWORKPATHS)));
            list.removeDuplicates();
            foreach (const QString &p, list)
                grpHeaderPaths += {FilePath::fromUserInput(p).toString(), HeaderPathType::Framework};

            rpp.setHeaderPaths(grpHeaderPaths);

            rpp.setDisplayName(grp.name());
            rpp.setProjectFileLocation(grp.location().filePath(),
                                       grp.location().line(), grp.location().column());
            rpp.setBuildSystemTarget(QbsProject::uniqueProductName(prd));
            rpp.setBuildTargetType(prd.isRunnable() ? ProjectExplorer::BuildTargetType::Executable
                                                    : ProjectExplorer::BuildTargetType::Library);

            QHash<QString, qbs::ArtifactData> filePathToSourceArtifact;
            bool hasCFiles = false;
            bool hasCxxFiles = false;
            bool hasObjcFiles = false;
            bool hasObjcxxFiles = false;
            foreach (const qbs::ArtifactData &source, grp.allSourceArtifacts()) {
                filePathToSourceArtifact.insert(source.filePath(), source);

                foreach (const QString &tag, source.fileTags()) {
                    if (tag == "c")
                        hasCFiles = true;
                    else if (tag == "cpp")
                        hasCxxFiles = true;
                    else if (tag == "objc")
                        hasObjcFiles = true;
                    else if (tag == "objcpp")
                        hasObjcxxFiles = true;
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

                        const FilePathList fileNames = Utils::transform(generated,
                                                                        [](const QString &s) {
                            return Utils::FilePath::fromString(s);
                        });
                        m_extraCompilers.append((*i)->create(
                                project(), FilePath::fromString(source.filePath()), fileNames));
                    }
                }
            }

            QSet<QString> pchFiles;
            if (hasCFiles && props.getModuleProperty("cpp", "useCPrecompiledHeader").toBool()
                    && !cPch.isEmpty()) {
                pchFiles << cPch;
            }
            if (hasCxxFiles && props.getModuleProperty("cpp", "useCxxPrecompiledHeader").toBool()
                    && !cxxPch.isEmpty()) {
                pchFiles << cxxPch;
            }
            if (hasObjcFiles && props.getModuleProperty("cpp", "useObjcPrecompiledHeader").toBool()
                    && !objcPch.isEmpty()) {
                pchFiles << objcPch;
            }
            if (hasObjcxxFiles
                    && props.getModuleProperty("cpp", "useObjcxxPrecompiledHeader").toBool()
                    && !objcxxPch.isEmpty()) {
                pchFiles << objcxxPch;
            }
            if (pchFiles.count() > 1) {
                qCWarning(qbsPmLog) << "More than one pch file enabled for source files in group"
                                    << grp.name() << "in product" << prd.name();
                qCWarning(qbsPmLog) << "Expect problems with code model";
            }
            rpp.setPreCompiledHeaders(Utils::toList(pchFiles));
            rpp.setFiles(grp.allFilePaths(),
                         {},
                         [filePathToSourceArtifact](const QString &filePath) {
                             // Keep this lambda thread-safe!
                             return getMimeType(filePathToSourceArtifact.value(filePath));
                         });

            rpps.append(rpp);

        }
    }

    CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
    m_cppCodeModelUpdater->update({project(), kitInfo, activeParseEnvironment(), rpps});
}

void QbsBuildSystem::updateQmlJsCodeModel()
{
    OpTimer optimer("updateQmlJsCodeModel");
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(project());
    foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
        static const QString propertyName = QLatin1String("qmlImportPaths");
        foreach (const QString &path, product.properties().value(propertyName).toStringList()) {
            projectInfo.importPaths.maybeInsert(Utils::FilePath::fromString(path),
                                                QmlJS::Dialect::Qml);
        }
    }

    project()->setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID,
                       !projectInfo.sourceFiles.isEmpty());
    modelManager->updateProjectInfo(projectInfo, project());
}

void QbsBuildSystem::updateApplicationTargets()
{
    QList<BuildTargetInfo> applications;
    foreach (const qbs::ProductData &productData, m_projectData.allProducts()) {
        if (!productData.isEnabled() || !productData.isRunnable())
            continue;
        const bool isQtcRunnable = productData.properties().value("qtcRunnable").toBool();
        const bool usesTerminal = productData.properties().value("consoleApplication").toBool();
        const QString projectFile = productData.location().filePath();
        QString targetFile;
        foreach (const qbs::ArtifactData &ta, productData.targetArtifacts()) {
            QTC_ASSERT(ta.isValid(), continue);
            if (ta.isExecutable()) {
                targetFile = ta.filePath();
                break;
            }
        }

        BuildTargetInfo bti;
        bti.buildKey = QbsProject::uniqueProductName(productData);
        bti.targetFilePath = FilePath::fromString(targetFile);
        bti.projectFilePath = FilePath::fromString(projectFile);
        bti.isQtcRunnable = isQtcRunnable; // Fixed up below.
        bti.usesTerminal = usesTerminal;
        bti.displayName = productData.fullDisplayName();
        bti.runEnvModifier = [targetFile, productData, this](Utils::Environment &env, bool usingLibraryPaths) {
            if (!qbsProject().isValid())
                return;

            const QString key = env.toStringList().join(QChar())
                    + QbsProject::uniqueProductName(productData)
                    + QString::number(usingLibraryPaths);
            const auto it = m_envCache.constFind(key);
            if (it != m_envCache.constEnd()) {
                env = it.value();
                return;
            }

            QProcessEnvironment procEnv = env.toProcessEnvironment();
            procEnv.insert(QLatin1String("QBS_RUN_FILE_PATH"), targetFile);
            QStringList setupRunEnvConfig;
            if (!usingLibraryPaths)
                setupRunEnvConfig << QLatin1String("ignore-lib-dependencies");
            qbs::RunEnvironment qbsRunEnv = qbsProject().getRunEnvironment(productData,
                    qbs::InstallOptions(), procEnv, setupRunEnvConfig, QbsManager::settings());
            qbs::ErrorInfo error;
            procEnv = qbsRunEnv.runEnvironment(&error);
            if (error.hasError()) {
                Core::MessageManager::write(tr("Error retrieving run environment: %1")
                                            .arg(error.toString()));
            }
            if (!procEnv.isEmpty()) {
                env = Utils::Environment();
                foreach (const QString &key, procEnv.keys())
                    env.set(key, procEnv.value(key));
            }

            m_envCache.insert(key, env);
        };

        applications.append(bti);
    }
    setApplicationTargets(applications);
}

void QbsBuildSystem::updateDeploymentInfo()
{
    DeploymentData deploymentData;
    if (m_qbsProject.isValid()) {
        foreach (const qbs::ArtifactData &f, m_projectData.installableArtifacts()) {
            deploymentData.addFile(f.filePath(), f.installData().installDir(),
                    f.isExecutable() ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
        }
    }
    deploymentData.setLocalInstallRoot(installRoot());
    setDeploymentData(deploymentData);
}

void QbsBuildSystem::updateBuildTargetData()
{
    OpTimer optimer("updateBuildTargetData");
    updateApplicationTargets();
    updateDeploymentInfo();
}

} // namespace Internal
} // namespace QbsProjectManager
