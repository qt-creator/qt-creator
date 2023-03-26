// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    friend class GdbRunner;

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
    int m_maxProgress = 0;
    int m_operationsRemaining = 0;
    bool m_debug = false;
    bool m_inAppOutput = false;
    bool m_splitAppOutput = true; // as QXmlStreamReader reports the text attributes atomically it is better to split
    IosDeviceManager::AppOp m_requestedOperation{IosDeviceManager::AppOp::None};
    QFile m_outputFile;
    QString m_qmlPort;
    QString m_deltasPath;
    QXmlStreamWriter m_xmlWriter;
    std::unique_ptr<GdbRelayServer> m_gdbServer;
    std::unique_ptr<QmlRelayServer> m_qmlServer;
    std::unique_ptr<GdbRunner> m_gdbRunner;
    bool m_echoRelays = false;
};
}
