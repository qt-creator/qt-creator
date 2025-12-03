// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLoggingCategory>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>

namespace QKeychain {
class Job;
}

namespace QmlDesigner::DesignViewer {
Q_DECLARE_LOGGING_CATEGORY(deploymentPluginLog)

class DVAuthenticator : public QObject
{
    Q_OBJECT
public:
    DVAuthenticator(QObject *parent = nullptr);
    ~DVAuthenticator();

    void login();
    void logout();
    bool isAuthenticated() const;
    QString accessToken() const;

private:
    enum class KeychainOps { Get, Set, Delete };
    const QString m_settingsPath;

    QOAuth2AuthorizationCodeFlow m_oauth2;
    QOAuthHttpServerReplyHandler m_replyHandler;

    std::list<QKeychain::Job *> m_jobQueue;

private:
    void accessToKeychain(const KeychainOps &op, const QString &key, const QString &value = {});
    void refreshTokens();
    void loadTokens();
    void clearTokens();
    void saveToken(const QString &key, const QString &value);

signals:
    void authStatusChanged();
};

} // namespace QmlDesigner::DesignViewer
