// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsystem.h"

#include "buildconfiguration.h"
#include "extracompiler.h"
#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "runconfiguration.h"
#include "target.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/outputwindow.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/makestep.h>

#include <utils/algorithm.h>
#include <utils/processinterface.h>
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
    Target *m_target = nullptr;
    BuildConfiguration *m_buildConfiguration = nullptr;

    QTimer m_delayedParsingTimer;

    bool m_isParsing = false;
    bool m_hasParsingData = false;

    DeploymentData m_deploymentData;
    QList<BuildTargetInfo> m_appTargets;
};

BuildSystem::BuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc->target())
{
    d->m_buildConfiguration = bc;
}

BuildSystem::BuildSystem(Target *target)
    : d(new BuildSystemPrivate)
{
    QTC_CHECK(target);
    d->m_target = target;

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

Project *BuildSystem::project() const
{
    return d->m_target->project();
}

Target *BuildSystem::target() const
{
    return d->m_target;
}

Kit *BuildSystem::kit() const
{
    return d->m_target->kit();
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
    emit d->m_target->parsingStarted();
}

void BuildSystem::emitParsingFinished(bool success)
{
    // Intentionally no return, as we currently get start - start - end - end
    // sequences when switching qmake targets quickly.
    QTC_CHECK(d->m_isParsing);

    d->m_isParsing = false;
    d->m_hasParsingData = success;
    emit parsingFinished(success);
    emit d->m_target->parsingFinished(success);
}

FilePath BuildSystem::projectFilePath() const
{
    return d->m_target->project()->projectFilePath();
}

FilePath BuildSystem::projectDirectory() const
{
    return d->m_target->project()->projectDirectory();
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

void BuildSystem::requestParseWithCustomDelay(int delayInMs)
{
    requestParseHelper(delayInMs);
}

void BuildSystem::cancelDelayedParseRequest()
{
    d->m_delayedParsingTimer.stop();
}

void BuildSystem::setParseDelay(int delayInMs)
{
    d->m_delayedParsingTimer.setInterval(delayInMs);
}

int BuildSystem::parseDelay() const
{
    return d->m_delayedParsingTimer.interval();
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
    const BuildConfiguration *const bc = d->m_target->activeBuildConfiguration();
    if (bc)
        return bc->environment();

    const RunConfiguration *const rc = d->m_target->activeRunConfiguration();
    if (rc)
        return rc->runnable().environment;

    return d->m_target->kit()->buildEnvironment();
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

bool BuildSystem::renameFile(Node *, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    Q_UNUSED(oldFilePath)
    Q_UNUSED(newFilePath)
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
    QTC_ASSERT(target()->project()->hasMakeInstallEquivalent(), return {});

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
        emit target()->deploymentDataChanged();
    }
}

DeploymentData BuildSystem::deploymentData() const
{
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
    d->m_target->project()->setRootProjectNode(std::move(root));
}

void BuildSystem::emitBuildSystemUpdated()
{
    emit target()->buildSystemUpdated(this);
}

void BuildSystem::setExtraData(const QString &buildKey, Utils::Id dataKey, const QVariant &data)
{
    const ProjectNode *node = d->m_target->project()->findNodeForBuildKey(buildKey);
    QTC_ASSERT(node, return);
    node->setData(dataKey, data);
}

QVariant BuildSystem::extraData(const QString &buildKey, Utils::Id dataKey) const
{
    const ProjectNode *node = d->m_target->project()->findNodeForBuildKey(buildKey);
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
            msg += '\n' + Tr::tr("The project file \"%1\" does not exist.").arg(projectFilePath.toString());
        return msg;
    }
    return {};
}

CommandLine BuildSystem::commandLineForTests(const QList<QString> & /*tests*/,
                                             const QStringList & /*options*/) const
{
    return {};
}

} // namespace ProjectExplorer
