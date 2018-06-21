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

#include <utils/port.h>

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QProcess>

namespace Ios {
namespace Internal {
class IosToolHandlerPrivate;
class IosDeviceType;
}

class IosToolHandler : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString,QString> Dict;
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

    explicit IosToolHandler(const Internal::IosDeviceType &type, QObject *parent = 0);
    ~IosToolHandler();
    void requestTransferApp(const QString &bundlePath, const QString &deviceId, int timeout = 1000);
    void requestRunApp(const QString &bundlePath, const QStringList &extraArgs, RunKind runType,
                            const QString &deviceId, int timeout = 1000);
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    bool isRunning() const;
    void stop();

signals:
    void isTransferringApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                           const QString &deviceId, int progress, int maxProgress,
                           const QString &info);
    void didTransferApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                        const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void didStartApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                     const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void gotServerPorts(Ios::IosToolHandler *handler, const QString &bundlePath,
                            const QString &deviceId, Utils::Port gdbPort, Utils::Port qmlPort);
    void gotInferiorPid(Ios::IosToolHandler *handler, const QString &bundlePath,
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
