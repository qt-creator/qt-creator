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

#if 0
#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"

#include <cpptools/cpprawprojectpart.h>
#include <cpptools/cpptoolsconstants.h>

#include <cpptools/projectinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QElapsedTimer>
#include <QSet>
#endif

#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppprojectupdater.h>
#include <cpptools/generatedcodemodelsupport.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qtsupport/qtcppkitinfo.h>

#include <utils/fileutils.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace {

CMakeProjectManager::Internal::CMakeBuildConfiguration *activeBc(Project *p)
{
    if (!p)
        return nullptr;

    return qobject_cast<CMakeProjectManager::Internal::CMakeBuildConfiguration *>(
        p->activeTarget() ? p->activeTarget()->activeBuildConfiguration() : nullptr);
}

} // namespace

namespace CMakeProjectManager {

using namespace Internal;

Q_LOGGING_CATEGORY(cmakeBuildSystemLog, "qtc.cmake.buildsystem", QtWarningMsg);

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

BuildSystem::BuildSystem(Project *project)
    : m_project(project)
{
    QTC_CHECK(project);

    // Timer:
    m_delayedParsingTimer.setSingleShot(true);

    connect(&m_delayedParsingTimer, &QTimer::timeout, this, &BuildSystem::triggerParsing);
}

BuildSystem::~BuildSystem() = default;

Project *BuildSystem::project() const
{
    return m_project;
}

bool BuildSystem::isWaitingForParse() const
{
    return m_delayedParsingTimer.isActive();
}

void BuildSystem::requestParse(int reparseParameters)
{
    QTC_ASSERT(!(reparseParameters & PARAM_ERROR), return );
    if (reparseParameters & PARAM_IGNORE)
        return;

    m_delayedParsingTimer.setInterval((reparseParameters & PARAM_URGENT) ? 0 : 1000);
    m_delayedParsingTimer.start();
    m_delayedParsingParameters = m_delayedParsingParameters | reparseParameters;
}

void BuildSystem::triggerParsing()
{
    int parameters = m_delayedParsingParameters;
    m_delayedParsingParameters = BuildSystem::PARAM_DEFAULT;

    QTC_CHECK(!m_project->isParsing());
    QTC_ASSERT((parameters & BuildSystem::PARAM_ERROR) == 0, return );
    if (parameters & BuildSystem::PARAM_IGNORE)
        return;

    // Clear buildsystem specific parameters before passing them on!
    parameters = parameters
                 & ~(BuildSystem::PARAM_ERROR | BuildSystem::PARAM_IGNORE
                     | BuildSystem::PARAM_URGENT);

    {
        ParsingContext ctx(m_project->guardParsingRun(), parameters, m_project, activeBc(m_project));
        if (validateParsingContext(ctx))
            parseProject(std::move(ctx));
    }
}

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

CMakeBuildSystem::CMakeBuildSystem(CMakeProject *p)
    : BuildSystem(p)
    , m_cppCodeModelUpdater(new CppTools::CppProjectUpdater)
{
    // TreeScanner:
    connect(&m_treeScanner,
            &TreeScanner::finished,
            this,
            &CMakeBuildSystem::handleTreeScanningFinished);

    m_treeScanner.setFilter([this, p](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
        // Mime checks requires more resources, so keep it last in check list
        auto isIgnored = fn.toString().startsWith(p->projectFilePath().toString() + ".user")
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

bool CMakeBuildSystem::validateParsingContext(const ParsingContext &ctx)
{
    return ctx.project && qobject_cast<CMakeBuildConfiguration *>(ctx.buildConfiguration);
}

void CMakeBuildSystem::parseProject(ParsingContext &&ctx)
{
    m_currentContext = std::move(ctx);

    auto bc = qobject_cast<CMakeBuildConfiguration *>(m_currentContext.buildConfiguration);

    int parameters = m_currentContext.parameters;

    if (m_allFiles.isEmpty())
        parameters |= BuildDirManager::REPARSE_SCAN;

    m_waitingForScan = parameters & BuildDirManager::REPARSE_SCAN;
    m_waitingForParse = true;
    m_combinedScanAndParseResult = true;

    if (m_waitingForScan) {
        QTC_CHECK(m_treeScanner.isFinished());
        m_treeScanner.asyncScanForFiles(m_currentContext.project->projectDirectory());
        Core::ProgressManager::addTask(m_treeScanner.future(),
                                       tr("Scan \"%1\" project tree")
                                           .arg(m_currentContext.project->displayName()),
                                       "CMake.Scan.Tree");
    }

    bc->m_buildDirManager.parse(parameters);
}

void CMakeBuildSystem::handleTreeScanningFinished()
{
    QTC_CHECK(m_waitingForScan);

    qDeleteAll(m_allFiles);
    m_allFiles = Utils::transform(m_treeScanner.release(), [](const FileNode *fn) { return fn; });

    m_combinedScanAndParseResult = m_combinedScanAndParseResult && true;
    m_waitingForScan = false;

    combineScanAndParse();
}

void CMakeBuildSystem::handleParsingSuccess(CMakeBuildConfiguration *bc)
{
    if (bc != m_currentContext.buildConfiguration)
        return; // Not current information, ignore.

    QTC_ASSERT(m_waitingForParse, return );

    m_waitingForParse = false;
    m_combinedScanAndParseResult = m_combinedScanAndParseResult && true;

    combineScanAndParse();
}

void CMakeBuildSystem::handleParsingError(CMakeBuildConfiguration *bc)
{
    if (bc != m_currentContext.buildConfiguration)
        return; // Not current information, ignore.

    QTC_CHECK(m_waitingForParse);

    m_waitingForParse = false;
    m_combinedScanAndParseResult = false;

    combineScanAndParse();
}

void CMakeBuildSystem::combineScanAndParse()
{
    auto bc = qobject_cast<CMakeBuildConfiguration *>(m_currentContext.buildConfiguration);
    if (bc && bc->isActive()) {
        if (m_waitingForParse || m_waitingForScan)
            return;

        if (m_combinedScanAndParseResult) {
            updateProjectData(qobject_cast<CMakeProject *>(m_currentContext.project), bc);
            m_currentContext.guard.markAsSuccess();
        }
    }

    m_currentContext = BuildSystem::ParsingContext();
}

void CMakeBuildSystem::updateProjectData(CMakeProject *p, CMakeBuildConfiguration *bc)
{
    qCDebug(cmakeBuildSystemLog) << "Updating CMake project data";

    QTC_ASSERT(m_treeScanner.isFinished() && !bc->m_buildDirManager.isParsing(), return );

    project()->setExtraProjectFiles(bc->m_buildDirManager.takeProjectFilesToWatch());

    CMakeConfig patchedConfig = bc->configurationFromCMake();
    {
        CMakeConfigItem settingFileItem;
        settingFileItem.key = "ANDROID_DEPLOYMENT_SETTINGS_FILE";
        settingFileItem.value = bc->buildDirectory()
                                    .pathAppended("android_deployment_settings.json")
                                    .toString()
                                    .toUtf8();
        patchedConfig.append(settingFileItem);
    }
    {
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
        auto newRoot = bc->generateProjectTree(m_allFiles);
        if (newRoot) {
            p->setRootProjectNode(std::move(newRoot));
            if (p->rootProjectNode())
                p->setDisplayName(p->rootProjectNode()->displayName());

            for (const CMakeBuildTarget &bt : bc->buildTargets()) {
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
        m_extraCompilers = findExtraCompilers(p);
        CppTools::GeneratedCodeModelSupport::update(m_extraCompilers);
        qCDebug(cmakeBuildSystemLog) << "Extra compilers updated.";
    }

    QtSupport::CppKitInfo kitInfo(p);
    QTC_ASSERT(kitInfo.isValid(), return );

    {
        QString errorMessage;
        CppTools::RawProjectParts rpps = bc->m_buildDirManager.createRawProjectParts(errorMessage);
        if (!errorMessage.isEmpty())
            bc->setError(errorMessage);
        qCDebug(cmakeBuildSystemLog) << "Raw project parts created." << errorMessage;

        for (CppTools::RawProjectPart &rpp : rpps) {
            rpp.setQtVersion(
                kitInfo.projectPartQtVersion); // TODO: Check if project actually uses Qt.
            if (kitInfo.cxxToolChain)
                rpp.setFlagsForCxx({kitInfo.cxxToolChain, rpp.flagsForCxx.commandLineFlags});
            if (kitInfo.cToolChain)
                rpp.setFlagsForC({kitInfo.cToolChain, rpp.flagsForC.commandLineFlags});
        }

        m_cppCodeModelUpdater->update({p, kitInfo, bc->environment(), rpps});
    }
    {
        updateQmlJSCodeModel(p, bc);
    }

    emit p->fileListChanged();

    emit bc->emitBuildTypeChanged();

    bc->m_buildDirManager.resetData();

    qCDebug(cmakeBuildSystemLog) << "All CMake project data up to date.";
}

QList<ProjectExplorer::ExtraCompiler *> CMakeBuildSystem::findExtraCompilers(CMakeProject *p)
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
    const FilePathList fileList = p->files([&fileExtensions, p](const Node *n) {
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

        QStringList generated = p->filesGeneratedFrom(file.toString());
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     generated files:" << generated;
        if (generated.isEmpty())
            continue;

        const FilePathList fileNames = transform(generated, [](const QString &s) {
            return FilePath::fromString(s);
        });
        extraCompilers.append(factory->create(p, file, fileNames));
        qCDebug(cmakeBuildSystemLog)
            << "Finding Extra Compilers:     done with" << file.toUserOutput();
    }

    qCDebug(cmakeBuildSystemLog) << "Finding Extra Compilers: done.";

    return extraCompilers;
}

void CMakeBuildSystem::updateQmlJSCodeModel(CMakeProject *p, CMakeBuildConfiguration *bc)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    if (!modelManager)
        return;

    QmlJS::ModelManagerInterface::ProjectInfo projectInfo = modelManager
                                                                ->defaultProjectInfoForProject(p);

    projectInfo.importPaths.clear();

    const CMakeConfig &cm = bc->configurationFromCMake();
    const QString cmakeImports = QString::fromUtf8(CMakeConfigItem::valueOf("QML_IMPORT_PATH", cm));

    foreach (const QString &cmakeImport, CMakeConfigItem::cmakeSplitValue(cmakeImports))
        projectInfo.importPaths.maybeInsert(FilePath::fromString(cmakeImport), QmlJS::Dialect::Qml);

    modelManager->updateProjectInfo(projectInfo, p);
}

} // namespace CMakeProjectManager
