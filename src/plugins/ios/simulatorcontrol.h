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
#pragma once

#include <utils/fileutils.h>

#include <QObject>
#include <QFuture>
#include <QDebug>
#include <memory>

namespace Ios {
namespace Internal {

class SimulatorControlPrivate;


class SimulatorEntity {
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
    friend QDebug &operator<<(QDebug &, const SimulatorInfo &info);

public:
    bool isBooted() const { return state == "Booted"; }
    bool isShuttingDown() const { return state == "Shutting Down"; }
    bool isShutdown() const { return state == "Shutdown"; }
    bool operator==(const SimulatorInfo &other) const;
    bool operator!=(const SimulatorInfo &other) const { return !(*this == other); }
    bool available;
    QString state;
    QString runtimeName;
};

class RuntimeInfo : public SimulatorEntity{
public:
    QString version;
    QString build;
};

class DeviceTypeInfo : public SimulatorEntity {};

class SimulatorControl
{
public:
    struct ResponseData {
        ResponseData(const QString & udid):
            simUdid(udid) { }

        QString simUdid;
        bool success = false;
        qint64 pID = -1;
        QString commandOutput;
    };

public:
    static QList<DeviceTypeInfo> availableDeviceTypes();
    static QFuture<QList<DeviceTypeInfo> > updateDeviceTypes();
    static QList<RuntimeInfo> availableRuntimes();
    static QFuture<QList<RuntimeInfo> > updateRuntimes();
    static QList<SimulatorInfo> availableSimulators();
    static QFuture<QList<SimulatorInfo> > updateAvailableSimulators();
    static bool isSimulatorRunning(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FilePath &bundlePath);
    static QString bundleExecutable(const Utils::FilePath &bundlePath);

public:
    static QFuture<ResponseData> startSimulator(const QString &simUdid);
    static QFuture<ResponseData> installApp(const QString &simUdid,
                                            const Utils::FilePath &bundlePath);
    static QFuture<ResponseData> launchApp(const QString &simUdid,
                                           const QString &bundleIdentifier,
                                           bool waitForDebugger,
                                           const QStringList &extraArgs,
                                           const QString &stdoutPath = QString(),
                                           const QString &stderrPath = QString());
    static QFuture<ResponseData> deleteSimulator(const QString &simUdid);
    static QFuture<ResponseData> resetSimulator(const QString &simUdid);
    static QFuture<ResponseData> renameSimulator(const QString &simUdid, const QString &newName);
    static QFuture<ResponseData> createSimulator(const QString &name,
                                                 const DeviceTypeInfo &deviceType,
                                                 const RuntimeInfo &runtime);
    static QFuture<ResponseData> takeSceenshot(const QString &simUdid, const QString &filePath);
};

} // namespace Internal
} // namespace Ios

Q_DECLARE_METATYPE(Ios::Internal::DeviceTypeInfo)
Q_DECLARE_METATYPE(Ios::Internal::RuntimeInfo)
Q_DECLARE_METATYPE(Ios::Internal::SimulatorInfo)
