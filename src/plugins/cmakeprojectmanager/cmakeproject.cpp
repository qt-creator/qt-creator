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

#include "cmakeproject.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakeprojectmanager.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpprawprojectpart.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cpptoolsconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QElapsedTimer>
#include <QSet>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

Q_LOGGING_CATEGORY(cmakeProjectLog, "qtc.cmake.project", QtWarningMsg);

class TraceTimer
{
public:
    TraceTimer(const QString &msg)
        : m_message(msg)
    {
        m_timer.start();
    }

    ~TraceTimer()
    {
        qCInfo(cmakeProjectLog) << QString("%1 (%2ms)").arg(m_message).arg(m_timer.elapsed());
    }

private:
    QElapsedTimer m_timer;
    QString m_message;
};

using namespace Internal;

static CMakeBuildConfiguration *activeBc(const CMakeProject *p)
{
    return qobject_cast<CMakeBuildConfiguration *>(p->activeTarget() ? p->activeTarget()->activeBuildConfiguration() : nullptr);
}

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the actual compiler executable
// DEFINES

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(const FilePath &fileName) : Project(Constants::CMAKEMIMETYPE, fileName),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater),
    m_buildDirManager(this)
{
    setId(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();

    // Timer:
    m_delayedParsingTimer.setSingleShot(true);

    connect(&m_delayedParsingTimer, &QTimer::timeout,
            this, [this]() { startParsing(m_delayedParsingParameters); });

    // BuildDirManager:
    connect(&m_buildDirManager, &BuildDirManager::requestReparse,
            this, &CMakeProject::handleReparseRequest);
    connect(&m_buildDirManager, &BuildDirManager::dataAvailable,
            this, [this]() {
        CMakeBuildConfiguration *bc = m_buildDirManager.buildConfiguration();
        if (bc) {
            bc->clearError();
            handleParsingSuccess(bc);
        }
    });
    connect(&m_buildDirManager, &BuildDirManager::errorOccured,
            this, [this](const QString &msg) {
        reportError(msg);
        CMakeBuildConfiguration *bc = m_buildDirManager.buildConfiguration();
        if (bc) {
            QString errorMessage;
            bc->setConfigurationFromCMake(m_buildDirManager.takeCMakeConfiguration(errorMessage));
            // ignore errorMessage here, we already got one.
            handleParsingError(bc);
        }

    });
    connect(&m_buildDirManager, &BuildDirManager::parsingStarted,
            this, [this]() {
        CMakeBuildConfiguration *bc = m_buildDirManager.buildConfiguration();
        if (bc)
            bc->clearError(CMakeBuildConfiguration::ForceEnabledChanged::True);
    });

    // Kit changed:
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, [this](Kit *k) {
        CMakeBuildConfiguration *bc = activeBc(this);
        if (!bc || k != bc->target()->kit())
            return; // not for us...

        // Build configuration has not changed, but Kit settings might have:
        // reparse and check the configuration, independent of whether the reader has changed
        m_buildDirManager.setParametersAndRequestParse(
                    BuildDirParameters(bc),
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION,
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION);
    });

    // Target switched:
    connect(this, &Project::activeTargetChanged, this, [this]() {
        CMakeBuildConfiguration *bc = activeBc(this);

        if (!bc)
            return;

        // Target has switched, so the kit has changed, too.
        // * run cmake with configuration arguments if the reader needs to be switched
        // * run cmake without configuration arguments if the reader stays
        m_buildDirManager.setParametersAndRequestParse(
                    BuildDirParameters(bc),
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION,
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION);
    });

    // BuildConfiguration switched:
    subscribeSignal(&Target::activeBuildConfigurationChanged, this, [this]() {
        CMakeBuildConfiguration *bc = activeBc(this);

        if (!bc)
            return;

        // Build configuration has switched:
        // * Check configuration if reader changes due to it not existing yet:-)
        // * run cmake without configuration arguments if the reader stays
        m_buildDirManager.setParametersAndRequestParse(
                    BuildDirParameters(bc),
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION,
                    BuildDirManager::REPARSE_CHECK_CONFIGURATION);
    });

    // BuildConfiguration changed:
    subscribeSignal(&CMakeBuildConfiguration::environmentChanged, this, [this]() {
        auto senderBc = qobject_cast<CMakeBuildConfiguration *>(sender());

        if (senderBc && senderBc->isActive()) {
            // The environment on our BC has changed:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake without configuration arguments if the reader stays
            m_buildDirManager.setParametersAndRequestParse(
                BuildDirParameters(senderBc),
                BuildDirManager::REPARSE_CHECK_CONFIGURATION, // server-mode might need a restart...
                BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        }
    });
    subscribeSignal(&CMakeBuildConfiguration::buildDirectoryChanged, this, [this]() {
        auto senderBc = qobject_cast<CMakeBuildConfiguration *>(sender());

        if (senderBc && senderBc == m_buildDirManager.buildConfiguration()) {
            // The build directory of our BC has changed:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake without configuration arguments if the reader stays
            //   If no configuration exists, then the arguments will get added automatically by
            //   the reader.
            m_buildDirManager.setParametersAndRequestParse(
                        BuildDirParameters(senderBc),
                        BuildDirManager::REPARSE_FAIL,
                        BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        }
    });
    subscribeSignal(&CMakeBuildConfiguration::configurationForCMakeChanged, this, [this]() {
        auto senderBc = qobject_cast<CMakeBuildConfiguration *>(sender());

        if (senderBc && senderBc == m_buildDirManager.buildConfiguration()) {
            // The CMake configuration has changed on our BC:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake with configuration arguments if the reader stays
            m_buildDirManager.setParametersAndRequestParse(
                        BuildDirParameters(senderBc),
                        BuildDirManager::REPARSE_FAIL,
                        BuildDirManager::REPARSE_FORCE_CONFIGURATION);
        }
    });

    // TreeScanner:
    connect(&m_treeScanner, &TreeScanner::finished, this, &CMakeProject::handleTreeScanningFinished);

    m_treeScanner.setFilter([this](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored =
                fn.toString().startsWith(projectFilePath().toString() + ".user") ||
                TreeScanner::isWellKnownBinary(mimeType, fn);

        // Cache mime check result for speed up
        if (!isIgnored) {
            auto it = m_mimeBinaryCache.find(mimeType.name());
            if (it != m_mimeBinaryCache.end()) {
                isIgnored = *it;
            } else {
                isIgnored = TreeScanner::isMimeBinary(mimeType, fn);
                m_mimeBinaryCache[mimeType.name()] = isIgnored;
            }
        }

        return isIgnored;
    });

    m_treeScanner.setTypeFactory([](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
        auto type = TreeScanner::genericFileType(mimeType, fn);
        if (type == FileType::Unknown) {
            if (mimeType.isValid()) {
                const QString mt = mimeType.name();
                if (mt == CMakeProjectManager::Constants::CMAKEPROJECTMIMETYPE
                    || mt == CMakeProjectManager::Constants::CMAKEMIMETYPE)
                    type = FileType::Project;
            }
        }
        return type;
    });
}

CMakeProject::~CMakeProject()
{
    if (!m_treeScanner.isFinished()) {
        auto future = m_treeScanner.future();
        future.cancel();
        future.waitForFinished();
    }
    delete m_cppCodeModelUpdater;
    qDeleteAll(m_extraCompilers);
    qDeleteAll(m_allFiles);
}

void CMakeProject::updateProjectData(CMakeBuildConfiguration *bc)
{
    TraceTimer updateProjectTotalTimer(Q_FUNC_INFO);
    qCDebug(cmakeProjectLog) << "Updating CMake project data";
    const CMakeBuildConfiguration *aBc = activeBc(this);
    QString errorMessage;

    QTC_ASSERT(bc, return);
    QTC_ASSERT(bc == aBc, return);
    QTC_ASSERT(m_treeScanner.isFinished() && !m_buildDirManager.isParsing(), return);

    {
        TraceTimer buildTargetsTimer("    build targets");
        const QList<CMakeBuildTarget> buildTargets = m_buildDirManager.takeBuildTargets(
            errorMessage);
        checkAndReportError(errorMessage);
        bc->setBuildTargets(buildTargets);
        qCDebug(cmakeProjectLog) << "Build target data set.";
    }
    CMakeConfig patchedConfig;
    {
        TraceTimer cacheTimer("    cache data (plus patching)");
        const CMakeConfig cmakeConfig = m_buildDirManager.takeCMakeConfiguration(errorMessage);
        checkAndReportError(errorMessage);
        bc->setConfigurationFromCMake(cmakeConfig);
        qCDebug(cmakeProjectLog) << "CMake configuration data set.";

        patchedConfig = cmakeConfig;
        {
            CMakeConfigItem settingFileItem;
            settingFileItem.key = "ANDROID_DEPLOYMENT_SETTINGS_FILE";
            settingFileItem.value = bc->buildDirectory()
                                        .pathAppended("android_deployment_settings.json")
                                        .toString()
                                        .toUtf8();
            patchedConfig.append(settingFileItem);
        }
    }
    {
        TraceTimer appsTimer("    application data");
        QSet<QString> res;
        QStringList apps;
        for (const auto &target : bc->buildTargets()) {
            if (target.targetType == CMakeProjectManager::DynamicLibraryType) {
                res.insert(target.executable.parentDir().toString());
                apps.push_back(target.executable.toUserOutput());
            }
            // ### shall we add also the ExecutableType ?
        }
        {
            CMakeConfigItem paths;
            paths.key = "ANDROID_SO_LIBS_PATHS";
            paths.values = Utils::toList(res);
            patchedConfig.append(paths);
        }

        apps.sort();
        {
            CMakeConfigItem appsPaths;
            appsPaths.key = "TARGETS_BUILD_PATH";
            appsPaths.values = apps;
            patchedConfig.append(appsPaths);
        }
    }

    {
        TraceTimer projectTreeTimer("    project tree");
        auto newRoot = generateProjectTree(m_allFiles);
        if (newRoot) {
            setRootProjectNode(std::move(newRoot));
            if (rootProjectNode())
                setDisplayName(rootProjectNode()->displayName());

            for (const CMakeBuildTarget &bt : bc->buildTargets()) {
                const QString buildKey = bt.title;
                if (ProjectNode *node = findNodeForBuildKey(buildKey)) {
                    if (auto targetNode = dynamic_cast<CMakeTargetNode *>(node))
                        targetNode->setConfig(patchedConfig);
                }
            }
        }
    }

    {
        TraceTimer projectTreeTimer("    target updated");
        Target *t = bc->target();
        t->setApplicationTargets(bc->appTargets());
        t->setDeploymentData(bc->deploymentData());
    }

    {
        TraceTimer projectTreeTimer("    extra compilers");
        qDeleteAll(m_extraCompilers);
        m_extraCompilers = findExtraCompilers();
        CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
        qCDebug(cmakeProjectLog) << "Extra compilers updated.";
    }

    QtSupport::CppKitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);

    {
        TraceTimer cxxCodemodelTimer("    cxx codemodel");
        CppTools::RawProjectParts rpps = m_buildDirManager.createRawProjectParts(errorMessage);
        checkAndReportError(errorMessage);
        qCDebug(cmakeProjectLog) << "Raw project parts created.";

        for (CppTools::RawProjectPart &rpp : rpps) {
            rpp.setQtVersion(
                kitInfo.projectPartQtVersion); // TODO: Check if project actually uses Qt.
            if (kitInfo.cxxToolChain)
                rpp.setFlagsForCxx({kitInfo.cxxToolChain, rpp.flagsForCxx.commandLineFlags});
            if (kitInfo.cToolChain)
                rpp.setFlagsForC({kitInfo.cToolChain, rpp.flagsForC.commandLineFlags});
        }

        m_cppCodeModelUpdater->update({this, kitInfo, activeBuildEnvironment(), rpps});
    }
    {
        TraceTimer qmlCodemodelTimer("    qml codemodel");
        updateQmlJSCodeModel();
    }

    {
        TraceTimer resetTimer("    resetting builddirmanager");
        m_buildDirManager.resetData();
    }

    {
        TraceTimer emitTimer("    emitting signals");
        emit fileListChanged();

        bc->emitBuildTypeChanged();
    }
    qCDebug(cmakeProjectLog) << "All CMake project data up to date.";
}

void CMakeProject::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    if (!modelManager || !activeTarget() || !activeTarget()->activeBuildConfiguration())
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo =
            modelManager->defaultProjectInfoForProject(this);

    projectInfo.importPaths.clear();

    QString cmakeImports;
    auto bc = qobject_cast<const CMakeBuildConfiguration *>(activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    const CMakeConfig &cm = bc->configurationFromCMake();
    foreach (const CMakeConfigItem &di, cm) {
        if (di.key.contains("QML_IMPORT_PATH")) {
            cmakeImports = QString::fromUtf8(di.value);
            break;
        }
    }

    foreach (const QString &cmakeImport, CMakeConfigItem::cmakeSplitValue(cmakeImports))
        projectInfo.importPaths.maybeInsert(FilePath::fromString(cmakeImport), QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, this);
}

std::unique_ptr<CMakeProjectNode>
CMakeProject::generateProjectTree(const QList<const FileNode *> &allFiles) const
{
    if (m_buildDirManager.isParsing())
        return nullptr;

    QString errorMessage;
    auto root = m_buildDirManager.generateProjectTree(allFiles, errorMessage);
    checkAndReportError(errorMessage);
    return root;
}

bool CMakeProject::knowsAllBuildExecutables() const
{
    return false;
}

Tasks CMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!CMakeKitAspect::cmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, tr("No cmake tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, tr("No compilers set in kit.")));

    return result;
}

void CMakeProject::runCMake()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    if (isParsing() || !bc)
        return;

    BuildDirParameters parameters(bc);
    m_buildDirManager.setParametersAndRequestParse(parameters,
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                                       | BuildDirManager::REPARSE_FORCE_CMAKE_RUN,
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                                       | BuildDirManager::REPARSE_FORCE_CMAKE_RUN);
}

void CMakeProject::runCMakeAndScanProjectTree()
{
    CMakeBuildConfiguration *bc = activeBc(this);
    if (isParsing() || !bc)
        return;
    QTC_ASSERT(m_treeScanner.isFinished(), return);

    BuildDirParameters parameters(bc);
    m_buildDirManager.setParametersAndRequestParse(parameters,
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION | BuildDirManager::REPARSE_SCAN,
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION | BuildDirManager::REPARSE_SCAN);
}

void CMakeProject::buildCMakeTarget(const QString &buildTarget)
{
    QTC_ASSERT(!buildTarget.isEmpty(), return);
    CMakeBuildConfiguration *bc = activeBc(this);
    if (bc)
        bc->buildTarget(buildTarget);
}

ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = std::make_unique<CMakeProjectImporter>(projectFilePath());
    return m_projectImporter.get();
}

bool CMakeProject::persistCMakeState()
{
    return m_buildDirManager.persistCMakeState();
}

void CMakeProject::clearCMakeCache()
{
    m_buildDirManager.clearCache();
}

void CMakeProject::handleReparseRequest(int reparseParameters)
{
    QTC_ASSERT(!(reparseParameters & BuildDirManager::REPARSE_FAIL), return);
    if (reparseParameters & BuildDirManager::REPARSE_IGNORE)
        return;

    m_delayedParsingTimer.setInterval((reparseParameters & BuildDirManager::REPARSE_URGENT) ? 0 : 1000);
    m_delayedParsingTimer.start();
    m_delayedParsingParameters = m_delayedParsingParameters | reparseParameters;
    if (m_allFiles.isEmpty())
        m_delayedParsingParameters |= BuildDirManager::REPARSE_SCAN;
}

void CMakeProject::startParsing(int reparseParameters)
{
    m_delayedParsingParameters = BuildDirManager::REPARSE_DEFAULT;

    QTC_ASSERT((reparseParameters & BuildDirManager::REPARSE_FAIL) == 0, return);
    if (reparseParameters & BuildDirManager::REPARSE_IGNORE)
        return;

    QTC_ASSERT(activeBc(this), return);

    emitParsingStarted();

    m_waitingForScan = reparseParameters & BuildDirManager::REPARSE_SCAN;
    m_waitingForParse = true;
    m_combinedScanAndParseResult = true;

    if (m_waitingForScan) {
        QTC_CHECK(m_treeScanner.isFinished());
        m_treeScanner.asyncScanForFiles(projectDirectory());
        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       tr("Scan \"%1\" project tree").arg(displayName()),
                                       "CMake.Scan.Tree");
    }

    m_buildDirManager.parse(reparseParameters);
}

QStringList CMakeProject::buildTargetTitles() const
{
    CMakeBuildConfiguration *bc = activeBc(this);
    return bc ? bc->buildTargetTitles() : QStringList();
}

Project::RestoreResult CMakeProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    RestoreResult result = Project::fromMap(map, errorMessage);
    if (result != RestoreResult::Ok)
        return result;
    return RestoreResult::Ok;
}

bool CMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();
    return true;
}

void CMakeProject::reportError(const QString &errorMessage) const
{
    CMakeBuildConfiguration *bc = m_buildDirManager.buildConfiguration();
    if (bc)
        bc->setError(errorMessage);
}

void CMakeProject::handleTreeScanningFinished()
{
    QTC_CHECK(m_waitingForScan);

    qDeleteAll(m_allFiles);
    m_allFiles = Utils::transform(m_treeScanner.release(), [](const FileNode *fn) { return fn; });

    CMakeBuildConfiguration *bc = activeBc(this);
    QTC_ASSERT(bc, return);

    m_combinedScanAndParseResult = m_combinedScanAndParseResult && true;
    m_waitingForScan = false;

    combineScanAndParse(bc);
}

void CMakeProject::handleParsingSuccess(CMakeBuildConfiguration *bc)
{
    QTC_ASSERT(m_waitingForParse, return);

    m_waitingForParse = false;
    m_combinedScanAndParseResult = m_combinedScanAndParseResult && true;

    combineScanAndParse(bc);
}

void CMakeProject::handleParsingError(CMakeBuildConfiguration *bc)
{
    QTC_CHECK(m_waitingForParse);

    m_waitingForParse = false;
    m_combinedScanAndParseResult = false;

    combineScanAndParse(bc);
}

void CMakeProject::combineScanAndParse(CMakeBuildConfiguration *bc)
{
    QTC_ASSERT(bc && bc->isActive(), return);

    if (m_waitingForParse || m_waitingForScan)
        return;

    if (m_combinedScanAndParseResult)
        updateProjectData(bc);

    {
        TraceTimer parsingDoneTimer("    parsing finished signal");
        emitParsingFinished(m_combinedScanAndParseResult);
    }
}

QStringList CMakeProject::filesGeneratedFrom(const QString &sourceFile) const
{
    if (!activeTarget())
        return QStringList();
    QFileInfo fi(sourceFile);
    FilePath project = projectDirectory();
    FilePath baseDirectory = FilePath::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        const FilePath cmakeListsTxt = baseDirectory.pathAppended("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        baseDirectory = baseDirectory.parentDir();
    }

    QDir srcDirRoot = QDir(project.toString());
    QString relativePath = srcDirRoot.relativeFilePath(baseDirectory.toString());
    QDir buildDir = QDir(activeTarget()->activeBuildConfiguration()->buildDirectory().toString());
    QString generatedFilePath = buildDir.absoluteFilePath(relativePath);

    if (fi.suffix() == "ui") {
        generatedFilePath += "/ui_";
        generatedFilePath += fi.completeBaseName();
        generatedFilePath += ".h";
        return QStringList(QDir::cleanPath(generatedFilePath));
    } else if (fi.suffix() == "scxml") {
        generatedFilePath += "/";
        generatedFilePath += QDir::cleanPath(fi.completeBaseName());
        return QStringList({generatedFilePath + ".h",
                            generatedFilePath + ".cpp"});
    } else {
        // TODO: Other types will be added when adapters for their compilers become available.
        return QStringList();
    }
}

ProjectExplorer::DeploymentKnowledge CMakeProject::deploymentKnowledge() const
{
    return !files([](const ProjectExplorer::Node *n) {
                return n->filePath().fileName() == "QtCreatorDeployment.txt";
            })
                   .isEmpty()
               ? DeploymentKnowledge::Approximative
               : DeploymentKnowledge::Bad;
}

MakeInstallCommand CMakeProject::makeInstallCommand(const Target *target,
                                                    const QString &installRoot)
{
    MakeInstallCommand cmd;
    if (const BuildConfiguration * const bc = target->activeBuildConfiguration()) {
        if (const auto cmakeStep = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
                ->firstOfType<CMakeBuildStep>()) {
            if (CMakeTool *tool = CMakeKitAspect::cmakeTool(target->kit()))
                cmd.command = tool->cmakeExecutable();
        }
    }
    cmd.arguments << "--build" << "." << "--target" << "install";
    cmd.environment.set("DESTDIR", QDir::toNativeSeparators(installRoot));
    return cmd;
}

bool CMakeProject::mustUpdateCMakeStateBeforeBuild()
{
    return m_delayedParsingTimer.isActive();
}

void CMakeProject::checkAndReportError(QString &errorMessage) const
{
    if (!errorMessage.isEmpty()) {
        reportError(errorMessage);
        errorMessage.clear();
    }
}

QList<ProjectExplorer::ExtraCompiler *> CMakeProject::findExtraCompilers() const
{
    qCDebug(cmakeProjectLog) << "Finding Extra Compilers: start.";

    QList<ProjectExplorer::ExtraCompiler *> extraCompilers;
    const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();

    qCDebug(cmakeProjectLog) << "Finding Extra Compilers: Got factories.";

    const QSet<QString> fileExtensions = Utils::transform<QSet>(factories,
                                                                &ExtraCompilerFactory::sourceTag);

    qCDebug(cmakeProjectLog) << "Finding Extra Compilers: Got file extensions:" << fileExtensions;

    // Find all files generated by any of the extra compilers, in a rather crude way.
    const FilePathList fileList = files([&fileExtensions](const Node *n) {
        if (!SourceFiles(n))
            return false;
        const QString fp = n->filePath().toString();
        const int pos = fp.lastIndexOf('.');
        return pos >= 0 && fileExtensions.contains(fp.mid(pos + 1));
    });

    qCDebug(cmakeProjectLog) << "Finding Extra Compilers: Got list of files to check.";

    // Generate the necessary information:
    for (const FilePath &file : fileList) {
        qCDebug(cmakeProjectLog) << "Finding Extra Compilers: Processing" << file.toUserOutput();
        ExtraCompilerFactory *factory = Utils::findOrDefault(factories,
                                                             [&file](const ExtraCompilerFactory *f) {
                                                                 return file.endsWith(
                                                                     '.' + f->sourceTag());
                                                             });
        QTC_ASSERT(factory, continue);

        QStringList generated = filesGeneratedFrom(file.toString());
        qCDebug(cmakeProjectLog) << "Finding Extra Compilers:     generated files:" << generated;
        if (generated.isEmpty())
            continue;

        const FilePathList fileNames
                = transform(generated,
                            [](const QString &s) { return FilePath::fromString(s); });
        extraCompilers.append(factory->create(this, file, fileNames));
        qCDebug(cmakeProjectLog) << "Finding Extra Compilers:     done with" << file.toUserOutput();
    }

    qCDebug(cmakeProjectLog) << "Finding Extra Compilers: done.";

    return extraCompilers;
}

} // namespace CMakeProjectManager
