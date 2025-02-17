// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dvconnector.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <qmldesigner/qmldesignerconstants.h>
#include <qmldesigner/qmldesignerplugin.h>

#include <coreplugin/icore.h>

#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QWebEngineCookieStore>

namespace QmlDesigner::DesignViewer {
Q_LOGGING_CATEGORY(deploymentPluginLog, "qtc.designer.deploymentPlugin", QtWarningMsg)

namespace DVEndpoints {
using namespace Qt::Literals;
#ifdef QDS_DESIGNVIEWER_USE_STAGING
constexpr auto serviceUrl = "https://api-designviewer-staging.qt.io"_L1;
#else
constexpr auto serviceUrl = "https://api-designviewer.qt.io"_L1;
#endif
constexpr auto project = "/api/v2/project"_L1;
constexpr auto projectThumbnail = "/api/v2/project/image"_L1;
constexpr auto share = "/api/v2/share"_L1;
constexpr auto shareThumbnail = "/api/v2/share/image"_L1;
constexpr auto login = "/api/v2/auth/login"_L1;
constexpr auto logout = "/api/v2/auth/logout"_L1;
constexpr auto userInfo = "/api/v2/auth/userinfo"_L1;
constexpr auto refreshToken = "/api/v2/auth/token"_L1;
}; // namespace DVEndpoints

CustomWebEnginePage::CustomWebEnginePage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{}

void CustomWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                                   const QString &message,
                                                   int lineNumber,
                                                   const QString &sourceID)
{
    // Suppress JavaScript console messages
    if (level == QWebEnginePage::JavaScriptConsoleMessageLevel::InfoMessageLevel
        || level == QWebEnginePage::JavaScriptConsoleMessageLevel::WarningMessageLevel
        || level == QWebEnginePage::JavaScriptConsoleMessageLevel::ErrorMessageLevel) {
        return;
    }
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}

CustomCookieJar::CustomCookieJar(QObject *parent, const QString &cookieFilePath)
    : QNetworkCookieJar(parent)
    , m_cookieFilePath(cookieFilePath)
{}

CustomCookieJar::~CustomCookieJar()
{
    saveCookies();
}

void CustomCookieJar::loadCookies()
{
    QFile file(m_cookieFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(deploymentPluginLog) << "Failed to open cookie file for reading:" << m_cookieFilePath;
        return;
    }

    QList<QNetworkCookie> cookies;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QList<QNetworkCookie> lineCookies = QNetworkCookie::parseCookies(line.toUtf8());
        cookies.append(lineCookies);
        qCDebug(deploymentPluginLog) << "Loaded cookie:" << line;
    }
    setAllCookies(cookies);
    file.close();
}

void CustomCookieJar::saveCookies()
{
    QFile file(m_cookieFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(deploymentPluginLog) << "Failed to open cookie file for writing:" << m_cookieFilePath;
        return;
    }

    QTextStream out(&file);
    QList<QNetworkCookie> cookies = allCookies();
    for (const QNetworkCookie &cookie : cookies) {
        out << cookie.toRawForm() << "\n";
    }
    file.close();
}

void CustomCookieJar::clearCookies()
{
    setAllCookies(QList<QNetworkCookie>());
    QFile file(m_cookieFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCDebug(deploymentPluginLog) << "Failed to open cookie file for writing:" << m_cookieFilePath;
        return;
    }

    file.close();
}

DVConnector::DVConnector(QObject *parent)
    : QObject{parent}
    , m_isWebViewerVisible(false)
    , m_connectorStatus(ConnectorStatus::NotLoggedIn)
{
    QWebEngineProfile *webEngineProfile = new QWebEngineProfile("DesignViewer");
    webEngineProfile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    m_webEnginePage = new CustomWebEnginePage(webEngineProfile);
    webEngineProfile->setParent(m_webEnginePage);

    m_webEngineView = new QWebEngineView(m_webEnginePage, Core::ICore::instance()->dialogParent());
    m_webEngineView->resize(1024, 750);
    m_webEngineView->setWindowFlag(Qt::Dialog);
    m_webEngineView->installEventFilter(this);
    m_webEngineView->hide();

    m_networkCookieJar.reset(
        new CustomCookieJar(this, webEngineProfile->persistentStoragePath() + "/dv_cookies.txt"));
    m_networkAccessManager.reset(new QNetworkAccessManager(this));
    m_networkAccessManager->setCookieJar(m_networkCookieJar.data());
    m_networkCookieJar->loadCookies();

    connect(webEngineProfile->cookieStore(),
            &QWebEngineCookieStore::cookieAdded,
            this,
            [&](const QNetworkCookie &cookie) {
                const QByteArray cookieName = cookie.name();
                qCDebug(deploymentPluginLog) << "Login Cookie:" << cookieName << cookie.value();
                if (cookieName != "jwt" && cookieName != "jwt_refresh")
                    return;
                m_networkAccessManager->cookieJar()->insertCookie(cookie);
                m_networkCookieJar->saveCookies();

                if (cookieName == "jwt") {
                    qCDebug(deploymentPluginLog) << "Got JWT";
                    m_webEngineView->hide();
                    m_connectorStatus = ConnectorStatus::LoggedIn;
                    emit connectorStatusUpdated(m_connectorStatus);
                    fetchUserInfoInternal();
                }
            });

    connect(&m_resourceGenerator,
            &QmlProjectManager::QmlProjectExporter::ResourceGenerator::qmlrcCreated,
            this,
            [this](const std::optional<Utils::FilePath> &resourcePath) {
                emit projectIsUploading();
                QString projectName = ProjectExplorer::ProjectManager::startupProject()->displayName();
                uploadProject(projectName, resourcePath->toUrlishString());
            });

    connect(&m_resourceGenerator,
            &QmlProjectManager::QmlProjectExporter::ResourceGenerator::errorOccurred,
            [this](const QString &errorString) {
                qCWarning(deploymentPluginLog) << "Error occurred while packing the project";
                emit projectPackingFailed(errorString);
            });

    refreshToken();
}

DVConnector::ConnectorStatus DVConnector::connectorStatus() const
{
    return m_connectorStatus;
}

QByteArray DVConnector::userInfo() const
{
    return m_userInfo;
}

bool DVConnector::isWebViewerVisible() const
{
    return m_isWebViewerVisible;
}

QString DVConnector::loginUrl() const
{
    return DVEndpoints::serviceUrl + DVEndpoints::login;
}

bool DVConnector::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_webEngineView) {
        if (m_isWebViewerVisible != m_webEngineView->isVisible()) {
            m_isWebViewerVisible = m_webEngineView->isVisible();
            emit webViewerVisibleChanged();
        }
    }
    return QObject::eventFilter(obj, e);
}

void DVConnector::projectList()
{
    qCDebug(deploymentPluginLog) << "Fetching project list";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::project);
    QNetworkRequest request(url);
    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Project list";
    evaluatorData.successCallback = [this](const QByteArray &reply, const QList<RawHeaderPair> &) {
        emit projectListReceived(reply);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::uploadCurrentProject()
{
    ProjectExplorer::Project* project = ProjectExplorer::ProjectManager::startupProject();
    m_resourceGenerator.createQmlrcAsync(project);
    emit projectIsPacking();
}

void DVConnector::uploadProject(const QString &projectId, const QString &filePath)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_UPLOADED);

    QFileInfo fileInfo(filePath);
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qCWarning(deploymentPluginLog) << "File not found";
        return;
    }

    const QString newProjectId = projectId.endsWith(".qmlrc") ? projectId : projectId + ".qmlrc";
    qCDebug(deploymentPluginLog) << "Uploading project:" << fileInfo.fileName()
                                 << "with projectId:" << newProjectId;

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multiPart);

    QHttpPart filePart;
    QHttpPart displayNamePart;
    QHttpPart descriptionPart;
    QHttpPart qdsVersionPart;

    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));

    // fileName is not required by the server, but it is required by the QHttpPart.
    // it's also useful to set custom file names for the uploaded files, such as uuids.
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + newProjectId + "\""));

    filePart.setBodyDevice(file);
    displayNamePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"displayName\""));
    displayNamePart.setBody(fileInfo.fileName().toUtf8());

    descriptionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"description\""));
    descriptionPart.setBody("testDescription");

    qdsVersionPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                             QVariant("form-data; name=\"qdsVersion\""));
    // qdsVersionPart.setBody(Core::ICore::versionString().toUtf8());
    qdsVersionPart.setBody("1.0.0");

    multiPart->append(filePart);
    multiPart->append(displayNamePart);
    multiPart->append(descriptionPart);
    multiPart->append(qdsVersionPart);

    QNetworkRequest request;
    request.setUrl(QUrl(DVEndpoints::serviceUrl + DVEndpoints::project));
    request.setTransferTimeout(10000);
    qCDebug(deploymentPluginLog) << "Sending request to: " << request.url().toString();

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->post(request, multiPart);
    evaluatorData.description = "Upload project";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit projectUploaded();
        // call userInfo to update storage info in the UI
        fetchUserInfo();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit projectUploadError(errorCode, errorString);
    };

    multiPart->setParent(evaluatorData.reply);
    emit projectUploadProgress(0.0);
    connect(evaluatorData.reply,
            &QNetworkReply::uploadProgress,
            this,
            [this](qint64 bytesSent, qint64 bytesTotal) {
                if (bytesTotal != 0)
                    emit projectUploadProgress(100.0 * (double) bytesSent / (double) bytesTotal);
            });
    evaluatorData.connectCallbacks(this);
}

void DVConnector::uploadProjectThumbnail(const QString &projectId, const QString &filePath)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_THUMBNAIL_UPLOADED);

    QFileInfo fileInfo(filePath);
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qCWarning(deploymentPluginLog) << "File not found";
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    file->setParent(multiPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"" + fileInfo.fileName() + "\""));

    filePart.setBodyDevice(file);

    multiPart->append(filePart);

    QNetworkRequest request;
    request.setUrl(QUrl(DVEndpoints::serviceUrl + DVEndpoints::projectThumbnail + "/" + projectId));
    request.setTransferTimeout(10000);
    qCDebug(deploymentPluginLog) << "Sending request to: " << request.url().toString()
                                 << "file: " << fileInfo.fileName();

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->post(request, multiPart);
    evaluatorData.description = "Upload project thumbnail";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit thumbnailUploaded();
        // call userInfo to update storage info in the UI
        fetchUserInfo();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit thumbnailUploadError(errorCode, errorString);
    };

    multiPart->setParent(evaluatorData.reply);
    emit thumbnailUploadProgress(0.0);
    connect(evaluatorData.reply,
            &QNetworkReply::uploadProgress,
            this,
            [this](qint64 bytesSent, qint64 bytesTotal) {
                emit thumbnailUploadProgress(100.0 * (double) bytesSent / (double) bytesTotal);
            });
    evaluatorData.connectCallbacks(this);
}

void DVConnector::deleteProject(const QString &projectId)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_DELETED);

    qCDebug(deploymentPluginLog) << "Deleting project with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::project + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->deleteResource(request);
    evaluatorData.description = "Delete project";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit projectDeleted();
        // call userInfo to update storage info in the UI
        fetchUserInfo();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit projectDeleteError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::deleteProjectThumbnail(const QString &projectId)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_THUMBNAIL_DELETED);

    qCDebug(deploymentPluginLog) << "Deleting project thumbnail with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::projectThumbnail + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->deleteResource(request);
    evaluatorData.description = "Delete project thumbnail";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit thumbnailDeleted();
        // call userInfo to update storage info in the UI
        fetchUserInfo();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit thumbnailDeleteError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::downloadProject(const QString &projectId, const QString &filePath)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_DOWNLOADED);

    qCDebug(deploymentPluginLog) << "Downloading project with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::project + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Download project";
    evaluatorData.successCallback = [this, filePath](const QByteArray &reply,
                                                     const QList<RawHeaderPair> &) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(deploymentPluginLog) << "Failed to open file for writing:" << filePath;
            return;
        }
        file.write(reply);
        file.close();
        emit projectDownloaded();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit projectDownloadError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::downloadProjectThumbnail(const QString &projectId, const QString &filePath)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_THUMBNAIL_DOWNLOADED);

    qCDebug(deploymentPluginLog) << "Downloading project thumbnail with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::projectThumbnail + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Download project thumbnail";
    evaluatorData.successCallback = [this, filePath](const QByteArray &reply,
                                                     const QList<RawHeaderPair> &) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(deploymentPluginLog) << "Failed to open file for writing:" << filePath;
            return;
        }
        file.write(reply);
        file.close();
        emit thumbnailDownloaded();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit thumbnailDownloadError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::sharedProjectList()
{
    qCDebug(deploymentPluginLog) << "Fetching shared project list";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::share);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Shared project list";
    evaluatorData.successCallback = [this](const QByteArray &reply, const QList<RawHeaderPair> &) {
        emit sharedProjectListReceived(reply);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::shareProject(const QString &projectId,
                               const QString &password,
                               const int ttlDays,
                               const QString &description)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_SHARED);

    qCDebug(deploymentPluginLog) << "Sharing project with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::share);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["fileName"] = projectId;
    json["password"] = password;
    json["ttlDays"] = ttlDays;
    json["description"] = description;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->post(request, data);
    evaluatorData.description = "Share project";
    evaluatorData.successCallback = [this, projectId](const QByteArray &reply,
                                                      const QList<RawHeaderPair> &) {
        qCDebug(deploymentPluginLog) << "Project shared: " << reply;
        const QString shareUuid = QJsonDocument::fromJson(reply).object()["id"].toString();
        emit projectShared(projectId, shareUuid);
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit projectShareError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::unshareProject(const QString &shareUUID)
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_UNSHARED);

    qCDebug(deploymentPluginLog) << "Unsharing project with id: " << shareUUID;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::share + "/" + shareUUID);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->deleteResource(request);
    evaluatorData.description = "Unshare project";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit projectUnshared();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit projectUnshareError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::unshareAllProjects()
{
    QmlDesigner::QmlDesignerPlugin::emitUsageStatistics(
        QmlDesigner::Constants::EVENT_DESIGNVIEWER_PROJECT_UNSHARED_ALL);
    qCDebug(deploymentPluginLog) << "Unsharing all projects";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::share);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->deleteResource(request);
    evaluatorData.description = "Unshare all projects";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        emit allProjectsUnshared();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit allProjectsUnshareError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::downloadSharedProject(const QString &projectId, const QString &filePath)
{
    qCDebug(deploymentPluginLog) << "Downloading shared project with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::share + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Download shared project";
    evaluatorData.successCallback = [this, filePath](const QByteArray &reply,
                                                     const QList<RawHeaderPair> &) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(deploymentPluginLog) << "Failed to open file for writing:" << filePath;
            return;
        }
        file.write(reply);
        file.close();
        emit sharedProjectDownloaded();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit sharedProjectDownloadError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::downloadSharedProjectThumbnail(const QString &projectId, const QString &filePath)
{
    qCDebug(deploymentPluginLog) << "Downloading shared project thumbnail with id: " << projectId;
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::shareThumbnail + "/" + projectId);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Download shared project thumbnail";
    evaluatorData.successCallback = [this, filePath](const QByteArray &reply,
                                                     const QList<RawHeaderPair> &) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qCWarning(deploymentPluginLog) << "Failed to open file for writing:" << filePath;
            return;
        }
        file.write(reply);
        file.close();
        emit sharedProjectThumbnailDownloaded();
    };
    evaluatorData.errorPreCallback = [this](const int errorCode, const QString &errorString) {
        emit sharedProjectThumbnailDownloadError(errorCode, errorString);
    };
    evaluatorData.connectCallbacks(this);
}

void DVConnector::login()
{
    if (m_connectorStatus == ConnectorStatus::LoggedIn) {
        qCDebug(deploymentPluginLog) << "Already logged in";
        return;
    }

    qCDebug(deploymentPluginLog) << "Logging in";
    m_webEnginePage->load(QUrl(DVEndpoints::serviceUrl + DVEndpoints::login));
    m_webEngineView->show();
    m_webEngineView->raise();
}

void DVConnector::internalLogin()
{
    qCDebug(deploymentPluginLog) << "Internal login";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::login);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Internal Login";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        m_connectorStatus = ConnectorStatus::LoggedIn;
        emit connectorStatusUpdated(m_connectorStatus);
        fetchUserInfo();
    };

    evaluatorData.connectCallbacks(this);
}

void DVConnector::logout()
{
    if (m_connectorStatus == ConnectorStatus::NotLoggedIn) {
        qCDebug(deploymentPluginLog) << "Already logged out";
        return;
    }

    qCDebug(deploymentPluginLog) << "Logging out";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::logout);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.description = "Logout";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &) {
        QWebEngineCookieStore *cookieStore = m_webEnginePage->profile()->cookieStore();
        cookieStore->deleteAllCookies();
        m_connectorStatus = ConnectorStatus::NotLoggedIn;
        m_userInfo.clear();
        emit connectorStatusUpdated(m_connectorStatus);
    };

    evaluatorData.connectCallbacks(this);
}

void DVConnector::fetchUserInfo()
{
    refreshToken();
}

void DVConnector::refreshToken()
{
    qCDebug(deploymentPluginLog) << "Refreshing token";
    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::refreshToken);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.reply = m_networkAccessManager->post(request, QByteArray());
    evaluatorData.description = "Refresh token";
    evaluatorData.successCallback = [this](const QByteArray &, const QList<RawHeaderPair> &headers) {
        qCDebug(deploymentPluginLog) << "Token refreshed";
        for (const RawHeaderPair &header : headers) {
            if (header.first.compare("Set-Cookie", Qt::CaseInsensitive) == 0) {
                const QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(header.second);
                for (const QNetworkCookie &cookie : cookies) {
                    qCDebug(deploymentPluginLog)
                        << "Refresh Cookie:" << cookie.name() << cookie.value();
                    m_networkAccessManager->cookieJar()->insertCookie(cookie);
                    m_networkCookieJar->saveCookies();
                }
            }
        }
        m_connectorStatus = ConnectorStatus::LoggedIn;
        emit connectorStatusUpdated(m_connectorStatus);
        fetchUserInfoInternal();
    };

    evaluatorData.connectCallbacks(this);
}

void DVConnector::fetchUserInfoInternal()
{
    qCDebug(deploymentPluginLog) << "Fetching user info";

    QUrl url(DVEndpoints::serviceUrl + DVEndpoints::userInfo);
    QNetworkRequest request(url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.description = "User info";
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.successCallback = [&](const QByteArray &reply, const QList<RawHeaderPair> &) {
        m_userInfo = reply;
        emit userInfoReceived(reply);
    };

    evaluatorData.errorCodeOtherCallback = [this](const int, const QString &) {
        QTimer::singleShot(1000, this, &DVConnector::fetchUserInfo);
    };

    evaluatorData.connectCallbacks(this);
}

void DVConnector::evaluateReply(const ReplyEvaluatorData &evaluatorData)
{
    if (evaluatorData.reply->error() == QNetworkReply::NoError) {
        qCDebug(deploymentPluginLog) << evaluatorData.description << "request finished successfully";

        if (evaluatorData.successCallback) {
            qCDebug(deploymentPluginLog)
                << evaluatorData.description << ": Executing success callback";
            evaluatorData.successCallback(evaluatorData.reply->readAll(),
                                          evaluatorData.reply->rawHeaderPairs());
        }
        return;
    }

    qCDebug(deploymentPluginLog) << evaluatorData.description << "Request error. Return Code:"
                                 << evaluatorData.reply
                                        ->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                                        .toInt()
                                 << ", Message:" << evaluatorData.reply->errorString();
    if (evaluatorData.errorPreCallback) {
        qCDebug(deploymentPluginLog)
            << evaluatorData.description << ": Executing custom error pre callback";
        evaluatorData.errorPreCallback(evaluatorData.reply->error(),
                                       evaluatorData.reply->errorString());
    }

    if (evaluatorData.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
        if (evaluatorData.errorCodeUnauthorizedCallback) {
            qCDebug(deploymentPluginLog)
                << evaluatorData.description << ": Executing custom unauthorized callback";
            evaluatorData.errorCodeUnauthorizedCallback(
                evaluatorData.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                evaluatorData.reply->errorString());
        } else {
            qCDebug(deploymentPluginLog)
                << evaluatorData.description << ": Executing default unauthorized callback";
            m_connectorStatus = ConnectorStatus::NotLoggedIn;
            emit connectorStatusUpdated(m_connectorStatus);
        }
    } else {
        if (evaluatorData.errorCodeOtherCallback) {
            qCDebug(deploymentPluginLog)
                << evaluatorData.description << ": Executing custom error callback";
            evaluatorData.errorCodeOtherCallback(
                evaluatorData.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                evaluatorData.reply->errorString());
        }
    }

    evaluatorData.reply->deleteLater();
}

} // namespace QmlDesigner::DesignViewer
