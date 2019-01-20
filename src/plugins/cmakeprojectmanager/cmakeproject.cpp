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
#include <utils/stringutils.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QSet>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

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
CMakeProject::CMakeProject(const FileName &fileName) : Project(Constants::CMAKEMIMETYPE, fileName),
    m_cppCodeModelUpdater(new CppTools::CppProjectUpdater)
{
    setId(CMakeProjectManager::Constants::CMAKEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());

    // Timer:
    m_delayedParsingTimer.setSingleShot(true);

    connect(&m_delayedParsingTimer, &QTimer::timeout,
            this, [this]() { startParsing(m_delayedParsingParameters); });

    // BuildDirManager:
    connect(&m_buildDirManager, &BuildDirManager::requestReparse,
            this, &CMakeProject::handleReparseRequest);
    connect(&m_buildDirManager, &BuildDirManager::dataAvailable,
            this, [this]() {
        CMakeBuildConfiguration *bc = activeBc(this);
        if (bc && bc == m_buildDirManager.buildConfiguration()) {
            bc->clearError();
            handleParsingSuccess(bc);
        }
    });
    connect(&m_buildDirManager, &BuildDirManager::errorOccured,
            this, [this](const QString &msg) {
        CMakeBuildConfiguration *bc = activeBc(this);
        if (bc && bc == m_buildDirManager.buildConfiguration()) {
            bc->setError(msg);
            bc->setConfigurationFromCMake(m_buildDirManager.takeCMakeConfiguration());
            handleParsingError(bc);
        }
    });
    connect(&m_buildDirManager, &BuildDirManager::parsingStarted,
            this, [this]() {
        CMakeBuildConfiguration *bc = activeBc(this);
        if (bc && bc == m_buildDirManager.buildConfiguration())
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
                        BuildDirManager::REPARSE_FAIL,
                        BuildDirManager::REPARSE_CHECK_CONFIGURATION);
        }
    });
    subscribeSignal(&CMakeBuildConfiguration::buildDirectoryChanged, this, [this]() {
        auto senderBc = qobject_cast<CMakeBuildConfiguration *>(sender());

        if (senderBc && senderBc->isActive() && senderBc == m_buildDirManager.buildConfiguration()) {
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

        if (senderBc && senderBc->isActive() && senderBc == m_buildDirManager.buildConfiguration()) {
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

    m_treeScanner.setFilter([this](const Utils::MimeType &mimeType, const Utils::FileName &fn) {
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

    m_treeScanner.setTypeFactory([](const Utils::MimeType &mimeType, const Utils::FileName &fn) {
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
    const CMakeBuildConfiguration *aBc = activeBc(this);

    QTC_ASSERT(bc, return);
    QTC_ASSERT(bc == aBc, return);
    QTC_ASSERT(m_treeScanner.isFinished() && !m_buildDirManager.isParsing(), return);

    bc->setBuildTargets(m_buildDirManager.takeBuildTargets());
    bc->setConfigurationFromCMake(m_buildDirManager.takeCMakeConfiguration());

    auto newRoot = generateProjectTree(m_allFiles);
    if (newRoot) {
        setDisplayName(newRoot->displayName());
        setRootProjectNode(std::move(newRoot));
    }

    Target *t = bc->target();
    t->setApplicationTargets(bc->appTargets());
    t->setDeploymentData(bc->deploymentData());

    t->updateDefaultRunConfigurations();

    qDeleteAll(m_extraCompilers);
    m_extraCompilers = findExtraCompilers();
    CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);

    QtSupport::CppKitInfo kitInfo(this);
    QTC_ASSERT(kitInfo.isValid(), return);

    CppTools::RawProjectParts rpps = m_buildDirManager.createRawProjectParts();

    for (CppTools::RawProjectPart &rpp : rpps) {
        rpp.setQtVersion(kitInfo.projectPartQtVersion); // TODO: Check if project actually uses Qt.
        if (kitInfo.cxxToolChain)
            rpp.setFlagsForCxx({kitInfo.cxxToolChain, rpp.flagsForCxx.commandLineFlags});
        if (kitInfo.cToolChain)
            rpp.setFlagsForC({kitInfo.cToolChain, rpp.flagsForC.commandLineFlags});
    }

    m_cppCodeModelUpdater->update({this, kitInfo, rpps});

    updateQmlJSCodeModel();

    m_buildDirManager.resetData();

    emit fileListChanged();

    bc->emitBuildTypeChanged();
}

void CMakeProject::updateQmlJSCodeModel()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    QTC_ASSERT(modelManager, return);

    if (!activeTarget() || !activeTarget()->activeBuildConfiguration())
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
        projectInfo.importPaths.maybeInsert(FileName::fromString(cmakeImport), QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, this);
}

std::unique_ptr<CMakeProjectNode>
CMakeProject::generateProjectTree(const QList<const FileNode *> &allFiles) const
{
    if (m_buildDirManager.isParsing())
        return nullptr;

    auto root = std::make_unique<CMakeProjectNode>(projectDirectory());
    m_buildDirManager.generateProjectTree(root.get(), allFiles);
    return root;
}

bool CMakeProject::knowsAllBuildExecutables() const
{
    return false;
}

QList<Task> CMakeProject::projectIssues(const Kit *k) const
{
    QList<Task> result = Project::projectIssues(k);

    if (!CMakeKitInformation::cmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, tr("No cmake tool set.")));
    if (ToolChainKitInformation::toolChains(k).isEmpty())
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
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION,
                                                   BuildDirManager::REPARSE_CHECK_CONFIGURATION);
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

    if (!bc || !bc->isActive())
        return;

    m_waitingForParse = false;
    m_combinedScanAndParseResult = m_combinedScanAndParseResult && true;

    combineScanAndParse(bc);
}

void CMakeProject::handleParsingError(CMakeBuildConfiguration *bc)
{
    QTC_CHECK(m_waitingForParse);

    if (!bc || !bc->isActive())
        return;

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

    emitParsingFinished(m_combinedScanAndParseResult);
}

QStringList CMakeProject::filesGeneratedFrom(const QString &sourceFile) const
{
    if (!activeTarget())
        return QStringList();
    QFileInfo fi(sourceFile);
    FileName project = projectDirectory();
    FileName baseDirectory = FileName::fromString(fi.absolutePath());

    while (baseDirectory.isChildOf(project)) {
        FileName cmakeListsTxt = baseDirectory;
        cmakeListsTxt.appendPath("CMakeLists.txt");
        if (cmakeListsTxt.exists())
            break;
        QDir dir(baseDirectory.toString());
        dir.cdUp();
        baseDirectory = FileName::fromString(dir.absolutePath());
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

bool CMakeProject::mustUpdateCMakeStateBeforeBuild()
{
    return m_delayedParsingTimer.isActive();
}

QList<ProjectExplorer::ExtraCompiler *> CMakeProject::findExtraCompilers() const
{
    QList<ProjectExplorer::ExtraCompiler *> extraCompilers;
    const QList<ExtraCompilerFactory *> factories =
            ExtraCompilerFactory::extraCompilerFactories();

    const QSet<QString> fileExtensions
            = Utils::transform<QSet>(factories, &ExtraCompilerFactory::sourceTag);

    // Find all files generated by any of the extra compilers, in a rather crude way.
    const FileNameList fileList = files([&fileExtensions](const Node *n) {
        if (!SourceFiles(n))
            return false;
        const QString fp = n->filePath().toString();
        const int pos = fp.lastIndexOf('.');
        return pos >= 0 && fileExtensions.contains(fp.mid(pos + 1));
    });

    // Generate the necessary information:
    for (const FileName &file : fileList) {
        ExtraCompilerFactory *factory = Utils::findOrDefault(factories, [&file](const ExtraCompilerFactory *f) {
            return file.endsWith('.' + f->sourceTag());
        });
        QTC_ASSERT(factory, continue);

        QStringList generated = filesGeneratedFrom(file.toString());
        if (generated.isEmpty())
            continue;

        const FileNameList fileNames
                = transform(generated,
                            [](const QString &s) { return FileName::fromString(s); });
        extraCompilers.append(factory->create(this, file, fileNames));
    }

    return extraCompilers;
}

} // namespace CMakeProjectManager
