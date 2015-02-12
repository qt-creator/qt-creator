/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    static QString iosSimulatorToolPath();

    explicit IosToolHandler(const Internal::IosDeviceType &type, QObject *parent = 0);
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
    void gotServerPorts(Ios::IosToolHandler *handler, const QString &bundlePath,
                            const QString &deviceId, int gdbPort, int qmlPort);
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
