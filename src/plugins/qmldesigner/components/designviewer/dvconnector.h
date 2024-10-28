// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

namespace QmlDesigner::DesignViewer {

class CustomWebEnginePage : public QWebEnginePage
{
public:
    CustomWebEnginePage(QWebEngineProfile *profile, QObject *parent = nullptr);

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override;
};

class CustomCookieJar : public QNetworkCookieJar
{
    Q_OBJECT
public:
    CustomCookieJar(QObject *parent = nullptr, const QString &cookieFilePath = "cookies.txt");
    ~CustomCookieJar();

    void loadCookies();
    void saveCookies();
    void clearCookies();

private:
    const QString m_cookieFilePath;
};

class DVConnector : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ConnectorStatus connectorStatus READ connectorStatus NOTIFY connectorStatusUpdated)
public:
    explicit DVConnector(QObject *parent = nullptr);

    enum ConnectorStatus { FetchingUserInfo, NotLoggedIn, LoggedIn };
    Q_ENUM(ConnectorStatus)

public:
    ConnectorStatus connectorStatus() const;

    void projectList();
    void uploadProject(const QString &projectId, const QString &filePath);
    void deleteProject(const QString &projectId);
    void downloadProject(const QString &projectId, const QString &filePath);

    void uploadProjectThumbnail(const QString &projectId, const QString &filePath);
    void deleteProjectThumbnail(const QString &projectId);
    void downloadProjectThumbnail(const QString &projectId, const QString &filePath);

    void sharedProjectList();
    void shareProject(const QString &projectId,
                      const QString &password = {},
                      const int ttlDays = 30,
                      const QString &description = {});
    void unshareProject(const QString &shareUUID);
    void unshareAllProjects();
    void downloadSharedProject(const QString &projectId, const QString &filePath);
    void downloadSharedProjectThumbnail(const QString &projectId, const QString &filePath);

    void login();
    void logout();
    void userInfo();

private:
    // network
    QScopedPointer<QNetworkAccessManager> m_networkAccessManager;
    QScopedPointer<CustomCookieJar> m_networkCookieJar;

    // login
    QScopedPointer<QWebEngineProfile> m_webEngineProfile;
    QScopedPointer<QWebEnginePage> m_webEnginePage;
    QScopedPointer<QWebEngineView> m_webEngineView;

    // status
    ConnectorStatus m_connectorStatus;

    struct ReplyEvaluatorData
    {
        QNetworkReply *reply = nullptr;
        QString description;
        std::function<void(const QByteArray &)> successCallback = nullptr;
        std::function<void(const int, const QString &)> errorPreCallback = nullptr;
        std::function<void(const int, const QString &)> errorCodeUnauthorizedCallback = nullptr;
        std::function<void(const int, const QString &)> errorCodeOtherCallback = nullptr;

        void connectCallbacks(DVConnector *connector)
        {
            ReplyEvaluatorData newData = *this;
            QObject::connect(reply, &QNetworkReply::finished, connector, [newData, connector] {
                connector->evaluateReply(newData);
            });
        }
    };

private:
    void evaluateReply(const ReplyEvaluatorData &evaluator);

signals:
    // backend integration - project related signals
    void projectListReceived(const QByteArray &reply);
    void projectUploaded();
    void projectUploadError(const int errorCode, const QString &message);
    void projectUploadProgress(const double progress);
    void projectDeleted();
    void projectDeleteError(const int errorCode, const QString &message);
    void projectDownloaded();
    void projectDownloadError(const int errorCode, const QString &message);

    // backend integration - project thumbnail related signals
    void thumbnailUploaded();
    void thumbnailUploadError(const int errorCode, const QString &message);
    void thumbnailUploadProgress(const double progress);
    void thumbnailDeleted();
    void thumbnailDeleteError(const int errorCode, const QString &message);
    void thumbnailDownloaded();
    void thumbnailDownloadError(const int errorCode, const QString &message);

    // backend integration - shared project related signals
    void sharedProjectListReceived(const QByteArray &reply);
    void projectShared(const QString &projectId, const QString &shareUUID);
    void projectShareError(const int errorCode, const QString &message);
    void projectUnshared();
    void projectUnshareError(const int errorCode, const QString &message);
    void allProjectsUnshared();
    void allProjectsUnshareError(const int errorCode, const QString &message);
    void sharedProjectDownloaded();
    void sharedProjectDownloadError(const int errorCode, const QString &message);
    void sharedProjectThumbnailDownloaded();
    void sharedProjectThumbnailDownloadError(const int errorCode, const QString &message);

    // backend integration - login/user related signals
    void userInfoReceived(const QByteArray &reply);

    // internal signals
    void connectorStatusUpdated(const ConnectorStatus status);
};

} // namespace QmlDesigner::DesignViewer
