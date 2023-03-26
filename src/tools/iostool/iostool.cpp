// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iostool.h"

#include "relayserver.h"
#include "gdbrunner.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QThread>

namespace Ios {

IosTool::IosTool(QObject *parent):
    QObject(parent),
    m_xmlWriter(&m_outputFile)
{
    m_outputFile.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle);
    m_xmlWriter.setAutoFormatting(true);
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

    m_xmlWriter.writeStartDocument();
    m_xmlWriter.writeStartElement(QLatin1String("query_result"));

    QCommandLineParser cmdLineParser;
    cmdLineParser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
    cmdLineParser.setApplicationDescription("iOS Deployment and run helper");

    const QList<QCommandLineOption> options{
        QCommandLineOption(QStringList{"h", "help"}),
        QCommandLineOption(QStringList{"i", "id"}, "target device id", "id"),
        QCommandLineOption(QStringList{"b", "bundle"}, "path to the target bundle.app", "bundle"),
        QCommandLineOption("delta-path", "path to the deploy delta directory", "delta-path"),
        QCommandLineOption("install", "Install a bundle on a device"),
        QCommandLineOption(QStringList{"r", "run"}, "Run the bundle on the device"),
        QCommandLineOption(QStringList{"d", "debug"}, "Debug the bundle on the device"),
        QCommandLineOption("device-info", "Retrieve information about the selected device"),
        QCommandLineOption(QStringList{"t", "timeout"}, "Operation timeout (s)", "timeout"),
        QCommandLineOption(QStringList{"v", "verbose"}, "Verbose output"),
    };

    cmdLineParser.addOptions(options);

    cmdLineParser.process(args);

    if (cmdLineParser.isSet("verbose"))
        m_echoRelays = true;

    if (cmdLineParser.isSet("id"))
        deviceId = cmdLineParser.value("id");

    if (cmdLineParser.isSet("bundle"))
        bundlePath = cmdLineParser.value("bundle");

    if (cmdLineParser.isSet("delta-path"))
        m_deltasPath = cmdLineParser.value("delta-path");

    if (cmdLineParser.isSet("install")) {
        m_requestedOperation = IosDeviceManager::AppOp(m_requestedOperation
                                                       | IosDeviceManager::Install);
    }
    else if (cmdLineParser.isSet("run")) {
        m_requestedOperation = IosDeviceManager::AppOp(m_requestedOperation | IosDeviceManager::Run);
    }

    if (cmdLineParser.isSet("debug")) {
        m_requestedOperation = IosDeviceManager::AppOp(m_requestedOperation | IosDeviceManager::Run);
        m_debug = true;
    }

    if (cmdLineParser.isSet("device-info"))
        deviceInfo = true;

    if (cmdLineParser.isSet("timeout")) {
        bool ok = false;
        timeout = cmdLineParser.value("timeout").toInt(&ok);
        if (!ok || timeout < 0) {
            writeMsg("timeout value should be an integer");
            printHelp = true;
        }
    }

    extraArgs = cmdLineParser.positionalArguments();

    if (printHelp || cmdLineParser.isSet("help")) {
        m_xmlWriter.writeStartElement(QLatin1String("msg"));
        m_xmlWriter.writeCharacters(
            QLatin1String("iostool [--id <device_id>] [--bundle <bundle.app>] [--delta-path] "
                          "[--install] [--run] [--debug]\n"));
        m_xmlWriter.writeCharacters(QLatin1String(
            "    [--device-info] [--timeout <timeout_in_ms>] [--verbose]\n")); // to do pass in env as stub does
        m_xmlWriter.writeCharacters(QLatin1String("    [-- <arguments for the target app>]"));
        m_xmlWriter.writeEndElement();
        doExit(-1);
        return;
    }

    m_outputFile.flush();
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
        ++m_operationsRemaining;
        manager->requestDeviceInfo(deviceId, timeout);
    } else if (!bundlePath.isEmpty()) {
        switch (m_requestedOperation) {
        case IosDeviceManager::None:
            break;
        case IosDeviceManager::Install:
        case IosDeviceManager::Run:
            ++m_operationsRemaining;
            break;
        case IosDeviceManager::InstallAndRun:
            m_operationsRemaining += 2;
            break;
        }
        m_maxProgress = 200;
        manager->requestAppOp(bundlePath, extraArgs, m_requestedOperation, deviceId, timeout, m_deltasPath);
    }
    if (m_operationsRemaining == 0)
        doExit(0);
}

void IosTool::stopXml(int errorCode)
{
    QMutexLocker l(&m_xmlMutex);
    m_xmlWriter.writeEmptyElement(QLatin1String("exit"));
    m_xmlWriter.writeAttribute(QLatin1String("code"), QString::number(errorCode));
    m_xmlWriter.writeEndElement(); // result element (hopefully)
    m_xmlWriter.writeEndDocument();
    m_outputFile.flush();
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
    m_xmlWriter.writeStartElement(QLatin1String("status"));
    m_xmlWriter.writeAttribute(QLatin1String("progress"), QString::number(progress));
    m_xmlWriter.writeAttribute(QLatin1String("max_progress"), QString::number(m_maxProgress));
    m_xmlWriter.writeCharacters(info);
    m_xmlWriter.writeEndElement();
    m_outputFile.flush();
}

void IosTool::didTransferApp(const QString &bundlePath, const QString &deviceId,
                             IosDeviceManager::OpStatus status)
{
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)
    {
        QMutexLocker l(&m_xmlMutex);
        if (status == IosDeviceManager::Success) {
            m_xmlWriter.writeStartElement(QLatin1String("status"));
            m_xmlWriter.writeAttribute(QLatin1String("progress"), QString::number(m_maxProgress));
            m_xmlWriter.writeAttribute(QLatin1String("max_progress"), QString::number(m_maxProgress));
            m_xmlWriter.writeCharacters(QLatin1String("App Transferred"));
            m_xmlWriter.writeEndElement();
        }
        m_xmlWriter.writeEmptyElement(QLatin1String("app_transfer"));
        m_xmlWriter.writeAttribute(QLatin1String("status"),
                           (status == IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        m_outputFile.flush();
    }
    if (status != IosDeviceManager::Success || --m_operationsRemaining == 0)
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
        m_xmlWriter.writeEmptyElement(QLatin1String("app_started"));
        m_xmlWriter.writeAttribute(QLatin1String("status"),
                           (status == IosDeviceManager::Success) ?
                               QLatin1String("SUCCESS") :
                               QLatin1String("FAILURE"));
        //out.writeCharacters(QString()); // trigger a complete closing of the empty element
        m_outputFile.flush();
    }
    if (status != IosDeviceManager::Success || m_requestedOperation == IosDeviceManager::Install) {
        doExit();
        return;
    }
    if (gdbFd <= 0) {
        writeMsg("no gdb connection");
        doExit(-2);
        return;
    }
    if (m_requestedOperation != IosDeviceManager::InstallAndRun && m_requestedOperation != IosDeviceManager::Run) {
        writeMsg(QString::fromLatin1("unexpected appOp value %1").arg(m_requestedOperation));
        doExit(-3);
        return;
    }
    if (deviceSession) {
        int qmlPort = deviceSession->qmljsDebugPort();
        if (qmlPort) {
            m_qmlServer = std::make_unique<QmlRelayServer>(this, qmlPort, deviceSession);
            m_qmlServer->startServer();
        }
    }
    if (m_debug) {
        m_gdbServer = std::make_unique<GdbRelayServer>(this, gdbFd, conn);
        if (!m_gdbServer->startServer()) {
            doExit(-4);
            return;
        }
    }
    {
        QMutexLocker l(&m_xmlMutex);
        m_xmlWriter.writeStartElement(QLatin1String("server_ports"));
        m_xmlWriter.writeAttribute(QLatin1String("gdb_server"),
                           QString::number(m_gdbServer ? m_gdbServer->serverPort() : -1));
        m_xmlWriter.writeAttribute(QLatin1String("qml_server"),
                           QString::number(m_qmlServer ? m_qmlServer->serverPort() : -1));
        m_xmlWriter.writeEndElement();
        m_outputFile.flush();
    }
    if (!m_debug) {
        m_gdbRunner = std::make_unique<GdbRunner>(this, conn);

        // we should not stop the event handling of the main thread
        // all output moves to the new thread (other option would be to signal it back)
        QThread *gdbProcessThread = new QThread();
        m_gdbRunner->moveToThread(gdbProcessThread);
        QObject::connect(gdbProcessThread, &QThread::started, m_gdbRunner.get(), &GdbRunner::run);
        QObject::connect(m_gdbRunner.get(), &GdbRunner::finished, gdbProcessThread, &QThread::quit);
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
    m_xmlWriter.writeStartElement(QLatin1String("msg"));
    writeTextInElement(msg);
    m_xmlWriter.writeCharacters(QLatin1String("\n"));
    m_xmlWriter.writeEndElement();
    m_outputFile.flush();
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
    m_xmlWriter.writeStartElement(QLatin1String("msg"));
    m_xmlWriter.writeCharacters(extraMsg);
    m_xmlWriter.writeCharacters(QLatin1String(buf2));
    for (quintptr i = 0; i < len; ++i) {
        if (msg[i] < 0x20 || msg[i] > 0x7f)
            buf2[i] = '_';
        else
            buf2[i] = msg[i];
    }
    buf2[len] = 0;
    m_xmlWriter.writeCharacters(QLatin1String(buf2));
    delete[] buf2;
    m_xmlWriter.writeEndElement();
    m_outputFile.flush();
}

void IosTool::deviceInfo(const QString &deviceId, const IosDeviceManager::Dict &devInfo)
{
    Q_UNUSED(deviceId)
    {
        QMutexLocker l(&m_xmlMutex);
        m_xmlWriter.writeTextElement(QLatin1String("device_id"), deviceId);
        m_xmlWriter.writeStartElement(QLatin1String("device_info"));
        for (auto i = devInfo.cbegin(); i != devInfo.cend(); ++i) {
            m_xmlWriter.writeStartElement(QLatin1String("item"));
            m_xmlWriter.writeTextElement(QLatin1String("key"), i.key());
            m_xmlWriter.writeTextElement(QLatin1String("value"), i.value());
            m_xmlWriter.writeEndElement();
        }
        m_xmlWriter.writeEndElement();
        m_outputFile.flush();
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
        m_xmlWriter.writeCharacters(output.mid(oldPos, pos - oldPos));
        m_xmlWriter.writeEmptyElement(QLatin1String("control_char"));
        m_xmlWriter.writeAttribute(QLatin1String("code"), QString::number(output.at(pos).toLatin1()));
        pos += 1;
        oldPos = pos;
    }
    m_xmlWriter.writeCharacters(output.mid(oldPos, output.length() - oldPos));
}

void IosTool::appOutput(const QString &output)
{
    QMutexLocker l(&m_xmlMutex);
    if (!m_inAppOutput)
        m_xmlWriter.writeStartElement(QLatin1String("app_output"));
    writeTextInElement(output);
    if (!m_inAppOutput)
        m_xmlWriter.writeEndElement();
    m_outputFile.flush();
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
    if (m_gdbRunner) {
        m_gdbRunner->stop(0);
        QTimer::singleShot(100, this, &IosTool::stopGdbRunner2);
    }
}

void IosTool::stopGdbRunner2()
{
    if (m_gdbRunner)
        m_gdbRunner->stop(1);
}

void IosTool::stopRelayServers(int errorCode)
{
    if (echoRelays())
        writeMsg("gdbServerStops");
    if (m_qmlServer)
        m_qmlServer->stopServer();
    if (m_gdbServer)
        m_gdbServer->stopServer();
    doExit(errorCode);
}

}
