// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsystem.h"

#include "buildaspects.h"
#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "deployconfiguration.h"
#include "extracompiler.h"
#include "makestep.h"
#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projecttree.h"
#include "runconfiguration.h"
#include "target.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/outputwindow.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTimer>

using namespace Utils;

namespace ProjectExplorer {

// --------------------------------------------------------------------
// BuildSystem:
// --------------------------------------------------------------------

class BuildSystemPrivate
{
public:
    BuildConfiguration *m_buildConfiguration = nullptr;

    QTimer m_delayedParsingTimer;

    bool m_isParsing = false;
    bool m_hasParsingData = false;

    DeploymentData m_deploymentData;
    QList<BuildTargetInfo> m_appTargets;
};

BuildSystem::BuildSystem(BuildConfiguration *bc) : d(new BuildSystemPrivate)
{
    QTC_CHECK(bc);
    d->m_buildConfiguration = bc;

    // Timer:
    d->m_delayedParsingTimer.setSingleShot(true);

    connect(&d->m_delayedParsingTimer, &QTimer::timeout, this, [this] {
        if (ProjectManager::hasProject(project()))
            triggerParsing();
        else
            requestDelayedParse();
    });
}

BuildSystem::~BuildSystem()
{
    delete d;
}

QString BuildSystem::name() const
{
    return project()->buildSystemName();
}

Project *BuildSystem::project() const
{
    return d->m_buildConfiguration->project();
}

Target *BuildSystem::target() const
{
    return d->m_buildConfiguration->target();
}

Kit *BuildSystem::kit() const
{
    return d->m_buildConfiguration->kit();
}

BuildConfiguration *BuildSystem::buildConfiguration() const
{
    return d->m_buildConfiguration;
}

void BuildSystem::emitParsingStarted()
{
    QTC_ASSERT(!d->m_isParsing, return);

    d->m_isParsing = true;
    emit parsingStarted();
    emit project()->anyParsingStarted();
    emit ProjectManager::instance()->projectStartedParsing(project());
    if (this == activeBuildSystemForActiveProject())
        emit ProjectManager::instance()->parsingStartedActive(this);
    if (this == activeBuildSystemForCurrentProject())
        emit ProjectManager::instance()->parsingStartedCurrent(this);
}

void BuildSystem::emitParsingFinished(bool success)
{
    // Intentionally no return, as we currently get start - start - end - end
    // sequences when switching qmake targets quickly.
    QTC_CHECK(d->m_isParsing);

    d->m_isParsing = false;
    d->m_hasParsingData = success;
    emit parsingFinished(success);
    emit project()->anyParsingFinished(success);
    emit ProjectManager::instance()->projectFinishedParsing(project());
    if (this == activeBuildSystemForActiveProject())
        emit ProjectManager::instance()->parsingFinishedActive(success, this);
    if (this == activeBuildSystemForCurrentProject())
        emit ProjectManager::instance()->parsingFinishedCurrent(success, this);
}

FilePath BuildSystem::projectFilePath() const
{
    return project()->projectFilePath();
}

FilePath BuildSystem::projectDirectory() const
{
    return project()->projectDirectory();
}

bool BuildSystem::isWaitingForParse() const
{
    return d->m_delayedParsingTimer.isActive();
}

void BuildSystem::requestParse()
{
    requestParseHelper(0);
}

void BuildSystem::requestDelayedParse()
{
    requestParseHelper(1000);
}

void BuildSystem::cancelDelayedParseRequest()
{
    d->m_delayedParsingTimer.stop();
}

bool BuildSystem::isParsing() const
{
    return d->m_isParsing;
}

bool BuildSystem::hasParsingData() const
{
    return d->m_hasParsingData;
}

Environment BuildSystem::activeParseEnvironment() const
{
    QTC_ASSERT(d->m_buildConfiguration, return {});
    return d->m_buildConfiguration->environment();
}

void BuildSystem::requestParseHelper(int delay)
{
    d->m_delayedParsingTimer.setInterval(delay);
    d->m_delayedParsingTimer.start();
}

ExtraCompiler *BuildSystem::findExtraCompiler(
        const std::function<bool (const ExtraCompiler *)> &) const
{
    return nullptr;
}

bool BuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *notAdded)
{
    Q_UNUSED(filePaths)
    Q_UNUSED(notAdded)
    return false;
}

RemovedFilesFromProject BuildSystem::removeFiles(Node *, const FilePaths &filePaths,
                                                 FilePaths *notRemoved)
{
    Q_UNUSED(filePaths)
    Q_UNUSED(notRemoved)
    return RemovedFilesFromProject::Error;
}

bool BuildSystem::deleteFiles(Node *, const FilePaths &filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool BuildSystem::canRenameFile(Node *, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    Q_UNUSED(oldFilePath)
    Q_UNUSED(newFilePath)
    return true;
}

bool BuildSystem::renameFiles(Node *, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    if (notRenamed)
        *notRenamed = firstPaths(filesToRename);
    return false;
}

bool BuildSystem::addDependencies(Node *, const QStringList &dependencies)
{
    Q_UNUSED(dependencies)
    return false;
}

bool BuildSystem::supportsAction(Node *, ProjectAction, const Node *) const
{
    return false;
}

ExtraCompiler *BuildSystem::extraCompilerForSource(const Utils::FilePath &source) const
{
    return findExtraCompiler([source](const ExtraCompiler *ec) { return ec->source() == source; });
}

ExtraCompiler *BuildSystem::extraCompilerForTarget(const Utils::FilePath &target) const
{
    return findExtraCompiler([target](const ExtraCompiler *ec) {
        return ec->targets().contains(target);
    });
}

MakeInstallCommand BuildSystem::makeInstallCommand(const FilePath &installRoot) const
{
    QTC_ASSERT(project()->hasMakeInstallEquivalent(), return {});
    QTC_ASSERT(buildConfiguration(), return {});

    BuildStepList *buildSteps = buildConfiguration()->buildSteps();
    QTC_ASSERT(buildSteps, return {});

    MakeInstallCommand cmd;
    if (const auto makeStep = buildSteps->firstOfType<MakeStep>()) {
        cmd.command.setExecutable(makeStep->makeExecutable());
        cmd.command.addArg("install");
        cmd.command.addArg("INSTALL_ROOT=" + installRoot.nativePath());
    }
    return cmd;
}

FilePaths BuildSystem::filesGeneratedFrom(const FilePath &sourceFile) const
{
    Q_UNUSED(sourceFile)
    return {};
}

QVariant BuildSystem::additionalData(Utils::Id id) const
{
    Q_UNUSED(id)
    return {};
}

// ParseGuard

BuildSystem::ParseGuard::ParseGuard(BuildSystem::ParseGuard &&other)
    : m_buildSystem{std::move(other.m_buildSystem)}
    , m_success{std::move(other.m_success)}
{
    // No need to release this as this is invalid anyway:-)
    other.m_buildSystem = nullptr;
}

BuildSystem::ParseGuard::ParseGuard(BuildSystem *p)
    : m_buildSystem(p)
{
    if (m_buildSystem && !m_buildSystem->isParsing())
        m_buildSystem->emitParsingStarted();
    else
        m_buildSystem = nullptr;
}

void BuildSystem::ParseGuard::release()
{
    if (m_buildSystem)
        m_buildSystem->emitParsingFinished(m_success);
    m_buildSystem = nullptr;
}

BuildSystem::ParseGuard &BuildSystem::ParseGuard::operator=(BuildSystem::ParseGuard &&other)
{
    release();

    m_buildSystem = std::move(other.m_buildSystem);
    m_success = std::move(other.m_success);

    other.m_buildSystem = nullptr;
    return *this;
}

void BuildSystem::setDeploymentData(const DeploymentData &deploymentData)
{
    if (d->m_deploymentData != deploymentData) {
        d->m_deploymentData = deploymentData;
        emit deploymentDataChanged();
    }
}

DeploymentData BuildSystem::deploymentData() const
{
    const DeployConfiguration * const dc = buildConfiguration()->activeDeployConfiguration();
    if (dc && dc->usesCustomDeploymentData())
        return dc->customDeploymentData();
    return d->m_deploymentData;
}

void BuildSystem::setApplicationTargets(const QList<BuildTargetInfo> &appTargets)
{
    d->m_appTargets = appTargets;
}

const QList<BuildTargetInfo> BuildSystem::applicationTargets() const
{
    return d->m_appTargets;
}

BuildTargetInfo BuildSystem::buildTarget(const QString &buildKey) const
{
    return Utils::findOrDefault(d->m_appTargets, [&buildKey](const BuildTargetInfo &ti) {
        return ti.buildKey == buildKey;
    });
}

void BuildSystem::setRootProjectNode(std::unique_ptr<ProjectNode> &&root)
{
    project()->setRootProjectNode(std::move(root));
}

void BuildSystem::updateQmlCodeModel()
{
    project()->updateQmlCodeModel(kit(), buildConfiguration());
}

void BuildSystem::updateQmlCodeModelInfo(QmlCodeModelInfo &)
{
    // Nothing by default.
}

void BuildSystem::emitBuildSystemUpdated()
{
    emit updated();
}

void BuildSystem::setExtraData(const QString &buildKey, Utils::Id dataKey, const QVariant &data)
{
    const ProjectNode *node = project()->findNodeForBuildKey(buildKey);
    QTC_ASSERT(node, return);
    node->setData(dataKey, data);
}

QVariant BuildSystem::extraData(const QString &buildKey, Utils::Id dataKey) const
{
    const ProjectNode *node = project()->findNodeForBuildKey(buildKey);
    QTC_ASSERT(node, return {});
    return node->data(dataKey);
}

void BuildSystem::startNewBuildSystemOutput(const QString &message)
{
    Core::OutputWindow *outputArea = ProjectExplorerPlugin::buildSystemOutput();
    outputArea->grayOutOldContent();
    outputArea->appendMessage(message + '\n', Utils::GeneralMessageFormat);
    Core::MessageManager::writeFlashing(message);
}

void BuildSystem::appendBuildSystemOutput(const QString &message)
{
    Core::OutputWindow *outputArea = ProjectExplorerPlugin::buildSystemOutput();
    outputArea->appendMessage(message + '\n', Utils::GeneralMessageFormat);
    Core::MessageManager::writeSilently(message);
}

QString BuildSystem::disabledReason(const QString &buildKey) const
{
    if (!hasParsingData()) {
        QString msg = isParsing() ? Tr::tr("The project is currently being parsed.")
                                  : Tr::tr("The project could not be fully parsed.");
        const FilePath projectFilePath = buildTarget(buildKey).projectFilePath;
        if (!projectFilePath.isEmpty() && !projectFilePath.exists())
            msg += '\n' + Tr::tr("The project file \"%1\" does not exist.").arg(projectFilePath.toUrlishString());
        return msg;
    }
    return {};
}

CommandLine BuildSystem::commandLineForTests(const QList<QString> & /*tests*/,
                                             const QStringList & /*options*/) const
{
    return {};
}

BuildSystem *activeBuildSystem(const Project *project)
{
    return project ? project->activeBuildSystem() : nullptr;
}

BuildSystem *activeBuildSystemForActiveProject()
{
    return activeBuildSystem(ProjectManager::startupProject());
}

BuildSystem *activeBuildSystemForCurrentProject()
{
    return activeBuildSystem(ProjectTree::currentProject());
}

} // namespace ProjectExplorer
