// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pluginarserver.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <coreplugin/messagemanager.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QAbstractTaskTreeRunner>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMap>

using namespace Core;
using namespace Utils;
using namespace QtTaskTree;

namespace Axivion::Internal {

Q_LOGGING_CATEGORY(log, "qtc.axivion.pluginarserver", QtWarningMsg)

class PluginArServer;

// running pluginarservers per bauhaus suite
static QMap<FilePath, PluginArServer *> s_pluginArServers;

QSingleTaskTreeRunner *taskTreeRunner()
{
    static QSingleTaskTreeRunner singleTaskTreeRunner;
    return &singleTaskTreeRunner;
}

class PluginArServer
{
public:
    ~PluginArServer();
    static bool setupNewPluginArServer(const FilePath &bauhausSuite, const CallbackFunc &onRunning);
    void sendCleanShutdown();

    QUrl serverUrl() const;
    QString startSessionRel() const;
    QString finishSessionRel() const;
    QString disposeIssuesRel() const;
    QByteArray basicAuth() const;

    void addStartedArSession(int id, const QString &pipeName);
    void removeRunningArSession(int id);
    QList<int> runningSessions() const;
    QString pipeNameForSession(int id) const;

    void triggerOnRunning() { if (m_onRunning) m_onRunning(); }

private:
    enum class State { Starting, HttpModeRequested, Initialized };
    explicit PluginArServer(const FilePath &bauhausSuite, const CallbackFunc &onRunning);
    void handleOutput();
    void handleMessage(const QByteArray &msg);
    void writeMessage(const QJsonDocument &msg);

    Process *m_process = nullptr;
    FilePath m_bauhausSuite;
    QByteArray m_buffer;
    QByteArray m_accessSecret;
    QUrl m_serverUrl;
    QString m_disposeIssuesRel;
    QString m_arSessionStartRel;
    QString m_arSessionFinishRel;
    QMap<int, QString> m_runningSessions;
    CallbackFunc m_onRunning;
    int m_msgCounter = 0;

    State m_state = State::Starting;
};


PluginArServer::PluginArServer(const FilePath &bauhausSuite, const CallbackFunc &onRunning)
    : m_bauhausSuite(bauhausSuite)
    , m_onRunning(onRunning)
{
    QTC_CHECK(!m_bauhausSuite.isEmpty());
    QTC_CHECK(!s_pluginArServers.contains(m_bauhausSuite));
    s_pluginArServers.insert(m_bauhausSuite, this);
    Environment env = Environment::systemEnvironment();
    if (!bauhausSuite.isEmpty())
        env.prependOrSetPath(bauhausSuite.pathAppended("bin"));
    if (!settings().javaHome().isEmpty())
        env.set("JAVA_HOME", settings().javaHome().toUserOutput());
    if (!settings().bauhausPython().isEmpty())
        env.set("BAUHAUS_PYTHON", settings().bauhausPython().toUserOutput());
    env.set("PYTHON_IO_ENCODING", "utf-8:replace");
    const QString userAgent = QString("Axivion" + QCoreApplication::applicationName()
                                      + "Plugin/" + QCoreApplication::applicationVersion());
    env.set("AXIVION_USER_AGENT", userAgent);

    m_process = new Process;
    CommandLine cmd = HostOsInfo::isWindowsHost() ? CommandLine{"cmd", {"/c"}}
                                                  : CommandLine{"/bin/sh", {"-c"}};
    cmd.addCommandLineAsArgs({FilePath::fromString("pluginARServer").withExecutableSuffix(), {}},
                             CommandLine::Raw);
    m_process->setCommand(cmd);

    m_process->setEnvironment(env);
    m_process->setProcessMode(ProcessMode::Writer);
    QObject::connect(m_process, &Process::readyReadStandardOutput,
                     m_process, [this] { handleOutput(); });
    QObject::connect(m_process, &Process::done, m_process,
                     [this] { shutdownPluginArServer(m_bauhausSuite); }, Qt::QueuedConnection);
    // TODO should we handle this somehow? includes FileNotFound,
    //      issues with python / java, not existing bauhaus config
    m_process->setStdErrLineCallback([](const QString &line) { qCInfo(log) << "E" << line; });
    qCDebug(log) << "Starting PluginArServer" << cmd.toUserOutput();
    m_process->start();
}

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

void PluginArServer::handleOutput()
{
    m_buffer.append(m_process->readAllRawStandardOutput());
    int endOfFrame = m_buffer.indexOf(0x17);
    if (endOfFrame == -1)
        return;

    qsizetype start = 0;
    do {
        qsizetype eoMessage = m_buffer.indexOf(0x10, start);
        if (eoMessage == -1)
            break;
        handleMessage(m_buffer.mid(start, eoMessage - start));
        start = eoMessage + 1;
    } while (true);
    if (start == 0) {
        handleMessage(m_buffer.mid(start, endOfFrame));
    }
    // cut processed output
    m_buffer = m_buffer.mid(endOfFrame + 1);
    // should we do another round?
}

static void writeError(const QString &error)
{
    static const QString prefix{"[Axivion|PluginArServer] "};
    MessageManager::writeFlashing(prefix + error);
}

void PluginArServer::handleMessage(const QByteArray &msg)
{
    qCInfo(log) << "handling message:" << msg;
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(msg, &error);
    if (error.error != QJsonParseError::NoError) {
        writeError(Tr::tr("Failed to parse json '%1': %2")
                   .arg(QString::fromUtf8(msg)).arg(error.error));
        return;
    }
    if (!json.isObject()) {
        writeError(Tr::tr("Not a json object: %1").arg(QString::fromUtf8(msg)));
        return;
    }
    const QJsonObject obj = json.object();

    switch (m_state) {
    case State::Starting: {
        if (!obj.contains("serverVersion") || !obj.contains("accessSecret")) {
            writeError(Tr::tr("Unexpected response when starting PluginArServer."));
            return;
        }
        m_accessSecret = obj.value("accessSecret").toString().toUtf8();
        const QVariantMap args{{"serveIssues", true}};
        const QJsonDocument message = msgToJson(++m_msgCounter, 0, "Command: Start HTTP Server",
                                                args);
        m_state = State::HttpModeRequested;
        qCDebug(log) << "requesting https server start";
        writeMessage(message);
    } break;
    case State::HttpModeRequested: {
        int inReplyTo = obj.value("inReplyTo").toInt();
        QTC_CHECK(inReplyTo == m_msgCounter); // should we handle this?
        if (obj.value("subject").toString() != "Response: OK") {
            writeError(Tr::tr("Unexpected response for HTTP Server start request."));
            QMetaObject::invokeMethod(
                        m_process, [this] { shutdownPluginArServer(m_bauhausSuite); }, Qt::QueuedConnection);
            return;
        }
        const QVariantMap args = obj.value("arguments").toVariant().toMap();
        m_serverUrl = args.value("httpServerUri").toString();
        m_disposeIssuesRel = args.value("httpServerDisposeIssuesRelativeUri").toString();
        m_arSessionStartRel = args.value("httpServerARSessionStartRelativeUri").toString();
        m_arSessionFinishRel = args.value("httpServerARSessionFinishRelativeUri").toString();
        QTC_CHECK(!m_serverUrl.isEmpty());
        // QTC_CHECK(!m_disposeIssuesRel.isEmpty()); // relevant only if serverIssues=true arg
        QTC_CHECK(!m_arSessionStartRel.isEmpty());
        QTC_CHECK(!m_arSessionFinishRel.isEmpty());
        qCDebug(log) << "server started at" << m_serverUrl;
        m_state = State::Initialized;
        if (m_onRunning)
            m_onRunning();
    } break;
    case State::Initialized:
        int inReplyTo = obj.value("inReplyTo").toInt();
        QTC_CHECK(inReplyTo == m_msgCounter); // should we handle this?
        qCDebug(log) << "I" << msg; // json - atm we expect a response ok for clean shutdown..
        break;
    }
}

void PluginArServer::writeMessage(const QJsonDocument &msg)
{
    QTC_ASSERT(m_process, return);
    m_process->writeRaw(msg.toJson(QJsonDocument::Compact));
    m_process->writeRaw(QByteArray{1, 0x17});
}

PluginArServer::~PluginArServer()
{
    m_process->close();
    delete m_process;
    s_pluginArServers.remove(m_bauhausSuite);
}

bool PluginArServer::setupNewPluginArServer(const FilePath &bauhausSuite,
                                            const CallbackFunc &onRunning)
{
    QTC_ASSERT(!s_pluginArServers.contains(bauhausSuite), return false);
    (void)new PluginArServer(bauhausSuite, onRunning);
    return true;
}

void PluginArServer::sendCleanShutdown()
{
    qCDebug(log) << "requesting clean shutdown";
    writeMessage(msgToJson(++m_msgCounter, 0, "Command: Shutdown", {}));
}

QUrl PluginArServer::serverUrl() const
{
    if (m_state != State::Initialized) {
        qCDebug(log) << "cannot get server url of uninitialized pluginarserver";
        return {};
    }
    return m_serverUrl;
}

QString PluginArServer::startSessionRel() const
{
    return m_arSessionStartRel;
}

QString PluginArServer::finishSessionRel() const
{
    return m_arSessionFinishRel;
}

QString PluginArServer::disposeIssuesRel() const
{
    return m_disposeIssuesRel;
}

QByteArray PluginArServer::basicAuth() const
{
    if (m_state != State::Initialized) {
        qCDebug(log) << "cannot authenticate with uninitialized pluginarserver";
        return {};
    }
    return "Basic " + QByteArray(m_accessSecret).prepend(':').toBase64();
}

void PluginArServer::addStartedArSession(int id, const QString &pipeName)
{
    if (m_state != State::Initialized) {
        qCDebug(log) << "cannot add started session with uninitialized pluginarserver";
        return;
    }
    QTC_ASSERT(!m_runningSessions.contains(id), return);
    m_runningSessions.insert(id, pipeName);
}

void PluginArServer::removeRunningArSession(int id)
{
    if (m_state != State::Initialized) {
        qCDebug(log) << "cannot add started session with uninitialized pluginarserver";
        return;
    }
    QTC_ASSERT(m_runningSessions.contains(id), return);
    m_runningSessions.remove(id);
}

QList<int> PluginArServer::runningSessions() const
{
    if (m_state != State::Initialized) {
        qCDebug(log) << "cannot get running sessions with uninitialized pluginarserver";
        return {};
    }
    return m_runningSessions.keys();
}

QString PluginArServer::pipeNameForSession(int session) const
{
    return m_runningSessions.value(session);
}

void startPluginArServer(const FilePath &bauhausSuite, const CallbackFunc &onRunning)
{
    auto server = s_pluginArServers.value(bauhausSuite);
    if (server) {
        server->triggerOnRunning();
        return;
    }

    PluginArServer::setupNewPluginArServer(bauhausSuite, onRunning);
}

void shutdownPluginArServer(const FilePath &bauhausSuite)
{
    if (!s_pluginArServers.contains(bauhausSuite))
        return;
    qCDebug(log) << "deleting instance for" << bauhausSuite;
    delete s_pluginArServers.take(bauhausSuite); // for now...
}

void cleanShutdownPluginArServer(const FilePath &bauhausSuite)
{
    if (auto pluginArServer = s_pluginArServers.value(bauhausSuite))
        pluginArServer->sendCleanShutdown();
}

void shutdownAllPluginArServers()
{
    QMap<FilePath, PluginArServer *> copy = s_pluginArServers;
    s_pluginArServers.clear();
    qDeleteAll(copy);
}

static void setupQuery(QNetworkReplyWrapper &query, const QUrl &url, const QByteArray &auth,
                       const QJsonDocument &body)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", auth);
    request.setRawHeader("Content-Type", "application/json");
    query.setRequest(request);
    query.setOperation(QNetworkAccessManager::PostOperation);
    if (!body.isNull())
        query.setData(body.toJson(QJsonDocument::Compact));
    query.setNetworkAccessManager(axivionNetworkManager());
}

using OnDoneCallback = std::function<DoneResult(const QByteArray &)>;

template<typename T, typename Base>
static void sendQuery(T (Base::* member)()const,
                      const Utils::FilePath &bauhausSuite,
                      const QJsonDocument &body,
                      const OnDoneCallback &onSuccess,
                      const OnDoneCallback &onError)
{
    PluginArServer *pluginArServer = s_pluginArServers.value(bauhausSuite);
    QTC_ASSERT(pluginArServer, return);
    const QUrl serverUrl = pluginArServer->serverUrl();
    QTC_ASSERT(!serverUrl.isEmpty(), return);
    const QUrl url = serverUrl.resolved(std::mem_fn(member)(pluginArServer));
    const QByteArray auth = pluginArServer->basicAuth();

    qCDebug(log) << "URL" << url;
    const auto onNetworkQuerySetup = [url, auth, body](QNetworkReplyWrapper &query) {
        setupQuery(query, url, auth, body);
    };

    const auto onNetworkQueryDone
            = [bauhausSuite, onSuccess, onError]
            (const QNetworkReplyWrapper &query, DoneWith doneWith) {
        QNetworkReply *reply = query.reply();
        // const QNetworkReply::NetworkError error = reply->error();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString contentType = reply->header(QNetworkRequest::ContentTypeHeader)
                .toString().split(";").constFirst().trimmed().toLower();
        if (doneWith == DoneWith::Success && status == 200 && contentType == "application/json")
            return onSuccess(reply->readAll());
        return onError(reply->readAll());
    };

    taskTreeRunner()->start({QNetworkReplyWrapperTask(onNetworkQuerySetup, onNetworkQueryDone)});
}

void requestArSessionStart(const Utils::FilePath &bauhausSuite,
                           const SessionCallbackFunc &onStarted)
{
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
        PluginArServer *pluginAr = s_pluginArServers.value(bauhausSuite);
        QTC_ASSERT(pluginAr, return DoneResult::Error);
        pluginAr->addStartedArSession(id, pipe);
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
    sendQuery(&PluginArServer::startSessionRel, bauhausSuite, {}, onSuccess, onError);
}

void requestArSessionFinish(const Utils::FilePath &bauhausSuite, int sessionId, bool abort)
{
    QJsonObject jsonObj;
    jsonObj.insert("arSessionId", sessionId);
    jsonObj.insert("abort", abort);
    const QJsonDocument json{jsonObj};

    const auto onSuccess = [bauhausSuite, sessionId](const QByteArray &reply) {
        qCDebug(log) << "pluginar session finished for" << bauhausSuite;
        // handle the response....
        if (auto pluginAr = s_pluginArServers.value(bauhausSuite))
            pluginAr->removeRunningArSession(sessionId);
        QJsonParseError error;
        // we have a FileViewDto here.. but for now just simple
        QJsonDocument json = QJsonDocument::fromJson(reply, &error);
        if (error.error != QJsonParseError::NoError || !json.isObject()) {
            qCDebug(log) << "parse error (session start reply)";
            return DoneResult::Error;
        }
        qCDebug(log) << reply;
        return DoneResult::Success;
    };
    const auto onError = [bauhausSuite, sessionId](const QByteArray &reply) {
        qCDebug(log) << "error finishing pluginar session" << sessionId << "for" << bauhausSuite;
        qCDebug(log) << "resp:" << reply;
        return DoneResult::Error;
    };

    qCDebug(log) << "finish pluginar session" << bauhausSuite << "session" << sessionId;
    sendQuery(&PluginArServer::finishSessionRel, bauhausSuite, json, onSuccess, onError);
}

void requestIssuesDisposal(const Utils::FilePath &bauhausSuite, int sessionId,
                           const QList<long> &issues)
{
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
    const auto onError = [bauhausSuite, sessionId](const QByteArray &reply) {
        qCDebug(log) << "error disposing issues" << sessionId << "for" << bauhausSuite;
        qCDebug(log) << "resp:" << reply;
        return DoneResult::Error;
    };

    qCDebug(log) << "dispose issues for" << bauhausSuite << "session" << sessionId;
    sendQuery(&PluginArServer::disposeIssuesRel, bauhausSuite, json, onSuccess, onError);
}

QString pluginArPipeOut(const Utils::FilePath &bauhausSuite, int sessionId)
{
    auto server = s_pluginArServers.find(bauhausSuite);
    QTC_ASSERT(server != s_pluginArServers.end(), return {});
    return (*server)->pipeNameForSession(sessionId);
}

} // namespace Axivion::Internal
