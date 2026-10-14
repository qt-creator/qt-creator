// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginarserver.h"

#include "axivionperspective.h"
#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontextmarks.h"
#include "axivionutils.h"
#include "axiviontr.h"

#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/globaltasktree.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QtTaskTree/QConditional>
#include <QtTaskTree/QMappedTaskTreeRunner>
#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QTimer>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

namespace Axivion::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.axivion.pluginarserver", QtWarningMsg)

class ArServerData
{
public:
    QByteArray m_accessSecret;
    QUrl m_serverUrl;
    QString m_disposeIssuesRel;
    QString m_arSessionStartRel;
    QString m_arSessionFinishRel;
    QHash<int, QString> m_runningSessions;
    // callers waiting for the server to become initialized
    QList<std::pair<OnServerStarted, OnServerStartFailed>> m_waiting;
};

class ArServerProcessData;

class PluginArServers final : public QObject
{
    Q_OBJECT

public:
    PluginArServers();
    ~PluginArServers() final
    {
        QTC_CHECK(!m_runner.isRunning()); // shutdownAll() must be done already
    }

    void start(const FilePath &bauhausSuite, const OnServerStarted &onRunning,
               const OnServerStartFailed &onFailed);
    void shutdown(const FilePath &bauhausSuite);
    void cleanShutdown(const FilePath &bauhausSuite) { emit shutdownRequested(bauhausSuite); }
    bool shutdownAll(const std::function<void()> &callback);

    ArServerData *runningServer(const FilePath &bauhausSuite);

signals:
    void shutdownRequested(const FilePath &bauhausSuite);

private:
    void handleOutput(const FilePath &bauhausSuite, ArServerProcessData *data, Process *process);
    void handleMessage(const QByteArray &msg, const FilePath &bauhausSuite,
                       ArServerProcessData *data, Process *process);
    void notifyStarted(const FilePath &bauhausSuite);
    void notifyFailed(const FilePath &bauhausSuite);
    void onServerFinished();
    void finishShutdown();

    QHash<FilePath, ArServerData> m_servers;
    QMappedTaskTreeRunner<FilePath> m_runner;
    QSingleTaskTreeRunner m_shutdownRunner;
    QTimer m_shutdownTimer;
    std::function<void()> m_onShutdownDone;
};

static GuardedObject<PluginArServers> s_pluginArServers;

static ExecutableItem disposeIssuesRecipe(const FilePath &bauhausSuite,
                                          const QList<qint64> &issues);

enum class State { Starting, HttpModeRequested, Initialized, Failed, ShuttingDown };

static QJsonDocument msgToJson(int msgId, int inReplyTo, const QString &subject,
                               const QVariantMap &args)
{
    QJsonObject message;
    message.insert("messageId", msgId);
    message.insert("inReplyTo", inReplyTo);
    message.insert("subject", subject);
    if (!args.isEmpty())
        message.insert("arguments", QJsonValue::fromVariant(args));
    return QJsonDocument{message};
}

static void writeError(const QString &error)
{
    static const QString prefix{"[Axivion|PluginArServer] "};
    MessageManager::writeFlashing(prefix + error);
}

class ArServerProcessData
{
public:
    QByteArray m_buffer = {};
    int m_msgCounter = 0;
    State m_state = State::Starting;
};

static void writeMessage(Process *process, const QJsonDocument &msg)
{
    process->writeRaw(msg.toJson(QJsonDocument::Compact));
    process->writeRaw(QByteArray{1, 0x17});
}

void PluginArServers::handleMessage(const QByteArray &msg, const FilePath &bauhausSuite,
                                    ArServerProcessData *data, Process *process)
{
    qCInfo(log) << "handling message:" << msg;
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(msg, &error);
    if (error.error != QJsonParseError::NoError) {
        writeError(
            Tr::tr("Cannot parse JSON data \"%1\": %2").arg(QString::fromUtf8(msg)).arg(error.error));
        return;
    }
    if (!json.isObject()) {
        writeError(Tr::tr("Not a JSON object: %1").arg(QString::fromUtf8(msg)));
        return;
    }
    const QJsonObject obj = json.object();

    ArServerData &serverData = m_servers[bauhausSuite];

    switch (data->m_state) {
    case State::Starting: {
        if (!obj.contains("serverVersion") || !obj.contains("accessSecret")) {
            writeError(Tr::tr("Unexpected response when starting PluginArServer."));
            return;
        }
        serverData.m_accessSecret = obj.value("accessSecret").toString().toUtf8();
        const QVariantMap args{{"serveIssues", true}};
        const QJsonDocument message = msgToJson(++data->m_msgCounter, 0, "Command: Start HTTP Server",
                                                args);
        data->m_state = State::HttpModeRequested;
        qCDebug(log) << "requesting http server start";
        writeMessage(process, message);
    } break;
    case State::HttpModeRequested: {
        const int inReplyTo = obj.value("inReplyTo").toInt();
        QTC_CHECK(inReplyTo == data->m_msgCounter); // should we handle this?
        if (obj.value("subject").toString() != "Response: OK") {
            writeError(Tr::tr("Unexpected response for HTTP server start request."));
            data->m_state = State::Failed;
            QMetaObject::invokeMethod(process,
                [this, bauhausSuite] { shutdown(bauhausSuite); }, Qt::QueuedConnection);
            return;
        }
        const QVariantMap args = obj.value("arguments").toVariant().toMap();
        serverData.m_serverUrl = args.value("httpServerUri").toString();
        serverData.m_disposeIssuesRel = args.value("httpServerDisposeIssuesRelativeUri").toString();
        serverData.m_arSessionStartRel = args.value("httpServerARSessionStartRelativeUri").toString();
        serverData.m_arSessionFinishRel = args.value("httpServerARSessionFinishRelativeUri").toString();
        QTC_CHECK(!serverData.m_serverUrl.isEmpty());
        // QTC_CHECK(!m_disposeIssuesRel.isEmpty()); // relevant only if serverIssues=true arg
        QTC_CHECK(!serverData.m_arSessionStartRel.isEmpty());
        QTC_CHECK(!serverData.m_arSessionFinishRel.isEmpty());
        qCDebug(log) << "server started at" << serverData.m_serverUrl;
        data->m_state = State::Initialized;
        notifyStarted(bauhausSuite);
    } break;
    case State::Failed:
    case State::ShuttingDown:
        qCDebug(log) << "ignoring message" << msg;
        break;
    case State::Initialized:
        const int inReplyTo = obj.value("inReplyTo").toInt();
        QTC_CHECK(inReplyTo == data->m_msgCounter); // should we handle this?
        qCDebug(log) << "I" << msg; // json - atm we expect a response ok for clean shutdown..
        break;
    }
}

void PluginArServers::handleOutput(const FilePath &bauhausSuite, ArServerProcessData *data,
                                   Process *process)
{
    data->m_buffer.append(process->readAllRawStandardOutput());
    const int endOfFrame = data->m_buffer.indexOf(0x17);
    if (endOfFrame == -1)
        return;

    qsizetype start = 0;
    do {
        qsizetype eoMessage = data->m_buffer.indexOf(0x10, start);
        if (eoMessage == -1)
            break;
        handleMessage(data->m_buffer.mid(start, eoMessage - start), bauhausSuite, data, process);
        start = eoMessage + 1;
    } while (true);
    if (start == 0) {
        handleMessage(data->m_buffer.mid(start, endOfFrame), bauhausSuite, data, process);
    }
    // cut processed output
    data->m_buffer = data->m_buffer.mid(endOfFrame + 1);
}

PluginArServers::PluginArServers()
{
    m_shutdownTimer.setSingleShot(true);
    m_shutdownTimer.setInterval(std::chrono::seconds(5));
    connect(&m_shutdownTimer, &QTimer::timeout, this, [this] {
        qCDebug(log) << "clean shutdown timed out, killing remaining servers";
        m_shutdownRunner.reset();
        m_runner.reset();
        finishShutdown();
    });
}

void PluginArServers::start(const FilePath &bauhausSuite, const OnServerStarted &onRunning,
                            const OnServerStartFailed &onFailed)
{
    if (ExtensionSystem::PluginManager::isShuttingDown())
        return;

    if (m_runner.isKeyRunning(bauhausSuite)) {
        if (runningServer(bauhausSuite)) {
            if (onRunning)
                onRunning();
        } else {
            m_servers[bauhausSuite].m_waiting.append({onRunning, onFailed});
        }
        return;
    }

    const Storage<ArServerProcessData> storage;

    const auto onSetup = [this, bauhausSuite, storage](Process &process) {
        Environment env = Environment::systemEnvironment();
        if (!bauhausSuite.isEmpty())
            env.prependOrSetPath(bauhausSuite.pathAppended("bin"));
        if (!settings().javaHome().isEmpty())
            env.set("JAVA_HOME", settings().javaHome().nativePath());
        if (!settings().bauhausPython().isEmpty())
            env.set("BAUHAUS_PYTHON", settings().bauhausPython().nativePath());
        env.set("PYTHON_IO_ENCODING", "utf-8:replace");
        env.set("PYTHONUNBUFFERED", "1");
        env.set("AXIVION_USER_AGENT", QString::fromUtf8(axivionUserAgent()));

        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs({FilePath::fromString("pluginARServer").withExecutableSuffix(), {}},
                                 CommandLine::Raw);
        process.setCommand(cmd);

        process.setEnvironment(env);
        process.setProcessMode(ProcessMode::Writer);
        connect(&process, &Process::readyReadStandardOutput, &process,
                [this, bauhausSuite, data = storage.activeStorage(), process = &process] {
            handleOutput(bauhausSuite, data, process);
        });
        connect(this, &PluginArServers::shutdownRequested, &process,
                [this, bauhausSuite, data = storage.activeStorage(), process = &process]
                (const FilePath &suite) {
            if (bauhausSuite != suite || data->m_state == State::Failed
                    || data->m_state == State::ShuttingDown) {
                return;
            }
            data->m_state = State::ShuttingDown;
            if (process->state() != ProcessState::Running) {
                QMetaObject::invokeMethod(process, [this, bauhausSuite] { shutdown(bauhausSuite); },
                                          Qt::QueuedConnection);
                return;
            }
            writeMessage(process, msgToJson(++data->m_msgCounter, 0, "Command: Shutdown", {}));
        });
    };
    const auto onDone = [this, bauhausSuite, storage](const Process &proc) {
        if (storage->m_state == State::Starting
                || storage->m_state == State::HttpModeRequested) {
            const QString details = proc.error() == ProcessError::UnknownError
                    ? proc.cleanedStdErr() : proc.errorString();
            writeError(Tr::tr("PluginArServer stopped unexpectedly.").append('\n').append(details));
        }
        if (storage->m_state != State::Initialized && storage->m_state != State::ShuttingDown)
            notifyFailed(bauhausSuite);
        m_servers.remove(bauhausSuite);
    };

    const Group recipe {
        storage,
        ProcessTask(onSetup, onDone)
    };

    ArServerData serverData;
    serverData.m_waiting.append({onRunning, onFailed});
    m_servers.insert(bauhausSuite, serverData);
    m_runner.start(bauhausSuite, recipe, {}, [this] { onServerFinished(); });
}

void PluginArServers::shutdown(const FilePath &bauhausSuite)
{
    m_runner.cancelKey(bauhausSuite);
}

bool PluginArServers::shutdownAll(const std::function<void()> &callback)
{
    // issues must be disposed before the server is asked to shut down
    const QHash<FilePath, QList<qint64>> issueIds = takeAllSfaIssues();
    if (!m_runner.isRunning()) {
        m_servers.clear();
        return false;
    }

    m_onShutdownDone = callback;
    m_shutdownTimer.start();

    QList<GroupItem> perServer{parallel};
    const QList<FilePath> bauhausSuites = m_servers.keys();
    for (const FilePath &bauhausSuite : bauhausSuites) {
        const QList<qint64> issues = issueIds.value(bauhausSuite);
        perServer << Group {
            finishAllAndSuccess,
            issues.isEmpty() ? nullItem : disposeIssuesRecipe(bauhausSuite, issues),
            QSyncTask([this, bauhausSuite] { cleanShutdown(bauhausSuite); })
        };
    }
    m_shutdownRunner.start(perServer);
    return true;
}

ArServerData *PluginArServers::runningServer(const FilePath &bauhausSuite)
{
    const auto it = m_servers.find(bauhausSuite);
    if (it == m_servers.end() || it->m_serverUrl.isEmpty())
        return nullptr;
    return &*it;
}

void PluginArServers::notifyStarted(const FilePath &bauhausSuite)
{
    const auto it = m_servers.find(bauhausSuite);
    QTC_ASSERT(it != m_servers.end(), return);
    const auto waiting = std::exchange(it->m_waiting, {});
    for (const auto &[onRunning, onFailed] : waiting) {
        if (onRunning)
            onRunning();
    }
}

void PluginArServers::notifyFailed(const FilePath &bauhausSuite)
{
    const auto it = m_servers.find(bauhausSuite);
    if (it == m_servers.end())
        return;
    const auto waiting = std::exchange(it->m_waiting, {});
    for (const auto &[onRunning, onFailed] : waiting) {
        if (onFailed)
            onFailed();
    }
}

void PluginArServers::onServerFinished()
{
    if (m_shutdownTimer.isActive() && !m_runner.isRunning())
        finishShutdown();
}

void PluginArServers::finishShutdown()
{
    m_shutdownTimer.stop();
    m_shutdownRunner.reset();
    m_servers.clear();
    if (const auto callback = std::exchange(m_onShutdownDone, {}))
        callback();
}

void startPluginArServer(const FilePath &bauhausSuite, const OnServerStarted &onRunning,
                         const OnServerStartFailed &onFailed)
{
    s_pluginArServers->start(bauhausSuite, onRunning, onFailed);
}

bool shutdownAllPluginArServers(const std::function<void()> &callback)
{
    return s_pluginArServers->shutdownAll(callback);
}

static void setupQuery(QNetworkReplyWrapper &query, const QUrl &url, const QByteArray &auth,
                       const QJsonDocument &body)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", auth);
    request.setRawHeader("Content-Type", contentTypeData(ContentType::Json));
    request.setRawHeader("X-Axivion-User-Agent", axivionUserAgent());
    query.setRequest(request);
    query.setOperation(QNetworkAccessManager::PostOperation);
    if (!body.isNull())
        query.setData(body.toJson(QJsonDocument::Compact));
    query.setNetworkAccessManager(axivionNetworkManager());
}

using OnDoneCallback = std::function<DoneResult(const QByteArray &)>;

static QByteArray basicAuth(const QByteArray &secret)
{
    return "Basic " + QByteArray(secret).prepend(':').toBase64();
}

static ExecutableItem sendQueryRecipe(const Storage<QByteArray> &replyStorage,
                                      const QString &relative,
                                      const FilePath &bauhausSuite,
                                      const QJsonDocument &body = {})
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server)
        return errorItem;

    const QUrl url = server->m_serverUrl.resolved(relative);
    const QByteArray auth = basicAuth(server->m_accessSecret);

    qCDebug(log) << "URL" << url;
    const auto onNetworkQuerySetup = [url, auth, body](QNetworkReplyWrapper &query) {
        setupQuery(query, url, auth, body);
    };
    const auto onNetworkQueryDone = [bauhausSuite, replyStorage](const QNetworkReplyWrapper &query,
                                                                 DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray contentType = contentTypeFromRawHeader(reply);
        *replyStorage = reply->readAll();
        if (doneWith == DoneWith::Success && status == 200 && contentType == s_jsonContentType)
            return DoneResult::Success;
        if (doneWith == DoneWith::Success && status > 200 && status < 300) // e.g. issue disposal
            return DoneResult::Success;
        return DoneResult::Error;
    };

    return QNetworkReplyWrapperTask(onNetworkQuerySetup, onNetworkQueryDone);
}

void requestArSessionStart(const FilePath &bauhausSuite, const OnSessionStarted &onStarted,
                           const OnSessionStartFailed &onFailed)
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server) {
        if (onFailed)
            onFailed();
        return;
    }

    const Storage<QByteArray> replyStorage;
    const auto onSuccess = [bauhausSuite, onStarted, replyStorage] {
        qCDebug(log) << "pluginar session started for" << bauhausSuite;
        // handle the response....
        QJsonParseError error;
        const QJsonDocument json = QJsonDocument::fromJson(*replyStorage, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            qCDebug(log) << "parse error (session start reply)";
            return DoneResult::Error;
        }
        const QJsonObject jsonObj = json.object();
        const int id = jsonObj.value("arSessionId").toInt();
        const QString pipe = jsonObj.value("pipeName").toString();
        ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
        if (!server)
            return DoneResult::Error;

        server->m_runningSessions.insert(id, pipe);
        if (onStarted)
            onStarted(id);
        return DoneResult::Success;
    };
    const auto onDone = [bauhausSuite, onFailed, replyStorage](DoneWith result) {
        if (result == DoneWith::Success)
            return;
        qCDebug(log) << "error starting pluginar session for" << bauhausSuite;
        qCDebug(log) << "resp:" << *replyStorage;
        if (onFailed)
            onFailed();
    };

    qCDebug(log) << "start pluginar session for" << bauhausSuite.toUserOutput();
    const Group recipe {
        replyStorage,
        sendQueryRecipe(replyStorage, server->m_arSessionStartRel, bauhausSuite),
        QSyncTask(onSuccess),
        onGroupDone(onDone)
    };
    GlobalTaskTree::start(recipe);
}

static void deserializeFileView(QPromise<Result<Dto::FileViewDto>> &promise,
                                const QByteArray &json)
{
    if (promise.isCanceled())
        return;
    Result<Dto::FileViewDto> result = Dto::FileViewDto::deserializeExpected(json);
    if (!result) {
        qCDebug(log) << "error deserializing fileview dto" << result.error();
        return;
    }
    qCDebug(log) << "deserialize succeeded" << result->fileName << result->lineMarkers.size();
    promise.addResult(result);
}

void requestArSessionFinish(const Utils::FilePath &bauhausSuite, int sessionId, bool abort)
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server)
        return;

    QJsonObject jsonObj;
    jsonObj.insert("arSessionId", sessionId);
    jsonObj.insert("abort", abort);
    const QJsonDocument json{jsonObj};

    const Storage<QByteArray> replyStorage;
    const auto onDeserializeSetup = [bauhausSuite, sessionId, replyStorage](QTaskTree &task) {
        qCDebug(log) << "pluginar session finished for" << bauhausSuite;
        // handle the response....
        ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
        if (!server)
            return SetupResult::StopWithError;

        server->m_runningSessions.remove(sessionId);
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(*replyStorage, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            qCDebug(log) << "parse error (session start reply)";
            return SetupResult::StopWithError;
        }
        qCDebug(log) << *replyStorage;
        const QJsonArray filesInfo = json.object().value("filesInfo").toArray();
        QByteArrayList jsonList;
        for (const QJsonValue &v : filesInfo)
            jsonList.append(QJsonDocument{v.toObject()}.toJson());
        const ListIterator iterator(jsonList);

        const auto onSetup = [iterator](Async<Result<Dto::FileViewDto>> &async) {
            async.setConcurrentCallData(deserializeFileView, *iterator);
        };
        const auto onDone = [bauhausSuite](const Async<Result<Dto::FileViewDto>> &async) {
            const QList<Result<Dto::FileViewDto>> &results = async.results();
            for (const Result<Dto::FileViewDto> &result : results) {
                handleIssuesForFile(*result, FilePath::fromUserInput(result->fileName),
                                    bauhausSuite, LineMarkerType::SFA, {}, {});
            }
        };
        const Group deserializeRecipe = For (iterator) >> Do {
            ParallelLimit(4),
            AsyncTask<Result<Dto::FileViewDto>>(onSetup, onDone, CallDoneFlag::OnSuccess)
        };

        task.setRecipe(deserializeRecipe);
        return SetupResult::Continue;
    };
    const auto onError = [bauhausSuite, sessionId, replyStorage] {
        qCDebug(log) << "error finishing pluginar session" << sessionId << "for" << bauhausSuite;
        qCDebug(log) << "resp:" << *replyStorage;
    };

    qCDebug(log) << "finish pluginar session" << bauhausSuite << "session" << sessionId;
    const Group recipe {
        replyStorage,
        If (sendQueryRecipe(replyStorage, server->m_arSessionFinishRel, bauhausSuite, json)) >> Then {
            QTaskTreeTask(onDeserializeSetup)
        } >> Else {
            QSyncTask(onError) && DoneResult::Error
        }
    };
    GlobalTaskTree::start(recipe);
}

static ExecutableItem disposeIssuesRecipe(const FilePath &bauhausSuite,
                                          const QList<qint64> &issues)
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server)
        return errorItem;

    QJsonArray jsonArray;
    for (long issue : issues)
        jsonArray.append(double(issue));
    const QJsonDocument json{jsonArray};

    const Storage<QByteArray> replyStorage;
    const auto onSuccess = [bauhausSuite, replyStorage] {
        qCDebug(log) << "dispose of issues succeeded" << bauhausSuite;
        // handle the response....
        qCDebug(log) << *replyStorage;
    };
    const auto onError = [bauhausSuite, replyStorage] {
        qCDebug(log) << "error disposing issues for" << bauhausSuite;
        qCDebug(log) << "resp:" << *replyStorage;
    };

    qCDebug(log) << "dispose issues for" << bauhausSuite << "session";
    return Group {
        replyStorage,
        If (sendQueryRecipe(replyStorage, server->m_disposeIssuesRel, bauhausSuite, json)) >> Then {
            QSyncTask(onSuccess)
        } >> Else {
            QSyncTask(onError) && DoneResult::Error
        }
    };
}

void requestIssuesDisposal(const Utils::FilePath &bauhausSuite, const QList<qint64> &issues)
{
    if (!s_pluginArServers->runningServer(bauhausSuite))
        return;

    GlobalTaskTree::start({disposeIssuesRecipe(bauhausSuite, issues)});
}

QString pluginArPipeOut(const Utils::FilePath &bauhausSuite, int sessionId)
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server)
        return {};

    return server->m_runningSessions.value(sessionId);
}

void fetchIssueInfoFromPluginAr(const Utils::FilePath &bauhausSuite, const QString &relativeIssue)
{
    const ArServerData *server = s_pluginArServers->runningServer(bauhausSuite);
    if (!server)
        return;

    const QString issueProperties{relativeIssue + "/properties"};
    const QUrl url = server->m_serverUrl.resolved(issueProperties);
    const QByteArray auth = basicAuth(server->m_accessSecret);

    qCDebug(log) << "URL" << url;
    const auto onNetworkQuerySetup = [url, auth](QNetworkReplyWrapper &query) {
        setupQuery(query, url, auth, {});
        query.setOperation(QNetworkAccessManager::GetOperation);
    };
    auto onSuccess = [](const QByteArray &reply) {
        updateIssueDetails(QString::fromUtf8(fixIssueDetailsHtml(reply)), {}, std::nullopt);
        showIssueDetails();
        return DoneResult::Success;
    };

    const auto onNetworkQueryDone
            = [bauhausSuite, onSuccess]
            (const QNetworkReplyWrapper &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray contentType = contentTypeFromRawHeader(reply);
        if (doneWith == DoneWith::Success && status == 200 && contentType == s_htmlContentType)
            return onSuccess(reply->readAll());
        qCDebug(log) << "fetching issue failed" << reply->errorString() << reply->error() << status;
        return DoneResult::Error;
    };

    GlobalTaskTree::start({QNetworkReplyWrapperTask(onNetworkQuerySetup, onNetworkQueryDone)});
}

} // namespace Axivion::Internal

#include "pluginarserver.moc"
