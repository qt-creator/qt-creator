// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidmanifesteditor.h"

#include "androidconstants.h"
#include "androidtr.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/helpitem.h>
#include <coreplugin/icore.h>

#include <texteditor/basehoverhandler.h>

#include <utils/infobar.h>
#include <utils/tooltip/tooltip.h>

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
const QStringList manifestSectionKeywords = {
    "action",
    "application",
    "activity",
    "category",
    "data",
    "intent-filter",
    "manifest",
    "meta-data",
    "provider",
    "receiver",
    "service",
    "uses-permission",
    "uses-sdk",
    "uses-feature",
};
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

    if (manifestSectionKeywords.contains(keyword)) {
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
        setSuspendAllowed(false);
    }

private:
    bool isSaveAsAllowed() const override { return false; }
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

AndroidManifestEditor::AndroidManifestEditor(AndroidManifestEditorWidget *editorWidget)
{
    setWidget(editorWidget);
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

} // Android::Internal
