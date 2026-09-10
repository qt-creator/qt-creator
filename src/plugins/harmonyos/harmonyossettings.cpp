// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "harmonyossettings.h"

#include "harmonyosconfigurations.h"
#include "harmonyosconstants.h"
#include "harmonyossdk.h"
#include "harmonyostr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/qtasktree.h>

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

static void reportDownloadError(const QString &error)
{
    QMessageBox::warning(Core::ICore::dialogParent(), downloadTitle(), error);
}

static void verifyArchive(QPromise<Result<>> &promise, const FilePath &archive,
                          const QByteArray &sha256)
{
    QFile file(archive.toFSPathString());
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file)) {
        promise.addResult(ResultError(Tr::tr("Cannot read \"%1\": %2.")
                                          .arg(archive.toUserOutput(), file.errorString())));
        return;
    }
    if (hash.result() != sha256) {
        promise.addResult(ResultError(
            Tr::tr("The checksum of the downloaded archive is not the published one.")));
        return;
    }
    promise.addResult(ResultOk);
}

// Installs the components of the public OpenHarmony SDK that Qt Creator reads into sdkPath.
// The archive they come in holds one directory per host and one zip per component in it,
// and is a few gigabytes, so it is streamed to disk and only the wanted zips are unpacked
// out of it.
static QtTaskTree::GroupItem downloadSdkRecipe(const FilePath &sdkPath)
{
    using namespace QtTaskTree;

    constexpr qint64 megaByte = 1024 * 1024;

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
            reportDownloadError(Tr::tr("The published checksum of the SDK archive cannot "
                                       "be read."));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto onDownloadSetup = [storage, sdk](QNetworkReplyWrapper &query) {
        query.setRequest(QNetworkRequest(sdk.url));
        query.setNetworkAccessManager(NetworkAccessManager::instance());

        QProgressDialog *progressDialog = storage->progressDialog.get();
        QObject::connect(&query, &QNetworkReplyWrapper::downloadProgress, progressDialog,
                         [progressDialog](qint64 received, qint64 total) {
            // Gigabytes do not fit the dialog's int range, so it counts megabytes.
            progressDialog->setRange(0, total / megaByte);
            progressDialog->setValue(received / megaByte);
        });
        // Gigabytes do not fit into memory either, so the reply is drained as it arrives.
        QObject::connect(&query, &QNetworkReplyWrapper::started, &query,
                         [storage, queryPtr = &query] {
            QNetworkReply *reply = queryPtr->reply();
            QObject::connect(reply, &QNetworkReply::readyRead, reply, [storage, reply] {
                if (storage->file->write(reply->readAll()) < 0)
                    reply->abort();
            });
        });
#if QT_CONFIG(ssl)
        QObject::connect(&query, &QNetworkReplyWrapper::sslErrors, &query,
                         [queryPtr = &query](const QList<QSslError> &errors) {
            reportDownloadError(Tr::tr("The download was aborted: %1")
                                    .arg(errors.constFirst().errorString()));
            queryPtr->reply()->abort();
        });
#endif
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
        async.setConcurrentCallData(verifyArchive, storage->archive, storage->sha256);
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

        updateStatus();

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
