// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "filedownloader.h"

#include <private/qqmldata_p.h>
#include <utils/networkaccessmanager.h>
#include <utils/filepath.h>

#include <QDir>
#include <QQmlEngine>
#include <QRandomGenerator>
#include <QSslSocket>

namespace QmlDesigner {

FileDownloader::FileDownloader(QObject *parent)
    : QObject(parent)
{
    QObject::connect(this, &FileDownloader::downloadFailed, this, [this]() {
        if (m_outputFile.exists())
            m_outputFile.remove();
    });

    QObject::connect(this, &FileDownloader::downloadCanceled, this, [this]() {
        if (m_outputFile.exists())
            m_outputFile.remove();
    });
}

FileDownloader::~FileDownloader()
{
    // Delete the temp file only if a target Path was set (i.e. file will be moved)
    if (deleteFileAtTheEnd() && m_outputFile.exists())
        m_outputFile.remove();
}

bool FileDownloader::deleteFileAtTheEnd() const
{
    return m_targetFilePath.isEmpty();
}

QNetworkRequest FileDownloader::makeRequest() const
{
    QUrl url = m_url;

    if (url.scheme() == "https") {
#ifndef QT_NO_SSL
        if (!QSslSocket::supportsSsl())
#endif
        {
            qWarning() << "SSL is not available. HTTP will be used instead of HTTPS.";
            url.setScheme("http");
        }
    }

    auto request = QNetworkRequest(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::UserVerifiedRedirectPolicy);

    return request;
}

void FileDownloader::start()
{
    emit downloadStarting();

    auto uniqueText = QByteArray::number(QRandomGenerator::global()->generate(), 16);
    QString tempFileName = QDir::tempPath() + "/.qds_" + uniqueText + "_download_" + url().fileName();

    m_outputFile.setFileName(tempFileName);
    m_outputFile.open(QIODevice::WriteOnly);

    QNetworkRequest request = makeRequest();
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(request);
    m_reply = reply;

    QNetworkReply::connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        bool isDownloadingFile = false;
        QString contentType;
        if (!reply->hasRawHeader("Content-Type")) {
            isDownloadingFile = true;
        } else {
            contentType = QString::fromUtf8(reply->rawHeader("Content-Type"));

            if (contentType.startsWith("application/")
                || contentType.startsWith("image/")
                || contentType.startsWith("binary/")) {
                isDownloadingFile = true;
            } else {
                qWarning() << "FileDownloader: Content type '" << contentType << "' is not supported";
            }
        }

        if (isDownloadingFile)
            m_outputFile.write(reply->readAll());
        else
            reply->close();
    });

    QNetworkReply::connect(reply,
                           &QNetworkReply::downloadProgress,
                           this,
                           [this](qint64 current, qint64 max) {
                               if (max <= 0) {
                                   // NOTE: according to doc, we might have the second arg
                                   // of QNetworkReply::downloadProgress less than 0.
                                   return;
                               }

                               m_progress = current * 100 / max;
                               emit progressChanged();
                           });

    QNetworkReply::connect(reply, &QNetworkReply::redirected, [reply](const QUrl &) {
        emit reply->redirectAllowed();
    });

    QNetworkReply::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error()) {
            if (reply->error() != QNetworkReply::OperationCanceledError) {
                qWarning() << Q_FUNC_INFO << m_url << reply->errorString();
                emit downloadFailed();
            } else {
                emit downloadCanceled();
            }
        } else {
            m_outputFile.flush();
            m_outputFile.close();

            QString dirPath = QFileInfo(m_targetFilePath).dir().absolutePath();
            if (!deleteFileAtTheEnd()) {
                if (!QDir{}.mkpath(dirPath)) {
                    emit downloadFailed();
                    return;
                }

                if (m_overwriteTarget && QFileInfo().exists(m_targetFilePath)) {
                    if (!QFile::remove(m_targetFilePath)) {
                        emit downloadFailed();
                        return;
                    }
                }

                if (!QFileInfo().exists(m_targetFilePath) && !m_outputFile.rename(m_targetFilePath)) {
                    emit downloadFailed();
                    return;
                }
            }

            m_finished = true;
            emit outputFileChanged();
            emit finishedChanged();
        }

        reply->deleteLater();
        m_reply = nullptr;
    });
}

void FileDownloader::setProbeUrl(bool value)
{
    if (m_probeUrl == value)
        return;

    m_probeUrl = value;
    emit probeUrlChanged();
}

bool FileDownloader::probeUrl() const
{
    return m_probeUrl;
}

void FileDownloader::cancel()
{
    if (m_reply)
        m_reply->abort();
}

void FileDownloader::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        emit urlChanged();
    }

    if (m_probeUrl)
        doProbeUrl();
}

QUrl FileDownloader::url() const
{
    return m_url;
}

void FileDownloader::setDownloadEnabled(bool value)
{
    if (m_downloadEnabled == value)
        return;

    m_downloadEnabled = value;
    emit downloadEnabledChanged();

    if (!m_url.isEmpty() && m_probeUrl)
        doProbeUrl();
}

bool FileDownloader::downloadEnabled() const
{
    return m_downloadEnabled;
}

bool FileDownloader::overwriteTarget() const
{
    return m_overwriteTarget;
}

void FileDownloader::setOverwriteTarget(bool value)
{
    if (value != m_overwriteTarget) {
        m_overwriteTarget = value;
        emit overwriteTargetChanged();
    }
}

bool FileDownloader::finished() const
{
    return m_finished;
}

bool FileDownloader::error() const
{
    return m_error;
}

QString FileDownloader::name() const
{
    const QFileInfo fileInfo(m_url.path());
    return fileInfo.baseName();
}

QString FileDownloader::completeBaseName() const
{
    return QFileInfo(m_url.path()).completeBaseName();
}

int FileDownloader::progress() const
{
    return m_progress;
}

QString FileDownloader::outputFile() const
{
    return QFileInfo(m_outputFile).canonicalFilePath();
}

QDateTime FileDownloader::lastModified() const
{
    return m_lastModified;
}

bool FileDownloader::available() const
{
    return m_available;
}

void FileDownloader::doProbeUrl()
{
    if (!m_probeUrl)
        return;

    if (!m_downloadEnabled) {
        m_available = false;
        emit availableChanged();
        return;
    }

    QNetworkRequest request = makeRequest();
    QNetworkReply *reply = Utils::NetworkAccessManager::instance()->head(request);

    QNetworkReply::connect(reply, &QNetworkReply::redirected, [reply](const QUrl &) {
        emit reply->redirectAllowed();
    });

    QNetworkReply::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error())
            return;

        m_lastModified = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
        emit lastModifiedChanged();

        m_available = true;
        emit availableChanged();

        reply->deleteLater();
    });

    QNetworkReply::connect(reply,
                           &QNetworkReply::errorOccurred,
                           this,
                           [this](QNetworkReply::NetworkError code) {

                               if (QQmlData::wasDeleted(this)) {
                                   qDebug() << Q_FUNC_INFO << "FileDownloader was deleted.";
                                   return;
                               }

                               qDebug() << Q_FUNC_INFO << "Network error:" << code
                                        << qobject_cast<QNetworkReply *>(sender())->errorString();

                               m_available = false;
                               emit availableChanged();
                           });
}

void FileDownloader::setTargetFilePath(const QString &path)
{
    m_targetFilePath = path;
}

QString FileDownloader::targetFilePath() const
{
    return m_targetFilePath;
}

} // namespace QmlDesigner
