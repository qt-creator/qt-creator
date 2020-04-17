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

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androidavdmanager.h"
#include "androidsdkmanager.h"
#include "avddialog.h"
#include "androidsdkdownloader.h"
#include "androidsdkmanagerwidget.h"

#include <utils/qtcassert.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>
#include <utils/utilsicons.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionmanager.h>

#include <QAbstractTableModel>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QList>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QModelIndex>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QWidget>

#include <memory>

namespace {
static Q_LOGGING_CATEGORY(androidsettingswidget, "qtc.android.androidsettingswidget", QtWarningMsg);
}

namespace Android {
namespace Internal {

class AndroidSdkManagerWidget;

class AndroidAvdManager;

class AvdModel final : public QAbstractTableModel
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AvdModel)

public:
    void setAvdList(const AndroidDeviceInfoList &list);
    QString avdName(const QModelIndex &index) const;
    QModelIndex indexForAvdName(const QString &avdName) const;

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const final;
    int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    int columnCount(const QModelIndex &parent = QModelIndex()) const final;

private:
    AndroidDeviceInfoList m_list;
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

    void validateJdk();
    Utils::FilePath findJdkInCommonPaths();
    void validateNdk();
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
    void manageAVD();
    void createKitToggled();

    void updateUI();
    void updateAvds();

    void startUpdateAvd();
    void enableAvdControls();
    void disableAvdControls();

    void downloadSdk();
    bool allEssentialsInstalled();
    bool sdkToolsOk() const;
    Utils::FilePath getDefaultSdkPath();
    void showEvent(QShowEvent *event) override;
    void addCustomNdkItem();
    void validateOpenSsl();

    Ui_AndroidSettingsWidget *m_ui;
    AndroidSdkManagerWidget *m_sdkManagerWidget = nullptr;
    AndroidConfig m_androidConfig;
    AvdModel m_AVDModel;
    QFutureWatcher<CreateAvdInfo> m_futureWatcher;

    QFutureWatcher<AndroidDeviceInfoList> m_virtualDevicesWatcher;
    QString m_lastAddedAvd;
    std::unique_ptr<AndroidAvdManager> m_avdManager;
    std::unique_ptr<AndroidSdkManager> m_sdkManager;
    std::unique_ptr<AndroidSdkDownloader> m_sdkDownloader;
    bool m_isInitialReloadDone = false;
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
    NdkPathExistsRow,
    NdkDirStructureRow,
    NdkinstallDirOkRow
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
        Utils::InfoLabel *m_infoLabel = nullptr;
        bool m_valid = false;
    };

public:
    SummaryWidget(const QMap<int, QString> &validationPoints, const QString &validText,
                  const QString &invalidText, Utils::DetailsWidget *detailsWidget) :
        QWidget(detailsWidget),
        m_validText(validText),
        m_invalidText(invalidText),
        m_detailsWidget(detailsWidget)
    {
        QTC_CHECK(m_detailsWidget);
        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        for (auto itr = validationPoints.cbegin(); itr != validationPoints.cend(); ++itr) {
            RowData data;
            data.m_infoLabel = new Utils::InfoLabel(itr.value());
            layout->addWidget(data.m_infoLabel);
            m_validationData[itr.key()] = data;
            setPointValid(itr.key(), true);
        }
    }

    void setPointValid(int key, bool valid)
    {
        if (!m_validationData.contains(key))
            return;
        RowData& data = m_validationData[key];
        data.m_valid = valid;
        data.m_infoLabel->setType(valid ? Utils::InfoLabel::Ok : Utils::InfoLabel::NotOk);
        updateUi();
    }

    bool rowsOk(QList<int> keys) const
    {
        for (auto key : keys) {
            if (!m_validationData[key].m_valid)
                return false;
        }
        return true;
    }

    bool allRowsOk() const { return rowsOk(m_validationData.keys()); }
    void setInfoText(const QString &text) {
        m_infoText = text;
        updateUi();
    }

private:
    void updateUi() {
        bool ok = allRowsOk();
        m_detailsWidget->setIcon(ok ? Utils::Icons::OK.icon() :
                                      Utils::Icons::CRITICAL.icon());
        m_detailsWidget->setSummaryText(ok ? QString("%1 %2").arg(m_validText).arg(m_infoText)
                                           : m_invalidText);
    }
    QString m_validText;
    QString m_invalidText;
    QString m_infoText;
    Utils::DetailsWidget *m_detailsWidget = nullptr;
    QMap<int, RowData> m_validationData;
};

void AvdModel::setAvdList(const AndroidDeviceInfoList &list)
{
    beginResetModel();
    m_list = list;
    endResetModel();
}

QModelIndex AvdModel::indexForAvdName(const QString &avdName) const
{
    for (int i = 0; i < m_list.size(); ++i) {
        if (m_list.at(i).serialNumber == avdName)
            return index(i, 0);
    }
    return QModelIndex();
}

QString AvdModel::avdName(const QModelIndex &index) const
{
    return m_list.at(index.row()).avdname;
}

QVariant AvdModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return QVariant();

    const AndroidDeviceInfo currentRow = m_list.at(index.row());
    switch (index.column()) {
        case 0:
            return currentRow.avdname;
        case 1:
            return currentRow.sdk;
        case 2: {
            QStringList cpuAbis = currentRow.cpuAbi;
            return cpuAbis.isEmpty() ? QVariant() : QVariant(cpuAbis.first());
        }
        case 3:
            return currentRow.avdDevice.isEmpty() ? QVariant("Custom")
                                                  : currentRow.avdDevice;
        case 4:
            return currentRow.avdTarget;
        case 5:
            return currentRow.avdSdcardSize;
    }
    return QVariant();
}

QVariant AvdModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                //: AVD - Android Virtual Device
                return tr("AVD Name");
            case 1:
                return tr("API");
            case 2:
                return tr("CPU/ABI");
            case 3:
                return tr("Device type");
            case 4:
                return tr("Target");
            case 5:
                return tr("SD-card size");
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role );
}

int AvdModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_list.size();
}

int AvdModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 6;
}

Utils::FilePath AndroidSettingsWidget::getDefaultSdkPath()
{
    QString sdkFromEnvVar = QString::fromLocal8Bit(getenv("ANDROID_SDK_ROOT"));
    if (!sdkFromEnvVar.isEmpty())
        return Utils::FilePath::fromString(sdkFromEnvVar);

    // Set default path of SDK as used by Android Studio
    if (Utils::HostOsInfo::isMacHost()) {
        return Utils::FilePath::fromString(
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
            + "/../Android/sdk");
    }

    if (Utils::HostOsInfo::isWindowsHost()) {
        return Utils::FilePath::fromString(
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Android/sdk");
    }

    return Utils::FilePath::fromString(
                QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/Android/Sdk");
}

void AndroidSettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (!m_isInitialReloadDone) {
        // Reloading SDK packages (force) is still synchronous. Use zero timer
        // to let settings dialog open first.
        QTimer::singleShot(0, std::bind(&AndroidSdkManager::reloadPackages,
                                        m_sdkManager.get(), false));
        validateOpenSsl();
        m_isInitialReloadDone = true;
    }
}

void AndroidSettingsWidget::updateNdkList()
{
    m_ui->ndkListWidget->clear();
    for (const Ndk *ndk : m_sdkManager->installedNdkPackages()) {
        m_ui->ndkListWidget->addItem(new QListWidgetItem(Utils::Icons::LOCKED.icon(),
                                                         ndk->installedLocation().toString()));
    }

    for (const QString &ndk : m_androidConfig.getCustomNdkList()) {
        if (m_androidConfig.isValidNdk(ndk)) {
            m_ui->ndkListWidget->addItem(
                new QListWidgetItem(Utils::Icons::UNLOCKED.icon(), ndk));
        } else {
            m_androidConfig.removeCustomNdk(ndk);
        }
    }

    m_ui->ndkListWidget->setCurrentRow(0);
}

void AndroidSettingsWidget::addCustomNdkItem()
{
    const QString homePath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    const QString ndkPath = QFileDialog::getExistingDirectory(this, tr("Select an NDK"), homePath);

    if (m_androidConfig.isValidNdk(ndkPath)) {
        m_androidConfig.addCustomNdk(ndkPath);
        if (m_ui->ndkListWidget->findItems(ndkPath, Qt::MatchExactly).size() == 0) {
            m_ui->ndkListWidget->addItem(
                new QListWidgetItem(Utils::Icons::UNLOCKED.icon(), ndkPath));
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
    : m_ui(new Ui_AndroidSettingsWidget),
      m_androidConfig(AndroidConfigurations::currentConfig()),
      m_avdManager(new AndroidAvdManager(m_androidConfig)),
      m_sdkManager(new AndroidSdkManager(m_androidConfig)),
      m_sdkDownloader(new AndroidSdkDownloader())
{
    m_ui->setupUi(this);
    m_sdkManagerWidget = new AndroidSdkManagerWidget(m_androidConfig, m_sdkManager.get(),
                                                     m_ui->sdkManagerTab);
    auto sdkMangerLayout = new QVBoxLayout(m_ui->sdkManagerTab);
    sdkMangerLayout->setContentsMargins(0, 0, 0, 0);
    sdkMangerLayout->addWidget(m_sdkManagerWidget);
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::updatingSdk, [this]() {
        m_ui->SDKLocationPathChooser->setEnabled(false);
        // Disable the tab bar to restrict the user moving away from sdk manager tab untill
        // operations finish.
        m_ui->managerTabWidget->tabBar()->setEnabled(false);
    });
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::updatingSdkFinished, [this]() {
        m_ui->SDKLocationPathChooser->setEnabled(true);
        m_ui->managerTabWidget->tabBar()->setEnabled(true);
    });
    connect(m_sdkManagerWidget, &AndroidSdkManagerWidget::licenseWorkflowStarted, [this]() {
       m_ui->scrollArea->ensureWidgetVisible(m_ui->managerTabWidget);
    });

    QMap<int, QString> javaValidationPoints;
    javaValidationPoints[JavaPathExistsRow] = tr("JDK path exists.");
    javaValidationPoints[JavaJdkValidRow] = tr("JDK path is a valid JDK root folder.");
    auto javaSummary = new SummaryWidget(javaValidationPoints, tr("Java Settings are OK."),
                                         tr("Java settings have errors."), m_ui->javaDetailsWidget);
    m_ui->javaDetailsWidget->setWidget(javaSummary);

    QMap<int, QString> androidValidationPoints;
    androidValidationPoints[SdkPathExistsRow] = tr("Android SDK path exists.");
    androidValidationPoints[SdkPathWritableRow] = tr("Android SDK path writable.");
    androidValidationPoints[SdkToolsInstalledRow] = tr("SDK tools installed.");
    androidValidationPoints[PlatformToolsInstalledRow] = tr("Platform tools installed.");
    androidValidationPoints[SdkManagerSuccessfulRow] = tr(
        "SDK manager runs (requires exactly Java 1.8).");
    androidValidationPoints[AllEssentialsInstalledRow] = tr(
        "All essential packages installed for all installed Qt versions.");
    androidValidationPoints[BuildToolsInstalledRow] = tr("Build tools installed.");
    androidValidationPoints[PlatformSdkInstalledRow] = tr("Platform SDK installed.");
    androidValidationPoints[NdkPathExistsRow] = tr("Default Android NDK path exists.");
    androidValidationPoints[NdkDirStructureRow] = tr("Default Android NDK directory structure is correct.");
    androidValidationPoints[NdkinstallDirOkRow] = tr("Default Android NDK installed into a path without "
                                                     "spaces.");
    auto androidSummary = new SummaryWidget(androidValidationPoints, tr("Android settings are OK."),
                                            tr("Android settings have errors."),
                                            m_ui->androidDetailsWidget);
    m_ui->androidDetailsWidget->setWidget(androidSummary);

    QMap<int, QString> openSslValidationPoints;
    openSslValidationPoints[OpenSslPathExistsRow] = tr("OpenSSL path exists.");
    openSslValidationPoints[OpenSslPriPathExists] = tr(
        "QMake include project (openssl.pri) exists.");
    openSslValidationPoints[OpenSslCmakeListsPathExists] = tr(
        "CMake include project (CMakeLists.txt) exists.");
    auto openSslSummary = new SummaryWidget(openSslValidationPoints,
                                            tr("OpenSSL Settings are OK."),
                                            tr("OpenSSL settings have errors."),
                                            m_ui->openSslDetailsWidget);
    m_ui->openSslDetailsWidget->setWidget(openSslSummary);

    connect(m_ui->OpenJDKLocationPathChooser, &Utils::PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::validateJdk);
    Utils::FilePath currentJdkPath = m_androidConfig.openJDKLocation();
    if (currentJdkPath.isEmpty())
        currentJdkPath = findJdkInCommonPaths();
    m_ui->OpenJDKLocationPathChooser->setFileName(currentJdkPath);
    m_ui->OpenJDKLocationPathChooser->setPromptDialogTitle(tr("Select JDK Path"));

    Utils::FilePath currentSDKPath = m_androidConfig.sdkLocation();
    if (currentSDKPath.isEmpty())
        currentSDKPath = getDefaultSdkPath();

    m_ui->SDKLocationPathChooser->setFileName(currentSDKPath);
    m_ui->SDKLocationPathChooser->setPromptDialogTitle(tr("Select Android SDK folder"));

    m_ui->openSslPathChooser->setPromptDialogTitle(tr("Select OpenSSL Include Project File"));
    Utils::FilePath currentOpenSslPath = m_androidConfig.openSslLocation();
    if (currentOpenSslPath.isEmpty())
        currentOpenSslPath = currentSDKPath.pathAppended("android_openssl");
    m_ui->openSslPathChooser->setFileName(currentOpenSslPath);

    m_ui->DataPartitionSizeSpinBox->setValue(m_androidConfig.partitionSize());
    m_ui->CreateKitCheckBox->setChecked(m_androidConfig.automaticKitCreation());
    m_ui->AVDTableView->setModel(&m_AVDModel);
    m_ui->AVDTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->AVDTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    m_ui->downloadOpenJDKToolButton->setVisible(!Utils::HostOsInfo::isLinuxHost());

    const QIcon downloadIcon = Utils::Icons::DOWNLOAD.icon();
    m_ui->downloadSDKToolButton->setIcon(downloadIcon);
    m_ui->downloadNDKToolButton->setIcon(downloadIcon);
    m_ui->downloadOpenJDKToolButton->setIcon(downloadIcon);
    m_ui->downloadOpenSSLPrebuiltLibs->setIcon(downloadIcon);
    m_ui->sdkToolsAutoDownloadButton->setIcon(Utils::Icons::RELOAD.icon());
    m_ui->sdkToolsAutoDownloadButton->setToolTip(tr(
            "Automatically download Android SDK Tools to selected location.\n\n"
            "If the selected path contains no valid SDK Tools, the SDK Tools package "
            "is downloaded from %1, and extracted to the selected path.\n"
            "After the SDK Tools are properly set up, you are prompted to install "
            "any essential packages required for Qt to build for Android.\n")
                                                 .arg(m_androidConfig.sdkToolsUrl().toString()));

    connect(m_ui->SDKLocationPathChooser, &Utils::PathChooser::rawPathChanged,
            this, &AndroidSettingsWidget::onSdkPathChanged);

    connect(m_ui->ndkListWidget, &QListWidget::currentTextChanged, [this](const QString &ndk) {
        validateNdk();
        m_ui->removeCustomNdkButton->setEnabled(m_androidConfig.getCustomNdkList().contains(ndk));
    });
    connect(m_ui->addCustomNdkButton, &QPushButton::clicked, this,
            &AndroidSettingsWidget::addCustomNdkItem);
    connect(m_ui->removeCustomNdkButton, &QPushButton::clicked, this, [this]() {
        m_androidConfig.removeCustomNdk(m_ui->ndkListWidget->currentItem()->text());
        m_ui->ndkListWidget->takeItem(m_ui->ndkListWidget->currentRow());
    });

    connect(m_ui->openSslPathChooser, &Utils::PathChooser::rawPathChanged, this,
            &AndroidSettingsWidget::validateOpenSsl);
    connect(&m_virtualDevicesWatcher, &QFutureWatcherBase::finished,
            this, &AndroidSettingsWidget::updateAvds);
    connect(m_ui->AVDRefreshPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::startUpdateAvd);
    connect(&m_futureWatcher, &QFutureWatcherBase::finished, this, &AndroidSettingsWidget::avdAdded);
    connect(m_ui->AVDAddPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::addAVD);
    connect(m_ui->AVDRemovePushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::removeAVD);
    connect(m_ui->AVDStartPushButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::startAVD);
    connect(m_ui->AVDTableView, &QAbstractItemView::activated,
            this, &AndroidSettingsWidget::avdActivated);
    connect(m_ui->AVDTableView, &QAbstractItemView::clicked,
            this, &AndroidSettingsWidget::avdActivated);
    connect(m_ui->DataPartitionSizeSpinBox, &QAbstractSpinBox::editingFinished,
            this, &AndroidSettingsWidget::dataPartitionSizeEditingFinished);
    connect(m_ui->nativeAvdManagerButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::manageAVD);
    connect(m_ui->CreateKitCheckBox, &QAbstractButton::toggled,
            this, &AndroidSettingsWidget::createKitToggled);
    connect(m_ui->downloadNDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openNDKDownloadUrl);
    connect(m_ui->downloadSDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openSDKDownloadUrl);
    connect(m_ui->downloadOpenSSLPrebuiltLibs, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadOpenSslRepo);
    connect(m_ui->downloadOpenJDKToolButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::openOpenJDKDownloadUrl);
    // Validate SDK again after any change in SDK packages.
    connect(m_sdkManager.get(), &AndroidSdkManager::packageReloadFinished,
            this, &AndroidSettingsWidget::validateSdk);
    connect(m_ui->sdkToolsAutoDownloadButton, &QAbstractButton::clicked,
            this, &AndroidSettingsWidget::downloadSdk);
    connect(m_sdkDownloader.get(), &AndroidSdkDownloader::sdkDownloaderError, this, [this](const QString &error) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(), error);
    });
    connect(m_sdkDownloader.get(), &AndroidSdkDownloader::sdkExtracted, this, [this]() {
        m_sdkManager->reloadPackages(true);
        updateUI();
        apply();

        QMetaObject::Connection *const openSslOneShot = new QMetaObject::Connection;
        *openSslOneShot = connect(m_sdkManager.get(), &AndroidSdkManager::packageReloadFinished,
                                  this, [this, openSslOneShot]() {
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
    delete m_ui;
    m_futureWatcher.waitForFinished();
}

void AndroidSettingsWidget::disableAvdControls()
{
    m_ui->AVDAddPushButton->setEnabled(false);
    m_ui->AVDTableView->setEnabled(false);
    m_ui->AVDRemovePushButton->setEnabled(false);
    m_ui->AVDStartPushButton->setEnabled(false);
}

void AndroidSettingsWidget::enableAvdControls()
{
    m_ui->AVDTableView->setEnabled(true);
    m_ui->AVDAddPushButton->setEnabled(true);
    avdActivated(m_ui->AVDTableView->currentIndex());
}

void AndroidSettingsWidget::startUpdateAvd()
{
    disableAvdControls();
    m_virtualDevicesWatcher.setFuture(m_avdManager->avdList());
}

void AndroidSettingsWidget::updateAvds()
{
    m_AVDModel.setAvdList(m_virtualDevicesWatcher.result());
    if (!m_lastAddedAvd.isEmpty()) {
        m_ui->AVDTableView->setCurrentIndex(m_AVDModel.indexForAvdName(m_lastAddedAvd));
        m_lastAddedAvd.clear();
    }
    enableAvdControls();
}

void AndroidSettingsWidget::validateJdk()
{
    auto javaPath = Utils::FilePath::fromUserInput(m_ui->OpenJDKLocationPathChooser->rawPath());
    m_androidConfig.setOpenJDKLocation(javaPath);
    bool jdkPathExists = m_androidConfig.openJDKLocation().exists();
    auto summaryWidget = static_cast<SummaryWidget *>(m_ui->javaDetailsWidget->widget());
    summaryWidget->setPointValid(JavaPathExistsRow, jdkPathExists);

    const Utils::FilePath bin = m_androidConfig.openJDKLocation().pathAppended("bin/javac" QTC_HOST_EXE_SUFFIX);
    summaryWidget->setPointValid(JavaJdkValidRow, jdkPathExists && bin.exists());
    updateUI();
}

void AndroidSettingsWidget::validateOpenSsl()
{
    auto openSslPath = Utils::FilePath::fromUserInput(m_ui->openSslPathChooser->rawPath());
    m_androidConfig.setOpenSslLocation(openSslPath);

    auto summaryWidget = static_cast<SummaryWidget *>(m_ui->openSslDetailsWidget->widget());
    summaryWidget->setPointValid(OpenSslPathExistsRow, m_androidConfig.openSslLocation().exists());

    const bool priFileExists = m_androidConfig.openSslLocation().pathAppended("openssl.pri").exists();
    summaryWidget->setPointValid(OpenSslPriPathExists, priFileExists);
    const bool cmakeListsExists
        = m_androidConfig.openSslLocation().pathAppended("CMakeLists.txt").exists();
    summaryWidget->setPointValid(OpenSslCmakeListsPathExists, cmakeListsExists);
    updateUI();
}

Utils::FilePath AndroidSettingsWidget::findJdkInCommonPaths()
{
    QString jdkFromEnvVar = QString::fromLocal8Bit(getenv("JAVA_HOME"));
    if (!jdkFromEnvVar.isEmpty())
        return Utils::FilePath::fromUserInput(jdkFromEnvVar);

    if (Utils::HostOsInfo::isWindowsHost()) {
        QString jdkRegisteryPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\JDK\\";
        QSettings jdkSettings(jdkRegisteryPath, QSettings::NativeFormat);

        QStringList jdkVersions = jdkSettings.childGroups();
        Utils::FilePath jdkHome;

        for (const QString &version : jdkVersions) {
            jdkSettings.beginGroup(version);
            jdkHome = Utils::FilePath::fromUserInput(jdkSettings.value("JavaHome").toString());
            jdkSettings.endGroup();
            if (version.startsWith("1.8"))
                return jdkHome;
        }

        return jdkHome;
    }

    QProcess findJdkPathProc;

    QString cmd;
    QStringList args;

    if (Utils::HostOsInfo::isMacHost()) {
        cmd = "sh";
        args << "-c" << "/usr/libexec/java_home";
    } else {
        cmd = "sh";
        args << "-c" << "readlink -f $(which java)";
    }

    findJdkPathProc.start(cmd, args);
    findJdkPathProc.waitForFinished();
    QByteArray jdkPath = findJdkPathProc.readAllStandardOutput().trimmed();

    if (Utils::HostOsInfo::isMacHost())
        return Utils::FilePath::fromUtf8(jdkPath);
    else
        return Utils::FilePath::fromUtf8(jdkPath.replace("jre/bin/java", ""));
}

void AndroidSettingsWidget::validateNdk()
{
    const QListWidgetItem *currentItem = m_ui->ndkListWidget->currentItem();
    Utils::FilePath ndkPath = Utils::FilePath::fromString(currentItem ? currentItem->text() : "");

    auto summaryWidget = static_cast<SummaryWidget *>(m_ui->androidDetailsWidget->widget());
    summaryWidget->setPointValid(NdkPathExistsRow, ndkPath.exists());

    const Utils::FilePath ndkPlatformsDir = ndkPath.pathAppended("platforms");
    const Utils::FilePath ndkToolChainsDir = ndkPath.pathAppended("toolchains");
    const Utils::FilePath ndkSourcesDir = ndkPath.pathAppended("sources/cxx-stl");
    summaryWidget->setPointValid(NdkDirStructureRow,
                                 ndkPlatformsDir.exists()
                                 && ndkToolChainsDir.exists()
                                 && ndkSourcesDir.exists());
    summaryWidget->setPointValid(NdkinstallDirOkRow,
                                 ndkPlatformsDir.exists() &&
                                 !ndkPlatformsDir.toString().contains(' '));
    updateUI();
}

void AndroidSettingsWidget::onSdkPathChanged()
{
    auto sdkPath = Utils::FilePath::fromUserInput(m_ui->SDKLocationPathChooser->rawPath());
    m_androidConfig.setSdkLocation(sdkPath);
    Utils::FilePath currentOpenSslPath = m_androidConfig.openSslLocation();
    if (currentOpenSslPath.isEmpty() || !currentOpenSslPath.exists())
        currentOpenSslPath = sdkPath.pathAppended("android_openssl");
    m_ui->openSslPathChooser->setFileName(currentOpenSslPath);
    // Package reload will trigger validateSdk.
    m_sdkManager->reloadPackages();
}

void AndroidSettingsWidget::validateSdk()
{
    auto sdkPath = Utils::FilePath::fromUserInput(m_ui->SDKLocationPathChooser->rawPath());
    m_androidConfig.setSdkLocation(sdkPath);

    auto summaryWidget = static_cast<SummaryWidget *>(m_ui->androidDetailsWidget->widget());
    summaryWidget->setPointValid(SdkPathExistsRow, m_androidConfig.sdkLocation().exists());
    summaryWidget->setPointValid(SdkPathWritableRow, m_androidConfig.sdkLocation().isWritablePath());
    summaryWidget->setPointValid(SdkToolsInstalledRow,
                                 !m_androidConfig.sdkToolsVersion().isNull());
    summaryWidget->setPointValid(PlatformToolsInstalledRow,
                                 m_androidConfig.adbToolPath().exists());
    summaryWidget->setPointValid(BuildToolsInstalledRow,
                                 !m_androidConfig.buildToolsVersion().isNull());
    summaryWidget->setPointValid(SdkManagerSuccessfulRow, m_sdkManager->packageListingSuccessful());
    // installedSdkPlatforms should not trigger a package reload as validate SDK is only called
    // after AndroidSdkManager::packageReloadFinished.
    summaryWidget->setPointValid(PlatformSdkInstalledRow,
                                 !m_sdkManager->installedSdkPlatforms().isEmpty());

    summaryWidget->setPointValid(AllEssentialsInstalledRow, allEssentialsInstalled());
    updateUI();
    bool sdkToolsOk = summaryWidget->rowsOk(
        {SdkPathExistsRow, SdkPathWritableRow, SdkToolsInstalledRow, SdkManagerSuccessfulRow});
    bool componentsOk = summaryWidget->rowsOk({PlatformToolsInstalledRow,
                                                      BuildToolsInstalledRow,
                                                      PlatformSdkInstalledRow,
                                                      AllEssentialsInstalledRow});
    m_androidConfig.setSdkFullyConfigured(sdkToolsOk && componentsOk);
    if (sdkToolsOk && !componentsOk && !m_androidConfig.useNativeUiTools()) {
        // Ask user to install essential SDK components. Works only for sdk tools version >= 26.0.0
        QString message = tr("Android SDK installation is missing necessary packages. Do you "
                             "want to install the missing packages?");
        auto userInput = QMessageBox::information(this, tr("Missing Android SDK packages"),
                                                  message, QMessageBox::Yes | QMessageBox::No);
        if (userInput == QMessageBox::Yes) {
            m_ui->managerTabWidget->setCurrentWidget(m_ui->sdkManagerTab);
            m_sdkManagerWidget->installEssentials();
        }
    }

    startUpdateAvd();
    updateNdkList();
    validateNdk();
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
    QDesktopServices::openUrl(QUrl::fromUserInput("https://www.oracle.com/java/technologies/javase-jdk8-downloads.html"));
}

void AndroidSettingsWidget::downloadOpenSslRepo(const bool silent)
{
    const Utils::FilePath openSslPath = m_ui->openSslPathChooser->fileName();
    const QString openSslCloneTitle(tr("OpenSSL Cloning"));

    auto openSslSummaryWidget = static_cast<SummaryWidget *>(m_ui->openSslDetailsWidget->widget());
    if (openSslSummaryWidget->allRowsOk()) {
        if (!silent) {
            QMessageBox::information(this, openSslCloneTitle,
                tr("OpenSSL prebuilt libraries repository is already configured."));
        }
        return;
    }

    const QString openSslRepo("https://github.com/KDAB/android_openssl.git");
    Utils::QtcProcess *gitCloner = new Utils::QtcProcess(this);
    Utils::CommandLine gitCloneCommand("git",
                            {"clone", "--depth=1", openSslRepo, openSslPath.toString()});
    gitCloner->setCommand(gitCloneCommand);

    qCDebug(androidsettingswidget) << "Cloning OpenSSL repo: " <<
                                      gitCloneCommand.toUserOutput();

    QDir openSslDir(openSslPath.toString());
    if (openSslDir.exists()) {
        auto userInput = QMessageBox::information(this, openSslCloneTitle,
            tr("The selected download path (%1) for OpenSSL already exists. "
               "Remove and overwrite its content?")
                .arg(QDir::toNativeSeparators(openSslPath.toString())),
            QMessageBox::Yes | QMessageBox::No);
        if (userInput == QMessageBox::Yes)
            openSslDir.removeRecursively();
        else
            return;
    }

    QProgressDialog *openSslProgressDialog
        = new QProgressDialog(tr("Cloning OpenSSL prebuilt libraries..."),
                              tr("Cancel"), 0, 0);
    openSslProgressDialog->setWindowModality(Qt::WindowModal);
    openSslProgressDialog->setWindowTitle(openSslCloneTitle);
    openSslProgressDialog->setFixedSize(openSslProgressDialog->sizeHint());

    connect(openSslProgressDialog, &QProgressDialog::canceled, this, [gitCloner]() {
        gitCloner->kill();
    });

    gitCloner->start();
    openSslProgressDialog->show();

    connect(gitCloner, QOverload<int, Utils::QtcProcess::ExitStatus>::of(&Utils::QtcProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                openSslProgressDialog->close();
                validateOpenSsl();

                if (!openSslProgressDialog->wasCanceled() ||
                    (exitStatus == Utils::QtcProcess::NormalExit && exitCode != 0)) {
                    QMessageBox::information(this, openSslCloneTitle,
                                             tr("OpenSSL prebuilt libraries cloning failed. "
                                                "Opening OpenSSL URL for manual download."));
                    QDesktopServices::openUrl(QUrl::fromUserInput(openSslRepo));
                }
            });
}

void AndroidSettingsWidget::addAVD()
{
    disableAvdControls();
    CreateAvdInfo info = AvdDialog::gatherCreateAVDInfo(this, m_sdkManager.get(), m_androidConfig);

    if (!info.isValid()) {
        enableAvdControls();
        return;
    }

    m_futureWatcher.setFuture(m_avdManager->createAvd(info));
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
    QString avdName = m_AVDModel.avdName(m_ui->AVDTableView->currentIndex());
    if (QMessageBox::question(this, tr("Remove Android Virtual Device"),
                              tr("Remove device \"%1\"? This cannot be undone.").arg(avdName),
                              QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::No) {
        enableAvdControls();
        return;
    }

    m_avdManager->removeAvd(avdName);
    startUpdateAvd();
}

void AndroidSettingsWidget::startAVD()
{
    m_avdManager->startAvdAsync(m_AVDModel.avdName(m_ui->AVDTableView->currentIndex()));
}

void AndroidSettingsWidget::avdActivated(const QModelIndex &index)
{
    m_ui->AVDRemovePushButton->setEnabled(index.isValid());
    m_ui->AVDStartPushButton->setEnabled(index.isValid());
}

void AndroidSettingsWidget::dataPartitionSizeEditingFinished()
{
    m_androidConfig.setPartitionSize(m_ui->DataPartitionSizeSpinBox->value());
}

void AndroidSettingsWidget::createKitToggled()
{
    m_androidConfig.setAutomaticKitCreation(m_ui->CreateKitCheckBox->isChecked());
}

void AndroidSettingsWidget::updateUI()
{
    auto javaSummaryWidget = static_cast<SummaryWidget *>(m_ui->javaDetailsWidget->widget());
    auto androidSummaryWidget = static_cast<SummaryWidget *>(m_ui->androidDetailsWidget->widget());
    auto openSslSummaryWidget = static_cast<SummaryWidget *>(m_ui->openSslDetailsWidget->widget());
    const bool javaSetupOk = javaSummaryWidget->allRowsOk();
    const bool sdkToolsOk = androidSummaryWidget->rowsOk({SdkPathExistsRow, SdkPathWritableRow, SdkToolsInstalledRow});
    const bool androidSetupOk = androidSummaryWidget->allRowsOk();
    const bool openSslOk = openSslSummaryWidget->allRowsOk();

    m_ui->avdManagerTab->setEnabled(javaSetupOk && androidSetupOk);
    m_ui->sdkManagerTab->setEnabled(sdkToolsOk);
    m_sdkManagerWidget->setSdkManagerControlsEnabled(!m_androidConfig.useNativeUiTools());

    const QListWidgetItem *currentItem = m_ui->ndkListWidget->currentItem();
    Utils::FilePath currentNdk = Utils::FilePath::fromString(currentItem ? currentItem->text() : "");
    auto infoText = tr("(SDK Version: %1, NDK Bundle Version: %2)")
                        .arg(m_androidConfig.sdkToolsVersion().toString())
                        .arg(currentNdk.isEmpty() ? "" : m_androidConfig.ndkVersion(currentNdk).toString());
    androidSummaryWidget->setInfoText(androidSetupOk ? infoText : "");

    m_ui->javaDetailsWidget->setState(javaSetupOk ? Utils::DetailsWidget::Collapsed :
                                                    Utils::DetailsWidget::Expanded);
    m_ui->androidDetailsWidget->setState(androidSetupOk ? Utils::DetailsWidget::Collapsed :
                                                          Utils::DetailsWidget::Expanded);
    m_ui->openSslDetailsWidget->setState(openSslOk ? Utils::DetailsWidget::Collapsed :
                                                        Utils::DetailsWidget::Expanded);
}

void AndroidSettingsWidget::manageAVD()
{
    if (m_androidConfig.useNativeUiTools()) {
        m_avdManager->launchAvdManagerUiTool();
    } else {
        QMessageBox::warning(this, tr("AVD Manager Not Available"),
                             tr("AVD manager UI tool is not available in the installed SDK tools "
                                "(version %1). Use the command line tool \"avdmanager\" for "
                                "advanced AVD management.")
                             .arg(m_androidConfig.sdkToolsVersion().toString()));
    }
}

void AndroidSettingsWidget::downloadSdk()
{
    if (sdkToolsOk()) {
        QMessageBox::warning(this, AndroidSdkDownloader::dialogTitle(),
                             tr("The selected path already has a valid SDK Tools package."));
        validateSdk();
        return;
    }

    QString message(tr("Download and install Android SDK Tools to: %1?")
                        .arg(QDir::toNativeSeparators(m_ui->SDKLocationPathChooser->rawPath())));
    auto userInput = QMessageBox::information(this, AndroidSdkDownloader::dialogTitle(),
                                              message, QMessageBox::Yes | QMessageBox::No);
    if (userInput == QMessageBox::Yes) {
        auto javaSummaryWidget = static_cast<SummaryWidget *>(m_ui->javaDetailsWidget->widget());
        if (javaSummaryWidget->allRowsOk()) {
            auto javaPath = Utils::FilePath::fromUserInput(m_ui->OpenJDKLocationPathChooser->rawPath());
            m_sdkDownloader->downloadAndExtractSdk(javaPath.toString(),
                                                 m_ui->SDKLocationPathChooser->path());
        }
    }
}

bool AndroidSettingsWidget::allEssentialsInstalled()
{
    QStringList essentialPkgs(m_androidConfig.allEssentials());
    for (const AndroidSdkPackage *pkg : m_sdkManager->installedSdkPackages()) {
        if (essentialPkgs.contains(pkg->sdkStylePath()))
            essentialPkgs.removeOne(pkg->sdkStylePath());
        if (essentialPkgs.isEmpty())
            break;
    }
    return essentialPkgs.isEmpty() ? true : false;
}

bool AndroidSettingsWidget::sdkToolsOk() const
{
    bool exists = m_androidConfig.sdkLocation().exists();
    bool writable = m_androidConfig.sdkLocation().isWritablePath();
    bool sdkToolsExist = !m_androidConfig.sdkToolsVersion().isNull();
    return exists && writable && sdkToolsExist;
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
        pal.setColor(QPalette::Window, Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal));
        widget->setPalette(pal);
        return widget;
    });
}

} // namespace Internal
} // namespace Android
