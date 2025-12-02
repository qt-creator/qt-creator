// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "permissionscontainerwidget.h"
#include "androidtoolmenu.h"
#include "androidtr.h"
#include "androidmanifestutils.h"

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/fileutils.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QListView>
#include <QPushButton>
#include <QAbstractListModel>

using namespace Utils;

namespace Android::Internal {

class PermissionsModel : public QAbstractListModel
{
public:
    PermissionsModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    void setPermissions(const QStringList &permissions)
    {
        beginResetModel();
        m_permissions = permissions;
        endResetModel();
    }

    const QStringList &permissions() const { return m_permissions; }

    QModelIndex addPermission(const QString &permission)
    {
        if (!m_permissions.contains(permission)) {
            int row = m_permissions.size();
            beginInsertRows(QModelIndex(), row, row);
            m_permissions.append(permission);
            endInsertRows();
            return index(row, 0);
        }
        return QModelIndex();
    }

    void removePermission(int row)
    {
        if (row >= 0 && row < m_permissions.size()) {
            beginRemoveRows(QModelIndex(), row, row);
            m_permissions.removeAt(row);
            endRemoveRows();
        }
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() >= m_permissions.size())
            return QVariant();

        if (role == Qt::DisplayRole)
            return m_permissions.at(index.row());

        return QVariant();
    }

protected:
    int rowCount(const QModelIndex &parent) const override
    {
        return parent.isValid() ? 0 : m_permissions.size();
    }

private:
    QStringList m_permissions;
};

PermissionsContainerWidget::PermissionsContainerWidget(QWidget *parent)
    : QWidget(parent)
{
}

bool PermissionsContainerWidget::initialize(TextEditor::TextEditorWidget *textEditorWidget)
{
    if (!textEditorWidget)
        return false;

    m_textEditorWidget = textEditorWidget;
    m_manifestDirectory = manifestDir(textEditorWidget);

    auto mainLayout = new QVBoxLayout(this);
    auto permissionsGroupBox = new QGroupBox(this);
    permissionsGroupBox->setTitle(Android::Tr::tr("Permissions"));
    auto layout = new QGridLayout(permissionsGroupBox);

    m_defaultPermissonsCheckBox = new QCheckBox(this);
    m_defaultPermissonsCheckBox->setText(Android::Tr::tr("Include default permissions for Qt modules."));
    layout->addWidget(m_defaultPermissonsCheckBox, 0, 0);

    m_defaultFeaturesCheckBox = new QCheckBox(this);
    m_defaultFeaturesCheckBox->setText(Android::Tr::tr("Include default features for Qt modules."));
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
    m_addPermissionButton->setText(Android::Tr::tr("Add"));
    layout->addWidget(m_addPermissionButton, 2, 1);

    m_permissionsModel = new PermissionsModel(this);

    m_permissionsListView = new QListView(permissionsGroupBox);
    m_permissionsListView->setModel(m_permissionsModel);
    layout->addWidget(m_permissionsListView, 3, 0, 3, 1);

    m_removePermissionButton = new QPushButton(permissionsGroupBox);
    m_removePermissionButton->setText(Android::Tr::tr("Remove"));
    layout->addWidget(m_removePermissionButton, 3, 1);

    permissionsGroupBox->setLayout(layout);
    mainLayout->addWidget(permissionsGroupBox);
    setLayout(mainLayout);

    updateAddRemovePermissionButtons();
    loadPermissionsFromManifest();

    connect(m_defaultPermissonsCheckBox, &QCheckBox::stateChanged,
            this, &PermissionsContainerWidget::defaultPermissionOrFeatureCheckBoxClicked);
    connect(m_defaultFeaturesCheckBox, &QCheckBox::stateChanged,
            this, &PermissionsContainerWidget::defaultPermissionOrFeatureCheckBoxClicked);
    connect(m_addPermissionButton, &QAbstractButton::clicked,
            this, &PermissionsContainerWidget::addPermission);
    connect(m_removePermissionButton, &QAbstractButton::clicked,
            this, &PermissionsContainerWidget::removePermission);
    connect(m_permissionsComboBox, &QComboBox::currentTextChanged,
            this, &PermissionsContainerWidget::updateAddRemovePermissionButtons);
    connect(m_permissionsListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PermissionsContainerWidget::updateAddRemovePermissionButtons);

    return true;
}

void PermissionsContainerWidget::addPermission()
{
    QString permission = m_permissionsComboBox->currentText().trimmed();
    if (!permission.isEmpty()) {
        m_permissionsModel->addPermission(permission);
        updateManifestPermissions();
        emit permissionsModified();
    }
}

void PermissionsContainerWidget::removePermission()
{
    QModelIndex index = m_permissionsListView->currentIndex();
    if (index.isValid()) {
        m_permissionsModel->removePermission(index.row());
        updateManifestPermissions();
        emit permissionsModified();
    }
}

void PermissionsContainerWidget::updateAddRemovePermissionButtons()
{
    m_addPermissionButton->setEnabled(!m_permissionsComboBox->currentText().trimmed().isEmpty());
    m_removePermissionButton->setEnabled(m_permissionsListView->currentIndex().isValid());
}

void PermissionsContainerWidget::defaultPermissionOrFeatureCheckBoxClicked()
{
    updateManifestPermissions();
    emit permissionsModified();
}

void PermissionsContainerWidget::updateManifestPermissions()
{
    if (m_manifestDirectory.isEmpty())
        return;

    Utils::FilePath manifestPath = m_manifestDirectory / "AndroidManifest.xml";
    if (!manifestPath.exists())
        return;

    bool includeDefaultPermissions = m_defaultPermissonsCheckBox->isChecked();
    bool includeDefaultFeatures = m_defaultFeaturesCheckBox->isChecked();

    Android::Internal::updateManifestPermissions(
        manifestPath,
        m_permissionsModel->permissions(),
        includeDefaultPermissions,
        includeDefaultFeatures);

    if (m_textEditorWidget && m_textEditorWidget->textDocument())
        m_textEditorWidget->textDocument()->reload();
}

void PermissionsContainerWidget::loadPermissionsFromManifest()
{
    if (m_manifestDirectory.isEmpty())
        return;

    Utils::FilePath manifestPath = m_manifestDirectory / "AndroidManifest.xml";
    if (!manifestPath.exists())
        return;

    auto dataResult = AndroidManifestParser::readManifest(manifestPath);
    if (!dataResult)
        return;

    const AndroidManifestParser::ManifestData &data = *dataResult;

    m_defaultPermissonsCheckBox->setChecked(data.hasDefaultPermissionsComment);
    m_defaultFeaturesCheckBox->setChecked(data.hasDefaultFeaturesComment);
    m_permissionsModel->setPermissions(data.permissions);
}

} // namespace Android::Internal

#include "permissionscontainerwidget.moc"
