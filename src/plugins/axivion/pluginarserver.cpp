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

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QAbstractTaskTreeRunner>

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

namespace Axivion::Internal {

Q_LOGGING_CATEGORY(log, "qtc.axivion.pluginarserver", QtWarningMsg)

class PluginArServer;

class ArServerData
{
public:
    QByteArray m_accessSecret;
    QUrl m_serverUrl;
    QString m_disposeIssuesRel;
    QString m_arSessionStartRel;
    QString m_arSessionFinishRel;
    QHash<int, QString> m_runningSessions;
};

static QHash<FilePath, ArServerData> s_arServers;
static GuardedObject<QMappedTaskTreeRunner<FilePath>> s_arServerRunner;
static GuardedObject<QParallelTaskTreeRunner> s_networkRunner;

class ShutdownNotifier : public QObject
{
    Q_OBJECT

signals:
    void shutdownRequested(const FilePath &bauhausSuite);
};

static GuardedObject<ShutdownNotifier> s_shutdownNotifier;

enum class State { Starting, HttpModeRequested, Initialized };

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
    OnServerStarted m_onRunning;
    QByteArray m_buffer = {};
    int m_msgCounter = 0;
    State m_state = State::Starting;
};

static void writeMessage(Process *process, const QJsonDocument &msg)
{
    process->writeRaw(msg.toJson(QJsonDocument::Compact));
    process->writeRaw(QByteArray{1, 0x17});
}

static void handleMessage(const QByteArray &msg, const FilePath &bauhausSuite,
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

    ArServerData &serverData = s_arServers[bauhausSuite];

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
            QMetaObject::invokeMethod(process,
                [bauhausSuite] { shutdownPluginArServer(bauhausSuite); }, Qt::QueuedConnection);
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
        if (data->m_onRunning)
            data->m_onRunning();
    } break;
    case State::Initialized:
        const int inReplyTo = obj.value("inReplyTo").toInt();
        QTC_CHECK(inReplyTo == data->m_msgCounter); // should we handle this?
        qCDebug(log) << "I" << msg; // json - atm we expect a response ok for clean shutdown..
        break;
    }
}

static void handleOutput(const FilePath &bauhausSuite, ArServerProcessData *data, Process *process)
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

void startPluginArServer(const FilePath &bauhausSuite, const OnServerStarted &onRunning,
                         const OnServerStartFailed &onFailed)
{
    if (s_arServerRunner->isKeyRunning(bauhausSuite)) {
        if (onRunning)
            onRunning();
        return;
    }

    const Storage<ArServerProcessData> storage{onRunning};

    const auto onSetup = [bauhausSuite, storage](Process &process) {
        Environment env = Environment::systemEnvironment();
        if (!bauhausSuite.isEmpty())
            env.prependOrSetPath(bauhausSuite.pathAppended("bin"));
        if (!settings().javaHome().isEmpty())
            env.set("JAVA_HOME", settings().javaHome().toUserOutput());
        if (!settings().bauhausPython().isEmpty())
            env.set("BAUHAUS_PYTHON", settings().bauhausPython().toUserOutput());
        env.set("PYTHON_IO_ENCODING", "utf-8:replace");
        env.set("AXIVION_USER_AGENT", QString::fromUtf8(axivionUserAgent()));

        CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                      : CommandLine{"/bin/sh", {"-c"}};
        cmd.addCommandLineAsArgs({FilePath::fromString("pluginARServer").withExecutableSuffix(), {}},
                                 CommandLine::Raw);
        process.setCommand(cmd);

        process.setEnvironment(env);
        process.setProcessMode(ProcessMode::Writer);
        QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                         [bauhausSuite, data = storage.activeStorage(), process = &process] {
            handleOutput(bauhausSuite, data, process);
        });
        QObject::connect(s_shutdownNotifier.get(), &ShutdownNotifier::shutdownRequested, &process,
                         [bauhausSuite, data = storage.activeStorage(), process = &process]
                         (const FilePath &suite) {
            if (bauhausSuite == suite)
                writeMessage(process, msgToJson(++data->m_msgCounter, 0, "Command: Shutdown", {}));
        });
    };
    const auto onDone = [bauhausSuite, storage, onFailed](const Process &proc) {
        if (storage->m_state == State::Starting && proc.error() == QProcess::UnknownError) {
            const QString error = Tr::tr("PluginArServer stopped with unknown error.").append('\n')
                    .append(proc.cleanedStdErr());
            writeError(error);
            if (onFailed)
                onFailed();
        }
        s_arServers.remove(bauhausSuite);
    };

    const Group recipe {
        storage,
        ProcessTask(onSetup, onDone)
    };

    s_arServers.insert(bauhausSuite, {});
    s_arServerRunner->start(bauhausSuite, recipe);
}

void shutdownPluginArServer(const FilePath &bauhausSuite)
{
    s_arServers.remove(bauhausSuite);
    s_arServerRunner->resetKey(bauhausSuite);
}

void cleanShutdownPluginArServer(const FilePath &bauhausSuite)
{
    if (s_shutdownNotifier.get())
        emit s_shutdownNotifier->shutdownRequested(bauhausSuite);
}

void shutdownAllPluginArServers()
{
    s_arServers.clear();
    s_arServerRunner->reset();
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

static void sendQuery(const QString &relative,
                      const Utils::FilePath &bauhausSuite,
                      const QJsonDocument &body,
                      const OnDoneCallback &onSuccess,
                      const OnDoneCallback &onError)
{
    const auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return;

    const QUrl serverUrl = it->m_serverUrl;
    QTC_ASSERT(!serverUrl.isEmpty(), return);
    const QUrl url = serverUrl.resolved(relative);
    const QByteArray auth = basicAuth(it->m_accessSecret);

    qCDebug(log) << "URL" << url;
    const auto onNetworkQuerySetup = [url, auth, body](QNetworkReplyWrapper &query) {
        setupQuery(query, url, auth, body);
    };

    const auto onNetworkQueryDone
            = [bauhausSuite, onSuccess, onError]
            (const QNetworkReplyWrapper &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray contentType = contentTypeFromRawHeader(reply);
        if (doneWith == DoneWith::Success && status == 200 && contentType == s_jsonContentType)
            return onSuccess(reply->readAll());
        if (doneWith == DoneWith::Success && status > 200 && status < 300) // e.g. issue disposal
            return onSuccess(reply->readAll());
        return onError(reply->readAll());
    };

    s_networkRunner->start({QNetworkReplyWrapperTask(onNetworkQuerySetup, onNetworkQueryDone)});
}

void requestArSessionStart(const FilePath &bauhausSuite, const OnSessionStarted &onStarted)
{
    const auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return;

    const auto onSuccess = [bauhausSuite, onStarted](const QByteArray &reply) {
        qCDebug(log) << "pluginar session started for" << bauhausSuite;
        // handle the response....
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(reply, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            qCDebug(log) << "parse error (session start reply)";
            return DoneResult::Error;
        }
        const QJsonObject jsonObj = json.object();
        const int id = jsonObj.value("arSessionId").toInt();
        const QString pipe = jsonObj.value("pipeName").toString();
        const auto it = s_arServers.find(bauhausSuite);
        if (it == s_arServers.end())
            return DoneResult::Error;

        it->m_runningSessions.insert(id, pipe);
        if (onStarted)
            onStarted(id);
        return DoneResult::Success;
    };
    const auto onError = [bauhausSuite](const QByteArray &reply) {
        qCDebug(log) << "error starting pluginar session for" << bauhausSuite;
        qCDebug(log) << "resp:" << reply;
        return DoneResult::Error;
    };

    qCDebug(log) << "start pluginar session for" << bauhausSuite.toUserOutput();
    sendQuery(it->m_arSessionStartRel, bauhausSuite, {}, onSuccess, onError);
}

void requestArSessionFinish(const Utils::FilePath &bauhausSuite, int sessionId, bool abort)
{
    const auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return;

    QJsonObject jsonObj;
    jsonObj.insert("arSessionId", sessionId);
    jsonObj.insert("abort", abort);
    const QJsonDocument json{jsonObj};

    const auto onSuccess = [bauhausSuite, sessionId](const QByteArray &reply) {
        qCDebug(log) << "pluginar session finished for" << bauhausSuite;
        // handle the response....
        const auto it = s_arServers.find(bauhausSuite);
        if (it == s_arServers.end())
            return DoneResult::Error;

        it->m_runningSessions.remove(sessionId);
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(reply, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            qCDebug(log) << "parse error (session start reply)";
            return DoneResult::Error;
        }
        qCDebug(log) << reply;
        // for now just quick and dirty
        const QJsonArray filesInfo = json.object().value("filesInfo").toArray();
        QTC_ASSERT(filesInfo.size() == 1, return DoneResult::Error);
        const QJsonObject fileView = filesInfo.first().toObject();
        Result<Dto::FileViewDto> result
                = Dto::FileViewDto::deserializeExpected(QJsonDocument{fileView}.toJson());
        if (!result) {
            qCDebug(log) << "error deserializing filewview dto" << result.error();
            return DoneResult::Error;
        }
        qCDebug(log) << "deserialize succeeded" << result->fileName << result->lineMarkers.size();
        // handle line markers for this file
        handleIssuesForFile(*result, FilePath::fromUserInput(result->fileName), bauhausSuite);
        return DoneResult::Success;
    };
    const auto onError = [bauhausSuite, sessionId](const QByteArray &reply) {
        qCDebug(log) << "error finishing pluginar session" << sessionId << "for" << bauhausSuite;
        qCDebug(log) << "resp:" << reply;
        return DoneResult::Error;
    };

    qCDebug(log) << "finish pluginar session" << bauhausSuite << "session" << sessionId;
    sendQuery(it->m_arSessionFinishRel, bauhausSuite, json, onSuccess, onError);
}

void requestIssuesDisposal(const Utils::FilePath &bauhausSuite, const QList<qint64> &issues)
{
    const auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return;

    QJsonArray jsonArray;
    for (long issue : issues)
        jsonArray.append(double(issue));
    const QJsonDocument json{jsonArray};

    const auto onSuccess = [bauhausSuite](const QByteArray &reply) {
        qCDebug(log) << "dispose of issues succeeded" << bauhausSuite;
        // handle the response....
        qCDebug(log) << reply;
        return DoneResult::Success;
    };
    const auto onError = [bauhausSuite](const QByteArray &reply) {
        qCDebug(log) << "error disposing issues for" << bauhausSuite;
        qCDebug(log) << "resp:" << reply;
        return DoneResult::Error;
    };

    qCDebug(log) << "dispose issues for" << bauhausSuite << "session";
    sendQuery(it->m_disposeIssuesRel, bauhausSuite, json, onSuccess, onError);
}

QString pluginArPipeOut(const Utils::FilePath &bauhausSuite, int sessionId)
{
    const auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return {};

    return it->m_runningSessions.value(sessionId);
}

void fetchIssueInfoFromPluginAr(const Utils::FilePath &bauhausSuite, const QString &relativeIssue)
{
    auto it = s_arServers.constFind(bauhausSuite);
    if (it == s_arServers.constEnd() || it->m_serverUrl.isEmpty())
        return;

    const QUrl serverUrl = it->m_serverUrl;
    const QString issueProperties{relativeIssue + "/properties"};
    const QUrl url = serverUrl.resolved(issueProperties);
    const QByteArray auth = basicAuth(it->m_accessSecret);

    qCDebug(log) << "URL" << url;
    const auto onNetworkQuerySetup = [url, auth](QNetworkReplyWrapper &query) {
        setupQuery(query, url, auth, {});
        query.setOperation(QNetworkAccessManager::GetOperation);
    };
    auto onSuccess = [](const QByteArray &reply) {
        updateIssueDetails(QString::fromUtf8(fixIssueDetailsHtml(reply)), {});
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

    s_networkRunner->start({QNetworkReplyWrapperTask(onNetworkQuerySetup, onNetworkQueryDone)});
}

} // namespace Axivion::Internal

#include "pluginarserver.moc"
