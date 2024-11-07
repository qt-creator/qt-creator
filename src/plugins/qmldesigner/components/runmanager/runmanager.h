// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

//#include <devicesharing/device.h>
#include <devicesharing/devicemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <utils/id.h>

namespace QmlDesigner {

struct NormalTarget
{
    QString name() const;
    Utils::Id id() const;
    bool enabled() const;

    void run() const;
};

struct LivePreviewTarget
{
    QString name() const;
    Utils::Id id() const;
    bool enabled() const;

    void run() const;
};

struct AndroidTarget
{
    AndroidTarget(const QString &deviceId);
    QString name() const;
    Utils::Id id() const;
    bool enabled() const;

    void run() const;

private:
    QString m_deviceId;
};

using Target = std::variant<NormalTarget, LivePreviewTarget, AndroidTarget>;

using RunningTarget = std::variant<QPointer<ProjectExplorer::RunControl>, QString>;

class RunManager : public QObject
{
    Q_OBJECT

public:
    explicit RunManager(DeviceShare::DeviceManager &deviceManager);

    enum TargetState { Starting, Running, NotRunning };
    Q_ENUM(TargetState)

    void udpateTargets();

    const QList<Target> targets() const;

    void toggleCurrentTarget();

    int currentTargetIndex() const;

    bool selectRunTarget(Utils::Id id);
    bool selectRunTarget(const QString &targetName);

    TargetState state() const { return m_state; }

private:
    std::optional<Target> runTarget(Utils::Id targetId) const;
    int runTargetIndex(Utils::Id targetId) const;

    DeviceShare::DeviceManager &m_deviceManager;

    QList<Target> m_targets;
    Utils::Id m_currentTargetId;
    //std::unique_ptr<RunningTarget> m_runningTarget;

    QList<RunningTarget> m_runningTargets;

    TargetState m_state = TargetState::NotRunning;

signals:
    void runTargetChanged();
    void stateChanged();
    void targetsChanged();
};

} // namespace QmlDesigner
