/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "androidmanifesteditorwidget.h"
#include "androidmanifesteditoriconcontainerwidget.h"
#include "androidmanifesteditor.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanifestdocument.h"
#include "androidmanager.h"
#include "androidservicewidget.h"
#include "splashiconcontainerwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>

#include <qtsupport/qtkitinformation.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectwindow.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitinformation.h>

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
#include <QDir>
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
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>

#include <algorithm>
#include <limits>

using namespace ProjectExplorer;
using namespace Android;
using namespace Android::Internal;

namespace {
const char infoBarId[] = "Android.AndroidManifestEditor.InfoBar";

bool checkPackageName(const QString &packageName)
{
    const QLatin1String packageNameRegExp("^([a-z]{1}[a-z0-9_]+(\\.[a-zA-Z]{1}[a-zA-Z0-9_]*)*)$");
    return QRegularExpression(packageNameRegExp).match(packageName).hasMatch();
}

Target *androidTarget(const Utils::FilePath &fileName)
{
    for (Project *project : SessionManager::projects()) {
        if (Target *target = project->activeTarget()) {
            Kit *kit = target->kit();
            if (DeviceTypeKitAspect::deviceTypeId(kit) == Android::Constants::ANDROID_DEVICE_TYPE
                    && fileName.isChildOf(project->projectDirectory()))
                return target;
        }
    }
    return nullptr;
}

} // anonymous namespace

AndroidManifestEditorWidget::AndroidManifestEditorWidget()
    : QStackedWidget(),
      m_dirty(false),
      m_stayClean(false)
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

void AndroidManifestEditorWidget::initializePage()
{
    QWidget *mainWidget = new QWidget; // different name

    auto topLayout = new QVBoxLayout(mainWidget);

    auto packageGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(packageGroupBox);

    auto setDirtyFunc = [this] { setDirty(); };
    packageGroupBox->setTitle(tr("Package"));
    {
        auto formLayout = new QFormLayout();

        m_packageNameLineEdit = new QLineEdit(packageGroupBox);
        m_packageNameLineEdit->setToolTip(tr(
                    "<p align=\"justify\">Please choose a valid package name "
                    "for your application (for example, \"org.example.myapplication\").</p>"
                    "<p align=\"justify\">Packages are usually defined using a hierarchical naming pattern, "
                    "with levels in the hierarchy separated by periods (.) (pronounced \"dot\").</p>"
                    "<p align=\"justify\">In general, a package name begins with the top level domain name"
                    " of the organization and then the organization's domain and then any subdomains listed"
                    " in reverse order. The organization can then choose a specific name for their package."
                    " Package names should be all lowercase characters whenever possible.</p>"
                    "<p align=\"justify\">Complete conventions for disambiguating package names and rules for"
                    " naming packages when the Internet domain name cannot be directly used as a package name"
                    " are described in section 7.7 of the Java Language Specification.</p>"));
        formLayout->addRow(tr("Package name:"), m_packageNameLineEdit);

        m_packageNameWarning = new QLabel;
        m_packageNameWarning->setText(tr("The package name is not valid."));
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
        formLayout->addRow(tr("Version code:"), m_versionCodeLineEdit);

        m_versionNameLinedit = new QLineEdit(packageGroupBox);
        formLayout->addRow(tr("Version name:"), m_versionNameLinedit);

        m_androidMinSdkVersion = new QComboBox(packageGroupBox);
        m_androidMinSdkVersion->setToolTip(
                    tr("Sets the minimum required version on which this application can be run."));
        m_androidMinSdkVersion->addItem(tr("Not set"), 0);

        formLayout->addRow(tr("Minimum required SDK:"), m_androidMinSdkVersion);

        m_androidTargetSdkVersion = new QComboBox(packageGroupBox);
        m_androidTargetSdkVersion->setToolTip(
                  tr("Sets the target SDK. Set this to the highest tested version. "
                     "This disables compatibility behavior of the system for your application."));
        m_androidTargetSdkVersion->addItem(tr("Not set"), 0);

        formLayout->addRow(tr("Target SDK:"), m_androidTargetSdkVersion);

        packageGroupBox->setLayout(formLayout);

        updateSdkVersions();

        connect(m_packageNameLineEdit, &QLineEdit::textEdited,
                this, &AndroidManifestEditorWidget::setPackageName);
        connect(m_versionCodeLineEdit, &QLineEdit::textEdited,
                 this, setDirtyFunc);
        connect(m_versionNameLinedit, &QLineEdit::textEdited,
                this, setDirtyFunc);
        connect(m_androidMinSdkVersion,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, setDirtyFunc);
        connect(m_androidTargetSdkVersion,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, setDirtyFunc);

    }

    // Application
    auto applicationGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(applicationGroupBox);

    applicationGroupBox->setTitle(tr("Application"));
    {
        auto formLayout = new QFormLayout();

        m_appNameLineEdit = new QLineEdit(applicationGroupBox);
        formLayout->addRow(tr("Application name:"), m_appNameLineEdit);

        m_activityNameLineEdit = new QLineEdit(applicationGroupBox);
        formLayout->addRow(tr("Activity name:"), m_activityNameLineEdit);

        m_targetLineEdit = new QComboBox(applicationGroupBox);
        m_targetLineEdit->setEditable(true);
        m_targetLineEdit->setDuplicatesEnabled(true);
        m_targetLineEdit->installEventFilter(this);
        formLayout->addRow(tr("Run:"), m_targetLineEdit);

        m_styleExtractMethod = new QComboBox(applicationGroupBox);
        formLayout->addRow(tr("Style extraction:"), m_styleExtractMethod);
        const QList<QStringList> styleMethodsMap = {
            {"default", "In most cases this will be the same as \"full\", but it can also be something else if needed, e.g. for compatibility reasons."},
            {"full", "Useful for Qt Widgets & Qt Quick Controls 1 apps."},
            {"minimal", "Useful for Qt Quick Controls 2 apps, it is much faster than \"full\"."},
            {"none", "Useful for apps that don't use Qt Widgets, Qt Quick Controls 1 or Qt Quick Controls 2."}};
        for (int i = 0; i <styleMethodsMap.size(); ++i) {
            m_styleExtractMethod->addItem(styleMethodsMap.at(i).first());
            m_styleExtractMethod->setItemData(i, styleMethodsMap.at(i).at(1), Qt::ToolTipRole);
        }

        m_iconButtons = new AndroidManifestEditorIconContainerWidget(applicationGroupBox, m_textEditorWidget);
        formLayout->addRow(tr("Application icon:"), new QLabel());
        formLayout->addRow(QString(), m_iconButtons);

        m_splashButtons = new SplashIconContainerWidget(applicationGroupBox, m_textEditorWidget);
        formLayout->addRow(tr("Splash screen:"), new QLabel());
        formLayout->addRow(QString(), m_splashButtons);

        m_services = new AndroidServiceWidget(this);
        formLayout->addRow(tr("Android services:"), m_services);

        applicationGroupBox->setLayout(formLayout);

        connect(m_appNameLineEdit, &QLineEdit::textEdited,
                this, setDirtyFunc);
        connect(m_activityNameLineEdit, &QLineEdit::textEdited,
                this, setDirtyFunc);
        connect(m_targetLineEdit, &QComboBox::currentTextChanged,
                this, setDirtyFunc);
        connect(m_styleExtractMethod,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, setDirtyFunc);
        connect(m_services, &AndroidServiceWidget::servicesModified,
                this, setDirtyFunc);
        connect(m_splashButtons, &SplashIconContainerWidget::splashScreensModified,
                this, setDirtyFunc);
        connect(m_services, &AndroidServiceWidget::servicesModified,
                this, &AndroidManifestEditorWidget::clearInvalidServiceInfo);
        connect(m_services, &AndroidServiceWidget::servicesInvalid,
                this, &AndroidManifestEditorWidget::setInvalidServiceInfo);
    }


    // Permissions
    auto permissionsGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(permissionsGroupBox);

    permissionsGroupBox->setTitle(tr("Permissions"));
    {
        auto layout = new QGridLayout(permissionsGroupBox);

        m_defaultPermissonsCheckBox = new QCheckBox(this);
        m_defaultPermissonsCheckBox->setText(tr("Include default permissions for Qt modules."));
        layout->addWidget(m_defaultPermissonsCheckBox, 0, 0);

        m_defaultFeaturesCheckBox = new QCheckBox(this);
        m_defaultFeaturesCheckBox->setText(tr("Include default features for Qt modules."));
        layout->addWidget(m_defaultFeaturesCheckBox, 1, 0);

        m_permissionsComboBox = new QComboBox(permissionsGroupBox);
        m_permissionsComboBox->insertItems(0, QStringList()
         << QLatin1String("android.permission.ACCESS_CHECKIN_PROPERTIES")
         << QLatin1String("android.permission.ACCESS_COARSE_LOCATION")
         << QLatin1String("android.permission.ACCESS_FINE_LOCATION")
         << QLatin1String("android.permission.ACCESS_LOCATION_EXTRA_COMMANDS")
         << QLatin1String("android.permission.ACCESS_MOCK_LOCATION")
         << QLatin1String("android.permission.ACCESS_NETWORK_STATE")
         << QLatin1String("android.permission.ACCESS_SURFACE_FLINGER")
         << QLatin1String("android.permission.ACCESS_WIFI_STATE")
         << QLatin1String("android.permission.ACCOUNT_MANAGER")
         << QLatin1String("com.android.voicemail.permission.ADD_VOICEMAIL")
         << QLatin1String("android.permission.AUTHENTICATE_ACCOUNTS")
         << QLatin1String("android.permission.BATTERY_STATS")
         << QLatin1String("android.permission.BIND_ACCESSIBILITY_SERVICE")
         << QLatin1String("android.permission.BIND_APPWIDGET")
         << QLatin1String("android.permission.BIND_DEVICE_ADMIN")
         << QLatin1String("android.permission.BIND_INPUT_METHOD")
         << QLatin1String("android.permission.BIND_REMOTEVIEWS")
         << QLatin1String("android.permission.BIND_TEXT_SERVICE")
         << QLatin1String("android.permission.BIND_VPN_SERVICE")
         << QLatin1String("android.permission.BIND_WALLPAPER")
         << QLatin1String("android.permission.BLUETOOTH")
         << QLatin1String("android.permission.BLUETOOTH_ADMIN")
         << QLatin1String("android.permission.BRICK")
         << QLatin1String("android.permission.BROADCAST_PACKAGE_REMOVED")
         << QLatin1String("android.permission.BROADCAST_SMS")
         << QLatin1String("android.permission.BROADCAST_STICKY")
         << QLatin1String("android.permission.BROADCAST_WAP_PUSH")
         << QLatin1String("android.permission.CALL_PHONE")
         << QLatin1String("android.permission.CALL_PRIVILEGED")
         << QLatin1String("android.permission.CAMERA")
         << QLatin1String("android.permission.CHANGE_COMPONENT_ENABLED_STATE")
         << QLatin1String("android.permission.CHANGE_CONFIGURATION")
         << QLatin1String("android.permission.CHANGE_NETWORK_STATE")
         << QLatin1String("android.permission.CHANGE_WIFI_MULTICAST_STATE")
         << QLatin1String("android.permission.CHANGE_WIFI_STATE")
         << QLatin1String("android.permission.CLEAR_APP_CACHE")
         << QLatin1String("android.permission.CLEAR_APP_USER_DATA")
         << QLatin1String("android.permission.CONTROL_LOCATION_UPDATES")
         << QLatin1String("android.permission.DELETE_CACHE_FILES")
         << QLatin1String("android.permission.DELETE_PACKAGES")
         << QLatin1String("android.permission.DEVICE_POWER")
         << QLatin1String("android.permission.DIAGNOSTIC")
         << QLatin1String("android.permission.DISABLE_KEYGUARD")
         << QLatin1String("android.permission.DUMP")
         << QLatin1String("android.permission.EXPAND_STATUS_BAR")
         << QLatin1String("android.permission.FACTORY_TEST")
         << QLatin1String("android.permission.FLASHLIGHT")
         << QLatin1String("android.permission.FORCE_BACK")
         << QLatin1String("android.permission.GET_ACCOUNTS")
         << QLatin1String("android.permission.GET_PACKAGE_SIZE")
         << QLatin1String("android.permission.GET_TASKS")
         << QLatin1String("android.permission.GLOBAL_SEARCH")
         << QLatin1String("android.permission.HARDWARE_TEST")
         << QLatin1String("android.permission.INJECT_EVENTS")
         << QLatin1String("android.permission.INSTALL_LOCATION_PROVIDER")
         << QLatin1String("android.permission.INSTALL_PACKAGES")
         << QLatin1String("android.permission.INTERNAL_SYSTEM_WINDOW")
         << QLatin1String("android.permission.INTERNET")
         << QLatin1String("android.permission.KILL_BACKGROUND_PROCESSES")
         << QLatin1String("android.permission.MANAGE_ACCOUNTS")
         << QLatin1String("android.permission.MANAGE_APP_TOKENS")
         << QLatin1String("android.permission.MASTER_CLEAR")
         << QLatin1String("android.permission.MODIFY_AUDIO_SETTINGS")
         << QLatin1String("android.permission.MODIFY_PHONE_STATE")
         << QLatin1String("android.permission.MOUNT_FORMAT_FILESYSTEMS")
         << QLatin1String("android.permission.MOUNT_UNMOUNT_FILESYSTEMS")
         << QLatin1String("android.permission.NFC")
         << QLatin1String("android.permission.PERSISTENT_ACTIVITY")
         << QLatin1String("android.permission.PROCESS_OUTGOING_CALLS")
         << QLatin1String("android.permission.READ_CALENDAR")
         << QLatin1String("android.permission.READ_CALL_LOG")
         << QLatin1String("android.permission.READ_CONTACTS")
         << QLatin1String("android.permission.READ_EXTERNAL_STORAGE")
         << QLatin1String("android.permission.READ_FRAME_BUFFER")
         << QLatin1String("com.android.browser.permission.READ_HISTORY_BOOKMARKS")
         << QLatin1String("android.permission.READ_INPUT_STATE")
         << QLatin1String("android.permission.READ_LOGS")
         << QLatin1String("android.permission.READ_PHONE_STATE")
         << QLatin1String("android.permission.READ_PROFILE")
         << QLatin1String("android.permission.READ_SMS")
         << QLatin1String("android.permission.READ_SOCIAL_STREAM")
         << QLatin1String("android.permission.READ_SYNC_SETTINGS")
         << QLatin1String("android.permission.READ_SYNC_STATS")
         << QLatin1String("android.permission.READ_USER_DICTIONARY")
         << QLatin1String("android.permission.REBOOT")
         << QLatin1String("android.permission.RECEIVE_BOOT_COMPLETED")
         << QLatin1String("android.permission.RECEIVE_MMS")
         << QLatin1String("android.permission.RECEIVE_SMS")
         << QLatin1String("android.permission.RECEIVE_WAP_PUSH")
         << QLatin1String("android.permission.RECORD_AUDIO")
         << QLatin1String("android.permission.REORDER_TASKS")
         << QLatin1String("android.permission.RESTART_PACKAGES")
         << QLatin1String("android.permission.SEND_SMS")
         << QLatin1String("android.permission.SET_ACTIVITY_WATCHER")
         << QLatin1String("com.android.alarm.permission.SET_ALARM")
         << QLatin1String("android.permission.SET_ALWAYS_FINISH")
         << QLatin1String("android.permission.SET_ANIMATION_SCALE")
         << QLatin1String("android.permission.SET_DEBUG_APP")
         << QLatin1String("android.permission.SET_ORIENTATION")
         << QLatin1String("android.permission.SET_POINTER_SPEED")
         << QLatin1String("android.permission.SET_PREFERRED_APPLICATIONS")
         << QLatin1String("android.permission.SET_PROCESS_LIMIT")
         << QLatin1String("android.permission.SET_TIME")
         << QLatin1String("android.permission.SET_TIME_ZONE")
         << QLatin1String("android.permission.SET_WALLPAPER")
         << QLatin1String("android.permission.SET_WALLPAPER_HINTS")
         << QLatin1String("android.permission.SIGNAL_PERSISTENT_PROCESSES")
         << QLatin1String("android.permission.STATUS_BAR")
         << QLatin1String("android.permission.SUBSCRIBED_FEEDS_READ")
         << QLatin1String("android.permission.SUBSCRIBED_FEEDS_WRITE")
         << QLatin1String("android.permission.SYSTEM_ALERT_WINDOW")
         << QLatin1String("android.permission.UPDATE_DEVICE_STATS")
         << QLatin1String("android.permission.USE_CREDENTIALS")
         << QLatin1String("android.permission.USE_SIP")
         << QLatin1String("android.permission.VIBRATE")
         << QLatin1String("android.permission.WAKE_LOCK")
         << QLatin1String("android.permission.WRITE_APN_SETTINGS")
         << QLatin1String("android.permission.WRITE_CALENDAR")
         << QLatin1String("android.permission.WRITE_CALL_LOG")
         << QLatin1String("android.permission.WRITE_CONTACTS")
         << QLatin1String("android.permission.WRITE_EXTERNAL_STORAGE")
         << QLatin1String("android.permission.WRITE_GSERVICES")
         << QLatin1String("com.android.browser.permission.WRITE_HISTORY_BOOKMARKS")
         << QLatin1String("android.permission.WRITE_PROFILE")
         << QLatin1String("android.permission.WRITE_SECURE_SETTINGS")
         << QLatin1String("android.permission.WRITE_SETTINGS")
         << QLatin1String("android.permission.WRITE_SMS")
         << QLatin1String("android.permission.WRITE_SOCIAL_STREAM")
         << QLatin1String("android.permission.WRITE_SYNC_SETTINGS")
         << QLatin1String("android.permission.WRITE_USER_DICTIONARY")
        );
        m_permissionsComboBox->setEditable(true);
        layout->addWidget(m_permissionsComboBox, 2, 0);

        m_addPermissionButton = new QPushButton(permissionsGroupBox);
        m_addPermissionButton->setText(tr("Add"));
        layout->addWidget(m_addPermissionButton, 2, 1);

        m_permissionsModel = new PermissionsModel(this);

        m_permissionsListView = new QListView(permissionsGroupBox);
        m_permissionsListView->setModel(m_permissionsModel);
        layout->addWidget(m_permissionsListView, 3, 0, 3, 1);

        m_removePermissionButton = new QPushButton(permissionsGroupBox);
        m_removePermissionButton->setText(tr("Remove"));
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
    }

    topLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));

    auto mainWidgetScrollArea = new QScrollArea;
    mainWidgetScrollArea->setWidgetResizable(true);
    mainWidgetScrollArea->setWidget(mainWidget);
    mainWidgetScrollArea->setFocusProxy(m_packageNameLineEdit);

    insertWidget(General, mainWidgetScrollArea);
    insertWidget(Source, m_textEditorWidget);
}

bool AndroidManifestEditorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_targetLineEdit) {
        if (event->type() == QEvent::FocusIn)
            QTimer::singleShot(0, this, &AndroidManifestEditorWidget::updateTargetComboBox);
    }

    return QWidget::eventFilter(obj, event);
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

void AndroidManifestEditorWidget::updateTargetComboBox()
{
    QStringList items;
    if (Target *target = androidTarget(m_textEditorWidget->textDocument()->filePath())) {
        ProjectNode *root = target->project()->rootProjectNode();
        root->forEachProjectNode([&items](const ProjectNode *projectNode) {
            items << projectNode->targetApplications();
        });
        items.sort();
    }

    // QComboBox randomly resets what the user has entered
    // if all rows are removed, thus we ensure that the current text
    // is not removed by first adding it and then removing all old rows
    // and then adding the new rows
    QString text = m_targetLineEdit->currentText();
    m_targetLineEdit->addItem(text);
    while (m_targetLineEdit->count() > 1)
        m_targetLineEdit->removeItem(0);
    items.removeDuplicates();
    items.removeAll(text);
    m_targetLineEdit->addItems(items);
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

bool servicesValid(const QList<AndroidServiceData> &services)
{
    for (auto &&x : services)
        if (!x.isValid())
            return false;
    return true;
}

bool AndroidManifestEditorWidget::setActivePage(EditorPage page)
{
    EditorPage prevPage = activePage();

    if (prevPage == page)
        return true;

    if (page == Source) {
        if (!servicesValid(m_services->services())) {
            QMessageBox::critical(nullptr, tr("Service Definition Invalid"),
                                  tr("Cannot switch to source when there are invalid services."));
            return false;
        }
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
        if (!servicesValid(m_services->services())) {
            QMessageBox::critical(nullptr, tr("Service Definition Invalid"),
                                  tr("Cannot save when there are invalid services."));
            return;
        }
        syncToEditor();
    }

    // no need to emit changed() since this is called as part of saving
    updateInfoBar();
}

void AndroidManifestEditorWidget::postSave()
{
    const Utils::FilePath docPath = m_textEditorWidget->textDocument()->filePath();
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
        *errorMessage = tr("The structure of the Android manifest file is corrupted. Expected a top level 'manifest' node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    } else if (manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).isNull()) {
        // missing either application or activity element
        *errorMessage = tr("The structure of the Android manifest file is corrupted. Expected an 'application' and 'activity' sub node.");
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
    QPair<int, int> apiLevels = AndroidManager::apiLevelRange();
    for (int i = apiLevels.first; i < apiLevels.second + 1; ++i)
        m_androidMinSdkVersion->addItem(tr("API %1: %2")
                                        .arg(i)
                                        .arg(AndroidManager::androidNameForApiLevel(i)),
                                        i);

    for (int i = apiLevels.first; i < apiLevels.second + 1; ++i)
        m_androidTargetSdkVersion->addItem(tr("API %1: %2")
                                           .arg(i)
                                           .arg(AndroidManager::androidNameForApiLevel(i)),
                                           i);
}

void AndroidManifestEditorWidget::updateInfoBar(const QString &errorMessage, int line, int column)
{
    Utils::InfoBar *infoBar = m_textEditorWidget->textDocument()->infoBar();
    QString text;
    if (line < 0)
        text = tr("Could not parse file: \"%1\".").arg(errorMessage);
    else
        text = tr("%2: Could not parse file: \"%1\".").arg(errorMessage).arg(line);
    Utils::InfoBarEntry infoBarEntry(infoBarId, text);
    infoBarEntry.setCustomButtonInfo(tr("Goto error"), [this]() {
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
    Utils::InfoBar *infoBar = m_textEditorWidget->textDocument()->infoBar();
        infoBar->removeInfo(infoBarId);
        m_timerParseCheck.stop();
}

static const char kServicesInvalid[] = "AndroidServiceDefinitionInvalid";

void AndroidManifestEditorWidget::setInvalidServiceInfo()
{
    Utils::Id id(kServicesInvalid);
    if (m_textEditorWidget->textDocument()->infoBar()->containsInfo(id))
        return;
    Utils::InfoBarEntry info(id,
          tr("Services invalid. "
             "Manifest cannot be saved. Correct the service definitions before saving."));
    m_textEditorWidget->textDocument()->infoBar()->addInfo(info);

}

void AndroidManifestEditorWidget::clearInvalidServiceInfo()
{
    m_textEditorWidget->textDocument()->infoBar()->removeInfo(Utils::Id(kServicesInvalid));
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

    QString baseDir = m_textEditorWidget->textDocument()->filePath().toFileInfo().absolutePath();

    QDomElement applicationElement = manifest.firstChildElement(QLatin1String("application"));
    m_appNameLineEdit->setText(applicationElement.attribute(QLatin1String("android:label")));

    QDomElement activityElem = applicationElement.firstChildElement(QLatin1String("activity"));
    m_activityNameLineEdit->setText(activityElem.attribute(QLatin1String("android:label")));

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
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")
                && !(activityParseGuard & ActivityParseGuard::libName)) {
            m_targetLineEdit->setEditText(metadataElem.attribute(QLatin1String("android:value")));
            activityParseGuard |= ActivityParseGuard::libName;
        } else if (metadataElem.attribute(QLatin1String("android:name"))
                   == QLatin1String("android.app.extract_android_style")
                   && !(activityParseGuard & ActivityParseGuard::styleExtract)) {
            m_styleExtractMethod->setCurrentText(
                metadataElem.attribute(QLatin1String("android:value")));
            activityParseGuard |= ActivityParseGuard::styleExtract;
        } else if (metadataElem.attribute(QLatin1String("android:name"))
                   == QLatin1String("android.app.splash_screen_sticky")
                   && !(activityParseGuard & ActivityParseGuard::stickySplash)) {
            QString sticky = metadataElem.attribute(QLatin1String("android:value"));
            m_splashButtons->setSticky(sticky == QLatin1String("true"));
            activityParseGuard |= ActivityParseGuard::stickySplash;
        } else if (metadataElem.attribute(QLatin1String("android:name"))
                   .startsWith(QLatin1String("android.app.splash_screen_drawable"))
                   && !(activityParseGuard & ActivityParseGuard::splashImages)
                   && !(splashParseGuard & SplashImageParseGuard::splashDone)) {
            QString attrName = metadataElem.attribute(QLatin1String("android:name"));
            QLatin1String drawable = QLatin1String("@drawable/");
            QString splashImageValue = metadataElem.attribute(QLatin1String("android:resource"));
            QString splashImageName;
            if (splashImageValue.startsWith(drawable)) {
                splashImageName = splashImageValue.mid(drawable.size());
            }
            if (attrName == QLatin1String("android.app.splash_screen_drawable")) {
                m_splashButtons->setImageFileName(splashImageName);
                splashParseGuard |= SplashImageParseGuard::splash;
            } else if (attrName == QLatin1String("android.app.splash_screen_drawable_portrait")) {
                    m_splashButtons->setPortraitImageFileName(splashImageName);
                    splashParseGuard |= SplashImageParseGuard::portraitSplash;
            } else if (attrName == QLatin1String("android.app.splash_screen_drawable_landscape")) {
                m_splashButtons->setLandscapeImageFileName(splashImageName);
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

    QList<AndroidServiceData> services;
    QDomElement serviceElem = applicationElement.firstChildElement(QLatin1String("service"));
    while (!serviceElem.isNull()) {
        AndroidServiceData service;
        service.setClassName(serviceElem.attribute(QLatin1String("android:name")));
        QString process = serviceElem.attribute(QLatin1String("android:process"));
        service.setRunInExternalProcess(!process.isEmpty());
        service.setExternalProcessName(process);
        QDomElement serviceMetadataElem = serviceElem.firstChildElement(QLatin1String("meta-data"));
        while (!serviceMetadataElem.isNull()) {
            QString metadataName = serviceMetadataElem.attribute(QLatin1String("android:name"));
            if (metadataName == QLatin1String("android.app.lib_name")) {
                QString metadataValue = serviceMetadataElem.attribute(QLatin1String("android:value"));
                service.setRunInExternalLibrary(metadataValue != QLatin1String("-- %%INSERT_APP_LIB_NAME%% --"));
                service.setExternalLibraryName(metadataValue);
            }
            else if (metadataName == QLatin1String("android.app.arguments")) {
                QString metadataValue = serviceMetadataElem.attribute(QLatin1String("android:value"));
                service.setServiceArguments(metadataValue);
            }
            serviceMetadataElem = serviceMetadataElem.nextSiblingElement(QLatin1String("meta-data"));
        }
        services << service;
        serviceElem = serviceElem.nextSiblingElement(QLatin1String("service"));
    }
    m_services->setServices(services);

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
    return string.midRef(4, index - 4).toInt();
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
    foreach (const QXmlStreamAttribute &attribute, input) {
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
                foreach (const QString &permission, permissions) {
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
            parseNewServices(writer);
            writer.writeCurrentToken(reader);
            m_services->servicesSaved();
            return;
        } else if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("activity"))
                parseActivity(reader, writer);
            else if (reader.name() == QLatin1String("service"))
                parseService(reader, writer);
            else
                parseUnknownElement(reader, writer);
        } else if (reader.isWhitespace()) {
            /* no copying of whitespace */
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
    if (m_splashButtons->hasImages())
        writeMetadataElement("android.app.splash_screen_drawable",
                             "android:resource", QLatin1String("@drawable/") + m_splashButtons->imageFileName(),
                             writer);
    if (m_splashButtons->hasPortraitImages())
        writeMetadataElement("android.app.splash_screen_drawable_portrait",
                             "android:resource", QLatin1String("@drawable/") + m_splashButtons->portraitImageFileName(),
                             writer);
    if (m_splashButtons->hasLandscapeImages())
        writeMetadataElement("android.app.splash_screen_drawable_landscape",
                             "android:resource", QLatin1String("@drawable/") + m_splashButtons->landscapeImageFileName(),
                             writer);
    if (m_splashButtons->isSticky())
        writeMetadataElement("android.app.splash_screen_sticky",
                             "android:value", "true",
                             writer);
}

static int findService(const QString &name, const QList<AndroidServiceData> &data)
{
    for (int i  = 0; i < data.size(); ++i) {
        if (data[i].className() == name)
            return i;
    }
    return -1;
}

static void writeMetadataElement(const char *name,
                                        const char *attributeName,
                                        const char *value,
                                        QXmlStreamWriter &writer)
{
    writer.writeStartElement(QLatin1String("meta-data"));
    writer.writeAttribute(QLatin1String("android:name"), QLatin1String(name));
    writer.writeAttribute(QLatin1String(attributeName), QLatin1String(value));
    writer.writeEndElement();

}

static void addServiceArgumentsAndLibName(const AndroidServiceData &service, QXmlStreamWriter &writer)
{
    if (!service.isRunInExternalLibrary() && !service.serviceArguments().isEmpty())
        writeMetadataElement("android.app.arguments", "android:value", service.serviceArguments(), writer);
    if (service.isRunInExternalLibrary() && !service.externalLibraryName().isEmpty())
        writeMetadataElement("android.app.lib_name", "android:value", service.externalLibraryName(), writer);
    else
        writeMetadataElement("android.app.lib_name", "android:value", "-- %%INSERT_APP_LIB_NAME%% --", writer);
}

static void addServiceMetadata(QXmlStreamWriter &writer)
{
    writeMetadataElement("android.app.qt_sources_resource_id", "android:resource", "@array/qt_sources", writer);
    writeMetadataElement("android.app.repository", "android:value", "default", writer);
    writeMetadataElement("android.app.qt_libs_resource_id", "android:resource", "@array/qt_libs", writer);
    writeMetadataElement("android.app.bundled_libs_resource_id", "android:resource", "@array/bundled_libs", writer);
    writeMetadataElement("android.app.bundle_local_qt_libs", "android:value", "-- %%BUNDLE_LOCAL_QT_LIBS%% --", writer);
    writeMetadataElement("android.app.use_local_qt_libs", "android:value", "-- %%USE_LOCAL_QT_LIBS%% --", writer);
    writeMetadataElement("android.app.libs_prefix", "android:value", "/data/local/tmp/qt/", writer);
    writeMetadataElement("android.app.load_local_libs_resource_id", "android:resource", "@array/load_local_libs", writer);
    writeMetadataElement("android.app.load_local_jars", "android:value", "-- %%INSERT_LOCAL_JARS%% --", writer);
    writeMetadataElement("android.app.static_init_classes", "android:value", "-- %%INSERT_INIT_CLASSES%% --", writer);
}

void AndroidManifestEditorWidget::parseService(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());
    const auto &services = m_services->services();
    QString serviceName = reader.attributes().value(QLatin1String("android:name")).toString();
    int serviceIndex = findService(serviceName, services);
    const AndroidServiceData* serviceFound = (serviceIndex >= 0) ? &services[serviceIndex] : nullptr;
    if (serviceFound && serviceFound->isValid()) {
        writer.writeStartElement(reader.name().toString());
        writer.writeAttribute(QLatin1String("android:name"), serviceFound->className());
        if (serviceFound->isRunInExternalProcess())
            writer.writeAttribute(QLatin1String("android:process"), serviceFound->externalProcessName());
    }

    reader.readNext();

    bool bundleTagFound = false;

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            if (serviceFound && serviceFound->isValid()) {
                addServiceArgumentsAndLibName(*serviceFound, writer);
                if (serviceFound->isRunInExternalProcess() && !bundleTagFound)
                    addServiceMetadata(writer);
                writer.writeCurrentToken(reader);
            }
            return;
        } else if (reader.isStartElement()) {
            if (serviceFound && !serviceFound->isValid())
                parseUnknownElement(reader, writer, true);
            else if (reader.name() == QLatin1String("meta-data")) {
                QString metaTagName = reader.attributes().value(QLatin1String("android:name")).toString();
                if (serviceFound) {
                    if (metaTagName == QLatin1String("android.app.bundle_local_qt_libs"))
                        bundleTagFound = true;
                    if (metaTagName == QLatin1String("android.app.arguments"))
                        parseUnknownElement(reader, writer, true);
                    else if (metaTagName == QLatin1String("android.app.lib_name"))
                        parseUnknownElement(reader, writer, true);
                    else if (serviceFound->isRunInExternalProcess()
                        || metaTagName == QLatin1String("android.app.background_running"))
                        parseUnknownElement(reader, writer);
                    else
                        parseUnknownElement(reader, writer, true);
                } else
                    parseUnknownElement(reader, writer, true);
            } else
                parseUnknownElement(reader, writer, true);
        } else if (reader.isWhitespace()) {
            /* no copying of whitespace */
        } else {
            if (serviceFound)
                writer.writeCurrentToken(reader);
        }
        reader.readNext();
    }
}

void AndroidManifestEditorWidget::parseNewServices(QXmlStreamWriter &writer)
{
    const auto &services = m_services->services();
    for (const auto &x : services) {
        if (x.isNewService() && x.isValid()) {
            writer.writeStartElement(QLatin1String("service"));
            writer.writeAttribute(QLatin1String("android:name"), x.className());
            if (x.isRunInExternalProcess()) {
                writer.writeAttribute(QLatin1String("android:process"),
                                      x.externalProcessName());
            }
            addServiceArgumentsAndLibName(x, writer);
            if (x.isRunInExternalProcess())
                addServiceMetadata(writer);
            writer.writeStartElement(QLatin1String("meta-data"));
            writer.writeAttribute(QLatin1String("android:name"), QLatin1String("android.app.background_running"));
            writer.writeAttribute(QLatin1String("android:value"), QLatin1String("true"));
            writer.writeEndElement();
            writer.writeEndElement();
        }
    }
}

void AndroidManifestEditorWidget::parseActivity(QXmlStreamReader &reader, QXmlStreamWriter &writer)
{
    Q_ASSERT(reader.isStartElement());

    writer.writeStartElement(reader.name().toString());
    QXmlStreamAttributes attributes = reader.attributes();
    QStringList keys = { QLatin1String("android:label") };
    QStringList values = { m_activityNameLineEdit->text() };
    QXmlStreamAttributes result = modifyXmlStreamAttributes(attributes, keys, values);
    writer.writeAttributes(result);

    reader.readNext();

    bool found = false;

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            parseSplashScreen(writer);
            if (!found) {
                writer.writeEmptyElement(QLatin1String("meta-data"));
                writer.writeAttribute(QLatin1String("android:name"),
                                      QLatin1String("android.app.lib_name"));
                writer.writeAttribute(QLatin1String("android:value"),
                                      m_targetLineEdit->currentText());
            }
            writer.writeCurrentToken(reader);
            return;
        } else if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("meta-data")) {
                QString metaTagName = reader.attributes().value(QLatin1String("android:name")).toString();
                if (metaTagName.startsWith(QLatin1String("android.app.splash_screen")))
                    parseUnknownElement(reader, writer, true);
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

    if (attributes.value(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
        keys = QStringList("android:value");
        values = QStringList(m_targetLineEdit->currentText());
        result = modifyXmlStreamAttributes(attributes, keys, values);
        found = true;
    } else if (attributes.value(QLatin1String("android:name"))
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

void AndroidManifestEditorWidget::parseUnknownElement(QXmlStreamReader &reader, QXmlStreamWriter &writer,
                                                      bool ignore)
{
    Q_ASSERT(reader.isStartElement());
    if (!ignore)
        writer.writeCurrentToken(reader);
    reader.readNext();

    while (!reader.atEnd()) {
        if (reader.isEndElement()) {
            if (!ignore)
                writer.writeCurrentToken(reader);
            return;
        } else if (reader.isStartElement()) {
            parseUnknownElement(reader, writer, ignore);
        } else {
            if (!ignore)
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
    m_permissions = permissions;
    Utils::sort(m_permissions);
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

bool PermissionsModel::updatePermission(const QModelIndex &index, const QString &permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;

    auto it = std::lower_bound(m_permissions.constBegin(), m_permissions.constEnd(), permission);
    const int newIndex = it - m_permissions.constBegin();
    if (newIndex == index.row() || newIndex == index.row() + 1) {
        m_permissions[index.row()] = permission;
        emit dataChanged(index, index);
        return true;
    }

    beginMoveRows(QModelIndex(), index.row(), index.row(), QModelIndex(), newIndex);

    if (newIndex > index.row()) {
        m_permissions.insert(newIndex, permission);
        m_permissions.removeAt(index.row());
    } else {
        m_permissions.removeAt(index.row());
        m_permissions.insert(newIndex, permission);
    }
    endMoveRows();

    return true;
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
        return QVariant();
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
