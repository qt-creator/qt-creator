/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidsettingswidget.h"

#include "ui_androidsettingswidget.h"

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidsdkdownloader.h"
#include "androidsdkmanager.h"
#include "androidsdkmanagerwidget.h"
#include "androidtoolchain.h"
#include "avddialog.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/listmodel.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/utilsicons.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QList>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QModelIndex>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QWidget>

#include <memory>

using namespace Utils;

namespace {
static Q_LOGGING_CATEGORY(androidsettingswidget, "qtc.android.androidsettingswidget", QtWarningMsg);
}

namespace Android {
namespace Internal {

class AndroidSdkManagerWidget;
class AndroidAvdManager;
class SummaryWidget;

class AvdModel final : public ListModel<AndroidDeviceInfo>
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AvdModel)

public:
    AvdModel();

    QVariant itemData(const AndroidDeviceInfo &info, int column, int role) const final;

    QString avdName(const QModelIndex &index) const;
    QModelIndex indexForAvdName(const QString &avdName) const;
};

class AndroidSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidSettingsWidget)

public:
    // Todo: This would be so much simpler if it just used Utils::PathChooser!!!
    AndroidSettingsWidget();
    ~AndroidSettingsWidget() final;

private:
    void apply() final { AndroidConfigurations::setConfig(m_androidConfig); }

    void showEvent(QShowEvent *event) override;

    void validateJdk();
    void updateNdkList();
    void onSdkPathChanged();
    void validateSdk();
    void openSDKDownloadUrl();
    void openNDKDownloadUrl();
    void openOpenJDKDownloadUrl();
    void downloadOpenSslRepo(const bool silent = false);
    void addAVD();
    void avdAdded();
    void removeAVD();
    void startAVD();
    void avdActivated(const QModelIndex &);
    void dataPartitionSizeEditingFinished();
    void createKitToggled();

    void updateUI();
    void updateAvds();

    void startUpdateAvd();
    void enableAvdControls();
    void disableAvdControls();

    void downloadSdk();
    void addCustomNdkItem();
    void validateOpenSsl();

    Ui_AndroidSettingsWidget m_ui;
    AndroidSdkManagerWidget *m_sdkManagerWidget = nullptr;
    AndroidConfig m_androidConfig{AndroidConfigurations::currentConfig()};
    AvdModel m_AVDModel;
    QFutureWatcher<CreateAvdInfo> m_futureWatcher;

    QFutureWatcher<AndroidDeviceInfoList> m_virtualDevicesWatcher;
    QString m_lastAddedAvd;
    AndroidAvdManager m_avdManager{m_androidConfig};
    AndroidSdkManager m_sdkManager{m_androidConfig};
    AndroidSdkDownloader m_sdkDownloader;
    bool m_isInitialReloadDone = false;

    SummaryWidget *m_androidSummary = nullptr;
    SummaryWidget *m_javaSummary = nullptr;
    SummaryWidget *m_openSslSummary = nullptr;
};

enum JavaValidation {
    JavaPathExistsRow,
    JavaJdkValidRow
};

enum AndroidValidation {
    SdkPathExistsRow,
    SdkPathWritableRow,
    SdkToolsInstalledRow,
    PlatformToolsInstalledRow,
    BuildToolsInstalledRow,
    SdkManagerSuccessfulRow,
    PlatformSdkInstalledRow,
    AllEssentialsInstalledRow,
};

enum OpenSslValidation {
    OpenSslPathExistsRow,
    OpenSslPriPathExists,
    OpenSslCmakeListsPathExists
};

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

    void setSetupOk(bool ok)
    {
        m_detailsWidget->setState(ok ? DetailsWidget::Collapsed : DetailsWidget::Expanded);
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

QModelIndex AvdModel::indexForAvdName(const QString &avdName) const
{
    return findIndex([avdName](const AndroidDeviceInfo &info) { return info.avdname == avdName; });
}

QString AvdModel::avdName(const QModelIndex &index) const
{
    return dataAt(index.row()).avdname;
}

QVariant AvdModel::itemData(const AndroidDeviceInfo &info, int column, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    switch (column) {
        case 0:
            return info.avdname;
        case 1:
            return info.sdk;
        case 2:
            return info.cpuAbi.isEmpty() ? QVariant() : QVariant(info.cpuAbi.first());
        case 3:
            return info.avdDevice.isEmpty() ? QVariant("Custom") : info.avdDevice;
        case 4:
            return info.avdTarget;
        case 5:
            return info.avdSdcardSize;
    }
    return {};
}

AvdModel::AvdModel()
{
    //: AVD - Android Virtual Device
    setHeader({tr("AVD Name"), tr("API"), tr("CPU/ABI"), tr("Device Type"), tr("Target"), tr("SD-card Size")});
}

void AndroidSettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!m_isInitialReloadDone) {
        validateJdk();
        // Reloading SDK packages (force) is still synchronous. Use zero timer
        // to let settings dialog open first.
        QTimer::singleShot(0, std::bind(&AndroidSdkManager::reloadPackages,
                                        &m_sdkManager, false));
        validateOpenSsl();
        m_isInitialReloadDone = true;
    }
}

void AndroidSettingsWidget::updateNdkList()
{
    m_ui.ndkListWidget->clear();
    for (const Ndk *ndk : m_sdkManager.installedNdkPackages()) {
        m_ui.ndkListWidget->addItem(new QListWidgetItem(Icons::LOCKED.icon(),
                                                        ndk->installedLocation().toString()));
    }

    for (const QString &ndk : m_androidConfig.getCustomNdkList()) {
        if (m_androidConfig.isValidNdk(ndk)) {
            m_ui.ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndk));
        } else {
            m_androidConfig.removeCustomNdk(ndk);
        }
    }

    m_ui.ndkListWidget->setCurrentRow(0);
}

void AndroidSettingsWidget::addCustomNdkItem()
{
    const QString homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    const QString ndkPath = QFileDialog::getExistingDirectory(this, tr("Select an NDK"), homePath);

    if (m_androidConfig.isValidNdk(ndkPath)) {
        m_androidConfig.addCustomNdk(ndkPath);
        if (m_ui.ndkListWidget->findItems(ndkPath, Qt::MatchExactly).size() == 0) {
            m_ui.ndkListWidget->addItem(new QListWidgetItem(Icons::UNLOCKED.icon(), ndkPath));
        }
    } else if (!ndkPath.isEmpty()) {
        QMessageBox::warning(
            this,
            tr("Add Custom NDK"),
            tr("The selected path has an invalid NDK. This might mean that the path contains space "
               "characters, or that it does not have a \"toolchains\" sub-directory, or that the "
               "NDK version could not be retrieved because of a missing \"source.properties\" or "
               "\"RELEASE.TXT\" file"));
    }
}

AndroidSettingsWidget::AndroidSettingsWidget()
{
    m_ui.setupUi(this);
    m_sdkManagerWidget = new AndroidSdkManagerWidget(m_androidConfig, &m_sdkManager,
                                                     m_ui.sdkManagerTab);
    auto sdkMangerLayout = new QVBoxLayout(m_ui.sdkManagerTab);
    sdkMangerLayout->setContentsMargins(0, 0, 0, 0);
    sdkMangerLayout->addWidget(m_sdkManagerWidget);
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::updatingSdk, [this] {
        // Disable the top level UI to keep the user from unintentionally interrupting operations
        m_ui.javaSettingsGroupBox->setEnabled(false);
        m_ui.androidSettingsGroupBox->setEnabled(false);
        m_ui.androidOpenSSLSettingsGroupBox->setEnabled(false);
        m_ui.CreateKitCheckBox->setEnabled(false);
        m_ui.managerTabWidget->tabBar()->setEnabled(false);
    });
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::updatingSdkFinished, [this] {
        m_ui.javaSettingsGroupBox->setEnabled(true);
        m_ui.androidSettingsGroupBox->setEnabled(true);
        m_ui.androidOpenSSLSettingsGroupBox->setEnabled(true);
        m_ui.CreateKitCheckBox->setEnabled(true);
        m_ui.managerTabWidget->tabBar()->setEnabled(true);
    });
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::licenseWorkflowStarted, [this] {
       m_ui.scrollArea->ensureWidgetVisible(m_ui.managerTabWidget);
    });

    QMap<int, QString> javaValidationPoints;
    javaValidationPoints[JavaPathExistsRow] = tr("JDK path exists.");
    javaValidationPoints[JavaJdkValidRow] = tr("JDK path is a valid JDK root folder.");
    m_javaSummary = new SummaryWidget(javaValidationPoints, tr("Java Settings are OK."),
                                      tr("Java settings have errors."), m_ui.javaDetailsWidget);

    QMap<int, QString> androidValidationPoints;
    androidValidationPoints[SdkPathExistsRow] = tr("Android SDK path exists.");
    androidValidationPoints[SdkPathWritableRow] = tr("Android SDK path writable.");
    androidValidationPoints[SdkToolsInstalledRow] = tr("SDK tools installed.");
    androidValidationPoints[PlatformToolsInstalledRow] = tr("Platform tools installed.");
    androidValidationPoints[SdkManagerSuccessfulRow] = tr(
        "SDK manager runs (SDK Tools versions <= 26.x require exactly Java 1.8).");
    androidValidationPoints[AllEssentialsInstalledRow] = tr(
        "All essential packages installed for all installed Qt versions.");
    androidValidationPoints[BuildToolsInstalledRow] = tr("Build tools installed.");
    androidValidationPoints[PlatformSdkInstalledRow] = tr("Platform SDK installed.");
    m_androidSummary = new SummaryWidget(androidValidationPoints, tr("Android settings are OK."),
                                         tr("Android settings have errors."),
                                         m_ui.androidDetailsWidget);

    QMap<int, QString> openSslValidationPoints;
    openSslValidationPoints[OpenSslPathExistsRow] = tr("OpenSSL path exists.");
    openSslValidationPoints[OpenSslPriPathExists] = tr(
        "QMake include project (openssl.pri) exists.");
    openSslValidationPoints[OpenSslCmakeListsPathExists] = tr(
        "CMake include project (CMakeLists.txt) exists.");
    m_openSslSummary = new SummaryWidget(openSslValidationPoints,
                                         tr("OpenSSL Settings are OK."),
                                         tr("OpenSSL settings have errors."),
                                         m_ui.openSslDetailsWidget);

    connect(m_ui.OpenJDKLocationPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateJdk);
    FilePath currentJdkPath = m_androidConfig.openJDKLocation();
    if (currentJdkPath.isEmpty())
        currentJdkPath = AndroidConfig::getJdkPath();
    m_ui.OpenJDKLocationPathChooser->setFilePath(currentJdkPath);
    m_ui.OpenJDKLocationPathChooser->setPromptDialogTitle(tr("Select JDK Path"));

    FilePath currentSDKPath = m_androidConfig.sdkLocation();
    if (currentSDKPath.isEmpty())
        currentSDKPath = AndroidConfig::defaultSdkPath();

    m_ui.SDKLocationPathChooser->setFilePath(currentSDKPath);
    m_ui.SDKLocationPathChooser->setPromptDialogTitle(tr("Select Android SDK Folder"));

    m_ui.openSslPathChooser->setPromptDialogTitle(tr("Select OpenSSL Include Project File"));
    FilePath currentOpenSslPath = m_androidConfig.openSslLocation();
    if (currentOpenSslPath.isEmpty())
        currentOpenSslPath = currentSDKPath.pathAppended("android_openssl");
    m_ui.openSslPathChooser->setFilePath(currentOpenSslPath);

    m_ui.DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize());
    m_ui.CreateKitCheckBox->setChecked(m_androidConfig.automaticKitCreation());
    m_ui.AVDTableView->setModel(&m_AVDModel);
    m_ui.AVDTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui.AVDTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    const QIcon downloadIcon = Icons::ONLINE.icon();
    m_ui.downloadSDKToolButton->setIcon(downloadIcon);
    m_ui.downloadNDKToolButton->setIcon(downloadIcon);
    m_ui.downloadOpenJDKToolButton->setIcon(downloadIcon);
    m_ui.sdkToolsAutoDownloadButton->setToolTip(tr(
            "Automatically download Android SDK Tools to selected location.\n\n"
            "If the selected path contains no valid SDK Tools, the SDK Tools package "
            "is downloaded from %1, and extracted to the selected path.\n"
            "After the SDK Tools are properly set up, you are prompted to install "
            "any essential packages required for Qt to build for Android.")
                                                 .arg(m_androidConfig.sdkToolsUrl().toString()));

    connect(m_ui.SDKLocationPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::onSdkPathChanged);

    connect(m_ui.ndkListWidget, &QListWidget::currentTextChanged, [this](const QString &ndk) {
        updateUI();
        m_ui.removeCustomNdkButton->setEnabled(m_androidConfig.getCustomNdkList().contains(ndk));
    });
    connect(m_ui.addCustomNdkButton, &QPushButton::clicked, this,
            &AndroidSettingsWidget::addCustomNdkItem);
    connect(m_ui.removeCustomNdkButton, &QPushButton::clicked, this, [this] {
        m_androidConfig.removeCustomNdk(m_ui.ndkListWidget->currentItem()->text());
        m_ui.ndkListWidget->takeItem(m_ui.ndkListWidget->currentRow());
    });

    connect(m_ui.openSslPathChooser, &PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateOpenSsl);
    connect(&m_virtualDevicesWatcher, &QFutureWatcherBase::finished,
            this, &AndroidSettingsWidget::updateAvds);
    connect(m_ui.AVDRefreshPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::startUpdateAvd);
    connect(&m_futureWatcher, &QFutureWatcherBase::finished,
            this, &AndroidSettingsWidget::avdAdded);
    connect(m_ui.AVDAddPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::addAVD);
    connect(m_ui.AVDRemovePushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::removeAVD);
    connect(m_ui.AVDStartPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::startAVD);
    connect(m_ui.AVDTableView, &QAbstractItemView::activated,
            this, &AndroidSettingsWidget::avdActivated);
    connect(m_ui.AVDTableView, &QAbstractItemView::clicked,
            this, &AndroidSettingsWidget::avdActivated);
    connect(m_ui.DataPartitionSizeSpinBox, &QAbstractSpinBox::editingFinished,
            this, &AndroidSettingsWidget::dataPartitionSizeEditingFinished);
    connect(m_ui.CreateKitCheckBox, &QAbstractButton::toggled,
            this, &AndroidSettingsWidget::createKitToggled);
    connect(m_ui.downloadNDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openNDKDownloadUrl);
    connect(m_ui.downloadSDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openSDKDownloadUrl);
    connect(m_ui.downloadOpenSSLPrebuiltLibs, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadOpenSslRepo);
    connect(m_ui.downloadOpenJDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openOpenJDKDownloadUrl);
    // Validate SDK again after any change in SDK packages.
    connect(&m_sdkManager, &AndroidSdkManager::packageReloadFinished,
            this, &AndroidSettingsWidget::validateSdk);
    connect(m_ui.sdkToolsAutoDownloadButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadSdk);
    connect(&m_sdkDownloader, &AndroidSdkDownloader::sdkDownloaderError, this, [this](const QString &error) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(), error);
    });
    connect(&m_sdkDownloader, &AndroidSdkDownloader::sdkExtracted, this, [this] {
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
}

AndroidSettingsWidget::~AndroidSettingsWidget()
{
    // Deleting m_sdkManagerWidget will cancel all ongoing and pending sdkmanager operations.
    delete m_sdkManagerWidget;
    m_futureWatcher.waitForFinished();
}

void AndroidSettingsWidget::disableAvdControls()
{
    m_ui.AVDAddPushButton->setEnabled(false);
    m_ui.AVDTableView->setEnabled(false);
    m_ui.AVDRemovePushButton->setEnabled(false);
    m_ui.AVDStartPushButton->setEnabled(false);
}

void AndroidSettingsWidget::enableAvdControls()
{
    m_ui.AVDTableView->setEnabled(true);
    m_ui.AVDAddPushButton->setEnabled(true);
    avdActivated(m_ui.AVDTableView->currentIndex());
}

void AndroidSettingsWidget::startUpdateAvd()
{
    disableAvdControls();
    m_virtualDevicesWatcher.setFuture(m_avdManager.avdList());
}

void AndroidSettingsWidget::updateAvds()
{
    m_AVDModel.setAllData(m_virtualDevicesWatcher.result());
    if (!m_lastAddedAvd.isEmpty()) {
        m_ui.AVDTableView->setCurrentIndex(m_AVDModel.indexForAvdName(m_lastAddedAvd));
        m_lastAddedAvd.clear();
    }
    enableAvdControls();
}

void AndroidSettingsWidget::validateJdk()
{
    m_androidConfig.setOpenJDKLocation(m_ui.OpenJDKLocationPathChooser->filePath());
    bool jdkPathExists = m_androidConfig.openJDKLocation().exists();
    m_javaSummary->setPointValid(JavaPathExistsRow, jdkPathExists);

    const FilePath bin = m_androidConfig.openJDKLocation().pathAppended("bin/javac" QTC_HOST_EXE_SUFFIX);
    m_javaSummary->setPointValid(JavaJdkValidRow, jdkPathExists && bin.exists());

    updateUI();

    if (m_isInitialReloadDone)
        m_sdkManager.reloadPackages(true);
}

void AndroidSettingsWidget::validateOpenSsl()
{
    m_androidConfig.setOpenSslLocation(m_ui.openSslPathChooser->filePath());

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
    auto sdkPath = FilePath::fromUserInput(m_ui.SDKLocationPathChooser->rawPath());
    m_androidConfig.setSdkLocation(sdkPath);
    FilePath currentOpenSslPath = m_androidConfig.openSslLocation();
    if (currentOpenSslPath.isEmpty() || !currentOpenSslPath.exists())
        currentOpenSslPath = sdkPath.pathAppended("android_openssl");
    m_ui.openSslPathChooser->setFileName(currentOpenSslPath);
    // Package reload will trigger validateSdk.
    m_sdkManager.reloadPackages();
}

void AndroidSettingsWidget::validateSdk()
{
    auto sdkPath = FilePath::fromUserInput(m_ui.SDKLocationPathChooser->rawPath());
    m_androidConfig.setSdkLocation(sdkPath);

    m_androidSummary->setPointValid(SdkPathExistsRow, m_androidConfig.sdkLocation().exists());
    m_androidSummary->setPointValid(SdkPathWritableRow, m_androidConfig.sdkLocation().isWritablePath());
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

    const bool sdkToolsOk = m_androidSummary->rowsOk({SdkPathExistsRow,
                                                      SdkPathWritableRow,
                                                      SdkToolsInstalledRow,
                                                      SdkManagerSuccessfulRow});
    const bool componentsOk = m_androidSummary->rowsOk({PlatformToolsInstalledRow,
                                                        BuildToolsInstalledRow,
                                                        PlatformSdkInstalledRow,
                                                        AllEssentialsInstalledRow});
    m_androidConfig.setSdkFullyConfigured(sdkToolsOk && componentsOk);
    if (sdkToolsOk && !componentsOk) {
        // Ask user to install essential SDK components. Works only for sdk tools version >= 26.0.0
        QString message = tr("Android SDK installation is missing necessary packages. Do you "
                             "want to install the missing packages?");
        auto userInput = QMessageBox::information(this, tr("Missing Android SDK Packages"),
                                                  message, QMessageBox::Yes | QMessageBox::No);
        if (userInput == QMessageBox::Yes) {
            m_ui.managerTabWidget->setCurrentWidget(m_ui.sdkManagerTab);
            m_sdkManagerWidget->installEssentials();
        }
    }

    startUpdateAvd();
    updateNdkList();
    updateUI();
}

void AndroidSettingsWidget::openSDKDownloadUrl()
{
    QDesktopServices::openUrl(QUrl::fromUserInput("https://developer.android.com/studio/"));
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
    const FilePath openSslPath = m_ui.openSslPathChooser->filePath();
    const QString openSslCloneTitle(tr("OpenSSL Cloning"));

    if (m_openSslSummary->allRowsOk()) {
        if (!silent) {
            QMessageBox::information(this, openSslCloneTitle,
                tr("OpenSSL prebuilt libraries repository is already configured."));
        }
        return;
    }

    const QString openSslRepo("https://github.com/KDAB/android_openssl.git");
    QtcProcess *gitCloner = new QtcProcess(this);
    CommandLine gitCloneCommand("git", {"clone", "--depth=1", openSslRepo, openSslPath.toString()});
    gitCloner->setCommand(gitCloneCommand);

    qCDebug(androidsettingswidget) << "Cloning OpenSSL repo: " << gitCloneCommand.toUserOutput();

    QDir openSslDir(openSslPath.toString());
    const bool isEmptyDir = openSslDir.isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot
                                               | QDir::Hidden | QDir::System);
    if (openSslDir.exists() && !isEmptyDir) {
        QMessageBox::information(
            this,
            openSslCloneTitle,
            tr("The selected download path (%1) for OpenSSL already exists and the directory is "
               "not empty. Select a different path or make sure it is an empty directory.")
                .arg(QDir::toNativeSeparators(openSslPath.toString())));
        return;
    }

    QProgressDialog *openSslProgressDialog
        = new QProgressDialog(tr("Cloning OpenSSL prebuilt libraries..."),
                              tr("Cancel"), 0, 0);
    openSslProgressDialog->setWindowModality(Qt::ApplicationModal);
    openSslProgressDialog->setWindowTitle(openSslCloneTitle);
    openSslProgressDialog->setFixedSize(openSslProgressDialog->sizeHint());

    connect(openSslProgressDialog, &QProgressDialog::canceled, gitCloner, &QtcProcess::kill);

    auto failDialog = [=](const QString &msgSuffix = {}) {
        QMessageBox msgBox;
        msgBox.setText(tr("OpenSSL prebuilt libraries cloning failed. ") + msgSuffix
                       + tr("Opening OpenSSL URL for manual download."));
        msgBox.addButton(tr("OK"), QMessageBox::YesRole);
        QAbstractButton *openButton = msgBox.addButton(tr("Open Download URL"), QMessageBox::ActionRole);
        msgBox.exec();

        if (msgBox.clickedButton() == openButton)
            QDesktopServices::openUrl(QUrl::fromUserInput(openSslRepo));
        openButton->deleteLater();
    };

    connect(gitCloner,
            QOverload<int, QtcProcess::ExitStatus>::of(&QtcProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                openSslProgressDialog->close();
                validateOpenSsl();
                m_ui.openSslPathChooser->triggerChanged(); // After cloning, the path exists

                if (!openSslProgressDialog->wasCanceled()
                    || (exitStatus == QtcProcess::NormalExit && exitCode != 0)) {
                    failDialog();
                }
            });

    connect(gitCloner, &QtcProcess::errorOccurred, this, [=](QProcess::ProcessError error) {
        openSslProgressDialog->close();
        if (error == QProcess::FailedToStart) {
            failDialog(tr("The Git tool might not be installed properly on your system. "));
        } else {
            failDialog();
        }
    });

    openSslProgressDialog->show();
    gitCloner->start();
}

void AndroidSettingsWidget::addAVD()
{
    disableAvdControls();
    CreateAvdInfo info = AvdDialog::gatherCreateAVDInfo(this, &m_sdkManager, m_androidConfig);

    if (!info.isValid()) {
        enableAvdControls();
        return;
    }

    m_futureWatcher.setFuture(m_avdManager.createAvd(info));
}

void AndroidSettingsWidget::avdAdded()
{
    CreateAvdInfo info = m_futureWatcher.result();
    if (!info.error.isEmpty()) {
        enableAvdControls();
        QMessageBox::critical(this, QApplication::translate("AndroidConfig", "Error Creating AVD"), info.error);
        return;
    }

    startUpdateAvd();
    m_lastAddedAvd = info.name;
}

void AndroidSettingsWidget::removeAVD()
{
    disableAvdControls();
    QString avdName = m_AVDModel.avdName(m_ui.AVDTableView->currentIndex());
    if (QMessageBox::question(this, tr("Remove Android Virtual Device"),
                              tr("Remove device \"%1\"? This cannot be undone.").arg(avdName),
                              QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::No) {
        enableAvdControls();
        return;
    }

    m_avdManager.removeAvd(avdName);
    startUpdateAvd();
}

void AndroidSettingsWidget::startAVD()
{
    m_avdManager.startAvdAsync(m_AVDModel.avdName(m_ui.AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(const QModelIndex &index)
{
    m_ui.AVDRemovePushButton->setEnabled(index.isValid());
    m_ui.AVDStartPushButton->setEnabled(index.isValid());
}

void AndroidSettingsWidget::dataPartitionSizeEditingFinished()
{
    m_androidConfig.setPartitionSize(m_ui.DataPartitionSizeSpinBox->value());
}

void AndroidSettingsWidget::createKitToggled()
{
    m_androidConfig.setAutomaticKitCreation(m_ui.CreateKitCheckBox->isChecked());
}

void AndroidSettingsWidget::updateUI()
{
    const bool javaSetupOk = m_javaSummary->allRowsOk();
    const bool sdkToolsOk = m_androidSummary->rowsOk({SdkPathExistsRow, SdkPathWritableRow, SdkToolsInstalledRow});
    const bool androidSetupOk = m_androidSummary->allRowsOk();
    const bool openSslOk = m_openSslSummary->allRowsOk();

    m_ui.avdManagerTab->setEnabled(javaSetupOk && androidSetupOk);
    m_ui.sdkManagerTab->setEnabled(sdkToolsOk);

    const QListWidgetItem *currentItem = m_ui.ndkListWidget->currentItem();
    const FilePath currentNdk = FilePath::fromString(currentItem ? currentItem->text() : "");
    const QString infoText = tr("(SDK Version: %1, NDK Version: %2)")
            .arg(m_androidConfig.sdkToolsVersion().toString())
            .arg(currentNdk.isEmpty() ? "" : m_androidConfig.ndkVersion(currentNdk).toString());
    m_androidSummary->setInfoText(androidSetupOk ? infoText : "");

    m_javaSummary->setSetupOk(javaSetupOk);
    m_androidSummary->setSetupOk(androidSetupOk);
    m_openSslSummary->setSetupOk(openSslOk);
}

void AndroidSettingsWidget::downloadSdk()
{
    if (m_androidConfig.sdkToolsOk()) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(),
                             tr("The selected path already has a valid SDK Tools package."));
        validateSdk();
        return;
    }

    const QString message = tr("Download and install Android SDK Tools to: %1?")
                        .arg(QDir::toNativeSeparators(m_ui.SDKLocationPathChooser->rawPath()));
    auto userInput = QMessageBox::information(this, AndroidSdkDownloader::dialogTitle(),
                                              message, QMessageBox::Yes | QMessageBox::No);
    if (userInput == QMessageBox::Yes) {
        if (m_javaSummary->allRowsOk()) {
            auto javaPath = FilePath::fromUserInput(m_ui.OpenJDKLocationPathChooser->rawPath());
            m_sdkDownloader.downloadAndExtractSdk(javaPath.toString(),
                                                  m_ui.SDKLocationPathChooser->filePath().toString());
        }
    }
}

// AndroidSettingsPage

AndroidSettingsPage::AndroidSettingsPage()
{
    setId(Constants::ANDROID_SETTINGS_ID);
    setDisplayName(AndroidSettingsWidget::tr("Android"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] {
        auto widget = new AndroidSettingsWidget;
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Window, Utils::creatorTheme()->color(Theme::BackgroundColorNormal));
        widget->setPalette(pal);
        return widget;
    });
}

} // namespace Internal
} // namespace Android
