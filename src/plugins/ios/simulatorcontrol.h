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

#include <QObject>
#include <QFuture>
#include "utils/fileutils.h"

#include <memory>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

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

class SimulatorInfo : public SimulatorEntity {
public:
    bool isBooted() const { return state.compare(QStringLiteral("Booted")) == 0; }
    bool isShutdown() const { return state.compare(QStringLiteral("Shutdown")) == 0; }
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

class SimulatorControl : public QObject
{
    Q_OBJECT
public:
    struct ResponseData {
        ResponseData(const QString & udid):
            simUdid(udid) { }

        QString simUdid;
        bool success = false;
        qint64 pID = -1;
        QByteArray commandOutput = "";
    };

public:
    explicit SimulatorControl(QObject* parent = nullptr);
    ~SimulatorControl();

public:
    static QList<DeviceTypeInfo> availableDeviceTypes();
    static QFuture<QList<DeviceTypeInfo> > updateDeviceTypes();
    static QList<RuntimeInfo> availableRuntimes();
    static QFuture<QList<RuntimeInfo> > updateRuntimes();
    static QList<SimulatorInfo> availableSimulators();
    static QFuture<QList<SimulatorInfo> > updateAvailableSimulators();
    static bool isSimulatorRunning(const QString &simUdid);
    static QString bundleIdentifier(const Utils::FileName &bundlePath);
    static QString bundleExecutable(const Utils::FileName &bundlePath);

public:
    QFuture<ResponseData> startSimulator(const QString &simUdid) const;
    QFuture<ResponseData> installApp(const QString &simUdid, const Utils::FileName &bundlePath) const;
    QFuture<ResponseData> launchApp(const QString &simUdid, const QString &bundleIdentifier,
                                    bool waitForDebugger, const QStringList &extraArgs,
                                    const QString& stdoutPath = QString(),
                                    const QString& stderrPath = QString()) const;
    QFuture<ResponseData> deleteSimulator(const QString &simUdid) const;
    QFuture<ResponseData> resetSimulator(const QString &simUdid) const;
    QFuture<ResponseData> renameSimulator(const QString &simUdid, const QString &newName) const;
    QFuture<ResponseData> createSimulator(const QString &name,
                                          const DeviceTypeInfo &deviceType,
                                          const RuntimeInfo &runtime);
    QFuture<ResponseData> takeSceenshot(const QString &simUdid, const QString &filePath);

private:
    SimulatorControlPrivate *d;
};
} // namespace Internal
} // namespace Ios

Q_DECLARE_METATYPE(Ios::Internal::DeviceTypeInfo)
Q_DECLARE_METATYPE(Ios::Internal::RuntimeInfo)
Q_DECLARE_METATYPE(Ios::Internal::SimulatorInfo)
