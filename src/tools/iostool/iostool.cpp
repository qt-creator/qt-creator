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

#include "iostool.h"

#include "relayserver.h"
#include "gdbrunner.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QThread>

namespace Ios {

IosTool::IosTool(QObject *parent):
    QObject(parent),
    maxProgress(0),
    opLeft(0),
    debug(false),
    inAppOutput(false),
    splitAppOutput(true),
    appOp(IosDeviceManager::None),
    outFile(),
    out(&outFile),
    gdbServer(0),
    qmlServer(0)
{
    outFile.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle);
    out.setAutoFormatting(true);
}

IosTool::~IosTool()
{
}

void IosTool::run(const QStringList &args)
{
    IosDeviceManager *manager = IosDeviceManager::instance();
    QString deviceId;
    QString bundlePath;
    bool deviceInfo = false;
    bool printHelp = false;
    int timeout = 1000;
    QStringList extraArgs;

    out.writeStartDocument();
    out.writeStartElement(QLatin1String("query_result"));
    for (int iarg = 1; iarg < args.size(); ++iarg) {
        const QString &arg = args[iarg];
        if (arg == QLatin1String("-i") || arg == QLatin1String("--id")) {
            if (++iarg == args.size()) {
                writeMsg(QStringLiteral("missing device id value after ") + arg);
                printHelp = true;
            }
            deviceId = args.value(iarg);
        } else if (arg == QLatin1String("-b") || arg == QLatin1String("--bundle")) {
            if (++iarg == args.size()) {
                writeMsg(QStringLiteral("missing bundle path after ") + arg);
                printHelp = true;
            }
            bundlePath = args.value(iarg);
        } else if (arg == QLatin1String("--install")) {
            appOp = IosDeviceManager::AppOp(appOp | IosDeviceManager::Install);
        } else if (arg == QLatin1String("--run")) {
            appOp = IosDeviceManager::AppOp(appOp | IosDeviceManager::Run);
        } else if (arg == QLatin1String("--noninteractive")) {
            // ignored for compatibility
        } else if (arg == QLatin1String("-v") || arg == QLatin1String("--verbose")) {
            m_echoRelays = true;
        } else if (arg == QLatin1String("-d") || arg == QLatin1String("--debug")) {
            appOp = IosDeviceManager::AppOp(appOp | IosDeviceManager::Run);
            debug = true;
        } else if (arg == QLatin1String("--device-info")) {
            deviceInfo = true;
        } else if (arg == QLatin1String("-t") || arg == QLatin1String("--timeout")) {
            if (++iarg == args.size()) {
                writeMsg(QStringLiteral("missing timeout value after ") + arg);
                printHelp = true;
            }
            bool ok = false;
            int tOut = args.value(iarg).toInt(&ok);
            if (ok && tOut > 0) {
                timeout = tOut;
            } else {
                writeMsg("timeout value should be an integer");
                printHelp = true;
            }
        } else if (arg == QLatin1String("-a") || arg == QLatin1String("--args")) {
            extraArgs = args.mid(iarg + 1, args.size() - iarg - 1);
            iarg = args.size();
        } else if (arg == QLatin1String("-h") || arg == QLatin1String("--help")) {
            printHelp = true;
        } else {
            writeMsg(QString::fromLatin1("unexpected argument \"%1\"").arg(arg));
        }
    }
    if (printHelp) {
        out.writeStartElement(QLatin1String("msg"));
        out.writeCharacters(QLatin1String("iostool [--id <device_id>] [--bundle <bundle.app>] [--install] [--run] [--debug]\n"));
        out.writeCharacters(QLatin1String("    [--device-info] [--timeout <timeout_in_ms>] [--verbose]\n")); // to do pass in env as stub does
        out.writeCharacters(QLatin1String("    [--args <arguments for the target app>]"));
        out.writeEndElement();
        doExit(-1);
        return;
    }
    outFile.flush();
    connect(manager, &IosDeviceManager::isTransferringApp, this, &IosTool::isTransferringApp);
    connect(manager, &IosDeviceManager::didTransferApp, this, &IosTool::didTransferApp);
    connect(manager, &IosDeviceManager::didStartApp, this, &IosTool::didStartApp);
    connect(manager, &IosDeviceManager::deviceInfo, this, &IosTool::deviceInfo);
    connect(manager, &IosDeviceManager::appOutput, this, &IosTool::appOutput);
    connect(manager, &IosDeviceManager::errorMsg, this, &IosTool::errorMsg);
    manager->watchDevices();
    const QRegularExpression qmlPortRe(QLatin1String("-qmljsdebugger=port:([0-9]+)"));
    for (const QString &arg : extraArgs) {
        const QRegularExpressionMatch match = qmlPortRe.match(arg);
        if (match.hasMatch()) {
            bool ok;
            int qmlPort = match.captured(1).toInt(&ok);
            if (ok && qmlPort > 0 && qmlPort <= 0xFFFF)
                m_qmlPort = match.captured(1);
            break;
        }
    }
    if (deviceInfo) {
        if (!bundlePath.isEmpty())
            writeMsg("--device-info overrides --bundle");
        ++opLeft;
        manager->requestDeviceInfo(deviceId, timeout);
    } else if (!bundlePath.isEmpty()) {
        switch (appOp) {
        case IosDeviceManager::None:
            break;
        case IosDeviceManager::Install:
        case IosDeviceManager::Run:
            ++opLeft;
            break;
        case IosDeviceManager::InstallAndRun:
            opLeft += 2;
            break;
        }
        maxProgress = 200;
        manager->requestAppOp(bundlePath, extraArgs, appOp, deviceId, timeout);
    }
    if (opLeft == 0)
        doExit(0);
}

void IosTool::stopXml(int errorCode)
{
    QMutexLocker l(&m_xmlMutex);
    out.writeEmptyElement(QLatin1String("exit"));
    out.writeAttribute(QLatin1String("code"), QString::number(errorCode));
    out.writeEndElement(); // result element (hopefully)
    out.writeEndDocument();
    outFile.flush();
}

void IosTool::doExit(int errorCode)
{
    stopXml(errorCode);
    QCoreApplication::exit(errorCode); // sometime does not really exit
    exit(errorCode);
}

void IosTool::isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                                const QString &info)
{
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)
    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("status"));
    out.writeAttribute(QLatin1String("progress"), QString::number(progress));
    out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
    out.writeCharacters(info);
    out.writeEndElement();
    outFile.flush();
}

void IosTool::didTransferApp(const QString &bundlePath, const QString &deviceId,
                             IosDeviceManager::OpStatus status)
{
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)
    {
        QMutexLocker l(&m_xmlMutex);
        if (status == IosDeviceManager::Success) {
            out.writeStartElement(QLatin1String("status"));
            out.writeAttribute(QLatin1String("progress"), QString::number(maxProgress));
            out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
            out.writeCharacters(QLatin1String("App Transferred"));
            out.writeEndElement();
        }
        out.writeEmptyElement(QLatin1String("app_transfer"));
        out.writeAttribute(QLatin1String("status"),
                           (status == IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        outFile.flush();
    }
    if (status != IosDeviceManager::Success || --opLeft == 0)
        doExit((status == IosDeviceManager::Success) ? 0 : -1);
}

void IosTool::didStartApp(const QString &bundlePath, const QString &deviceId,
                          IosDeviceManager::OpStatus status, ServiceConnRef conn, int gdbFd,
                          DeviceSession *deviceSession)
{
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeEmptyElement(QLatin1String("app_started"));
        out.writeAttribute(QLatin1String("status"),
                           (status == IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        outFile.flush();
    }
    if (status != IosDeviceManager::Success || appOp == IosDeviceManager::Install) {
        doExit();
        return;
    }
    if (gdbFd <= 0) {
        writeMsg("no gdb connection");
        doExit(-2);
        return;
    }
    if (appOp != IosDeviceManager::InstallAndRun && appOp != IosDeviceManager::Run) {
        writeMsg(QString::fromLatin1("unexpected appOp value %1").arg(appOp));
        doExit(-3);
        return;
    }
    if (deviceSession) {
        int qmlPort = deviceSession->qmljsDebugPort();
        if (qmlPort) {
            qmlServer = new QmlRelayServer(this, qmlPort, deviceSession);
            qmlServer->startServer();
        }
    }
    if (debug) {
        gdbServer = new GdbRelayServer(this, gdbFd, conn);
        if (!gdbServer->startServer()) {
            doExit(-4);
            return;
        }
    }
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeStartElement(QLatin1String("server_ports"));
        out.writeAttribute(QLatin1String("gdb_server"),
                           QString::number(gdbServer ? gdbServer->serverPort() : -1));
        out.writeAttribute(QLatin1String("qml_server"),
                           QString::number(qmlServer ? qmlServer->serverPort() : -1));
        out.writeEndElement();
        outFile.flush();
    }
    if (!debug) {
        gdbRunner = new GdbRunner(this, conn);
        // we should not stop the event handling of the main thread
        // all output moves to the new thread (other option would be to signal it back)
        QThread *gdbProcessThread = new QThread();
        gdbRunner->moveToThread(gdbProcessThread);
        QObject::connect(gdbProcessThread, &QThread::started, gdbRunner, &GdbRunner::run);
        QObject::connect(gdbRunner, &GdbRunner::finished, gdbProcessThread, &QThread::quit);
        QObject::connect(gdbProcessThread, &QThread::finished,
                         gdbProcessThread, &QObject::deleteLater);
        gdbProcessThread->start();

        new std::thread([this]() -> void { readStdin();});
    }
}

void IosTool::writeMsg(const char *msg)
{
    writeMsg(QString::fromLatin1(msg));
}

void IosTool::writeMsg(const QString &msg)
{
    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("msg"));
    writeTextInElement(msg);
    out.writeCharacters(QLatin1String("\n"));
    out.writeEndElement();
    outFile.flush();
}

void IosTool::writeMaybeBin(const QString &extraMsg, const char *msg, quintptr len)
{
    char *buf2 = new char[len * 2 + 4];
    buf2[0] = '[';
    const char toHex[] = "0123456789abcdef";
    for (quintptr i = 0; i < len; ++i) {
        buf2[2 * i + 1] = toHex[(0xF & (msg[i] >> 4))];
        buf2[2 * i + 2] = toHex[(0xF & msg[i])];
    }
    buf2[2 * len + 1] = ']';
    buf2[2 * len + 2] = ' ';
    buf2[2 * len + 3] = 0;

    QMutexLocker l(&m_xmlMutex);
    out.writeStartElement(QLatin1String("msg"));
    out.writeCharacters(extraMsg);
    out.writeCharacters(QLatin1String(buf2));
    for (quintptr i = 0; i < len; ++i) {
        if (msg[i] < 0x20 || msg[i] > 0x7f)
            buf2[i] = '_';
        else
            buf2[i] = msg[i];
    }
    buf2[len] = 0;
    out.writeCharacters(QLatin1String(buf2));
    delete[] buf2;
    out.writeEndElement();
    outFile.flush();
}

void IosTool::deviceInfo(const QString &deviceId, const IosDeviceManager::Dict &devInfo)
{
    Q_UNUSED(deviceId)
    {
        QMutexLocker l(&m_xmlMutex);
        out.writeTextElement(QLatin1String("device_id"), deviceId);
        out.writeStartElement(QLatin1String("device_info"));
        for (auto i = devInfo.cbegin(); i != devInfo.cend(); ++i) {
            out.writeStartElement(QLatin1String("item"));
            out.writeTextElement(QLatin1String("key"), i.key());
            out.writeTextElement(QLatin1String("value"), i.value());
            out.writeEndElement();
        }
        out.writeEndElement();
        outFile.flush();
    }
    doExit();
}

void IosTool::writeTextInElement(const QString &output)
{
    const QRegularExpression controlCharRe(QLatin1String("[\x01-\x08]|\x0B|\x0C|[\x0E-\x1F]|\\0000"));
    int pos = 0;
    int oldPos = 0;

    while ((pos = output.indexOf(controlCharRe, pos)) != -1) {
        QMutexLocker l(&m_xmlMutex);
        out.writeCharacters(output.mid(oldPos, pos - oldPos));
        out.writeEmptyElement(QLatin1String("control_char"));
        out.writeAttribute(QLatin1String("code"), QString::number(output.at(pos).toLatin1()));
        pos += 1;
        oldPos = pos;
    }
    out.writeCharacters(output.mid(oldPos, output.length() - oldPos));
}

void IosTool::appOutput(const QString &output)
{
    QMutexLocker l(&m_xmlMutex);
    if (!inAppOutput)
        out.writeStartElement(QLatin1String("app_output"));
    writeTextInElement(output);
    if (!inAppOutput)
        out.writeEndElement();
    outFile.flush();
}

void IosTool::readStdin()
{
    int c = getchar();
    if (c == 'k') {
        QMetaObject::invokeMethod(this, "stopGdbRunner");
        errorMsg(QLatin1String("iostool: Killing inferior.\n"));
    } else if (c != EOF) {
        errorMsg(QLatin1String("iostool: Unexpected character in stdin, stop listening.\n"));
    }
}

void IosTool::errorMsg(const QString &msg)
{
    writeMsg(msg);
}

void IosTool::stopGdbRunner()
{
    if (gdbRunner) {
        gdbRunner->stop(0);
        QTimer::singleShot(100, this, &IosTool::stopGdbRunner2);
    }
}

void IosTool::stopGdbRunner2()
{
    if (gdbRunner)
        gdbRunner->stop(1);
}

void IosTool::stopRelayServers(int errorCode)
{
    if (echoRelays())
        writeMsg("gdbServerStops");
    if (qmlServer)
        qmlServer->stopServer();
    if (gdbServer)
        gdbServer->stopServer();
    doExit(errorCode);
}

}
