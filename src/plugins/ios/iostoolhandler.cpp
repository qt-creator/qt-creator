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

#include "iostoolhandler.h"
#include "iosconfigurations.h"

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QProcess>
#include <QXmlStreamReader>
#include <QSocketNotifier>
#include <QDebug>
#include <QCoreApplication>
#include <QList>
#include <QScopedArrayPointer>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

static const bool debugToolHandler = false;

namespace Ios {

namespace Internal {

class MyProcess: public QProcess
{
    Q_OBJECT
public:
    explicit MyProcess(QObject *parent = 0);
    int processOutput();
    QSocketNotifier *notifier();
protected:
    virtual void setupChildProcess();
private:
    int m_sockets[2];
    QSocketNotifier *m_notifier;
};

struct ParserState {
    enum Kind {
        Msg,
        DeviceId,
        Key,
        Value,
        QueryResult,
        AppOutput,
        AppStarted,
        InferiorPid,
        Item,
        Status,
        AppTransfer,
        DeviceInfo,
        Exit
    };
    Kind kind;
    QString elName;
    QString chars;
    QString key;
    QString value;
    QMap<QString,QString> info;
    int progress, maxProgress;
    bool collectChars() {
        switch (kind) {
        case Msg:
        case DeviceId:
        case Key:
        case Value:
        case Status:
        case InferiorPid:
            return true;
        case QueryResult:
        case AppOutput:
        case AppStarted:
        case AppTransfer:
        case Item:
        case DeviceInfo:
        case Exit:
            return false;
        }
    }

    ParserState(Kind kind) :
        kind(kind) { }
};

class IosToolHandlerPrivate
{
public:
    enum State {
        NonStarted,
        Starting,
        StartedInferior,
        XmlEndSeenNotProcessed,
        XmlEndProcessed,
        Stopped
    };
    enum Op {
        OpNone,
        OpAppTransfer,
        OpDeviceInfo,
        OpAppRun
    };

    explicit IosToolHandlerPrivate(IosToolHandler::DeviceType devType, IosToolHandler *q);
    virtual void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                    int timeout = 1000) = 0;
    virtual void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000) = 0;
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000) = 0;
    bool isRunning();
    void start(const QString &exe, const QStringList &args);
    void stop();

    // signals
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           int maxProgress, const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        IosToolHandler::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     IosToolHandler::OpStatus status);
    void gotGdbserverSocket(const QString &bundlePath, const QString &deviceId, int gdbFd);
    void gotInferiorPid(const QString &bundlePath, const QString &deviceId, pid_t pid);
    void deviceInfo(const QString &deviceId, const IosToolHandler::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void toolExited(int code);
    // slots
    void subprocessError(QProcess::ProcessError error);
    void subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void subprocessHasData(int socket);
    virtual bool expectsFileDescriptor() = 0;
protected:
    int checkForXmlEnd();
    void processXml();

    IosToolHandler *q;
    MyProcess process;
    QXmlStreamReader outputParser;
    QString deviceId;
    QString bundlePath;
    IosToolHandler::RunKind runKind;
    State state;
    Op op;
    IosToolHandler::DeviceType devType;
    static const int lookaheadSize = 67;
    QByteArray buffer;
    QByteArray currentData;
    int iBegin, iEnd, gdbSocket;
    QList<ParserState> stack;
};

class IosDeviceToolHandlerPrivate : public IosToolHandlerPrivate
{
public:
    explicit IosDeviceToolHandlerPrivate(IosToolHandler::DeviceType devType, IosToolHandler *q);
    virtual void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                    int timeout = 1000);
    virtual void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000);
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    virtual bool expectsFileDescriptor();
};

class IosSimulatorToolHandlerPrivate : public IosToolHandlerPrivate
{
public:
    explicit IosSimulatorToolHandlerPrivate(IosToolHandler::DeviceType devType, IosToolHandler *q);
    virtual void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                    int timeout = 1000);
    virtual void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000);
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    virtual bool expectsFileDescriptor();
private:
    void addDeviceArguments(QStringList &args) const;
};

MyProcess::MyProcess(QObject *parent) : QProcess(parent)
{
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, &m_sockets[0]) == -1) {
        qDebug() << "IosToolHandler socketpair failed ";
    }
    shutdown(m_sockets[0], SHUT_WR);
    m_notifier = new QSocketNotifier(m_sockets[0], QSocketNotifier::Read, this);
}

int MyProcess::processOutput()
{
    return m_sockets[0];
}

QSocketNotifier *MyProcess::notifier()
{
    return m_notifier;
}

void MyProcess::setupChildProcess()
{
    if (dup2(m_sockets[1], 1) == -1) { // use the unix socket as stdout
        qDebug() << "IosToolHandler dup2 call failed";
        emit finished(-1, QProcess::CrashExit);
        exit(-1);
    }
    shutdown(1, SHUT_RD); // leave open for handshake when transferring fd?
}

IosToolHandlerPrivate::IosToolHandlerPrivate(IosToolHandler::DeviceType devType,
                                             Ios::IosToolHandler *q) :
    q(q), state(NonStarted), devType(devType), buffer(4*lookaheadSize, 0), iBegin(0), iEnd(0),
    gdbSocket(-1)
{
    QObject::connect(process.notifier(), SIGNAL(activated(int)), q, SLOT(subprocessHasData(int)));
    QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)),
            q, SLOT(subprocessFinished(int,QProcess::ExitStatus)));
    QObject::connect(&process, SIGNAL(error(QProcess::ProcessError)),
            q, SLOT(subprocessError(QProcess::ProcessError)));
    int accessFlags = fcntl(process.processOutput(), F_GETFL);
    if (fcntl(process.processOutput(), F_SETFL, accessFlags | O_NONBLOCK) == -1)
        qDebug() << "IosToolHandler fcntl F_SETFL failed to set non blocking mode"
                 << qt_error_string(errno);
}

bool IosToolHandlerPrivate::isRunning()
{
    return process.state() != QProcess::NotRunning;
}

void IosToolHandlerPrivate::start(const QString &exe, const QStringList &args)
{
    QTC_CHECK(state == NonStarted);
    state = Starting;
    if (debugToolHandler)
        qDebug() << "running " << exe << args;
    process.start(exe, args);
    state = StartedInferior;
}

void IosToolHandlerPrivate::stop()
{
    if (debugToolHandler)
        qDebug() << "IosToolHandlerPrivate::stop";
    if (process.state() != QProcess::NotRunning) {
        close(process.processOutput());
        process.close();
        process.kill();
    }
    if (state != Stopped) {
        state = Stopped;
        emit q->finished(q);
    }
}

// signals
void IosToolHandlerPrivate::isTransferringApp(const QString &bundlePath, const QString &deviceId,
                                              int progress, int maxProgress, const QString &info)
{
    emit q->isTransferringApp(q, bundlePath, deviceId, progress, maxProgress, info);
}

void IosToolHandlerPrivate::didTransferApp(const QString &bundlePath, const QString &deviceId,
                    Ios::IosToolHandler::OpStatus status)
{
    emit q->didTransferApp(q, bundlePath, deviceId, status);
}

void IosToolHandlerPrivate::didStartApp(const QString &bundlePath, const QString &deviceId,
                                        IosToolHandler::OpStatus status)
{
    emit q->didStartApp(q, bundlePath, deviceId, status);
}

void IosToolHandlerPrivate::gotGdbserverSocket(const QString &bundlePath, const QString &deviceId,
                  int gdbFd)
{
    emit q->gotGdbserverSocket(q, bundlePath, deviceId, gdbFd);
}

void IosToolHandlerPrivate::gotInferiorPid(const QString &bundlePath, const QString &deviceId,
                                           pid_t pid)
{
    emit q->gotInferiorPid(q, bundlePath, deviceId, pid);
}

void IosToolHandlerPrivate::deviceInfo(const QString &deviceId,
                                       const Ios::IosToolHandler::Dict &info)
{
    emit q->deviceInfo(q, deviceId, info);
}

void IosToolHandlerPrivate::appOutput(const QString &output)
{
    emit q->appOutput(q, output);
}

void IosToolHandlerPrivate::errorMsg(const QString &msg)
{
    emit q->errorMsg(q, msg);
}

void IosToolHandlerPrivate::toolExited(int code)
{
    emit q->toolExited(q, code);
}

void IosToolHandlerPrivate::subprocessError(QProcess::ProcessError error)
{
    switch (state) {
    case NonStarted:
        qDebug() << "subprocessError() when state was NonStarted";
        // pass
    case Starting:
        switch (op){
        case OpNone:
            qDebug() << "subprocessError() when op is OpNone";
            break;
        case OpAppTransfer:
            didTransferApp(bundlePath, deviceId, IosToolHandler::Failure);
            break;
        case OpAppRun:
            didStartApp(bundlePath, deviceId, IosToolHandler::Failure);
            break;
        case OpDeviceInfo:
            break;
        }
        // pass
    case StartedInferior:
    case XmlEndSeenNotProcessed:
    case XmlEndProcessed:
        errorMsg(q->tr("Subprocess Error %1").arg(error));
        toolExited(-1);
        break;
    case Stopped:
        qDebug() << "IosToolHandler, subprocessError() in an already stopped process";
    }
}

void IosToolHandlerPrivate::subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // process potentially pending data
    subprocessHasData(process.processOutput());
    switch (state) {
    case NonStarted:
        qDebug() << "subprocessFinished() when state was NonStarted";
        // pass
    case Starting:
        switch (op){
        case OpNone:
            qDebug() << "subprocessFinished() when op was OpNone";
            break;
        case OpAppTransfer:
            didTransferApp(bundlePath, deviceId, IosToolHandler::Failure);
            break;
        case OpAppRun:
            didStartApp(bundlePath, deviceId, IosToolHandler::Failure);
            break;
        case OpDeviceInfo:
            break;
        }
        // pass
    case StartedInferior:
    case XmlEndSeenNotProcessed:
    case XmlEndProcessed:
        toolExited((exitStatus == QProcess::CrashExit && exitCode == 0) ? -1 : exitCode);
        break;
    case Stopped:
        if (debugToolHandler)
            qDebug() << "IosToolHandler, subprocessFinished() in an already stopped process (normal)";
        break;
    }
}

#ifndef CMSG_SPACE
size_t CMSG_SPACE(size_t len) {
        msghdr msg;
        cmsghdr cmsg;
        msg.msg_control = &cmsg;
        msg.msg_controllen =  ~socklen_t(0); /* To maximize the chance that CMSG_NXTHDR won't return NULL */
        cmsg.cmsg_len = CMSG_LEN(len);
        return reinterpret_cast<unsigned char *>(CMSG_NXTHDR(&msg, &cmsg)) - reinterpret_cast<unsigned char *>(&cmsg);
}
#endif

int recv_fd(int socket)
{
    int sent_fd;
    char message_buffer[1];

    iovec io_vector[1];
    memset(&io_vector[0], 0, sizeof(iovec));
    /* setup a place to fill in message contents */
    io_vector[0].iov_base = message_buffer;
    io_vector[0].iov_len = 1;

    msghdr socket_message;
    memset(&socket_message, 0, sizeof(struct msghdr));
    socket_message.msg_iov = io_vector;
    socket_message.msg_iovlen = 1;

    /* provide space for the ancillary data */
    size_t dimAncillaryEl = CMSG_SPACE(sizeof(int));
    QScopedArrayPointer<char> ancillary_element_buffer(new char[dimAncillaryEl]);
    memset(ancillary_element_buffer.data(), 0, dimAncillaryEl);
    socket_message.msg_control = ancillary_element_buffer.data();
    socket_message.msg_controllen = dimAncillaryEl;

    int flags = 0;
#ifdef MSG_CMSG_CLOEXEC
    flags = MSG_CMSG_CLOEXEC;
#endif
    if (recvmsg(socket, &socket_message, flags) < 0)
        return -1;

    if (message_buffer[0] != '.') {
        qDebug() << "IosToolHandler, unexpected inband data when receiving socket";
        return -1;
    }

    if ((socket_message.msg_flags & MSG_CTRUNC) == MSG_CTRUNC) {
        qDebug() << "IosToolHandler, not provide enough space for the ancillary element array";
        return -1;
    }

    /* iterate ancillary elements */
    cmsghdr *control_message = NULL;
    for (control_message = CMSG_FIRSTHDR(&socket_message);
        control_message != NULL;
        control_message = CMSG_NXTHDR(&socket_message, control_message)) {
        if ( (control_message->cmsg_level == SOL_SOCKET) &&
                (control_message->cmsg_type == SCM_RIGHTS) ) {
            sent_fd = *((int *) CMSG_DATA(control_message));
            return sent_fd;
        }
    }

    return -1;
}

int IosToolHandlerPrivate::checkForXmlEnd()
{
    const char *xmlEnd = "</query_result>";
    int lenXmlEnd = 15;
    int i = 0, j = 0;
    while (i < lenXmlEnd && j < iEnd) {
        if (buffer.at(j) != xmlEnd[i]) {
            if (i == 0) {
                ++j;
                if (j + lenXmlEnd > iEnd)
                    break;
            } else {
                i = 0;
            }
        } else {
            ++i;
            ++j;
        }
    }
    if (i == lenXmlEnd)
        return j;
    return -1;
}

void IosToolHandlerPrivate::processXml()
{
    while (!outputParser.atEnd()) {
        QXmlStreamReader::TokenType tt = outputParser.readNext();
        //qDebug() << "processXml, tt=" << tt;
        switch (tt) {
        case QXmlStreamReader::NoToken:
            // The reader has not yet read anything.
            continue;
        case QXmlStreamReader::Invalid:
            // An error has occurred, reported in error() and errorString().
            break;
        case QXmlStreamReader::StartDocument:
            // The reader reports the XML version number in documentVersion(), and the encoding
            // as specified in the XML document in documentEncoding(). If the document is declared
            // standalone, isStandaloneDocument() returns true; otherwise it returns false.
            break;
        case QXmlStreamReader::EndDocument:
            // The reader reports the end of the document.
            // state = XmlEndProcessed;
            break;
        case QXmlStreamReader::StartElement:
            // The reader reports the start of an element with namespaceUri() and name(). Empty
            // elements are also reported as StartElement, followed directly by EndElement.
            // The convenience function readElementText() can be called to concatenate all content
            // until the corresponding EndElement. Attributes are reported in attributes(),
            // namespace declarations in namespaceDeclarations().
        {
            QStringRef elName = outputParser.name();
            if (elName == QLatin1String("msg")) {
                stack.append(ParserState(ParserState::Msg));
            } else if (elName == QLatin1String("exit")) {
                stack.append(ParserState(ParserState::Exit));
                toolExited(outputParser.attributes().value(QLatin1String("code"))
                           .toString().toInt());
            } else if (elName == QLatin1String("device_id")) {
                stack.append(ParserState(ParserState::DeviceId));
            } else if (elName == QLatin1String("key")) {
                stack.append(ParserState(ParserState::Key));
            } else if (elName == QLatin1String("value")) {
                stack.append(ParserState(ParserState::Value));
            } else if (elName == QLatin1String("query_result")) {
                stack.append(ParserState(ParserState::QueryResult));
            } else if (elName == QLatin1String("app_output")) {
                stack.append(ParserState(ParserState::AppOutput));
            } else if (elName == QLatin1String("item")) {
                stack.append(ParserState(ParserState::Item));
            } else if (elName == QLatin1String("status")) {
                ParserState pState(ParserState::Status);
                QXmlStreamAttributes attributes = outputParser.attributes();
                pState.progress = attributes.value(QLatin1String("progress")).toString().toInt();
                pState.maxProgress = attributes.value(QLatin1String("max_progress")).toString().toInt();
                stack.append(pState);
            } else if (elName == QLatin1String("app_started")) {
                stack.append(ParserState(ParserState::AppStarted));
                QXmlStreamAttributes attributes = outputParser.attributes();
                QStringRef statusStr = attributes.value(QLatin1String("status"));
                Ios::IosToolHandler::OpStatus status = Ios::IosToolHandler::Unknown;
                if (statusStr.compare(QLatin1String("success"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Success;
                else if (statusStr.compare(QLatin1String("failure"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Failure;
                didStartApp(bundlePath, deviceId, status);
            } else if (elName == QLatin1String("app_transfer")) {
                stack.append(ParserState(ParserState::AppTransfer));
                QXmlStreamAttributes attributes = outputParser.attributes();
                QStringRef statusStr = attributes.value(QLatin1String("status"));
                Ios::IosToolHandler::OpStatus status = Ios::IosToolHandler::Unknown;
                if (statusStr.compare(QLatin1String("success"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Success;
                else if (statusStr.compare(QLatin1String("failure"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Failure;
                emit didTransferApp(bundlePath, deviceId, status);
            } else if (elName == QLatin1String("device_info")) {
                stack.append(ParserState(ParserState::DeviceInfo));
            } else if (elName == QLatin1String("inferior_pid")) {
                stack.append(ParserState(ParserState::InferiorPid));
            } else {
                qDebug() << "unexpected element " << elName;
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            // The reader reports the end of an element with namespaceUri() and name().
        {
            ParserState p = stack.last();
            stack.removeLast();
            switch (p.kind) {
            case ParserState::Msg:
                errorMsg(p.chars);
                break;
            case ParserState::DeviceId:
                if (deviceId.isEmpty())
                    deviceId = p.chars;
                else
                    QTC_CHECK(deviceId.compare(p.chars, Qt::CaseInsensitive) == 0);
                break;
            case ParserState::Key:
                stack.last().key = p.chars;
                break;
            case ParserState::Value:
                stack.last().value = p.chars;
                break;
            case ParserState::Status:
                isTransferringApp(bundlePath, deviceId, p.progress, p.maxProgress, p.chars);
                break;
            case ParserState::QueryResult:
                state = XmlEndProcessed;
                break;
            case ParserState::AppOutput:
                break;
            case ParserState::AppStarted:
                break;
            case ParserState::AppTransfer:
                break;
            case ParserState::Item:
                stack.last().info.insert(p.key, p.value);
                break;
            case ParserState::DeviceInfo:
                deviceInfo(deviceId, p.info);
                break;
            case ParserState::Exit:
                break;
            case ParserState::InferiorPid:
                gotInferiorPid(bundlePath, deviceId, pid_t(p.chars.toInt()));
                break;
            }
            break;
        }
        case QXmlStreamReader::Characters:
            // The reader reports characters in text(). If the characters are all white-space,
            // isWhitespace() returns true. If the characters stem from a CDATA section,
            // isCDATA() returns true.
            if (stack.isEmpty())
                break;
            if (stack.last().kind == ParserState::AppOutput)
                emit appOutput(outputParser.text().toString());
            else if (stack.last().collectChars())
                stack.last().chars.append(outputParser.text());
            break;
        case QXmlStreamReader::Comment:
            // The reader reports a comment in text().
            break;
        case QXmlStreamReader::DTD:
            // The reader reports a DTD in text(), notation declarations in notationDeclarations(),
            // and entity declarations in entityDeclarations(). Details of the DTD declaration are
            // reported in in dtdName(), dtdPublicId(), and dtdSystemId().
            break;
        case QXmlStreamReader::EntityReference:
            // The reader reports an entity reference that could not be resolved. The name of
            // the reference is reported in name(), the replacement text in text().
            break;
        case QXmlStreamReader::ProcessingInstruction:
            break;
        }
    }
    if (outputParser.hasError()
            && outputParser.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qDebug() << "error parsing iosTool output:" << outputParser.errorString();
        stop();
    }
}

void IosToolHandlerPrivate::subprocessHasData(int socket)
{
    if (debugToolHandler)
        qDebug() << "subprocessHasData, state:" << state;
    process.notifier()->setEnabled(false);
    while (true) {
        switch (state) {
        case NonStarted:
            qDebug() << "IosToolHandler unexpected state in subprocessHasData: NonStarted";
            // pass
        case Starting:
        case StartedInferior:
            // read some data
        {
            if (iEnd + lookaheadSize > buffer.size()) {
                memmove(buffer.data(), buffer.data() + (iEnd - lookaheadSize), lookaheadSize);
                iBegin = lookaheadSize;
                iEnd = iBegin;
            } else {
                iBegin = iEnd;
            }
            currentData.clear();
            ssize_t reallyRead = recv(socket, buffer.data() + iBegin, lookaheadSize, 0);
            if (reallyRead == 0) { // eof
                stop();
                return;
            }
            if (reallyRead == -1) {
                if (errno == EAGAIN) { // read all so far
                    if (debugToolHandler)
                        qDebug() << "read all for now";
                    process.notifier()->setEnabled(true);
                    return;
                }
                if (errno == EINTR)
                    continue;
                qDebug() << "IosToolHandlerPrivate::subprocessHasData " << qt_error_string(errno);
                stop();
                return;
            }
            iEnd = iBegin + reallyRead;
            int xmlEnd = checkForXmlEnd();
            if (xmlEnd != -1) {
                state = XmlEndSeenNotProcessed;
                currentData = buffer.mid(iBegin, xmlEnd - iBegin);
            } else {
                currentData = buffer.mid(iBegin, reallyRead);
            }
            if (debugToolHandler)
                qDebug() << "subprocessHasData read " << currentData;
            outputParser.addData(currentData);
            processXml();
            break;
        }
        case XmlEndSeenNotProcessed:
            qDebug() << "IosToolHandler unexpected state in subprocessHasData: XmlEndSeenNotProcessed";
            // pass
        case XmlEndProcessed:
        {
            // check for sent fd
            if (!expectsFileDescriptor()) {
                stop();
                return;
            }
            int lenToRead = lookaheadSize;
            int spacerStart = iBegin + currentData.size();
            while (spacerStart < iEnd && buffer.at(spacerStart) != 'n')
                ++spacerStart;
            if (iEnd - (iBegin + currentData.size()) < lenToRead) {
                int lastXmlSize = currentData.size();
                if (iBegin > 0) {
                    memmove(buffer.data(), buffer.data() + iBegin, iEnd - iBegin);
                    iEnd -= iBegin;
                    iBegin = 0;
                    spacerStart -= iBegin;
                    currentData = buffer.mid(0, lastXmlSize); // remove this??
                }
                ssize_t toRead = lookaheadSize - (iEnd - spacerStart);
                ssize_t reallyRead = recv(socket, buffer.data() + iBegin, toRead, 0);
                if (reallyRead == 0) { // eof
                    stop();
                    return;
                }
                if (reallyRead == -1) {
                    if (errno == EAGAIN) { // read all so far
                        if (debugToolHandler)
                            qDebug() << "read all for now2";
                        process.notifier()->setEnabled(true);
                        return;
                    }
                    if (errno == EINTR)
                        continue;
                    if (debugToolHandler)
                        qDebug() << "IosToolHandlerPrivate::subprocessHasData " << qt_error_string(errno);
                    stop();
                    return;
                }
                iEnd += reallyRead;
                if (reallyRead != toRead)
                    continue;
                if (spacerStart < iEnd && buffer.at(spacerStart) != 'n') {
                    ++spacerStart;
                    while (spacerStart < iEnd && buffer.at(spacerStart) != 'n')
                        ++spacerStart;
                    continue;
                }
            }
            gdbSocket = recv_fd(socket);
            gotGdbserverSocket(bundlePath, deviceId, gdbSocket);
            stop();
            return;
        }
        case Stopped:
            return;
        }
    }
    process.notifier()->setEnabled(true);
}

// IosDeviceToolHandlerPrivate

IosDeviceToolHandlerPrivate::IosDeviceToolHandlerPrivate(IosToolHandler::DeviceType devType,
                                                         IosToolHandler *q)
    : IosToolHandlerPrivate(devType, q)
{ }

void IosDeviceToolHandlerPrivate::requestTransferApp(const QString &bundlePath,
                                                     const QString &deviceId, int timeout)
{
    this->bundlePath = bundlePath;
    this->deviceId = deviceId;
    QStringList args;
    args << QLatin1String("-device-id") << deviceId << QLatin1String("-bundle")
         << bundlePath << QLatin1String("-timeout") << QString::number(timeout)
         << QLatin1String("-deploy");
    start(IosToolHandler::iosDeviceToolPath(), args);
}

void IosDeviceToolHandlerPrivate::requestRunApp(const QString &bundlePath,
                                                const QStringList &extraArgs,
                                                IosToolHandler::RunKind runType,
                                                const QString &deviceId, int timeout)
{
    this->bundlePath = bundlePath;
    this->deviceId = deviceId;
    this->runKind = runType;
    QStringList args;
    args << QLatin1String("-device-id") << deviceId << QLatin1String("-bundle")
         << bundlePath << QLatin1String("-timeout") << QString::number(timeout);
    //args << QLatin1String("--deploy"); // to remove when the separate deploy step is functional
    switch (runType) {
    case IosToolHandler::NormalRun:
        args << QLatin1String("-run");
        break;
    case IosToolHandler::DebugRun:
        args << QLatin1String("-debug");
        break;
    }
    args << QLatin1String("-extra-args") << extraArgs;
    op = OpAppRun;
    start(IosToolHandler::iosDeviceToolPath(), args);
}

void IosDeviceToolHandlerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    this->deviceId = deviceId;
    QStringList args;
    args << QLatin1String("-device-id") << deviceId << QLatin1String("-device-info")
         << QLatin1String("-timeout") << QString::number(timeout);
    op = OpDeviceInfo;
    start(IosToolHandler::iosDeviceToolPath(), args);
}

bool IosDeviceToolHandlerPrivate::expectsFileDescriptor()
{
    return op == OpAppRun && runKind == IosToolHandler::DebugRun;
}

// IosSimulatorToolHandlerPrivate

IosSimulatorToolHandlerPrivate::IosSimulatorToolHandlerPrivate(IosToolHandler::DeviceType devType,
                                                         IosToolHandler *q)
    : IosToolHandlerPrivate(devType, q)
{ }

void IosSimulatorToolHandlerPrivate::requestTransferApp(const QString &bundlePath,
                                                        const QString &deviceId, int timeout)
{
    Q_UNUSED(timeout);
    this->bundlePath = bundlePath;
    this->deviceId = deviceId;
    emit didTransferApp(bundlePath, deviceId, IosToolHandler::Success);
}

void IosSimulatorToolHandlerPrivate::requestRunApp(const QString &bundlePath,
                                                   const QStringList &extraArgs,
                                                   IosToolHandler::RunKind runType,
                                                   const QString &deviceId, int timeout)
{
    Q_UNUSED(timeout);
    this->bundlePath = bundlePath;
    this->deviceId = deviceId;
    this->runKind = runType;
    QStringList args;

    args << QLatin1String("launch") << bundlePath;
    Utils::FileName devPath = IosConfigurations::instance().config().developerPath;
    if (!devPath.isEmpty())
        args << QLatin1String("--developer-path") << devPath.toString();
    addDeviceArguments(args);
    switch (runType) {
    case IosToolHandler::NormalRun:
        break;
    case IosToolHandler::DebugRun:
        args << QLatin1String("--wait-for-debugger");
        break;
    }
    args << QLatin1String("--args") << extraArgs;
    op = OpAppRun;
    start(IosToolHandler::iosSimulatorToolPath(), args);
}

void IosSimulatorToolHandlerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    Q_UNUSED(timeout);
    this->deviceId = deviceId;
    QStringList args;
    args << QLatin1String("showsdks");
    op = OpDeviceInfo;
    start(IosToolHandler::iosSimulatorToolPath(), args);
}

bool IosSimulatorToolHandlerPrivate::expectsFileDescriptor()
{
    return false;
}

void IosSimulatorToolHandlerPrivate::addDeviceArguments(QStringList &args) const
{
    switch (devType) {
    case IosToolHandler::IosDeviceType:
        qDebug() << "IosSimulatorToolHandlerPrivate has device type IosDeviceType";
        break;
    case IosToolHandler::IosSimulatedIphoneType:
        args << QLatin1String("--family") << QLatin1String("iphone");
        break;
    case IosToolHandler::IosSimulatedIpadType:
        args << QLatin1String("--family") << QLatin1String("ipad");
        break;
    case IosToolHandler::IosSimulatedIphoneRetina4InchType:
        args << QLatin1String("--family") << QLatin1String("iphone")
             << QLatin1String("--retina") << QLatin1String("--tall");
        break;
    case IosToolHandler::IosSimulatedIphoneRetina3_5InchType:
        args << QLatin1String("--family") << QLatin1String("iphone") << QLatin1String("--retina");
        break;
    case IosToolHandler::IosSimulatedIpadRetinaType:
        args << QLatin1String("--family") << QLatin1String("ipad") << QLatin1String("--retina");
        break;
    }
}

} // namespace Internal

QString IosToolHandler::iosDeviceToolPath()
{
    QString res = QCoreApplication::applicationDirPath() + QLatin1String("/iosTool");
    return res;
}

QString IosToolHandler::iosSimulatorToolPath()
{
    QString res = QCoreApplication::applicationDirPath() + QLatin1String("/iossim");
    return res;
}

IosToolHandler::IosToolHandler(DeviceType devType, QObject *parent) :
    QObject(parent)
{
    if (devType == IosDeviceType)
        d = new Internal::IosDeviceToolHandlerPrivate(devType, this);
    else
        d = new Internal::IosSimulatorToolHandlerPrivate(devType, this);
}

void IosToolHandler::stop()
{
    d->stop();
}

void IosToolHandler::requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                        int timeout)
{
    d->requestTransferApp(bundlePath, deviceId, timeout);
}

void IosToolHandler::requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                                   RunKind runType, const QString &deviceId, int timeout)
{
    d->requestRunApp(bundlePath, extraArgs, runType, deviceId, timeout);
}

void IosToolHandler::requestDeviceInfo(const QString &deviceId, int timeout)
{
    d->requestDeviceInfo(deviceId, timeout);
}

bool IosToolHandler::isRunning()
{
    return d->isRunning();
}

void IosToolHandler::subprocessError(QProcess::ProcessError error)
{
    d->subprocessError(error);
}

void IosToolHandler::subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    d->subprocessFinished(exitCode, exitStatus);
}

void IosToolHandler::subprocessHasData(int socket)
{
    d->subprocessHasData(socket);
}

} // namespace Ios

#include "iostoolhandler.moc"
