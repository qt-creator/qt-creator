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

#include "cmakebuildsystem.h"

#include "cmakebuildconfiguration.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"

#include <android/androidconstants.h>

#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/fileutils.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

Q_LOGGING_CATEGORY(cmakeBuildSystemLog, "qtc.cmake.buildsystem", QtWarningMsg);

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

CMakeBuildSystem::CMakeBuildSystem(CMakeBuildConfiguration *bc)
    : BuildSystem(bc)
    , m_buildConfiguration(bc)
    , m_buildDirManager(this)
    , m_cppCodeModelUpdater(new CppTools::CppProjectUpdater)
{
    // TreeScanner:
    connect(&m_treeScanner, &TreeScanner::finished,
            this, &CMakeBuildSystem::handleTreeScanningFinished);

    m_treeScanner.setFilter([this](const MimeType &mimeType, const FilePath &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored = fn.toString().startsWith(projectFilePath().toString() + ".user")
                         || TreeScanner::isWellKnownBinary(mimeType, fn);

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

    // BuildDirManager:
    connect(&m_buildDirManager, &BuildDirManager::requestReparse, this, [this] {
        if (m_buildConfiguration->isActive())
            requestParse();
    });
    connect(&m_buildDirManager, &BuildDirManager::requestDelayedReparse, this, [this] {
        if (m_buildConfiguration->isActive())
            requestDelayedParse();
    });

    connect(&m_buildDirManager, &BuildDirManager::dataAvailable,
            this, &CMakeBuildSystem::handleParsingSucceeded);

    connect(&m_buildDirManager, &BuildDirManager::errorOccured,
            this, &CMakeBuildSystem::handleParsingFailed);

    connect(&m_buildDirManager, &BuildDirManager::parsingStarted, this, [this]() {
        m_buildConfiguration->clearError(CMakeBuildConfiguration::ForceEnabledChanged::True);
    });

    // Kit changed:
    connect(KitManager::instance(), &KitManager::kitUpdated, this, [this](Kit *k) {
        if (k != target()->kit())
            return; // not for us...
        // Build configuration has not changed, but Kit settings might have:
        // reparse and check the configuration, independent of whether the reader has changed
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to kit being updated";
        m_buildDirManager.setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                                       BuildDirManager::REPARSE_CHECK_CONFIGURATION);
    });

    // Became active/inactive:
    connect(project(), &Project::activeTargetChanged, this, [this](Target *t) {
        if (t == target()) {
            // Build configuration has switched:
            // * Check configuration if reader changes due to it not existing yet:-)
            // * run cmake without configuration arguments if the reader stays
            qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active target changed";
            m_buildDirManager
                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                              BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        } else {
            m_buildDirManager.stopParsingAndClearState();
        }
    });
    connect(target(), &Target::activeBuildConfigurationChanged, this, [this](BuildConfiguration *bc) {
        if (m_buildConfiguration->isActive()) {
            if (m_buildConfiguration == bc) {
                // Build configuration has switched:
                // * Check configuration if reader changes due to it not existing yet:-)
                // * run cmake without configuration arguments if the reader stays
                qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active BC changed";
                m_buildDirManager
                                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                                              BuildDirManager::REPARSE_CHECK_CONFIGURATION);
            } else {
                m_buildDirManager.stopParsingAndClearState();
            }
        }
    });

    // BuildConfiguration changed:
    connect(m_buildConfiguration, &CMakeBuildConfiguration::environmentChanged, this, [this]() {
        if (m_buildConfiguration->isActive()) {
            // The environment on our BC has changed:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake without configuration arguments if the reader stays
            qCDebug(cmakeBuildSystemLog) << "Requesting parse due to environment change";
            m_buildDirManager
                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                              BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        }
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::buildDirectoryChanged, this, [this]() {
        if (m_buildConfiguration->isActive()) {
            // The build directory of our BC has changed:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake without configuration arguments if the reader stays
            //   If no configuration exists, then the arguments will get added automatically by
            //   the reader.
            qCDebug(cmakeBuildSystemLog) << "Requesting parse due to build directory change";
            m_buildDirManager
                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                              BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        }
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::configurationForCMakeChanged, this, [this]() {
        if (m_buildConfiguration->isActive()) {
            // The CMake configuration has changed on our BC:
            // * Error out if the reader updates, cannot happen since all BCs share a target/kit.
            // * run cmake with configuration arguments if the reader stays
            qCDebug(cmakeBuildSystemLog) << "Requesting parse due to cmake configuration change";
            m_buildDirManager
                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                              BuildDirManager::REPARSE_FORCE_CONFIGURATION);
        }
    });

    connect(project(), &Project::projectFileIsDirty, this, [this]() {
        if (m_buildConfiguration->isActive()) {
            qCDebug(cmakeBuildSystemLog) << "Requesting parse due to dirty project file";
            m_buildDirManager
                .setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                              BuildDirManager::REPARSE_DEFAULT);
        }
    });

    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to initial CMake BuildSystem setup";
    m_buildDirManager.setParametersAndRequestParse(BuildDirParameters(m_buildConfiguration),
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION);
}

CMakeBuildSystem::~CMakeBuildSystem()
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

void CMakeBuildSystem::triggerParsing()
{
    qCDebug(cmakeBuildSystemLog) << "Parsing has been triggered";
    m_currentGuard = guardParsingRun();

    QTC_CHECK(m_currentGuard.guardsProject());

    if (m_allFiles.isEmpty())
        m_buildDirManager.requestFilesystemScan();

    m_waitingForScan = m_buildDirManager.isFilesystemScanRequested();
    m_waitingForParse = true;
    m_combinedScanAndParseResult = true;

    if (m_waitingForScan) {
        QTC_CHECK(m_treeScanner.isFinished());
        m_treeScanner.asyncScanForFiles(projectDirectory());
        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       tr("Scan \"%1\" project tree")
                                           .arg(project()->displayName()),
                                       "CMake.Scan.Tree");
    }

    m_buildDirManager.parse();
}

QStringList CMakeBuildSystem::filesGeneratedFrom(const QString &sourceFile) const
{
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
    QDir buildDir = QDir(target()->activeBuildConfiguration()->buildDirectory().toString());
    QString generatedFilePath = buildDir.absoluteFilePath(relativePath);

    if (fi.suffix() == "ui") {
        generatedFilePath += "/ui_";
        generatedFilePath += fi.completeBaseName();
        generatedFilePath += ".h";
        return {QDir::cleanPath(generatedFilePath)};
    }
    if (fi.suffix() == "scxml") {
        generatedFilePath += "/";
        generatedFilePath += QDir::cleanPath(fi.completeBaseName());
        return {generatedFilePath + ".h", generatedFilePath + ".cpp"};
    }

    // TODO: Other types will be added when adapters for their compilers become available.
    return {};
}

void CMakeBuildSystem::runCMake()
{
    BuildDirParameters parameters(m_buildConfiguration);
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due \"Run CMake\" command";
    m_buildDirManager.setParametersAndRequestParse(parameters,
                                                       BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                                       | BuildDirManager::REPARSE_FORCE_CMAKE_RUN
                                                       | BuildDirManager::REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeAndScanProjectTree()
{
    BuildDirParameters parameters(m_buildConfiguration);
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    m_buildDirManager.setParametersAndRequestParse(parameters,
                                                       BuildDirManager::REPARSE_CHECK_CONFIGURATION
                                                       | BuildDirManager::REPARSE_SCAN);
}

void CMakeBuildSystem::buildCMakeTarget(const QString &buildTarget)
{
    QTC_ASSERT(!buildTarget.isEmpty(), return);
    m_buildConfiguration->buildTarget(buildTarget);
}

void CMakeBuildSystem::handleTreeScanningFinished()
{
    QTC_CHECK(m_waitingForScan);

    qDeleteAll(m_allFiles);
    m_allFiles = Utils::transform(m_treeScanner.release(), [](const FileNode *fn) { return fn; });

    m_waitingForScan = false;

    combineScanAndParse();
}

bool CMakeBuildSystem::persistCMakeState()
{
    return m_buildDirManager.persistCMakeState();
}

void CMakeBuildSystem::clearCMakeCache()
{
    m_buildDirManager.clearCache();
}

void CMakeBuildSystem::handleParsingSuccess()
{
    QTC_ASSERT(m_waitingForParse, return );

    m_waitingForParse = false;

    combineScanAndParse();
}

void CMakeBuildSystem::handleParsingError()
{
    QTC_CHECK(m_waitingForParse);

    m_waitingForParse = false;
    m_combinedScanAndParseResult = false;

    combineScanAndParse();
}

std::unique_ptr<CMakeProjectNode>
    CMakeBuildSystem::generateProjectTree(const QList<const FileNode *> &allFiles)
{
    QString errorMessage;
    auto root = m_buildDirManager.generateProjectTree(allFiles, errorMessage);
    checkAndReportError(errorMessage);
    return root;
}

void CMakeBuildSystem::combineScanAndParse()
{
    if (m_buildConfiguration->isActive()) {
        if (m_waitingForParse || m_waitingForScan)
            return;

        if (m_combinedScanAndParseResult) {
            updateProjectData();
            m_currentGuard.markAsSuccess();
        }
    }

    m_currentGuard = {};

    emitBuildSystemUpdated();
}

void CMakeBuildSystem::checkAndReportError(QString &errorMessage)
{
    if (!errorMessage.isEmpty()) {
        m_buildConfiguration->setError(errorMessage);
        errorMessage.clear();
    }
}

void CMakeBuildSystem::updateProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating CMake project data";

    QTC_ASSERT(m_treeScanner.isFinished() && !m_buildDirManager.isParsing(), return);

    m_buildConfiguration->project()->setExtraProjectFiles(m_buildDirManager.takeProjectFilesToWatch());

    CMakeConfig patchedConfig = m_buildConfiguration->configurationFromCMake();
    {
        CMakeConfigItem settingFileItem;
        settingFileItem.key = "ANDROID_DEPLOYMENT_SETTINGS_FILE";
        settingFileItem.value = m_buildConfiguration->buildDirectory()
                                    .pathAppended("android_deployment_settings.json")
                                    .toString()
                                    .toUtf8();
        patchedConfig.append(settingFileItem);
    }
    {
        QSet<QString> res;
        QStringList apps;
        for (const auto &target : m_buildTargets) {
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

    Project *p = project();
    {
        auto newRoot = generateProjectTree(m_allFiles);
        if (newRoot) {
            setRootProjectNode(std::move(newRoot));
            if (p->rootProjectNode())
                p->setDisplayName(p->rootProjectNode()->displayName());

            for (const CMakeBuildTarget &bt : m_buildTargets) {
                const QString buildKey = bt.title;
                if (ProjectNode *node = p->findNodeForBuildKey(buildKey)) {
                    if (auto targetNode = dynamic_cast<CMakeTargetNode *>(node))
                        targetNode->setConfig(patchedConfig);
                }
            }
        }
    }

    {
        qDeleteAll(m_extraCompilers);
        m_extraCompilers = findExtraCompilers();
        CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
        qCDebug(cmakeBuildSystemLog) << "Extra compilers updated.";
    }

    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return );

    {
        QString errorMessage;
        RawProjectParts rpps = m_buildDirManager.createRawProjectParts(errorMessage);
        if (!errorMessage.isEmpty())
            m_buildConfiguration->setError(errorMessage);
        qCDebug(cmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

        for (RawProjectPart &rpp : rpps) {
            rpp.setQtVersion(
                kitInfo.projectPartQtVersion); // TODO: Check if project actually uses Qt.
            if (kitInfo.cxxToolChain)
                rpp.setFlagsForCxx({kitInfo.cxxToolChain, rpp.flagsForCxx.commandLineFlags});
            if (kitInfo.cToolChain)
                rpp.setFlagsForC({kitInfo.cToolChain, rpp.flagsForC.commandLineFlags});
        }

        m_cppCodeModelUpdater->update({p, kitInfo, m_buildConfiguration->environment(), rpps});
    }
    {
        updateQmlJSCodeModel();
    }

    emit p->fileListChanged();

    emit m_buildConfiguration->emitBuildTypeChanged();

    m_buildDirManager.resetData();

    qCDebug(cmakeBuildSystemLog) << "All CMake project data up to date.";
}

void CMakeBuildSystem::handleParsingSucceeded()
{
    if (!m_buildConfiguration->isActive()) {
        m_buildDirManager.stopParsingAndClearState();
        return;
    }

    m_buildConfiguration->clearError();

    QString errorMessage;
    {
        m_buildTargets = m_buildDirManager.takeBuildTargets(errorMessage);
        checkAndReportError(errorMessage);
    }

    {
        const CMakeConfig cmakeConfig = m_buildDirManager.takeCMakeConfiguration(errorMessage);
        checkAndReportError(errorMessage);
        m_buildConfiguration->setConfigurationFromCMake(cmakeConfig);
    }

    setApplicationTargets(appTargets());
    setDeploymentData(deploymentData());

    handleParsingSuccess();
}

void CMakeBuildSystem::handleParsingFailed(const QString &msg)
{
    m_buildConfiguration->setError(msg);

    QString errorMessage;
    m_buildConfiguration->setConfigurationFromCMake(m_buildDirManager.takeCMakeConfiguration(errorMessage));
    // ignore errorMessage here, we already got one.

    handleParsingError();
}

BuildConfiguration *CMakeBuildSystem::buildConfiguration() const
{
    return m_buildConfiguration;
}

CMakeBuildConfiguration *CMakeBuildSystem::cmakeBuildConfiguration() const
{
    return m_buildConfiguration;
}

const QList<BuildTargetInfo> CMakeBuildSystem::appTargets() const
{
    QList<BuildTargetInfo> appTargetList;
    const bool forAndroid = DeviceTypeKitAspect::deviceTypeId(target()->kit())
            == Android::Constants::ANDROID_DEVICE_TYPE;
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == UtilityType)
            continue;

        if (ct.targetType == ExecutableType || (forAndroid && ct.targetType == DynamicLibraryType)) {
            BuildTargetInfo bti;
            bti.displayName = ct.title;
            bti.targetFilePath = ct.executable;
            bti.projectFilePath = ct.sourceDirectory.stringAppended("/");
            bti.workingDirectory = ct.workingDirectory;
            bti.buildKey = ct.title;
            bti.usesTerminal = !ct.linksToQtGui;

            // Workaround for QTCREATORBUG-19354:
            bti.runEnvModifier = [this](Environment &env, bool) {
                if (HostOsInfo::isWindowsHost()) {
                    const Kit *k = target()->kit();
                    if (const QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k))
                        env.prependOrSetPath(qt->binPath().toString());
                }
            };

            appTargetList.append(bti);
        }
    }

    return appTargetList;
}

QStringList CMakeBuildSystem::buildTargetTitles() const
{
    return transform(m_buildTargets, &CMakeBuildTarget::title);
}

const QList<CMakeBuildTarget> &CMakeBuildSystem::buildTargets() const
{
    return m_buildTargets;
}

DeploymentData CMakeBuildSystem::deploymentData() const
{
    DeploymentData result;

    QDir sourceDir = target()->project()->projectDirectory().toString();
    QDir buildDir = buildConfiguration()->buildDirectory().toString();

    QString deploymentPrefix;
    QString deploymentFilePath = sourceDir.filePath("QtCreatorDeployment.txt");

    bool hasDeploymentFile = QFileInfo::exists(deploymentFilePath);
    if (!hasDeploymentFile) {
        deploymentFilePath = buildDir.filePath("QtCreatorDeployment.txt");
        hasDeploymentFile = QFileInfo::exists(deploymentFilePath);
    }
    if (!hasDeploymentFile)
        return result;

    deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath,
                                                         sourceDir.absolutePath());
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
            if (!ct.executable.isEmpty()
                    && result.deployableForLocalFile(ct.executable).localFilePath() != ct.executable) {
                result.addFile(ct.executable.toString(),
                               deploymentPrefix + buildDir.relativeFilePath(ct.executable.toFileInfo().dir().path()),
                               DeployableFile::TypeExecutable);
            }
        }
    }

    return result;
}

QList<ProjectExplorer::ExtraCompiler *> CMakeBuildSystem::findExtraCompilers()
{
    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: start.";

    QList<ProjectExplorer::ExtraCompiler *> extraCompilers;
    const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got factories.";

    const QSet<QString> fileExtensions = Utils::transform<QSet>(factories,
                                                                &ExtraCompilerFactory::sourceTag);

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got file extensions:"
                                 << fileExtensions;

    // Find all files generated by any of the extra compilers, in a rather crude way.
    Project *p = project();
    const FilePaths fileList = p->files([&fileExtensions, p](const Node *n) {
        if (!p->SourceFiles(n))
            return false;
        const QString fp = n->filePath().toString();
        const int pos = fp.lastIndexOf('.');
        return pos >= 0 && fileExtensions.contains(fp.mid(pos + 1));
    });

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got list of files to check.";

    // Generate the necessary information:
    for (const FilePath &file : fileList) {
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers: Processing" << file.toUserOutput();
        ExtraCompilerFactory *factory = Utils::findOrDefault(factories,
                                                             [&file](const ExtraCompilerFactory *f) {
                                                                 return file.endsWith(
                                                                     '.' + f->sourceTag());
                                                             });
        QTC_ASSERT(factory, continue);

        QStringList generated = filesGeneratedFrom(file.toString());
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     generated files:" << generated;
        if (generated.isEmpty())
            continue;

        const FilePaths fileNames = transform(generated, [](const QString &s) {
            return FilePath::fromString(s);
        });
        extraCompilers.append(factory->create(p, file, fileNames));
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     done with" << file.toUserOutput();
    }

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: done.";

    return extraCompilers;
}

void CMakeBuildSystem::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    if (!modelManager)
        return;

    Project *p = project();
    QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager
                                                                ->defaultProjectInfoForProject(p);

    projectInfo.importPaths.clear();

    const CMakeConfig &cm = m_buildConfiguration->configurationFromCMake();
    const QString cmakeImports = QString::fromUtf8(CMakeConfigItem::valueOf("QML_IMPORT_PATH", cm));

    foreach (const QString &cmakeImport, CMakeConfigItem::cmakeSplitValue(cmakeImports))
        projectInfo.importPaths.maybeInsert(FilePath::fromString(cmakeImport), QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, p);
}

} // namespace Internal
} // namespace CMakeProjectManager
