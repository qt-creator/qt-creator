// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "filedownloader.h"

#include <private/qqmldata_p.h>
#include <utils/networkaccessmanager.h>

#include <QDir>
#include <QQmlEngine>

namespace QmlDesigner {

FileDownloader::FileDownloader(QObject *parent)
    : QObject(parent)
{}

FileDownloader::~FileDownloader()
{
    if (m_tempFile.exists())
        m_tempFile.remove();
}

void FileDownloader::start()
{
    emit downloadStarting();

    m_tempFile.setFileName(QDir::tempPath() + "/" + name() + ".XXXXXX" + ".zip");
    m_tempFile.open(QIODevice::WriteOnly);

    auto request = QNetworkRequest(m_url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::UserVerifiedRedirectPolicy);
    m_reply = Utils::NetworkAccessManager::instance()->get(request);

    QNetworkReply::connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        bool isDownloadingFile = false;
        QString contentType;
        if (!m_reply->hasRawHeader("Content-Type")) {
            isDownloadingFile = true;
        } else {
            contentType = QString::fromUtf8(m_reply->rawHeader("Content-Type"));

            if (contentType.startsWith("application/")
                || contentType.startsWith("image/")
                || contentType.startsWith("binary/")) {
                isDownloadingFile = true;
            } else {
                qWarning() << "FileDownloader: Content type '" << contentType << "' is not supported";
            }
        }

        if (isDownloadingFile)
            m_tempFile.write(m_reply->readAll());
        else
            m_reply->close();
    });

    QNetworkReply::connect(m_reply,
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

    QNetworkReply::connect(m_reply, &QNetworkReply::redirected, [this](const QUrl &) {
        emit m_reply->redirectAllowed();
    });

    QNetworkReply::connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (m_reply->error()) {
            if (m_tempFile.exists())
                m_tempFile.remove();

            if (m_reply->error() != QNetworkReply::OperationCanceledError) {
                qWarning() << Q_FUNC_INFO << m_url << m_reply->errorString();
                emit downloadFailed();
            } else {
                emit downloadCanceled();
            }
        } else {
            m_tempFile.flush();
            m_tempFile.close();
            m_finished = true;
            emit tempFileChanged();
            emit finishedChanged();
        }

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

QString FileDownloader::tempFile() const
{
    return QFileInfo(m_tempFile).canonicalFilePath();
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

    auto request = QNetworkRequest(m_url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::UserVerifiedRedirectPolicy);
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
    });

    QNetworkReply::connect(reply,
                           &QNetworkReply::errorOccurred,
                           this,
                           [this](QNetworkReply::NetworkError) {
                               QQmlData *data = QQmlData::get(this, false);
                               if (!data) {
                                   qDebug() << Q_FUNC_INFO << "FileDownloader is nullptr.";
                                   return;
                               }

                               if (QQmlData::wasDeleted(this)) {
                                   qDebug() << Q_FUNC_INFO << "FileDownloader was deleted.";
                                   return;
                               }

                               m_available = false;
                               emit availableChanged();
                           });
}

} // namespace QmlDesigner
