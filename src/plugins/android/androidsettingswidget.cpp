// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidsdkmanager.h"
#include "androidsdkmanagerdialog.h"
#include "androidsettingswidget.h"
#include "androidtr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/async.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/summarywidget.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QGuiApplication>
#include <QList>
#include <QListWidget>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

using namespace Utils;

namespace Android::Internal {

static Q_LOGGING_CATEGORY(androidsettingswidget, "qtc.android.androidsettingswidget", QtWarningMsg);
constexpr int requiredJavaMajorVersion = 17;

class AndroidSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    // Todo: This would be so much simpler if it just used Utils::PathChooser!!!
    AndroidSettingsWidget();

private:
    void showEvent(QShowEvent *event) override;

    void validateJdk();
    void updateNdkList();
    void onSdkPathChanged();
    void validateSdk();
    void openSDKDownloadUrl();
    void openNDKDownloadUrl();
    void openOpenJDKDownloadUrl();
    void downloadOpenSslRepo(const bool silent = false);
    void createKitToggled();

    void updateUI();

    void downloadSdk();
    void addCustomNdkItem();
    bool isDefaultNdkSelected() const;
    void validateOpenSsl();

    Tasking::TaskTreeRunner m_sdkDownloader;
    bool m_isInitialReloadDone = false;

    SummaryWidget *m_androidSummary = nullptr;
    SummaryWidget *m_openSslSummary = nullptr;

    PathChooser *m_sdkLocationPathChooser;
    QPushButton *m_makeDefaultNdkButton;
    QListWidget *m_ndkListWidget;
    PathChooser *m_openJdkLocationPathChooser;
    QCheckBox *m_createKitCheckBox;
    PathChooser *m_openSslPathChooser;
};

enum AndroidValidation {
    JavaPathExistsAndWritableRow,
    SdkPathExistsAndWritableRow,
    SdkToolsInstalledRow,
    SdkManagerSuccessfulRow,
    PlatformToolsInstalledRow,
    PlatformSdkInstalledRow,
    BuildToolsInstalledRow,
    AllEssentialsInstalledRow,
};

enum OpenSslValidation {
    OpenSslPathExistsRow,
    OpenSslPriPathExists,
    OpenSslCmakeListsPathExists
};

static Result<> testJavaC(const FilePath &jdkPath)
{
    if (!jdkPath.isReadableDir())
        return ResultError(Tr::tr("The selected path does not exist or is not readable."));

    const QString javacCommand("javac");
    const QString versionParameter("-version");
    const FilePath bin = jdkPath / "bin" / (javacCommand + QTC_HOST_EXE_SUFFIX);

    if (!bin.isExecutableFile())
        return ResultError(
            Tr::tr("Could not find \"%1\" in the selected path.")
                .arg(bin.toUserOutput()));

    QVersionNumber jdkVersion;

    Process javacProcess;
    const CommandLine cmd(bin, {versionParameter});
    javacProcess.setProcessChannelMode(QProcess::ProcessChannelMode::MergedChannels);
    javacProcess.setCommand(cmd);
    javacProcess.runBlocking();

    const QString stdOut = javacProcess.stdOut().trimmed();

    if (javacProcess.exitCode() != 0)
        return ResultError(
            Tr::tr("The selected path does not contain a valid JDK. (%1 failed: %2)")
                .arg(cmd.toUserOutput(), stdOut));

    // We expect "javac <version>" where <version> is "major.minor.patch"
    const QString outputPrefix = javacCommand + " ";
    if (!stdOut.startsWith(outputPrefix))
        return ResultError(Tr::tr("Unexpected output from \"%1\": %2")
                                   .arg(cmd.toUserOutput(), stdOut));

    jdkVersion = QVersionNumber::fromString(stdOut.mid(outputPrefix.length()).split('\n').first());

    if (jdkVersion.isNull() /* || jdkVersion.majorVersion() != requiredJavaMajorVersion */ ) {
        return ResultError(Tr::tr("Unsupported JDK version (needs to be %1): %2 (parsed: %3)")
                                   .arg(requiredJavaMajorVersion)
                                   .arg(stdOut, jdkVersion.toString()));
    }

    return {};
}

AndroidSettingsWidget::AndroidSettingsWidget()
{
    setWindowTitle(Tr::tr("Android Configuration"));

    const QIcon downloadIcon = Icons::ONLINE.icon();

    m_sdkLocationPathChooser = new PathChooser;

    auto downloadSdkToolButton = new QToolButton;
    downloadSdkToolButton->setIcon(downloadIcon);
    downloadSdkToolButton->setToolTip(Tr::tr("Open Android SDK download URL in the system's browser."));

    auto addCustomNdkButton = new QPushButton(Tr::tr("Add..."));
    addCustomNdkButton->setToolTip(Tr::tr("Add the selected custom NDK. The toolchains "
                                          "and debuggers will be created automatically."));

    auto removeCustomNdkButton = new QPushButton(Tr::tr("Remove"));
    removeCustomNdkButton->setEnabled(false);
    removeCustomNdkButton->setToolTip(Tr::tr("Remove the selected NDK if it has been added manually."));

    m_makeDefaultNdkButton = new QPushButton;
    m_makeDefaultNdkButton->setToolTip(Tr::tr("Force a specific NDK installation to be used by all "
                                              "Android kits.<br/>Note that the forced NDK might not "
                                              "be compatible with all registered Qt versions."));

    auto androidDetailsWidget = new DetailsWidget;
    sdkManager().setSpinnerTarget(androidDetailsWidget);

    m_ndkListWidget = new QListWidget;
    m_ndkListWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_ndkListWidget->setIconSize(QSize(16, 16));
    m_ndkListWidget->setResizeMode(QListView::Adjust);
    m_ndkListWidget->setModelColumn(0);
    m_ndkListWidget->setSortingEnabled(false);

    m_openJdkLocationPathChooser = new PathChooser;

    auto downloadOpenJdkToolButton = new QToolButton;
    downloadOpenJdkToolButton->setIcon(downloadIcon);
    downloadOpenJdkToolButton->setToolTip(Tr::tr("Open JDK download URL in the system's browser."));

    auto sdkToolsAutoDownloadButton = new QPushButton(Tr::tr("Set Up SDK"));
    sdkToolsAutoDownloadButton->setToolTip(
                Tr::tr("Automatically download Android SDK Tools to selected location.\n\n"
                       "If the selected path contains no valid SDK Tools, the SDK Tools package is downloaded\n"
                       "from %1,\n"
                       "and extracted to the selected path.\n"
                       "After the SDK Tools are properly set up, you are prompted to install any essential\n"
                       "packages required for Qt to build for Android.")
                .arg(AndroidConfig::sdkToolsUrl().toString()));

    auto sdkManagerToolButton = new QPushButton(Tr::tr("SDK Manager"));

    auto downloadNdkToolButton = new QToolButton;
    downloadNdkToolButton->setToolTip(Tr::tr("Open Android NDK download URL in the system's browser."));

    m_createKitCheckBox = new QCheckBox(Tr::tr("Automatically create kits for Android tool chains"));
    m_createKitCheckBox->setChecked(true);

    auto openSslDetailsWidget = new DetailsWidget;

    m_openSslPathChooser = new PathChooser;
    m_openSslPathChooser->setToolTip(Tr::tr("Select the path of the prebuilt OpenSSL binaries."));

    auto downloadOpenSslPrebuiltLibs = new QPushButton(Tr::tr("Download OpenSSL"));
    downloadOpenSslPrebuiltLibs->setToolTip(
                Tr::tr("Automatically download OpenSSL prebuilt libraries.\n\n"
                       "These libraries can be shipped with your application if any SSL operations\n"
                       "are performed. Find the checkbox under \"Projects > Build > Build Steps >\n"
                       "Build Android APK > Additional Libraries\".\n"
                       "If the automatic download fails, Qt Creator proposes to open the download URL\n"
                       "in the system's browser for manual download."));

    const QMap<int, QString> androidValidationPoints = {
        { JavaPathExistsAndWritableRow, Tr::tr("JDK path exists and is writable.") },
        { SdkPathExistsAndWritableRow, Tr::tr("Android SDK path exists and is writable.") },
        { SdkToolsInstalledRow, Tr::tr("Android SDK Command-line Tools installed.") },
        { SdkManagerSuccessfulRow, Tr::tr("Android SDK Command-line Tools runs.") },
        { PlatformToolsInstalledRow, Tr::tr("Android SDK Platform-Tools installed.") },
        { AllEssentialsInstalledRow,
            Tr::tr( "All essential packages installed for all installed Qt versions.") },
        { BuildToolsInstalledRow, Tr::tr("Android SDK Build-Tools installed.") },
        { PlatformSdkInstalledRow, Tr::tr("Android Platform SDK (version) installed.") }
    };

    m_androidSummary = new SummaryWidget(androidValidationPoints, Tr::tr("Android settings are OK."),
                                         Tr::tr("Android settings have errors."),
                                         androidDetailsWidget);
    const QMap<int, QString> openSslValidationPoints = {
        { OpenSslPathExistsRow, Tr::tr("OpenSSL path exists.") },
        { OpenSslPriPathExists, Tr::tr("QMake include project (openssl.pri) exists.") },
        { OpenSslCmakeListsPathExists, Tr::tr("CMake include project (CMakeLists.txt) exists.") }
   };
    m_openSslSummary = new SummaryWidget(openSslValidationPoints,
                                         Tr::tr("OpenSSL Settings are OK."),
                                         Tr::tr("OpenSSL settings have errors."),
                                         openSslDetailsWidget);

    m_openJdkLocationPathChooser->setValidationFunction([](const QString &s) {
        return Utils::asyncRun([s]() -> Result<QString> {
            Result<> test = testJavaC(FilePath::fromUserInput(s));
            if (!test) {
                Core::MessageManager::writeSilently(test.error());
                return ResultError(test.error());
            }
            return s;
        });
    });

    connect(m_openJdkLocationPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateJdk);
    if (AndroidConfig::openJDKLocation().isEmpty())
        AndroidConfig::setOpenJDKLocation(AndroidConfig::getJdkPath());
    m_openJdkLocationPathChooser->setFilePath(AndroidConfig::openJDKLocation());
    m_openJdkLocationPathChooser->setPromptDialogTitle(Tr::tr("Select JDK Path"));

    if (AndroidConfig::sdkLocation().isEmpty())
        AndroidConfig::setSdkLocation(AndroidConfig::defaultSdkPath());
    m_sdkLocationPathChooser->setFilePath(AndroidConfig::sdkLocation());
    m_sdkLocationPathChooser->setPromptDialogTitle(Tr::tr("Select Android SDK Folder"));

    m_openSslPathChooser->setPromptDialogTitle(Tr::tr("Select OpenSSL Include Project File"));
    if (AndroidConfig::openSslLocation().isEmpty())
        AndroidConfig::setOpenSslLocation(AndroidConfig::sdkLocation() / ("android_openssl"));
    m_openSslPathChooser->setFilePath(AndroidConfig::openSslLocation());

    m_createKitCheckBox->setChecked(AndroidConfig::automaticKitCreation());

    downloadNdkToolButton->setIcon(downloadIcon);

    using namespace Layouting;

    Column {
        Tr::tr("All changes on this page take effect immediately."),
        Group {
            title(Tr::tr("Android Settings")),
            Grid {
                Tr::tr("JDK location:"),
                m_openJdkLocationPathChooser,
                empty,
                downloadOpenJdkToolButton,
                br,

                Tr::tr("Android SDK location:"),
                m_sdkLocationPathChooser,
                sdkToolsAutoDownloadButton,
                downloadSdkToolButton,
                br,

                empty, empty, sdkManagerToolButton, br,

                Column { Tr::tr("Android NDK list:"), st },
                m_ndkListWidget,
                Column {
                    addCustomNdkButton,
                    removeCustomNdkButton,
                    m_makeDefaultNdkButton,
                },
                downloadNdkToolButton,
                br,

                Span(4, androidDetailsWidget), br,

                Span(4, m_createKitCheckBox)
            }
        },
        Group {
            title(Tr::tr("Android OpenSSL Settings (Optional)")),
            Grid {
                Tr::tr("OpenSSL binaries location:"),
                m_openSslPathChooser,
                downloadOpenSslPrebuiltLibs,
                br,

                Span(4, openSslDetailsWidget)
            }
        },
        st
    }.attachTo(this);

    connect(m_sdkLocationPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::onSdkPathChanged);

    connect(m_ndkListWidget, &QListWidget::currentTextChanged,
            this, [this, removeCustomNdkButton](const QString &ndk) {
        updateUI();
        removeCustomNdkButton->setEnabled(AndroidConfig::getCustomNdkList().contains(FilePath::fromUserInput(ndk)));
    });
    connect(addCustomNdkButton, &QPushButton::clicked, this,
            &AndroidSettingsWidget::addCustomNdkItem);
    connect(removeCustomNdkButton, &QPushButton::clicked, this, [this] {
        if (isDefaultNdkSelected())
            AndroidConfig::setDefaultNdk({});
        AndroidConfig::removeCustomNdk(FilePath::fromUserInput(m_ndkListWidget->currentItem()->text()));
        m_ndkListWidget->takeItem(m_ndkListWidget->currentRow());
    });
    connect(m_makeDefaultNdkButton, &QPushButton::clicked, this, [this] {
        const FilePath defaultNdk = isDefaultNdkSelected()
                ? FilePath()
                : FilePath::fromUserInput(m_ndkListWidget->currentItem()->text());
        AndroidConfig::setDefaultNdk(defaultNdk);
        updateUI();
    });

    connect(m_openSslPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateOpenSsl);
    connect(m_createKitCheckBox, &QAbstractButton::toggled,
            this, &AndroidSettingsWidget::createKitToggled);
    connect(downloadNdkToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openNDKDownloadUrl);
    connect(downloadSdkToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openSDKDownloadUrl);
    connect(downloadOpenSslPrebuiltLibs, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadOpenSslRepo);
    connect(downloadOpenJdkToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openOpenJDKDownloadUrl);
    connect(sdkManagerToolButton, &QAbstractButton::clicked, this, &executeAndroidSdkManagerDialog);
    connect(sdkToolsAutoDownloadButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadSdk);
    connect(&m_sdkDownloader, &Tasking::TaskTreeRunner::done, this, [this](Tasking::DoneWith result) {
        if (result != Tasking::DoneWith::Success)
            return;
        // Make sure the sdk path is created before installing packages
        const FilePath sdkPath = AndroidConfig::sdkLocation();
        if (!sdkPath.createDir()) {
            QMessageBox::warning(this, Android::Internal::dialogTitle(),
                                 Tr::tr("Failed to create the SDK Tools path %1.")
                                 .arg("\n\"" + sdkPath.toUserOutput() + "\""));
        }
        sdkManager().reloadPackages();
        updateUI();
        apply();

        connect(&sdkManager(), &AndroidSdkManager::packagesReloaded, this, [this] {
            downloadOpenSslRepo(true);
        }, Qt::SingleShotConnection);
    });

    setOnApply([] { AndroidConfigurations::applyConfig(); });
}

void AndroidSettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!m_isInitialReloadDone) {
        validateJdk();
        // Reloading SDK packages (force) is still synchronous. Use zero timer
        // to let settings dialog open first.
        QTimer::singleShot(0, this, [this] {
            sdkManager().refreshPackages();
            validateSdk();
            // Validate SDK again after any change in SDK packages.
            connect(&sdkManager(), &AndroidSdkManager::packagesReloaded, this, [this] {
                m_androidSummary->setInProgressText("Packages reloaded");
                m_sdkLocationPathChooser->triggerChanged();
                validateSdk();
            }, Qt::QueuedConnection); // Hack: Let AndroidSdkModel::refreshData() be called first,
                                      // otherwise the nested loop inside validateSdk() may trigger
                                      // the repaint for the old data, containing pointers
                                      // to the deleted packages. That's why we queue the signal.
        });
        validateOpenSsl();
        m_isInitialReloadDone = true;
    }
}

void AndroidSettingsWidget::updateNdkList()
{
    m_ndkListWidget->clear();
    const auto installedPkgs = sdkManager().installedNdkPackages();
    for (const Ndk *ndk : installedPkgs) {
        m_ndkListWidget->addItem(new QListWidgetItem(Icons::LOCKED.icon(),
                                                        ndk->installedLocation().toUserOutput()));
    }

    const FilePaths customNdks = AndroidConfig::getCustomNdkList();
    for (const FilePath &ndk : customNdks) {
        if (AndroidConfig::isValidNdk(ndk)) {
            m_ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndk.toUrlishString()));
        } else {
            AndroidConfig::removeCustomNdk(ndk);
        }
    }

    updateUI();
}

void AndroidSettingsWidget::addCustomNdkItem()
{
    const FilePath homePath = FilePath::fromUserInput(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)
            .constFirst());
    const FilePath ndkPath = FileUtils::getExistingDirectory(Tr::tr("Select an NDK"), homePath);

    if (AndroidConfig::isValidNdk(ndkPath)) {
        AndroidConfig::addCustomNdk(ndkPath);
        if (m_ndkListWidget->findItems(ndkPath.toUrlishString(), Qt::MatchExactly).size() == 0) {
            m_ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndkPath.toUrlishString()));
        }
    } else if (!ndkPath.isEmpty()) {
        QMessageBox::warning(
                    this,
                    Tr::tr("Add Custom NDK"),
                    Tr::tr("The selected path has an invalid NDK. This might mean that the path contains space "
                           "characters, or that it does not have a \"toolchains\" sub-directory, or that the "
                           "NDK version could not be retrieved because of a missing \"source.properties\" or "
                           "\"RELEASE.TXT\" file"));
    }
}


bool AndroidSettingsWidget::isDefaultNdkSelected() const
{
    if (!AndroidConfig::defaultNdk().isEmpty()) {
        if (const QListWidgetItem *item = m_ndkListWidget->currentItem()) {
            return FilePath::fromUserInput(item->text()) == AndroidConfig::defaultNdk();
        }
    }
    return false;
}

void AndroidSettingsWidget::validateJdk()
{
    AndroidConfig::setOpenJDKLocation(m_openJdkLocationPathChooser->filePath());
    Result<> test = testJavaC(AndroidConfig::openJDKLocation());

    m_androidSummary->setPointValid(JavaPathExistsAndWritableRow, test);

    updateUI();

    if (m_isInitialReloadDone)
        sdkManager().reloadPackages();
}

void AndroidSettingsWidget::validateOpenSsl()
{
    AndroidConfig::setOpenSslLocation(m_openSslPathChooser->filePath());

    m_openSslSummary->setPointValid(OpenSslPathExistsRow, AndroidConfig::openSslLocation().exists());

    const bool priFileExists = AndroidConfig::openSslLocation().pathAppended("openssl.pri").exists();
    m_openSslSummary->setPointValid(OpenSslPriPathExists, priFileExists);
    const bool cmakeListsExists
        = AndroidConfig::openSslLocation().pathAppended("CMakeLists.txt").exists();
    m_openSslSummary->setPointValid(OpenSslCmakeListsPathExists, cmakeListsExists);

    updateUI();
}

void AndroidSettingsWidget::onSdkPathChanged()
{
    const FilePath sdkPath = m_sdkLocationPathChooser->filePath().cleanPath();
    AndroidConfig::setSdkLocation(sdkPath);
    FilePath currentOpenSslPath = AndroidConfig::openSslLocation();
    if (currentOpenSslPath.isEmpty() || !currentOpenSslPath.exists())
        currentOpenSslPath = sdkPath.pathAppended("android_openssl");
    m_openSslPathChooser->setFilePath(currentOpenSslPath);
    // Package reload will trigger validateSdk.
    sdkManager().refreshPackages();
}

void AndroidSettingsWidget::validateSdk()
{
    const FilePath sdkPath = m_sdkLocationPathChooser->filePath().cleanPath();
    AndroidConfig::setSdkLocation(sdkPath);

    m_androidSummary->setPointValid(SdkPathExistsAndWritableRow,
                                    sdkPath.exists() && sdkPath.isWritableDir());
    m_androidSummary->setPointValid(SdkToolsInstalledRow,
                                    !AndroidConfig::sdkToolsVersion().isNull());
    m_androidSummary->setPointValid(SdkManagerSuccessfulRow, // TODO: track me
                                    sdkManager().packageListingSuccessful());
    m_androidSummary->setPointValid(PlatformToolsInstalledRow, // TODO: track me
                                    AndroidConfig::adbToolPath().exists());
    m_androidSummary->setPointValid(AllEssentialsInstalledRow,
                                    AndroidConfig::allEssentialsInstalled());
    m_androidSummary->setPointValid(BuildToolsInstalledRow,
                                    !AndroidConfig::buildToolsVersion().isNull());
    // installedSdkPlatforms should not trigger a package reload as validate SDK is only called
    // after AndroidSdkManager::packageReloadFinished.
    m_androidSummary->setPointValid(PlatformSdkInstalledRow,
                                    !sdkManager().installedSdkPlatforms().isEmpty());

    const bool sdkToolsOk = m_androidSummary->rowsOk({SdkPathExistsAndWritableRow,
                                                      SdkToolsInstalledRow,
                                                      SdkManagerSuccessfulRow});
    const bool componentsOk = m_androidSummary->rowsOk({PlatformToolsInstalledRow,
                                                        BuildToolsInstalledRow,
                                                        PlatformSdkInstalledRow,
                                                        AllEssentialsInstalledRow});
    AndroidConfig::setSdkFullyConfigured(sdkToolsOk && componentsOk);
    if (sdkToolsOk && !componentsOk) {
        const QStringList notFoundEssentials = sdkManager().notFoundEssentialSdkPackages();
        if (!notFoundEssentials.isEmpty()) {
            QMessageBox::warning(Core::ICore::dialogParent(),
                Tr::tr("Android SDK Changes"),
                Tr::tr("%1 cannot find the following essential packages: \"%2\".\n"
                       "Install them manually after the current operation is done.\n")
                    .arg(QGuiApplication::applicationDisplayName(),
                         notFoundEssentials.join("\", \"")));
        }
        QStringList missingPkgs = sdkManager().missingEssentialSdkPackages();
        // Add the a system image with highest API level only if there are other
        // essentials needed, so it would practicaly be somewhat optional.
        if (!missingPkgs.isEmpty()) {
            const QString sysImage = AndroidConfig::optionalSystemImagePackage();
            if (!sysImage.isEmpty())
                missingPkgs.append(sysImage);
        }
        sdkManager().runInstallationChange({missingPkgs},
            Tr::tr("Android SDK installation is missing necessary packages. "
                   "Do you want to install the missing packages?"));
    }

    updateNdkList();
    updateUI();
}

void AndroidSettingsWidget::openSDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput(
                                  "https://developer.android.com/studio#command-line-tools-only"));
}

void AndroidSettingsWidget::openNDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput("https://developer.android.com/ndk/downloads/"));
}

void AndroidSettingsWidget::openOpenJDKDownloadUrl()
{
    const QString url =
        QString::fromLatin1("https://adoptium.net/temurin/releases/?package=jdk&version=%1")
                            .arg(requiredJavaMajorVersion);
    QDesktopServices::openUrl(QUrl::fromUserInput(url));
}

void AndroidSettingsWidget::downloadOpenSslRepo(const bool silent)
{
    const FilePath openSslPath = m_openSslPathChooser->filePath();
    const QString openSslCloneTitle(Tr::tr("OpenSSL Cloning"));

    if (m_openSslSummary->allRowsOk()) {
        if (!silent) {
            QMessageBox::information(this, openSslCloneTitle,
                Tr::tr("OpenSSL prebuilt libraries repository is already configured."));
        }
        return;
    }

    if (openSslPath.exists() && !openSslPath.isEmpty()) {
        QMessageBox::information(
            this,
            openSslCloneTitle,
            Tr::tr(
                "The selected download path (%1) for OpenSSL already exists and the directory is "
                "not empty. Select a different path or make sure it is an empty directory.")
                .arg(openSslPath.toUserOutput()));
        return;
    }

    QProgressDialog *openSslProgressDialog
        = new QProgressDialog(Tr::tr("Cloning OpenSSL prebuilt libraries..."),
                              Tr::tr("Cancel"), 0, 0);
    openSslProgressDialog->setWindowModality(Qt::ApplicationModal);
    openSslProgressDialog->setWindowTitle(openSslCloneTitle);
    openSslProgressDialog->setFixedSize(openSslProgressDialog->sizeHint());

    const QString openSslRepo("https://github.com/KDAB/android_openssl.git");
    Process *gitCloner = new Process(this);
    const CommandLine
        gitCloneCommand("git", {"clone", "--depth=1", openSslRepo, openSslPath.path()});
    gitCloner->setCommand(gitCloneCommand);

    qCDebug(androidsettingswidget) << "Cloning OpenSSL repo: " << gitCloneCommand.toUserOutput();

    connect(openSslProgressDialog, &QProgressDialog::canceled, gitCloner, &QObject::deleteLater);

    const auto failDialog = [=](const QString &msgSuffix = {}) {
        QStringList sl;
        sl << Tr::tr("OpenSSL prebuilt libraries cloning failed.");
        if (!msgSuffix.isEmpty())
            sl << msgSuffix;
        sl << Tr::tr("Opening OpenSSL URL for manual download.");
        QMessageBox msgBox;
        msgBox.setText(sl.join(" "));
        msgBox.addButton(Tr::tr("Cancel"), QMessageBox::RejectRole);
        QAbstractButton *openButton = msgBox.addButton(Tr::tr("Open Download URL"), QMessageBox::ActionRole);
        msgBox.exec();

        if (msgBox.clickedButton() == openButton)
            QDesktopServices::openUrl(QUrl::fromUserInput(openSslRepo));
        openButton->deleteLater();
    };

    connect(gitCloner, &Process::done, this, [this, openSslProgressDialog, gitCloner, failDialog] {
        openSslProgressDialog->close();
        if (gitCloner->error() != QProcess::UnknownError) {
            if (gitCloner->error() == QProcess::FailedToStart) {
                failDialog(Tr::tr("The Git tool might not be installed properly on your system."));
                return;
            } else {
                failDialog();
            }
        }
        validateOpenSsl();
        m_openSslPathChooser->triggerChanged(); // After cloning, the path exists

        if (!openSslProgressDialog->wasCanceled()
                || gitCloner->result() == ProcessResult::FinishedWithError) {
            failDialog();
        }
    });

    openSslProgressDialog->show();
    gitCloner->start();
}

void AndroidSettingsWidget::createKitToggled()
{
    AndroidConfig::setAutomaticKitCreation(m_createKitCheckBox->isChecked());
}

void AndroidSettingsWidget::updateUI()
{
    const bool androidSetupOk = m_androidSummary->allRowsOk();
    const bool openSslOk = m_openSslSummary->allRowsOk();

    const QString infoText = Tr::tr("(SDK Version: %1)")
            .arg(AndroidConfig::sdkToolsVersion().toString());
    m_androidSummary->setInfoText(androidSetupOk ? infoText : "");

    m_androidSummary->setSetupOk(androidSetupOk);
    m_openSslSummary->setSetupOk(openSslOk);

    // Mark default entry in NDK list widget
    {
        const QFont font = m_ndkListWidget->font();
        QFont markedFont = font;
        markedFont.setItalic(true);
        for (int row = 0; row < m_ndkListWidget->count(); ++row) {
            QListWidgetItem *item = m_ndkListWidget->item(row);
            const bool isDefaultNdk =
                    FilePath::fromUserInput(item->text()) == AndroidConfig::defaultNdk();
            item->setFont(isDefaultNdk ? markedFont : font);
        }
    }

    m_makeDefaultNdkButton->setEnabled(m_ndkListWidget->currentItem() != nullptr);
    m_makeDefaultNdkButton->setText(isDefaultNdkSelected() ? Tr::tr("Unset Default")
                                                           : Tr::tr("Make Default"));
}

void AndroidSettingsWidget::downloadSdk()
{
    if (AndroidConfig::sdkToolsOk()) {
        QMessageBox::warning(this, Android::Internal::dialogTitle(),
                             Tr::tr("The selected path already has a valid SDK Tools package."));
        validateSdk();
        return;
    }

    const QString message = Tr::tr("Download and install Android SDK Tools to %1?")
            .arg("\n\"" + m_sdkLocationPathChooser->filePath().cleanPath().toUserOutput()
                 + "\"");
    auto userInput = QMessageBox::information(this, Android::Internal::dialogTitle(),
                                              message, QMessageBox::Yes | QMessageBox::No);
    if (userInput == QMessageBox::Yes)
        m_sdkDownloader.start({Android::Internal::downloadSdkRecipe()});
}

// AndroidSettingsPage

class AndroidSettingsPage final : public Core::IOptionsPage
{
public:
    AndroidSettingsPage()
    {
        setId(Constants::ANDROID_SETTINGS_ID);
        setDisplayName(Tr::tr("Android"));
        setCategory(ProjectExplorer::Constants::SDK_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new AndroidSettingsWidget; });
    }
};

void setupAndroidSettingsPage()
{
    static AndroidSettingsPage theAndroidSettingsPage;
}

} // Android::Internal
