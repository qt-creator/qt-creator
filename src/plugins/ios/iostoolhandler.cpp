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
#include "simulatorcontrol.h"

#include "debugger/debuggerconstants.h"
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>
#include <utils/qtcfallthrough.h>
#include "utils/runextensions.h"
#include "utils/synchronousprocess.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QLoggingCategory>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopedArrayPointer>
#include <QSocketNotifier>
#include <QTemporaryFile>
#include <QTimer>
#include <QXmlStreamReader>

#include <signal.h>
#include <string.h>
#include <errno.h>

static Q_LOGGING_CATEGORY(toolHandlerLog, "qtc.ios.toolhandler")

namespace Ios {

namespace Internal {

using namespace std::placeholders;

// As per the currrent behavior, any absolute path given to simctl --stdout --stderr where the
// directory after the root also exists on the simulator's file system will map to
// simulator's file system i.e. simctl translates $TMPDIR/somwhere/out.txt to
// your_home_dir/Library/Developer/CoreSimulator/Devices/data/$TMP_DIR/somwhere/out.txt.
// Because /var also exists on simulator's file system.
// Though the log files located at CONSOLE_PATH_TEMPLATE are deleted on
// app exit any leftovers shall be removed on simulator restart.
static QString CONSOLE_PATH_TEMPLATE = QDir::homePath() +
        "/Library/Developer/CoreSimulator/Devices/%1/data/tmp/%2";

class LogTailFiles : public QObject
{
    Q_OBJECT
public:

    void exec(QFutureInterface<void> &fi, std::shared_ptr<QTemporaryFile> stdoutFile,
                    std::shared_ptr<QTemporaryFile> stderrFile)
    {
        if (fi.isCanceled())
            return;

        // The future is canceled when app on simulator is stoped.
        QEventLoop loop;
        QFutureWatcher<void> watcher;
        connect(&watcher, &QFutureWatcher<void>::canceled, [&](){
            loop.quit();
        });
        watcher.setFuture(fi.future());

        // Process to print the console output while app is running.
        auto logProcess = [this, fi](QProcess *tailProcess, std::shared_ptr<QTemporaryFile> file) {
            QObject::connect(tailProcess, &QProcess::readyReadStandardOutput, [=]() {
                if (!fi.isCanceled())
                    emit logMessage(QString::fromLocal8Bit(tailProcess->readAll()));
            });
            tailProcess->start(QStringLiteral("tail"), {"-f", file->fileName()});
        };

        auto processDeleter = [](QProcess *process) {
            if (process->state() != QProcess::NotRunning) {
                process->terminate();
                process->waitForFinished();
            }
            delete process;
        };

        std::unique_ptr<QProcess, void(*)(QProcess *)> tailStdout(new QProcess, processDeleter);
        if (stdoutFile)
            logProcess(tailStdout.get(), stdoutFile);

        std::unique_ptr<QProcess, void(*)(QProcess *)> tailStderr(new QProcess, processDeleter);
        if (stderrFile)
            logProcess(tailStderr.get(), stderrFile);

        // Blocks untill tool is deleted or toolexited is called.
        loop.exec();
    }

signals:
    void logMessage(QString message);
};

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
    explicit IosToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
    virtual ~IosToolHandlerPrivate();
    virtual void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                                    int timeout = 1000) = 0;
    virtual void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000) = 0;
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000) = 0;
    virtual bool isRunning() const = 0;
    virtual void stop(int errorCode) = 0;

    // signals
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           int maxProgress, const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        IosToolHandler::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     IosToolHandler::OpStatus status);
    void gotServerPorts(const QString &bundlePath, const QString &deviceId, Utils::Port gdbPort,
                        Utils::Port qmlPort);
    void gotInferiorPid(const QString &bundlePath, const QString &deviceId, qint64 pid);
    void deviceInfo(const QString &deviceId, const IosToolHandler::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void toolExited(int code);

protected:
    IosToolHandler *q;
    QString m_deviceId;
    QString m_bundlePath;
    IosToolHandler::RunKind m_runKind;
    IosDeviceType m_devType;
};

class IosDeviceToolHandlerPrivate : public IosToolHandlerPrivate
{
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
public:
    explicit IosDeviceToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
    ~IosDeviceToolHandlerPrivate();

// IosToolHandlerPrivate overrides
public:
    void requestTransferApp(const QString &bundlePath, const QString &deviceId,
                            int timeout = 1000) override;
    void requestRunApp(const QString &bundlePath, const QStringList &extraArgs,
                       IosToolHandler::RunKind runKind,
                       const QString &deviceId, int timeout = 1000) override;
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000) override;
    bool isRunning() const override;
    void start(const QString &exe, const QStringList &args);
    void stop(int errorCode) override;

private:
    void subprocessError(QProcess::ProcessError error);
    void subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void subprocessHasData();
    void processXml();
    void killProcess();

    QTimer killTimer;
    std::shared_ptr<QProcess> process;
    State state = NonStarted;
    Op op = OpNone;
    QXmlStreamReader outputParser;
    QList<ParserState> stack;
};

/****************************************************************************
 * Flow to install an app on simulator:-
 *      +------------------+
 *      |   Transfer App   |
 *      +--------+---------+
 *               |
 *               v
 *     +---------+----------+             +--------------------------------+
 *     |  SimulatorRunning  +---No------> +SimulatorControl::startSimulator|
 *     +---------+----------+             +--------+-----------------------+
 *              Yes                                |
 *               |                                 |
 *               v                                 |
 * +---------+--------------------+                |
 * | SimulatorControl::installApp | <--------------+
 * +------------------------------+
 *
 *
 *
 * Flow to launch an app on Simulator:-
 *            +---------+
 *            | Run App |
 *            +----+----+
 *                 |
 *                 v
 *       +-------------------+             +----------------------------- - --+
 *       | SimulatorRunning? +---NO------> + SimulatorControl::startSimulator |
 *       +--------+----------+             +----------------+-----------------+
 *                YES                                       |
 *                 |                                        |
 *                 v                                        |
 * +---------+------------------------------+               |
 * | SimulatorControl::launchAppOnSimulator | <-------------+
 * +----------------------------------------+
 *
 ***************************************************************************/
class IosSimulatorToolHandlerPrivate : public IosToolHandlerPrivate
{
public:
    explicit IosSimulatorToolHandlerPrivate(const IosDeviceType &devType, IosToolHandler *q);
    ~IosSimulatorToolHandlerPrivate();

// IosToolHandlerPrivate overrides
public:
    void requestTransferApp(const QString &appBundlePath, const QString &deviceIdentifier,
                            int timeout = 1000) override;
    void requestRunApp(const QString &appBundlePath, const QStringList &extraArgs,
                       IosToolHandler::RunKind runKind,
                       const QString &deviceIdentifier, int timeout = 1000) override;
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000) override;
    bool isRunning() const override;
    void stop(int errorCode) override;

private:
    void installAppOnSimulator();
    void launchAppOnSimulator(const QStringList &extraArgs);
    bool isResponseValid(const SimulatorControl::ResponseData &responseData);

private:
    qint64 m_pid = -1;
    SimulatorControl *simCtl;
    LogTailFiles outputLogger;
    QList<QFuture<void>> futureList;
};

IosToolHandlerPrivate::IosToolHandlerPrivate(const IosDeviceType &devType,
                                             Ios::IosToolHandler *q) :
    q(q),
    m_devType(devType)
{
}

IosToolHandlerPrivate::~IosToolHandlerPrivate()
{
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

void IosToolHandlerPrivate::gotServerPorts(const QString &bundlePath, const QString &deviceId,
                                           Utils::Port gdbPort, Utils::Port qmlPort)
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

void IosDeviceToolHandlerPrivate::subprocessError(QProcess::ProcessError error)
{
    if (state != Stopped)
        errorMsg(IosToolHandler::tr("iOS tool error %1").arg(error));
    stop(-1);
    if (error == QProcess::FailedToStart) {
        qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
        emit q->finished(q);
    }
}

void IosDeviceToolHandlerPrivate::subprocessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    stop((exitStatus == QProcess::NormalExit) ? exitCode : -1 );
    qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
    killTimer.stop();
    emit q->finished(q);
}

void IosDeviceToolHandlerPrivate::processXml()
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
                QChar c[1] = {QChar::fromLatin1(static_cast<char>(attributes.value(QLatin1String("code")).toString().toInt()))};
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
                didStartApp(m_bundlePath, m_deviceId, status);
            } else if (elName == QLatin1String("app_transfer")) {
                stack.append(ParserState(ParserState::AppTransfer));
                QXmlStreamAttributes attributes = outputParser.attributes();
                QStringRef statusStr = attributes.value(QLatin1String("status"));
                Ios::IosToolHandler::OpStatus status = Ios::IosToolHandler::Unknown;
                if (statusStr.compare(QLatin1String("success"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Success;
                else if (statusStr.compare(QLatin1String("failure"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Failure;
                emit didTransferApp(m_bundlePath, m_deviceId, status);
            } else if (elName == QLatin1String("device_info") || elName == QLatin1String("deviceinfo")) {
                stack.append(ParserState(ParserState::DeviceInfo));
            } else if (elName == QLatin1String("inferior_pid")) {
                stack.append(ParserState(ParserState::InferiorPid));
            } else if (elName == QLatin1String("server_ports")) {
                stack.append(ParserState(ParserState::ServerPorts));
                QXmlStreamAttributes attributes = outputParser.attributes();
                Utils::Port gdbServerPort(
                            attributes.value(QLatin1String("gdb_server")).toString().toInt());
                Utils::Port qmlServerPort(
                            attributes.value(QLatin1String("qml_server")).toString().toInt());
                gotServerPorts(m_bundlePath, m_deviceId, gdbServerPort, qmlServerPort);
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
                if (m_deviceId.isEmpty())
                    m_deviceId = p.chars;
                else
                    QTC_CHECK(m_deviceId.compare(p.chars, Qt::CaseInsensitive) == 0);
                break;
            case ParserState::Key:
                stack.last().key = p.chars;
                break;
            case ParserState::Value:
                stack.last().value = p.chars;
                break;
            case ParserState::Status:
                isTransferringApp(m_bundlePath, m_deviceId, p.progress, p.maxProgress, p.chars);
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
                deviceInfo(m_deviceId, p.info);
                break;
            case ParserState::Exit:
                break;
            case ParserState::InferiorPid:
                gotInferiorPid(m_bundlePath, m_deviceId, p.chars.toLongLong());
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

void IosDeviceToolHandlerPrivate::killProcess()
{
    if (isRunning())
        process->kill();
}

void IosDeviceToolHandlerPrivate::subprocessHasData()
{
    qCDebug(toolHandlerLog) << "subprocessHasData, state:" << state;
    while (true) {
        switch (state) {
        case NonStarted:
            qCWarning(toolHandlerLog) << "IosToolHandler unexpected state in subprocessHasData: NonStarted";
            Q_FALLTHROUGH();
        case Starting:
        case StartedInferior:
            // read some data
        {
            char buf[200];
            while (isRunning()) {
                qint64 rRead = process->read(buf, sizeof(buf));
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
            break;
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
{
    killTimer.setSingleShot(true);

    auto deleter = [](QProcess *p) {
        if (p->state() != QProcess::NotRunning) {
            p->kill();
            if (!p->waitForFinished(2000))
                p->terminate();
        }
        delete p;
    };
    process = std::shared_ptr<QProcess>(new QProcess, deleter);

    // Prepare & set process Environment.
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
    process->setProcessEnvironment(env);

    QObject::connect(process.get(), &QProcess::readyReadStandardOutput,
                     std::bind(&IosDeviceToolHandlerPrivate::subprocessHasData,this));

    QObject::connect(process.get(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     std::bind(&IosDeviceToolHandlerPrivate::subprocessFinished,this, _1,_2));

    QObject::connect(process.get(), &QProcess::errorOccurred,
                     std::bind(&IosDeviceToolHandlerPrivate::subprocessError, this, _1));

    QObject::connect(&killTimer, &QTimer::timeout, std::bind(&IosDeviceToolHandlerPrivate::killProcess, this));
}

IosDeviceToolHandlerPrivate::~IosDeviceToolHandlerPrivate()
{
    if (isRunning()) {
        // Disconnect the signals to avoid notifications while destructing.
        // QTCREATORBUG-18147
        process->disconnect();
        // Quit ios-tool gracefully before kill is executed.
        process->write("k\n\r");
        process->closeWriteChannel();
        // Give some time to ios-tool to finish.
        process->waitForFinished(2000);
    }
}

void IosDeviceToolHandlerPrivate::requestTransferApp(const QString &bundlePath,
                                                     const QString &deviceId, int timeout)
{
    m_bundlePath = bundlePath;
    m_deviceId = deviceId;
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
    m_bundlePath = bundlePath;
    m_deviceId = deviceId;
    m_runKind = runType;
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
    m_deviceId = deviceId;
    QStringList args;
    args << QLatin1String("--id") << m_deviceId << QLatin1String("--device-info")
         << QLatin1String("--timeout") << QString::number(timeout);
    op = OpDeviceInfo;
    start(IosToolHandler::iosDeviceToolPath(), args);
}

bool IosDeviceToolHandlerPrivate::isRunning() const
{
    return process && (process->state() != QProcess::NotRunning);
}

void IosDeviceToolHandlerPrivate::start(const QString &exe, const QStringList &args)
{
    Q_ASSERT(process);
    QTC_CHECK(state == NonStarted);
    state = Starting;
    qCDebug(toolHandlerLog) << "running " << exe << args;
    process->start(exe, args);
    state = StartedInferior;
}


void IosDeviceToolHandlerPrivate::stop(int errorCode)
{
    qCDebug(toolHandlerLog) << "IosToolHandlerPrivate::stop";
    State oldState = state;
    state = Stopped;
    switch (oldState) {
    case NonStarted:
        qCWarning(toolHandlerLog) << "IosToolHandler::stop() when state was NonStarted";
        Q_FALLTHROUGH();
    case Starting:
        switch (op){
        case OpNone:
            qCWarning(toolHandlerLog) << "IosToolHandler::stop() when op was OpNone";
            break;
        case OpAppTransfer:
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
            break;
        case OpAppRun:
            didStartApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
            break;
        case OpDeviceInfo:
            break;
        }
        Q_FALLTHROUGH();
    case StartedInferior:
    case XmlEndProcessed:
        toolExited(errorCode);
        break;
    case Stopped:
        return;
    }
    if (isRunning()) {
        process->write("k\n\r");
        process->closeWriteChannel();
        killTimer.start(1500);
    }
}


// IosSimulatorToolHandlerPrivate

IosSimulatorToolHandlerPrivate::IosSimulatorToolHandlerPrivate(const IosDeviceType &devType,
                                                         IosToolHandler *q)
    : IosToolHandlerPrivate(devType, q),
      simCtl(new SimulatorControl)
{
    QObject::connect(&outputLogger, &LogTailFiles::logMessage,
                     std::bind(&IosToolHandlerPrivate::appOutput, this, _1));
}

IosSimulatorToolHandlerPrivate::~IosSimulatorToolHandlerPrivate()
{
    foreach (auto f, futureList) {
        if (!f.isFinished())
            f.cancel();
    }
    delete simCtl;
}
void IosSimulatorToolHandlerPrivate::requestTransferApp(const QString &appBundlePath,
                                                        const QString &deviceIdentifier, int timeout)
{
    Q_UNUSED(timeout);
    m_bundlePath = appBundlePath;
    m_deviceId = deviceIdentifier;
    isTransferringApp(m_bundlePath, m_deviceId, 0, 100, "");

    auto onSimulatorStart = [this](const SimulatorControl::ResponseData &response) {
        if (!isResponseValid(response))
            return;

        if (response.success) {
            installAppOnSimulator();
        } else {
            errorMsg(IosToolHandler::tr("Application install on simulator failed. Simulator not running."));
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
            emit q->finished(q);
        }
    };

    if (SimulatorControl::isSimulatorRunning(m_deviceId))
        installAppOnSimulator();
    else
        futureList << Utils::onResultReady(simCtl->startSimulator(m_deviceId), onSimulatorStart);
}

void IosSimulatorToolHandlerPrivate::requestRunApp(const QString &appBundlePath,
                                                   const QStringList &extraArgs,
                                                   IosToolHandler::RunKind runType,
                                                   const QString &deviceIdentifier, int timeout)
{
    Q_UNUSED(timeout);
    Q_UNUSED(deviceIdentifier);
    m_bundlePath = appBundlePath;
    m_deviceId = m_devType.identifier;
    m_runKind = runType;

    Utils::FileName appBundle = Utils::FileName::fromString(m_bundlePath);
    if (!appBundle.exists()) {
        errorMsg(IosToolHandler::tr("Application launch on simulator failed. Invalid bundle path %1")
                 .arg(m_bundlePath));
        didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
        return;
    }

    auto onSimulatorStart = [this, extraArgs] (const SimulatorControl::ResponseData &response) {
        if (!isResponseValid(response))
            return;
        if (response.success) {
            launchAppOnSimulator(extraArgs);
        } else {
            errorMsg(IosToolHandler::tr("Application launch on simulator failed. Simulator not running."));
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
        }
    };

    if (SimulatorControl::isSimulatorRunning(m_deviceId))
        launchAppOnSimulator(extraArgs);
    else
        futureList << Utils::onResultReady(simCtl->startSimulator(m_deviceId), onSimulatorStart);
}

void IosSimulatorToolHandlerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    Q_UNUSED(timeout);
    Q_UNUSED(deviceId);
}

bool IosSimulatorToolHandlerPrivate::isRunning() const
{
#ifdef Q_OS_UNIX
    return m_pid > 0 && (kill(m_pid, 0) == 0);
#else
    return false;
#endif
}

void IosSimulatorToolHandlerPrivate::stop(int errorCode)
{
#ifdef Q_OS_UNIX
    if (m_pid > 0)
        kill(m_pid, SIGKILL);
#endif
    m_pid = -1;
    foreach (auto f, futureList) {
        if (!f.isFinished())
            f.cancel();
    }

    toolExited(errorCode);
    q->finished(q);
}

void IosSimulatorToolHandlerPrivate::installAppOnSimulator()
{
    auto onResponseAppInstall = [this](const SimulatorControl::ResponseData &response) {
        if (!isResponseValid(response))
            return;

        if (response.success) {
            isTransferringApp(m_bundlePath, m_deviceId, 100, 100, "");
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Success);
        } else {
            errorMsg(IosToolHandler::tr("Application install on simulator failed. %1")
                     .arg(QString::fromLocal8Bit(response.commandOutput)));
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
        }
        emit q->finished(q);
    };

    isTransferringApp(m_bundlePath, m_deviceId, 20, 100, "");
    auto installFuture = simCtl->installApp(m_deviceId, Utils::FileName::fromString(m_bundlePath));
    futureList << Utils::onResultReady(installFuture, onResponseAppInstall);
}

void IosSimulatorToolHandlerPrivate::launchAppOnSimulator(const QStringList &extraArgs)
{
    const Utils::FileName appBundle = Utils::FileName::fromString(m_bundlePath);
    const QString bundleId = SimulatorControl::bundleIdentifier(appBundle);
    const bool debugRun = m_runKind == IosToolHandler::DebugRun;
    bool captureConsole = IosConfigurations::xcodeVersion() >= QVersionNumber(8);
    std::shared_ptr<QTemporaryFile> stdoutFile;
    std::shared_ptr<QTemporaryFile> stderrFile;

    if (captureConsole) {
        const QString fileTemplate = CONSOLE_PATH_TEMPLATE.arg(m_deviceId).arg(bundleId);
        stdoutFile.reset(new QTemporaryFile(fileTemplate + ".stdout"));
        stderrFile.reset(new QTemporaryFile(fileTemplate + ".stderr"));

        captureConsole = stdoutFile->open() && stderrFile->open();
        if (!captureConsole)
            errorMsg(IosToolHandler::tr("Cannot capture console output from %1. "
                                        "Error redirecting output to %2.*")
                     .arg(bundleId).arg(fileTemplate));
    } else {
        errorMsg(IosToolHandler::tr("Cannot capture console output from %1. "
                    "Install Xcode 8 or later.").arg(bundleId));
    }

    auto monitorPid = [this](QFutureInterface<void> &fi, qint64 pid) {
#ifdef Q_OS_UNIX
        do {
            // Poll every 1 sec to check whether the app is running.
            QThread::msleep(1000);
        } while (!fi.isCanceled() && kill(pid, 0) == 0);
#else
    Q_UNUSED(pid);
#endif
        // Future is cancelled if the app is stopped from the qt creator.
        if (!fi.isCanceled())
            stop(0);
    };

    auto onResponseAppLaunch = [=](const SimulatorControl::ResponseData &response) {
        if (!isResponseValid(response))
            return;
        if (response.success) {
            m_pid = response.pID;
            gotInferiorPid(m_bundlePath, m_deviceId, response.pID);
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Success);
            // Start monitoring app's life signs.
            futureList << Utils::runAsync(monitorPid, response.pID);
            if (captureConsole)
                futureList << Utils::runAsync(&LogTailFiles::exec, &outputLogger, stdoutFile,
                                              stderrFile);
        } else {
            m_pid = -1;
            errorMsg(IosToolHandler::tr("Application launch on simulator failed. %1")
                     .arg(QString::fromLocal8Bit(response.commandOutput)));
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
            stop(-1);
            q->finished(q);
        }
    };

    futureList << Utils::onResultReady(
                      simCtl->launchApp(m_deviceId, bundleId, debugRun, extraArgs,
                                        captureConsole ? stdoutFile->fileName() : QString(),
                                        captureConsole ? stderrFile->fileName() : QString()),
                      onResponseAppLaunch);
}

bool IosSimulatorToolHandlerPrivate::isResponseValid(const SimulatorControl::ResponseData &responseData)
{
    if (responseData.simUdid.compare(m_deviceId) != 0) {
        errorMsg(IosToolHandler::tr("Invalid simulator response. Device Id mismatch. "
                                    "Device Id = %1 Response Id = %2")
                 .arg(responseData.simUdid)
                 .arg(m_deviceId));
        emit q->finished(q);
        return false;
    }
    return true;
}

} // namespace Internal

QString IosToolHandler::iosDeviceToolPath()
{
    QString res = Core::ICore::libexecPath() + QLatin1String("/ios/iostool");
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

bool IosToolHandler::isRunning() const
{
    return d->isRunning();
}

} // namespace Ios

#include "iostoolhandler.moc"
