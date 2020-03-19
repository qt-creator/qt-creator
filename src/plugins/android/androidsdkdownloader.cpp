/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidsdkdownloader.h"

#include <QDir>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QProcess>
#include <QCryptographicHash>
#include <QStandardPaths>

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
{
    m_androidConfig = AndroidConfigurations::currentConfig();
    connect(&m_manager, &QNetworkAccessManager::finished, this, &AndroidSdkDownloader::downloadFinished);
}

#if QT_CONFIG(ssl)
void AndroidSdkDownloader::sslErrors(const QList<QSslError> &sslErrors)
{
    for (const QSslError &error : sslErrors)
        qCDebug(sdkDownloaderLog, "SSL error: %s\n", qPrintable(error.errorString()));
    cancelWithError(tr("Encountered SSL errors, download is aborted."));
}
#endif

static void setSdkFilesExecPermission( const QString &sdkExtractPath)
{
    QDirIterator it(sdkExtractPath + "/tools", QStringList() << "*",
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFile file(it.next());
        if (!file.fileName().contains('.')) {
            QFlags<QFileDevice::Permission> currentPermissions
                = file.permissions();
            file.setPermissions(currentPermissions | QFileDevice::ExeOwner);
        }
    }
}

void AndroidSdkDownloader::downloadAndExtractSdk(const QString &jdkPath, const QString &sdkExtractPath)
{
    if (m_androidConfig.sdkToolsUrl().isEmpty()) {
        logError(tr("The SDK Tools download URL is empty."));
        return;
    }

    QNetworkRequest request(m_androidConfig.sdkToolsUrl());
    m_reply = m_manager.get(request);

#if QT_CONFIG(ssl)
    connect(m_reply, &QNetworkReply::sslErrors, this, &AndroidSdkDownloader::sslErrors);
#endif

    m_progressDialog = new QProgressDialog(tr("Downloading SDK Tools package..."), tr("Cancel"), 0, 100);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setWindowTitle(dialogTitle());
    m_progressDialog->setFixedSize(m_progressDialog->sizeHint());

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 max) {
        m_progressDialog->setRange(0, max);
        m_progressDialog->setValue(received);
    });

    connect(m_progressDialog, &QProgressDialog::canceled, this, &AndroidSdkDownloader::cancel);

    connect(this, &AndroidSdkDownloader::sdkPackageWriteFinished, this, [this, jdkPath, sdkExtractPath]() {
        if (extractSdk(jdkPath, sdkExtractPath)) {
            setSdkFilesExecPermission(sdkExtractPath);
            emit sdkExtracted();
        }
    });
}

bool AndroidSdkDownloader::extractSdk(const QString &jdkPath, const QString &sdkExtractPath)
{
    QDir sdkDir = QDir(sdkExtractPath);
    if (!sdkDir.exists()) {
        if (!sdkDir.mkpath(".")) {
            logError(QString(tr("Could not create the SDK folder %1.")).arg(sdkExtractPath));
            return false;
        }
    }

    QProcess *jarExtractProc = new QProcess();
    jarExtractProc->setWorkingDirectory(sdkExtractPath);
    QString jarCmdPath(jdkPath + "/bin/jar");
    jarExtractProc->start(jarCmdPath, {"xf", m_sdkFilename});
    jarExtractProc->waitForFinished();
    jarExtractProc->close();

    return jarExtractProc->exitCode() ? false : true;
}

bool AndroidSdkDownloader::verifyFileIntegrity()
{
    QFile f(m_sdkFilename);
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
    return tr("Download SDK Tools");
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

QString AndroidSdkDownloader::getSaveFilename(const QUrl &url)
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

    QString fullPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
                       + QDir::separator() + basename;
    return fullPath;
}

bool AndroidSdkDownloader::saveToDisk(const QString &filename, QIODevice *data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString(tr("Could not open %1 for writing: %2.")).arg(filename, file.errorString()));
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
        cancelWithError(QString(tr("Downloading Android SDK Tools from URL %1 has failed: %2."))
                            .arg(url.toString(), reply->errorString()));
    } else {
        if (isHttpRedirect(reply)) {
            cancelWithError(QString(tr("Download from %1 was redirected.")).arg(url.toString()));
        } else {
            m_sdkFilename = getSaveFilename(url);
            if (saveToDisk(m_sdkFilename, reply) && verifyFileIntegrity())
                emit sdkPackageWriteFinished();
            else
                cancelWithError(
                    tr("Writing and verifying the integrity of the downloaded file has failed."));
        }
    }

    reply->deleteLater();
}

} // Internal
} // Android
