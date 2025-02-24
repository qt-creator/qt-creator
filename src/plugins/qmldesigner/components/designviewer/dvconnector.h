// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

#include <qmlprojectmanager/qmlprojectexporter/resourcegenerator.h>

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
    Q_PROPERTY(QByteArray userInfo READ userInfo NOTIFY userInfoReceived)
    Q_PROPERTY(bool isWebViewerVisible READ isWebViewerVisible NOTIFY webViewerVisibleChanged)
public:
    explicit DVConnector(QObject *parent = nullptr);

    enum ConnectorStatus { FetchingUserInfo, NotLoggedIn, LoggedIn };
    Q_ENUM(ConnectorStatus)

public:
    // getters for UI
    ConnectorStatus connectorStatus() const;
    QByteArray userInfo() const;
    bool isWebViewerVisible() const;
    Q_INVOKABLE QString loginUrl() const;

    void projectList();
    Q_INVOKABLE void uploadCurrentProject();
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

    Q_INVOKABLE void login();
    Q_INVOKABLE void logout();
    Q_INVOKABLE void fetchUserInfo();

private:
    // network
    QScopedPointer<QNetworkAccessManager> m_networkAccessManager;
    QScopedPointer<CustomCookieJar> m_networkCookieJar;

    // login
    QWebEnginePage *m_webEnginePage;
    QWebEngineView *m_webEngineView;
    bool m_isWebViewerVisible;

    // status
    ConnectorStatus m_connectorStatus;
    QByteArray m_userInfo;

    // other internals
    QmlProjectManager::QmlProjectExporter::ResourceGenerator m_resourceGenerator;

    typedef QPair<QByteArray, QByteArray> RawHeaderPair;
    struct ReplyEvaluatorData
    {
        QNetworkReply *reply = nullptr;
        QString description;
        std::function<void(const QByteArray &, const QList<RawHeaderPair> &)> successCallback = nullptr;
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
    bool eventFilter(QObject *obj, QEvent *e) override;
    void internalLogin();
    void refreshToken();
    void fetchUserInfoInternal();

signals:
    // service integration - project related signals
    void projectListReceived(const QByteArray &reply);
    void projectUploaded();
    void projectUploadError(const int errorCode, const QString &message);
    void projectUploadProgress(const double progress);
    void projectDeleted();
    void projectDeleteError(const int errorCode, const QString &message);
    void projectDownloaded();
    void projectDownloadError(const int errorCode, const QString &message);

    // service integration - project thumbnail related signals
    void thumbnailUploaded();
    void thumbnailUploadError(const int errorCode, const QString &message);
    void thumbnailUploadProgress(const double progress);
    void thumbnailDeleted();
    void thumbnailDeleteError(const int errorCode, const QString &message);
    void thumbnailDownloaded();
    void thumbnailDownloadError(const int errorCode, const QString &message);

    // service integration - shared project related signals
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

    // UI integration - login/user
    void userInfoReceived(const QByteArray &reply);
    void webViewerVisibleChanged();

    // UI integration - project packing/uploading
    void projectIsPacking();
    void projectPackingFailed(const QString &errorString);
    void projectIsUploading();

    // internal signals
    void connectorStatusUpdated(const ConnectorStatus status);
};

} // namespace QmlDesigner::DesignViewer
