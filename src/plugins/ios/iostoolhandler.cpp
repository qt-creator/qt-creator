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

#include "iostoolhandler.h"
#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iossimulator.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QList>
#include <QLoggingCategory>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopedArrayPointer>
#include <QSocketNotifier>
#include <QTimer>
#include <QXmlStreamReader>

#include <string.h>
#include <errno.h>

static Q_LOGGING_CATEGORY(toolHandlerLog, "qtc.ios.toolhandler")

namespace Ios {

namespace Internal {

struct ParserState {
    enum Kind {
        Msg,
        DeviceId,
        Key,
        Value,
        QueryResult,
        AppOutput,
        ControlChar,
        AppStarted,
        InferiorPid,
        ServerPorts,
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
    int gdbPort, qmlPort;
    bool collectChars() {
        switch (kind) {
        case Msg:
        case DeviceId:
        case Key:
        case Value:
        case Status:
        case InferiorPid:
        case AppOutput:
            return true;
        case ServerPorts:
        case QueryResult:
        case ControlChar:
        case AppStarted:
        case AppTransfer:
        case Item:
        case DeviceInfo:
        case Exit:
            break;
        }
        return false;
    }

    ParserState(Kind kind) :
        kind(kind), gdbPort(0), qmlPort(0) { }
};

class IosToolHandlerPrivate
{
public:
    enum State {
        NonStarted,
        Starting,
        StartedInferior,
        XmlEndProcessed,
        Stopped
    };
    enum Op {
        OpNone,
        OpAppTransfer,
        OpDeviceInfo,
        OpAppRun
    };

    explicit IosToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
    virtual ~IosToolHandlerPrivate() {}
    virtual void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                    int timeout = 1000) = 0;
    virtual void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000) = 0;
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000) = 0;
    bool isRunning();
    void start(const QString &exe, const QStringList &args);
    void stop(int errorCode);

    // signals
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           int maxProgress, const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        IosToolHandler::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     IosToolHandler::OpStatus status);
    void gotServerPorts(const QString &bundlePath, const QString &deviceId, int gdbPort,
                        int qmlPort);
    void gotInferiorPid(const QString &bundlePath, const QString &deviceId, qint64 pid);
    void deviceInfo(const QString &deviceId, const IosToolHandler::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void toolExited(int code);
    // slots
    void subprocessError(QProcess::ProcessError error);
    void subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void subprocessHasData();
    void killProcess();
    virtual bool expectsFileDescriptor() = 0;
protected:
    void processXml();

    IosToolHandler *q;
    QProcess process;
    QTimer killTimer;
    QXmlStreamReader outputParser;
    QString deviceId;
    QString bundlePath;
    IosToolHandler::RunKind runKind;
    State state;
    Op op;
    IosDeviceType devType;
    static const int lookaheadSize = 67;
    int iBegin, iEnd, gdbSocket;
    QList<ParserState> stack;
};

class IosDeviceToolHandlerPrivate : public IosToolHandlerPrivate
{
public:
    explicit IosDeviceToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
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
    explicit IosSimulatorToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
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

IosToolHandlerPrivate::IosToolHandlerPrivate(const IosDeviceType &devType,
                                             Ios::IosToolHandler *q) :
    q(q), state(NonStarted), devType(devType), iBegin(0), iEnd(0),
    gdbSocket(-1)
{
    killTimer.setSingleShot(true);
    QProcessEnvironment env(QProcessEnvironment::systemEnvironment());
    foreach (const QString &k, env.keys())
        if (k.startsWith(QLatin1String("DYLD_")))
            env.remove(k);
    QStringList frameworkPaths;
    Utils::FileName xcPath = IosConfigurations::developerPath();
    QString privateFPath = xcPath.appendPath(QLatin1String("Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks")).toFileInfo().canonicalFilePath();
    if (!privateFPath.isEmpty())
        frameworkPaths << privateFPath;
    QString otherFPath = xcPath.appendPath(QLatin1String("../OtherFrameworks")).toFileInfo().canonicalFilePath();
    if (!otherFPath.isEmpty())
        frameworkPaths << otherFPath;
    QString sharedFPath = xcPath.appendPath(QLatin1String("../SharedFrameworks")).toFileInfo().canonicalFilePath();
    if (!sharedFPath.isEmpty())
        frameworkPaths << sharedFPath;
    frameworkPaths << QLatin1String("/System/Library/Frameworks")
                   << QLatin1String("/System/Library/PrivateFrameworks");
    env.insert(QLatin1String("DYLD_FALLBACK_FRAMEWORK_PATH"), frameworkPaths.join(QLatin1Char(':')));
    qCDebug(toolHandlerLog) << "IosToolHandler runEnv:" << env.toStringList();
    process.setProcessEnvironment(env);
    QObject::connect(&process, SIGNAL(readyReadStandardOutput()), q, SLOT(subprocessHasData()));
    QObject::connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)),
            q, SLOT(subprocessFinished(int,QProcess::ExitStatus)));
    QObject::connect(&process, SIGNAL(error(QProcess::ProcessError)),
            q, SLOT(subprocessError(QProcess::ProcessError)));
    QObject::connect(&killTimer, SIGNAL(timeout()),
            q, SLOT(killProcess()));
}

bool IosToolHandlerPrivate::isRunning()
{
    return process.state() != QProcess::NotRunning;
}

void IosToolHandlerPrivate::start(const QString &exe, const QStringList &args)
{
    QTC_CHECK(state == NonStarted);
    state = Starting;
    qCDebug(toolHandlerLog) << "running " << exe << args;
    process.start(exe, args);
    state = StartedInferior;
}

void IosToolHandlerPrivate::stop(int errorCode)
{
    qCDebug(toolHandlerLog) << "IosToolHandlerPrivate::stop";
    State oldState = state;
    state = Stopped;
    switch (oldState) {
    case NonStarted:
        qCWarning(toolHandlerLog) << "IosToolHandler::stop() when state was NonStarted";
        // pass
    case Starting:
        switch (op){
        case OpNone:
            qCWarning(toolHandlerLog) << "IosToolHandler::stop() when op was OpNone";
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
    case XmlEndProcessed:
        toolExited(errorCode);
        break;
    case Stopped:
        return;
    }
    if (process.state() != QProcess::NotRunning) {
        process.write("k\n\r");
        process.closeWriteChannel();
        killTimer.start(1500);
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

void IosToolHandlerPrivate::gotServerPorts(const QString &bundlePath,
                                           const QString &deviceId, int gdbPort, int qmlPort)
{
    emit q->gotServerPorts(q, bundlePath, deviceId, gdbPort, qmlPort);
}

void IosToolHandlerPrivate::gotInferiorPid(const QString &bundlePath, const QString &deviceId,
                                           qint64 pid)
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
    if (state != Stopped)
        errorMsg(IosToolHandler::tr("iOS tool Error %1").arg(error));
    stop(-1);
    if (error == QProcess::FailedToStart) {
        qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
        emit q->finished(q);
    }
}

void IosToolHandlerPrivate::subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    stop((exitStatus == QProcess::NormalExit) ? exitCode : -1 );
    qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
    killTimer.stop();
    emit q->finished(q);
}

void IosToolHandlerPrivate::processXml()
{
    while (!outputParser.atEnd()) {
        QXmlStreamReader::TokenType tt = outputParser.readNext();
        //qCDebug(toolHandlerLog) << "processXml, tt=" << tt;
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
            } else if (elName == QLatin1String("control_char")) {
                QXmlStreamAttributes attributes = outputParser.attributes();
                QChar c[1] = { QChar::fromLatin1(static_cast<char>(attributes.value(QLatin1String("code")).toString().toInt())) };
                if (stack.size() > 0 && stack.last().collectChars())
                    stack.last().chars.append(c[0]);
                stack.append(ParserState(ParserState::ControlChar));
                break;
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
            } else if (elName == QLatin1String("device_info") || elName == QLatin1String("deviceinfo")) {
                stack.append(ParserState(ParserState::DeviceInfo));
            } else if (elName == QLatin1String("inferior_pid")) {
                stack.append(ParserState(ParserState::InferiorPid));
            } else if (elName == QLatin1String("server_ports")) {
                stack.append(ParserState(ParserState::ServerPorts));
                QXmlStreamAttributes attributes = outputParser.attributes();
                int gdbServerPort = attributes.value(QLatin1String("gdb_server")).toString().toInt();
                int qmlServerPort = attributes.value(QLatin1String("qml_server")).toString().toInt();
                gotServerPorts(bundlePath, deviceId, gdbServerPort, qmlServerPort);
            } else {
                qCWarning(toolHandlerLog) << "unexpected element " << elName;
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
                stop(0);
                return;
            case ParserState::AppOutput:
                appOutput(p.chars);
                break;
            case ParserState::ControlChar:
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
                gotInferiorPid(bundlePath, deviceId, p.chars.toLongLong());
                break;
            case ParserState::ServerPorts:
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
            if (stack.last().collectChars())
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
        qCWarning(toolHandlerLog) << "error parsing iosTool output:" << outputParser.errorString();
        stop(-1);
    }
}

void IosToolHandlerPrivate::subprocessHasData()
{
    qCDebug(toolHandlerLog) << "subprocessHasData, state:" << state;
    while (true) {
        switch (state) {
        case NonStarted:
            qCWarning(toolHandlerLog) << "IosToolHandler unexpected state in subprocessHasData: NonStarted";
            // pass
        case Starting:
        case StartedInferior:
            // read some data
        {
            char buf[200];
            while (true) {
                qint64 rRead = process.read(buf, sizeof(buf));
                if (rRead == -1) {
                    stop(-1);
                    return;
                }
                if (rRead == 0)
                    return;
                qCDebug(toolHandlerLog) << "subprocessHasData read " << QByteArray(buf, rRead);
                outputParser.addData(QByteArray(buf, rRead));
                processXml();
            }
        }
        case XmlEndProcessed:
            stop(0);
            return;
        case Stopped:
            return;
        }
    }
}

// IosDeviceToolHandlerPrivate

IosDeviceToolHandlerPrivate::IosDeviceToolHandlerPrivate(const IosDeviceType &devType,
                                                         IosToolHandler *q)
    : IosToolHandlerPrivate(devType, q)
{ }

void IosDeviceToolHandlerPrivate::requestTransferApp(const QString &bundlePath,
                                                     const QString &deviceId, int timeout)
{
    this->bundlePath = bundlePath;
    this->deviceId = deviceId;
    QStringList args;
    args << QLatin1String("--id") << deviceId << QLatin1String("--bundle")
         << bundlePath << QLatin1String("--timeout") << QString::number(timeout)
         << QLatin1String("--install");
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
    args << QLatin1String("--id") << deviceId << QLatin1String("--bundle")
         << bundlePath << QLatin1String("--timeout") << QString::number(timeout);
    switch (runType) {
    case IosToolHandler::NormalRun:
        args << QLatin1String("--run");
        break;
    case IosToolHandler::DebugRun:
        args << QLatin1String("--debug");
        break;
    }
    args << QLatin1String("--args") << extraArgs;
    op = OpAppRun;
    start(IosToolHandler::iosDeviceToolPath(), args);
}

void IosDeviceToolHandlerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    this->deviceId = deviceId;
    QStringList args;
    args << QLatin1String("--id") << deviceId << QLatin1String("--device-info")
         << QLatin1String("--timeout") << QString::number(timeout);
    op = OpDeviceInfo;
    start(IosToolHandler::iosDeviceToolPath(), args);
}

bool IosDeviceToolHandlerPrivate::expectsFileDescriptor()
{
    return op == OpAppRun && runKind == IosToolHandler::DebugRun;
}

// IosSimulatorToolHandlerPrivate

IosSimulatorToolHandlerPrivate::IosSimulatorToolHandlerPrivate(const IosDeviceType &devType,
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
    Utils::FileName devPath = IosConfigurations::developerPath();
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
    args << QLatin1String("showdevicetypes");
    op = OpDeviceInfo;
    start(IosToolHandler::iosSimulatorToolPath(), args);
}

bool IosSimulatorToolHandlerPrivate::expectsFileDescriptor()
{
    return false;
}

void IosSimulatorToolHandlerPrivate::addDeviceArguments(QStringList &args) const
{
    if (devType.type != IosDeviceType::SimulatedDevice) {
        qCWarning(toolHandlerLog) << "IosSimulatorToolHandlerPrivate device type is not SimulatedDevice";
        return;
    }
    args << QLatin1String("--devicetypeid") << devType.identifier;
}

void IosToolHandlerPrivate::killProcess()
{
    if (process.state() != QProcess::NotRunning)
        process.kill();
}

} // namespace Internal

QString IosToolHandler::iosDeviceToolPath()
{
    QString res = Core::ICore::libexecPath() + QLatin1String("/ios/iostool");
    return res;
}

QString IosToolHandler::iosSimulatorToolPath()
{
    Utils::FileName devPath = Internal::IosConfigurations::developerPath();
    bool version182 = devPath.appendPath(QLatin1String(
        "Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/iPhoneSimulatorRemoteClient.framework"))
            .exists();
    QString res = Core::ICore::libexecPath() + QLatin1String("/ios/iossim");
    if (version182)
        res = res.append(QLatin1String("_1_8_2"));
    return res;
}

IosToolHandler::IosToolHandler(const Internal::IosDeviceType &devType, QObject *parent) :
    QObject(parent)
{
    if (devType.type == Internal::IosDeviceType::IosDevice)
        d = new Internal::IosDeviceToolHandlerPrivate(devType, this);
    else
        d = new Internal::IosSimulatorToolHandlerPrivate(devType, this);
}

IosToolHandler::~IosToolHandler()
{
    delete d;
}

void IosToolHandler::stop()
{
    d->stop(-1);
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

void IosToolHandler::subprocessHasData()
{
    d->subprocessHasData();
}

void IosToolHandler::killProcess()
{
    d->killProcess();
}

} // namespace Ios
