// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runmanager.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesignertr.h>

#include <devicesharing/device.h>

namespace QmlDesigner {

Q_LOGGING_CATEGORY(runManagerLog, "qtc.designer.runManager", QtWarningMsg)

namespace {
// helper type for the visitor
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace

DeviceShare::DeviceManager *s_deviceManager = nullptr;

static DeviceShare::DeviceManager *deviceManager()
{
    return s_deviceManager;
}

RunManager::RunManager(DeviceShare::DeviceManager &deviceManager)
    : QObject()
    , m_deviceManager(deviceManager)
{
    s_deviceManager = &m_deviceManager;

    // Connect DeviceManager with internal target generation
    connect(&m_deviceManager, &DeviceShare::DeviceManager::deviceAdded, this, &RunManager::udpateTargets);
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::deviceRemoved,
            this,
            &RunManager::udpateTargets);
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::deviceActivated,
            this,
            &RunManager::udpateTargets);
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::deviceDeactivated,
            this,
            &RunManager::udpateTargets);
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::deviceAliasChanged,
            this,
            &RunManager::udpateTargets);

    // If device going offline is currently running force stop
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::deviceOffline,
            this,
            [this](const QString &deviceId) {
                qCDebug(runManagerLog) << "Device offline." << deviceId;

                if (m_runningTargets.empty())
                    return;

                auto findRunningTarget = [&](const auto &runningTarget) {
                    return std::visit(overloaded{[](const QPointer<ProjectExplorer::RunControl>) {
                                                     return false;
                                                 },
                                                 [&](const QString &arg) { return arg == deviceId; }},
                                      runningTarget);
                };

                const auto it = std::ranges::find_if(m_runningTargets, findRunningTarget);

                if (it != m_runningTargets.end()) {
                    std::visit(overloaded{[](const QPointer<ProjectExplorer::RunControl>) {},
                                          [&](const QString &) { m_deviceManager.stopProject(); }},
                               *it);
                }
            });

    // Packing
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectPacking,
            this,
            [this](const QString &deviceId) {
                qCDebug(runManagerLog) << "Project packing." << deviceId;
                setState(TargetState::Packing);
            });
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectPackingError,
            this,
            &RunManager::handleError);

    // Sending
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectSendingProgress,
            this,
            [this](const QString &deviceId, const int percentage) {
                qCDebug(runManagerLog) << "Project sending." << deviceId << percentage;
                setProgress(percentage);
                setState(TargetState::Sending);
            });
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectSendingError,
            this,
            &RunManager::handleError);

    // Starting
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectStarting,
            this,
            [this](const QString &deviceId) {
                qCDebug(runManagerLog) << "Project starting." << deviceId;
                setState(TargetState::Starting);
            });

    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectStartingError,
            this,
            &RunManager::handleError);

    connect(&m_deviceManager, &DeviceShare::DeviceManager::internalError, this, &RunManager::handleError);

    // Connect Android/Device run/stop project signals
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectStarted,
            this,
            [this](const QString &deviceId) {
                qCDebug(runManagerLog) << "Project started." << deviceId;

                m_runningTargets.append(deviceId);

                setState(TargetState::Running);
            });
    connect(&m_deviceManager,
            &DeviceShare::DeviceManager::projectStopped,
            this,
            [this](const QString &deviceId) {
                qCDebug(runManagerLog) << "Project stopped." << deviceId;

                auto findRunningTarget = [&](const auto &runningTarget) {
                    return std::visit(overloaded{[](const QPointer<ProjectExplorer::RunControl>) {
                                                     return false;
                                                 },
                                                 [&](const QString &arg) { return arg == deviceId; }},
                                      runningTarget);
                };

                const auto [first, last] = std::ranges::remove_if(m_runningTargets, findRunningTarget);
                m_runningTargets.erase(first, last);

                if (!m_runningTargets.isEmpty())
                    return;

                setState(TargetState::NotRunning);
            });

    // Connect Creator run/stop project signals
    auto projectExplorerPlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(projectExplorerPlugin,
            &ProjectExplorer::ProjectExplorerPlugin::runControlStarted,
            this,
            [this](ProjectExplorer::RunControl *runControl) {
                qCDebug(runManagerLog) << "Run Control started.";

                m_runningTargets.append(QPointer(runControl));

                setState(TargetState::Running);
            });
    connect(projectExplorerPlugin,
            &ProjectExplorer::ProjectExplorerPlugin::runControlStoped,
            this,
            [this](ProjectExplorer::RunControl *runControl) {
                qCDebug(runManagerLog) << "Run Control stoped.";

                auto findRunningTarget = [&](const auto &runningTarget) {
                    return std::visit(overloaded{[&](const QPointer<ProjectExplorer::RunControl> arg) {
                                                     return arg.get() == runControl;
                                                 },
                                                 [](const QString &) { return false; }},
                                      runningTarget);
                };

                const auto [first, last] = std::ranges::remove_if(m_runningTargets, findRunningTarget);
                m_runningTargets.erase(first, last);

                if (!m_runningTargets.isEmpty())
                    return;

                setState(TargetState::NotRunning);
            });

    udpateTargets();
}

void RunManager::udpateTargets()
{
    m_targets.clear();

    m_targets.append(NormalTarget());
    m_targets.append(LivePreviewTarget());

    for (const auto &device : m_deviceManager.devices()) {
        if (device->deviceSettings().active())
            m_targets.append(AndroidTarget(device->deviceSettings().deviceId()));
    }

    emit targetsChanged();

    bool currentTargetReset = false;
    auto target = runTarget(m_currentTargetId);

    if (target) {
        bool enabled = std::visit([&](const auto &arg) { return arg.enabled(); }, *target);

        if (m_currentTargetId.isValid() && enabled)
            currentTargetReset = selectRunTarget(m_currentTargetId);
    }

    if (!currentTargetReset) // default run target
        selectRunTarget(NormalTarget().id());
}

const QList<Target> RunManager::targets() const
{
    return m_targets;
}

void RunManager::toggleCurrentTarget()
{
    if (!m_runningTargets.isEmpty()) {
        for (auto runningTarget : m_runningTargets) {
            std::visit(overloaded{[](const QPointer<ProjectExplorer::RunControl> arg) {
                                      if (!arg.isNull())
                                          arg.get()->initiateStop();
                                  },
                                  [](const QString &) { deviceManager()->stopProject(); }},
                       runningTarget);
        }
        return;
    }

    auto target = runTarget(m_currentTargetId);
    if (!target)
        return;

    bool enabled = std::visit([&](const auto &arg) { return arg.enabled(); }, *target);
    if (!enabled) {
        qCDebug(runManagerLog) << "Can't start run target" << m_currentTargetId << "not enabled.";
        return;
    }

    setError("");

    std::visit([&](const auto &arg) { arg.run(); }, *target);
}

void RunManager::cancelCurrentTarget()
{
    deviceManager()->stopProject();
}

int RunManager::currentTargetIndex() const
{
    return runTargetIndex(m_currentTargetId);
}

bool RunManager::selectRunTarget(Utils::Id id)
{
    if (m_currentTargetId == id)
        return true;

    int index = runTargetIndex(id);

    if (index == -1) {
        qCDebug(runManagerLog) << "Couldn't find run target" << id;
        return false;
    }

    m_currentTargetId = id;
    emit runTargetChanged();

    return true;
}

bool RunManager::selectRunTarget(const QString &targetName)
{
    return selectRunTarget(Utils::Id::fromString(targetName));
}

void RunManager::setState(TargetState state)
{
    if (state == m_state)
        return;

    m_state = state;
    emit stateChanged();
}

RunManager::TargetState RunManager::state() const
{
    return m_state;
}

void RunManager::setProgress(int progress)
{
    m_progress = progress;
    emit progressChanged();
}

int RunManager::progress() const
{
    return m_progress;
}

void RunManager::setError(const QString &error)
{
    if (error == m_error)
        return;

    m_error = error;
    emit errorChanged();
}

const QString &RunManager::error() const
{
    return m_error;
}

std::optional<Target> RunManager::runTarget(Utils::Id targetId) const
{
    auto find_id = [&](const auto &target) {
        return std::visit([&](const auto &arg) { return arg.id() == targetId; }, target);
    };

    auto result = std::ranges::find_if(m_targets, find_id);

    if (result != m_targets.end())
        return *result;

    qCDebug(runManagerLog) << "Couldn't find run target" << targetId;

    return {};
}

int RunManager::runTargetIndex(Utils::Id targetId) const
{
    auto find_id = [&](const auto &target) {
        return std::visit([&](const auto &arg) { return arg.id() == targetId; }, target);
    };

    auto result = std::ranges::find_if(m_targets, find_id);

    if (result != m_targets.end())
        return std::distance(m_targets.begin(), result);

    return -1;
}

void RunManager::handleError(const QString &deviceId, const QString &error)
{
    qCDebug(runManagerLog) << "Error" << deviceId << error;
    setError(error);
    setState(TargetState::NotRunning);
}

QString NormalTarget::name() const
{
    return Tr::tr("Run App");
}

Utils::Id NormalTarget::id() const
{
    return ProjectExplorer::Constants::NORMAL_RUN_MODE;
}

bool NormalTarget::enabled() const
{
    return bool(ProjectExplorer::ProjectExplorerPlugin::canRunStartupProject(id()));
}

void NormalTarget::run() const
{
    ProjectExplorer::ProjectExplorerPlugin::runStartupProject(id());
}

QString LivePreviewTarget::name() const
{
    return Tr::tr("Live Preview");
}

Utils::Id LivePreviewTarget::id() const
{
    return ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE;
}

bool LivePreviewTarget::enabled() const
{
    return bool(ProjectExplorer::ProjectExplorerPlugin::canRunStartupProject(id()));
}

void LivePreviewTarget::run() const
{
    ProjectExplorer::ProjectExplorerPlugin::runStartupProject(id());
}

AndroidTarget::AndroidTarget(const QString &deviceId)
    : m_deviceId(deviceId)
{}

QString AndroidTarget::name() const
{
    if (auto deviceSettings = deviceManager()->deviceSettings(m_deviceId))
        return deviceSettings->alias();

    return {};
}

Utils::Id AndroidTarget::id() const
{
    if (auto deviceSettings = deviceManager()->deviceSettings(m_deviceId))
        return Utils::Id::fromString(deviceSettings->deviceId());

    return {};
}

bool AndroidTarget::enabled() const
{
    return deviceManager()->deviceIsConnected(m_deviceId).value_or(false);
}

void AndroidTarget::run() const
{
    if (!ProjectExplorer::ProjectExplorerPlugin::saveModifiedFiles())
        return;

    deviceManager()->runProject(m_deviceId);
}

} // namespace QmlDesigner
