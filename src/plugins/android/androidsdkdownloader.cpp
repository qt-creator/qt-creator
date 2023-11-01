// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidtr.h"

#include <solutions/tasking/networkquery.h>

#include <utils/filepath.h>
#include <utils/networkaccessmanager.h>
#include <utils/unarchiver.h>

#include <coreplugin/icore.h>

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QProgressDialog>
#include <QStandardPaths>

using namespace Utils;

namespace { Q_LOGGING_CATEGORY(sdkDownloaderLog, "qtc.android.sdkDownloader", QtWarningMsg) }

namespace Android::Internal {
/**
 * @class SdkDownloader
 * @brief Download Android SDK tools package from within Qt Creator.
 */
AndroidSdkDownloader::AndroidSdkDownloader()
    : m_androidConfig(AndroidConfigurations::currentConfig())
{}

AndroidSdkDownloader::~AndroidSdkDownloader() = default;

static bool isHttpRedirect(QNetworkReply *reply)
{
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 305
           || statusCode == 307 || statusCode == 308;
}

static FilePath sdkFromUrl(const QUrl &url)
{
    const QString path = url.path();
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "sdk-tools.zip";

    if (QFileInfo::exists(basename)) {
        int i = 0;
        basename += '.';
        while (QFileInfo::exists(basename + QString::number(i)))
            ++i;
        basename += QString::number(i);
    }

    return FilePath::fromString(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
           / basename;
}

// TODO: Make it a separate async task in a chain?
static std::optional<QString> saveToDisk(const FilePath &filename, QIODevice *data)
{
    QFile file(filename.toString());
    if (!file.open(QIODevice::WriteOnly)) {
        return Tr::tr("Could not open \"%1\" for writing: %2.")
            .arg(filename.toUserOutput(), file.errorString());
    }
    file.write(data->readAll());
    return {};
}

// TODO: Make it a separate async task in a chain?
static bool verifyFileIntegrity(const FilePath fileName, const QByteArray &sha256)
{
    QFile file(fileName.toString());
    if (file.open(QFile::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        if (hash.addData(&file))
            return hash.result() == sha256;
    }
    return false;
}

void AndroidSdkDownloader::downloadAndExtractSdk()
{
    if (m_androidConfig.sdkToolsUrl().isEmpty()) {
        logError(Tr::tr("The SDK Tools download URL is empty."));
        return;
    }

    m_progressDialog.reset(new QProgressDialog(Tr::tr("Downloading SDK Tools package..."),
                                               Tr::tr("Cancel"), 0, 100, Core::ICore::dialogParent()));
    m_progressDialog->setWindowModality(Qt::ApplicationModal);
    m_progressDialog->setWindowTitle(dialogTitle());
    m_progressDialog->setFixedSize(m_progressDialog->sizeHint());
    m_progressDialog->setAutoClose(false);
    connect(m_progressDialog.get(), &QProgressDialog::canceled, this, [this] {
        m_progressDialog.release()->deleteLater();
        m_taskTree.reset();
    });

    using namespace Tasking;

    TreeStorage<std::optional<FilePath>> storage;

    const auto onQuerySetup = [this](NetworkQuery &query) {
        query.setRequest(QNetworkRequest(m_androidConfig.sdkToolsUrl()));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        NetworkQuery *queryPtr = &query;
        connect(queryPtr, &NetworkQuery::started, this, [this, queryPtr] {
            QNetworkReply *reply = queryPtr->reply();
            if (!reply)
                return;
            connect(reply, &QNetworkReply::downloadProgress,
                    this, [this](qint64 received, qint64 max) {
                m_progressDialog->setRange(0, max);
                m_progressDialog->setValue(received);
            });
#if QT_CONFIG(ssl)
            connect(reply, &QNetworkReply::sslErrors,
                    this, [this, reply](const QList<QSslError> &sslErrors) {
                for (const QSslError &error : sslErrors)
                    qCDebug(sdkDownloaderLog, "SSL error: %s\n", qPrintable(error.errorString()));
                logError(Tr::tr("Encountered SSL errors, download is aborted."));
                reply->abort();
            });
#endif
        });
    };
    const auto onQueryDone = [this, storage](const NetworkQuery &query) {
        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return);
        const QUrl url = reply->url();
        if (isHttpRedirect(reply)) {
            logError(Tr::tr("Download from %1 was redirected.").arg(url.toString()));
            return;
        }
        const FilePath sdkFileName = sdkFromUrl(url);
        const std::optional<QString> saveResult = saveToDisk(sdkFileName, reply);
        if (saveResult) {
            logError(*saveResult);
            return;
        }
        *storage = sdkFileName;
    };
    const auto onQueryError = [this](const NetworkQuery &query) {
        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return);
        const QUrl url = reply->url();
        logError(Tr::tr("Downloading Android SDK Tools from URL %1 has failed: %2.")
                            .arg(url.toString(), reply->errorString()));
    };

    const auto onUnarchiveSetup = [this, storage](Unarchiver &unarchiver) {
        m_progressDialog->setRange(0, 0);
        m_progressDialog->setLabelText(Tr::tr("Unarchiving SDK Tools package..."));
        if (!*storage)
            return SetupResult::StopWithError;
        const FilePath sdkFileName = **storage;
        if (!verifyFileIntegrity(sdkFileName, m_androidConfig.getSdkToolsSha256())) {
            logError(Tr::tr("Verifying the integrity of the downloaded file has failed."));
            return SetupResult::StopWithError;
        }
        const auto sourceAndCommand = Unarchiver::sourceAndCommand(sdkFileName);
        if (!sourceAndCommand) {
            logError(sourceAndCommand.error());
            return SetupResult::StopWithError;
        }
        unarchiver.setSourceAndCommand(*sourceAndCommand);
        unarchiver.setDestDir(sdkFileName.parentDir());
        return SetupResult::Continue;
    };
    const auto onUnarchiverDone = [this, storage](const Unarchiver &) {
        m_androidConfig.setTemporarySdkToolsPath(
            (*storage)->parentDir().pathAppended(Constants::cmdlineToolsName));
        QMetaObject::invokeMethod(this, [this] { emit sdkExtracted(); }, Qt::QueuedConnection);
    };
    const auto onUnarchiverError = [this](const Unarchiver &) {
        logError(Tr::tr("Unarchiving error."));
    };

    const Group root {
        Tasking::Storage(storage),
        NetworkQueryTask(onQuerySetup, onQueryDone, onQueryError),
        UnarchiverTask(onUnarchiveSetup, onUnarchiverDone, onUnarchiverError)
    };

    m_taskTree.reset(new TaskTree(root));
    const auto onDone = [this] {
        m_taskTree.release()->deleteLater();
        m_progressDialog.reset();
    };
    connect(m_taskTree.get(), &TaskTree::done, this, onDone);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, onDone);
    m_taskTree->start();
}

QString AndroidSdkDownloader::dialogTitle()
{
    return Tr::tr("Download SDK Tools");
}

void AndroidSdkDownloader::logError(const QString &error)
{
    qCDebug(sdkDownloaderLog, "%s", error.toUtf8().data());
    QMetaObject::invokeMethod(this, [this, error] { emit sdkDownloaderError(error); },
        Qt::QueuedConnection);
}

} // namespace Android::Internal
