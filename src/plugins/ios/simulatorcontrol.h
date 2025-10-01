// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>
#include <QFuture>
#include <QDebug>

#include <memory>

namespace Ios::Internal {

class SimulatorControlPrivate;

class SimulatorEntity
{
public:
    QString name;
    QString identifier;
    bool operator <(const SimulatorEntity &o) const
    {
        return name < o.name;
    }
};

class SimulatorInfo : public SimulatorEntity
{
public:
    QString toString() const;

    bool isBooted() const { return state == "Booted"; }
    bool isShuttingDown() const { return state == "Shutting Down"; }
    bool isShutdown() const { return state == "Shutdown"; }
    bool operator==(const SimulatorInfo &other) const;
    bool operator!=(const SimulatorInfo &other) const { return !(*this == other); }
    bool available;
    QString state;
    QString runtimeName;
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
    static QFuture<QList<SimulatorInfo>> updateAvailableSimulators(QObject *context);
    static bool isSimulatorRunning(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FilePath &bundlePath);

    static QFuture<Response> startSimulator(const QString &simUdid);
    static QFuture<Response> installApp(const QString &simUdid, const Utils::FilePath &bundlePath);
    static QFuture<Response> launchApp(const QString &simUdid,
                                       const QString &bundleIdentifier,
                                       bool waitForDebugger,
                                       const QStringList &extraArgs,
                                       const QString &stdoutPath = {},
                                       const QString &stderrPath = {});
};

} // Ios::Internal

Q_DECLARE_METATYPE(Ios::Internal::SimulatorInfo)
