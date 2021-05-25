/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "iosdevicemanager.h"

#include <QFile>
#include <QObject>
#include <QRecursiveMutex>
#include <QString>
#include <QXmlStreamWriter>

namespace Ios {
class GdbRunner;
class GdbRelayServer;
class QmlRelayServer;

class IosTool: public QObject
{
    Q_OBJECT

public:
    IosTool(QObject *parent = 0);
    virtual ~IosTool();
    void run(const QStringList &args);
    void doExit(int errorCode = 0);
    void writeMsg(const char *msg);
    void writeMsg(const QString &msg);
    void stopXml(int errorCode);
    void writeTextInElement(const QString &output);
    void stopRelayServers(int errorCode = 0);
    void writeMaybeBin(const QString &extraMsg, const char *msg, quintptr len);
    void errorMsg(const QString &msg);
    Q_INVOKABLE void stopGdbRunner();
    bool echoRelays() const { return m_echoRelays; }

private:
    void stopGdbRunner2();
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     IosDeviceManager::OpStatus status, ServiceConnRef conn, int gdbFd,
                     DeviceSession *deviceSession);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void readStdin();

    QRecursiveMutex m_xmlMutex;
    int maxProgress;
    int opLeft;
    bool debug;
    bool inAppOutput;
    bool splitAppOutput; // as QXmlStreamReader reports the text attributes atomically it is better to split
    Ios::IosDeviceManager::AppOp appOp;
    QFile outFile;
    QString m_qmlPort;
    QXmlStreamWriter out;
    GdbRelayServer *gdbServer;
    QmlRelayServer *qmlServer;
    GdbRunner *gdbRunner;
    bool m_echoRelays = false;
    friend class GdbRunner;
};
}
