// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidtr.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/networkquery.h>

#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/networkaccessmanager.h>
#include <utils/unarchiver.h>

#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStandardPaths>

using namespace Tasking;
using namespace Utils;

namespace { Q_LOGGING_CATEGORY(sdkDownloaderLog, "qtc.android.sdkDownloader", QtWarningMsg) }

namespace Android::Internal {

static void logError(const QString &error)
{
    qCDebug(sdkDownloaderLog, "%s", error.toUtf8().data());
    QMessageBox::warning(Core::ICore::dialogParent(), dialogTitle(), error);
}

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
    const expected_str<qint64> result = filename.writeFileContents(data->readAll());
    if (!result) {
        return Tr::tr("Could not open \"%1\" for writing: %2.")
            .arg(filename.toUserOutput(), result.error());
    }

    return {};
}

static void validateFileIntegrity(QPromise<void> &promise, const FilePath &fileName,
                                  const QByteArray &sha256)
{
    const expected_str<QByteArray> result = fileName.fileContents();
    if (result) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(*result);
        if (hash.result() == sha256)
            return;
    }
    promise.future().cancel();
}

GroupItem downloadSdkRecipe()
{
    struct StorageStruct
    {
        StorageStruct() {
            progressDialog.reset(new QProgressDialog(Tr::tr("Downloading SDK Tools package..."),
                                 Tr::tr("Cancel"), 0, 100, Core::ICore::dialogParent()));
            progressDialog->setWindowModality(Qt::ApplicationModal);
            progressDialog->setWindowTitle(dialogTitle());
            progressDialog->setFixedSize(progressDialog->sizeHint());
            progressDialog->setAutoClose(false);
            progressDialog->show(); // TODO: Should not be needed. Investigate possible QT_BUG
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        std::optional<FilePath> sdkFileName;
    };

    Storage<StorageStruct> storage;

    const auto onSetup = [] {
        if (AndroidConfig::sdkToolsUrl().isEmpty()) {
            logError(Tr::tr("The SDK Tools download URL is empty."));
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    const auto onQuerySetup = [storage](NetworkQuery &query) {
        query.setRequest(QNetworkRequest(AndroidConfig::sdkToolsUrl()));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        QProgressDialog *progressDialog = storage->progressDialog.get();
        QObject::connect(&query, &NetworkQuery::downloadProgress,
                         progressDialog, [progressDialog](qint64 received, qint64 max) {
            progressDialog->setRange(0, max);
            progressDialog->setValue(received);
        });
#if QT_CONFIG(ssl)
        QObject::connect(&query, &NetworkQuery::sslErrors,
                         &query, [queryPtr = &query](const QList<QSslError> &sslErrors) {
            for (const QSslError &error : sslErrors)
                qCDebug(sdkDownloaderLog, "SSL error: %s\n", qPrintable(error.errorString()));
            logError(Tr::tr("Encountered SSL errors, download is aborted."));
            queryPtr->reply()->abort();
        });
#endif
    };
    const auto onQueryDone = [storage](const NetworkQuery &query, DoneWith result) {
        if (result == DoneWith::Cancel)
            return;

        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return);
        const QUrl url = reply->url();
        if (result != DoneWith::Success) {
            logError(Tr::tr("Downloading Android SDK Tools from URL %1 has failed: %2.")
                         .arg(url.toString(), reply->errorString()));
            return;
        }
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
        storage->sdkFileName = sdkFileName;
    };
    const auto onValidationSetup = [storage](Async<void> &async) {
        if (!storage->sdkFileName)
            return SetupResult::StopWithError;
        async.setConcurrentCallData(validateFileIntegrity, *storage->sdkFileName,
                                    AndroidConfig::getSdkToolsSha256());
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Verifying package integrity..."));
        return SetupResult::Continue;
    };
    const auto onValidationDone = [](DoneWith result) {
        if (result != DoneWith::Error)
            return;
        logError(Tr::tr("Verifying the integrity of the downloaded file has failed."));
    };
    const auto onUnarchiveSetup = [storage](Unarchiver &unarchiver) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Unarchiving SDK Tools package..."));
        const FilePath sdkFileName = *storage->sdkFileName;
        const auto sourceAndCommand = Unarchiver::sourceAndCommand(sdkFileName);
        if (!sourceAndCommand) {
            logError(sourceAndCommand.error());
            return SetupResult::StopWithError;
        }
        unarchiver.setSourceAndCommand(*sourceAndCommand);
        unarchiver.setDestDir(sdkFileName.parentDir());
        return SetupResult::Continue;
    };
    const auto onUnarchiverDone = [storage](DoneWith result) {
        if (result == DoneWith::Cancel)
            return;

        if (result != DoneWith::Success) {
            logError(Tr::tr("Unarchiving error."));
            return;
        }
        AndroidConfig::setTemporarySdkToolsPath(
            storage->sdkFileName->parentDir().pathAppended(Constants::cmdlineToolsName));
    };
    const auto onCancelSetup = [storage] { return std::make_pair(storage->progressDialog.get(),
                                                                 &QProgressDialog::canceled); };

    return Group {
        storage,
        Group {
            onGroupSetup(onSetup),
            NetworkQueryTask(onQuerySetup, onQueryDone),
            AsyncTask<void>(onValidationSetup, onValidationDone),
            UnarchiverTask(onUnarchiveSetup, onUnarchiverDone)
        }.withCancel(onCancelSetup)
    };
}

QString dialogTitle() { return Tr::tr("Download SDK Tools"); }

} // namespace Android::Internal
