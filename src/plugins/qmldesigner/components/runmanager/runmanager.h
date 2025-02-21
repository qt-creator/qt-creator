// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    enum TargetState { Packing, Sending, Starting, Running, NotRunning };
    Q_ENUM(TargetState)

    enum TargetType { Normal, LivePreview, Android };
    Q_ENUM(TargetType)

    void udpateTargets();

    const QList<Target> targets() const;

    void toggleCurrentTarget();
    void cancelCurrentTarget();

    int currentTargetIndex() const;
    TargetType currentTargetType() const;

    bool selectRunTarget(Utils::Id id);
    bool selectRunTarget(const QString &targetName);

    void setState(TargetState state);
    TargetState state() const;

    void setProgress(int progress);
    int progress() const;

    void setError(const QString &error);
    const QString &error() const;

private:
    std::optional<Target> runTarget(Utils::Id targetId) const;
    int runTargetIndex(Utils::Id targetId) const;

    void handleError(const QString &deviceId, const QString &error);

    DeviceShare::DeviceManager &m_deviceManager;

    QList<Target> m_targets;
    Utils::Id m_currentTargetId;
    TargetType m_currentTargetType = TargetType::Normal;

    QList<RunningTarget> m_runningTargets;

    TargetState m_state = TargetState::NotRunning;
    int m_progress = 0;
    QString m_error;

signals:
    void runTargetChanged();
    void runTargetTypeChanged();
    void stateChanged();
    void targetsChanged();
    void progressChanged();
    void errorChanged();
};

} // namespace QmlDesigner
