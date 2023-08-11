// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"
#include "androidmanifestdocument.h"
#include "androidmanifesteditor.h"
#include "androidmanifesteditoriconcontainerwidget.h"
#include "androidmanifesteditorwidget.h"
#include "androidtr.h"
#include "splashscreencontainerwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>

#include <qtsupport/qtkitaspect.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectwindow.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/infobar.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <limits>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

const char infoBarId[] = "Android.AndroidManifestEditor.InfoBar";

static bool checkPackageName(const QString &packageName)
{
    const QLatin1String packageNameRegExp("^([a-z]{1}[a-z0-9_]+(\\.[a-zA-Z]{1}[a-zA-Z0-9_]*)*)$");
    return QRegularExpression(packageNameRegExp).match(packageName).hasMatch();
}

static Target *androidTarget(const FilePath &fileName)
{
    for (Project *project : ProjectManager::projects()) {
        if (Target *target = project->activeTarget()) {
            Kit *kit = target->kit();
            if (DeviceTypeKitAspect::deviceTypeId(kit) == Android::Constants::ANDROID_DEVICE_TYPE
                    && fileName.isChildOf(project->projectDirectory()))
                return target;
        }
    }
    return nullptr;
}

class PermissionsModel: public QAbstractListModel
{
public:
    PermissionsModel(QObject *parent = nullptr);
    void setPermissions(const QStringList &permissions);
    const QStringList &permissions();
    QModelIndex addPermission(const QString &permission);
    void removePermission(int index);
    QVariant data(const QModelIndex &index, int role) const override;

protected:
    int rowCount(const QModelIndex &parent) const override;

private:
    QStringList m_permissions;
};

class AndroidManifestTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    explicit AndroidManifestTextEditorWidget(AndroidManifestEditorWidget *parent);

private:
    Core::IContext *m_context;
};

AndroidManifestEditorWidget::AndroidManifestEditorWidget()
{
    m_textEditorWidget = new AndroidManifestTextEditorWidget(this);

    initializePage();

    m_timerParseCheck.setInterval(800);
    m_timerParseCheck.setSingleShot(true);

    m_editor = new AndroidManifestEditor(this);

    connect(&m_timerParseCheck, &QTimer::timeout,
            this, &AndroidManifestEditorWidget::delayedParseCheck);

    connect(m_textEditorWidget->document(), &QTextDocument::contentsChanged,
            this, &AndroidManifestEditorWidget::startParseCheck);
    connect(m_textEditorWidget->textDocument(), &TextEditor::TextDocument::reloadFinished,
            this, [this](bool success) { if (success) updateAfterFileLoad(); });
    connect(m_textEditorWidget->textDocument(), &TextEditor::TextDocument::openFinishedSuccessfully,
            this, &AndroidManifestEditorWidget::updateAfterFileLoad);
}

QGroupBox *AndroidManifestEditorWidget::createPermissionsGroupBox(QWidget *parent)
{
    auto permissionsGroupBox = new QGroupBox(parent);
    permissionsGroupBox->setTitle(::Android::Tr::tr("Permissions"));
    auto layout = new QGridLayout(permissionsGroupBox);

    m_defaultPermissonsCheckBox = new QCheckBox(this);
    m_defaultPermissonsCheckBox->setText(::Android::Tr::tr("Include default permissions for Qt modules."));
    layout->addWidget(m_defaultPermissonsCheckBox, 0, 0);

    m_defaultFeaturesCheckBox = new QCheckBox(this);
    m_defaultFeaturesCheckBox->setText(::Android::Tr::tr("Include default features for Qt modules."));
    layout->addWidget(m_defaultFeaturesCheckBox, 1, 0);

    m_permissionsComboBox = new QComboBox(permissionsGroupBox);
    m_permissionsComboBox->insertItems(0, {
        "android.permission.ACCESS_CHECKIN_PROPERTIES",
        "android.permission.ACCESS_COARSE_LOCATION",
        "android.permission.ACCESS_FINE_LOCATION",
        "android.permission.ACCESS_LOCATION_EXTRA_COMMANDS",
        "android.permission.ACCESS_MOCK_LOCATION",
        "android.permission.ACCESS_NETWORK_STATE",
        "android.permission.ACCESS_SURFACE_FLINGER",
        "android.permission.ACCESS_WIFI_STATE",
        "android.permission.ACCOUNT_MANAGER",
        "com.android.voicemail.permission.ADD_VOICEMAIL",
        "android.permission.AUTHENTICATE_ACCOUNTS",
        "android.permission.BATTERY_STATS",
        "android.permission.BIND_ACCESSIBILITY_SERVICE",
        "android.permission.BIND_APPWIDGET",
        "android.permission.BIND_DEVICE_ADMIN",
        "android.permission.BIND_INPUT_METHOD",
        "android.permission.BIND_REMOTEVIEWS",
        "android.permission.BIND_TEXT_SERVICE",
        "android.permission.BIND_VPN_SERVICE",
        "android.permission.BIND_WALLPAPER",
        "android.permission.BLUETOOTH",
        "android.permission.BLUETOOTH_ADMIN",
        "android.permission.BRICK",
        "android.permission.BROADCAST_PACKAGE_REMOVED",
        "android.permission.BROADCAST_SMS",
        "android.permission.BROADCAST_STICKY",
        "android.permission.BROADCAST_WAP_PUSH",
        "android.permission.CALL_PHONE",
        "android.permission.CALL_PRIVILEGED",
        "android.permission.CAMERA",
        "android.permission.CHANGE_COMPONENT_ENABLED_STATE",
        "android.permission.CHANGE_CONFIGURATION",
        "android.permission.CHANGE_NETWORK_STATE",
        "android.permission.CHANGE_WIFI_MULTICAST_STATE",
        "android.permission.CHANGE_WIFI_STATE",
        "android.permission.CLEAR_APP_CACHE",
        "android.permission.CLEAR_APP_USER_DATA",
        "android.permission.CONTROL_LOCATION_UPDATES",
        "android.permission.DELETE_CACHE_FILES",
        "android.permission.DELETE_PACKAGES",
        "android.permission.DEVICE_POWER",
        "android.permission.DIAGNOSTIC",
        "android.permission.DISABLE_KEYGUARD",
        "android.permission.DUMP",
        "android.permission.EXPAND_STATUS_BAR",
        "android.permission.FACTORY_TEST",
        "android.permission.FLASHLIGHT",
        "android.permission.FORCE_BACK",
        "android.permission.GET_ACCOUNTS",
        "android.permission.GET_PACKAGE_SIZE",
        "android.permission.GET_TASKS",
        "android.permission.GLOBAL_SEARCH",
        "android.permission.HARDWARE_TEST",
        "android.permission.INJECT_EVENTS",
        "android.permission.INSTALL_LOCATION_PROVIDER",
        "android.permission.INSTALL_PACKAGES",
        "android.permission.INTERNAL_SYSTEM_WINDOW",
        "android.permission.INTERNET",
        "android.permission.KILL_BACKGROUND_PROCESSES",
        "android.permission.MANAGE_ACCOUNTS",
        "android.permission.MANAGE_APP_TOKENS",
        "android.permission.MASTER_CLEAR",
        "android.permission.MODIFY_AUDIO_SETTINGS",
        "android.permission.MODIFY_PHONE_STATE",
        "android.permission.MOUNT_FORMAT_FILESYSTEMS",
        "android.permission.MOUNT_UNMOUNT_FILESYSTEMS",
        "android.permission.NFC",
        "android.permission.PERSISTENT_ACTIVITY",
        "android.permission.PROCESS_OUTGOING_CALLS",
        "android.permission.READ_CALENDAR",
        "android.permission.READ_CALL_LOG",
        "android.permission.READ_CONTACTS",
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.READ_FRAME_BUFFER",
        "com.android.browser.permission.READ_HISTORY_BOOKMARKS",
        "android.permission.READ_INPUT_STATE",
        "android.permission.READ_LOGS",
        "android.permission.READ_PHONE_STATE",
        "android.permission.READ_PROFILE",
        "android.permission.READ_SMS",
        "android.permission.READ_SOCIAL_STREAM",
        "android.permission.READ_SYNC_SETTINGS",
        "android.permission.READ_SYNC_STATS",
        "android.permission.READ_USER_DICTIONARY",
        "android.permission.REBOOT",
        "android.permission.RECEIVE_BOOT_COMPLETED",
        "android.permission.RECEIVE_MMS",
        "android.permission.RECEIVE_SMS",
        "android.permission.RECEIVE_WAP_PUSH",
        "android.permission.RECORD_AUDIO",
        "android.permission.REORDER_TASKS",
        "android.permission.RESTART_PACKAGES",
        "android.permission.SEND_SMS",
        "android.permission.SET_ACTIVITY_WATCHER",
        "com.android.alarm.permission.SET_ALARM",
        "android.permission.SET_ALWAYS_FINISH",
        "android.permission.SET_ANIMATION_SCALE",
        "android.permission.SET_DEBUG_APP",
        "android.permission.SET_ORIENTATION",
        "android.permission.SET_POINTER_SPEED",
        "android.permission.SET_PREFERRED_APPLICATIONS",
        "android.permission.SET_PROCESS_LIMIT",
        "android.permission.SET_TIME",
        "android.permission.SET_TIME_ZONE",
        "android.permission.SET_WALLPAPER",
        "android.permission.SET_WALLPAPER_HINTS",
        "android.permission.SIGNAL_PERSISTENT_PROCESSES",
        "android.permission.STATUS_BAR",
        "android.permission.SUBSCRIBED_FEEDS_READ",
        "android.permission.SUBSCRIBED_FEEDS_WRITE",
        "android.permission.SYSTEM_ALERT_WINDOW",
        "android.permission.UPDATE_DEVICE_STATS",
        "android.permission.USE_CREDENTIALS",
        "android.permission.USE_SIP",
        "android.permission.VIBRATE",
        "android.permission.WAKE_LOCK",
        "android.permission.WRITE_APN_SETTINGS",
        "android.permission.WRITE_CALENDAR",
        "android.permission.WRITE_CALL_LOG",
        "android.permission.WRITE_CONTACTS",
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.WRITE_GSERVICES",
        "com.android.browser.permission.WRITE_HISTORY_BOOKMARKS",
        "android.permission.WRITE_PROFILE",
        "android.permission.WRITE_SECURE_SETTINGS",
        "android.permission.WRITE_SETTINGS",
        "android.permission.WRITE_SMS",
        "android.permission.WRITE_SOCIAL_STREAM",
        "android.permission.WRITE_SYNC_SETTINGS",
        "android.permission.WRITE_USER_DICTIONARY",
    });
    m_permissionsComboBox->setEditable(true);
    layout->addWidget(m_permissionsComboBox, 2, 0);

    m_addPermissionButton = new QPushButton(permissionsGroupBox);
    m_addPermissionButton->setText(::Android::Tr::tr("Add"));
    layout->addWidget(m_addPermissionButton, 2, 1);

    m_permissionsModel = new PermissionsModel(this);

    m_permissionsListView = new QListView(permissionsGroupBox);
    m_permissionsListView->setModel(m_permissionsModel);
    layout->addWidget(m_permissionsListView, 3, 0, 3, 1);

    m_removePermissionButton = new QPushButton(permissionsGroupBox);
    m_removePermissionButton->setText(::Android::Tr::tr("Remove"));
    layout->addWidget(m_removePermissionButton, 3, 1);

    permissionsGroupBox->setLayout(layout);

    connect(m_defaultPermissonsCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);
    connect(m_defaultFeaturesCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);

    connect(m_addPermissionButton, &QAbstractButton::clicked,
            this, &AndroidManifestEditorWidget::addPermission);
    connect(m_removePermissionButton, &QAbstractButton::clicked,
            this, &AndroidManifestEditorWidget::removePermission);
    connect(m_permissionsComboBox, &QComboBox::currentTextChanged,
            this, &AndroidManifestEditorWidget::updateAddRemovePermissionButtons);

    return permissionsGroupBox;
}

QGroupBox *AndroidManifestEditorWidget::createPackageFormLayout(QWidget *parent)
{
    auto packageGroupBox = new QGroupBox(parent);
    packageGroupBox->setTitle(::Android::Tr::tr("Package"));
    auto formLayout = new QFormLayout();

    m_packageNameLineEdit = new QLineEdit(packageGroupBox);
    m_packageNameLineEdit->setToolTip(::Android::Tr::tr(
        "<p align=\"justify\">Please choose a valid package name for your application (for "
        "example, \"org.example.myapplication\").</p><p align=\"justify\">Packages are usually "
        "defined using a hierarchical naming pattern, with levels in the hierarchy separated "
        "by periods (.) (pronounced \"dot\").</p><p align=\"justify\">In general, a package "
        "name begins with the top level domain name of the organization and then the "
        "organization's domain and then any subdomains listed in reverse order. The "
        "organization can then choose a specific name for their package. Package names should "
        "be all lowercase characters whenever possible.</p><p align=\"justify\">Complete "
        "conventions for disambiguating package names and rules for naming packages when the "
        "Internet domain name cannot be directly used as a package name are described in "
        "section 7.7 of the Java Language Specification.</p>"));
    formLayout->addRow(::Android::Tr::tr("Package name:"), m_packageNameLineEdit);

    m_packageNameWarning = new QLabel;
    m_packageNameWarning->setText(::Android::Tr::tr("The package name is not valid."));
    m_packageNameWarning->setVisible(false);

    m_packageNameWarningIcon = new QLabel;
    m_packageNameWarningIcon->setPixmap(Utils::Icons::WARNING.pixmap());
    m_packageNameWarningIcon->setVisible(false);
    m_packageNameWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto warningRow = new QHBoxLayout;
    warningRow->setContentsMargins(0, 0, 0, 0);
    warningRow->addWidget(m_packageNameWarningIcon);
    warningRow->addWidget(m_packageNameWarning);

    formLayout->addRow(QString(), warningRow);

    m_versionCodeLineEdit = new QLineEdit(packageGroupBox);
    formLayout->addRow(::Android::Tr::tr("Version code:"), m_versionCodeLineEdit);

    m_versionNameLinedit = new QLineEdit(packageGroupBox);
    formLayout->addRow(::Android::Tr::tr("Version name:"), m_versionNameLinedit);

    m_androidMinSdkVersion = new QComboBox(packageGroupBox);
    m_androidMinSdkVersion->setToolTip(
                ::Android::Tr::tr("Sets the minimum required version on which this application can be run."));
    m_androidMinSdkVersion->addItem(::Android::Tr::tr("Not set"), 0);

    formLayout->addRow(::Android::Tr::tr("Minimum required SDK:"), m_androidMinSdkVersion);

    m_androidTargetSdkVersion = new QComboBox(packageGroupBox);
    m_androidTargetSdkVersion->setToolTip(
                ::Android::Tr::tr("Sets the target SDK. Set this to the highest tested version. "
                       "This disables compatibility behavior of the system for your application."));
    m_androidTargetSdkVersion->addItem(::Android::Tr::tr("Not set"), 0);

    formLayout->addRow(::Android::Tr::tr("Target SDK:"), m_androidTargetSdkVersion);

    packageGroupBox->setLayout(formLayout);

    updateSdkVersions();

    connect(m_packageNameLineEdit, &QLineEdit::textEdited,
            this, &AndroidManifestEditorWidget::setPackageName);
    connect(m_versionCodeLineEdit, &QLineEdit::textEdited, this, [this] { setDirty(); });
    connect(m_versionNameLinedit, &QLineEdit::textEdited, this, [this] { setDirty(); });
    connect(m_androidMinSdkVersion, &QComboBox::currentIndexChanged, this, [this] { setDirty(); });
    connect(m_androidTargetSdkVersion, &QComboBox::currentIndexChanged,
            this, [this] { setDirty(); });

    return packageGroupBox;
}

QGroupBox *Android::Internal::AndroidManifestEditorWidget::createApplicationGroupBox(QWidget *parent)
{
    auto applicationGroupBox = new QGroupBox(parent);
    applicationGroupBox->setTitle(::Android::Tr::tr("Application"));
    auto formLayout = new QFormLayout();

    m_appNameLineEdit = new QLineEdit(applicationGroupBox);
    formLayout->addRow(::Android::Tr::tr("Application name:"), m_appNameLineEdit);

    m_activityNameLineEdit = new QLineEdit(applicationGroupBox);
    formLayout->addRow(::Android::Tr::tr("Activity name:"), m_activityNameLineEdit);

    m_styleExtractMethod = new QComboBox(applicationGroupBox);
    formLayout->addRow(::Android::Tr::tr("Style extraction:"), m_styleExtractMethod);
    const QList<QStringList> styleMethodsMap = {
        {"default",
         "In most cases this will be the same as \"full\", but it can also be something else "
         "if needed, e.g. for compatibility reasons."},
        {"full", "Useful for Qt Widgets & Qt Quick Controls 1 apps."},
        {"minimal", "Useful for Qt Quick Controls 2 apps, it is much faster than \"full\"."},
        {"none", "Useful for apps that don't use Qt Widgets, Qt Quick Controls 1 or Qt Quick Controls 2."}};
    for (int i = 0; i <styleMethodsMap.size(); ++i) {
        m_styleExtractMethod->addItem(styleMethodsMap.at(i).first());
        m_styleExtractMethod->setItemData(i, styleMethodsMap.at(i).at(1), Qt::ToolTipRole);
    }

    m_screenOrientation = new QComboBox(applicationGroupBox);
    formLayout->addRow(::Android::Tr::tr("Screen orientation:"), m_screenOrientation);
    // https://developer.android.com/guide/topics/manifest/activity-element#screen
    const QList<QStringList> screenOrientationMap = {
        {"unspecified", "The default value. The system chooses the orientation. The policy it uses, and therefore the "
                        "choices made in specific contexts, may differ from device to device."},
        {"behind", "The same orientation as the activity that's immediately beneath it in the activity stack."},
        {"landscape", "Landscape orientation (the display is wider than it is tall)."},
        {"portrait", "Portrait orientation (the display is taller than it is wide)."},
        {"reverseLandscape", "Landscape orientation in the opposite direction from normal landscape."},
        {"reversePortrait", "Portrait orientation in the opposite direction from normal portrait."},
        {"sensorLandscape", "Landscape orientation, but can be either normal or reverse landscape based on the device "
                            "sensor. The sensor is used even if the user has locked sensor-based rotation."},
        {"sensorPortrait", "Portrait orientation, but can be either normal or reverse portrait based on the device sensor. "
                           "The sensor is used even if the user has locked sensor-based rotation."},
        {"userLandscape", "Landscape orientation, but can be either normal or reverse landscape based on the device "
                          "sensor and the user's preference."},
        {"userPortrait", "Portrait orientation, but can be either normal or reverse portrait based on the device sensor "
                         "and the user's preference."},
        {"sensor", "The orientation is determined by the device orientation sensor. The orientation of the display "
                   "depends on how the user is holding the device; it changes when the user rotates the device. "
                   "Some devices, though, will not rotate to all four possible orientations, by default. To allow all four "
                   "orientations, use \"fullSensor\" The sensor is used even if the user locked sensor-based rotation."},
        {"fullSensor", "The orientation is determined by the device orientation sensor for any of the 4 orientations. This "
                       "is similar to \"sensor\" except this allows any of the 4 possible screen orientations, regardless "
                       "of what the device will normally do (for example, some devices won't normally use reverse "
                       "portrait or reverse landscape, but this enables those)."},
        {"nosensor", "The orientation is determined without reference to a physical orientation sensor. The sensor is "
                     "ignored, so the display will not rotate based on how the user moves the device."},
        {"user", "The user's current preferred orientation."},
        {"fullUser", "If the user has locked sensor-based rotation, this behaves the same as user, otherwise it "
                     "behaves the same as fullSensor and allows any of the 4 possible screen orientations."},
        {"locked", "Locks the orientation to its current rotation, whatever that is."}};
    for (int i = 0; i <screenOrientationMap.size(); ++i) {
        m_screenOrientation->addItem(screenOrientationMap.at(i).first());
        m_screenOrientation->setItemData(i, screenOrientationMap.at(i).at(1), Qt::ToolTipRole);
    }
    applicationGroupBox->setLayout(formLayout);

    connect(m_appNameLineEdit, &QLineEdit::textEdited, this, [this] { setDirty(); });
    connect(m_activityNameLineEdit, &QLineEdit::textEdited, this, [this] { setDirty(); });
    connect(m_styleExtractMethod, &QComboBox::currentIndexChanged, this, [this] { setDirty(); });
    connect(m_screenOrientation, &QComboBox::currentIndexChanged, this, [this] { setDirty(); });

    return applicationGroupBox;
}

QGroupBox *AndroidManifestEditorWidget::createAdvancedGroupBox(QWidget *parent)
{
    auto otherGroupBox = new QGroupBox(parent);
    otherGroupBox->setTitle(::Android::Tr::tr("Advanced"));
    m_advanvedTabWidget = new QTabWidget(otherGroupBox);
    auto formLayout = new QFormLayout();

    m_iconButtons = new AndroidManifestEditorIconContainerWidget(otherGroupBox, m_textEditorWidget);
    m_advanvedTabWidget->addTab(m_iconButtons, ::Android::Tr::tr("Application icon"));

    m_splashButtons = new SplashScreenContainerWidget(otherGroupBox,
                                                      m_textEditorWidget);
    m_advanvedTabWidget->addTab(m_splashButtons, ::Android::Tr::tr("Splash screen"));

    connect(m_splashButtons, &SplashScreenContainerWidget::splashScreensModified,
            this, [this] { setDirty(); });
    connect(m_iconButtons, &AndroidManifestEditorIconContainerWidget::iconsModified,
            this, [this] { setDirty(); });

    formLayout->addRow(m_advanvedTabWidget);
    otherGroupBox->setLayout(formLayout);

    return otherGroupBox;
}

void AndroidManifestEditorWidget::initializePage()
{
    QWidget *mainWidget = new QWidget(); // different name
    auto topLayout = new QGridLayout(mainWidget);

    topLayout->addWidget(createPackageFormLayout(mainWidget), 0, 0);
    topLayout->addWidget(createApplicationGroupBox(mainWidget), 0, 1);
    topLayout->addWidget(createPermissionsGroupBox(mainWidget), 1, 0, 1, 2);
    topLayout->addWidget(createAdvancedGroupBox(mainWidget), 2, 0, 1, 2);
    topLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding), 3, 0);

    auto mainWidgetScrollArea = new QScrollArea;
    mainWidgetScrollArea->setWidgetResizable(true);
    mainWidgetScrollArea->setWidget(mainWidget);
    mainWidgetScrollArea->setFocusProxy(m_packageNameLineEdit);

    insertWidget(General, mainWidgetScrollArea);
    insertWidget(Source, m_textEditorWidget);
}

void AndroidManifestEditorWidget::focusInEvent(QFocusEvent *event)
{
    if (currentWidget()) {
        if (currentWidget()->focusWidget())
            currentWidget()->focusWidget()->setFocus(event->reason());
        else
            currentWidget()->setFocus(event->reason());
    }
}

void AndroidManifestEditorWidget::updateAfterFileLoad()
{
    QString error;
    int errorLine;
    int errorColumn;
    QDomDocument doc;
    if (doc.setContent(m_textEditorWidget->toPlainText(), &error, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &error, &errorLine, &errorColumn)) {
            if (activePage() != Source)
                syncToWidgets(doc);
            return;
        }
    }
    // some error occurred
    updateInfoBar(error, errorLine, errorColumn);
    setActivePage(Source);
}

void AndroidManifestEditorWidget::setDirty(bool dirty)
{
    if (m_stayClean || dirty == m_dirty)
        return;
    m_dirty = dirty;
    emit guiChanged();
}

bool AndroidManifestEditorWidget::isModified() const
{
    return m_dirty;
}

AndroidManifestEditorWidget::EditorPage AndroidManifestEditorWidget::activePage() const
{
    return AndroidManifestEditorWidget::EditorPage(currentIndex());
}

bool AndroidManifestEditorWidget::setActivePage(EditorPage page)
{
    EditorPage prevPage = activePage();

    if (prevPage == page)
        return true;

    if (page == Source) {
        syncToEditor();
    } else {
        if (!syncToWidgets())
            return false;
    }

    setCurrentIndex(page);

    QWidget *cw = currentWidget();
    if (cw) {
        if (cw->focusWidget())
            cw->focusWidget()->setFocus();
        else
            cw->setFocus();
    }
    return true;
}

void AndroidManifestEditorWidget::preSave()
{
    if (activePage() != Source) {
        syncToEditor();
    }

    // no need to emit changed() since this is called as part of saving
    updateInfoBar();
}

void AndroidManifestEditorWidget::postSave()
{
    const FilePath docPath = m_textEditorWidget->textDocument()->filePath();
    if (Target *target = androidTarget(docPath)) {
        if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
            QString androidNdkPlatform = AndroidConfigurations::currentConfig().bestNdkPlatformMatch(
                AndroidManager::minimumSDK(target),
                QtSupport::QtKitAspect::qtVersion(
                    androidTarget(m_textEditorWidget->textDocument()->filePath())->kit()));
            if (m_androidNdkPlatform != androidNdkPlatform) {
                m_androidNdkPlatform = androidNdkPlatform;
                bc->updateCacheAndEmitEnvironmentChanged();
                bc->regenerateBuildFiles(nullptr);
            }
        }
    }
}

Core::IEditor *AndroidManifestEditorWidget::editor() const
{
    return m_editor;
}

TextEditor::TextEditorWidget *AndroidManifestEditorWidget::textEditorWidget() const
{
    return m_textEditorWidget;
}

bool AndroidManifestEditorWidget::syncToWidgets()
{
    QDomDocument doc;
    QString errorMessage;
    int errorLine, errorColumn;
    if (doc.setContent(m_textEditorWidget->toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            syncToWidgets(doc);
            return true;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
    return false;
}

bool AndroidManifestEditorWidget::checkDocument(const QDomDocument &doc, QString *errorMessage,
                                                int *errorLine, int *errorColumn)
{
    QDomElement manifest = doc.documentElement();
    if (manifest.tagName() != QLatin1String("manifest")) {
        *errorMessage = ::Android::Tr::tr("The structure of the Android manifest file is corrupted. Expected a top level 'manifest' node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    } else if (manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).isNull()) {
        // missing either application or activity element
        *errorMessage = ::Android::Tr::tr("The structure of the Android manifest file is corrupted. Expected an 'application' and 'activity' sub node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    }
    return true;
}

void AndroidManifestEditorWidget::startParseCheck()
{
    m_timerParseCheck.start();
}

void AndroidManifestEditorWidget::delayedParseCheck()
{
    updateInfoBar();
}

void AndroidManifestEditorWidget::updateInfoBar()
{
    if (activePage() != Source) {
        m_timerParseCheck.stop();
        return;
    }
    QDomDocument doc;
    int errorLine, errorColumn;
    QString errorMessage;
    if (doc.setContent(m_textEditorWidget->toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            return;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
}

void AndroidManifestEditorWidget::updateSdkVersions()
{
    static const QPair<int, int> sdkPair{16, 31};
    int minSdk = sdkPair.first;
    const int targetSdk = sdkPair.second;
    const Target *target = androidTarget(m_textEditorWidget->textDocument()->filePath());
    if (target) {
        const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(target->kit());
        minSdk = AndroidManager::defaultMinimumSDK(qt);
    }

    for (int i = minSdk; i <= targetSdk; ++i) {
        const QString apiStr = ::Android::Tr::tr("API %1: %2").arg(i)
                .arg(AndroidManager::androidNameForApiLevel(i));
        m_androidMinSdkVersion->addItem(apiStr, i);
        m_androidTargetSdkVersion->addItem(apiStr, i);
    }
}

void AndroidManifestEditorWidget::updateInfoBar(const QString &errorMessage, int line, int column)
{
    InfoBar *infoBar = m_textEditorWidget->textDocument()->infoBar();
    QString text;
    if (line < 0)
        text = ::Android::Tr::tr("Could not parse file: \"%1\".").arg(errorMessage);
    else
        text = ::Android::Tr::tr("%2: Could not parse file: \"%1\".").arg(errorMessage).arg(line);
    InfoBarEntry infoBarEntry(infoBarId, text);
    infoBarEntry.addCustomButton(::Android::Tr::tr("Goto error"), [this] {
        m_textEditorWidget->gotoLine(m_errorLine, m_errorColumn);
    });
    infoBar->removeInfo(infoBarId);
    infoBar->addInfo(infoBarEntry);

    m_errorLine = line;
    m_errorColumn = column;
    m_timerParseCheck.stop();
}

void AndroidManifestEditorWidget::hideInfoBar()
{
    InfoBar *infoBar = m_textEditorWidget->textDocument()->infoBar();
        infoBar->removeInfo(infoBarId);
        m_timerParseCheck.stop();
}

void setApiLevel(QComboBox *box, const QDomElement &element, const QString &attribute)
{
    if (!element.isNull() && element.hasAttribute(attribute)) {
        bool ok;
        int tmp = element.attribute(attribute).toInt(&ok);
        if (ok) {
            int index = box->findData(tmp);
            if (index != -1) {
                box->setCurrentIndex(index);
                return;
            }
        }
    }
    int index = box->findData(0);
    box->setCurrentIndex(index);
}

void AndroidManifestEditorWidget::syncToWidgets(const QDomDocument &doc)
{
    m_stayClean = true;
    QDomElement manifest = doc.documentElement();
    m_packageNameLineEdit->setText(manifest.attribute(QLatin1String("package")));
    m_versionCodeLineEdit->setText(manifest.attribute(QLatin1String("android:versionCode")));
    m_versionNameLinedit->setText(manifest.attribute(QLatin1String("android:versionName")));

    QDomElement usesSdkElement = manifest.firstChildElement(QLatin1String("uses-sdk"));
    m_androidMinSdkVersion->setEnabled(!usesSdkElement.isNull());
    m_androidTargetSdkVersion->setEnabled(!usesSdkElement.isNull());
    if (!usesSdkElement.isNull()) {
        setApiLevel(m_androidMinSdkVersion, usesSdkElement, QLatin1String("android:minSdkVersion"));
        setApiLevel(m_androidTargetSdkVersion, usesSdkElement, QLatin1String("android:targetSdkVersion"));
    }

    QDomElement applicationElement = manifest.firstChildElement(QLatin1String("application"));
    m_appNameLineEdit->setText(applicationElement.attribute(QLatin1String("android:label")));

    QDomElement activityElem = applicationElement.firstChildElement(QLatin1String("activity"));
    m_activityNameLineEdit->setText(activityElem.attribute(QLatin1String("android:label")));
    m_screenOrientation->setCurrentText(activityElem.attribute(QLatin1String("android:screenOrientation")));

    QString appIconValue = applicationElement.attribute(QLatin1String("android:icon"));
    if (!appIconValue.isEmpty()) {
        QLatin1String drawable = QLatin1String("@drawable/");
        if (appIconValue.startsWith(drawable)) {
            QString appIconName = appIconValue.mid(drawable.size());
            m_iconButtons->setIconFileName(appIconName);
        }
    }

    QDomElement metadataElem = activityElem.firstChildElement(QLatin1String("meta-data"));
    enum ActivityParseGuard {none = 0, libName = 1, styleExtract = 2, stickySplash = 4, splashImages = 8, done = 16};
    int activityParseGuard = ActivityParseGuard::none;
    enum SplashImageParseGuard {splashNone = 0, splash = 1, portraitSplash = 2, landscapeSplash = 4, splashDone = 8};
    int splashParseGuard = SplashImageParseGuard::splashNone;
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name"))
                   == QLatin1String("android.app.extract_android_style")
                   && !(activityParseGuard & ActivityParseGuard::styleExtract)) {
            m_styleExtractMethod->setCurrentText(
                metadataElem.attribute(QLatin1String("android:value")));
            activityParseGuard |= ActivityParseGuard::styleExtract;
        } else if (metadataElem.attribute(QLatin1String("android:name"))
                   == QLatin1String("android.app.splash_screen_sticky")
                   && !(activityParseGuard & ActivityParseGuard::stickySplash)) {
            QString sticky = metadataElem.attribute(QLatin1String("android:value"));
            m_currentsplashSticky = (sticky == QLatin1String("true"));
            m_splashButtons->setSticky(m_currentsplashSticky);
            activityParseGuard |= ActivityParseGuard::stickySplash;
        } else if (metadataElem.attribute(QLatin1String("android:name"))
                   .startsWith(QLatin1String("android.app.splash_screen_drawable"))
                   && !(activityParseGuard & ActivityParseGuard::splashImages)
                   && !(splashParseGuard & SplashImageParseGuard::splashDone)) {
            QString attrName = metadataElem.attribute(QLatin1String("android:name"));
            QLatin1String drawable = QLatin1String("@drawable/");
            QString splashImageValue = metadataElem.attribute(QLatin1String("android:resource"));
            QString splashImageName;
            if (!splashImageValue.startsWith(drawable))
                continue;
            splashImageName = splashImageValue.mid(drawable.size());
            if (attrName == QLatin1String("android.app.splash_screen_drawable")) {
                m_splashButtons->checkSplashscreenImage(splashImageName);
                m_currentsplashImageName[0] = splashImageName;
                splashParseGuard |= SplashImageParseGuard::splash;
            } else if (attrName == QLatin1String("android.app.splash_screen_drawable_portrait")) {
                m_splashButtons->checkSplashscreenImage(splashImageName);
                m_currentsplashImageName[1] = splashImageName;
                splashParseGuard |= SplashImageParseGuard::portraitSplash;
            } else if (attrName == QLatin1String("android.app.splash_screen_drawable_landscape")) {
                m_splashButtons->checkSplashscreenImage(splashImageName);
                m_currentsplashImageName[2] = splashImageName;
                splashParseGuard |= SplashImageParseGuard::landscapeSplash;
            }
            if (splashParseGuard & SplashImageParseGuard::splashDone)
                activityParseGuard |= ActivityParseGuard::splashImages;
        }
        if (activityParseGuard == ActivityParseGuard::done)
            break;
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }

    disconnect(m_defaultPermissonsCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);
    disconnect(m_defaultFeaturesCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);

    m_defaultPermissonsCheckBox->setChecked(false);
    m_defaultFeaturesCheckBox->setChecked(false);
    QDomNodeList manifestChilds = manifest.childNodes();
    bool foundPermissionComment = false;
    bool foundFeatureComment = false;
    for (int i = 0; i < manifestChilds.size(); ++i) {
        const QDomNode &child = manifestChilds.at(i);
        if (child.isComment()) {
            QDomComment comment = child.toComment();
            if (comment.data().trimmed() == QLatin1String("%%INSERT_PERMISSIONS"))
                foundPermissionComment = true;
            else if (comment.data().trimmed() == QLatin1String("%%INSERT_FEATURES"))
                foundFeatureComment = true;
        }
    }

    m_defaultPermissonsCheckBox->setChecked(foundPermissionComment);
    m_defaultFeaturesCheckBox->setChecked(foundFeatureComment);

    connect(m_defaultPermissonsCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);
    connect(m_defaultFeaturesCheckBox, &QCheckBox::stateChanged,
            this, &AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked);

    QStringList permissions;
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        permissions << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }

    m_permissionsModel->setPermissions(permissions);
    updateAddRemovePermissionButtons();

    m_iconButtons->loadIcons();
    m_splashButtons->loadImages();

    m_stayClean = false;
    m_dirty = false;
}

int extractVersion(const QString &string)
{
    if (!string.startsWith(QLatin1String("API")))
        return 0;
    int index = string.indexOf(QLatin1Char(':'));
    if (index == -1)
        return 0;
    return string.mid(4, index - 4).toInt();
}

void AndroidManifestEditorWidget::syncToEditor()
{
    QString result;
    QXmlStreamReader reader(m_textEditorWidget->toPlainText());
    reader.setNamespaceProcessing(false);
    QXmlStreamWriter writer(&result);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(4);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.hasError()) {
            // This should not happen
            updateInfoBar();
            return;
        } else {
            if (reader.name() == QLatin1String("manifest"))
                parseManifest(reader, writer);
            else if (reader.isStartElement())
                parseUnknownElement(reader, writer);
            else
                writer.writeCurrentToken(reader);
        }
    }

    if (result == m_textEditorWidget->toPlainText())
        return;

    m_textEditorWidget->setPlainText(result);
    m_textEditorWidget->document()->setModified(true);

    m_dirty = false;
}

namespace {
QXmlStreamAttributes modifyXmlStreamAttributes(const QXmlStreamAttributes &input, const QStringList &keys,
                                               const QStringList &values, const QStringList &remove = QStringList())
{
    Q_ASSERT(keys.size() == values.size());
    QXmlStreamAttributes result;
    result.reserve(input.size());
    for (const QXmlStreamAttribute &attribute : input) {
        const QString &name = attribute.qualifiedName().toString();
        if (remove.contains(name))
            continue;
        int index = keys.indexOf(name);
        if (index == -1)
            result.push_back(attribute);
        else
            result.push_back(QXmlStreamAttribute(name,
                                                 values.at(index)));
    }

    for (int i = 0; i < keys.size(); ++i) {
        if (!result.hasAttribute(keys.at(i)))
            result.push_back(QXmlStreamAttribute(keys.at(i), values.at(i)));
    }
    return result;
}
} // end namespace

void AndroidManifestEditorWidget::parseManifest(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());
    writer.writeStartElement(reader.name().toString());

    QXmlStreamAttributes attributes = reader.attributes();
    QStringList keys = QStringList()
            << QLatin1String("package")
            << QLatin1String("android:versionCode")
            << QLatin1String("android:versionName");
    QStringList values = QStringList()
            << m_packageNameLineEdit->text()
            << m_versionCodeLineEdit->text()
            << m_versionNameLinedit->text();

    QXmlStreamAttributes result = modifyXmlStreamAttributes(attributes, keys, values);
    writer.writeAttributes(result);

    QSet<QString> permissions = Utils::toSet(m_permissionsModel->permissions());

    bool foundUsesSdk = false;
    bool foundPermissionComment = false;
    bool foundFeatureComment = false;
    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.name() == QLatin1String("application")) {
            parseApplication(reader, writer);
        } else if (reader.name() == QLatin1String("uses-sdk")) {
            parseUsesSdk(reader, writer);
            foundUsesSdk = true;
        } else if (reader.name() == QLatin1String("uses-permission")) {
            permissions.remove(parseUsesPermission(reader, writer, permissions));
        } else if (reader.isEndElement()) {
            if (!foundUsesSdk) {
                int minimumSdk = extractVersion(m_androidMinSdkVersion->currentText());
                int targetSdk = extractVersion(m_androidTargetSdkVersion->currentText());
                if (minimumSdk == 0 && targetSdk == 0) {
                    // and doesn't need to exist
                } else {
                    writer.writeEmptyElement(QLatin1String("uses-sdk"));
                    if (minimumSdk != 0)
                        writer.writeAttribute(QLatin1String("android:minSdkVersion"),
                                              QString::number(minimumSdk));
                    if (targetSdk != 0)
                        writer.writeAttribute(QLatin1String("android:targetSdkVersion"),
                                              QString::number(targetSdk));
                }
            }

            if (!foundPermissionComment && m_defaultPermissonsCheckBox->checkState() == Qt::Checked)
                writer.writeComment(QLatin1String(" %%INSERT_PERMISSIONS "));

            if (!foundFeatureComment && m_defaultFeaturesCheckBox->checkState() == Qt::Checked)
                writer.writeComment(QLatin1String(" %%INSERT_FEATURES "));

            if (!permissions.isEmpty()) {
                for (const QString &permission : std::as_const(permissions)) {
                    writer.writeEmptyElement(QLatin1String("uses-permission"));
                    writer.writeAttribute(QLatin1String("android:name"), permission);
                }
            }

            writer.writeCurrentToken(reader);
            return;
        } else if (reader.isComment()) {
            QString commentText = parseComment(reader, writer);
            if (commentText == QLatin1String("%%INSERT_PERMISSIONS"))
                foundPermissionComment = true;
            else if (commentText == QLatin1String("%%INSERT_FEATURES"))
                foundFeatureComment = true;
        } else if (reader.isStartElement()) {
            parseUnknownElement(reader, writer);
        } else {
            writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
}

void AndroidManifestEditorWidget::parseApplication(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());
    writer.writeStartElement(reader.name().toString());

    QXmlStreamAttributes attributes = reader.attributes();
    QStringList keys = {QLatin1String("android:label")};
    QStringList values = {m_appNameLineEdit->text()};
    QStringList remove;
    bool ensureIconAttribute = m_iconButtons->hasIcons();
    if (ensureIconAttribute) {
        keys << QLatin1String("android:icon");
        values << (QLatin1String("@drawable/")  + m_iconButtons->iconFileName());
    } else
        remove << QLatin1String("android:icon");

    QXmlStreamAttributes result = modifyXmlStreamAttributes(attributes, keys, values, remove);
    writer.writeAttributes(result);

    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            writer.writeCurrentToken(reader);
            return;
        } else if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("activity"))
                parseActivity(reader, writer);
            else
                parseUnknownElement(reader, writer);
        } else {
            writer.writeCurrentToken(reader);
        }

        reader.readNext();
    }
}

static void writeMetadataElement(const char *name,
                                        const char *attributeName,
                                        const QString &value,
                                        QXmlStreamWriter &writer)
{
    writer.writeStartElement(QLatin1String("meta-data"));
    writer.writeAttribute(QLatin1String("android:name"), QLatin1String(name));
    writer.writeAttribute(QLatin1String(attributeName), value);
    writer.writeEndElement();

}

void AndroidManifestEditorWidget::parseSplashScreen(QXmlStreamWriter &writer)
{
    QString splashImageName[3];
    bool splashSticky;

    if (m_splashButtons->isSplashscreenEnabled()) {
        if (m_splashButtons->hasImages())
            splashImageName[0] = m_splashButtons->imageName();
        if (m_splashButtons->hasPortraitImages())
            splashImageName[1] = m_splashButtons->portraitImageName();
        if (m_splashButtons->hasLandscapeImages())
            splashImageName[2] = m_splashButtons->landscapeImageName();
        splashSticky = m_splashButtons->isSticky();
    } else {
        for (int i = 0; i < 3; i++)
            splashImageName[i] = m_currentsplashImageName[i];
        splashSticky = m_currentsplashSticky;
    }

    if (!splashImageName[0].isEmpty())
        writeMetadataElement("android.app.splash_screen_drawable",
                             "android:resource", QLatin1String("@drawable/%1").arg(splashImageName[0]),
                             writer);
    if (!splashImageName[1].isEmpty())
        writeMetadataElement("android.app.splash_screen_drawable_portrait",
                             "android:resource", QLatin1String("@drawable/%1").arg(splashImageName[1]),
                             writer);
    if (!splashImageName[2].isEmpty())
        writeMetadataElement("android.app.splash_screen_drawable_landscape",
                             "android:resource", QLatin1String("@drawable/%1").arg(splashImageName[2]),
                             writer);
    if (splashSticky)
        writeMetadataElement("android.app.splash_screen_sticky",
                             "android:value", "true",
                             writer);
}

void AndroidManifestEditorWidget::parseActivity(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());

    writer.writeStartElement(reader.name().toString());
    QXmlStreamAttributes attributes = reader.attributes();
    QStringList keys = { QLatin1String("android:label"), QLatin1String("android:screenOrientation") };
    QStringList values = { m_activityNameLineEdit->text(), m_screenOrientation->currentText() };
    QStringList removes;
    if (m_splashButtons->hasImages() || m_splashButtons->hasPortraitImages() || m_splashButtons->hasLandscapeImages()) {
        keys << QLatin1String("android:theme");
        values << QLatin1String("@style/splashScreenTheme");
    } else {
        removes << QLatin1String("android:theme");
    }
    QXmlStreamAttributes result = modifyXmlStreamAttributes(attributes, keys, values, removes);
    writer.writeAttributes(result);

    reader.readNext();

    bool found = false;

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            parseSplashScreen(writer);
            writer.writeCurrentToken(reader);
            return;
        } else if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("meta-data")) {
                QString metaTagName = reader.attributes().value(QLatin1String("android:name")).toString();
                if (metaTagName.startsWith(QLatin1String("android.app.splash_screen")))
                    parseUnknownElement(reader, writer);
                else
                    found = parseMetaData(reader, writer) || found; // ORDER MATTERS
            } else
                parseUnknownElement(reader, writer);
        } else if (reader.isWhitespace()) {
            /* no copying of whitespace */
        } else {
            writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
}

bool AndroidManifestEditorWidget::parseMetaData(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());

    bool found = false;
    QXmlStreamAttributes attributes = reader.attributes();
    QXmlStreamAttributes result;
    QStringList keys;
    QStringList values;

    if (attributes.value(QLatin1String("android:name"))
               == QLatin1String("android.app.extract_android_style")) {
        keys = QStringList("android:value");
        values = QStringList(m_styleExtractMethod->currentText());
        result = modifyXmlStreamAttributes(attributes, keys, values);
        found = true;
    } else {
        result = attributes;
    }

    writer.writeStartElement(QLatin1String("meta-data"));
    writer.writeAttributes(result);

    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            writer.writeCurrentToken(reader);
            return found;
        } else if (reader.isStartElement()) {
            parseUnknownElement(reader, writer);
        } else {
            writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
    return found; // should never be reached
}

void AndroidManifestEditorWidget::parseUsesSdk(QXmlStreamReader &reader, QXmlStreamWriter & writer)
{
    int minimumSdk = extractVersion(m_androidMinSdkVersion->currentText());
    int targetSdk = extractVersion(m_androidTargetSdkVersion->currentText());

    QStringList keys;
    QStringList values;
    QStringList remove;
    if (minimumSdk == 0) {
        remove << QLatin1String("android:minSdkVersion");
    } else {
        keys << QLatin1String("android:minSdkVersion");
        values << QString::number(minimumSdk);
    }
    if (targetSdk == 0) {
        remove << QLatin1String("android:targetSdkVersion");
    } else {
        keys << QLatin1String("android:targetSdkVersion");
        values << QString::number(targetSdk);
    }

    QXmlStreamAttributes result = modifyXmlStreamAttributes(reader.attributes(),
                                                            keys, values, remove);
    bool removeUseSdk = result.isEmpty();
    if (!removeUseSdk) {
        writer.writeStartElement(reader.name().toString());
        writer.writeAttributes(result);
    }

    reader.readNext();
    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            if (!removeUseSdk)
                writer.writeCurrentToken(reader);
            return;
        } else {
            if (removeUseSdk) {
                removeUseSdk = false;
                writer.writeStartElement(QLatin1String("uses-sdk"));
            }

            if (reader.isStartElement())
                parseUnknownElement(reader, writer);
            else
                writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
}

QString AndroidManifestEditorWidget::parseUsesPermission(QXmlStreamReader &reader,
                                                         QXmlStreamWriter &writer,
                                                         const QSet<QString> &permissions)
{
    Q_ASSERT(reader.isStartElement());


    QString permissionName = reader.attributes().value(QLatin1String("android:name")).toString();
    bool writePermission = permissions.contains(permissionName);
    if (writePermission)
        writer.writeCurrentToken(reader);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            if (writePermission)
                writer.writeCurrentToken(reader);
            return permissionName;
        } else if (reader.isStartElement()) {
            parseUnknownElement(reader, writer);
        } else {
            writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
    return permissionName; // should not be reached
}

QString AndroidManifestEditorWidget::parseComment(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    QString commentText = reader.text().toString().trimmed();
    if (commentText == QLatin1String("%%INSERT_PERMISSIONS")) {
        if (m_defaultPermissonsCheckBox->checkState() == Qt::Unchecked)
            return commentText;
    }

    if (commentText == QLatin1String("%%INSERT_FEATURES")) {
        if (m_defaultFeaturesCheckBox->checkState() == Qt::Unchecked)
            return commentText;
    }

    writer.writeCurrentToken(reader);
    return commentText;
}

void AndroidManifestEditorWidget::parseUnknownElement(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());
    writer.writeCurrentToken(reader);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            writer.writeCurrentToken(reader);
            return;
        } else if (reader.isStartElement()) {
            parseUnknownElement(reader, writer);
        } else {
            writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
}

void AndroidManifestEditorWidget::defaultPermissionOrFeatureCheckBoxClicked()
{
    setDirty(true);
}

void AndroidManifestEditorWidget::updateAddRemovePermissionButtons()
{
    QStringList permissions = m_permissionsModel->permissions();
    m_removePermissionButton->setEnabled(!permissions.isEmpty());

    m_addPermissionButton->setEnabled(!permissions.contains(m_permissionsComboBox->currentText()));
}

void AndroidManifestEditorWidget::addPermission()
{
    m_permissionsModel->addPermission(m_permissionsComboBox->currentText());
    updateAddRemovePermissionButtons();
    setDirty(true);
}

void AndroidManifestEditorWidget::removePermission()
{
    QModelIndex idx = m_permissionsListView->currentIndex();
    if (idx.isValid())
        m_permissionsModel->removePermission(idx.row());
    updateAddRemovePermissionButtons();
    setDirty(true);
}

void AndroidManifestEditorWidget::setPackageName()
{
    const QString packageName= m_packageNameLineEdit->text();

    bool valid = checkPackageName(packageName);
    m_packageNameWarning->setVisible(!valid);
    m_packageNameWarningIcon->setVisible(!valid);
    setDirty(true);
}


///////////////////////////// PermissionsModel /////////////////////////////

PermissionsModel::PermissionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void PermissionsModel::setPermissions(const QStringList &permissions)
{
    beginResetModel();
    m_permissions = Utils::sorted(permissions);
    endResetModel();
}

const QStringList &PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString &permission)
{
    auto it = std::lower_bound(m_permissions.constBegin(), m_permissions.constEnd(), permission);
    const int idx = it - m_permissions.constBegin();
    beginInsertRows(QModelIndex(), idx, idx);
    m_permissions.insert(idx, permission);
    endInsertRows();
    return index(idx);
}

void PermissionsModel::removePermission(int index)
{
    if (index >= m_permissions.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_permissions.removeAt(index);
    endRemoveRows();
}

QVariant PermissionsModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole || !index.isValid())
        return {};
    return m_permissions[index.row()];
}

int PermissionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_permissions.count();
}


AndroidManifestTextEditorWidget::AndroidManifestTextEditorWidget(AndroidManifestEditorWidget *parent)
    : TextEditor::TextEditorWidget(parent)
{
    setTextDocument(TextEditor::TextDocumentPtr(new AndroidManifestDocument(parent)));
    textDocument()->setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    setupGenericHighlighter();
    setMarksVisible(false);

    // this context is used by the TextEditorActionHandler registered for the text editor in
    // the AndroidManifestEditorFactory
    m_context = new Core::IContext(this);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::ANDROID_MANIFEST_EDITOR_CONTEXT));
    Core::ICore::addContextObject(m_context);
}

} // Android::Internal
