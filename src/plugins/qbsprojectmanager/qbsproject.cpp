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
#include <qtsupport/qtkitinformation.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qbs.h>
#include <tools/scripttools.h> // qbs, remove once there is a expand method in Qbs itself!

#include <QFileInfo>

// --------------------------------------------------------------------
// Constants:
// --------------------------------------------------------------------

static const char CONFIG_CXXFLAGS[] = "cpp.cxxflags";
static const char CONFIG_DEFINES[] = "cpp.defines";
static const char CONFIG_INCLUDEPATHS[] = "cpp.includePaths";
static const char CONFIG_FRAMEWORKPATHS[] = "cpp.frameworkPaths";
static const char CONFIG_PRECOMPILEDHEADER[] = "modules.cpp.precompiledHeader";

static const char CONFIGURATION_PATH[] = "<configuration>";

// --------------------------------------------------------------------
// HELPERS:
// --------------------------------------------------------------------

// FIXME: All this should be in QBS! They do the same thing in JS.

static const char MODULES_KEY[] = "modules";

ProjectExplorer::TaskHub *taskHub()
{
    return ProjectExplorer::ProjectExplorerPlugin::instance()->taskHub();
}


static QVariant extract(const QVariantMap &data, const QString &key)
{
    QStringList keyParts = key.split(QLatin1Char('.'), QString::SkipEmptyParts);
    return qbs::Internal::getConfigProperty(data, keyParts);
}

static void expand(const QVariant &v, QStringList &partial, QSet<QString> &seenSet)
{
    if (v.isNull())
        return;

    QStringList tokenList;
    if (v.type() == QVariant::StringList)
        tokenList = v.toStringList();
    else
        tokenList << v.toString();

    foreach (const QString &token, tokenList) {
        if (!seenSet.contains(token)) {
            partial << token;
            seenSet.insert(token);
        }
    }
}

static QVariantList modules(const QVariantMap &root)
{
    QVariantList result;
    if (!root.contains(QLatin1String(MODULES_KEY)))
        return result;
    const QVariantMap &moduleRoot = root.value(QLatin1String(MODULES_KEY)).toMap();
    QVariantMap::const_iterator end = moduleRoot.end();
    for (QVariantMap::const_iterator i = moduleRoot.begin(); i != end; ++i)
        result << i.value();
    return result;
}

static void recursiveAppendAll(const QString &key, const QVariantMap &root,
                               QStringList &partial, QSet<QString> &seenSet)
{
    if (root.isEmpty())
        return;
    expand(extract(root, key), partial, seenSet);
    foreach (const QVariant &v, modules(root))
        recursiveAppendAll(key, v.toMap(), partial, seenSet);
}

static QStringList appendAll(const QVariantMap &data, const QString &key)
{
    QStringList result;
    QSet<QString> seenSet;
    recursiveAppendAll(key, data, result, seenSet);
    return result;
}

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsProject:
// --------------------------------------------------------------------

QbsProject::QbsProject(QbsManager *manager, const QString &fileName) :
    m_manager(manager), m_projectName(QFileInfo(fileName).completeBaseName()), m_fileName(fileName),
    m_rootProjectNode(new QbsProjectNode(fileName)),
    m_qbsSetupProjectJob(0),
    m_qbsUpdateFutureInterface(0),
    m_currentBc(0)
{
    setProjectContext(Core::Context(Constants::PROJECT_ID));
    setProjectLanguage(Core::Context(ProjectExplorer::Constants::LANG_CXX));

    connect(this, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(changeActiveTarget(ProjectExplorer::Target*)));
    connect(this, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(targetWasAdded(ProjectExplorer::Target*)));

    updateDocuments(0);
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
    QStringList result;
    if (m_rootProjectNode && m_rootProjectNode->projectData()) {
        foreach (const qbs::ProductData &prd, m_rootProjectNode->projectData()->products())
            foreach (const qbs::GroupData &grp, prd.groups())
                result.append(grp.allFilePaths());
    }
    return result;
}

void QbsProject::invalidate()
{
    prepareForParsing();
}

qbs::BuildJob *QbsProject::build(const qbs::BuildOptions &opts)
{
    if (!m_rootProjectNode || !m_rootProjectNode->project())
        return 0;
    if (!activeTarget() || !activeTarget()->kit())
        return 0;
    ProjectExplorer::BuildConfiguration *bc = 0;
    bc = activeTarget()->activeBuildConfiguration();
    if (!bc)
        return 0;

    QProcessEnvironment env = bc->environment().toProcessEnvironment();
    return m_rootProjectNode->project()->buildAllProducts(opts, env);
}

qbs::CleanJob *QbsProject::clean(const qbs::BuildOptions &opts, bool everything)
{
    if (!m_rootProjectNode || !m_rootProjectNode->project())
        return 0;
    return m_rootProjectNode->project()->cleanAllProducts(opts, everything ? qbs::Project::CleanupAll
                                                                           : qbs::Project::CleanupTemporaries);
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
    return m_rootProjectNode->project();
}

Utils::FileName QbsProject::defaultBuildDirectory() const
{
    QFileInfo fi(m_fileName);
    const QString buildDir = QDir(fi.canonicalPath()).absoluteFilePath(QString::fromLatin1("../%1-build").arg(fi.baseName()));
    return Utils::FileName::fromString(buildDir);
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

    updateDocuments(m_rootProjectNode->projectData());

    updateCppCodeModel(m_rootProjectNode->projectData());
    updateQmlJsCodeModel(m_rootProjectNode->projectData());

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
            this, SLOT(parseCurrentBuildConfiguration()));
    connect(t, SIGNAL(buildDirectoryChanged()),
            this, SLOT(parseCurrentBuildConfiguration()));
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
        disconnect(m_currentBc, SIGNAL(qbsConfigurationChanged()), this, SLOT(parseCurrentBuildConfiguration()));

    m_currentBc = qobject_cast<QbsBuildConfiguration *>(bc);
    if (m_currentBc) {
        connect(m_currentBc, SIGNAL(qbsConfigurationChanged()), this, SLOT(parseCurrentBuildConfiguration()));
        parseCurrentBuildConfiguration();
    } else {
        invalidate();
    }
}

void QbsProject::parseCurrentBuildConfiguration()
{
    if (!activeTarget())
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;
    parse(bc->qbsConfiguration(), bc->buildDirectory());
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

void QbsProject::generateErrors(const qbs::Error &e)
{
    foreach (const qbs::ErrorData &data, e.entries())
        taskHub()->addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                                 data.description(),
                                                 Utils::FileName::fromString(data.codeLocation().fileName),
                                                 data.codeLocation().line,
                                                 ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
}

void QbsProject::parse(const QVariantMap &config, const QString &dir)
{
    QTC_ASSERT(!dir.isNull(), return);
    prepareForParsing();
    m_qbsBuildConfig = config;
    m_qbsBuildRoot = dir;

    QTC_ASSERT(!m_qbsSetupProjectJob, return);
    qbs::SetupProjectParameters params;
    params.buildConfiguration = m_qbsBuildConfig;
    params.buildRoot = m_qbsBuildRoot;
    params.projectFilePath = m_fileName;
    params.ignoreDifferentProjectFilePath = false;

    m_qbsSetupProjectJob
            = qbs::Project::setupProject(params, m_manager->settings(), 0);

    connect(m_qbsSetupProjectJob, SIGNAL(finished(bool,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingDone(bool)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingTaskSetup(QString,int)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingProgress(int)));
}

void QbsProject::prepareForParsing()
{
    taskHub()->clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    if (m_qbsUpdateFutureInterface)
        m_qbsUpdateFutureInterface->reportCanceled();
    delete m_qbsUpdateFutureInterface;
    m_qbsUpdateFutureInterface = 0;

    // FIXME: Christian claims this should work
    delete m_qbsSetupProjectJob;
    m_qbsSetupProjectJob = 0;

    m_currentProgressBase = 0;
    m_qbsUpdateFutureInterface = new QFutureInterface<void>();
    m_qbsUpdateFutureInterface->setProgressRange(0, 0);
    Core::ICore::progressManager()->addTask(m_qbsUpdateFutureInterface->future(), tr("Evaluating"),
                                            QLatin1String(Constants::QBS_EVALUATE));
    m_qbsUpdateFutureInterface->reportStarted();
}

void QbsProject::updateDocuments(const qbs::ProjectData *prj)
{
    // Update documents:
    QSet<QString> newFiles;
    newFiles.insert(m_fileName); // make sure we always have the project file...

    if (prj) {
        newFiles.insert(prj->location().fileName);
        foreach (const qbs::ProductData &prd, prj->products())
            newFiles.insert(prd.location().fileName);
    }
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

void QbsProject::updateCppCodeModel(const qbs::ProjectData *prj)
{
    if (!prj)
        return;

    ProjectExplorer::Kit *k = 0;
    QtSupport::BaseQtVersion *qtVersion = 0;
    ProjectExplorer::ToolChain *tc = 0;
    if (ProjectExplorer::Target *target = activeTarget())
        k = target->kit();
    else
        k = ProjectExplorer::KitManager::instance()->defaultKit();
    qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);

    CPlusPlus::CppModelManagerInterface *modelmanager =
        CPlusPlus::CppModelManagerInterface::instance();

    if (!modelmanager)
        return;

    CPlusPlus::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
    pinfo.clearProjectParts();
    CPlusPlus::CppModelManagerInterface::ProjectPart::QtVersion qtVersionForPart
            = CPlusPlus::CppModelManagerInterface::ProjectPart::NoQt;
    if (qtVersion) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            qtVersionForPart = CPlusPlus::CppModelManagerInterface::ProjectPart::Qt4;
        else
            qtVersionForPart = CPlusPlus::CppModelManagerInterface::ProjectPart::Qt5;
    }

    QStringList allFiles;
    foreach (const qbs::ProductData &prd, prj->products()) {
        foreach (const qbs::GroupData &grp, prd.groups()) {
            QVariantMap props = grp.properties();

            QStringList grpIncludePaths;
            QStringList grpFrameworkPaths;
            QByteArray grpDefines;
            bool isCxx11;
            const QStringList cxxFlags = appendAll(props, QLatin1String(CONFIG_CXXFLAGS));

            // Toolchain specific stuff:
            QList<ProjectExplorer::HeaderPath> includePaths;
            if (tc) {
                includePaths = tc->systemHeaderPaths(cxxFlags, ProjectExplorer::SysRootKitInformation::sysRoot(k));
                grpDefines += tc->predefinedMacros(cxxFlags);
                if (tc->compilerFlags(cxxFlags) == ProjectExplorer::ToolChain::STD_CXX11)
                    isCxx11 = true;
            }
            foreach (const ProjectExplorer::HeaderPath &headerPath, includePaths) {
                if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                    grpFrameworkPaths.append(headerPath.path());
                else
                    grpIncludePaths.append(headerPath.path());
            }

            QStringList list = appendAll(props, QLatin1String(CONFIG_DEFINES));
            foreach (const QString &def, list)
                grpDefines += (QByteArray("#define ") + def.toUtf8() + '\n');

            list = appendAll(props, QLatin1String(CONFIG_INCLUDEPATHS));
            foreach (const QString &p, list) {
                const QString cp = Utils::FileName::fromUserInput(p).toString();
                grpIncludePaths.append(cp);
            }

            list = appendAll(props, QLatin1String(CONFIG_FRAMEWORKPATHS));
            foreach (const QString &p, list) {
                const QString cp = Utils::FileName::fromUserInput(p).toString();
                grpFrameworkPaths.append(cp);
            }

            const QString pch = extract(props, QLatin1String(CONFIG_PRECOMPILEDHEADER)).toString();

            QStringList cxxSources;
            QStringList cSources;
            QStringList headers;
            QStringList objcSources;
            cxxSources << QLatin1String(CONFIGURATION_PATH);
            cSources << QLatin1String(CONFIGURATION_PATH);
            objcSources << QLatin1String(CONFIGURATION_PATH);

            foreach (const QString &file, grp.allFilePaths()) {
                QFileInfo fi = QFileInfo(file);
                if (!fi.exists())
                    continue;

                Core::MimeType t = Core::ICore::mimeDatabase()->findByFile(fi);
                if (t.isNull())
                    continue;
                if (t.matchesType(QLatin1String("text/x-chdr")))
                    headers << file;
                else if (t.matchesType(QLatin1String("text/x-c++src")))
                    cxxSources << file;
                else if (t.matchesType(QLatin1String("text/x-objcsrc")))
                    objcSources << file;
                else if (t.matchesType(QLatin1String("text/x-csrc")))
                    cxxSources << file;
            }
            allFiles.append(headers);
            allFiles.append(cSources);
            allFiles.append(cxxSources);
            allFiles.append(objcSources);

            if (cxxSources.count() > 1) {
                CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(new CPlusPlus::CppModelManagerInterface::ProjectPart);
                part->qtVersion = qtVersionForPart;
                part->language = isCxx11 ? CPlusPlus::CppModelManagerInterface::ProjectPart::CXX11
                                         : CPlusPlus::CppModelManagerInterface::ProjectPart::CXX;
                part->sourceFiles = cxxSources;
                part->objcSourceFiles = objcSources;
                part->headerFiles = headers;
                part->includePaths = grpIncludePaths;
                part->frameworkPaths = grpFrameworkPaths;
                part->precompiledHeaders = QStringList(pch);
                part->defines = grpDefines;
                pinfo.appendProjectPart(part);
            }
            if (cSources.count() > 1) {
                CPlusPlus::CppModelManagerInterface::ProjectPart::Ptr part(new CPlusPlus::CppModelManagerInterface::ProjectPart);
                part->qtVersion = CPlusPlus::CppModelManagerInterface::ProjectPart::NoQt;
                part->language = CPlusPlus::CppModelManagerInterface::ProjectPart::C99; // FIXME: Can we find the exact c version from tc?
                part->sourceFiles = cxxSources;
                part->headerFiles = headers;
                part->includePaths = grpIncludePaths;
                part->frameworkPaths = grpFrameworkPaths;
                part->precompiledHeaders = QStringList(pch);
                part->defines = grpDefines;
                pinfo.appendProjectPart(part);
            }
        }
    }

    if (pinfo.projectParts().isEmpty())
        return;

    // Register update the code model:
    modelmanager->updateProjectInfo(pinfo);
    m_codeModelFuture = modelmanager->updateSourceFiles(allFiles);
}

void QbsProject::updateQmlJsCodeModel(const qbs::ProjectData *prj)
{
    // FIXME: No information about import directories, so ignore this for now.
#if 1
    Q_UNUSED(prj);
#else
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
#endif
}

} // namespace Internal
} // namespace QbsProjectManager
