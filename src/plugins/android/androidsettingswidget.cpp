// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidsdkmanager.h"
#include "androidsdkmanagerwidget.h"
#include "androidsettingswidget.h"
#include "androidtr.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/progressindicator.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
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

#include <memory>

using namespace Utils;

namespace Android::Internal {

static Q_LOGGING_CATEGORY(androidsettingswidget, "qtc.android.androidsettingswidget", QtWarningMsg);

class SummaryWidget : public QWidget
{
    class RowData {
    public:
        InfoLabel *m_infoLabel = nullptr;
        bool m_valid = false;
    };

public:
    SummaryWidget(const QMap<int, QString> &validationPoints, const QString &validText,
                  const QString &invalidText, DetailsWidget *detailsWidget) :
        QWidget(detailsWidget),
        m_validText(validText),
        m_invalidText(invalidText),
        m_detailsWidget(detailsWidget)
    {
        QTC_CHECK(m_detailsWidget);
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(22, 0, 0, 12);
        layout->setSpacing(4);
        for (auto itr = validationPoints.cbegin(); itr != validationPoints.cend(); ++itr) {
            RowData data;
            data.m_infoLabel = new InfoLabel(itr.value());
            layout->addWidget(data.m_infoLabel);
            m_validationData[itr.key()] = data;
            setPointValid(itr.key(), false);
        }
        m_detailsWidget->setWidget(this);
        setContentsMargins(0, 0, 0, 0);
    }

    void setPointValid(int key, bool valid)
    {
        if (!m_validationData.contains(key))
            return;
        RowData &data = m_validationData[key];
        data.m_valid = valid;
        data.m_infoLabel->setType(valid ? InfoLabel::Ok : InfoLabel::NotOk);
        updateUi();
    }

    bool rowsOk(const QList<int> &keys) const
    {
        for (auto key : keys) {
            if (!m_validationData[key].m_valid)
                return false;
        }
        return true;
    }

    bool allRowsOk() const
    {
        return rowsOk(m_validationData.keys());
    }

    void setInfoText(const QString &text)
    {
        m_infoText = text;
        updateUi();
    }

    void setInProgressText(const QString &text)
    {
        m_detailsWidget->setIcon({});
        m_detailsWidget->setSummaryText(QString("%1...").arg(text));
        m_detailsWidget->setState(DetailsWidget::Collapsed);
    }

    void setSetupOk(bool ok)
    {
        m_detailsWidget->setState(ok ? DetailsWidget::Collapsed : DetailsWidget::Expanded);
    }

    void setState(DetailsWidget::State state)
    {
        m_detailsWidget->setState(state);
    }

private:
    void updateUi() {
        bool ok = allRowsOk();
        m_detailsWidget->setIcon(ok ? Icons::OK.icon() : Icons::CRITICAL.icon());
        m_detailsWidget->setSummaryText(ok ? QString("%1 %2").arg(m_validText).arg(m_infoText)
                                           : m_invalidText);
    }
    QString m_validText;
    QString m_invalidText;
    QString m_infoText;
    DetailsWidget *m_detailsWidget = nullptr;
    QMap<int, RowData> m_validationData;
};

class AndroidSettingsWidget final : public Core::IOptionsPageWidget
{
public:
    // Todo: This would be so much simpler if it just used Utils::PathChooser!!!
    AndroidSettingsWidget();
    ~AndroidSettingsWidget() final;

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

    AndroidSdkManagerWidget *m_sdkManagerWidget = nullptr;
    AndroidConfig &m_androidConfig{AndroidConfigurations::currentConfig()};

    AndroidSdkManager m_sdkManager{m_androidConfig};
    AndroidSdkDownloader m_sdkDownloader;
    bool m_isInitialReloadDone = false;

    SummaryWidget *m_androidSummary = nullptr;
    SummaryWidget *m_openSslSummary = nullptr;

    ProgressIndicator *m_androidProgress = nullptr;

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
                .arg(m_androidConfig.sdkToolsUrl().toString()));

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


    m_sdkManagerWidget = new AndroidSdkManagerWidget(m_androidConfig, &m_sdkManager, this);

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
    m_androidProgress = new ProgressIndicator(ProgressIndicatorSize::Medium, this);
    m_androidProgress->attachToWidget(androidDetailsWidget);
    m_androidProgress->hide();

    const QMap<int, QString> openSslValidationPoints = {
        { OpenSslPathExistsRow, Tr::tr("OpenSSL path exists.") },
        { OpenSslPriPathExists, Tr::tr("QMake include project (openssl.pri) exists.") },
        { OpenSslCmakeListsPathExists, Tr::tr("CMake include project (CMakeLists.txt) exists.") }
   };
    m_openSslSummary = new SummaryWidget(openSslValidationPoints,
                                         Tr::tr("OpenSSL Settings are OK."),
                                         Tr::tr("OpenSSL settings have errors."),
                                         openSslDetailsWidget);

    connect(m_openJdkLocationPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateJdk);
    if (m_androidConfig.openJDKLocation().isEmpty())
        m_androidConfig.setOpenJDKLocation(AndroidConfig::getJdkPath());
    m_openJdkLocationPathChooser->setFilePath(m_androidConfig.openJDKLocation());
    m_openJdkLocationPathChooser->setPromptDialogTitle(Tr::tr("Select JDK Path"));

    if (m_androidConfig.sdkLocation().isEmpty())
        m_androidConfig.setSdkLocation(AndroidConfig::defaultSdkPath());
    m_sdkLocationPathChooser->setFilePath(m_androidConfig.sdkLocation());
    m_sdkLocationPathChooser->setPromptDialogTitle(Tr::tr("Select Android SDK Folder"));

    m_openSslPathChooser->setPromptDialogTitle(Tr::tr("Select OpenSSL Include Project File"));
    if (m_androidConfig.openSslLocation().isEmpty())
        m_androidConfig.setOpenSslLocation(m_androidConfig.sdkLocation() / ("android_openssl"));
    m_openSslPathChooser->setFilePath(m_androidConfig.openSslLocation());

    m_createKitCheckBox->setChecked(m_androidConfig.automaticKitCreation());

    downloadNdkToolButton->setIcon(downloadIcon);

    using namespace Layouting;

    Column {
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
            title(Tr::tr("Android OpenSSL settings (Optional)")),
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
        removeCustomNdkButton->setEnabled(m_androidConfig.getCustomNdkList().contains(ndk));
    });
    connect(addCustomNdkButton, &QPushButton::clicked, this,
            &AndroidSettingsWidget::addCustomNdkItem);
    connect(removeCustomNdkButton, &QPushButton::clicked, this, [this] {
        if (isDefaultNdkSelected())
            m_androidConfig.setDefaultNdk({});
        m_androidConfig.removeCustomNdk(m_ndkListWidget->currentItem()->text());
        m_ndkListWidget->takeItem(m_ndkListWidget->currentRow());
    });
    connect(m_makeDefaultNdkButton, &QPushButton::clicked, this, [this] {
        const FilePath defaultNdk = isDefaultNdkSelected()
                ? FilePath()
                : FilePath::fromUserInput(m_ndkListWidget->currentItem()->text());
        m_androidConfig.setDefaultNdk(defaultNdk);
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

    // Validate SDK again after any change in SDK packages.
    connect(&m_sdkManager, &AndroidSdkManager::packageReloadFinished,
            this, &AndroidSettingsWidget::validateSdk);
    connect(&m_sdkManager, &AndroidSdkManager::packageReloadFinished,
            m_androidProgress, &ProgressIndicator::hide);
    connect(&m_sdkManager, &AndroidSdkManager::packageReloadBegin, this, [this] {
        m_androidSummary->setInProgressText("Retrieving packages information");
        m_androidProgress->show();
    });
    connect(sdkManagerToolButton, &QAbstractButton::clicked,
            this, [this] { m_sdkManagerWidget->exec(); });
    connect(sdkToolsAutoDownloadButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadSdk);
    connect(&m_sdkDownloader, &AndroidSdkDownloader::sdkDownloaderError, this, [this](const QString &error) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(), error);
    });
    connect(&m_sdkDownloader, &AndroidSdkDownloader::sdkExtracted, this, [this] {
        // Make sure the sdk path is created before installing packages
        const FilePath sdkPath = m_androidConfig.sdkLocation();
        if (!sdkPath.createDir()) {
            QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(),
                                 Tr::tr("Failed to create the SDK Tools path %1.")
                                 .arg("\n\"" + sdkPath.toUserOutput() + "\""));
        }
        m_sdkManager.reloadPackages(true);
        updateUI();
        apply();

        QMetaObject::Connection *const openSslOneShot = new QMetaObject::Connection;
        *openSslOneShot = connect(&m_sdkManager, &AndroidSdkManager::packageReloadFinished,
                                  this, [this, openSslOneShot] {
                                      QObject::disconnect(*openSslOneShot);
                                      downloadOpenSslRepo(true);
                                      delete openSslOneShot;
        });
    });

    setOnApply([this] { AndroidConfigurations::setConfig(m_androidConfig); });
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    // Deleting m_sdkManagerWidget will cancel all ongoing and pending sdkmanager operations.
    delete m_sdkManagerWidget;
}

void AndroidSettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!m_isInitialReloadDone) {
        validateJdk();
        // Reloading SDK packages (force) is still synchronous. Use zero timer
        // to let settings dialog open first.
        QTimer::singleShot(0, &m_sdkManager, std::bind(&AndroidSdkManager::reloadPackages,
                                                       &m_sdkManager, false));
        validateOpenSsl();
        m_isInitialReloadDone = true;
    }
}

void AndroidSettingsWidget::updateNdkList()
{
    m_ndkListWidget->clear();
    const auto installedPkgs = m_sdkManager.installedNdkPackages();
    for (const Ndk *ndk : installedPkgs) {
        m_ndkListWidget->addItem(new QListWidgetItem(Icons::LOCKED.icon(),
                                                        ndk->installedLocation().toUserOutput()));
    }

    const auto customNdks = m_androidConfig.getCustomNdkList();
    for (const QString &ndk : customNdks) {
        if (m_androidConfig.isValidNdk(ndk)) {
            m_ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndk));
        } else {
            m_androidConfig.removeCustomNdk(ndk);
        }
    }

    m_ndkListWidget->setCurrentRow(0);

    updateUI();
}

void AndroidSettingsWidget::addCustomNdkItem()
{
    const QString homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation)
            .constFirst();
    const QString ndkPath = QFileDialog::getExistingDirectory(this, Tr::tr("Select an NDK"), homePath);

    if (m_androidConfig.isValidNdk(ndkPath)) {
        m_androidConfig.addCustomNdk(ndkPath);
        if (m_ndkListWidget->findItems(ndkPath, Qt::MatchExactly).size() == 0) {
            m_ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndkPath));
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
    if (!m_androidConfig.defaultNdk().isEmpty()) {
        if (const QListWidgetItem *item = m_ndkListWidget->currentItem()) {
            return FilePath::fromUserInput(item->text()) == m_androidConfig.defaultNdk();
        }
    }
    return false;
}

void AndroidSettingsWidget::validateJdk()
{
    m_androidConfig.setOpenJDKLocation(m_openJdkLocationPathChooser->filePath());
    bool jdkPathExists = m_androidConfig.openJDKLocation().exists();
    const FilePath bin = m_androidConfig.openJDKLocation()
                                        .pathAppended("bin/javac" QTC_HOST_EXE_SUFFIX);
    m_androidSummary->setPointValid(JavaPathExistsAndWritableRow, jdkPathExists && bin.exists());

    updateUI();

    if (m_isInitialReloadDone)
        m_sdkManager.reloadPackages(true);
}

void AndroidSettingsWidget::validateOpenSsl()
{
    m_androidConfig.setOpenSslLocation(m_openSslPathChooser->filePath());

    m_openSslSummary->setPointValid(OpenSslPathExistsRow, m_androidConfig.openSslLocation().exists());

    const bool priFileExists = m_androidConfig.openSslLocation().pathAppended("openssl.pri").exists();
    m_openSslSummary->setPointValid(OpenSslPriPathExists, priFileExists);
    const bool cmakeListsExists
        = m_androidConfig.openSslLocation().pathAppended("CMakeLists.txt").exists();
    m_openSslSummary->setPointValid(OpenSslCmakeListsPathExists, cmakeListsExists);

    updateUI();
}

void AndroidSettingsWidget::onSdkPathChanged()
{
    const FilePath sdkPath = m_sdkLocationPathChooser->filePath().cleanPath();
    m_androidConfig.setSdkLocation(sdkPath);
    FilePath currentOpenSslPath = m_androidConfig.openSslLocation();
    if (currentOpenSslPath.isEmpty() || !currentOpenSslPath.exists())
        currentOpenSslPath = sdkPath.pathAppended("android_openssl");
    m_openSslPathChooser->setFilePath(currentOpenSslPath);
    // Package reload will trigger validateSdk.
    m_sdkManager.reloadPackages();
}

void AndroidSettingsWidget::validateSdk()
{
    const FilePath sdkPath = m_sdkLocationPathChooser->filePath().cleanPath();
    m_androidConfig.setSdkLocation(sdkPath);

    const FilePath path = m_androidConfig.sdkLocation();
    m_androidSummary->setPointValid(SdkPathExistsAndWritableRow,
                                    path.exists() && path.isWritableDir());
    m_androidSummary->setPointValid(SdkToolsInstalledRow,
                                    !m_androidConfig.sdkToolsVersion().isNull());
    m_androidSummary->setPointValid(PlatformToolsInstalledRow,
                                    m_androidConfig.adbToolPath().exists());
    m_androidSummary->setPointValid(BuildToolsInstalledRow,
                                    !m_androidConfig.buildToolsVersion().isNull());
    m_androidSummary->setPointValid(SdkManagerSuccessfulRow, m_sdkManager.packageListingSuccessful());
    // installedSdkPlatforms should not trigger a package reload as validate SDK is only called
    // after AndroidSdkManager::packageReloadFinished.
    m_androidSummary->setPointValid(PlatformSdkInstalledRow,
                                    !m_sdkManager.installedSdkPlatforms().isEmpty());
    m_androidSummary->setPointValid(AllEssentialsInstalledRow,
                                    m_androidConfig.allEssentialsInstalled(&m_sdkManager));

    const bool sdkToolsOk = m_androidSummary->rowsOk({SdkPathExistsAndWritableRow,
                                                      SdkToolsInstalledRow,
                                                      SdkManagerSuccessfulRow});
    const bool componentsOk = m_androidSummary->rowsOk({PlatformToolsInstalledRow,
                                                        BuildToolsInstalledRow,
                                                        PlatformSdkInstalledRow,
                                                        AllEssentialsInstalledRow});
    m_androidConfig.setSdkFullyConfigured(sdkToolsOk && componentsOk);
    if (sdkToolsOk && !componentsOk)
        m_sdkManagerWidget->installEssentials();

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
    if (HostOsInfo::isLinuxHost())
        QDesktopServices::openUrl(QUrl::fromUserInput("https://openjdk.java.net/install/"));
    else
        QDesktopServices::openUrl(QUrl::fromUserInput("https://adoptopenjdk.net/"));
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

    QDir openSslDir(openSslPath.toString());
    const bool isEmptyDir = openSslDir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot
                                               | QDir::Hidden | QDir::System);
    if (openSslDir.exists() && !isEmptyDir) {
        QMessageBox::information(
            this,
            openSslCloneTitle,
            Tr::tr("The selected download path (%1) for OpenSSL already exists and the directory is "
                   "not empty. Select a different path or make sure it is an empty directory.")
            .arg(QDir::toNativeSeparators(openSslPath.toString())));
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
    CommandLine gitCloneCommand("git", {"clone", "--depth=1", openSslRepo, openSslPath.toString()});
    gitCloner->setCommand(gitCloneCommand);

    qCDebug(androidsettingswidget) << "Cloning OpenSSL repo: " << gitCloneCommand.toUserOutput();

    connect(openSslProgressDialog, &QProgressDialog::canceled, gitCloner, &QObject::deleteLater);

    auto failDialog = [=](const QString &msgSuffix = {}) {
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

    connect(gitCloner, &Process::done, this, [=] {
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
    m_androidConfig.setAutomaticKitCreation(m_createKitCheckBox->isChecked());
}

void AndroidSettingsWidget::updateUI()
{
    const bool androidSetupOk = m_androidSummary->allRowsOk();
    const bool openSslOk = m_openSslSummary->allRowsOk();

    const QListWidgetItem *currentItem = m_ndkListWidget->currentItem();
    const FilePath currentNdk = FilePath::fromUserInput(currentItem ? currentItem->text() : "");
    const QString infoText = Tr::tr("(SDK Version: %1, NDK Version: %2)")
            .arg(m_androidConfig.sdkToolsVersion().toString())
            .arg(currentNdk.isEmpty() ? "" : m_androidConfig.ndkVersion(currentNdk).toString());
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
                    FilePath::fromUserInput(item->text()) == m_androidConfig.defaultNdk();
            item->setFont(isDefaultNdk ? markedFont : font);
        }
    }

    m_makeDefaultNdkButton->setText(isDefaultNdkSelected() ? Tr::tr("Unset Default")
                                                           : Tr::tr("Make Default"));
}

void AndroidSettingsWidget::downloadSdk()
{
    if (m_androidConfig.sdkToolsOk()) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(),
                             Tr::tr("The selected path already has a valid SDK Tools package."));
        validateSdk();
        return;
    }

    const QString message = Tr::tr("Download and install Android SDK Tools to %1?")
            .arg("\n\"" + m_sdkLocationPathChooser->filePath().cleanPath().toUserOutput()
                 + "\"");
    auto userInput = QMessageBox::information(this, AndroidSdkDownloader::dialogTitle(),
                                              message, QMessageBox::Yes | QMessageBox::No);
    if (userInput == QMessageBox::Yes)
        m_sdkDownloader.downloadAndExtractSdk();
}

// AndroidSettingsPage

AndroidSettingsPage::AndroidSettingsPage()
{
    setId(Constants::ANDROID_SETTINGS_ID);
    setDisplayName(Tr::tr("Android"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new AndroidSettingsWidget; });
}

} // Android::Internal
