// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildsystem.h"

#include "builddirparameters.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildtarget.h"
#include "cmakekitinformation.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "projecttreehelper.h"

#include <android/androidconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppprojectupdater.h>

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qtsupport/qtcppkitinfo.h>
#include <qtsupport/qtkitinformation.h>

#include <app/app_version.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

static void copySourcePathsToClipboard(const FilePaths &srcPaths, const ProjectNode *node)
{
    QClipboard *clip = QGuiApplication::clipboard();

    QString data = Utils::transform(srcPaths, [projDir = node->filePath()](const FilePath &path) {
                       return path.relativePathFrom(projDir).cleanPath().toString();
                   }).join(" ");
    clip->setText(data);
}

static void noAutoAdditionNotify(const FilePaths &filePaths, const ProjectNode *node)
{
    const FilePaths srcPaths = Utils::filtered(filePaths, [](const FilePath &file) {
        const auto mimeType = Utils::mimeTypeForFile(file).name();
        return mimeType == CppEditor::Constants::C_SOURCE_MIMETYPE ||
               mimeType == CppEditor::Constants::C_HEADER_MIMETYPE ||
               mimeType == CppEditor::Constants::CPP_SOURCE_MIMETYPE ||
               mimeType == CppEditor::Constants::CPP_HEADER_MIMETYPE ||
               mimeType == ProjectExplorer::Constants::FORM_MIMETYPE ||
               mimeType == ProjectExplorer::Constants::RESOURCE_MIMETYPE ||
               mimeType == ProjectExplorer::Constants::SCXML_MIMETYPE;
    });

    if (!srcPaths.empty()) {
        auto settings = CMakeSpecificSettings::instance();
        switch (settings->afterAddFileSetting.value()) {
        case AskUser: {
            bool checkValue{false};
            QDialogButtonBox::StandardButton reply = CheckableMessageBox::question(
                Core::ICore::dialogParent(),
                Tr::tr("Copy to Clipboard?"),
                Tr::tr("Files are not automatically added to the "
                                "CMakeLists.txt file of the CMake project."
                                "\nCopy the path to the source files to the clipboard?"),
                "Remember My Choice",
                &checkValue,
                QDialogButtonBox::Yes | QDialogButtonBox::No,
                QDialogButtonBox::Yes);
            if (checkValue) {
                if (QDialogButtonBox::Yes == reply)
                    settings->afterAddFileSetting.setValue(CopyFilePath);
                else if (QDialogButtonBox::No == reply)
                    settings->afterAddFileSetting.setValue(NeverCopyFilePath);

                settings->writeSettings(Core::ICore::settings());
            }

            if (QDialogButtonBox::Yes == reply)
                copySourcePathsToClipboard(srcPaths, node);

            break;
        }

        case CopyFilePath: {
            copySourcePathsToClipboard(srcPaths, node);
            break;
        }

        case NeverCopyFilePath:
            break;
        }
    }
}

static Q_LOGGING_CATEGORY(cmakeBuildSystemLog, "qtc.cmake.buildsystem", QtWarningMsg);

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

CMakeBuildSystem::CMakeBuildSystem(CMakeBuildConfiguration *bc)
    : BuildSystem(bc)
    , m_cppCodeModelUpdater(new CppEditor::CppProjectUpdater)
{
    // TreeScanner:
    connect(&m_treeScanner, &TreeScanner::finished,
            this, &CMakeBuildSystem::handleTreeScanningFinished);

    m_treeScanner.setFilter([this](const MimeType &mimeType, const FilePath &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored = TreeScanner::isWellKnownBinary(mimeType, fn);

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

    m_treeScanner.setTypeFactory([](const MimeType &mimeType, const FilePath &fn) {
        auto type = TreeScanner::genericFileType(mimeType, fn);
        if (type == FileType::Unknown) {
            if (mimeType.isValid()) {
                const QString mt = mimeType.name();
                if (mt == CMakeProjectManager::Constants::CMAKE_PROJECT_MIMETYPE
                    || mt == CMakeProjectManager::Constants::CMAKE_MIMETYPE)
                    type = FileType::Project;
            }
        }
        return type;
    });

    connect(&m_reader, &FileApiReader::configurationStarted, this, [this] {
        clearError(ForceEnabledChanged::True);
    });

    connect(&m_reader,
            &FileApiReader::dataAvailable,
            this,
            &CMakeBuildSystem::handleParsingSucceeded);
    connect(&m_reader, &FileApiReader::errorOccurred, this, &CMakeBuildSystem::handleParsingFailed);
    connect(&m_reader, &FileApiReader::dirty, this, &CMakeBuildSystem::becameDirty);

    wireUpConnections();

    m_isMultiConfig = CMakeGeneratorKitAspect::isMultiConfigGenerator(bc->kit());
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
}

void CMakeBuildSystem::triggerParsing()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName() << "Parsing has been triggered";

    if (!buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog)
            << "Parsing has been triggered: SKIPPING since BC is not active -- clearing state.";
        stopParsingAndClearState();
        return; // ignore request, this build configuration is not active!
    }

    auto guard = guardParsingRun();

    if (!guard.guardsProject()) {
        // This can legitimately trigger if e.g. Build->Run CMake
        // is selected while this here is already running.

        // Stop old parse run and keep that ParseGuard!
        qCDebug(cmakeBuildSystemLog) << "Stopping current parsing run!";
        stopParsingAndClearState();
    } else {
        // Use new ParseGuard
        m_currentGuard = std::move(guard);
    }
    QTC_ASSERT(!m_reader.isParsing(), return );

    qCDebug(cmakeBuildSystemLog) << "ParseGuard acquired.";

    int reparseParameters = takeReparseParameters();

    m_waitingForParse = true;
    m_combinedScanAndParseResult = true;

    QTC_ASSERT(m_parameters.isValid(), return );

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    qCDebug(cmakeBuildSystemLog) << "Parse called with flags:"
                                 << reparseParametersString(reparseParameters);

    const QString cache = m_parameters.buildDirectory.pathAppended("CMakeCache.txt").toString();
    if (!QFileInfo::exists(cache)) {
        reparseParameters |= REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN;
        qCDebug(cmakeBuildSystemLog)
            << "No" << cache
            << "file found, new flags:" << reparseParametersString(reparseParameters);
    }

    if ((0 == (reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION))
        && mustApplyConfigurationChangesArguments(m_parameters)) {
        reparseParameters |= REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION;
    }

    // The code model will be updated after the CMake run. There is no need to have an
    // active code model updater when the next one will be triggered.
    m_cppCodeModelUpdater->cancel();

    qCDebug(cmakeBuildSystemLog) << "Asking reader to parse";
    m_reader.parse(reparseParameters & REPARSE_FORCE_CMAKE_RUN,
                   reparseParameters & REPARSE_FORCE_INITIAL_CONFIGURATION,
                   reparseParameters & REPARSE_FORCE_EXTRA_CONFIGURATION);
}

bool CMakeBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (dynamic_cast<CMakeTargetNode *>(context))
        return action == ProjectAction::AddNewFile;

    if (dynamic_cast<CMakeListsNode *>(context))
        return action == ProjectAction::AddNewFile;

    return BuildSystem::supportsAction(context, action, node);
}

bool CMakeBuildSystem::addFiles(Node *context, const FilePaths &filePaths, FilePaths *notAdded)
{
    if (auto n = dynamic_cast<CMakeProjectNode *>(context)) {
        noAutoAdditionNotify(filePaths, n);
        return true; // Return always true as autoadd is not supported!
    }

    if (auto n = dynamic_cast<CMakeTargetNode *>(context)) {
        noAutoAdditionNotify(filePaths, n);
        return true; // Return always true as autoadd is not supported!
    }

    return BuildSystem::addFiles(context, filePaths, notAdded);
}

FilePaths CMakeBuildSystem::filesGeneratedFrom(const FilePath &sourceFile) const
{
    FilePath project = projectDirectory();
    FilePath baseDirectory = sourceFile.parentDir();

    while (baseDirectory.isChildOf(project)) {
        const FilePath cmakeListsTxt = baseDirectory.pathAppended("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        baseDirectory = baseDirectory.parentDir();
    }

    const FilePath relativePath = baseDirectory.relativePathFrom(project);
    FilePath generatedFilePath = buildConfiguration()->buildDirectory().resolvePath(relativePath);

    if (sourceFile.suffix() == "ui") {
        generatedFilePath = generatedFilePath
                                .pathAppended("ui_" + sourceFile.completeBaseName() + ".h");
        return {generatedFilePath};
    }
    if (sourceFile.suffix() == "scxml") {
        generatedFilePath = generatedFilePath.pathAppended(sourceFile.completeBaseName());
        return {generatedFilePath.stringAppended(".h"), generatedFilePath.stringAppended(".cpp")};
    }

    // TODO: Other types will be added when adapters for their compilers become available.
    return {};
}

QString CMakeBuildSystem::reparseParametersString(int reparseFlags)
{
    QString result;
    if (reparseFlags == REPARSE_DEFAULT) {
        result = "<NONE>";
    } else {
        if (reparseFlags & REPARSE_URGENT)
            result += " URGENT";
        if (reparseFlags & REPARSE_FORCE_CMAKE_RUN)
            result += " FORCE_CMAKE_RUN";
        if (reparseFlags & REPARSE_FORCE_INITIAL_CONFIGURATION)
            result += " FORCE_CONFIG";
    }
    return result.trimmed();
}

void CMakeBuildSystem::reparse(int reparseParameters)
{
    setParametersAndRequestParse(BuildDirParameters(this), reparseParameters);
}

void CMakeBuildSystem::setParametersAndRequestParse(const BuildDirParameters &parameters,
                                                    const int reparseParameters)
{
    project()->clearIssues();

    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "setting parameters and requesting reparse"
                                 << reparseParametersString(reparseParameters);

    if (!buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog) << "setting parameters and requesting reparse: SKIPPING since BC is not active -- clearing state.";
        stopParsingAndClearState();
        return; // ignore request, this build configuration is not active!
    }

    const CMakeTool *tool = parameters.cmakeTool();
    if (!tool || !tool->isValid()) {
        TaskHub::addTask(
            BuildSystemTask(Task::Error,
                            Tr::tr("The kit needs to define a CMake tool to parse this project.")));
        return;
    }
    if (!tool->hasFileApi()) {
        TaskHub::addTask(
            BuildSystemTask(Task::Error,
                            CMakeKitAspect::msgUnsupportedVersion(tool->version().fullVersion)));
        return;
    }
    QTC_ASSERT(parameters.isValid(), return );

    m_parameters = parameters;
    ensureBuildDirectory(parameters);
    updateReparseParameters(reparseParameters);

    m_reader.setParameters(m_parameters);

    if (reparseParameters & REPARSE_URGENT) {
        qCDebug(cmakeBuildSystemLog) << "calling requestReparse";
        requestParse();
    } else {
        qCDebug(cmakeBuildSystemLog) << "calling requestDelayedReparse";
        requestDelayedParse();
    }
}

bool CMakeBuildSystem::mustApplyConfigurationChangesArguments(const BuildDirParameters &parameters) const
{
    if (parameters.configurationChangesArguments.isEmpty())
        return false;

    int answer = QMessageBox::question(Core::ICore::dialogParent(),
                                       Tr::tr("Apply configuration changes?"),
                                       "<p>" + Tr::tr("Run CMake with configuration changes?")
                                       + "</p><pre>"
                                       + parameters.configurationChangesArguments.join("\n")
                                       + "</pre>",
                                       QMessageBox::Apply | QMessageBox::Discard,
                                       QMessageBox::Apply);
    return answer == QMessageBox::Apply;
}

void CMakeBuildSystem::runCMake()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due \"Run CMake\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeAndScanProjectTree()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_URGENT);
}

void CMakeBuildSystem::runCMakeWithExtraArguments()
{
    qCDebug(cmakeBuildSystemLog) << "Requesting parse due to \"Rescan Project\" command";
    reparse(REPARSE_FORCE_CMAKE_RUN | REPARSE_FORCE_EXTRA_CONFIGURATION | REPARSE_URGENT);
}

void CMakeBuildSystem::stopCMakeRun()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "stopping CMake's run";
    m_reader.stopCMakeRun();
}

void CMakeBuildSystem::buildCMakeTarget(const QString &buildTarget)
{
    QTC_ASSERT(!buildTarget.isEmpty(), return);
    if (ProjectExplorerPlugin::saveModifiedFiles())
        cmakeBuildConfiguration()->buildTarget(buildTarget);
}

bool CMakeBuildSystem::persistCMakeState()
{
    BuildDirParameters parameters(this);
    QTC_ASSERT(parameters.isValid(), return false);

    const bool hadBuildDirectory = parameters.buildDirectory.exists();
    ensureBuildDirectory(parameters);

    int reparseFlags = REPARSE_DEFAULT;
    qCDebug(cmakeBuildSystemLog) << "Checking whether build system needs to be persisted:"
                                 << "buildDir:" << parameters.buildDirectory
                                 << "Has extraargs:" << !parameters.configurationChangesArguments.isEmpty();

    if (mustApplyConfigurationChangesArguments(parameters)) {
        reparseFlags = REPARSE_FORCE_EXTRA_CONFIGURATION;
        qCDebug(cmakeBuildSystemLog) << "   -> must run CMake with extra arguments.";
    }
    if (!hadBuildDirectory) {
        reparseFlags = REPARSE_FORCE_INITIAL_CONFIGURATION;
        qCDebug(cmakeBuildSystemLog) << "   -> must run CMake with initial arguments.";
    }

    if (reparseFlags == REPARSE_DEFAULT)
        return false;

    qCDebug(cmakeBuildSystemLog) << "Requesting parse to persist CMake State";
    setParametersAndRequestParse(parameters,
                                 REPARSE_URGENT | REPARSE_FORCE_CMAKE_RUN | reparseFlags);
    return true;
}

void CMakeBuildSystem::clearCMakeCache()
{
    QTC_ASSERT(m_parameters.isValid(), return );
    QTC_ASSERT(!m_isHandlingError, return );

    stopParsingAndClearState();

    const FilePath pathsToDelete[] = {
        m_parameters.buildDirectory / "CMakeCache.txt",
        m_parameters.buildDirectory / "CMakeCache.txt.prev",
        m_parameters.buildDirectory / "CMakeFiles",
        m_parameters.buildDirectory / ".cmake/api/v1/reply",
        m_parameters.buildDirectory / ".cmake/api/v1/reply.prev",
        m_parameters.buildDirectory / Constants::PACKAGE_MANAGER_DIR
    };

    for (const FilePath &path : pathsToDelete)
        path.removeRecursively();

    emit configurationCleared();
}

void CMakeBuildSystem::combineScanAndParse(bool restoredFromBackup)
{
    if (buildConfiguration()->isActive()) {
        if (m_waitingForParse)
            return;

        if (m_combinedScanAndParseResult) {
            updateProjectData();
            m_currentGuard.markAsSuccess();

            if (restoredFromBackup)
                project()->addIssue(
                    CMakeProject::IssueType::Warning,
                    Tr::tr("<b>CMake configuration failed<b>"
                           "<p>The backup of the previous configuration has been restored.</p>"
                           "<p>Issues and \"Projects > Build\" settings "
                           "show more information about the failure.</p>"));

            m_reader.resetData();

            m_currentGuard = {};
            m_testNames.clear();

            emitBuildSystemUpdated();

            runCTest();
        } else {
            updateFallbackProjectData();

            project()->addIssue(CMakeProject::IssueType::Warning,
                                Tr::tr("<b>Failed to load project<b>"
                                       "<p>Issues and \"Projects > Build\" settings "
                                       "show more information about the failure.</p>"));
        }
    }
}

void CMakeBuildSystem::checkAndReportError(QString &errorMessage)
{
    if (!errorMessage.isEmpty()) {
        setError(errorMessage);
        errorMessage.clear();
    }
}

void CMakeBuildSystem::updateProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating CMake project data";

    QTC_ASSERT(m_treeScanner.isFinished() && !m_reader.isParsing(), return );

    buildConfiguration()->project()->setExtraProjectFiles(m_reader.projectFilesToWatch());

    CMakeConfig patchedConfig = configurationFromCMake();
    {
        QSet<QString> res;
        QStringList apps;
        for (const auto &target : std::as_const(m_buildTargets)) {
            if (target.targetType == DynamicLibraryType) {
                res.insert(target.executable.parentDir().toString());
                apps.push_back(target.executable.toUserOutput());
            }
            // ### shall we add also the ExecutableType ?
        }
        {
            CMakeConfigItem paths;
            paths.key = Android::Constants::ANDROID_SO_LIBS_PATHS;
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
        auto newRoot = m_reader.rootProjectNode();
        if (newRoot) {
            setRootProjectNode(std::move(newRoot));

            if (QTC_GUARD(p->rootProjectNode())) {
                const QString nodeName = p->rootProjectNode()->displayName();
                p->setDisplayName(nodeName);

                // set config on target nodes
                const QSet<QString> buildKeys = Utils::transform<QSet>(m_buildTargets,
                                                                       &CMakeBuildTarget::title);
                p->rootProjectNode()->forEachProjectNode(
                    [patchedConfig, buildKeys](const ProjectNode *node) {
                        if (buildKeys.contains(node->buildKey())) {
                            auto targetNode = const_cast<CMakeTargetNode *>(
                                dynamic_cast<const CMakeTargetNode *>(node));
                            if (QTC_GUARD(targetNode))
                                targetNode->setConfig(patchedConfig);
                        }
                    });
            }
        }
    }

    {
        qDeleteAll(m_extraCompilers);
        m_extraCompilers = findExtraCompilers();
        qCDebug(cmakeBuildSystemLog) << "Extra compilers created.";
    }

    QtSupport::CppKitInfo kitInfo(kit());
    QTC_ASSERT(kitInfo.isValid(), return );

    QString errorMessage;
    RawProjectParts rpps = m_reader.createRawProjectParts(errorMessage);
    if (!errorMessage.isEmpty())
        setError(errorMessage);
    qCDebug(cmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

    for (RawProjectPart &rpp : rpps) {
        rpp.setQtVersion(
                    kitInfo.projectPartQtVersion); // TODO: Check if project actually uses Qt.
        const FilePath includeFileBaseDir = buildConfiguration()->buildDirectory();
        QStringList cxxFlags = rpp.flagsForCxx.commandLineFlags;
        QStringList cFlags = rpp.flagsForC.commandLineFlags;
        addTargetFlagForIos(cxxFlags, cFlags, this, [this] {
            return m_configurationFromCMake.stringValueOf("CMAKE_OSX_DEPLOYMENT_TARGET");
        });
        if (kitInfo.cxxToolChain)
            rpp.setFlagsForCxx({kitInfo.cxxToolChain, cxxFlags, includeFileBaseDir});
        if (kitInfo.cToolChain)
            rpp.setFlagsForC({kitInfo.cToolChain, cFlags, includeFileBaseDir});
    }

    m_cppCodeModelUpdater->update({p, kitInfo, buildConfiguration()->environment(), rpps},
                                  m_extraCompilers);

    {
        const bool mergedHeaderPathsAndQmlImportPaths = kit()->value(
                    QtSupport::KitHasMergedHeaderPathsWithQmlImportPaths::id(), false).toBool();
        QStringList extraHeaderPaths;
        QList<QByteArray> moduleMappings;
        for (const RawProjectPart &rpp : std::as_const(rpps)) {
            FilePath moduleMapFile = buildConfiguration()->buildDirectory()
                    .pathAppended("qml_module_mappings/" + rpp.buildSystemTarget);
            if (expected_str<QByteArray> content = moduleMapFile.fileContents()) {
                auto lines = content->split('\n');
                for (const QByteArray &line : lines) {
                    if (!line.isEmpty())
                        moduleMappings.append(line.simplified());
                }
            }

            if (mergedHeaderPathsAndQmlImportPaths) {
                for (const auto &headerPath : rpp.headerPaths) {
                    if (headerPath.type == HeaderPathType::User || headerPath.type == HeaderPathType::System)
                        extraHeaderPaths.append(headerPath.path);
                }
            }
        }
        updateQmlJSCodeModel(extraHeaderPaths, moduleMappings);
    }
    updateInitialCMakeExpandableVars();

    emit buildConfiguration()->buildTypeChanged();

    qCDebug(cmakeBuildSystemLog) << "All CMake project data up to date.";
}

void CMakeBuildSystem::handleTreeScanningFinished()
{
    TreeScanner::Result result = m_treeScanner.release();
    m_allFiles = result.folderNode;
    qDeleteAll(result.allFiles);

    updateFileSystemNodes();
}

void CMakeBuildSystem::updateFileSystemNodes()
{
    auto newRoot = std::make_unique<CMakeProjectNode>(m_parameters.sourceDirectory);
    newRoot->setDisplayName(m_parameters.sourceDirectory.fileName());

    if (!m_reader.topCmakeFile().isEmpty()) {
        auto node = std::make_unique<FileNode>(m_reader.topCmakeFile(), FileType::Project);
        node->setIsGenerated(false);

        std::vector<std::unique_ptr<FileNode>> fileNodes;
        fileNodes.emplace_back(std::move(node));

        addCMakeLists(newRoot.get(), std::move(fileNodes));
    }

    if (m_allFiles)
        addFileSystemNodes(newRoot.get(), m_allFiles);
    setRootProjectNode(std::move(newRoot));

    m_reader.resetData();

    m_currentGuard = {};
    emitBuildSystemUpdated();

    qCDebug(cmakeBuildSystemLog) << "All fallback CMake project data up to date.";
}

void CMakeBuildSystem::updateFallbackProjectData()
{
    qCDebug(cmakeBuildSystemLog) << "Updating fallback CMake project data";
    qCDebug(cmakeBuildSystemLog) << "Starting TreeScanner";
    QTC_CHECK(m_treeScanner.isFinished());
    if (m_treeScanner.asyncScanForFiles(projectDirectory()))
        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       Tr::tr("Scan \"%1\" project tree")
                                           .arg(project()->displayName()),
                                       "CMake.Scan.Tree");
}

void CMakeBuildSystem::updateCMakeConfiguration(QString &errorMessage)
{
    CMakeConfig cmakeConfig = m_reader.takeParsedConfiguration(errorMessage);
    for (auto &ci : cmakeConfig)
        ci.inCMakeCache = true;
    if (!errorMessage.isEmpty()) {
        const CMakeConfig changes = configurationChanges();
        for (const auto &ci : changes) {
            if (ci.isInitial)
                continue;
            const bool haveConfigItem = Utils::contains(cmakeConfig, [ci](const CMakeConfigItem& i) {
                return i.key == ci.key;
            });
            if (!haveConfigItem)
                cmakeConfig.append(ci);
        }
    }
    setConfigurationFromCMake(cmakeConfig);
}

void CMakeBuildSystem::handleParsingSucceeded(bool restoredFromBackup)
{
    if (!buildConfiguration()->isActive()) {
        stopParsingAndClearState();
        return;
    }

    clearError();

    QString errorMessage;
    {
        m_buildTargets = Utils::transform(CMakeBuildStep::specialTargets(m_reader.usesAllCapsTargets()), [this](const QString &t) {
            CMakeBuildTarget result;
            result.title = t;
            result.workingDirectory = m_parameters.buildDirectory;
            result.sourceDirectory = m_parameters.sourceDirectory;
            return result;
        });
        m_buildTargets += m_reader.takeBuildTargets(errorMessage);
        checkAndReportError(errorMessage);
    }

    {
        updateCMakeConfiguration(errorMessage);
        checkAndReportError(errorMessage);
    }

    if (const CMakeTool *tool = m_parameters.cmakeTool())
        m_ctestPath = tool->cmakeExecutable().withNewPath(m_reader.ctestPath());

    setApplicationTargets(appTargets());

    // Note: This is practically always wrong and resulting in an empty view.
    // Setting the real data is triggered from a successful run of a
    // MakeInstallStep.
    setDeploymentData(deploymentDataFromFile());

    QTC_ASSERT(m_waitingForParse, return );
    m_waitingForParse = false;

    combineScanAndParse(restoredFromBackup);
}

void CMakeBuildSystem::handleParsingFailed(const QString &msg)
{
    setError(msg);

    QString errorMessage;
    updateCMakeConfiguration(errorMessage);
    // ignore errorMessage here, we already got one.

    m_ctestPath.clear();

    QTC_CHECK(m_waitingForParse);
    m_waitingForParse = false;
    m_combinedScanAndParseResult = false;

    combineScanAndParse(false);
}

void CMakeBuildSystem::wireUpConnections()
{
    // At this point the entire project will be fully configured, so let's connect everything and
    // trigger an initial parser run

    // Became active/inactive:
    connect(target(), &Target::activeBuildConfigurationChanged, this, [this] {
        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active BC changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });
    connect(project(), &Project::activeTargetChanged, this, [this] {
        // Build configuration has changed:
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to active target changed";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    });

    // BuildConfiguration changed:
    connect(buildConfiguration(), &BuildConfiguration::environmentChanged, this, [this] {
        // The environment on our BC has changed, force CMake run to catch up with possible changes
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to environment change";
        reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
    });
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, this, [this] {
        // The build directory of our BC has changed:
        // Does the directory contain a CMakeCache ? Existing build, just parse
        // No CMakeCache? Run with initial arguments!
        qCDebug(cmakeBuildSystemLog) << "Requesting parse due to build directory change";
        const BuildDirParameters parameters(this);
        const FilePath cmakeCacheTxt = parameters.buildDirectory.pathAppended("CMakeCache.txt");
        const bool hasCMakeCache = cmakeCacheTxt.exists();
        const auto options = ReparseParameters(
                    hasCMakeCache
                    ? REPARSE_DEFAULT
                    : (REPARSE_FORCE_INITIAL_CONFIGURATION | REPARSE_FORCE_CMAKE_RUN));
        if (hasCMakeCache) {
            QString errorMessage;
            const CMakeConfig config = CMakeConfig::fromFile(cmakeCacheTxt, &errorMessage);
            if (!config.isEmpty() && errorMessage.isEmpty()) {
                QString cmakeBuildTypeName = config.stringValueOf("CMAKE_BUILD_TYPE");
                setCMakeBuildType(cmakeBuildTypeName, true);
            }
        }
        reparse(options);
    });

    connect(project(), &Project::projectFileIsDirty, this, [this] {
        if (buildConfiguration()->isActive() && !isParsing()) {
            auto settings = CMakeSpecificSettings::instance();
            if (settings->autorunCMake.value()) {
                qCDebug(cmakeBuildSystemLog) << "Requesting parse due to dirty project file";
                reparse(CMakeBuildSystem::REPARSE_FORCE_CMAKE_RUN);
            }
        }
    });

    // Force initial parsing run:
    if (buildConfiguration()->isActive()) {
        qCDebug(cmakeBuildSystemLog) << "Initial run:";
        reparse(CMakeBuildSystem::REPARSE_DEFAULT);
    }
}

void CMakeBuildSystem::ensureBuildDirectory(const BuildDirParameters &parameters)
{
    const FilePath bdir = parameters.buildDirectory;

    if (!buildConfiguration()->createBuildDirectory()) {
        handleParsingFailed(Tr::tr("Failed to create build directory \"%1\".").arg(bdir.toUserOutput()));
        return;
    }

    const CMakeTool *tool = parameters.cmakeTool();
    if (!tool) {
        handleParsingFailed(Tr::tr("No CMake tool set up in kit."));
        return;
    }

    if (tool->cmakeExecutable().needsDevice()) {
        if (!tool->cmakeExecutable().ensureReachable(bdir)) {
            // Make sure that the build directory is available on the device.
            handleParsingFailed(
                Tr::tr("The remote CMake executable cannot write to the local build directory."));
        }
    }
}

void CMakeBuildSystem::stopParsingAndClearState()
{
    qCDebug(cmakeBuildSystemLog) << buildConfiguration()->displayName()
                                 << "stopping parsing run!";
    m_reader.stop();
    m_reader.resetData();
}

void CMakeBuildSystem::becameDirty()
{
    qCDebug(cmakeBuildSystemLog) << "CMakeBuildSystem: becameDirty was triggered.";
    if (isParsing())
        return;

    reparse(REPARSE_DEFAULT);
}

void CMakeBuildSystem::updateReparseParameters(const int parameters)
{
    m_reparseParameters |= parameters;
}

int CMakeBuildSystem::takeReparseParameters()
{
    int result = m_reparseParameters;
    m_reparseParameters = REPARSE_DEFAULT;
    return result;
}

void CMakeBuildSystem::runCTest()
{
    if (!m_error.isEmpty() || m_ctestPath.isEmpty()) {
        qCDebug(cmakeBuildSystemLog) << "Cancel ctest run after failed cmake run";
        emit testInformationUpdated();
        return;
    }
    qCDebug(cmakeBuildSystemLog) << "Requesting ctest run after cmake run";

    const BuildDirParameters parameters(this);
    QTC_ASSERT(parameters.isValid(), return);

    ensureBuildDirectory(parameters);
    m_ctestProcess.reset(new QtcProcess);
    m_ctestProcess->setEnvironment(buildConfiguration()->environment());
    m_ctestProcess->setWorkingDirectory(parameters.buildDirectory);
    m_ctestProcess->setCommand({m_ctestPath, { "-N", "--show-only=json-v1"}});
    connect(m_ctestProcess.get(), &QtcProcess::done, this, [this] {
        if (m_ctestProcess->result() == ProcessResult::FinishedWithSuccess) {
            const QJsonDocument json
                = QJsonDocument::fromJson(m_ctestProcess->readAllRawStandardOutput());
            if (!json.isEmpty() && json.isObject()) {
                const QJsonObject jsonObj = json.object();
                const QJsonObject btGraph = jsonObj.value("backtraceGraph").toObject();
                const QJsonArray cmakelists = btGraph.value("files").toArray();
                const QJsonArray nodes = btGraph.value("nodes").toArray();
                const QJsonArray tests = jsonObj.value("tests").toArray();
                int counter = 0;
                for (const QJsonValue &testVal : tests) {
                    ++counter;
                    const QJsonObject test = testVal.toObject();
                    QTC_ASSERT(!test.isEmpty(), continue);
                    int file = -1;
                    int line = -1;
                    const int bt = test.value("backtrace").toInt(-1);
                    // we may have no real backtrace due to different registering
                    if (bt != -1) {
                        QSet<int> seen;
                        std::function<QJsonObject(int)> findAncestor = [&](int index){
                            const QJsonObject node = nodes.at(index).toObject();
                            const int parent = node.value("parent").toInt(-1);
                            if (seen.contains(parent) || parent < 0)
                                return node;
                            seen << parent;
                            return findAncestor(parent);
                        };
                        const QJsonObject btRef = findAncestor(bt);
                        file = btRef.value("file").toInt(-1);
                        line = btRef.value("line").toInt(-1);
                    }
                    // we may have no CMakeLists.txt file reference due to different registering
                    const FilePath cmakeFile = file != -1
                            ? FilePath::fromString(cmakelists.at(file).toString()) : FilePath();
                    m_testNames.append({ test.value("name").toString(), counter, cmakeFile, line });
                }
            }
        }
        emit testInformationUpdated();
    });
    m_ctestProcess->start();
}

CMakeBuildConfiguration *CMakeBuildSystem::cmakeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(BuildSystem::buildConfiguration());
}

static FilePaths librarySearchPaths(const CMakeBuildSystem *bs, const QString &buildKey)
{
    const CMakeBuildTarget cmakeBuildTarget
        = Utils::findOrDefault(bs->buildTargets(), Utils::equal(&CMakeBuildTarget::title, buildKey));

    return cmakeBuildTarget.libraryDirectories;
}

const QList<BuildTargetInfo> CMakeBuildSystem::appTargets() const
{
    QList<BuildTargetInfo> appTargetList;
    const bool forAndroid = DeviceTypeKitAspect::deviceTypeId(kit())
                            == Android::Constants::ANDROID_DEVICE_TYPE;
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (CMakeBuildSystem::filteredOutTarget(ct))
            continue;

        if (ct.targetType == ExecutableType || (forAndroid && ct.targetType == DynamicLibraryType)) {
            const QString buildKey = ct.title;

            BuildTargetInfo bti;
            bti.displayName = ct.title;
            bti.targetFilePath = ct.executable;
            bti.projectFilePath = ct.sourceDirectory.cleanPath();
            bti.workingDirectory = ct.workingDirectory;
            bti.buildKey = buildKey;
            bti.usesTerminal = !ct.linksToQtGui;
            bti.isQtcRunnable = ct.qtcRunnable;

            // Workaround for QTCREATORBUG-19354:
            bti.runEnvModifier = [this, buildKey](Environment &env, bool enabled) {
                if (enabled)
                    env.prependOrSetLibrarySearchPaths(librarySearchPaths(this, buildKey));
            };

            appTargetList.append(bti);
        }
    }

    return appTargetList;
}

QStringList CMakeBuildSystem::buildTargetTitles() const
{
    auto nonAutogenTargets = filtered(m_buildTargets, [](const CMakeBuildTarget &target){
        return !CMakeBuildSystem::filteredOutTarget(target);
    });
    return transform(nonAutogenTargets, &CMakeBuildTarget::title);
}

const QList<CMakeBuildTarget> &CMakeBuildSystem::buildTargets() const
{
    return m_buildTargets;
}

bool CMakeBuildSystem::filteredOutTarget(const CMakeBuildTarget &target)
{
    return target.title.endsWith("_autogen") ||
           target.title.endsWith("_autogen_timestamp_deps");
}

bool CMakeBuildSystem::isMultiConfig() const
{
    return m_isMultiConfig;
}

void CMakeBuildSystem::setIsMultiConfig(bool isMultiConfig)
{
    m_isMultiConfig = isMultiConfig;
}

bool CMakeBuildSystem::isMultiConfigReader() const
{
    return m_reader.isMultiConfig();
}

bool CMakeBuildSystem::usesAllCapsTargets() const
{
    return m_reader.usesAllCapsTargets();
}

CMakeProject *CMakeBuildSystem::project() const
{
    return static_cast<CMakeProject *>(ProjectExplorer::BuildSystem::project());
}

const QList<TestCaseInfo> CMakeBuildSystem::testcasesInfo() const
{
    return m_testNames;
}

CommandLine CMakeBuildSystem::commandLineForTests(const QList<QString> &tests,
                                                  const QStringList &options) const
{
    QStringList args = options;
    const QSet<QString> testsSet = Utils::toSet(tests);
    auto current = Utils::transform<QSet<QString>>(m_testNames, &TestCaseInfo::name);
    if (tests.isEmpty() || current == testsSet)
        return {m_ctestPath, args};

    QString testNumbers("0,0,0"); // start, end, stride
    for (const TestCaseInfo &info : m_testNames) {
        if (testsSet.contains(info.name))
            testNumbers += QString(",%1").arg(info.number);
    }
    args << "-I" << testNumbers;
    return {m_ctestPath, args};
}

DeploymentData CMakeBuildSystem::deploymentDataFromFile() const
{
    DeploymentData result;

    FilePath sourceDir = project()->projectDirectory();
    FilePath buildDir = buildConfiguration()->buildDirectory();

    QString deploymentPrefix;
    FilePath deploymentFilePath = sourceDir.pathAppended("QtCreatorDeployment.txt");

    bool hasDeploymentFile = deploymentFilePath.exists();
    if (!hasDeploymentFile) {
        deploymentFilePath = buildDir.pathAppended("QtCreatorDeployment.txt");
        hasDeploymentFile = deploymentFilePath.exists();
    }
    if (!hasDeploymentFile)
        return result;

    deploymentPrefix = result.addFilesFromDeploymentFile(deploymentFilePath, sourceDir);
    for (const CMakeBuildTarget &ct : m_buildTargets) {
        if (ct.targetType == ExecutableType || ct.targetType == DynamicLibraryType) {
            if (!ct.executable.isEmpty()
                    && result.deployableForLocalFile(ct.executable).localFilePath() != ct.executable) {
                result.addFile(ct.executable,
                               deploymentPrefix + buildDir.relativeChildPath(ct.executable).toString(),
                               DeployableFile::TypeExecutable);
            }
        }
    }

    return result;
}

QList<ExtraCompiler *> CMakeBuildSystem::findExtraCompilers()
{
    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: start.";

    QList<ExtraCompiler *> extraCompilers;
    const QList<ExtraCompilerFactory *> factories = ExtraCompilerFactory::extraCompilerFactories();

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got factories.";

    const QSet<QString> fileExtensions = Utils::transform<QSet>(factories,
                                                                &ExtraCompilerFactory::sourceTag);

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: Got file extensions:"
                                 << fileExtensions;

    // Find all files generated by any of the extra compilers, in a rather crude way.
    Project *p = project();
    const FilePaths fileList = p->files([&fileExtensions](const Node *n) {
        if (!Project::SourceFiles(n) || !n->isEnabled()) // isEnabled excludes nodes from the file system tree
            return false;
        const QString suffix = n->filePath().suffix();
        return !suffix.isEmpty() && fileExtensions.contains(suffix);
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

        FilePaths generated = filesGeneratedFrom(file);
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     generated files:" << generated;
        if (generated.isEmpty())
            continue;

        extraCompilers.append(factory->create(p, file, generated));
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     done with" << file.toUserOutput();
    }

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: done.";

    return extraCompilers;
}

void CMakeBuildSystem::updateQmlJSCodeModel(const QStringList &extraHeaderPaths,
                                            const QList<QByteArray> &moduleMappings)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    if (!modelManager)
        return;

    Project *p = project();
    QmlJS::ModelManagerInterface::ProjectInfo projectInfo
        = modelManager->defaultProjectInfoForProject(p, p->files(Project::HiddenRccFolders));

    projectInfo.importPaths.clear();

    auto addImports = [&projectInfo](const QString &imports) {
        const QStringList importList = CMakeConfigItem::cmakeSplitValue(imports);
        for (const QString &import : importList)
            projectInfo.importPaths.maybeInsert(FilePath::fromString(import), QmlJS::Dialect::Qml);
    };

    const CMakeConfig &cm = configurationFromCMake();
    addImports(cm.stringValueOf("QML_IMPORT_PATH"));
    addImports(kit()->value(QtSupport::KitQmlImportPath::id()).toString());

    for (const QString &extraHeaderPath : extraHeaderPaths)
        projectInfo.importPaths.maybeInsert(FilePath::fromString(extraHeaderPath),
                                            QmlJS::Dialect::Qml);

    for (const QByteArray &mm : moduleMappings) {
        auto kvPair = mm.split('=');
        if (kvPair.size() != 2)
            continue;
        QString from = QString::fromUtf8(kvPair.at(0).trimmed());
        QString to = QString::fromUtf8(kvPair.at(1).trimmed());
        if (!from.isEmpty() && !to.isEmpty() && from != to) {
            // The QML code-model does not support sub-projects, so if there are multiple mappings for a single module,
            // choose the shortest one.
            if (projectInfo.moduleMappings.contains(from)) {
                if (to.size() < projectInfo.moduleMappings.value(from).size())
                    projectInfo.moduleMappings.insert(from, to);
            } else {
                projectInfo.moduleMappings.insert(from, to);
            }
        }
    }

    project()->setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID,
                                  !projectInfo.sourceFiles.isEmpty());
    modelManager->updateProjectInfo(projectInfo, p);
}

void CMakeBuildSystem::updateInitialCMakeExpandableVars()
{
    const CMakeConfig &cm = configurationFromCMake();
    const CMakeConfig &initialConfig = initialCMakeConfiguration();

    CMakeConfig config;

    const FilePath projectDirectory = project()->projectDirectory();
    const auto samePath = [projectDirectory](const FilePath &first, const FilePath &second) {
        // if a path is relative, resolve it relative to the project directory
        // this is not 100% correct since CMake resolve them to CMAKE_CURRENT_SOURCE_DIR
        // depending on context, but we cannot do better here
        return first == second
               || projectDirectory.resolvePath(first)
                      == projectDirectory.resolvePath(second)
               || projectDirectory.resolvePath(first).canonicalPath()
                      == projectDirectory.resolvePath(second).canonicalPath();
    };

    // Replace path values that do not  exist on file system
    const QByteArrayList singlePathList = {
        "CMAKE_C_COMPILER",
        "CMAKE_CXX_COMPILER",
        "QT_QMAKE_EXECUTABLE",
        "QT_HOST_PATH",
        "CMAKE_PROJECT_INCLUDE_BEFORE",
        "CMAKE_TOOLCHAIN_FILE"
    };
    for (const auto &var : singlePathList) {
        auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const CMakeConfigItem &item) {
            return item.key == var && !item.isInitial;
        });

        if (it != cm.cend()) {
            const QByteArray initialValue = initialConfig.expandedValueOf(kit(), var).toUtf8();
            const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));
            const FilePath path = FilePath::fromUserInput(QString::fromUtf8(it->value));

            if (!initialValue.isEmpty() && !samePath(path, initialPath) && !path.exists()) {
                CMakeConfigItem item(*it);
                item.value = initialValue;

                config << item;
            }
        }
    }

    // Prepend new values to existing path lists
    const QByteArrayList multiplePathList = {
        "CMAKE_PREFIX_PATH",
        "CMAKE_FIND_ROOT_PATH"
    };
    for (const auto &var : multiplePathList) {
        auto it = std::find_if(cm.cbegin(), cm.cend(), [var](const CMakeConfigItem &item) {
            return item.key == var && !item.isInitial;
        });

        if (it != cm.cend()) {
            const QByteArrayList initialValueList = initialConfig.expandedValueOf(kit(), var).toUtf8().split(';');

            for (const auto &initialValue: initialValueList) {
                const FilePath initialPath = FilePath::fromUserInput(QString::fromUtf8(initialValue));

                const bool pathIsContained
                    = Utils::contains(it->value.split(';'), [samePath, initialPath](const QByteArray &p) {
                          return samePath(FilePath::fromUserInput(QString::fromUtf8(p)), initialPath);
                      });
                if (!initialValue.isEmpty() && !pathIsContained) {
                    CMakeConfigItem item(*it);
                    item.value = initialValue;
                    item.value.append(";");
                    item.value.append(it->value);

                    config << item;
                }
            }
        }
    }

    if (!config.isEmpty())
        emit configurationChanged(config);
}

MakeInstallCommand CMakeBuildSystem::makeInstallCommand(const FilePath &installRoot) const
{
    MakeInstallCommand cmd;
    if (CMakeTool *tool = CMakeKitAspect::cmakeTool(target()->kit()))
        cmd.command.setExecutable(tool->cmakeExecutable());

    QString installTarget = "install";
    if (usesAllCapsTargets())
        installTarget = "INSTALL";

    FilePath buildDirectory = ".";
    if (auto bc = buildConfiguration())
        buildDirectory = bc->buildDirectory();

    cmd.command.addArg("--build");
    cmd.command.addArg(buildDirectory.onDevice(cmd.command.executable()).path());
    cmd.command.addArg("--target");
    cmd.command.addArg(installTarget);

    if (isMultiConfigReader())
        cmd.command.addArgs({"--config", cmakeBuildType()});

    cmd.environment.set("DESTDIR", installRoot.nativePath());
    return cmd;
}

QList<QPair<Id, QString>> CMakeBuildSystem::generators() const
{
    if (!buildConfiguration())
        return {};
    const CMakeTool * const cmakeTool
            = CMakeKitAspect::cmakeTool(buildConfiguration()->target()->kit());
    if (!cmakeTool)
        return {};
    QList<QPair<Id, QString>> result;
    const QList<CMakeTool::Generator> &generators = cmakeTool->supportedGenerators();
    for (const CMakeTool::Generator &generator : generators) {
        result << qMakePair(Id::fromSetting(generator.name),
                            Tr::tr("%1 (via cmake)").arg(generator.name));
        for (const QString &extraGenerator : generator.extraGenerators) {
            const QString displayName = extraGenerator + " - " + generator.name;
            result << qMakePair(Id::fromSetting(displayName),
                                Tr::tr("%1 (via cmake)").arg(displayName));
        }
    }
    return result;
}

void CMakeBuildSystem::runGenerator(Id id)
{
    QTC_ASSERT(cmakeBuildConfiguration(), return);
    const auto showError = [](const QString &detail) {
        Core::MessageManager::writeDisrupting(Tr::tr("cmake generator failed: %1.").arg(detail));
    };
    const CMakeTool * const cmakeTool
            = CMakeKitAspect::cmakeTool(buildConfiguration()->target()->kit());
    if (!cmakeTool) {
        showError(Tr::tr("Kit does not have a cmake binary set"));
        return;
    }
    const QString generator = id.toSetting().toString();
    const FilePath outDir = buildConfiguration()->buildDirectory()
            / ("qtc_" + FileUtils::fileSystemFriendlyName(generator));
    if (!outDir.ensureWritableDir()) {
        showError(Tr::tr("Cannot create output directory \"%1\"").arg(outDir.toString()));
        return;
    }
    CommandLine cmdLine(cmakeTool->cmakeExecutable(), {"-S", buildConfiguration()->target()
                        ->project()->projectDirectory().toUserOutput(), "-G", generator});
    if (!cmdLine.executable().isExecutableFile()) {
        showError(Tr::tr("No valid cmake executable"));
        return;
    }
    const auto itemFilter = [](const CMakeConfigItem &item) {
        return !item.isNull()
                && item.type != CMakeConfigItem::STATIC
                && item.type != CMakeConfigItem::INTERNAL
                && !item.key.contains("GENERATOR");
    };
    QList<CMakeConfigItem> configItems = Utils::filtered(m_configurationChanges.toList(),
                                                         itemFilter);
    const QList<CMakeConfigItem> initialConfigItems
            = Utils::filtered(initialCMakeConfiguration().toList(), itemFilter);
    for (const CMakeConfigItem &item : std::as_const(initialConfigItems)) {
        if (!Utils::contains(configItems, [&item](const CMakeConfigItem &existingItem) {
            return existingItem.key == item.key;
        })) {
            configItems << item;
        }
    }
    for (const CMakeConfigItem &item : std::as_const(configItems))
        cmdLine.addArg(item.toArgument(buildConfiguration()->macroExpander()));
    if (const auto optionsAspect = buildConfiguration()->aspect<AdditionalCMakeOptionsAspect>();
            optionsAspect && !optionsAspect->value().isEmpty()) {
        cmdLine.addArgs(optionsAspect->value(), CommandLine::Raw);
    }
    const auto proc = new QtcProcess(this);
    connect(proc, &QtcProcess::done, proc, &QtcProcess::deleteLater);
    connect(proc, &QtcProcess::readyReadStandardOutput, this, [proc] {
        Core::MessageManager::writeFlashing(QString::fromLocal8Bit(proc->readAllRawStandardOutput()));
    });
    connect(proc, &QtcProcess::readyReadStandardError, this, [proc] {
        Core::MessageManager::writeDisrupting(QString::fromLocal8Bit(proc->readAllRawStandardError()));
    });
    proc->setWorkingDirectory(outDir);
    proc->setEnvironment(buildConfiguration()->environment());
    proc->setCommand(cmdLine);
    Core::MessageManager::writeFlashing(Tr::tr("Running in %1: %2")
                                        .arg(outDir.toUserOutput(), cmdLine.toUserOutput()));
    proc->start();
}

ExtraCompiler *CMakeBuildSystem::findExtraCompiler(const ExtraCompilerFilter &filter) const
{
    return Utils::findOrDefault(m_extraCompilers, filter);
}

} // CMakeProjectManager::Internal
