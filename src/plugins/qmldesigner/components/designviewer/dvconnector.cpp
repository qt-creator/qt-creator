// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dvconnector.h"
#include "dvauthenticator.h"

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

namespace QmlDesigner::DesignViewer {

namespace Endpoints {
using namespace Qt::Literals;
#ifdef QDS_DESIGNVIEWER_USE_STAGING
constexpr auto serviceUrl = "https://api-designviewer-staging.qt.io/api/v2"_L1;
constexpr auto appUrl = "https://designviewer-staging.qt.io"_L1;
#else
constexpr auto serviceUrl = "https://api-designviewer.qt.io/api/v2"_L1;
constexpr auto appUrl = "https://designviewer.qt.io"_L1;
#endif
constexpr auto project = "/project"_L1;
constexpr auto projectThumbnail = "/project/image"_L1;
constexpr auto share = "/share"_L1;
constexpr auto shareThumbnail = "/share/image"_L1;
constexpr auto userInfo = "/auth/userinfo"_L1;
}; // namespace Endpoints

namespace ReturnCodes {
constexpr int unauthorized = 401;
}

class CustomNetworkRequest : public QNetworkRequest
{
public:
    explicit CustomNetworkRequest(const QScopedPointer<DVAuthenticator> &auth, const QUrl &url = {})
        : QNetworkRequest(url)
    {
        this->setRawHeader("Authorization", auth->accessToken().prepend("Bearer ").toUtf8());
        this->setRawHeader("accept", "application/json, text/plain, */*");
    }
};

DVConnector::~DVConnector() = default;
DVConnector::DVConnector(QObject *parent)
    : QObject{parent}
{
    m_auth.reset(new DVAuthenticator);
    m_networkAccessManager.reset(new QNetworkAccessManager(this));
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

    connect(m_auth.data(), &DVAuthenticator::authStatusChanged, this, [this] {
        emit connectorStatusUpdated(connectorStatus());
    });
}

DVConnector::ConnectorStatus DVConnector::connectorStatus() const
{
    return m_auth->isAuthenticated() ? ConnectorStatus::LoggedIn : ConnectorStatus::NotLoggedIn;
}

QByteArray DVConnector::userInfo() const
{
    return m_userInfo;
}

QString DVConnector::loginUrl() const
{
    return Endpoints::appUrl;
}

void DVConnector::projectList()
{
    qCDebug(deploymentPluginLog) << "Fetching project list";
    QUrl url(Endpoints::serviceUrl + Endpoints::project);
    CustomNetworkRequest request(m_auth, url);
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

    multiPart->append(displayNamePart);
    multiPart->append(descriptionPart);
    multiPart->append(qdsVersionPart);
    multiPart->append(filePart);

    CustomNetworkRequest request(m_auth);
    request.setUrl({Endpoints::serviceUrl + Endpoints::project});
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

    CustomNetworkRequest request(m_auth);
    request.setUrl({Endpoints::serviceUrl + Endpoints::projectThumbnail + "/" + projectId});
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
    QUrl url(Endpoints::serviceUrl + Endpoints::project + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::projectThumbnail + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::project + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::projectThumbnail + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::share);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::share);
    CustomNetworkRequest request(m_auth, url);
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
    QUrl url(Endpoints::serviceUrl + Endpoints::share + "/" + shareUUID);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::share);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::share + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    QUrl url(Endpoints::serviceUrl + Endpoints::shareThumbnail + "/" + projectId);
    CustomNetworkRequest request(m_auth, url);

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
    qCDebug(deploymentPluginLog) << "Logging in";
    m_auth->login();
}

void DVConnector::logout()
{
    qCDebug(deploymentPluginLog) << "Logging out";
    m_auth->logout();
}

void DVConnector::fetchUserInfo()
{
    qCDebug(deploymentPluginLog) << "Fetching user info";

    QUrl url(Endpoints::serviceUrl + Endpoints::userInfo);
    CustomNetworkRequest request(m_auth, url);

    ReplyEvaluatorData evaluatorData;
    evaluatorData.description = "User info";
    evaluatorData.reply = m_networkAccessManager->get(request);
    evaluatorData.successCallback = [&](const QByteArray &reply, const QList<RawHeaderPair> &) {
        m_userInfo = reply;
        emit userInfoReceived(reply);
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

    if (evaluatorData.reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
        == ReturnCodes::unauthorized) {
        qCDebug(deploymentPluginLog)
            << evaluatorData.description << ": Executing default unauthorized callback";
        m_auth->logout();
        emit connectorStatusUpdated(ConnectorStatus::NotLoggedIn);
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
