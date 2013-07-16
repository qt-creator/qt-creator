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

#include "qbsproject.h"

#include "qbsbuildconfiguration.h"
#include "qbslogsink.h"
#include "qbsprojectfile.h"
#include "qbsprojectmanagerconstants.h"
#include "qbsnodes.h"

#include <coreplugin/documentmanager.h>
#include <utils/qtcassert.h>

#include <language/language.h>

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/mimedatabase.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildenvironmentwidget.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>
#include <qtsupport/qtkitinformation.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qbs.h>
#include <tools/scripttools.h> // qbs, remove once there is a expand method in Qbs itself!

#include <QFileInfo>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CPP_MODULE[] = "cpp";
static const char CONFIG_CXXFLAGS[] = "cxxflags";
static const char CONFIG_CFLAGS[] = "cflags";
static const char CONFIG_DEFINES[] = "defines";
static const char CONFIG_INCLUDEPATHS[] = "includePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "frameworkPaths";
static const char CONFIG_PRECOMPILEDHEADER[] = "precompiledHeader";

static const char CONFIGURATION_PATH[] = "<configuration>";

// --------------------------------------------------------------------
// HELPERS:
// --------------------------------------------------------------------

ProjectExplorer::TaskHub *taskHub()
{
    return ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub();
}


namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsProject:
// --------------------------------------------------------------------

QbsProject::QbsProject(QbsManager *manager, const QString &fileName) :
    m_manager(manager),
    m_projectName(QFileInfo(fileName).completeBaseName()),
    m_fileName(fileName),
    m_rootProjectNode(0),
    m_qbsSetupProjectJob(0),
    m_qbsUpdateFutureInterface(0),
    m_currentProgressBase(0),
    m_forceParsing(false),
    m_currentBc(0)
{
    m_parsingDelay.setInterval(1000); // delay parsing by 1s.

    setProjectContext(Core::Context(Constants::PROJECT_ID));
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(changeActiveTarget(ProjectExplorer::Target*)));
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetWasAdded(ProjectExplorer::Target*)));
    connect(this, SIGNAL(environmentChanged()), this, SLOT(delayParsing()));

    connect(&m_parsingDelay, SIGNAL(timeout()), this, SLOT(parseCurrentBuildConfiguration()));

    updateDocuments(QSet<QString>() << fileName);
    m_rootProjectNode = new QbsProjectNode(this); // needs documents to be initialized!
}

QbsProject::~QbsProject()
{
    m_codeModelFuture.cancel();
}

QString QbsProject::displayName() const
{
    return m_projectName;
}

Core::Id QbsProject::id() const
{
    return Core::Id(Constants::PROJECT_ID);
}

Core::IDocument *QbsProject::document() const
{
    foreach (Core::IDocument *doc, m_qbsDocuments) {
        if (doc->fileName() == m_fileName)
            return doc;
    }
    QTC_ASSERT(false, return 0);
}

QbsManager *QbsProject::projectManager() const
{
    return m_manager;
}

ProjectExplorer::ProjectNode *QbsProject::rootProjectNode() const
{
    return m_rootProjectNode;
}

QStringList QbsProject::files(ProjectExplorer::Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    QSet<QString> result;
    if (m_rootProjectNode && m_rootProjectNode->qbsProjectData().isValid()) {
        foreach (const qbs::ProductData &prd, m_rootProjectNode->qbsProjectData().allProducts()) {
            foreach (const qbs::GroupData &grp, prd.groups()) {
                foreach (const QString &file, grp.allFilePaths())
                    result.insert(file);
                result.insert(grp.location().fileName());
            }
            result.insert(prd.location().fileName());
        }
        result.insert(m_rootProjectNode->qbsProjectData().location().fileName());
    }
    return result.toList();
}

void QbsProject::invalidate()
{
    prepareForParsing();
}

qbs::BuildJob *QbsProject::build(const qbs::BuildOptions &opts, QStringList productNames)
{
    if (!qbsProject() || isParsing())
        return 0;
    if (productNames.isEmpty()) {
        return qbsProject()->buildAllProducts(opts);
    } else {
        QList<qbs::ProductData> products;
        foreach (const QString &productName, productNames) {
            bool found = false;
            foreach (const qbs::ProductData &data, qbsProjectData().allProducts()) {
                if (data.name() == productName) {
                    found = true;
                    products.append(data);
                    break;
                }
            }
            if (!found)
                return 0;
        }

        return qbsProject()->buildSomeProducts(products, opts);
    }
}

qbs::CleanJob *QbsProject::clean(const qbs::CleanOptions &opts)
{
    if (!qbsProject())
        return 0;
    return qbsProject()->cleanAllProducts(opts);
}

qbs::InstallJob *QbsProject::install(const qbs::InstallOptions &opts)
{
    if (!qbsProject())
        return 0;
    return qbsProject()->installAllProducts(opts);
}

QString QbsProject::profileForTarget(const ProjectExplorer::Target *t) const
{
    return m_manager->profileForKit(t->kit());
}

bool QbsProject::isParsing() const
{
    return m_qbsUpdateFutureInterface;
}

bool QbsProject::hasParseResult() const
{
    return qbsProject();
}

Utils::FileName QbsProject::defaultBuildDirectory() const
{
    QFileInfo fi(m_fileName);
    const QString buildDir = QDir(fi.canonicalPath()).absoluteFilePath(QString::fromLatin1("../%1-build").arg(fi.baseName()));
    return Utils::FileName::fromString(buildDir);
}

const qbs::Project *QbsProject::qbsProject() const
{
    if (!m_rootProjectNode)
        return 0;
    return m_rootProjectNode->qbsProject();
}

const qbs::ProjectData QbsProject::qbsProjectData() const
{
    if (!m_rootProjectNode)
        return qbs::ProjectData();
    return m_rootProjectNode->qbsProjectData();
}

bool QbsProject::needsSpecialDeployment() const
{
    return true;
}

void QbsProject::handleQbsParsingDone(bool success)
{
    QTC_ASSERT(m_qbsSetupProjectJob, return);
    QTC_ASSERT(m_qbsUpdateFutureInterface, return);

    qbs::Project *project = 0;
    if (success) {
        project = new qbs::Project(m_qbsSetupProjectJob->project());
    } else {
        generateErrors(m_qbsSetupProjectJob->error());
        m_qbsUpdateFutureInterface->reportCanceled();
    }
    m_qbsSetupProjectJob->deleteLater();
    m_qbsSetupProjectJob = 0;

    m_qbsUpdateFutureInterface->reportFinished();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    m_rootProjectNode->update(project);

    updateDocuments(project ? project->buildSystemFiles() : QSet<QString>() << m_fileName);

    updateCppCodeModel(m_rootProjectNode->qbsProjectData());
    updateQmlJsCodeModel(m_rootProjectNode->qbsProjectData());

    foreach (ProjectExplorer::Target *t, targets())
        t->updateDefaultRunConfigurations();

    emit fileListChanged();
    emit projectParsingDone(success);
}

void QbsProject::handleQbsParsingProgress(int progress)
{
    if (m_qbsUpdateFutureInterface)
        m_qbsUpdateFutureInterface->setProgressValue(m_currentProgressBase + progress);
}

void QbsProject::handleQbsParsingTaskSetup(const QString &description, int maximumProgressValue)
{
    Q_UNUSED(description);
    if (m_qbsUpdateFutureInterface) {
        m_currentProgressBase = m_qbsUpdateFutureInterface->progressValue();
        m_qbsUpdateFutureInterface->setProgressRange(0, m_currentProgressBase + maximumProgressValue);
    }
}

void QbsProject::targetWasAdded(ProjectExplorer::Target *t)
{
    connect(t, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(delayForcedParsing()));
    connect(t, SIGNAL(buildDirectoryChanged()), this, SLOT(delayForcedParsing()));
}

void QbsProject::changeActiveTarget(ProjectExplorer::Target *t)
{
    ProjectExplorer::BuildConfiguration *bc = 0;
    if (t && t->kit())
        bc = t->activeBuildConfiguration();
    buildConfigurationChanged(bc);
}

void QbsProject::buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc)
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

void QbsProject::delayParsing()
{
    m_parsingDelay.start();
}

void QbsProject::delayForcedParsing()
{
    m_forceParsing = true;
    delayParsing();
}

void QbsProject::parseCurrentBuildConfiguration()
{
    m_parsingDelay.stop();

    if (!activeTarget())
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;
    parse(bc->qbsConfiguration(), bc->environment(), bc->buildDirectory());
}

bool QbsProject::fromMap(const QVariantMap &map)
{
    if (!Project::fromMap(map))
        return false;

    ProjectExplorer::KitManager *km = ProjectExplorer::KitManager::instance();
    if (!activeTarget() && km->defaultKit()) {
        ProjectExplorer::Target *t = new ProjectExplorer::Target(this, km->defaultKit());
        t->updateDefaultBuildConfigurations();
        t->updateDefaultDeployConfigurations();
        t->updateDefaultRunConfigurations();
        addTarget(t);
    }

    return true;
}

void QbsProject::generateErrors(const qbs::ErrorInfo &e)
{
    foreach (const qbs::ErrorItem &item, e.items())
        taskHub()->addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                                 item.description(),
                                                 Utils::FileName::fromString(item.codeLocation().fileName()),
                                                 item.codeLocation().line(),
                                                 ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void QbsProject::parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir)
{
    QTC_ASSERT(!dir.isNull(), return);

    // Clear buildsystem related tasks:
    ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub()
            ->clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    qbs::SetupProjectParameters params;
    params.setBuildConfiguration(config);
    qbs::ErrorInfo err = params.expandBuildConfiguration(m_manager->settings());
    if (err.hasError()) {
        generateErrors(err);
        return;
    }

    // Avoid useless reparsing:
    const qbs::Project *currentProject = qbsProject();
    if (!m_forceParsing
            && currentProject
            && currentProject->projectConfiguration() == params.buildConfiguration()) {
        QHash<QString, QString> usedEnv = currentProject->usedEnvironment();
        bool canSkip = true;
        for (QHash<QString, QString>::const_iterator i = usedEnv.constBegin();
             i != usedEnv.constEnd(); ++i) {
            if (env.value(i.key()) != i.value()) {
                canSkip = false;
                break;
            }
        }
        if (canSkip)
            return;
    }

    params.setBuildRoot(dir);
    params.setProjectFilePath(m_fileName);
    params.setIgnoreDifferentProjectFilePath(false);
    params.setEnvironment(env.toProcessEnvironment());
    qbs::Preferences *prefs = QbsManager::preferences();
    const QString buildDir = qbsBuildDir();
    params.setSearchPaths(prefs->searchPaths(buildDir));
    params.setPluginPaths(prefs->pluginPaths(buildDir));

    // Do the parsing:
    prepareForParsing();
    QTC_ASSERT(!m_qbsSetupProjectJob, return);

    m_qbsSetupProjectJob
            = qbs::Project::setupProject(params, m_manager->logSink(), 0);

    connect(m_qbsSetupProjectJob, SIGNAL(finished(bool,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingDone(bool)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingTaskSetup(QString,int)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingProgress(int)));

    emit projectParsingStarted();
}

void QbsProject::prepareForParsing()
{
    m_forceParsing = false;

    taskHub()->clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_COMPILE);
    if (m_qbsUpdateFutureInterface)
        m_qbsUpdateFutureInterface->reportCanceled();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    delete m_qbsSetupProjectJob;
    m_qbsSetupProjectJob = 0;

    m_currentProgressBase = 0;
    m_qbsUpdateFutureInterface = new QFutureInterface<void>();
    m_qbsUpdateFutureInterface->setProgressRange(0, 0);
    Core::ICore::progressManager()->addTask(m_qbsUpdateFutureInterface->future(), tr("Evaluating"),
                                            QLatin1String(Constants::QBS_EVALUATE));
    m_qbsUpdateFutureInterface->reportStarted();
}

void QbsProject::updateDocuments(const QSet<QString> &files)
{
    // Update documents:
    QSet<QString> newFiles = files;
    QTC_ASSERT(!newFiles.isEmpty(), newFiles << m_fileName);
    QSet<QString> oldFiles;
    foreach (Core::IDocument *doc, m_qbsDocuments)
        oldFiles.insert(doc->fileName());

    QSet<QString> filesToAdd = newFiles;
    filesToAdd.subtract(oldFiles);
    QSet<QString> filesToRemove = oldFiles;
    filesToRemove.subtract(newFiles);

    QSet<Core::IDocument *> currentDocuments = m_qbsDocuments;
    foreach (Core::IDocument *doc, currentDocuments) {
        if (filesToRemove.contains(doc->fileName())) {
            m_qbsDocuments.remove(doc);
            delete doc;
        }
    }
    QSet<Core::IDocument *> toAdd;
    foreach (const QString &f, filesToAdd)
        toAdd.insert(new QbsProjectFile(this, f));

    Core::DocumentManager::instance()->addDocuments(toAdd.toList());
    m_qbsDocuments.unite(toAdd);
}

void QbsProject::updateCppCodeModel(const qbs::ProjectData &prj)
{
    if (!prj.isValid())
        return;

    ProjectExplorer::Kit *k = 0;
    QtSupport::BaseQtVersion *qtVersion = 0;
    if (ProjectExplorer::Target *target = activeTarget())
        k = target->kit();
    else
        k = ProjectExplorer::KitManager::instance()->defaultKit();
    qtVersion = QtSupport::QtKitInformation::qtVersion(k);

    CppTools::CppModelManagerInterface *modelmanager =
        CppTools::CppModelManagerInterface::instance();

    if (!modelmanager)
        return;

    CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
    pinfo.clearProjectParts();
    CppTools::ProjectPart::QtVersion qtVersionForPart
            = CppTools::ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = CppTools::ProjectPart::Qt4;
        else
            qtVersionForPart = CppTools::ProjectPart::Qt5;
    }

    QStringList allFiles;
    foreach (const qbs::ProductData &prd, prj.allProducts()) {
        foreach (const qbs::GroupData &grp, prd.groups()) {
            const qbs::PropertyMap &props = grp.properties();

            const QStringList cxxFlags = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_CXXFLAGS));

            const QStringList cFlags = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_CFLAGS));

            QStringList list = props.getModulePropertiesAsStringList(
                        QLatin1String(CONFIG_CPP_MODULE),
                        QLatin1String(CONFIG_DEFINES));
            QByteArray grpDefines;
            foreach (const QString &def, list) {
                QByteArray data = def.toUtf8();
                int pos = data.indexOf('=');
                if (pos >= 0)
                    data[pos] = ' ';
                grpDefines += (QByteArray("#define ") + data + '\n');
            }

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_INCLUDEPATHS));
            QStringList grpIncludePaths;
            foreach (const QString &p, list) {
                const QString cp = Utils::FileName::fromUserInput(p).toString();
                grpIncludePaths.append(cp);
            }

            list = props.getModulePropertiesAsStringList(QLatin1String(CONFIG_CPP_MODULE),
                                                         QLatin1String(CONFIG_FRAMEWORKPATHS));
            QStringList grpFrameworkPaths;
            foreach (const QString &p, list) {
                const QString cp = Utils::FileName::fromUserInput(p).toString();
                grpFrameworkPaths.append(cp);
            }

            const QString pch = props.getModuleProperty(QLatin1String(CONFIG_CPP_MODULE),
                    QLatin1String(CONFIG_PRECOMPILEDHEADER)).toString();

            CppTools::ProjectPart::Ptr part(new CppTools::ProjectPart);
            part->evaluateToolchain(ProjectExplorer::ToolChainKitInformation::toolChain(k),
                                    cxxFlags,
                                    cFlags,
                                    ProjectExplorer::SysRootKitInformation::sysRoot(k));

            CppTools::ProjectFileAdder adder(part->files);
            foreach (const QString &file, grp.allFilePaths())
                if (adder.maybeAdd(file))
                    allFiles.append(file);
            part->files << CppTools::ProjectFile(QLatin1String(CONFIGURATION_PATH),
                                                  CppTools::ProjectFile::CXXHeader);

            part->qtVersion = qtVersionForPart;
            part->includePaths += grpIncludePaths;
            part->frameworkPaths += grpFrameworkPaths;
            part->precompiledHeaders = QStringList(pch);
            part->defines += grpDefines;
            pinfo.appendProjectPart(part);
        }
    }

    setProjectLanguage(ProjectExplorer::Constants::LANG_CXX, !allFiles.isEmpty());

    if (pinfo.projectParts().isEmpty())
        return;

    // Register update the code model:
    m_codeModelFuture = modelmanager->updateProjectInfo(pinfo);
}

void QbsProject::updateQmlJsCodeModel(const qbs::ProjectData &prj)
{
    Q_UNUSED(prj);
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            QmlJSTools::defaultProjectInfoForProject(this);

    setProjectLanguage(ProjectExplorer::Constants::LANG_QMLJS, !projectInfo.sourceFiles.isEmpty());
    modelManager->updateProjectInfo(projectInfo);
}

QString QbsProject::qbsBuildDir() const
{
    QString buildDir = Utils::Environment::systemEnvironment()
            .value(QLatin1String("QBS_BUILD_DIR"));
    if (buildDir.isEmpty())
        buildDir = Core::ICore::resourcePath() + QLatin1String("/qbs");
    return buildDir;
}

} // namespace Internal
} // namespace QbsProjectManager
