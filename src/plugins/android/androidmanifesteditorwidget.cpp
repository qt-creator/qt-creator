/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "androidmanifesteditorwidget.h"
#include "androidmanifesteditor.h"
#include "androidconstants.h"
#include "androidmanifestdocument.h"

#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <texteditor/plaintexteditor.h>
#include <projectexplorer/projectwindow.h>
#include <projectexplorer/iprojectproperties.h>
#include <texteditor/texteditoractionhandler.h>

#include <QLineEdit>
#include <QFileInfo>
#include <QDomDocument>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QDebug>
#include <QToolButton>
#include <utils/fileutils.h>
#include <utils/stylehelper.h>
#include <QListView>
#include <QPushButton>
#include <QFileDialog>
#include <QTimer>

namespace {
const QLatin1String packageNameRegExp("^([a-z_]{1}[a-z0-9_]+(\\.[a-zA-Z_]{1}[a-zA-Z0-9_]*)*)$");
const char infoBarId[] = "Android.AndroidManifestEditor.InfoBar";
const char androidManifestEditorGeneralPaneContextId[] = "AndroidManifestEditorWidget.GeneralWidget";

bool checkPackageName(const QString &packageName)
{
    return QRegExp(packageNameRegExp).exactMatch(packageName);
}
} // anonymous namespace


using namespace Android;
using namespace Android::Internal;

AndroidManifestEditorWidget::AndroidManifestEditorWidget(QWidget *parent, TextEditor::TextEditorActionHandler *ah)
    : TextEditor::PlainTextEditorWidget(parent),
      m_dirty(false),
      m_stayClean(false),
      m_setAppName(false),
      m_ah(ah)
{
    QSharedPointer<AndroidManifestDocument> doc(new AndroidManifestDocument(this));
    doc->setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);
    configure(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));

    initializePage();

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_timerParseCheck.setInterval(800);
    m_timerParseCheck.setSingleShot(true);

    connect(&m_timerParseCheck, SIGNAL(timeout()),
            this, SLOT(delayedParseCheck()));

    connect(document(), SIGNAL(contentsChanged()),
            this, SLOT(startParseCheck()));
}

TextEditor::BaseTextEditor *AndroidManifestEditorWidget::createEditor()
{
    return new AndroidManifestEditor(this);
}

void AndroidManifestEditorWidget::initializePage()
{
    QWidget *mainWidget = new QWidget(this);
    mainWidget->setAutoFillBackground(true);
    // If the user clicks on the mainwidget it gets focus, even though that's not visible
    // This is to prevent the parent, the actual basetexteditorwidget from getting focus
    mainWidget->setFocusPolicy(Qt::WheelFocus);

    Core::IContext *myContext = new Core::IContext(this);
    myContext->setWidget(mainWidget);
    myContext->setContext(Core::Context(androidManifestEditorGeneralPaneContextId));
    Core::ICore::addContextObject(myContext);

    QVBoxLayout *topLayout = new QVBoxLayout(mainWidget);

    QGroupBox *packageGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(packageGroupBox);

    packageGroupBox->setTitle(tr("Package"));
    {
        QFormLayout *formLayout = new QFormLayout();

        m_packageNameLineEdit = new QLineEdit(packageGroupBox);
        m_packageNameLineEdit->setToolTip(tr(
                    "<p align=\"justify\">Please choose a valid package name "
                    "for your application (e.g. \"org.example.myapplication\").</p>"
                    "<p align=\"justify\">Packages are usually defined using a hierarchical naming pattern, "
                    "with levels in the hierarchy separated by periods (.) (pronounced \"dot\").</p>"
                    "<p align=\"justify\">In general, a package name begins with the top level domain name"
                    " of the organization and then the organization's domain and then any subdomains listed"
                    "in reverse order. The organization can then choose a specific name for their package."
                    " Package names should be all lowercase characters whenever possible.</p>"
                    "<p align=\"justify\">Complete conventions for disambiguating package names and rules for"
                    " naming packages when the Internet domain name cannot be directly used as a package name"
                    " are described in section 7.7 of the Java Language Specification.</p>"));
        formLayout->addRow(tr("Package name:"), m_packageNameLineEdit);

        m_packageNameWarning = new QLabel;
        m_packageNameWarning->setText(tr("The package name is not valid."));
        m_packageNameWarning->setVisible(false);

        m_packageNameWarningIcon = new QLabel;
        m_packageNameWarningIcon->setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
        m_packageNameWarningIcon->setVisible(false);
        m_packageNameWarningIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QHBoxLayout *warningRow = new QHBoxLayout;
        warningRow->setMargin(0);
        warningRow->addWidget(m_packageNameWarningIcon);
        warningRow->addWidget(m_packageNameWarning);

        formLayout->addRow(QString(), warningRow);


        m_versionCode = new QSpinBox(packageGroupBox);
        m_versionCode->setMaximum(99);
        m_versionCode->setValue(1);
        m_versionCode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        formLayout->addRow(tr("Version code:"), m_versionCode);

        m_versionNameLinedit = new QLineEdit(packageGroupBox);
        formLayout->addRow(tr("Version name:"), m_versionNameLinedit);

        packageGroupBox->setLayout(formLayout);

        connect(m_packageNameLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setPackageName()));
        connect(m_versionCode, SIGNAL(valueChanged(int)),
                this, SLOT(setDirty()));
        connect(m_versionNameLinedit, SIGNAL(textEdited(QString)),
                this, SLOT(setDirty()));

    }

    // Application
    QGroupBox *applicationGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(applicationGroupBox);

    applicationGroupBox->setTitle(tr("Application"));
    {
        QFormLayout *formLayout = new QFormLayout();

        m_appNameLineEdit = new QLineEdit(applicationGroupBox);
        formLayout->addRow(tr("Application name:"), m_appNameLineEdit);

        m_targetLineEdit = new QLineEdit(applicationGroupBox);
        formLayout->addRow(tr("Run:"), m_targetLineEdit);

        QHBoxLayout *iconLayout = new QHBoxLayout();
        m_lIconButton = new QToolButton(applicationGroupBox);
        m_lIconButton->setMinimumSize(QSize(48, 48));
        m_lIconButton->setMaximumSize(QSize(48, 48));
        m_lIconButton->setToolTip(tr("Select low dpi icon"));
        iconLayout->addWidget(m_lIconButton);

        iconLayout->addItem(new QSpacerItem(28, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        m_mIconButton = new QToolButton(applicationGroupBox);
        m_mIconButton->setMinimumSize(QSize(48, 48));
        m_mIconButton->setMaximumSize(QSize(48, 48));
        m_mIconButton->setToolTip(tr("Select medium dpi icon"));
        iconLayout->addWidget(m_mIconButton);

        iconLayout->addItem(new QSpacerItem(28, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        m_hIconButton = new QToolButton(applicationGroupBox);
        m_hIconButton->setMinimumSize(QSize(48, 48));
        m_hIconButton->setMaximumSize(QSize(48, 48));
        m_hIconButton->setToolTip(tr("Select high dpi icon"));
        iconLayout->addWidget(m_hIconButton);

        formLayout->addRow(tr("Application icon:"), iconLayout);

        applicationGroupBox->setLayout(formLayout);

        connect(m_appNameLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setAppName()));
        connect(m_targetLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(setDirty()));

        connect(m_lIconButton, SIGNAL(clicked()), SLOT(setLDPIIcon()));
        connect(m_mIconButton, SIGNAL(clicked()), SLOT(setMDPIIcon()));
        connect(m_hIconButton, SIGNAL(clicked()), SLOT(setHDPIIcon()));
    }


    // Permissions
    QGroupBox *permissionsGroupBox = new QGroupBox(mainWidget);
    topLayout->addWidget(permissionsGroupBox);

    permissionsGroupBox->setTitle(tr("Permissions"));
    {
        QGridLayout *layout = new QGridLayout(permissionsGroupBox);

        m_permissionsModel = new PermissionsModel(this);

        m_permissionsListView = new QListView(permissionsGroupBox);
        m_permissionsListView->setModel(m_permissionsModel);
        m_permissionsListView->setMinimumSize(QSize(0, 200));
        layout->addWidget(m_permissionsListView, 0, 0, 3, 1);

        m_removePermissionButton = new QPushButton(permissionsGroupBox);
        m_removePermissionButton->setText(tr("Remove"));
        layout->addWidget(m_removePermissionButton, 0, 1);

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
        layout->addWidget(m_permissionsComboBox, 4, 0);

        m_addPermissionButton = new QPushButton(permissionsGroupBox);
        m_addPermissionButton->setText(tr("Add"));
        layout->addWidget(m_addPermissionButton, 4, 1);

        permissionsGroupBox->setLayout(layout);

        connect(m_addPermissionButton, SIGNAL(clicked()),
                this, SLOT(addPermission()));
        connect(m_removePermissionButton, SIGNAL(clicked()),
                this, SLOT(removePermission()));
        connect(m_permissionsComboBox, SIGNAL(currentTextChanged(QString)),
                this, SLOT(updateAddRemovePermissionButtons()));
    }

    topLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));

    m_overlayWidget = mainWidget;
}

void AndroidManifestEditorWidget::resizeEvent(QResizeEvent *event)
{
    PlainTextEditorWidget::resizeEvent(event);
    QSize s = QSize(rect().width(), rect().height());
    m_overlayWidget->resize(s);
}

bool AndroidManifestEditorWidget::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    bool result = PlainTextEditorWidget::open(errorString, fileName, realFileName);
    if (!result)
        return result;

    Q_UNUSED(errorString);
    QString error;
    int errorLine;
    int errorColumn;
    QDomDocument doc;
    if (doc.setContent(toPlainText(), &error, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &error, &errorLine, &errorColumn)) {
            if (activePage() != Source)
                syncToWidgets(doc);
            return true;
        }
    }
    // some error occured
    updateInfoBar(error, errorLine, errorColumn);
    setActivePage(Source);

    return true;
}

void AndroidManifestEditorWidget::setDirty(bool dirty)
{
    if (m_stayClean)
        return;
    m_dirty = dirty;
    emit changed();
}

bool AndroidManifestEditorWidget::isModified() const
{
    return m_dirty
            || !m_hIconPath.isEmpty()
            || !m_mIconPath.isEmpty()
            || !m_lIconPath.isEmpty()
            || m_setAppName;
}

AndroidManifestEditorWidget::EditorPage AndroidManifestEditorWidget::activePage() const
{
    return m_overlayWidget->isVisibleTo(const_cast<AndroidManifestEditorWidget *>(this)) ? General : Source;
}

bool AndroidManifestEditorWidget::setActivePage(EditorPage page)
{
    EditorPage prevPage = activePage();

    if (prevPage == page)
        return true;

    if (page == Source) {
        syncToEditor();
        setFocus();
    } else {
        if (!syncToWidgets()) {
            return false;
        }

        QWidget *fw = m_overlayWidget->focusWidget();
        if (fw && fw != m_overlayWidget)
            fw->setFocus();
        else
            m_packageNameLineEdit->setFocus();
    }

    m_overlayWidget->setVisible(page == General);
    if (page == General) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    return true;
}

void AndroidManifestEditorWidget::preSave()
{
    if (activePage() != Source)
        syncToEditor();

    if (m_setAppName) {
        QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
        QString fileName = baseDir + QLatin1String("/res/values/strings.xml");
        QFile f(fileName);
        if (f.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            if (doc.setContent(f.readAll())) {
                QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
                while (!metadataElem.isNull()) {
                    if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
                        metadataElem.removeChild(metadataElem.firstChild());
                        metadataElem.appendChild(doc.createTextNode(m_appNameLineEdit->text()));
                        break;
                    }
                    metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
                }

                f.close();
                f.open(QIODevice::WriteOnly);
                f.write(doc.toByteArray((4)));
            }
        }
        m_setAppName = false;
    }

    QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
    if (!m_lIconPath.isEmpty()) {
        copyIcon(LowDPI, baseDir, m_lIconPath);
        m_lIconPath.clear();
    }
    if (!m_mIconPath.isEmpty()) {
        copyIcon(MediumDPI, baseDir, m_mIconPath);
        m_mIconPath.clear();
    }
    if (!m_hIconPath.isEmpty()) {
        copyIcon(HighDPI, baseDir, m_hIconPath);
        m_hIconPath.clear();
    }
    // no need to emit changed() since this is called as part of saving

    updateInfoBar();
}

bool AndroidManifestEditorWidget::syncToWidgets()
{
    QDomDocument doc;
    QString errorMessage;
    int errorLine, errorColumn;
    if (doc.setContent(toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            syncToWidgets(doc);
            return true;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
    return false;
}

bool AndroidManifestEditorWidget::checkDocument(QDomDocument doc, QString *errorMessage, int *errorLine, int *errorColumn)
{
    QDomElement manifest = doc.documentElement();
    if (manifest.tagName() != QLatin1String("manifest")) {
        *errorMessage = tr("The structure of the android manifest file is corrupt. Expected a top level 'manifest' node.");
        *errorLine = -1;
        *errorColumn = -1;
        return false;
    } else if (manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).isNull()) {
        // missing either application or activity element
        *errorMessage = tr("The structure of the android manifest file is corrupt. Expected a 'application' and 'activity' sub node.");
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
    if (doc.setContent(toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            return;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
}

void AndroidManifestEditorWidget::updateInfoBar(const QString &errorMessage, int line, int column)
{
    Core::InfoBar *infoBar = editorDocument()->infoBar();
    QString text;
    if (line < 0)
        text = tr("Could not parse file: '%1'").arg(errorMessage);
    else
        text = tr("%2: Could not parse file: '%1'").arg(errorMessage).arg(line);
    Core::InfoBarEntry infoBarEntry(infoBarId, text);
    infoBarEntry.setCustomButtonInfo(tr("Goto error"), this, SLOT(gotoError()));
    infoBar->removeInfo(infoBarId);
    infoBar->addInfo(infoBarEntry);

    m_errorLine = line;
    m_errorColumn = column;
    m_timerParseCheck.stop();
}

void AndroidManifestEditorWidget::hideInfoBar()
{
    Core::InfoBar *infoBar = editorDocument()->infoBar();
    infoBar->removeInfo(infoBarId);
    m_timerParseCheck.stop();
}

void AndroidManifestEditorWidget::gotoError()
{
    gotoLine(m_errorLine, m_errorColumn);
}

void AndroidManifestEditorWidget::syncToWidgets(const QDomDocument &doc)
{
    m_stayClean = true;
    QDomElement manifest = doc.documentElement();
    m_packageNameLineEdit->setText(manifest.attribute(QLatin1String("package")));
    m_versionCode->setValue(manifest.attribute(QLatin1String("android:versionCode")).toInt());
    m_versionNameLinedit->setText(manifest.attribute(QLatin1String("android:versionName")));

    QString baseDir = QFileInfo(static_cast<AndroidManifestDocument *>(editor()->document())->fileName()).absolutePath();
    QString fileName = baseDir + QLatin1String("/res/values/strings.xml");

    QFile f(fileName);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        QDomDocument doc;
        if (doc.setContent(&f)) {
            QDomElement metadataElem = doc.documentElement().firstChildElement(QLatin1String("string"));
            while (!metadataElem.isNull()) {
                if (metadataElem.attribute(QLatin1String("name")) == QLatin1String("app_name")) {
                    m_appNameLineEdit->setText(metadataElem.text());
                    break;
                }
                metadataElem = metadataElem.nextSiblingElement(QLatin1String("string"));
            }
        }
    }

    QDomElement metadataElem = manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")).firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            m_targetLineEdit->setText(metadataElem.attribute(QLatin1String("android:value")));
            break;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }

    m_lIconButton->setIcon(icon(baseDir, LowDPI));
    m_mIconButton->setIcon(icon(baseDir, MediumDPI));
    m_hIconButton->setIcon(icon(baseDir, HighDPI));
    m_lIconPath.clear();
    m_mIconPath.clear();
    m_hIconPath.clear();

    QStringList permissions;
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        permissions << permissionElem.attribute(QLatin1String("android:name"));
        permissionElem = permissionElem.nextSiblingElement(QLatin1String("uses-permission"));
    }

    m_permissionsModel->setPermissions(permissions);
    updateAddRemovePermissionButtons();

    m_stayClean = false;
    m_dirty = false;
}

void AndroidManifestEditorWidget::syncToEditor()
{
    QDomDocument doc;
    if (!doc.setContent(toPlainText())) {
        // This should not happen
        updateInfoBar();
        return;
    }

    QDomElement manifest = doc.documentElement();
    manifest.setAttribute(QLatin1String("package"), m_packageNameLineEdit->text());
    manifest.setAttribute(QLatin1String("android:versionCode"), m_versionCode->value());
    manifest.setAttribute(QLatin1String("android:versionName"), m_versionNameLinedit->text());

    setAndroidAppLibName(doc, manifest.firstChildElement(QLatin1String("application")).firstChildElement(QLatin1String("activity")), m_targetLineEdit->text());

    // permissions
    QDomElement permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    while (!permissionElem.isNull()) {
        manifest.removeChild(permissionElem);
        permissionElem = manifest.firstChildElement(QLatin1String("uses-permission"));
    }

    foreach (const QString &permission, m_permissionsModel->permissions()) {
        permissionElem = doc.createElement(QLatin1String("uses-permission"));
        permissionElem.setAttribute(QLatin1String("android:name"), permission);
        manifest.appendChild(permissionElem);
    }

    bool ensureIconAttribute =  !m_lIconPath.isEmpty()
            || !m_mIconPath.isEmpty()
            || !m_hIconPath.isEmpty();

    if (ensureIconAttribute) {
        QDomElement applicationElem = manifest.firstChildElement(QLatin1String("application"));
        applicationElem.setAttribute(QLatin1String("android:icon"), QLatin1String("@drawable/icon"));
    }


    QString newText = doc.toString(4);
    if (newText == toPlainText())
        return;

    setPlainText(newText);
    document()->setModified(true); // Why is this necessary?

    m_dirty = false;
}

bool AndroidManifestEditorWidget::setAndroidAppLibName(QDomDocument document, QDomElement activity, const QString &name)
{
    QDomElement metadataElem = activity.firstChildElement(QLatin1String("meta-data"));
    while (!metadataElem.isNull()) {
        if (metadataElem.attribute(QLatin1String("android:name")) == QLatin1String("android.app.lib_name")) {
            metadataElem.setAttribute(QLatin1String("android:value"), name);
            return true;
        }
        metadataElem = metadataElem.nextSiblingElement(QLatin1String("meta-data"));
    }
    QDomElement elem = document.createElement(QLatin1String("meta-data"));
    elem.setAttribute(QLatin1String("android:name"), QLatin1String("android.app.lib_name"));
    elem.setAttribute(QLatin1String("android:value"), name);
    activity.appendChild(elem);
    return true;
}

QString AndroidManifestEditorWidget::iconPath(const QString &baseDir, IconDPI dpi)
{
    Utils::FileName fileName = Utils::FileName::fromString(baseDir);
    switch (dpi) {
    case HighDPI:
        fileName.appendPath(QLatin1String("res/drawable-hdpi/icon.png"));
        break;
    case MediumDPI:
        fileName.appendPath(QLatin1String("res/drawable-mdpi/icon.png"));
        break;
    case LowDPI:
        fileName.appendPath(QLatin1String("res/drawable-ldpi/icon.png"));
        break;
    default:
        return QString();
    }
    return fileName.toString();
}

QIcon AndroidManifestEditorWidget::icon(const QString &baseDir, IconDPI dpi)
{

    if (dpi == HighDPI && !m_hIconPath.isEmpty())
        return QIcon(m_hIconPath);

    if (dpi == MediumDPI && !m_mIconPath.isEmpty())
        return QIcon(m_mIconPath);

    if (dpi == LowDPI && !m_lIconPath.isEmpty())
        return QIcon(m_lIconPath);

    QString fileName = iconPath(baseDir, dpi);
    if (fileName.isEmpty())
        return QIcon();
    return QIcon(fileName);
}

void AndroidManifestEditorWidget::copyIcon(IconDPI dpi, const QString &baseDir, const QString &filePath)
{
    if (!QFileInfo(filePath).exists())
        return;

    const QString targetPath = iconPath(baseDir, dpi);
    QFile::remove(targetPath);
    QDir dir;
    dir.mkpath(QFileInfo(targetPath).absolutePath());
    QFile::copy(filePath, targetPath);
}

void AndroidManifestEditorWidget::setLDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Low DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_lIconPath = file;
    m_lIconButton->setIcon(QIcon(file));
    setDirty(true);
}

void AndroidManifestEditorWidget::setMDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose Medium DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_mIconPath = file;
    m_mIconButton->setIcon(QIcon(file));
    setDirty(true);
}

void AndroidManifestEditorWidget::setHDPIIcon()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Choose High DPI Icon"), QDir::homePath(), tr("PNG images (*.png)"));
    if (file.isEmpty())
        return;
    m_hIconPath = file;
    m_hIconButton->setIcon(QIcon(file));
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

void AndroidManifestEditorWidget::setAppName()
{
    m_setAppName = true;
    emit changed();
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
    qSort(m_permissions);
    endResetModel();
}

const QStringList &PermissionsModel::permissions()
{
    return m_permissions;
}

QModelIndex PermissionsModel::addPermission(const QString &permission)
{
    const int idx = qLowerBound(m_permissions, permission) - m_permissions.constBegin();
    beginInsertRows(QModelIndex(), idx, idx);
    m_permissions.insert(idx, permission);
    endInsertRows();
    return index(idx);
}

bool PermissionsModel::updatePermission(QModelIndex index, const QString &permission)
{
    if (!index.isValid())
        return false;
    if (m_permissions[index.row()] == permission)
        return false;

    int newIndex = qLowerBound(m_permissions.constBegin(), m_permissions.constEnd(), permission) - m_permissions.constBegin();
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
