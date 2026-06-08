// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abi.h>
#include <utils/filepath.h>

#include <QDebug>
#include <QObject>
#include <QPromise>

namespace Ios::Internal {

class SimulatorRuntime
{
public:
    QString id;
    QString version;
    QList<ProjectExplorer::Abi::Architecture> architectures;
};

class SimulatorInfo
{
public:
    QString toString() const;

    bool isBooted() const { return state == "Booted"; }
    bool isShuttingDown() const { return state == "Shutting Down"; }
    bool isShutdown() const { return state == "Shutdown"; }
    bool operator==(const SimulatorInfo &other) const;

    QString name;
    QString identifier;
    bool operator<(const SimulatorInfo &o) const { return name < o.name; }

    bool available;
    QString state;
    SimulatorRuntime runtime;
};

class SimulatorControl
{
public:
    struct ResponseData {
        ResponseData(const QString &udid)
            : simUdid(udid)
        {}

        QString simUdid;
        qint64 inferiorPid{-1};
    };

    using Response = Utils::Result<ResponseData>;

    static QList<SimulatorInfo> availableSimulators();
    static void updateAvailableSimulators(const std::function<void()> &doneHandler = {});
    static bool isSimulatorRunning(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FilePath &bundlePath);
};

void startSimulator(QPromise<SimulatorControl::Response> &promise, const QString &simUdid);
void installApp(QPromise<SimulatorControl::Response> &promise, const QString &simUdid,
                const Utils::FilePath &bundlePath);
void launchApp(QPromise<SimulatorControl::Response> &promise, const QString &simUdid,
               const QString &bundleIdentifier,
               bool waitForDebugger,
               const QStringList &extraArgs,
               const QString &stdoutPath,
               const QString &stderrPath);

} // Ios::Internal

Q_DECLARE_METATYPE(Ios::Internal::SimulatorInfo)
