// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "consolehost.h"

#include "terminalsettings.h"
#include "terminaltr.h"

#include <coreplugin/icore.h>

#include <QtTaskTree/QNetworkReplyWrapper>

#include <utils/async.h>
#include <utils/networkaccessmanager.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/unarchiver.h>
#include <utils/widgets.h>

#include <QCryptographicHash>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSysInfo>

using namespace QtTaskTree;
using namespace Utils;

namespace Terminal::Internal::ConsoleHost {

// Microsoft's ConPTY, the one Windows Terminal is built with. A published
// package never changes, so the checksum belongs to this version and no other.
const char packageId[] = "microsoft.windows.console.conpty";
const char packageVersion[] = "1.24.260710001";
const char packageSha256[] = "175640566a3b59c4b132070ee96c2c77e5ab7edd2e92732a5eb3610bbf63d90e";
const char packageProject[] = "https://github.com/microsoft/terminal";
const char packageLicense[] = "https://licenses.nuget.org/MIT";

// conpty.dll starts the OpenConsole.exe next to it, so the two are installed
// together and either one is worth nothing without the other.
const char libraryName[] = "conpty.dll";
const char hostName[] = "OpenConsole.exe";

QString dialogTitle()
{
    return Tr::tr("Download Console Host");
}

QString version()
{
    return QString::fromLatin1(packageVersion);
}

static QString architecture()
{
    const QString architecture = QSysInfo::buildCpuArchitecture();
    if (architecture == "arm64")
        return "arm64";
    if (architecture == "x86_64")
        return "x64";
    if (architecture == "i386")
        return "x86";
    return {};
}

bool isSupportedPlatform()
{
    return HostOsInfo::isWindowsHost() && !architecture().isEmpty();
}

FilePath downloadDirectory()
{
    return Core::ICore::userResourcePath("conpty") / version();
}

static bool holdsConsoleHost(const FilePath &directory)
{
    return (directory / libraryName).isFile() && (directory / hostName).isFile();
}

bool isDownloaded()
{
    return holdsConsoleHost(downloadDirectory());
}

FilePath inUse()
{
    return Pty::consoleHostDirectory();
}

void apply()
{
    if (!isSupportedPlatform())
        return;

    // A directory that has been emptied or is left over from a version that is
    // gone falls back to the downloaded one instead of turning the terminal
    // back into one without images.
    const FilePath fromSettings = settings().consoleHostDirectory();
    if (holdsConsoleHost(fromSettings))
        Pty::setConsoleHostDirectory(fromSettings);
    else if (isDownloaded())
        Pty::setConsoleHostDirectory(downloadDirectory());
    else
        Pty::setConsoleHostDirectory({});
}

static void warn(const QString &error)
{
    QMessageBox::warning(Core::ICore::dialogParent(), dialogTitle(), error);
}

static QString link(const char *url)
{
    return QString("<a href=\"%1\">%1</a>").arg(QLatin1String(url));
}

static bool acceptLicense()
{
    const QString text
        = "<p>" + Tr::tr("Download the console host %1 from nuget.org?").arg(version())
          + "</p><p>"
          + Tr::tr("A program running in the terminal can print a picture as well as text. "
                   "The console host that comes with Windows passes on only the text, so the "
                   "picture never arrives. This one, which Microsoft publishes under the MIT "
                   "license, passes on both.")
          + "</p><p>" + Tr::tr("Project: %1").arg(link(packageProject)) + "<br/>"
          + Tr::tr("License: %1").arg(link(packageLicense)) + "</p>";

    QMessageBox box(QMessageBox::Question,
                    dialogTitle(),
                    text,
                    QMessageBox::Cancel,
                    Core::ICore::dialogParent());
    box.setTextFormat(Qt::RichText);
    box.addButton(Tr::tr("Download"), QMessageBox::AcceptRole);

    return box.exec() != QMessageBox::Cancel;
}

static void verifyChecksum(QPromise<void> &promise, const FilePath &package)
{
    const Result<QByteArray> contents = package.fileContents();
    if (contents) {
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(*contents);
        if (hash.result().toHex() == QByteArray(packageSha256))
            return;
    }
    promise.future().cancel();
}

// Takes the two files this machine's architecture needs out of the unpacked
// package and leaves them where the terminal looks for them.
static Result<> install(const FilePath &unpacked)
{
    const QString architecture = ConsoleHost::architecture();
    const FilePath library = unpacked / "runtimes" / ("win-" + architecture) / "native"
                             / libraryName;
    const FilePath host = unpacked / "build" / "native" / "runtimes" / architecture / hostName;

    if (!library.isFile() || !host.isFile())
        return ResultError(Tr::tr("The package holds no console host for %1.").arg(architecture));

    const FilePath directory = downloadDirectory();
    if (const Result<> result = directory.ensureWritableDir(); !result)
        return result;

    for (const FilePath &source : {library, host}) {
        const FilePath target = directory / source.fileName();
        target.removeFile();
        if (const Result<> result = source.copyFile(target); !result)
            return result;
    }

    // Whatever is left of the versions downloaded before is of no use to anyone
    const FilePaths others = directory.parentDir().dirEntries(
        DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot);
    for (const FilePath &other : others) {
        if (other != directory)
            other.removeRecursively();
    }

    return ResultOk;
}

GroupItem downloadRecipe()
{
    struct StorageStruct
    {
        std::unique_ptr<QProgressDialog> progressDialog;
        std::unique_ptr<TemporaryDirectory> temporaryDirectory;
        FilePath package;
    };

    const Storage<StorageStruct> storage;

    const auto onSetup = [storage] {
        if (!isSupportedPlatform()) {
            warn(Tr::tr("There is no console host for this platform."));
            return SetupResult::StopWithError;
        }
        if (!acceptLicense())
            return SetupResult::StopWithError;

        storage->temporaryDirectory = std::make_unique<TemporaryDirectory>("qtc-conpty-XXXXXX");
        storage->package = storage->temporaryDirectory->filePath("conpty.nupkg");
        storage->progressDialog.reset(
            createProgressDialog(100, dialogTitle(), Tr::tr("Downloading console host...")));
        return SetupResult::Continue;
    };

    const auto onQuerySetup = [storage](QNetworkReplyWrapper &query) {
        const QString url = QString("https://api.nuget.org/v3-flatcontainer/%1/%2/%1.%2.nupkg")
                                .arg(QLatin1String(packageId), version());
        query.setRequest(QNetworkRequest(QUrl(url)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());

        QProgressDialog *progressDialog = storage->progressDialog.get();
        QObject::connect(&query,
                         &QNetworkReplyWrapper::downloadProgress,
                         progressDialog,
                         [progressDialog](qint64 received, qint64 max) {
                             progressDialog->setRange(0, max);
                             progressDialog->setValue(received);
                         });
    };
    const auto onQueryDone = [storage](const QNetworkReplyWrapper &query, DoneWith result) {
        if (result == DoneWith::Cancel)
            return;

        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return);
        if (result != DoneWith::Success) {
            warn(Tr::tr("Downloading the console host failed: %1").arg(reply->errorString()));
            storage->package.clear();
            return;
        }

        const Result<qint64> written = storage->package.writeFileContents(reply->readAll());
        if (!written) {
            warn(written.error());
            storage->package.clear();
        }
    };

    const auto onVerifySetup = [storage](Async<void> &async) {
        if (storage->package.isEmpty())
            return SetupResult::StopWithError;

        async.setConcurrentCallData(verifyChecksum, storage->package);
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Verifying package integrity..."));
        return SetupResult::Continue;
    };
    const auto onVerifyDone = [](DoneWith result) {
        if (result == DoneWith::Error)
            warn(Tr::tr("The downloaded package is not the one that was expected."));
    };

    const auto onUnarchiveSetup = [storage](Unarchiver &task) {
        storage->progressDialog->setLabelText(Tr::tr("Unpacking console host..."));
        task.setArchive(storage->package);
        task.setDestination(storage->temporaryDirectory->path());
    };
    const auto onUnarchiveDone = [storage](const Unarchiver &task) {
        const Result<> unpacked = task.result();
        if (!unpacked) {
            warn(Tr::tr("Unpacking the console host failed: %1").arg(unpacked.error()));
            return DoneResult::Error;
        }

        const Result<> installed = install(storage->temporaryDirectory->path());
        if (!installed) {
            warn(installed.error());
            return DoneResult::Error;
        }

        apply();
        return DoneResult::Success;
    };

    const auto onCancelSetup = [storage] {
        return makeObjectSignal(storage->progressDialog.get(), &QProgressDialog::canceled);
    };

    return Group {
        storage,
        Group {
            onGroupSetup(onSetup),
            QNetworkReplyWrapperTask(onQuerySetup, onQueryDone),
            AsyncTask<void>(onVerifySetup, onVerifyDone),
            UnarchiverTask(onUnarchiveSetup, onUnarchiveDone)
        }.withCancel(onCancelSetup)
    };
}

} // namespace Terminal::Internal::ConsoleHost
