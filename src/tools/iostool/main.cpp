/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <qglobal.h>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QApplication>
#else
#include <QGuiApplication>
#endif
#include <QTextStream>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QFile>
#include <QMapIterator>
#include "iosdevicemanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


class IosTool: public QObject {
    Q_OBJECT
public:
    IosTool(QObject *parent = 0);
    void run(const QStringList &args);
    void doExit(int errorCode = 0);
    void writeMsg(const char *msg);
    void writeMsg(const QString &msg);
    void stopXml(int errorCode);
public slots:
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, int gdbFd);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
private:
    int maxProgress;
    int opLeft;
    bool debug;
    bool inAppOutput;
    bool splitAppOutput; // as QXmlStreamReader reports the text attributes atomically it is better to split
    Ios::IosDeviceManager::AppOp appOp;
    QFile outFile;
    QXmlStreamWriter out;
};

IosTool::IosTool(QObject *parent):
    QObject(parent),
    maxProgress(0),
    opLeft(0),
    debug(false),
    inAppOutput(false),
    splitAppOutput(true),
    appOp(Ios::IosDeviceManager::Install),
    outFile(),
    out(&outFile)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    outFile.open(stdout, QIODevice::WriteOnly, QFile::DontCloseHandle);
#else
    outFile.open(stdout, QIODevice::WriteOnly, QFileDevice::DontCloseHandle);
#endif
    out.setAutoFormatting(true);
    out.setCodec("UTF-8");
}

void IosTool::run(const QStringList &args)
{
    Ios::IosDeviceManager *manager = Ios::IosDeviceManager::instance();
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
        if (arg == QLatin1String("-device-id")) {
            if (++iarg == args.size()) {
                writeMsg("missing device id value after -device-id");
                printHelp = true;
            }
            deviceId = args.value(iarg);
        } else if (arg == QLatin1String("-bundle")) {
            if (++iarg == args.size()) {
                writeMsg("missing bundle path after -bundle");
                printHelp = true;
            }
            bundlePath = args.value(iarg);
        } else if (arg == QLatin1String("-deploy")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Install);
        } else if (arg == QLatin1String("-run")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Run);
        } else if (arg == QLatin1String("-debug")) {
            appOp = Ios::IosDeviceManager::AppOp(appOp | Ios::IosDeviceManager::Run);
            debug = true;
        } else if (arg == QLatin1String("-device-info")) {
            deviceInfo = true;
        } else if (arg == QLatin1String("-timeout")) {
            if (++iarg == args.size()) {
                writeMsg("missing timeout value after -timeout");
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
        } else if (arg == QLatin1String("-extra-args")) {
            ++iarg;
            extraArgs = args.mid(iarg, args.size() - iarg);
        } else if (arg == QLatin1String("-help") || arg == QLatin1String("--help")) {
            printHelp = true;
        } else {
            writeMsg(QString::fromLatin1("unexpected argument '%1'").arg(arg));
        }
    }
    if (printHelp) {
        out.writeStartElement(QLatin1String("msg"));
        out.writeCharacters(QLatin1String("iosTool [-device-id <deviceId>] [-bundle <pathToBundle>] [-deploy] [-run] [-debug]\n"));
        out.writeCharacters(QLatin1String("    [-device-info] [-timeout <timeoutIn_ms>]\n")); // to do pass in env as stub does
        out.writeCharacters(QLatin1String("    [-extra-args <arguments for the target app>]"));
        out.writeEndElement();
        doExit(-1);
        return;
    }
    outFile.flush();
    connect(manager,SIGNAL(isTransferringApp(QString,QString,int,QString)),
            SLOT(isTransferringApp(QString,QString,int,QString)));
    connect(manager,SIGNAL(didTransferApp(QString,QString,Ios::IosDeviceManager::OpStatus)),
            SLOT(didTransferApp(QString,QString,Ios::IosDeviceManager::OpStatus)));
    connect(manager,SIGNAL(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int)),
            SLOT(didStartApp(QString,QString,Ios::IosDeviceManager::OpStatus,int)));
    connect(manager,SIGNAL(deviceInfo(QString,Ios::IosDeviceManager::Dict)),
            SLOT(deviceInfo(QString,Ios::IosDeviceManager::Dict)));
    connect(manager,SIGNAL(appOutput(QString)), SLOT(appOutput(QString)));
    connect(manager,SIGNAL(errorMsg(QString)), SLOT(errorMsg(QString)));
    manager->watchDevices();
    if (deviceInfo) {
        if (!bundlePath.isEmpty())
            writeMsg("-device-info overrides bundlePath");
        ++opLeft;
        manager->requestDeviceInfo(deviceId, timeout);
    } else if (!bundlePath.isEmpty()) {
        switch (appOp) {
        case Ios::IosDeviceManager::None:
            break;
        case Ios::IosDeviceManager::Install:
        case Ios::IosDeviceManager::Run:
            ++opLeft;
            break;
        case Ios::IosDeviceManager::InstallAndRun:
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
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    out.writeStartElement(QLatin1String("status"));
    out.writeAttribute(QLatin1String("progress"), QString::number(progress));
    out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
    out.writeCharacters(info);
    out.writeEndElement();
    outFile.flush();
}

int send_fd(int socket, int fd_to_send)
{
    /* storage space needed for an ancillary element with a paylod of length is CMSG_SPACE(sizeof(length)) */
    char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
    int available_ancillary_element_buffer_space;

    /* at least one vector of one byte must be sent */
    char message_buffer[1];
    message_buffer[0] = '.';
    iovec io_vector[1];

    io_vector[0].iov_base = message_buffer;
    io_vector[0].iov_len = 1;

    /* initialize socket message */
    msghdr socket_message;
    memset(&socket_message, 0, sizeof(msghdr));
    socket_message.msg_iov = io_vector;
    socket_message.msg_iovlen = 1;

    /* provide space for the ancillary data */
    available_ancillary_element_buffer_space = CMSG_SPACE(sizeof(int));
    memset(ancillary_element_buffer, 0, available_ancillary_element_buffer_space);
    socket_message.msg_control = ancillary_element_buffer;
    socket_message.msg_controllen = available_ancillary_element_buffer_space;

    /* initialize a single ancillary data element for fd passing */
    cmsghdr *control_message = NULL;
    control_message = CMSG_FIRSTHDR(&socket_message);
    control_message->cmsg_level = SOL_SOCKET;
    control_message->cmsg_type = SCM_RIGHTS;
    control_message->cmsg_len = CMSG_LEN(sizeof(int));
    *((int *) CMSG_DATA(control_message)) = fd_to_send;

    return sendmsg(socket, &socket_message, 0);
}

void IosTool::didTransferApp(const QString &bundlePath, const QString &deviceId,
                             Ios::IosDeviceManager::OpStatus status)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    if (status == Ios::IosDeviceManager::Success) {
        out.writeStartElement(QLatin1String("status"));
        out.writeAttribute(QLatin1String("progress"), QString::number(maxProgress));
        out.writeAttribute(QLatin1String("max_progress"), QString::number(maxProgress));
        out.writeCharacters(QLatin1String("App Transferred"));
        out.writeEndElement();
    }
    out.writeEmptyElement(QLatin1String("app_transfer"));
    out.writeAttribute(QLatin1String("status"),
                       (status == Ios::IosDeviceManager::Success) ?
                           QLatin1String("SUCCESS") :
                           QLatin1String("FAILURE"));
    //out.writeCharacters(QString()); // trigger a complete closing of the empty element
    outFile.flush();
    if (status != Ios::IosDeviceManager::Success || --opLeft == 0)
        doExit(-1);
}

void IosTool::didStartApp(const QString &bundlePath, const QString &deviceId,
                             Ios::IosDeviceManager::OpStatus status, int gdbFd)
{
    Q_UNUSED(bundlePath);
    Q_UNUSED(deviceId);
    out.writeEmptyElement(QLatin1String("app_started"));
    out.writeAttribute(QLatin1String("status"),
                       (status == Ios::IosDeviceManager::Success) ?
                           QLatin1String("SUCCESS") :
                           QLatin1String("FAILURE"));
    //out.writeCharacters(QString()); // trigger a complete closing of the empty element
    outFile.flush();
    if (appOp == Ios::IosDeviceManager::Install) {
        doExit();
        return;
    }
    if (gdbFd <= 0) {
        writeMsg("no gdb connection");
        doExit(-2);
        return;
    }
    if (appOp != Ios::IosDeviceManager::InstallAndRun && appOp != Ios::IosDeviceManager::Run) {
        writeMsg(QString::fromLatin1("unexpected appOp value %1").arg(appOp));
        doExit(-3);
        return;
    }
    if (debug) {
        stopXml(0);
        // these are 67 characters, this is used as read size on the other side...
        const char *msg = "now sending the gdbserver socket, will need a unix socket to succeed";
        outFile.write(msg, strlen(msg));
        outFile.flush();
        int sent = send_fd(1, gdbFd);
        sleep(1);
        QCoreApplication::exit(sent == -1);
    } else {
        if (!splitAppOutput) {
            out.writeStartElement(QLatin1String("app_output"));
            inAppOutput = true;
        }
        outFile.flush();
        Ios::IosDeviceManager::instance()->processGdbServer(gdbFd);
        if (!splitAppOutput) {
            inAppOutput = false;
            out.writeEndElement();
        }
        outFile.flush();
        close(gdbFd);
        doExit();
    }
}

void IosTool::writeMsg(const char *msg)
{
    out.writeTextElement(QLatin1String("msg"), QLatin1String(msg));
    outFile.flush();
}

void IosTool::writeMsg(const QString &msg)
{
    out.writeTextElement(QLatin1String("msg"), msg);
    outFile.flush();
}

void IosTool::deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &devInfo)
{
    Q_UNUSED(deviceId);
    out.writeTextElement(QLatin1String("device_id"), deviceId);
    out.writeStartElement(QLatin1String("device_info"));
    QMapIterator<QString,QString> i(devInfo);
    while (i.hasNext()) {
        i.next();
        out.writeStartElement(QLatin1String("item"));
        out.writeTextElement(QLatin1String("key"), i.key());
        out.writeTextElement(QLatin1String("value"), i.value());
        out.writeEndElement();
    }
    out.writeEndElement();
    outFile.flush();
    doExit();
}

void IosTool::appOutput(const QString &output)
{
    if (inAppOutput)
        out.writeCharacters(output);
    else
        out.writeTextElement(QLatin1String("app_output"), output);
    outFile.flush();
}

void IosTool::errorMsg(const QString &msg)
{
    writeMsg(msg + QLatin1Char('\n'));
}


int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QApplication a(argc, argv);
#else
    QGuiApplication a(argc, argv);
#endif
    IosTool tool;
    tool.run(QCoreApplication::arguments());
    int res = a.exec();
    exit(res);
}

#include "main.moc"
