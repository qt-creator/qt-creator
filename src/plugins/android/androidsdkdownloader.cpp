// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidtr.h"

#include <utils/archive.h>
#include <utils/filepath.h>

#include <coreplugin/icore.h>

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QStandardPaths>

using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(sdkDownloaderLog, "qtc.android.sdkDownloader", QtWarningMsg)
}

namespace Android {
namespace Internal {
/**
 * @class SdkDownloader
 * @brief Download Android SDK tools package from within Qt Creator.
 */
AndroidSdkDownloader::AndroidSdkDownloader()
    : m_androidConfig(AndroidConfigurations::currentConfig())
{
    connect(&m_manager, &QNetworkAccessManager::finished, this, &AndroidSdkDownloader::downloadFinished);
}

AndroidSdkDownloader::~AndroidSdkDownloader() = default;

#if QT_CONFIG(ssl)
void AndroidSdkDownloader::sslErrors(const QList<QSslError> &sslErrors)
{
    for (const QSslError &error : sslErrors)
        qCDebug(sdkDownloaderLog, "SSL error: %s\n", qPrintable(error.errorString()));
    cancelWithError(Tr::tr("Encountered SSL errors, download is aborted."));
}
#endif

void AndroidSdkDownloader::downloadAndExtractSdk()
{
    if (m_androidConfig.sdkToolsUrl().isEmpty()) {
        logError(Tr::tr("The SDK Tools download URL is empty."));
        return;
    }

    QNetworkRequest request(m_androidConfig.sdkToolsUrl());
    m_reply = m_manager.get(request);

#if QT_CONFIG(ssl)
    connect(m_reply, &QNetworkReply::sslErrors, this, &AndroidSdkDownloader::sslErrors);
#endif

    m_progressDialog = new QProgressDialog(Tr::tr("Downloading SDK Tools package..."), Tr::tr("Cancel"),
                                           0, 100, Core::ICore::dialogParent());
    m_progressDialog->setWindowModality(Qt::ApplicationModal);
    m_progressDialog->setWindowTitle(dialogTitle());
    m_progressDialog->setFixedSize(m_progressDialog->sizeHint());

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 max) {
        m_progressDialog->setRange(0, max);
        m_progressDialog->setValue(received);
    });

    connect(m_progressDialog, &QProgressDialog::canceled, this, &AndroidSdkDownloader::cancel);

    connect(this, &AndroidSdkDownloader::sdkPackageWriteFinished, this, [this] {
        if (!Archive::supportsFile(m_sdkFilename))
            return;
        const FilePath extractDir = m_sdkFilename.parentDir();
        m_archive.reset(new Archive(m_sdkFilename, extractDir));
        if (m_archive->isValid()) {
            connect(m_archive.get(), &Archive::finished, this, [this, extractDir](bool success) {
                if (success) {
                    // Save the extraction path temporarily which can be used by sdkmanager
                    // to install essential packages at firt time setup.
                    m_androidConfig.setTemporarySdkToolsPath(
                                extractDir.pathAppended(Constants::cmdlineToolsName));
                    emit sdkExtracted();
                }
                m_archive.release()->deleteLater();
            });
            m_archive->unarchive();
        }
    });
}

bool AndroidSdkDownloader::verifyFileIntegrity()
{
    QFile f(m_sdkFilename.toString());
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&f)) {
            return hash.result() == m_androidConfig.getSdkToolsSha256();
        }
    }
    return false;
}

QString AndroidSdkDownloader::dialogTitle()
{
    return Tr::tr("Download SDK Tools");
}

void AndroidSdkDownloader::cancel()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
    if (m_progressDialog)
        m_progressDialog->cancel();
}

void AndroidSdkDownloader::cancelWithError(const QString &error)
{
    cancel();
    logError(error);
}

void AndroidSdkDownloader::logError(const QString &error)
{
    qCDebug(sdkDownloaderLog, "%s", error.toUtf8().data());
    emit sdkDownloaderError(error);
}

FilePath AndroidSdkDownloader::getSaveFilename(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "sdk-tools.zip";

    if (QFile::exists(basename)) {
        int i = 0;
        basename += '.';
        while (QFile::exists(basename + QString::number(i)))
            ++i;
        basename += QString::number(i);
    }

    return FilePath::fromString(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            / basename;
}

bool AndroidSdkDownloader::saveToDisk(const FilePath &filename, QIODevice *data)
{
    QFile file(filename.toString());
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString(Tr::tr("Could not open %1 for writing: %2."))
                     .arg(filename.toUserOutput(), file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

bool AndroidSdkDownloader::isHttpRedirect(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 305
           || statusCode == 307 || statusCode == 308;
}

void AndroidSdkDownloader::downloadFinished(QNetworkReply *reply)
{
    QUrl url = reply->url();
    if (reply->error()) {
        cancelWithError(QString(Tr::tr("Downloading Android SDK Tools from URL %1 has failed: %2."))
                            .arg(url.toString(), reply->errorString()));
    } else {
        if (isHttpRedirect(reply)) {
            cancelWithError(QString(Tr::tr("Download from %1 was redirected.")).arg(url.toString()));
        } else {
            m_sdkFilename = getSaveFilename(url);
            if (saveToDisk(m_sdkFilename, reply) && verifyFileIntegrity())
                emit sdkPackageWriteFinished();
            else
                cancelWithError(
                    Tr::tr("Writing and verifying the integrity of the downloaded file has failed."));
        }
    }

    reply->deleteLater();
}

} // Internal
} // Android
