// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dvauthenticator.h"

#include <coreplugin/icore.h>
#include <qtkeychain/keychain.h>

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace QmlDesigner::DesignViewer {
Q_LOGGING_CATEGORY(deploymentPluginLog, "qtc.designer.deploymentPlugin", QtWarningMsg)

namespace QtLogin {
using namespace Qt::Literals;
#ifdef QDS_DESIGNVIEWER_USE_STAGING
constexpr auto serviceUrl = "https://login-qt-io-staging.herokuapp.com"_L1;
constexpr auto oauthClientId = "qt-design-studio-staging"_L1;
#else
constexpr auto serviceUrl = "https://login.qt.io"_L1;
constexpr auto oauthClientId = "qt-design-studio"_L1;
#endif
// constexpr auto logout = "/v1/oidc/logout"_L1;
constexpr auto authorize = "/v1/oidc/authorize"_L1;
constexpr auto token = "/v1/oidc/token"_L1;
}; // namespace QtLogin

namespace StringConstants {
using namespace Qt::Literals;
constexpr auto keychainDvService = "qds_design_viewer"_L1;
constexpr auto keychainDvJWTKey = "qds_design_viewer_jwt"_L1;
constexpr auto keychainDvJWTRefreshKey = "qds_design_viewer_jwt_refresh"_L1;

constexpr auto htmlSuccessResponse = R"(
<!DOCTYPE html>
<html>
<head>
    <title>OAuth2 Authorization</title>
</head>
<body>
    <h1>Authorization successful</h1>
    <p>You can now close this window.</p>
</body>
</html>
)"_L1;

constexpr auto htmlErrorResponse = R"(
<!DOCTYPE html>
<html>
<head>
    <title>OAuth2 Authorization</title>
</head>
<body>
    <h1>Authorization failed</h1>
    <p>There was an error during the authorization process.</p>
</body>
</html>
)"_L1;
} // namespace StringConstants

DVAuthenticator::~DVAuthenticator() = default;

DVAuthenticator::DVAuthenticator(QObject *parent)
    : QObject{parent}
    , m_settingsPath(Core::ICore::settings()->fileName() + "/qt_design_viewer.json")
    , m_oauth2(this)
    , m_replyHandler(QHostAddress::LocalHost, 54867, this)
{
    m_oauth2.setAuthorizationUrl(QUrl(QtLogin::serviceUrl + QtLogin::authorize));
    m_oauth2.setAccessTokenUrl(QUrl(QtLogin::serviceUrl + QtLogin::token));
    m_oauth2.setClientIdentifier(QtLogin::oauthClientId);
    m_oauth2.setScope("email api offline_access profile openid");
    m_oauth2.setPkceMethod(QOAuth2AuthorizationCodeFlow::PkceMethod::S256);
    m_replyHandler.setCallbackPath("/callback");
    m_oauth2.setModifyParametersFunction(
        [&](QAbstractOAuth::Stage stage, QMultiMap<QString, QVariant> *parameters) {
            if (stage == QAbstractOAuth::Stage::RequestingAuthorization) {
                parameters->insert("duration", "permanent");
                parameters->insert("prompt", "login consent");
                parameters->insert("resource", "https://login.qt.io");
            }
        });
    m_oauth2.setReplyHandler(&m_replyHandler);
    m_oauth2.setAutoRefresh(true);
    m_oauth2.setNonceMode(QOAuth2AuthorizationCodeFlow::NonceMode::Automatic);
    m_replyHandler.setCallbackText(StringConstants::htmlSuccessResponse);

    connect(&m_oauth2,
            &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            this,
            &QDesktopServices::openUrl);

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::statusChanged, [&](QAbstractOAuth::Status status) {
        if (status == QAbstractOAuth::Status::Granted) {
            m_replyHandler.setCallbackText(StringConstants::htmlSuccessResponse);
            qCDebug(deploymentPluginLog) << "Authenticated";
            saveToken(StringConstants::keychainDvJWTKey, m_oauth2.token());
            if (m_oauth2.refreshToken().isEmpty()) {
                m_oauth2.refreshTokens();
            } else {
                saveToken(StringConstants::keychainDvJWTRefreshKey, m_oauth2.refreshToken());
            }
        } else if (status == QAbstractOAuth::Status::NotAuthenticated) {
            qCWarning(deploymentPluginLog) << "Could not authenticate";
            clearTokens();
        } else if (status == QAbstractOAuth::Status::TemporaryCredentialsReceived) {
            qCDebug(deploymentPluginLog) << "Temporary credentials received";
        } else if (status == QAbstractOAuth::Status::RefreshingToken) {
            qCDebug(deploymentPluginLog) << "Refreshing token";
        }
        emit authStatusChanged();
    });

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::requestFailed, [&](QAbstractOAuth::Error error) {
        qCWarning(deploymentPluginLog) << "Authorization request failed:" << (int) error;
    });

    connect(&m_oauth2,
            &QOAuth2AuthorizationCodeFlow::error,
            [&](const QString &error, const QString &errorDescription, const QUrl &uri) {
                m_replyHandler.setCallbackText(StringConstants::htmlErrorResponse);
                qCWarning(deploymentPluginLog)
                    << "Authorization error:" << error << errorDescription << uri;
            });

    connect(&m_oauth2, &QOAuth2AuthorizationCodeFlow::refreshTokenChanged, [this]() {
        qCDebug(deploymentPluginLog) << "Refresh Token Updated";
        saveToken(StringConstants::keychainDvJWTRefreshKey, m_oauth2.refreshToken());
    });

    loadTokens();
}

bool DVAuthenticator::isAuthenticated() const
{
    return !m_oauth2.token().isEmpty();
}

void DVAuthenticator::login()
{
    m_oauth2.grant();
}

void DVAuthenticator::logout()
{
    clearTokens();
    emit authStatusChanged();
}

QString DVAuthenticator::accessToken() const
{
    return m_oauth2.token();
}

void DVAuthenticator::accessToKeychain(const KeychainOps &op, const QString &key, const QString &value)
{
    QKeychain::Job *job = nullptr;
    QKeychain::ReadPasswordJob *reader = nullptr;

    switch (op) {
    case KeychainOps::Get: {
        job = reader = new QKeychain::ReadPasswordJob(StringConstants::keychainDvService);
        m_jobQueue.push_back(job);
        break;
    }
    case KeychainOps::Set: {
        QKeychain::WritePasswordJob *writer = new QKeychain::WritePasswordJob(
            StringConstants::keychainDvService);
        writer->setBinaryData(value.toUtf8());
        job = writer;
        break;
    }
    case KeychainOps::Delete:
        job = new QKeychain::DeletePasswordJob(StringConstants::keychainDvService);
        break;
    }

    job->setAutoDelete(true);
    job->setKey(key);

    connect(job, &QKeychain::Job::finished, this, [this, op](QKeychain::Job *job) {
        using namespace Qt::Literals;

        if (job->error() != QKeychain::NoError && job->error() != QKeychain::EntryNotFound) {
            const auto message = (op == KeychainOps::Get) ? "Can't get the token from the keychain"_L1
                                 : (op == KeychainOps::Set)
                                     ? "Can't save the token into the keychain."_L1
                                     : "Can't remove the token from the keychain."_L1;
            qCCritical(deploymentPluginLog)
                << message << "Key:" << job->key() << "Reason:" << job->errorString();
            return;
        }

        if (op == KeychainOps::Get) {
            if (job->error() != QKeychain::EntryNotFound) {
                QKeychain::ReadPasswordJob *reader = qobject_cast<QKeychain::ReadPasswordJob *>(job);
                const auto token = QString::fromUtf8(reader->binaryData());
                const auto key = reader->key();

                if (key == StringConstants::keychainDvJWTKey)
                    m_oauth2.setToken(token);
                else
                    m_oauth2.setRefreshToken(token);
            }

            m_jobQueue.remove(job);
            if (!m_jobQueue.size())
                m_oauth2.refreshTokens();
        }
    });

    job->start();
}

void DVAuthenticator::loadTokens()
{
    if (QKeychain::isAvailable()) {
        const QLatin1StringView tokenKeys[] = {StringConstants::keychainDvJWTKey,
                                               StringConstants::keychainDvJWTRefreshKey};

        for (const auto &key : tokenKeys) {
            accessToKeychain(KeychainOps::Get, key);
        }
    } else {
        QFile file(m_settingsPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(deploymentPluginLog)
                << "Failed to open token file for reading:" << file.fileName();
            return;
        }

        QJsonParseError jsonParseErr;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &jsonParseErr);
        file.close();

        if (jsonParseErr.error != QJsonParseError::NoError) {
            qCWarning(deploymentPluginLog)
                << "Cannot parse the json data from the file: " << file.fileName();
            return;
        }

        QJsonObject rootObj = jsonDoc.object();
        m_oauth2.setToken(rootObj[StringConstants::keychainDvJWTKey].toString());
        m_oauth2.setRefreshToken(rootObj[StringConstants::keychainDvJWTRefreshKey].toString());

        file.close();
    }
}

void DVAuthenticator::clearTokens()
{
    m_oauth2.setToken("");
    // m_oauth2.setRefreshToken("");

    if (QKeychain::isAvailable()) {
        accessToKeychain(KeychainOps::Delete, StringConstants::keychainDvJWTKey);
        accessToKeychain(KeychainOps::Delete, StringConstants::keychainDvJWTRefreshKey);
    } else {
        if (QFile::exists(m_settingsPath) && !QFile::remove(m_settingsPath))
            qCWarning(deploymentPluginLog)
                << "Can't remove the old tokens file! Path: " << m_settingsPath;
    }
}

void DVAuthenticator::saveToken(const QString &key, const QString &value)
{
    if (QKeychain::isAvailable()) {
        accessToKeychain(KeychainOps::Set, key, value);
    } else {
        QFile file(m_settingsPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(deploymentPluginLog)
                << "Failed to open token file for reading:" << file.fileName();
            return;
        }

        QJsonParseError jsonParseErr;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &jsonParseErr);
        file.close();

        if (jsonParseErr.error != QJsonParseError::NoError) {
            qCWarning(deploymentPluginLog)
                << "Cannot parse the json data from the file: " << file.fileName();
            return;
        }

        QJsonObject rootObj = jsonDoc.object();
        rootObj.insert(key, value);
        jsonDoc.setObject(rootObj);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qCWarning(deploymentPluginLog)
                << "Failed to open token file for writing:" << file.fileName();
            return;
        }

        file.write(jsonDoc.toJson(QJsonDocument::Indented));
        file.close();
    }
}
} // namespace QmlDesigner::DesignViewer
