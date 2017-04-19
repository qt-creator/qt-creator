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
#include "qbsprojectparser.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsnodes.h"
#include "qbsnodetreebuilder.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectupdater.h>
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
#include <QElapsedTimer>
#include <QFileInfo>
#include <QMessageBox>
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

QbsProject::QbsProject(const FileName &fileName) :
    Project(Constants::MIME_TYPE, fileName, [this]() { delayParsing(); }),
    m_qbsProjectParser(0),
    m_qbsUpdateFutureInterface(0),
    m_parsingScheduled(false),
    m_cancelStatus(CancelStatusNone),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater(this)),
    m_currentBc(0),
    m_extraCompilersPending(false)
{
    m_parsingDelay.setInterval(1000); // delay parsing by 1s.

    setId(Constants::PROJECT_ID);

    setProjectContext(Context(Constants::PROJECT_ID));
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));

    setDisplayName(fileName.toFileInfo().completeBaseName());

    connect(this, &Project::activeTargetChanged, this, &QbsProject::changeActiveTarget);
    connect(this, &Project::addedTarget, this, &QbsProject::targetWasAdded);
    connect(this, &Project::removedTarget, this, &QbsProject::targetWasRemoved);
    connect(this, &Project::environmentChanged, this, &QbsProject::delayParsing);

    connect(&m_parsingDelay, &QTimer::timeout, this, &QbsProject::startParsing);

    connect(m_cppCodeModelUpdater, &CppTools::CppProjectUpdater::projectInfoUpdated, this,
            [this](const CppTools::ProjectInfo &projectInfo){
        m_cppCodeModelProjectInfo = projectInfo;
    });
}

QbsProject::~QbsProject()
{
    delete m_cppCodeModelUpdater;
    delete m_qbsProjectParser;
    if (m_qbsUpdateFutureInterface) {
        m_qbsUpdateFutureInterface->reportCanceled();
        m_qbsUpdateFutureInterface->reportFinished();
        delete m_qbsUpdateFutureInterface;
        m_qbsUpdateFutureInterface = 0;
    }
    qDeleteAll(m_extraCompilers);
}

QbsRootProjectNode *QbsProject::rootProjectNode() const
{
    return static_cast<QbsRootProjectNode *>(Project::rootProjectNode());
}

void QbsProject::projectLoaded()
{
    m_parsingDelay.start(0);
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
                                     tr("Failed"),
                                     tr("Could not write project file %1.").arg(file));
                return false;
            }
        }
    }
    return true;
}

bool QbsProject::addFilesToProduct(const QStringList &filePaths,
                                   const qbs::ProductData &productData,
                                   const qbs::GroupData &groupData, QStringList *notAdded)
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
        setRootProjectNode(Internal::QbsNodeTreeBuilder::buildTree(this));
    }
    return notAdded->isEmpty();
}

bool QbsProject::removeFilesFromProduct(const QStringList &filePaths,
                                        const qbs::ProductData &productData,
                                        const qbs::GroupData &groupData,
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
        setRootProjectNode(Internal::QbsNodeTreeBuilder::buildTree(this));
        emit fileListChanged();
    }
    return notRemoved->isEmpty();
}

bool QbsProject::renameFileInProduct(const QString &oldPath, const QString &newPath,
                                     const qbs::ProductData &productData,
                                     const qbs::GroupData &groupData)
{
    if (newPath.isEmpty())
        return false;
    QStringList dummy;
    if (!removeFilesFromProduct(QStringList(oldPath), productData, groupData, &dummy))
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

    return addFilesToProduct(QStringList() << newPath, newProductData, newGroupData, &dummy);
}

void QbsProject::invalidate()
{
    prepareForParsing();
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
qbs::AbstractJob *QbsProject::buildOrClean(const Options &opts, const QStringList &productNames,
                                           QString &error)
{
    QTC_ASSERT(qbsProject().isValid(), return 0);
    QTC_ASSERT(!isParsing(), return 0);

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
            const bool cleaningRequested = std::is_same<Options, qbs::CleanOptions>::value;
            error = tr("%1: Selected products do not exist anymore.")
                    .arg(cleaningRequested ? tr("Cannot clean") : tr("Cannot build"));
            return nullptr;
        }
    }
    return doBuildOrClean(qbsProject(), products, opts);
}

qbs::BuildJob *QbsProject::build(const qbs::BuildOptions &opts, QStringList productNames,
                                 QString &error)
{
    return static_cast<qbs::BuildJob *>(buildOrClean(opts, productNames, error));
}

qbs::CleanJob *QbsProject::clean(const qbs::CleanOptions &opts, const QStringList &productNames,
                                 QString &error)
{
    return static_cast<qbs::CleanJob *>(buildOrClean(opts, productNames, error));
}

qbs::InstallJob *QbsProject::install(const qbs::InstallOptions &opts)
{
    if (!qbsProject().isValid())
        return 0;
    return qbsProject().installAllProducts(opts);
}

QString QbsProject::profileForTarget(const Target *t) const
{
    return QbsManager::profileForKit(t->kit());
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

static QSet<QString> toQStringSet(const std::set<QString> &src)
{
    QSet<QString> result;
    result.reserve(int(src.size()));
    std::copy(src.begin(), src.end(), Utils::inserter(result));
    return result;
}

void QbsProject::updateAfterParse()
{
    qCDebug(qbsPmLog) << "Updating data after parse";
    OpTimer opTimer("updateAfterParse");
    updateProjectNodes();
    updateDocuments(toQStringSet(m_qbsProject.buildSystemFiles()));
    updateBuildTargetData();
    updateCppCodeModel();
    updateQmlJsCodeModel();
    emit fileListChanged();
}

void QbsProject::updateProjectNodes()
{
    OpTimer opTimer("updateProjectNodes");
    setRootProjectNode(Internal::QbsNodeTreeBuilder::buildTree(this));
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
    m_qbsProjects.insert(activeTarget(), m_qbsProject);
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

    m_qbsProjectParser->deleteLater();
    m_qbsProjectParser = 0;
    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    if (dataChanged)
        updateAfterParse();
    emit projectParsingDone(success);
    emit parsingFinished();
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
    m_qbsProjects.insert(t, qbs::Project());
    connect(t, &Target::activeBuildConfigurationChanged, this, &QbsProject::delayParsing);
    connect(t, &Target::buildDirectoryChanged, this, &QbsProject::delayParsing);
}

void QbsProject::targetWasRemoved(Target *t)
{
    m_qbsProjects.remove(t);
}

void QbsProject::changeActiveTarget(Target *t)
{
    BuildConfiguration *bc = 0;
    if (t) {
        m_qbsProject = m_qbsProjects.value(t);
        if (t->kit())
            bc = t->activeBuildConfiguration();
    }
    buildConfigurationChanged(bc);
}

void QbsProject::buildConfigurationChanged(BuildConfiguration *bc)
{
    if (m_currentBc)
        disconnect(m_currentBc, &QbsBuildConfiguration::qbsConfigurationChanged,
                   this, &QbsProject::delayParsing);

    m_currentBc = qobject_cast<QbsBuildConfiguration *>(bc);
    if (m_currentBc) {
        connect(m_currentBc, &QbsBuildConfiguration::qbsConfigurationChanged,
                this, &QbsProject::delayParsing);
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
    OpTimer opTimer("updateAfterBuild");
    QTC_ASSERT(m_qbsProject.isValid(), return);
    const qbs::ProjectData &projectData = m_qbsProject.projectData();
    if (projectData == m_projectData)
        return;
    qCDebug(qbsPmLog) << "Updating data after build";
    m_projectData = projectData;
    updateProjectNodes();
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
        connect(m_qbsProjectParser, &QbsProjectParser::done,
                this, &QbsProject::handleQbsParsingDone);
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
    OpTimer opTimer("updateDocuments");
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
        toAdd.insert(new ProjectDocument(Constants::MIME_TYPE, FileName::fromString(f),
                                         [this]() { delayParsing(); }));

    m_qbsDocuments.unite(toAdd);
}

static CppTools::ProjectFile::Kind cppFileType(const qbs::ArtifactData &sourceFile)
{
    if (sourceFile.fileTags().contains(QLatin1String("hpp"))) {
        if (CppTools::ProjectFile::isAmbiguousHeader(sourceFile.filePath()))
            return CppTools::ProjectFile::AmbiguousHeader;
        return CppTools::ProjectFile::CXXHeader;
    }
    if (sourceFile.fileTags().contains(QLatin1String("cpp")))
        return CppTools::ProjectFile::CXXSource;
    if (sourceFile.fileTags().contains(QLatin1String("c")))
        return CppTools::ProjectFile::CSource;
    if (sourceFile.fileTags().contains(QLatin1String("objc")))
        return CppTools::ProjectFile::ObjCSource;
    if (sourceFile.fileTags().contains(QLatin1String("objcpp")))
        return CppTools::ProjectFile::ObjCXXSource;
    return CppTools::ProjectFile::Unsupported;
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

        const QString cxxLanguageVersion = getCppProp("cxxLanguageVersion").toString();
        if (cxxLanguageVersion == "c++11")
            cxxFlags << "-std=c++0x";
        else if (cxxLanguageVersion == "c++14")
            cxxFlags << "-std=c++1y";
        else if (!cxxLanguageVersion.isEmpty())
            cxxFlags << ("-std=" + cxxLanguageVersion);
        const QString cxxStandardLibrary = getCppProp("cxxStandardLibrary").toString();
        if (!cxxStandardLibrary.isEmpty() && toolchain.contains("clang"))
            cxxFlags << ("-stdlib=" + cxxStandardLibrary);
        if (enableExceptions.isValid()) {
            cxxFlags << QLatin1String(enableExceptions.toBool()
                                      ? "-fexceptions" : "-fno-exceptions");
        }
        if (enableRtti.isValid())
            cxxFlags << QLatin1String(enableRtti.toBool() ? "-frtti" : "-fno-rtti");

        const QString cLanguageVersion = getCppProp("cLanguageVersion").toString();
        if (cLanguageVersion == "c11")
            cFlags << "-std=c1x";
        else if (!cLanguageVersion.isEmpty())
            cFlags << ("-std=" + cLanguageVersion);
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
    }
}

void QbsProject::updateCppCodeModel()
{
    OpTimer optimer("updateCppCodeModel");
    if (!m_projectData.isValid())
        return;

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

    QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit());

    CppTools::ProjectPart::QtVersion qtVersionFromKit = CppTools::ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionFromKit = CppTools::ProjectPart::Qt4;
        else
            qtVersionFromKit = CppTools::ProjectPart::Qt5;
    }

    QList<ProjectExplorer::ExtraCompilerFactory *> factories =
            ProjectExplorer::ExtraCompilerFactory::extraCompilerFactories();
    const auto factoriesBegin = factories.constBegin();
    const auto factoriesEnd = factories.constEnd();

    qDeleteAll(m_extraCompilers);
    m_extraCompilers.clear();

    CppTools::RawProjectParts rpps;
    foreach (const qbs::ProductData &prd, m_projectData.allProducts()) {
        QString cPch;
        QString cxxPch;
        QString objcPch;
        QString objcxxPch;
        const auto &pchFinder = [&cPch, &cxxPch, &objcPch, &objcxxPch](const qbs::ArtifactData &a) {
            if (a.fileTags().contains("c_pch_src"))
                cPch = a.filePath();
            else if (a.fileTags().contains("cpp_pch_src"))
                cxxPch = a.filePath();
            else if (a.fileTags().contains("objc_pch_src"))
                objcPch = a.filePath();
            else if (a.fileTags().contains("objcpp_pch_src"))
                objcxxPch = a.filePath();
        };
        const QList<qbs::ArtifactData> &generatedArtifacts = prd.generatedArtifacts();
        std::for_each(generatedArtifacts.cbegin(), generatedArtifacts.cend(), pchFinder);
        foreach (const qbs::GroupData &grp, prd.groups()) {
            const QList<qbs::ArtifactData> &sourceArtifacts = grp.allSourceArtifacts();
            std::for_each(sourceArtifacts.cbegin(), sourceArtifacts.cend(), pchFinder);
        }

        const CppTools::ProjectPart::QtVersion qtVersionForPart =
                 prd.moduleProperties().getModuleProperty("Qt.core", "version").isValid()
                    ? qtVersionFromKit
                    : CppTools::ProjectPart::NoQt;

        foreach (const qbs::GroupData &grp, prd.groups()) {
            CppTools::RawProjectPart rpp;
            rpp.setQtVersion(qtVersionForPart);
            const qbs::PropertyMap &props = grp.properties();
            rpp.setCallGroupId(groupLocationToCallGroupId(grp.location()));

            QStringList cFlags;
            QStringList cxxFlags;
            getExpandedCompilerFlags(cFlags, cxxFlags, props);
            rpp.setFlagsForC({cToolChain, cFlags});
            rpp.setFlagsForCxx({cxxToolChain, cxxFlags});

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
            rpp.setDefines(grpDefines);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_INCLUDEPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_INCLUDEPATHS)));
            list.removeDuplicates();
            CppTools::ProjectPartHeaderPaths grpHeaderPaths;
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPartHeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPartHeaderPath::IncludePath);

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_FRAMEWORKPATHS));
            list.append(props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                              QLatin1String(CONFIG_SYSTEM_FRAMEWORKPATHS)));
            list.removeDuplicates();
            foreach (const QString &p, list)
                grpHeaderPaths += CppTools::ProjectPartHeaderPath(
                            FileName::fromUserInput(p).toString(),
                            CppTools::ProjectPartHeaderPath::FrameworkPath);

            rpp.setHeaderPaths(grpHeaderPaths);

            rpp.setDisplayName(grp.name());
            rpp.setProjectFileLocation(grp.location().filePath(),
                                       grp.location().line(), grp.location().column());
            rpp.setBuildSystemTarget(uniqueProductName(prd));

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

                        const FileNameList fileNames = Utils::transform(generated,
                                                                        [](const QString &s) {
                            return Utils::FileName::fromString(s);
                        });
                        m_extraCompilers.append((*i)->create(
                                this, FileName::fromString(source.filePath()), fileNames));
                    }
                }
            }

            QStringList pchFiles;
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
            rpp.setPreCompiledHeaders(pchFiles);
            rpp.setFiles(grp.allFilePaths(), [filePathToSourceArtifact](const QString &filePath) {
                // Keep this lambda thread-safe!
                return cppFileType(filePathToSourceArtifact.value(filePath));
            });

            rpps.append(rpp);

        }
    }

    CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
    m_cppCodeModelUpdater->update({this, cToolChain, cxxToolChain, k, rpps});
}

void QbsProject::updateCppCompilerCallData()
{
    OpTimer optimer("updateCppCompilerCallData");
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    QTC_ASSERT(m_cppCodeModelProjectInfo == modelManager->projectInfo(this), return);

    CppTools::ProjectInfo::CompilerCallData data;
    foreach (const qbs::ProductData &product, m_projectData.allProducts()) {
        if (!product.isEnabled())
            continue;

        foreach (const qbs::GroupData &group, product.groups()) {
            if (!group.isEnabled())
                continue;

            CppTools::ProjectInfo::CompilerCallGroup compilerCallGroup;
            compilerCallGroup.groupId = groupLocationToCallGroupId(group.location());

            foreach (const qbs::ArtifactData &file, group.allSourceArtifacts()) {
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

    m_cppCodeModelProjectInfo = modelManager->updateCompilerCallDataForProject(this, data);
}

void QbsProject::updateQmlJsCodeModel()
{
    OpTimer optimer("updateQmlJsCodeModel");
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

    setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID,
                       !projectInfo.sourceFiles.isEmpty());
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
        foreach (const qbs::ArtifactData &ta, productData.targetArtifacts()) {
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
        foreach (const qbs::ArtifactData &f, m_projectData.installableArtifacts()) {
            deploymentData.addFile(f.filePath(), f.installData().installDir(),
                    f.isExecutable() ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal);
        }
    }
    activeTarget()->setDeploymentData(deploymentData);
}

void QbsProject::updateBuildTargetData()
{
    OpTimer optimer("updateBuildTargetData");
    updateApplicationTargets();
    updateDeploymentInfo();
    if (activeTarget())
        activeTarget()->updateDefaultRunConfigurations();
}

} // namespace Internal
} // namespace QbsProjectManager
