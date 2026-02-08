// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifesteditor.h"

#include "androidconstants.h"
#include "androidtr.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/helpitem.h>
#include <coreplugin/icore.h>

#include <texteditor/basehoverhandler.h>
#include <texteditor/codeassist/keywordscompletionassist.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/infobar.h>
#include <utils/tooltip/tooltip.h>

#include <QDomDocument>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

using namespace TextEditor;
using namespace Utils;

namespace {
constexpr char MANIFEST_GUIDE_BASE[] = "https://developer.android.com/guide/topics/manifest/";
}

namespace Android::Internal {

const char infoBarId[] = "Android.AndroidManifestEditor.InfoBar";
namespace {
inline const QSet<QString> manifestSectionDocsKeywords = {
    // Elements that have links to docs
    QStringLiteral("action"),
    QStringLiteral("application"),
    QStringLiteral("activity"),
    QStringLiteral("category"),
    QStringLiteral("data"),
    QStringLiteral("intent-filter"),
    QStringLiteral("manifest"),
    QStringLiteral("meta-data"),
    QStringLiteral("provider"),
    QStringLiteral("receiver"),
    QStringLiteral("service"),
    QStringLiteral("uses-permission"),
    QStringLiteral("uses-sdk"),
    QStringLiteral("uses-feature"),
};

inline const QSet<QString> manifestCompletionKeywords = {
    // Standard elements/tags
    QStringLiteral("activity-alias"),
    QStringLiteral("compatible-screens"),
    QStringLiteral("grant-uri-permission"),
    QStringLiteral("instrumentation"),
    QStringLiteral("intent-filter"),
    QStringLiteral("path-permission"),
    QStringLiteral("permission"),
    QStringLiteral("permission-group"),
    QStringLiteral("permission-tree"),
    QStringLiteral("provider"),
    QStringLiteral("queries"),
    QStringLiteral("receiver"),
    QStringLiteral("service"),
    QStringLiteral("supports-gl-texture"),
    QStringLiteral("supports-screens"),
    QStringLiteral("uses-configuration"),
    QStringLiteral("uses-feature"),
    QStringLiteral("uses-library"),
    QStringLiteral("uses-native-library"),
    QStringLiteral("uses-permission"),
    QStringLiteral("uses-sdk"),

    // Common attributes
    QStringLiteral("android:allowBackup"),
    QStringLiteral("android:configChanges"),
    QStringLiteral("android:enabled"),
    QStringLiteral("android:exported"),
    QStringLiteral("android:extractNativeLibs"),
    QStringLiteral("android:glEsVersion"),
    QStringLiteral("android:hardwareAccelerated"),
    QStringLiteral("android:hasCode"),
    QStringLiteral("android:icon"),
    QStringLiteral("android:installLocation"),
    QStringLiteral("android:label"),
    QStringLiteral("android:largeHeap"),
    QStringLiteral("android:launchMode"),
    QStringLiteral("android:localeConfig"),
    QStringLiteral("android:minSdkVersion"),
    QStringLiteral("android:name"),
    QStringLiteral("android:permission"),
    QStringLiteral("android:preserveLegacyExternalStorage"),
    QStringLiteral("android:process"),
    QStringLiteral("android:requestLegacyExternalStorage"),
    QStringLiteral("android:required"),
    QStringLiteral("android:roundIcon"),
    QStringLiteral("android:screenOrientation"),
    QStringLiteral("android:supportsRtl"),
    QStringLiteral("android:targetSdkVersion"),
    QStringLiteral("android:theme"),
    QStringLiteral("android:usesCleartextTraffic"),
    QStringLiteral("android:versionCode"),
    QStringLiteral("android:versionName"),
    QStringLiteral("xmlns:android"),
    QStringLiteral("package"),

    // Qt-specific meta-data keys
    QStringLiteral("android.app.arguments"),
    QStringLiteral("android.app.background_running"),
    QStringLiteral("android.app.extract_android_style"),
    QStringLiteral("android.app.lib_name"),
    QStringLiteral("android.app.splash_screen_drawable"),
    QStringLiteral("android.app.splash_screen_sticky"),
    QStringLiteral("android.app.system_libs_prefix"),
    QStringLiteral("android.app.trace_location"),
    QStringLiteral("-- %%INSERT_VERSION_CODE%% --"),
    QStringLiteral("-- %%INSERT_VERSION_NAME%% --"),
    QStringLiteral("<!-- %%INSERT_PERMISSIONS -->"),
    QStringLiteral("<!-- %%INSERT_FEATURES -->"),
    QStringLiteral("-- %%INSERT_APP_NAME%% --"),
    QStringLiteral("-- %%INSERT_APP_ICON%% --"),
    QStringLiteral("-- %%INSERT_APP_LIB_NAME%% --"),
    QStringLiteral("-- %%INSERT_APP_ARGUMENTS%% --"),

    // Permissions
    QStringLiteral("android.permission.ACCESS_CHECKIN_PROPERTIES"),
    QStringLiteral("android.permission.ACCESS_COARSE_LOCATION"),
    QStringLiteral("android.permission.ACCESS_FINE_LOCATION"),
    QStringLiteral("android.permission.ACCESS_LOCATION_EXTRA_COMMANDS"),
    QStringLiteral("android.permission.ACCESS_NETWORK_STATE"),
    QStringLiteral("android.permission.ACCESS_SURFACE_FLINGER"),
    QStringLiteral("android.permission.ACCESS_WIFI_STATE"),
    QStringLiteral("android.permission.ACCOUNT_MANAGER"),
    QStringLiteral("android.permission.AUTHENTICATE_ACCOUNTS"),
    QStringLiteral("android.permission.BATTERY_STATS"),
    QStringLiteral("android.permission.BIND_ACCESSIBILITY_SERVICE"),
    QStringLiteral("android.permission.BIND_APPWIDGET"),
    QStringLiteral("android.permission.BIND_DEVICE_ADMIN"),
    QStringLiteral("android.permission.BIND_INPUT_METHOD"),
    QStringLiteral("android.permission.BIND_REMOTEVIEWS"),
    QStringLiteral("android.permission.BIND_TEXT_SERVICE"),
    QStringLiteral("android.permission.BIND_VPN_SERVICE"),
    QStringLiteral("android.permission.BIND_WALLPAPER"),
    QStringLiteral("android.permission.BLUETOOTH"),
    QStringLiteral("android.permission.BLUETOOTH_ADMIN"),
    QStringLiteral("android.permission.BLUETOOTH_ADVERTISE"),
    QStringLiteral("android.permission.BLUETOOTH_CONNECT"),
    QStringLiteral("android.permission.BLUETOOTH_SCAN"),
    QStringLiteral("android.permission.BODY_SENSORS_BACKGROUND"),
    QStringLiteral("android.permission.BRICK"),
    QStringLiteral("android.permission.BROADCAST_PACKAGE_REMOVED"),
    QStringLiteral("android.permission.BROADCAST_SMS"),
    QStringLiteral("android.permission.BROADCAST_STICKY"),
    QStringLiteral("android.permission.BROADCAST_WAP_PUSH"),
    QStringLiteral("android.permission.CALL_PHONE"),
    QStringLiteral("android.permission.CALL_PRIVILEGED"),
    QStringLiteral("android.permission.CAMERA"),
    QStringLiteral("android.permission.CHANGE_COMPONENT_ENABLED_STATE"),
    QStringLiteral("android.permission.CHANGE_CONFIGURATION"),
    QStringLiteral("android.permission.CHANGE_NETWORK_STATE"),
    QStringLiteral("android.permission.CHANGE_WIFI_MULTICAST_STATE"),
    QStringLiteral("android.permission.CHANGE_WIFI_STATE"),
    QStringLiteral("android.permission.CLEAR_APP_CACHE"),
    QStringLiteral("android.permission.CLEAR_APP_USER_DATA"),
    QStringLiteral("android.permission.CONTROL_LOCATION_UPDATES"),
    QStringLiteral("android.permission.DELETE_CACHE_FILES"),
    QStringLiteral("android.permission.DELETE_PACKAGES"),
    QStringLiteral("android.permission.DEVICE_POWER"),
    QStringLiteral("android.permission.DIAGNOSTIC"),
    QStringLiteral("android.permission.DISABLE_KEYGUARD"),
    QStringLiteral("android.permission.DUMP"),
    QStringLiteral("android.permission.EXPAND_STATUS_BAR"),
    QStringLiteral("android.permission.FACTORY_TEST"),
    QStringLiteral("android.permission.FLASHLIGHT"),
    QStringLiteral("android.permission.FORCE_BACK"),
    QStringLiteral("android.permission.GET_PACKAGE_SIZE"),
    QStringLiteral("android.permission.GLOBAL_SEARCH"),
    QStringLiteral("android.permission.HARDWARE_TEST"),
    QStringLiteral("android.permission.HIGH_SAMPLING_RATE_SENSORS"),
    QStringLiteral("android.permission.INJECT_EVENTS"),
    QStringLiteral("android.permission.INSTALL_LOCATION_PROVIDER"),
    QStringLiteral("android.permission.INSTALL_PACKAGES"),
    QStringLiteral("android.permission.INTERNAL_SYSTEM_WINDOW"),
    QStringLiteral("android.permission.INTERNET"),
    QStringLiteral("android.permission.KILL_BACKGROUND_PROCESSES"),
    QStringLiteral("android.permission.MANAGE_ACCOUNTS"),
    QStringLiteral("android.permission.MANAGE_APP_TOKENS"),
    QStringLiteral("android.permission.MANAGE_WIFI_NETWORK_SELECTION"),
    QStringLiteral("android.permission.MASTER_CLEAR"),
    QStringLiteral("android.permission.MODIFY_AUDIO_SETTINGS"),
    QStringLiteral("android.permission.MODIFY_PHONE_STATE"),
    QStringLiteral("android.permission.NFC"),
    QStringLiteral("android.permission.POST_NOTIFICATIONS"),
    QStringLiteral("android.permission.PROCESS_OUTGOING_CALLS"),
    QStringLiteral("android.permission.READ_BASIC_PHONE_STATE"),
    QStringLiteral("android.permission.READ_CALENDAR"),
    QStringLiteral("android.permission.READ_CALL_LOG"),
    QStringLiteral("android.permission.READ_CONTACTS"),
    QStringLiteral("android.permission.READ_FRAME_BUFFER"),
    QStringLiteral("android.permission.READ_INPUT_STATE"),
    QStringLiteral("android.permission.READ_LOGS"),
    QStringLiteral("android.permission.READ_MEDIA_AUDIO"),
    QStringLiteral("android.permission.READ_MEDIA_IMAGES"),
    QStringLiteral("android.permission.READ_MEDIA_VIDEO"),
    QStringLiteral("android.permission.READ_MEDIA_VISUAL_USER_SELECTED"),
    QStringLiteral("android.permission.READ_PHONE_STATE"),
    QStringLiteral("android.permission.READ_PROFILE"),
    QStringLiteral("android.permission.READ_SMS"),
    QStringLiteral("android.permission.READ_SYNC_SETTINGS"),
    QStringLiteral("android.permission.READ_SYNC_STATS"),
    QStringLiteral("android.permission.READ_USER_DICTIONARY"),
    QStringLiteral("android.permission.REBOOT"),
    QStringLiteral("android.permission.RECEIVE_BOOT_COMPLETED"),
    QStringLiteral("android.permission.RECEIVE_MMS"),
    QStringLiteral("android.permission.RECEIVE_SMS"),
    QStringLiteral("android.permission.RECEIVE_WAP_PUSH"),
    QStringLiteral("android.permission.RECORD_AUDIO"),
    QStringLiteral("android.permission.REORDER_TASKS"),
    QStringLiteral("android.permission.SEND_SMS"),
    QStringLiteral("android.permission.SET_ACTIVITY_WATCHER"),
    QStringLiteral("android.permission.SET_ALWAYS_FINISH"),
    QStringLiteral("android.permission.SET_ANIMATION_SCALE"),
    QStringLiteral("android.permission.SET_DEBUG_APP"),
    QStringLiteral("android.permission.SET_ORIENTATION"),
    QStringLiteral("android.permission.SET_POINTER_SPEED"),
    QStringLiteral("android.permission.SET_PROCESS_LIMIT"),
    QStringLiteral("android.permission.SET_TIME"),
    QStringLiteral("android.permission.SET_TIME_ZONE"),
    QStringLiteral("android.permission.SET_WALLPAPER"),
    QStringLiteral("android.permission.SET_WALLPAPER_HINTS"),
    QStringLiteral("android.permission.SIGNAL_PERSISTENT_PROCESSES"),
    QStringLiteral("android.permission.STATUS_BAR"),
    QStringLiteral("android.permission.SYSTEM_ALERT_WINDOW"),
    QStringLiteral("android.permission.UPDATE_DEVICE_STATS"),
    QStringLiteral("android.permission.USE_CREDENTIALS"),
    QStringLiteral("android.permission.USE_SIP"),
    QStringLiteral("android.permission.VIBRATE"),
    QStringLiteral("android.permission.WAKE_LOCK"),
    QStringLiteral("android.permission.WRITE_APN_SETTINGS"),
    QStringLiteral("android.permission.WRITE_CALENDAR"),
    QStringLiteral("android.permission.WRITE_CALL_LOG"),
    QStringLiteral("android.permission.WRITE_CONTACTS"),
    QStringLiteral("android.permission.WRITE_GSERVICES"),
    QStringLiteral("android.permission.WRITE_PROFILE"),
    QStringLiteral("android.permission.WRITE_SECURE_SETTINGS"),
    QStringLiteral("android.permission.WRITE_SETTINGS"),
    QStringLiteral("android.permission.WRITE_SMS"),
    QStringLiteral("android.permission.WRITE_SYNC_SETTINGS"),
    QStringLiteral("android.permission.WRITE_USER_DICTIONARY"),
    QStringLiteral("com.android.alarm.permission.SET_ALARM"),
    QStringLiteral("com.android.voicemail.permission.ADD_VOICEMAIL")
};

inline const QStringList &allManifestKeywords()
{
    static const QStringList list = [] {
        QSet<QString> combinedSet = manifestSectionDocsKeywords;
        combinedSet.unite(manifestCompletionKeywords);

        return QList<QString>{combinedSet.begin(), combinedSet.end()};
    }();
    return list;
}
} // namespace

class AndroidManifestHoverHandler final : public TextEditor::BaseHoverHandler
{
private:
    QVariant m_contextHelp;
    QString m_helpToolTip;

public:
    void identifyMatch(TextEditorWidget *editorWidget, int pos, ReportPriority report) final;
    void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point) final;
};

// Expands the word under the cursor (at 'pos') to include common XML/code separators
// like '-' and '.', treating only specific punctuation and whitespace as breaks.
QString expandKeyword(QTextCursor cursor, TextEditorWidget &editorWidget, int pos)
{
    TextDocument *document = editorWidget.textDocument();
    const QList<QChar> delimiters
        = {'=', '%', '"', '\'', ' ', '\t', '\n', '\r', '\0', '<', '>', '/'};
    int start = pos;
    int end = pos;

    while (start > 0) {
        QChar charBefore = document->characterAt(start - 1);
        if (delimiters.contains(charBefore))
            break;
        start--;
    }

    int maxPos = editorWidget.textDocument()->document()->characterCount() - 1;
    while (end < maxPos) {
        QChar charAt = document->characterAt(end);
        if (delimiters.contains(charAt))
            break;
        end++;
    }
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    return cursor.selectedText().trimmed();
}

void AndroidManifestHoverHandler::identifyMatch(
    TextEditorWidget *editorWidget, int pos, ReportPriority report)
{
    const QScopeGuard cleanup([this, report] { report(priority()); });
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(pos);
    m_helpToolTip.clear();
    m_contextHelp = QVariant();
    const QString keyword = expandKeyword(cursor, *editorWidget, pos);

    if (manifestSectionDocsKeywords.contains(keyword)) {
        QStringList helpIds;
        //: %1 is an AndroidManifest keyword
        m_helpToolTip = Tr::tr("Open online documentation for %1.").arg("&lt;" + keyword + "&gt;");
        const QString docMark = keyword;
        const QString internalHelpId = keyword;
        const QUrl finalUrl = QUrl(MANIFEST_GUIDE_BASE + keyword + "-element");
        Core::HelpItem helpItem(finalUrl, docMark, Core::HelpItem::Unknown);

        helpIds << internalHelpId;
        helpItem.setHelpIds(helpIds);

        m_contextHelp = QVariant::fromValue(helpItem);
    }
    setPriority(!m_helpToolTip.isEmpty() ? Priority_Help : Priority_None);
}

void AndroidManifestHoverHandler::operateTooltip(TextEditorWidget *editorWidget, const QPoint &point)
{
    if (m_helpToolTip.isEmpty()) {
        ToolTip::hide();
    } else if (toolTip() != m_helpToolTip) {
        ToolTip::show(point, m_helpToolTip, Qt::MarkdownText, editorWidget, m_contextHelp);
        setToolTip(m_helpToolTip);
    }
}

// AndroidManifestDocument

class AndroidManifestDocument : public TextDocument
{
public:
    explicit AndroidManifestDocument()
    {
        setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
        setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
        setCompletionAssistProvider(
            new TextEditor::KeywordsCompletionAssistProvider(allManifestKeywords()));
        setSuspendAllowed(false);
    }

private:
    bool isSaveAsAllowed() const override { return false; }
};

// AndroidManifestEditorWidget

class AndroidManifestEditorWidget : public QWidget
{
public:
    explicit AndroidManifestEditorWidget();
    TextEditorWidget *textEditorWidget() const;

private:
    void updateInfoBar();
    void startParseCheck();
    void delayedParseCheck();

    void updateAfterFileLoad();

    bool checkDocument(const QDomDocument &doc, QString *errorMessage,
                       int *errorLine, int *errorColumn);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();

    int m_errorLine;
    int m_errorColumn;

    QTimer m_timerParseCheck;
    TextEditorWidget *m_textEditorWidget;
    Core::IContext *m_context;
};

AndroidManifestEditorWidget::AndroidManifestEditorWidget()
    : m_textEditorWidget(new TextEditorWidget(this))
{
    m_textEditorWidget->setTextDocument(TextDocumentPtr(new AndroidManifestDocument()));
    m_textEditorWidget->textDocument()->setMimeType(QLatin1String(Constants::ANDROID_MANIFEST_MIME_TYPE));
    m_textEditorWidget->setupGenericHighlighter();
    m_textEditorWidget->setMarksVisible(false);

    m_context = new Core::IContext(m_textEditorWidget);
    m_context->setWidget(m_textEditorWidget);
    m_context->setContext(Core::Context(Constants::ANDROID_MANIFEST_EDITOR_CONTEXT));

    m_context->setContextHelpProvider([](const Core::IContext::HelpCallback &callback) {
        const QVariant tipHelpValue = ToolTip::contextHelp();
        if (tipHelpValue.canConvert<Core::HelpItem>()) {
            const Core::HelpItem tipHelp = tipHelpValue.value<Core::HelpItem>();
            if (!tipHelp.isEmpty())
                callback(tipHelp); // executes showContextHelp(item)
        } else {
            callback(Core::HelpItem());
        }
    });

    Core::ICore::addContextObject(m_context);
    m_textEditorWidget->addHoverHandler(new AndroidManifestHoverHandler());
    m_textEditorWidget->setOptionalActions(OptionalActions::UnCommentSelection);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_textEditorWidget);

    m_timerParseCheck.setInterval(800);
    m_timerParseCheck.setSingleShot(true);

    connect(&m_timerParseCheck, &QTimer::timeout,
            this, &AndroidManifestEditorWidget::delayedParseCheck);

    connect(m_textEditorWidget->document(), &QTextDocument::contentsChanged,
            this, &AndroidManifestEditorWidget::startParseCheck);
    connect(m_textEditorWidget->textDocument(), &TextDocument::reloadFinished,
            this, [this](bool success) { if (success) updateAfterFileLoad(); });
    connect(m_textEditorWidget->textDocument(), &TextDocument::openFinishedSuccessfully,
            this, &AndroidManifestEditorWidget::updateAfterFileLoad);

    setEnabled(true);
}

void AndroidManifestEditorWidget::updateAfterFileLoad()
{
    QString error;
    int errorLine;
    int errorColumn;
    QDomDocument doc;
    if (doc.setContent(m_textEditorWidget->toPlainText(), &error, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &error, &errorLine, &errorColumn))
            return;
    }
    // some error occurred
    updateInfoBar(error, errorLine, errorColumn);
}

TextEditorWidget *AndroidManifestEditorWidget::textEditorWidget() const
{
    return m_textEditorWidget;
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
    QDomDocument doc;
    int errorLine = -1;
    int errorColumn = -1;
    QString errorMessage;
    if (doc.setContent(m_textEditorWidget->toPlainText(), &errorMessage, &errorLine, &errorColumn)) {
        if (checkDocument(doc, &errorMessage, &errorLine, &errorColumn)) {
            hideInfoBar();
            return;
        }
    }

    updateInfoBar(errorMessage, errorLine, errorColumn);
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
    infoBarEntry.addCustomButton(::Android::Tr::tr("Go to Error"), [this] {
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

// AndroidManifestEditor

class AndroidManifestEditor : public Core::IEditor
{
public:
    explicit AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget);

    QWidget *toolBar() override;
    Core::IDocument *document() const override;
    TextEditorWidget *textEditor() const;

    int currentLine() const override;
    int currentColumn() const override;
    void gotoLine(int line, int column = 0, bool centerLine = true)  override;

private:
    AndroidManifestEditorWidget *ownWidget() const;
    QToolBar *m_toolBar;
};

AndroidManifestEditor::AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget)
    : m_toolBar(nullptr)
{
    m_toolBar = new QToolBar(editorWidget);
    setWidget(editorWidget);
}

QWidget *AndroidManifestEditor::toolBar()
{
    return m_toolBar;
}

AndroidManifestEditorWidget *AndroidManifestEditor::ownWidget() const
{
    return static_cast<AndroidManifestEditorWidget *>(widget());
}

Core::IDocument *AndroidManifestEditor::document() const
{
    return textEditor()->textDocument();
}

TextEditorWidget *AndroidManifestEditor::textEditor() const
{
    return ownWidget()->textEditorWidget();
}

int AndroidManifestEditor::currentLine() const
{
    return textEditor()->textCursor().blockNumber() + 1;
}

int AndroidManifestEditor::currentColumn() const
{
    QTextCursor cursor = textEditor()->textCursor();
    return cursor.position() - cursor.block().position() + 1;
}

void AndroidManifestEditor::gotoLine(int line, int column, bool centerLine)
{
    textEditor()->gotoLine(line, column, centerLine);
}

// Factory

class AndroidManifestEditorFactory final : public Core::IEditorFactory
{
public:
    AndroidManifestEditorFactory()
    {
        setId(Constants::ANDROID_MANIFEST_EDITOR_ID);
        setDisplayName(Tr::tr("Android Manifest editor"));
        addMimeType(Constants::ANDROID_MANIFEST_MIME_TYPE);
        setEditorCreator([] {
            auto widget = new AndroidManifestEditorWidget;
            auto editor = new AndroidManifestEditor(widget);
            return editor;
        });
    }
};

void setupAndroidManifestEditor()
{
    static AndroidManifestEditorFactory theAndroidManifestEditorFactory;
}

} // Android::Internal
