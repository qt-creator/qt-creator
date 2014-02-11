/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IOSTOOLHANDLER_H
#define IOSTOOLHANDLER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QProcess>


namespace Ios {
namespace Internal { class IosToolHandlerPrivate; }

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
    enum DeviceType {
        IosDeviceType,
        IosSimulatedIphoneType,
        IosSimulatedIpadType,
        IosSimulatedIphoneRetina4InchType,
        IosSimulatedIphoneRetina3_5InchType,
        IosSimulatedIpadRetinaType
    };

    static QString iosDeviceToolPath();
    static QString iosSimulatorToolPath();

    explicit IosToolHandler(DeviceType = IosDeviceType, QObject *parent = 0);
    ~IosToolHandler();
    void requestTransferApp(const QString &bundlePath, const QString &deviceId, int timeout = 1000);
    void requestRunApp(const QString &bundlePath, const QStringList &extraArgs, RunKind runType,
                            const QString &deviceId, int timeout = 1000);
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    bool isRunning();
signals:
    void isTransferringApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                           const QString &deviceId, int progress, int maxProgress,
                           const QString &info);
    void didTransferApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                        const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void didStartApp(Ios::IosToolHandler *handler, const QString &bundlePath,
                     const QString &deviceId, Ios::IosToolHandler::OpStatus status);
    void gotGdbserverPort(Ios::IosToolHandler *handler, const QString &bundlePath,
                            const QString &deviceId, int gdbPort);
    void gotInferiorPid(Ios::IosToolHandler *handler, const QString &bundlePath,
                        const QString &deviceId, Q_PID pid);
    void deviceInfo(Ios::IosToolHandler *handler, const QString &deviceId,
                    const Ios::IosToolHandler::Dict &info);
    void appOutput(Ios::IosToolHandler *handler, const QString &output);
    void errorMsg(Ios::IosToolHandler *handler, const QString &msg);
    void toolExited(Ios::IosToolHandler *handler, int code);
    void finished(Ios::IosToolHandler *handler);
public slots:
    void stop();
private slots:
    void subprocessError(QProcess::ProcessError error);
    void subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void subprocessHasData();
    void killProcess();
private:
    friend class Ios::Internal::IosToolHandlerPrivate;
    Ios::Internal::IosToolHandlerPrivate *d;
};

} // namespace Ios

#endif // IOSTOOLHANDLER_H
