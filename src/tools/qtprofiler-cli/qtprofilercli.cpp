// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <profiler/profilerrecorder.h>
#include <profiler/sampler.h>
#include <profiler/traceformat.h>

#include <utils/appinfo.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/qtcsettings.h>
#include <utils/qtcsettings_p.h>
#include <utils/storekey.h>
#include <utils/temporarydirectory.h>

#include <app/app_version.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QTemporaryDir>

#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace Profiler::Internal;
using namespace Utils;
using namespace std::chrono;
using namespace Qt::StringLiterals;

namespace {

constexpr int protocolVersion = 1;

// The descriptors the protocol uses. A launched target inherits the standard
// ones, so the protocol moves to copies that are not inherited, and the standard
// ones are pointed somewhere harmless: the target reads nothing from fd 0, and
// what it writes to fd 1 goes to stderr.
struct ProtocolChannels
{
    int in = 0;
    int out = 1;
};

static ProtocolChannels takeStandardChannels()
{
#ifdef Q_OS_WIN
    const ProtocolChannels channels{_dup(0), _dup(1)};
    // _dup() makes inheritable copies, and a launched target is created with
    // handle inheritance on.
    for (const int fd : {channels.in, channels.out}) {
        SetHandleInformation(reinterpret_cast<HANDLE>(_get_osfhandle(fd)),
                             HANDLE_FLAG_INHERIT, 0);
    }
    const int null = _open("NUL", _O_RDWR);
    _dup2(null, 0);
    _dup2(2, 1);
    _close(null);
    return channels;
#else
    const ProtocolChannels channels{dup(0), dup(1)};
    fcntl(channels.in, F_SETFD, FD_CLOEXEC);
    fcntl(channels.out, F_SETFD, FD_CLOEXEC);
    const int null = open("/dev/null", O_RDWR);
    dup2(null, 0);
    dup2(2, 1);
    close(null);
    return channels;
#endif
}

// JSON-RPC 2.0 error codes.
constexpr int parseError = -32700;
constexpr int invalidRequest = -32600;
constexpr int methodNotFound = -32601;
constexpr int invalidParams = -32602;
// Our own: the request is fine, the tool is in the wrong state for it.
constexpr int wrongState = -32001;
// Our own: the recording could not be started.
constexpr int startFailed = -32002;

static QString formatName(TraceFormat format)
{
    switch (format) {
    case TraceFormat::Qml: return u"qml"_s;
    case TraceFormat::Perf: return u"perf"_s;
    case TraceFormat::Ctf: return u"ctf"_s;
    case TraceFormat::Sampler: return u"sampler"_s;
    case TraceFormat::Combined: return u"combined"_s;
    }
    return {};
}

class Controller : public QObject
{
    Q_OBJECT

public:
    Controller(bool rpc, int outputDescriptor, QObject *parent = nullptr)
        : QObject(parent)
        , m_rpc(rpc)
    {
        if (!m_out.open(outputDescriptor, QIODevice::WriteOnly | QIODevice::Unbuffered,
                        QFileDevice::AutoCloseHandle)) {
            writeError(u"Cannot open the output channel: %1"_s.arg(m_out.errorString()));
        }

        connect(&m_recorder, &ProfilerRecorder::started, this, [this](const QString &target) {
            notify(u"recordingStarted"_s, {{u"target"_s, target}});
        });
        connect(&m_recorder, &ProfilerRecorder::captureStarted, this, [this] {
            notify(u"captureStarted"_s);
        });
        connect(&m_recorder, &ProfilerRecorder::pausedChanged, this, [this](bool paused) {
            notify(u"pausedChanged"_s, {{u"paused"_s, paused}});
        });
        connect(&m_recorder, &ProfilerRecorder::processingStarted, this, [this] {
            notify(u"processingStarted"_s);
        });
        connect(&m_recorder, &ProfilerRecorder::progressChanged, this, [this](int percent) {
            notify(u"progress"_s, {{u"percent"_s, percent}});
        });
        connect(&m_recorder, &ProfilerRecorder::statusChanged, this,
                [this](const QString &text, const QString &toolTip) {
            notify(u"status"_s, {{u"text"_s, text}, {u"toolTip"_s, toolTip}});
        });
        connect(&m_recorder, &ProfilerRecorder::finished, this, [this](const FilePath &trace) {
            const QString path = trace.toFSPathString();
            notify(u"recordingFinished"_s,
                   {{u"tracePath"_s, path},
                    {u"format"_s, formatName(identifyTrace(trace).format)}});
            if (!m_rpc) {
                writeLine(path.toUtf8());
                QCoreApplication::exit(0);
            }
        });
        connect(&m_recorder, &ProfilerRecorder::error, this,
                [this](const QString &message, const std::optional<SamplerFix> &fix) {
            QJsonObject params{{u"message"_s, message}};
            if (fix) {
                params.insert(u"fix"_s,
                              QJsonObject{{u"buttonText"_s, fix->buttonText},
                                          {u"detail"_s, fix->detail},
                                          {u"command"_s, fix->command.toUserOutput()}});
            }
            notify(u"recordingFailed"_s, params);
            if (!m_rpc)
                QCoreApplication::exit(1);
        });
    }

    ProfilerRecorder &recorder() { return m_recorder; }

    void handleLine(const QByteArray &line)
    {
        if (line.trimmed().isEmpty())
            return;
        QJsonParseError parse;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parse);
        if (parse.error != QJsonParseError::NoError) {
            replyError({}, parseError, parse.errorString());
            return;
        }
        if (!document.isObject()) {
            replyError({}, invalidRequest, u"A request is a JSON object."_s);
            return;
        }
        const QJsonObject request = document.object();
        const QJsonValue id = request.value("id"_L1);
        const QString method = request.value("method"_L1).toString();
        if (method.isEmpty()) {
            replyError(id, invalidRequest, u"The request names no method."_s);
            return;
        }
        // A request without an id is a notification; it gets no answer.
        const Reply reply = dispatch(method, request.value("params"_L1).toObject());
        if (!id.isUndefined()) {
            if (reply)
                replyResult(id, reply.result);
            else
                replyError(id, reply.code, reply.message, reply.data);
        }
        sendHeldNotifications();
    }

    // Stops what runs and waits for it to wind down, for the end of input.
    void finish()
    {
        m_recorder.stopAndWait();
    }

    // The command-line mode: one recording, described by arguments.
    void recordFromArguments(const QString &backend, const QString &launch,
                             milliseconds duration)
    {
        QJsonObject params{{u"backend"_s, backend}};
        const CommandLine command = CommandLine::fromUserInput(launch);
        params.insert(u"executable"_s, command.executable().toFSPathString());
        params.insert(u"arguments"_s, command.arguments());
        if (duration > 0ms)
            params.insert(u"durationMs"_s, int(duration.count()));
        if (const Reply started = dispatch(u"startRecording"_s, params); !started) {
            writeError(started.message);
            QCoreApplication::exit(1);
        }
        sendHeldNotifications();
    }

    void listBackends()
    {
        const QList<Id> ids = m_recorder.backendIds();
        const QStringList names = m_recorder.backendNames();
        for (qsizetype i = 0; i < ids.size(); ++i)
            writeLine(u"%1\t%2"_s.arg(ids.at(i).toString(), names.at(i)).toUtf8());
    }

private:
    // What a method answers: a result, or an error with its JSON-RPC code.
    struct Reply
    {
        Reply(const QJsonObject &result = {}) : result(result) {}
        Reply(int code, const QString &message, const QJsonObject &data = {})
            : code(code), message(message), data(data)
        {}
        explicit operator bool() const { return code == 0; }

        QJsonObject result;
        int code = 0;
        QString message;
        QJsonObject data;
    };
    static Reply fail(int code, const QString &message, const QJsonObject &data = {})
    {
        return {code, message, data};
    }

    Reply dispatch(const QString &method, const QJsonObject &params)
    {
        if (method == "initialize"_L1)
            return initialize();
        if (method == "startRecording"_L1)
            return startRecording(params);
        if (method == "stopRecording"_L1) {
            if (!m_recorder.isRecording())
                return fail(wrongState, u"Nothing is being recorded."_s);
            m_recorder.stop();
            return QJsonObject();
        }
        if (method == "pauseRecording"_L1 || method == "resumeRecording"_L1) {
            if (!m_recorder.isRecording() || !m_recorder.canPause())
                return fail(wrongState, u"The recording cannot be paused."_s);
            if (method == "pauseRecording"_L1)
                m_recorder.pause();
            else
                m_recorder.resume();
            return QJsonObject();
        }
        if (method == "getStatus"_L1)
            return status();
        if (method == "shutdown"_L1) {
            // Answered first by handleLine(); quit once that is out.
            QMetaObject::invokeMethod(this, [this] {
                m_recorder.stopAndWait();
                QCoreApplication::exit(0);
            }, Qt::QueuedConnection);
            return QJsonObject();
        }
        return fail(methodNotFound, u"Unknown method \"%1\"."_s.arg(method));
    }

    QJsonObject initialize() const
    {
        QJsonArray backends;
        const QList<Id> ids = m_recorder.backendIds();
        const QStringList names = m_recorder.backendNames();
        for (qsizetype i = 0; i < ids.size(); ++i)
            backends.append(QJsonObject{{u"id"_s, ids.at(i).toString()}, {u"name"_s, names.at(i)}});
        return {{u"name"_s, u"qtprofiler-cli"_s},
                {u"version"_s, QCoreApplication::applicationVersion()},
                {u"protocolVersion"_s, protocolVersion},
                {u"backends"_s, backends}};
    }

    QJsonObject status() const
    {
        const char *state = "idle";
        if (m_recorder.isProcessing())
            state = "processing";
        else if (m_recorder.isCapturing())
            state = "capturing";
        else if (m_recorder.isRecording())
            state = "starting";
        return {{u"state"_s, QString::fromLatin1(state)},
                {u"paused"_s, m_recorder.isPaused()},
                {u"recordedMs"_s, int(m_recorder.recordedTime().count())}};
    }

    Reply startRecording(const QJsonObject &params)
    {
        if (m_recorder.isRecording())
            return fail(wrongState, u"A recording is already running."_s);

        const QString spec = params.value("backend"_L1).toString();
        const QList<Id> ids = m_recorder.backendIds();
        int index = -1;
        for (qsizetype i = 0; i < ids.size(); ++i) {
            // The whole id, or its last part: "CallStack" for "Profiler.Sampler.CallStack".
            const QString id = ids.at(i).toString();
            if (id == spec || id.endsWith(u'.' + spec, Qt::CaseInsensitive))
                index = int(i);
        }
        if (index < 0 && !m_recorder.selectBackend(spec))
            return fail(invalidParams, u"Unknown backend \"%1\"."_s.arg(spec));
        if (index >= 0)
            m_recorder.setCurrentBackend(index);
        Sampler *backend = m_recorder.backendById(ids.at(m_recorder.currentBackend()));
        SamplerSettings *settings = backend ? backend->settings() : nullptr;

        const QJsonObject options = params.value("options"_L1).toObject();
        for (auto it = options.begin(); it != options.end(); ++it) {
            BaseAspect *match = nullptr;
            if (settings) {
                for (BaseAspect *aspect : settings->aspects()) {
                    if (stringFromKey(aspect->settingsKey()) == it.key())
                        match = aspect;
                }
            }
            if (!match)
                return fail(invalidParams, u"Unknown option \"%1\"."_s.arg(it.key()));
            match->setVariantValue(it.value().toVariant());
        }

        const QString executable = params.value("executable"_L1).toString();
        if (!executable.isEmpty()) {
            Environment environment = Environment::systemEnvironment();
            const QJsonObject changes = params.value("environment"_L1).toObject();
            for (auto it = changes.begin(); it != changes.end(); ++it)
                environment.set(it.key(), it.value().toString());
            m_recorder.seedLaunchTarget(
                CommandLine(FilePath::fromUserInput(executable),
                            params.value("arguments"_L1).toString(), CommandLine::Raw),
                FilePath::fromUserInput(params.value("workingDirectory"_L1).toString()),
                environment);
        }

        const bool paused = params.value("paused"_L1).toBool();
        if (paused && !m_recorder.currentBackendCanPause())
            return fail(invalidParams, u"This backend cannot start paused."_s);

        // Most reasons a recording cannot start are found while starting it. Hold
        // back what that reports, so that such a start is answered with its error
        // rather than with success after the report of its failure.
        m_holdNotifications = true;
        const int durationMs = params.value("durationMs"_L1).toInt();
        if (paused)
            m_recorder.startPaused();
        else if (durationMs > 0)
            m_recorder.startTimed(milliseconds(durationMs));
        else
            m_recorder.start();
        m_holdNotifications = false;

        if (m_recorder.isRecording())
            return QJsonObject();
        QJsonObject failure;
        for (const auto &[method, notificationParams] : std::as_const(m_heldNotifications)) {
            if (method == "recordingFailed"_L1)
                failure = notificationParams;
        }
        m_heldNotifications.clear();
        const QString message = failure.value("message"_L1).toString();
        failure.remove("message"_L1);
        return fail(startFailed, message.isEmpty() ? u"The recording did not start."_s : message,
                    failure);
    }

    void sendHeldNotifications()
    {
        const QList<std::pair<QString, QJsonObject>> held = std::exchange(m_heldNotifications, {});
        for (const auto &[method, params] : held)
            notify(method, params);
    }

    void notify(const QString &method, const QJsonObject &params = {})
    {
        if (m_holdNotifications) {
            m_heldNotifications.append({method, params});
            return;
        }
        if (m_rpc) {
            writeLine(QJsonDocument(QJsonObject{{u"jsonrpc"_s, u"2.0"_s},
                                                {u"method"_s, method},
                                                {u"params"_s, params}})
                          .toJson(QJsonDocument::Compact));
            return;
        }
        // Human readable progress for the command-line mode.
        if (method == "status"_L1 && !params.value("text"_L1).toString().isEmpty())
            writeError(params.value("text"_L1).toString());
        else if (method == "recordingFailed"_L1)
            writeError(params.value("message"_L1).toString());
    }

    void replyResult(const QJsonValue &id, const QJsonObject &result)
    {
        writeLine(QJsonDocument(QJsonObject{{u"jsonrpc"_s, u"2.0"_s},
                                            {u"id"_s, id},
                                            {u"result"_s, result}})
                      .toJson(QJsonDocument::Compact));
    }

    void replyError(const QJsonValue &id, int code, const QString &message,
                    const QJsonObject &data = {})
    {
        QJsonObject error{{u"code"_s, code}, {u"message"_s, message}};
        if (!data.isEmpty())
            error.insert(u"data"_s, data);
        writeLine(QJsonDocument(QJsonObject{
                                    {u"jsonrpc"_s, u"2.0"_s},
                                    {u"id"_s, id.isUndefined() ? QJsonValue(QJsonValue::Null) : id},
                                    {u"error"_s, error}})
                      .toJson(QJsonDocument::Compact));
    }

    void writeLine(const QByteArray &line)
    {
        m_out.write(line + '\n');
        m_out.flush();
    }

    static void writeError(const QString &text)
    {
        QFile err;
        if (err.open(stderr, QIODevice::WriteOnly | QIODevice::Unbuffered))
            err.write(text.toUtf8() + '\n');
    }

    const bool m_rpc;
    QFile m_out;
    bool m_holdNotifications = false;
    QList<std::pair<QString, QJsonObject>> m_heldNotifications;
    ProfilerRecorder m_recorder;
};

// Lets the input thread hand lines to the controller only while it exists:
// main() clears `controller` before the controller goes.
struct ControllerLink
{
    QMutex lock;
    Controller *controller = nullptr;
};

} // namespace

// Sets up the settings the recorder uses, which only a few may (see
// qtcsettings_p.h): the tool is not always started through main().
class QtProfilerCliSettings
{
public:
    static void setup(QtcSettings *userSettings, QtcSettings *installSettings)
    {
        Utils::Internal::SettingsSetup::setupSettings(userSettings, installSettings);
    }
    static void destroy() { Utils::Internal::SettingsSetup::destroySettings(); }
};

// As a library, the tool is started by the launcher in qtprofilercli_win.cpp.
#ifdef QTPROFILERCLI_LIBRARY
extern "C" Q_DECL_EXPORT int qtProfilerCliMain(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
#ifndef Q_OS_WIN
    // A launched target that exits can deliver SIGHUP through the terminal;
    // the default action would end this tool as the recording finishes. A
    // handler, unlike an ignored signal, is reset for the processes it starts.
    std::signal(SIGHUP, [](int) {});
#endif
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(Core::Constants::IDE_SETTINGSVARIANT_STR);
    QCoreApplication::setApplicationName("QtProfilerCli");
    QCoreApplication::setApplicationVersion(Core::Constants::IDE_VERSION_COMPAT);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Records profiler traces without a user interface. With --rpc it is controlled "
        "through JSON-RPC 2.0, one message per line, on stdin and stdout; otherwise it makes "
        "one recording and prints the path of the trace.");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption rpc({u"r"_s, u"rpc"_s}, u"Be controlled through JSON-RPC 2.0 on stdio."_s);
    const QCommandLineOption listBackends(u"list-backends"_s,
                                          u"Print the available backends, as id and name, and exit."_s);
    const QCommandLineOption backend({u"b"_s, u"backend"_s},
                                     u"Recording backend, by id or part of its name."_s,
                                     u"backend"_s);
    const QCommandLineOption launch({u"l"_s, u"launch"_s},
                                    u"Command to launch and profile."_s, u"command"_s);
    const QCommandLineOption recordFor(u"record-for"_s,
                                       u"Stop recording after <seconds>."_s, u"seconds"_s);
    parser.addOptions({rpc, listBackends, backend, launch, recordFor});
    parser.process(app);
    // After the parser, which prints --help and --version to stdout.
    const ProtocolChannels channels = takeStandardChannels();

    // The recorder reads and writes settings: keep them out of the user's own.
    QTemporaryDir settingsDirectory;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QtcSettings *userSettings = new QtcSettings(QSettings::IniFormat, QSettings::UserScope,
                                                Core::Constants::IDE_SETTINGSVARIANT_STR);
    QtProfilerCliSettings::setup(userSettings, new QtcSettings);

    AppInfo info;
    info.id = u"qtprofilercli"_s;
    info.libexec = FilePath::fromUserInput(QCoreApplication::applicationDirPath());
    Internal::setAppInfo(info);
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/qtprofilercli-XXXXXX");

    int exitCode = 0;
    {
        Controller controller(parser.isSet(rpc), channels.out);
        if (parser.isSet(listBackends)) {
            controller.listBackends();
        } else if (parser.isSet(rpc)) {
            // The thread blocks in a read nothing can interrupt, so it is not joined:
            // it ends with the process, and only talks to a controller that is
            // still there (see ControllerLink).
            const auto link = std::make_shared<ControllerLink>();
            link->controller = &controller;
            std::thread([link, fd = channels.in] {
                const auto post = [&link](const std::function<void(Controller *)> &call) {
                    QMutexLocker locker(&link->lock);
                    if (Controller *controller = link->controller) {
                        QMetaObject::invokeMethod(controller, [controller, call] {
                            call(controller);
                        }, Qt::QueuedConnection);
                    }
                };
                QFile in;
                if (in.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
                    // An empty array is end of input; an empty line still has its newline.
                    for (QByteArray line = in.readLine(); !line.isEmpty(); line = in.readLine())
                        post([line](Controller *controller) { controller->handleLine(line); });
                }
                post([](Controller *controller) {
                    controller->finish();
                    QCoreApplication::exit(0);
                });
            }).detach();
            exitCode = app.exec();
            QMutexLocker locker(&link->lock);
            link->controller = nullptr;
        } else if (parser.isSet(launch)) {
            double seconds = 0;
            if (parser.isSet(recordFor)) {
                bool ok = false;
                seconds = parser.value(recordFor).toDouble(&ok);
                if (!ok || seconds <= 0) {
                    qCritical("--record-for expects a positive number of seconds");
                    return 1;
                }
            }
            QMetaObject::invokeMethod(&controller, [&] {
                controller.recordFromArguments(parser.value(backend), parser.value(launch),
                                               milliseconds(qint64(seconds * 1000)));
            }, Qt::QueuedConnection);
            exitCode = app.exec();
            controller.finish();
        } else {
            parser.showHelp(1);
        }
    }
    // Lets queued calls that still hold the finished recording run, and the
    // objects it asked to have deleted go, while the application is alive: its
    // destructor would otherwise delete them with the event list locked.
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QtProfilerCliSettings::destroy();
    return exitCode;
}

#include "qtprofilercli.moc"
