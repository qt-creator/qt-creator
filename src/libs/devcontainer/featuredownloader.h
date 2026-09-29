// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devcontainerfeature.h"

#include <QtTaskTree/QTaskTree>

#include <QObject>
#include <QPointer>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
QT_END_NAMESPACE

namespace DevContainer::Internal {

// Downloads the archive of a feature from an OCI registry or from a URL.
class FeatureDownloader : public QObject
{
    Q_OBJECT

public:
    FeatureDownloader();
    ~FeatureDownloader() override;

    void setReference(const FeatureReference &reference);
    void setNetworkAccessManager(QNetworkAccessManager *networkAccessManager);
    void setCredentials(const std::optional<RegistryCredentials> &credentials);
    // Downloads are kept there, and used when they are known to be current, or when the
    // server cannot be reached.
    void setCacheFolder(const Utils::FilePath &cacheFolder);
    void setLogFunction(const std::function<void(const QString &)> &logFunction);

    void start();

    QByteArray archive() const { return m_archive; }
    QString errorString() const { return m_errorString; }

signals:
    void started();
    void done(QtTaskTree::DoneResult result);

private:
    using Handler = void (FeatureDownloader::*)(QNetworkReply *reply);
    enum class SendAuthorization { No, Yes };

    void get(QNetworkRequest request, Handler handler);
    void post(QNetworkRequest request, const QByteArray &data, Handler handler);
    void track(QNetworkReply *reply, Handler handler);

    void requestManifest();
    void onManifest(QNetworkReply *reply);
    void authenticate(const QByteArray &challenge);
    void onToken(QNetworkReply *reply);
    void requestBlob(const QUrl &url, SendAuthorization sendAuthorization);
    void onBlob(QNetworkReply *reply);
    void onTarball(QNetworkReply *reply);

    QUrl registryUrl(const QString &path) const;
    QByteArray basicAuthorization() const;

    Utils::FilePath blobCacheFile(const QString &digest) const;
    Utils::FilePath refCacheFile() const;
    Utils::FilePath urlCacheFile() const;
    bool useCachedBlob(const QString &digest, const QString &message);
    void storeInCache(const Utils::FilePath &file, const QByteArray &data);

    void log(const QString &message) const;
    QString downloadError(const QString &details) const;
    void finish(const QString &errorString = {});

    FeatureReference m_reference;
    QNetworkAccessManager *m_networkAccessManager = nullptr;
    std::optional<RegistryCredentials> m_credentials;
    Utils::FilePath m_cacheFolder;
    std::function<void(const QString &)> m_logFunction;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_authorization;
    QString m_blobDigest;
    QUrl m_blobUrl;
    int m_redirects = 0;
    bool m_triedAuthentication = false;
    bool m_tooLarge = false;
    QByteArray m_archive;
    QString m_errorString;
};

using FeatureDownloaderTask = QtTaskTree::QCustomTask<FeatureDownloader>;

} // namespace DevContainer::Internal
