// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyossettings.h"

#include "harmonyosconfigurations.h"
#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyostr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/qtasktree.h>

#include <updateinfo/updateinfoservice.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/unarchiver.h>
#include <utils/widgets.h>

#include <QCryptographicHash>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace HarmonyOs::Internal {

HarmonyOsSettings &settings()
{
    static HarmonyOsSettings theSettings;
    return theSettings;
}

// SDK download

static QString downloadTitle()
{
    return Tr::tr("Download HarmonyOS SDK");
}

static void reportDownloadError(const QString &error, const QString &title = downloadTitle())
{
    QMessageBox::warning(Core::ICore::dialogParent(), title, error);
}

static void verifyArchive(QPromise<Result<>> &promise, const FilePath &archive,
                          const QByteArray &checksum, QCryptographicHash::Algorithm algorithm)
{
    QFile file(archive.toFSPathString());
    QCryptographicHash hash(algorithm);
    if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file)) {
        promise.addResult(ResultError(Tr::tr("Cannot read \"%1\": %2.")
                                          .arg(archive.toUserOutput(), file.errorString())));
        return;
    }
    if (hash.result() != checksum) {
        promise.addResult(ResultError(
            Tr::tr("The checksum of the downloaded archive is not the published one.")));
        return;
    }
    promise.addResult(ResultOk);
}

// Gigabytes neither fit the progress dialog's int range nor memory, so the dialog counts
// megabytes and the reply is drained into the file as it arrives.
static constexpr qint64 megaByte = 1024 * 1024;

static void setupDownload(QtTaskTree::QNetworkReplyWrapper &query, const QUrl &url,
                          QProgressDialog *progressDialog, QFile *file, const QString &title)
{
    using namespace QtTaskTree;

    query.setRequest(QNetworkRequest(url));
    query.setNetworkAccessManager(NetworkAccessManager::instance());

    QObject::connect(&query, &QNetworkReplyWrapper::downloadProgress, progressDialog,
                     [progressDialog](qint64 received, qint64 total) {
        progressDialog->setRange(0, total / megaByte);
        progressDialog->setValue(received / megaByte);
    });
    QObject::connect(&query, &QNetworkReplyWrapper::started, &query, [file, queryPtr = &query] {
        QNetworkReply *reply = queryPtr->reply();
        QObject::connect(reply, &QNetworkReply::readyRead, reply, [file, reply] {
            if (file->write(reply->readAll()) < 0)
                reply->abort();
        });
    });
#if QT_CONFIG(ssl)
    QObject::connect(&query, &QNetworkReplyWrapper::sslErrors, &query,
                     [title, queryPtr = &query](const QList<QSslError> &errors) {
        reportDownloadError(Tr::tr("The download was canceled: %1")
                                .arg(errors.constFirst().errorString()), title);
        queryPtr->reply()->abort();
    });
#endif
}

// Installs the components of the public OpenHarmony SDK that Qt Creator reads into sdkPath.
// The archive they come in holds one directory per host and one zip per component in it,
// and is a few gigabytes, so it is streamed to disk and only the wanted zips are unpacked
// out of it.
static QtTaskTree::GroupItem downloadSdkRecipe(const FilePath &sdkPath)
{
    using namespace QtTaskTree;

    struct DownloadStorage
    {
        DownloadStorage()
        {
            progressDialog.reset(createProgressDialog(0, downloadTitle(),
                                                      Tr::tr("Downloading the SDK...")));
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        std::unique_ptr<QFile> file;
        FilePath archive;
        QByteArray sha256;
        QMap<QString, FilePath> components;
    };

    const Storage<DownloadStorage> storage;
    const Sdk::PublicSdk sdk = Sdk::publicSdk();
    const QStringList wanted = Sdk::publicSdkComponents();
    const FilePath workPath = sdkPath / "download";

    const auto onSetup = [storage, sdk, workPath] {
        if (sdk.url.isEmpty()) {
            reportDownloadError(Tr::tr("The public OpenHarmony SDK is published for Linux, "
                                       "Windows and macOS only."));
            return SetupResult::StopWithError;
        }
        if (const Result<> writable = workPath.ensureWritableDir(); !writable) {
            reportDownloadError(writable.error());
            return SetupResult::StopWithError;
        }
        storage->archive = workPath / sdk.url.fileName();
        storage->file.reset(new QFile(storage->archive.toFSPathString()));
        if (!storage->file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            reportDownloadError(Tr::tr("Cannot write \"%1\": %2.")
                                    .arg(storage->archive.toUserOutput(),
                                         storage->file->errorString()));
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    const auto onReplyDone = [](const QNetworkReplyWrapper &query, DoneWith result) {
        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return false);
        if (result == DoneWith::Success)
            return true;
        if (result == DoneWith::Error) {
            reportDownloadError(Tr::tr("Downloading \"%1\" failed: %2.")
                                    .arg(reply->url().toString(), reply->errorString()));
        }
        return false;
    };

    const auto onChecksumSetup = [sdk](QNetworkReplyWrapper &query) {
        query.setRequest(QNetworkRequest(QUrl(sdk.url.toString() + ".sha256")));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onChecksumDone = [storage, onReplyDone](const QNetworkReplyWrapper &query,
                                                       DoneWith result) {
        if (!onReplyDone(query, result))
            return DoneResult::Error;
        const QByteArray published = query.reply()->readAll().simplified();
        storage->sha256 = QByteArray::fromHex(published.split(' ').constFirst());
        if (storage->sha256.size()
            != QCryptographicHash::hashLength(QCryptographicHash::Sha256)) {
            reportDownloadError(Tr::tr("Cannot read the published checksum of the SDK archive."));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onDownloadSetup = [storage, sdk](QNetworkReplyWrapper &query) {
        setupDownload(query, sdk.url, storage->progressDialog.get(), storage->file.get(),
                      downloadTitle());
    };
    const auto onDownloadDone = [storage, onReplyDone](const QNetworkReplyWrapper &query,
                                                       DoneWith result) {
        const bool written = storage->file->flush();
        storage->file->close();
        if (!onReplyDone(query, result))
            return DoneResult::Error;
        if (!written) {
            reportDownloadError(Tr::tr("Cannot write \"%1\": %2.")
                                    .arg(storage->archive.toUserOutput(),
                                         storage->file->errorString()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onVerifySetup = [storage](Async<Result<>> &async) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Verifying the downloaded archive..."));
        async.setConcurrentCallData(verifyArchive, storage->archive, storage->sha256,
                                    QCryptographicHash::Sha256);
    };
    const auto onVerifyDone = [](const Async<Result<>> &async, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        if (!async.isResultAvailable()) {
            reportDownloadError(Tr::tr("Verifying the downloaded archive failed."));
            return DoneResult::Error;
        }
        if (const Result<> verified = async.result(); !verified) {
            reportDownloadError(verified.error());
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onSelectSetup = [storage, sdk, wanted, workPath](Unarchiver &task) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Reading the archive..."));
        task.setArchive(storage->archive);
        task.setDestination(workPath);
        // Where the host directory sits inside the archive differs between SDK versions,
        // so the entries are matched by name rather than by depth.
        const QRegularExpression pattern("(?:^|/)" + QRegularExpression::escape(sdk.hostDirectory)
                                         + "/(\\w+)-[^/]*\\.zip$");
        task.setFilter([storage, wanted, workPath, pattern](const QString &entry) {
            const QRegularExpressionMatch match = pattern.match(entry);
            if (!match.hasMatch() || !wanted.contains(match.captured(1)))
                return false;
            storage->components.insert(match.captured(1), workPath.resolvePath(entry));
            return true;
        });
    };
    const auto onSelectDone = [storage, wanted](const Unarchiver &task, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        if (const Result<> unpacked = task.result(); !unpacked) {
            reportDownloadError(Tr::tr("Unpacking the archive failed: %1").arg(unpacked.error()));
            return DoneResult::Error;
        }
        const QStringList missing = Utils::filtered(wanted, [storage](const QString &name) {
            return !storage->components.contains(name);
        });
        if (!missing.isEmpty()) {
            reportDownloadError(Tr::tr("The archive holds no %1 for this computer.")
                                    .arg(missing.join(", ")));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    // Which components there are is known only once the archive has been read, so the loop
    // asks the storage how far to go instead of iterating a list captured here.
    const UntilIterator componentIterator([storage](qsizetype iteration) {
        return iteration < storage->components.size();
    });

    const auto onComponentSetup = [storage, sdkPath, componentIterator](Unarchiver &task) {
        const FilePath component = storage->components.values().at(componentIterator.iteration());
        storage->progressDialog->setLabelText(Tr::tr("Unpacking %1...").arg(component.fileName()));
        task.setArchive(component);
        task.setDestination(sdkPath);
    };
    const auto onComponentDone = [](const Unarchiver &task, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        if (const Result<> unpacked = task.result(); !unpacked) {
            reportDownloadError(Tr::tr("Unpacking a component failed: %1").arg(unpacked.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onCancel = [storage] {
        return makeObjectSignal(storage->progressDialog.get(), &QProgressDialog::canceled);
    };

    return Group {
        storage,
        Group {
            onGroupSetup(onSetup),
            QNetworkReplyWrapperTask(onChecksumSetup, onChecksumDone),
            QNetworkReplyWrapperTask(onDownloadSetup, onDownloadDone),
            AsyncTask<Result<>>(onVerifySetup, onVerifyDone),
            UnarchiverTask(onSelectSetup, onSelectDone),
            For (componentIterator) >> Do {
                UnarchiverTask(onComponentSetup, onComponentDone),
            },
        }.withCancel(onCancel),
        onGroupDone([workPath] { workPath.removeRecursively(); }),
    };
}

// Qt installation

// Qt for HarmonyOS comes from the Qt installer, so installing one means driving the
// maintenance tool, which the updateinfo plugin owns. Taking its service out of the
// object pool keeps that a run-time rather than a build-time dependency, and leaves
// the tool's own location to the only place that knows it.
static UpdateInfo::Service *installerService()
{
    return ExtensionSystem::PluginManager::getObject<UpdateInfo::Service>();
}

// The packages the installer publishes Qt for HarmonyOS as, e.g.
// "qt.qt6.6120.harmonyos_arm64_v8a".
static QString qtPackageFilter()
{
    return "qt\\.qt6\\..*harmonyos.*";
}

static QtSupport::QtVersions harmonyOsQtVersions()
{
    return QtSupport::QtVersionManager::versions([](const QtSupport::QtVersion *version) {
        return version->type() == QLatin1String(Constants::HARMONYOS_QT_TYPE);
    });
}

struct ScanResult
{
    QtSupport::QtVersions added;
    // Registered, but not as a Qt for HarmonyOS, which is what the installer's own
    // registration leaves behind: it names no device type, so nothing builds a
    // HarmonyOS kit from such a Qt.
    QtSupport::QtVersions mistyped;
};

// A Qt whose qmake runs describes itself best, but one that was downloaded for the device
// Qt Creator itself runs on has a qmake that cannot run here, and then its files have to do.
static QtSupport::QtVersion *createQtVersion(const FilePath &prefix, const FilePath &qmake)
{
    const ProjectExplorer::DetectionSource source = ProjectExplorer::DetectionSource::FromSystem;
    QtSupport::QtVersion *version = qmake.isExecutableFile()
        ? QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmake, source)
        : nullptr;
    if (version && !version->isValid()) {
        delete version;
        version = nullptr;
    }
    if (!version)
        version = QtSupport::QtVersionFactory::createQtVersionFromPrefix(prefix, source);
    if (version && !version->isValid()) {
        delete version;
        return nullptr;
    }
    return version;
}

// Registers the Qt for HarmonyOS installations in root that are not known here yet.
static ScanResult registerQtVersions(const FilePath &root)
{
    ScanResult result;
    const DirFilterFlags dirs = DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot;
    for (const FilePath &versionDir : root.dirEntries({{"*"}, dirs})) {
        for (const FilePath &prefix : versionDir.dirEntries({{"harmonyos*"}, dirs})) {
            const FilePath qmake = (prefix / "bin" / "qmake").withExecutableSuffix();
            if (QtSupport::QtVersion *known = QtSupport::QtVersionManager::version(
                    [&qmake, &prefix](const QtSupport::QtVersion *version) {
                        return version->qmakeFilePath() == qmake
                               || version->qtFilePath() == prefix;
                    })) {
                if (known->type() != QLatin1String(Constants::HARMONYOS_QT_TYPE))
                    result.mistyped.append(known);
                continue;
            }
            QtSupport::QtVersion *version = createQtVersion(prefix, qmake);
            if (!version)
                continue;
            QtSupport::QtVersionManager::addVersion(version);
            result.added.append(version);
        }
    }
    return result;
}

static QString versionNames(const QtSupport::QtVersions &versions)
{
    return Utils::transform<QStringList>(versions,
        [](const QtSupport::QtVersion *version) { return version->displayName(); }).join(", ");
}

// Qt repository download

static QString qtDownloadTitle()
{
    return Tr::tr("Download Qt for HarmonyOS");
}

// Where a Qt Creator that has no maintenance tool puts a downloaded Qt: beside its own
// settings, the one directory it is sure to be allowed to write to.
static FilePath downloadedQtRoot()
{
    return Core::ICore::userResourcePath("harmonyos/qt");
}

// Installs the newest Qt for HarmonyOS a repository offers into "<root>/<version>/<target>",
// which is the layout the Qt installer produces as well. A repository is plain files over
// HTTP, so this way needs no installer.
static QtTaskTree::GroupItem downloadQtRecipe(const QUrl &repository, const FilePath &root)
{
    using namespace QtTaskTree;

    struct QtDownloadStorage
    {
        QtDownloadStorage()
        {
            progressDialog.reset(createProgressDialog(0, qtDownloadTitle(),
                                                      Tr::tr("Reading the repository...")));
        }
        std::unique_ptr<QProgressDialog> progressDialog;
        std::unique_ptr<QFile> file;
        Sdk::QtPackage package;
        FilePath archive;
        QByteArray sha1;
    };

    const Storage<QtDownloadStorage> storage;
    const FilePath workPath = root / "download";

    const auto onSetup = [workPath] {
        if (const Result<> writable = workPath.ensureWritableDir(); !writable) {
            reportDownloadError(writable.error(), qtDownloadTitle());
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    const auto onReplyDone = [](const QNetworkReplyWrapper &query, DoneWith result) {
        QNetworkReply *reply = query.reply();
        QTC_ASSERT(reply, return false);
        if (result == DoneWith::Success)
            return true;
        if (result == DoneWith::Error) {
            reportDownloadError(Tr::tr("Downloading \"%1\" failed: %2.")
                                    .arg(reply->url().toString(), reply->errorString()),
                                qtDownloadTitle());
        }
        return false;
    };

    const auto onUpdatesSetup = [repository](QNetworkReplyWrapper &query) {
        query.setRequest(QNetworkRequest(Sdk::updatesXmlUrl(repository)));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onUpdatesDone = [storage, onReplyDone](const QNetworkReplyWrapper &query,
                                                      DoneWith result) {
        if (!onReplyDone(query, result))
            return DoneResult::Error;
        const QList<Sdk::QtPackage> packages = Sdk::harmonyOsQtPackages(query.reply()->readAll());
        if (packages.isEmpty()) {
            reportDownloadError(Tr::tr("This repository publishes no Qt for HarmonyOS."),
                                qtDownloadTitle());
            return DoneResult::Error;
        }
        storage->package = packages.constFirst();
        return DoneResult::Success;
    };

    // How many archives a package has is known only once the repository has been read.
    const UntilIterator archiveIterator([storage](qsizetype iteration) {
        return iteration < storage->package.archives.size();
    });
    const auto currentArchive = [storage, archiveIterator] {
        return storage->package.archives.at(archiveIterator.iteration());
    };

    const auto onArchiveSetup = [storage, workPath, currentArchive] {
        storage->archive = workPath / currentArchive();
        storage->file.reset(new QFile(storage->archive.toFSPathString()));
        if (!storage->file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            reportDownloadError(Tr::tr("Cannot write \"%1\": %2.")
                                    .arg(storage->archive.toUserOutput(),
                                         storage->file->errorString()),
                                qtDownloadTitle());
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    const auto onChecksumSetup = [storage, repository, currentArchive]
                                 (QNetworkReplyWrapper &query) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(Tr::tr("Downloading %1...").arg(currentArchive()));
        const QUrl archive = Sdk::archiveUrl(repository, storage->package, currentArchive());
        query.setRequest(QNetworkRequest(QUrl(archive.toString() + ".sha1")));
        query.setNetworkAccessManager(NetworkAccessManager::instance());
    };
    const auto onChecksumDone = [storage, onReplyDone](const QNetworkReplyWrapper &query,
                                                       DoneWith result) {
        if (!onReplyDone(query, result))
            return DoneResult::Error;
        const QByteArray published = query.reply()->readAll().simplified();
        storage->sha1 = QByteArray::fromHex(published.split(' ').constFirst());
        if (storage->sha1.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha1)) {
            reportDownloadError(Tr::tr("Cannot read the published checksum: \"%1\".")
                                    .arg(storage->archive.fileName()),
                                qtDownloadTitle());
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onDownloadSetup = [storage, repository, currentArchive]
                                 (QNetworkReplyWrapper &query) {
        setupDownload(query, Sdk::archiveUrl(repository, storage->package, currentArchive()),
                      storage->progressDialog.get(), storage->file.get(), qtDownloadTitle());
    };
    const auto onDownloadDone = [storage, onReplyDone](const QNetworkReplyWrapper &query,
                                                       DoneWith result) {
        const bool written = storage->file->flush();
        storage->file->close();
        if (!onReplyDone(query, result))
            return DoneResult::Error;
        if (!written) {
            reportDownloadError(Tr::tr("Cannot write \"%1\": %2.")
                                    .arg(storage->archive.toUserOutput(),
                                         storage->file->errorString()),
                                qtDownloadTitle());
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onVerifySetup = [storage](Async<Result<>> &async) {
        storage->progressDialog->setRange(0, 0);
        storage->progressDialog->setLabelText(
            Tr::tr("Verifying %1...").arg(storage->archive.fileName()));
        async.setConcurrentCallData(verifyArchive, storage->archive, storage->sha1,
                                    QCryptographicHash::Sha1);
    };
    const auto onVerifyDone = [](const Async<Result<>> &async, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        if (!async.isResultAvailable()) {
            reportDownloadError(Tr::tr("Verifying the downloaded archive failed."),
                                qtDownloadTitle());
            return DoneResult::Error;
        }
        if (const Result<> verified = async.result(); !verified) {
            reportDownloadError(verified.error(), qtDownloadTitle());
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onUnpackSetup = [storage, root](Unarchiver &task) {
        storage->progressDialog->setLabelText(
            Tr::tr("Unpacking %1...").arg(storage->archive.fileName()));
        task.setArchive(storage->archive);
        // The archives of a Qt package carry the contents of the installation itself, so
        // they unpack into the prefix rather than into the root.
        task.setDestination(root / storage->package.qtVersion() / storage->package.target());
    };
    const auto onUnpackDone = [storage](const Unarchiver &task, DoneWith result) {
        if (result == DoneWith::Cancel)
            return DoneResult::Error;
        if (const Result<> unpacked = task.result(); !unpacked) {
            reportDownloadError(Tr::tr("Unpacking \"%1\" failed: %2.")
                                    .arg(storage->archive.fileName(), unpacked.error()),
                                qtDownloadTitle());
            return DoneResult::Error;
        }
        // A Qt is gigabytes in a dozen archives, so an unpacked one goes away at once
        // rather than at the end.
        storage->archive.removeFile();
        return DoneResult::Success;
    };

    const auto onCancel = [storage] {
        return makeObjectSignal(storage->progressDialog.get(), &QProgressDialog::canceled);
    };

    return Group {
        storage,
        Group {
            onGroupSetup(onSetup),
            QNetworkReplyWrapperTask(onUpdatesSetup, onUpdatesDone),
            For (archiveIterator) >> Do {
                Group {
                    onGroupSetup(onArchiveSetup),
                    QNetworkReplyWrapperTask(onChecksumSetup, onChecksumDone),
                    QNetworkReplyWrapperTask(onDownloadSetup, onDownloadDone),
                    AsyncTask<Result<>>(onVerifySetup, onVerifyDone),
                    UnarchiverTask(onUnpackSetup, onUnpackDone),
                },
            },
        }.withCancel(onCancel),
        onGroupDone([workPath] { workPath.removeRecursively(); }),
    };
}

HarmonyOsSettings::HarmonyOsSettings()
{
    setSettingsGroup("HarmonyOSConfiguration");
    setAutoApply(false);

    sdkLocation.setSettingsKey("SdkLocation");
    sdkLocation.setExpectedKind(PathChooserKind::ExistingDirectory);
    sdkLocation.setLabelText(Tr::tr("HarmonyOS SDK location:"));

    automaticKitCreation.setSettingsKey("AutomaticKitCreation");
    automaticKitCreation.setLabelText(Tr::tr("Create kits automatically"));
    automaticKitCreation.setDefaultValue(true);

    additionalPackages.setSettingsKey("AdditionalPackages");
    additionalPackages.setExpectedKind(PathChooserKind::ExistingDirectory);
    additionalPackages.setLabelText(Tr::tr("Additional packages:"));
    additionalPackages.setToolTip(
        Tr::tr("Installation prefix of the third-party libraries built for HarmonyOS that Qt "
               "needs at run time - the directory their \"lib\" is in. Their contents are "
               "added to the application package."));

    deviceSdkLocation.setSettingsKey("DeviceSdkLocation");
    deviceSdkLocation.setExpectedKind(PathChooserKind::ExistingDirectory);
    deviceSdkLocation.setLabelText(Tr::tr("Device-hosted SDK:"));
    deviceSdkLocation.setToolTip(
        Tr::tr("An unpacked OpenHarmony \"native\" SDK whose binaries run on the device "
               "rather than on this computer, published as \"native-ohos-x64\" although "
               "the payload is for arm64. Qt Creator assembles a toolchain out of it and "
               "carries it in its own package, which is what makes building on the device "
               "possible. Leave it empty when the device is only a target."));

    qtRepository.setSettingsKey("QtRepository");
    qtRepository.setDisplayStyle(StringAspect::LineEditDisplay);
    qtRepository.setLabelText(Tr::tr("Qt repository:"));
    qtRepository.setToolTip(
        Tr::tr("The directory an \"Updates.xml\" describing Qt packages is published in, as "
               "the Qt installer reads them. It is what a Qt Creator without the maintenance "
               "tool, for example one running on a device, installs a Qt for HarmonyOS from."));

    runWithoutInstalling.setSettingsKey("RunWithoutInstalling");
    runWithoutInstalling.setLabelText(Tr::tr("Run without installing"));
    runWithoutInstalling.setToolTip(
        Tr::tr("Installs a package that holds a runner instead of the application, and hands "
               "the application to it over a channel at every run. Installing a package costs "
               "a minute of packaging, signing and installing; this way only the changed "
               "application library travels. The package keeps the bundle name, the "
               "permissions and the signature of a normal one, so what a run exercises is "
               "still the application - but it runs under the runner's identity, which is "
               "why anything that depends on the application's own manifest has to be "
               "checked with an installed package."));

    signingCertificate.setSettingsKey("SigningCertificate");
    signingCertificate.setExpectedKind(PathChooserKind::File);
    signingCertificate.setLabelText(Tr::tr("Certificate (.cer):"));

    signingProfile.setSettingsKey("SigningProfile");
    signingProfile.setExpectedKind(PathChooserKind::File);
    signingProfile.setLabelText(Tr::tr("Provisioning profile (.p7b):"));

    signingKeystore.setSettingsKey("SigningKeystore");
    signingKeystore.setExpectedKind(PathChooserKind::File);
    signingKeystore.setLabelText(Tr::tr("Keystore (.p12):"));

    signingKeyAlias.setSettingsKey("SigningKeyAlias");
    signingKeyAlias.setDisplayStyle(StringAspect::LineEditDisplay);
    signingKeyAlias.setLabelText(Tr::tr("Key alias:"));

    // Name the keychain entry explicitly: a settings key without a '.' cannot be
    // split into a service and a key, and the keychain access then fails.
    signingKeyPassword.setSettingsKey("SigningKeyPassword");
    signingKeyPassword.setService("HarmonyOS");
    signingKeyPassword.setKey("KeyPassword");
    signingKeyPassword.setLabelText(Tr::tr("Key password:"));

    signingStorePassword.setSettingsKey("SigningStorePassword");
    signingStorePassword.setService("HarmonyOS");
    signingStorePassword.setKey("StorePassword");
    signingStorePassword.setLabelText(Tr::tr("Keystore password:"));

    connect(this, &AspectContainer::applied, this, [this] {
        refreshSigningPasswords();
        applyConfig();
    });

    setLayouter([this] {
        auto instruction = new QLabel(
            Tr::tr("Select the installation directory of DevEco Studio or of the HarmonyOS "
                   "command-line tools. It must contain the OpenHarmony native SDK (with the "
                   "\"llvm\" and \"sysroot\" folders) used to build for HarmonyOS."));
        instruction->setWordWrap(true);

        auto autodetectButton = new QPushButton(Tr::tr("Auto-detect"));

        auto downloadButton = new QPushButton(Tr::tr("Download"));
        downloadButton->setToolTip(
            Tr::tr("Installs the public OpenHarmony SDK into the directory above. It carries "
                   "the compilers, the sysroot and hdc, which covers building for a device and "
                   "reaching one. Packaging an application additionally needs hvigor, which "
                   "only Huawei's command-line tools carry."));

        auto status = new InfoLabel;
        status->setElideMode(Qt::ElideNone);
        status->setWordWrap(true);

        auto qtNote = new QLabel(
            Tr::tr("Building for HarmonyOS needs a Qt that was cross-built for it. The Qt "
                   "installer publishes one from Qt 6.12 on, and can install it here. Where "
                   "there is no Qt installer, it is downloaded from a repository instead."));
        qtNote->setWordWrap(true);

        auto installQtButton = new QPushButton(Tr::tr("Install Qt for HarmonyOS"));
        if (!installerService()) {
            installQtButton->setToolTip(
                Tr::tr("Only Qt Creator installed with Qt Online Installer has Qt Maintenance "
                       "Tool, so the packages come from the repository below here."));
        }

        auto qtStatus = new InfoLabel;
        qtStatus->setElideMode(Qt::ElideNone);
        qtStatus->setWordWrap(true);

        auto signingNote = new QLabel(
            Tr::tr("Material for signing HarmonyOS packages, as issued for the developer "
                   "account. Projects that DevEco Studio set up for automatic signing use "
                   "their own material instead."));
        signingNote->setWordWrap(true);

        using namespace Layouting;
        Column column {
            Group {
                title(Tr::tr("HarmonyOS SDK")),
                Column {
                    instruction,
                    Row { sdkLocation, autodetectButton, downloadButton },
                    status,
                    Form {
                        additionalPackages, br,
                        deviceSdkLocation, br,
                    },
                    automaticKitCreation,
                },
            },
            Group {
                title(Tr::tr("Qt for HarmonyOS")),
                Column {
                    qtNote,
                    Row { installQtButton, st },
                    Form { qtRepository, br },
                    qtStatus,
                },
            },
            Group {
                title(Tr::tr("Running")),
                Column { runWithoutInstalling },
            },
            Group {
                title(Tr::tr("Package Signing")),
                Column {
                    signingNote,
                    Form {
                        signingCertificate, br,
                        signingProfile, br,
                        signingKeystore, br,
                        signingKeyAlias, br,
                        signingKeyPassword, br,
                        signingStorePassword, br,
                    },
                },
            },
            st,
        };

        // The path chooser widget only exists after the layout above was built.
        const auto updateStatus = [this, status] {
            const FilePath sdkRoot = sdkLocation.pathChooser()->filePath();
            if (sdkRoot.isEmpty()) {
                status->setType(InfoLabelType::None);
                status->setText({});
                return;
            }
            if (Sdk::isValidSdk(sdkRoot)) {
                status->setType(InfoLabelType::Ok);
                const bool hasHvigor = !Sdk::hvigorBinPath(sdkRoot).isEmpty();
                status->setText(hasHvigor
                    ? Tr::tr("A HarmonyOS SDK with the hvigor build tool was found.")
                    : Tr::tr("A HarmonyOS SDK was found, but the hvigor build tool is missing."));
            } else {
                status->setType(InfoLabelType::NotOk);
                status->setText(Tr::tr("No HarmonyOS native SDK was found in this directory."));
            }
        };

        connect(sdkLocation.pathChooser(), &PathChooser::textChanged, this, updateStatus);
        connect(autodetectButton, &QPushButton::clicked, this, [this, status, updateStatus] {
            const FilePath detected = Sdk::detectDevEcoSdk();
            if (detected.isEmpty()) {
                status->setType(InfoLabelType::Warning);
                status->setText(Tr::tr("Could not find an installed DevEco Studio SDK."));
                return;
            }
            sdkLocation.pathChooser()->setFilePath(detected);
            updateStatus();
        });

        connect(downloadButton, &QPushButton::clicked, this, [this, status, updateStatus] {
            const FilePath sdkPath = sdkLocation.pathChooser()->filePath();
            if (sdkPath.isEmpty()) {
                status->setType(InfoLabelType::Warning);
                status->setText(Tr::tr("Enter the directory to install the SDK into first."));
                return;
            }
            if (Sdk::isValidSdk(sdkPath)) {
                reportDownloadError(Tr::tr("This directory already holds a HarmonyOS SDK."));
                return;
            }
            const QString question
                = Tr::tr("Download the public OpenHarmony SDK %1 into \"%2\"?\n\n"
                         "The archive it comes in is a few gigabytes.")
                      .arg(QString::fromLatin1(Constants::PUBLIC_SDK_VERSION),
                           sdkPath.toUserOutput());
            if (QMessageBox::question(Core::ICore::dialogParent(), downloadTitle(), question)
                != QMessageBox::Yes) {
                return;
            }
            m_sdkDownloader.start({downloadSdkRecipe(sdkPath)}, {},
                                  [this, updateStatus](QtTaskTree::DoneWith result) {
                if (result != QtTaskTree::DoneWith::Success)
                    return;
                updateStatus();
                apply();
            });
        });

        const auto updateQtStatus = [qtStatus] {
            const QtSupport::QtVersions versions = harmonyOsQtVersions();
            if (versions.isEmpty()) {
                qtStatus->setType(InfoLabelType::NotOk);
                qtStatus->setText(Tr::tr("No Qt for HarmonyOS is registered."));
                return;
            }
            qtStatus->setType(InfoLabelType::Ok);
            qtStatus->setText(Tr::tr("Registered: %1.").arg(versionNames(versions)));
        };

        const auto reportScan = [qtStatus, updateQtStatus](const ScanResult &scan,
                                                           const FilePath &root) {
            updateQtStatus();
            if (!scan.added.isEmpty())
                return;
            if (!scan.mistyped.isEmpty()) {
                qtStatus->setType(InfoLabelType::Warning);
                qtStatus->setText(
                    Tr::tr("The Qt installer registered \"%1\" without naming HarmonyOS as "
                           "its device type, so no HarmonyOS kit is built from it. A Qt "
                           "installer that knows about HarmonyOS is needed for that.")
                        .arg(versionNames(scan.mistyped)));
                return;
            }
            qtStatus->setType(InfoLabelType::Warning);
            qtStatus->setText(Tr::tr("No Qt for HarmonyOS was found in \"%1\".")
                                  .arg(root.toUserOutput()));
        };

        connect(installQtButton, &QPushButton::clicked, this, [this, qtStatus, reportScan] {
            UpdateInfo::Service *service = installerService();
            const FilePath root = service ? service->installationRoot() : downloadedQtRoot();
            // A Qt that is installed but unknown here needs registering rather than
            // installing, and that is the case the installer reports as "already
            // installed", so look before offering an installation.
            const ScanResult scan = registerQtVersions(root);
            if (!scan.added.isEmpty() || !scan.mistyped.isEmpty()) {
                reportScan(scan, root);
                return;
            }
            if (service) {
                // The dialog's result tells whether it was closed on its install button,
                // not whether anything arrived, so look at the directory either way.
                service->installPackages(qtPackageFilter());
                reportScan(registerQtVersions(root), root);
                return;
            }
            const QUrl repository(qtRepository.volatileValue().trimmed());
            if (!repository.isValid() || repository.isRelative()) {
                qtStatus->setType(InfoLabelType::Warning);
                qtStatus->setText(Tr::tr("Enter the repository the Qt packages are "
                                         "published in first."));
                return;
            }
            m_qtDownloader.start({downloadQtRecipe(repository, root)}, {},
                                 [root, reportScan](QtTaskTree::DoneWith result) {
                if (result == QtTaskTree::DoneWith::Success)
                    reportScan(registerQtVersions(root), root);
            });
        });

        updateStatus();
        updateQtStatus();

        return column;
    });

    readSettings();
    refreshSigningPasswords();
}

void HarmonyOsSettings::refreshSigningPasswords()
{
    signingKeyPassword.requestValue([this](const Result<QString> &password) {
        if (password)
            m_keyPassword = *password;
    });
    signingStorePassword.requestValue([this](const Result<QString> &password) {
        if (password)
            m_storePassword = *password;
    });
}

// HarmonyOsSettingsPage

class HarmonyOsSettingsPage final : public Core::IOptionsPage
{
public:
    HarmonyOsSettingsPage()
    {
        setId(Constants::HARMONYOS_SETTINGS_ID);
        setDisplayName(Tr::tr("HarmonyOS"));
        setCategory(ProjectExplorer::Constants::SDK_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

void setupHarmonyOsSettingsPage()
{
    static HarmonyOsSettingsPage theHarmonyOsSettingsPage;
}

} // namespace HarmonyOs::Internal
