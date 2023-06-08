// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/port.h>

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

namespace Ios {
namespace Internal {
class IosToolHandlerPrivate;
class IosDeviceType;
}

class IosToolHandler : public QObject
{
    Q_OBJECT
public:
    using Dict = QMap<QString,QString>;
    enum RunKind {
        NormalRun,
        DebugRun
    };
    enum OpStatus {
        Success = 0,
        Unknown = 1,
        Failure = 2
    };

    static QString iosDeviceToolPath();

    explicit IosToolHandler(const Internal::IosDeviceType &type, QObject *parent = nullptr);
    ~IosToolHandler() override;
    void requestTransferApp(const Utils::FilePath &bundlePath, const QString &deviceId, int timeout = 1000);
    void requestRunApp(const Utils::FilePath &bundlePath, const QStringList &extraArgs, RunKind runType,
                            const QString &deviceId, int timeout = 1000);
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    bool isRunning() const;
    void stop();

signals:
    void isTransferringApp(Ios::IosToolHandler *handler, const Utils::FilePath &bundlePath,
                           const QString &deviceId, int progress, int maxProgress,
                           const QString &info);
    void didTransferApp(Ios::IosToolHandler *handler, const Utils::FilePath &bundlePath,
                        const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void didStartApp(Ios::IosToolHandler *handler, const Utils::FilePath &bundlePath,
                     const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void gotServerPorts(Ios::IosToolHandler *handler, const Utils::FilePath &bundlePath,
                            const QString &deviceId, Utils::Port gdbPort, Utils::Port qmlPort);
    void gotInferiorPid(Ios::IosToolHandler *handler, const Utils::FilePath &bundlePath,
                        const QString &deviceId, qint64 pid);
    void deviceInfo(Ios::IosToolHandler *handler, const QString &deviceId,
                    const Ios::IosToolHandler::Dict &info);
    void appOutput(Ios::IosToolHandler *handler, const QString &output);
    void errorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void toolExited(Ios::IosToolHandler *handler, int code);
    void finished(Ios::IosToolHandler *handler);

private:
    friend class Ios::Internal::IosToolHandlerPrivate;
    Ios::Internal::IosToolHandlerPrivate *d;
};

} // namespace Ios
