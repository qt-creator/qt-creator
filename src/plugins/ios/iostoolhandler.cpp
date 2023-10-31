// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iostoolhandler.h"

#include "iosconfigurations.h"
#include "iossimulator.h"
#include "iostr.h"
#include "simulatorcontrol.h"

#include <coreplugin/icore.h>

#include <debugger/debuggerconstants.h>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QLoggingCategory>
#include <QTemporaryFile>
#include <QXmlStreamReader>

#include <signal.h>
#include <string.h>
#include <errno.h>

static Q_LOGGING_CATEGORY(toolHandlerLog, "qtc.ios.toolhandler", QtWarningMsg)

using namespace Utils;

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
    void exec(QPromise<void> &promise, std::shared_ptr<QTemporaryFile> stdoutFile,
              std::shared_ptr<QTemporaryFile> stderrFile)
    {
        if (promise.isCanceled())
            return;

        // The future is canceled when app on simulator is stoped.
        QEventLoop loop;
        QFutureWatcher<void> watcher;
        connect(&watcher, &QFutureWatcher<void>::canceled, &loop, [&] { loop.quit(); });
        watcher.setFuture(promise.future());

        // Process to print the console output while app is running.
        auto logProcess = [&](Process *tailProcess, std::shared_ptr<QTemporaryFile> file) {
            QObject::connect(tailProcess, &Process::readyReadStandardOutput, &loop, [&, tailProcess] {
                if (!promise.isCanceled())
                    emit logMessage(QString::fromLocal8Bit(tailProcess->readAllRawStandardOutput()));
            });
            tailProcess->setCommand({FilePath::fromString("tail"), {"-f", file->fileName()}});
            tailProcess->start();
        };

        std::unique_ptr<Process> tailStdout(new Process);
        if (stdoutFile)
            logProcess(tailStdout.get(), stdoutFile);

        std::unique_ptr<Process> tailStderr(new Process);
        if (stderrFile)
            logProcess(tailStderr.get(), stderrFile);

        // Blocks untill tool is deleted or toolexited is called.
        loop.exec();
    }

signals:
    void logMessage(const QString &message);
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
    int progress = 0, maxProgress = 0;
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
    virtual void requestTransferApp(const FilePath &bundlePath, const QString &deviceId,
                                    int timeout = 1000) = 0;
    virtual void requestRunApp(const FilePath &bundlePath, const QStringList &extraArgs,
                               IosToolHandler::RunKind runKind,
                               const QString &deviceId, int timeout = 1000) = 0;
    virtual void requestDeviceInfo(const QString &deviceId, int timeout = 1000) = 0;
    virtual bool isRunning() const = 0;
    virtual void stop(int errorCode) = 0;

    // signals
    void isTransferringApp(const FilePath &bundlePath, const QString &deviceId, int progress,
                           int maxProgress, const QString &info);
    void didTransferApp(const FilePath &bundlePath, const QString &deviceId,
                        IosToolHandler::OpStatus status);
    void didStartApp(const FilePath &bundlePath, const QString &deviceId,
                     IosToolHandler::OpStatus status);
    void gotServerPorts(const FilePath &bundlePath, const QString &deviceId, Port gdbPort,
                        Port qmlPort);
    void gotInferiorPid(const FilePath &bundlePath, const QString &deviceId, qint64 pid);
    void deviceInfo(const QString &deviceId, const IosToolHandler::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
    void toolExited(int code);

protected:
    IosToolHandler *q;
    QString m_deviceId;
    FilePath m_bundlePath;
    IosToolHandler::RunKind m_runKind = IosToolHandler::NormalRun;
    IosDeviceType m_devType;
};

class IosDeviceToolHandlerPrivate final : public IosToolHandlerPrivate
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

// IosToolHandlerPrivate overrides
public:
    void requestTransferApp(const FilePath &bundlePath, const QString &deviceId,
                            int timeout = 1000) override;
    void requestRunApp(const FilePath &bundlePath, const QStringList &extraArgs,
                       IosToolHandler::RunKind runKind,
                       const QString &deviceId, int timeout = 1000) override;
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000) override;
    bool isRunning() const override;
    void start(const QString &exe, const QStringList &args);
    void stop(int errorCode) override;

private:
    void subprocessHasData();
    void processXml();

    struct Deleter {
        void operator()(Process *process) const
        {
            if (process->state() != QProcess::NotRunning) {
                process->write("k\n\r");
                process->closeWriteChannel();
            }
            delete process;
        };
    };
    std::unique_ptr<Process, Deleter> process;
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

// IosToolHandlerPrivate overrides
public:
    void requestTransferApp(const FilePath &appBundlePath, const QString &deviceIdentifier,
                            int timeout = 1000) override;
    void requestRunApp(const FilePath &appBundlePath, const QStringList &extraArgs,
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
    LogTailFiles outputLogger;
    FutureSynchronizer futureSynchronizer;
};

IosToolHandlerPrivate::IosToolHandlerPrivate(const IosDeviceType &devType,
                                             Ios::IosToolHandler *q) :
    q(q),
    m_devType(devType)
{
}

IosToolHandlerPrivate::~IosToolHandlerPrivate() = default;

// signals
void IosToolHandlerPrivate::isTransferringApp(const FilePath &bundlePath, const QString &deviceId,
                                              int progress, int maxProgress, const QString &info)
{
    emit q->isTransferringApp(q, bundlePath, deviceId, progress, maxProgress, info);
}

void IosToolHandlerPrivate::didTransferApp(const FilePath &bundlePath, const QString &deviceId,
                    Ios::IosToolHandler::OpStatus status)
{
    emit q->didTransferApp(q, bundlePath, deviceId, status);
}

void IosToolHandlerPrivate::didStartApp(const FilePath &bundlePath, const QString &deviceId,
                                        IosToolHandler::OpStatus status)
{
    emit q->didStartApp(q, bundlePath, deviceId, status);
}

void IosToolHandlerPrivate::gotServerPorts(const FilePath &bundlePath, const QString &deviceId,
                                           Port gdbPort, Port qmlPort)
{
    emit q->gotServerPorts(q, bundlePath, deviceId, gdbPort, qmlPort);
}

void IosToolHandlerPrivate::gotInferiorPid(const FilePath &bundlePath, const QString &deviceId,
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
            const auto elName = outputParser.name();
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
                const auto statusStr = attributes.value(QLatin1String("status"));
                Ios::IosToolHandler::OpStatus status = Ios::IosToolHandler::Unknown;
                if (statusStr.compare(QLatin1String("success"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Success;
                else if (statusStr.compare(QLatin1String("failure"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Failure;
                didStartApp(m_bundlePath, m_deviceId, status);
            } else if (elName == QLatin1String("app_transfer")) {
                stack.append(ParserState(ParserState::AppTransfer));
                QXmlStreamAttributes attributes = outputParser.attributes();
                const auto statusStr = attributes.value(QLatin1String("status"));
                Ios::IosToolHandler::OpStatus status = Ios::IosToolHandler::Unknown;
                if (statusStr.compare(QLatin1String("success"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Success;
                else if (statusStr.compare(QLatin1String("failure"), Qt::CaseInsensitive) == 0)
                    status = Ios::IosToolHandler::Failure;
                didTransferApp(m_bundlePath, m_deviceId, status);
            } else if (elName == QLatin1String("device_info") || elName == QLatin1String("deviceinfo")) {
                stack.append(ParserState(ParserState::DeviceInfo));
            } else if (elName == QLatin1String("inferior_pid")) {
                stack.append(ParserState(ParserState::InferiorPid));
            } else if (elName == QLatin1String("server_ports")) {
                stack.append(ParserState(ParserState::ServerPorts));
                QXmlStreamAttributes attributes = outputParser.attributes();
                Port gdbServerPort(
                    attributes.value(QLatin1String("gdb_server")).toString().toInt());
                Port qmlServerPort(
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
            while (isRunning()) {
                const QByteArray buffer = process->readAllRawStandardOutput();
                if (buffer.isEmpty())
                    return;
                qCDebug(toolHandlerLog) << "subprocessHasData read " << buffer;
                outputParser.addData(buffer);
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
    , process(new Process, Deleter())
{
    // Prepare & set process Environment.
    const Environment systemEnv = Environment::systemEnvironment();
    Environment env(systemEnv);
    systemEnv.forEachEntry([&env](const QString &key, const QString &, bool enabled) {
        if (enabled && key.startsWith(QLatin1String("DYLD_")))
            env.unset(key);
    });

    QStringList frameworkPaths;
    const FilePath libPath = IosConfigurations::developerPath().pathAppended("Platforms/iPhoneSimulator.platform/Developer/Library");
    for (const auto framework : {"PrivateFrameworks", "OtherFrameworks", "SharedFrameworks"}) {
        const QString frameworkPath =
                libPath.pathAppended(QLatin1String(framework)).toFileInfo().canonicalFilePath();
        if (!frameworkPath.isEmpty())
            frameworkPaths << frameworkPath;
    }
    frameworkPaths << "/System/Library/Frameworks" << "/System/Library/PrivateFrameworks";
    env.set(QLatin1String("DYLD_FALLBACK_FRAMEWORK_PATH"), frameworkPaths.join(QLatin1Char(':')));
    qCDebug(toolHandlerLog) << "IosToolHandler runEnv:" << env.toStringList();
    process->setEnvironment(env);
    process->setProcessMode(ProcessMode::Writer);
    process->setReaperTimeout(1500);

    QObject::connect(process.get(), &Process::readyReadStandardOutput,
                     q, [this] { subprocessHasData(); });
    QObject::connect(process.get(), &Process::done, q, [this] {
        if (process->result() == ProcessResult::FinishedWithSuccess) {
            stop((process->exitStatus() == QProcess::NormalExit) ? process->exitCode() : -1);
            qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
        } else {
            if (state != Stopped)
                errorMsg(Tr::tr("iOS tool error %1").arg(process->error()));
            stop(-1);
            if (process->result() == ProcessResult::StartFailed)
                qCDebug(toolHandlerLog) << "IosToolHandler::finished(" << this << ")";
        }
        emit IosToolHandlerPrivate::q->finished(IosToolHandlerPrivate::q);
    });
}

void IosDeviceToolHandlerPrivate::requestTransferApp(const FilePath &bundlePath,
                                                     const QString &deviceId, int timeout)
{
    m_bundlePath = bundlePath;
    m_deviceId = deviceId;
    QString tmpDeltaPath = TemporaryDirectory::masterDirectoryFilePath().pathAppended("ios").toString();
    QStringList args;
    args << QLatin1String("--id") << deviceId << QLatin1String("--bundle")
         << bundlePath.path() << QLatin1String("--timeout") << QString::number(timeout)
         << QLatin1String("--install")
         << QLatin1String("--delta-path")
         << tmpDeltaPath;

    start(IosToolHandler::iosDeviceToolPath(), args);
}

void IosDeviceToolHandlerPrivate::requestRunApp(const FilePath &bundlePath,
                                                const QStringList &extraArgs,
                                                IosToolHandler::RunKind runType,
                                                const QString &deviceId, int timeout)
{
    m_bundlePath = bundlePath;
    m_deviceId = deviceId;
    m_runKind = runType;
    QStringList args;
    args << QLatin1String("--id") << deviceId << QLatin1String("--bundle")
         << bundlePath.path() << QLatin1String("--timeout") << QString::number(timeout);
    switch (runType) {
    case IosToolHandler::NormalRun:
        args << QLatin1String("--run");
        break;
    case IosToolHandler::DebugRun:
        args << QLatin1String("--debug");
        break;
    }
    args << QLatin1String("--") << extraArgs;
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
    process->setCommand({FilePath::fromString(exe), args});
    process->start();
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
        process->stop();
    }
}


// IosSimulatorToolHandlerPrivate

IosSimulatorToolHandlerPrivate::IosSimulatorToolHandlerPrivate(const IosDeviceType &devType,
                                                               IosToolHandler *q)
    : IosToolHandlerPrivate(devType, q)
{
    QObject::connect(&outputLogger, &LogTailFiles::logMessage,
                     q, [q](const QString &message) { q->appOutput(q, message); });
}

void IosSimulatorToolHandlerPrivate::requestTransferApp(const FilePath &appBundlePath,
                                                        const QString &deviceIdentifier, int timeout)
{
    Q_UNUSED(timeout)
    m_bundlePath = appBundlePath;
    m_deviceId = deviceIdentifier;
    isTransferringApp(m_bundlePath, m_deviceId, 0, 100, "");

    auto onSimulatorStart = [this](const SimulatorControl::Response &response) {
        if (response) {
            if (!isResponseValid(*response))
                return;

            installAppOnSimulator();
        } else {
            errorMsg(Tr::tr("Application install on simulator failed. Simulator not running."));
            if (!response.error().isEmpty())
                errorMsg(response.error());
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
            emit q->finished(q);
        }
    };

    if (SimulatorControl::isSimulatorRunning(m_deviceId))
        installAppOnSimulator();
    else
        futureSynchronizer.addFuture(Utils::onResultReady(
            SimulatorControl::startSimulator(m_deviceId), q, onSimulatorStart));
}

void IosSimulatorToolHandlerPrivate::requestRunApp(const FilePath &appBundlePath,
                                                   const QStringList &extraArgs,
                                                   IosToolHandler::RunKind runType,
                                                   const QString &deviceIdentifier, int timeout)
{
    Q_UNUSED(timeout)
    Q_UNUSED(deviceIdentifier)
    m_bundlePath = appBundlePath;
    m_deviceId = m_devType.identifier;
    m_runKind = runType;

    if (!m_bundlePath.exists()) {
        errorMsg(Tr::tr("Application launch on simulator failed. Invalid bundle path %1")
                 .arg(m_bundlePath.toUserOutput()));
        didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
        return;
    }

    auto onSimulatorStart = [this, extraArgs](const SimulatorControl::Response &response) {
        if (response) {
            if (!isResponseValid(*response))
                return;

            launchAppOnSimulator(extraArgs);
        } else {
            errorMsg(Tr::tr("Application launch on simulator failed. Simulator not running. %1")
                         .arg(response.error()));
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
        }
    };

    if (SimulatorControl::isSimulatorRunning(m_deviceId))
        launchAppOnSimulator(extraArgs);
    else
        futureSynchronizer.addFuture(Utils::onResultReady(
            SimulatorControl::startSimulator(m_deviceId), q, onSimulatorStart));
}

void IosSimulatorToolHandlerPrivate::requestDeviceInfo(const QString &deviceId, int timeout)
{
    Q_UNUSED(timeout)
    Q_UNUSED(deviceId)
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
    futureSynchronizer.cancelAllFutures();
    futureSynchronizer.flushFinishedFutures();

    toolExited(errorCode);
    emit q->finished(q);
}

void IosSimulatorToolHandlerPrivate::installAppOnSimulator()
{
    auto onResponseAppInstall = [this](const SimulatorControl::Response &response) {
        if (response) {
            if (!isResponseValid(*response))
                return;
            isTransferringApp(m_bundlePath, m_deviceId, 100, 100, "");
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Success);
        } else {
            errorMsg(Tr::tr("Application install on simulator failed. %1").arg(response.error()));
            didTransferApp(m_bundlePath, m_deviceId, IosToolHandler::Failure);
        }
        emit q->finished(q);
    };

    isTransferringApp(m_bundlePath, m_deviceId, 20, 100, "");
    auto installFuture = SimulatorControl::installApp(m_deviceId, m_bundlePath);
    futureSynchronizer.addFuture(Utils::onResultReady(installFuture, q, onResponseAppInstall));
}

void IosSimulatorToolHandlerPrivate::launchAppOnSimulator(const QStringList &extraArgs)
{
    const QString bundleId = SimulatorControl::bundleIdentifier(m_bundlePath);
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
            errorMsg(Tr::tr("Cannot capture console output from %1. "
                            "Error redirecting output to %2.*")
                     .arg(bundleId).arg(fileTemplate));
    } else {
        errorMsg(Tr::tr("Cannot capture console output from %1. "
                        "Install Xcode 8 or later.").arg(bundleId));
    }

    auto monitorPid = [this](QPromise<void> &promise, qint64 pid) {
#ifdef Q_OS_UNIX
        do {
            // Poll every 1 sec to check whether the app is running.
            QThread::msleep(1000);
        } while (!promise.isCanceled() && kill(pid, 0) == 0);
#else
    Q_UNUSED(pid)
#endif
        // Future is cancelled if the app is stopped from the qt creator.
        if (!promise.isCanceled())
            stop(0);
    };

    auto onResponseAppLaunch = [=](const SimulatorControl::Response &response) {
        if (response) {
            if (!isResponseValid(*response))
                return;
            m_pid = response->inferiorPid;
            gotInferiorPid(m_bundlePath, m_deviceId, response->inferiorPid);
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Success);
            // Start monitoring app's life signs.
            futureSynchronizer.addFuture(Utils::asyncRun(monitorPid, response->inferiorPid));
            if (captureConsole)
                futureSynchronizer.addFuture(Utils::asyncRun(&LogTailFiles::exec, &outputLogger,
                                                             stdoutFile, stderrFile));
        } else {
            m_pid = -1;
            errorMsg(Tr::tr("Application launch on simulator failed. %1").arg(response.error()));
            didStartApp(m_bundlePath, m_deviceId, Ios::IosToolHandler::Failure);
            stop(-1);
            emit q->finished(q);
        }
    };

    futureSynchronizer.addFuture(Utils::onResultReady(SimulatorControl::launchApp(
            m_deviceId, bundleId, debugRun, extraArgs,
            captureConsole ? stdoutFile->fileName() : QString(),
            captureConsole ? stderrFile->fileName() : QString()),
        q, onResponseAppLaunch));
}

bool IosSimulatorToolHandlerPrivate::isResponseValid(const SimulatorControl::ResponseData &responseData)
{
    if (responseData.simUdid.compare(m_deviceId) != 0) {
        errorMsg(Tr::tr("Invalid simulator response. Device Id mismatch. "
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
    return Core::ICore::libexecPath("ios/iostool").toString();
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

void IosToolHandler::requestTransferApp(const FilePath &bundlePath, const QString &deviceId,
                                        int timeout)
{
    d->requestTransferApp(bundlePath, deviceId, timeout);
}

void IosToolHandler::requestRunApp(const FilePath &bundlePath, const QStringList &extraArgs,
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
