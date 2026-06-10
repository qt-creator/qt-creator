// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chatinputedit.h"
#include "acpclienttr.h"
#include "chatinputcompletion.h"

#include <utils/historycompleter.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/textutils.h>
#include <utils/theme/theme.h>

#include <texteditor/displaysettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textsuggestion.h>

#include <QAbstractItemModel>
#include <QApplication>
#include <QKeyEvent>
#include <QTextBlock>
#include <QTextLayout>

using namespace TextEditor;

namespace AcpClient::Internal {

ChatInputEdit::ChatInputEdit(QWidget *parent)
    : TextEditorWidget(parent)
{
    setupFallBackEditor(Utils::Id("AcpClient.ChatInput"));
    setTabChangesFocus(true);
    setPlaceholderText(Tr::tr("Type your message..."));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFrameShape(QFrame::NoFrame);
    setHighlightCurrentLine(false);
    setLineNumbersVisible(false);
    setMarksVisible(false);
    setMinimapVisible(false);
    setCompletionTriggerOverride(TriggeredCompletion);

    auto applyWidgetColors = [this] {
        FontSettingsData fontSettings = textDocument()->fontSettings();
        const QFont appFont = QApplication::font();
        fontSettings.setFamily(appFont.family());
        fontSettings.setFontSize(appFont.pointSize());
        fontSettings.setFontZoom(100);
        const QPalette appPalette = QApplication::palette();
        fontSettings.formatFor(C_TEXT).setForeground(creatorColor(Utils::Theme::Token_Text_Default));
        fontSettings.formatFor(C_TEXT).setBackground(creatorColor(Utils::Theme::Token_Background_Subtle));
        fontSettings.formatFor(C_DISABLED_CODE).setForeground(creatorColor(Utils::Theme::Token_Text_Subtle));
        fontSettings.formatFor(C_DISABLED_CODE).setBackground(creatorColor(Utils::Theme::Token_Background_Subtle));
        textDocument()->setFontSettings(fontSettings);
    };
    applyWidgetColors();
    connect(textDocument(), &TextEditor::TextDocument::fontSettingsChanged, this, applyWidgetColors);

    m_completionProvider = new ChatInputCompletionProvider(this);
    textDocument()->setCompletionAssistProvider(m_completionProvider);

    m_history = new Utils::HistoryCompleter("AcpChatInput", 100, this);

    updateHeight();
    connect(this, &TextEditor::TextEditorWidget::textChanged, this, &ChatInputEdit::updateHeight);
    connect(this, &TextEditor::TextEditorWidget::textChanged, this, &ChatInputEdit::updateSuggestion);
}

void ChatInputEdit::setAvailableCommands(const QList<CommandInfo> &commands)
{
    m_completionProvider->setAvailableCommands(commands);
}

void ChatInputEdit::setDisplaySettings(const DisplaySettingsData &settings)
{
    DisplaySettingsData overridden = settings;
    overridden.m_visualizeWhitespace = false;
    overridden.m_visualizeIndent = false;
    overridden.m_textWrapping = true;
    overridden.m_scrollBarHighlights = false;
    overridden.m_centerCursorOnScroll = false;
    TextEditorWidget::setDisplaySettings(overridden);
}

void ChatInputEdit::setMarginSettings(const MarginSettingsData &settings)
{
    MarginSettingsData overridden = settings;
    overridden.m_showMargin = false;
    overridden.m_useIndenter = false;
    TextEditorWidget::setMarginSettings(overridden);
}

void ChatInputEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (event->modifiers() & Qt::ShiftModifier) {
            TextEditorWidget::keyPressEvent(event);
        } else {
            const QString text = toPlainText().trimmed();
            if (!text.isEmpty())
                m_history->addEntry(text);
            m_historyIndex = -1;
            m_editBuffer.clear();
            event->accept();
            emit sendRequested();
        }
        return;
    }

    auto cursorVisualLine =
        [this]() {
            const QTextCursor cursor = textCursor();
            const QTextBlock block = cursor.block();
            const int blockStart = editorLayout()->firstLineNumberOf(block);
            return blockStart
                   + editorLayout()
                         ->blockLayout(block)
                         ->lineForTextPosition(cursor.positionInBlock())
                         .lineNumber();
        };

    if (event->key() == Qt::Key_Up && !(event->modifiers() & Qt::ShiftModifier)) {
        if (cursorVisualLine() == 0) {
            historyUp();
            return;
        }
    }

    if (event->key() == Qt::Key_Down && !(event->modifiers() & Qt::ShiftModifier)) {
        if (cursorVisualLine() == editorLayout()->lineCount() - 1) {
            historyDown();
            return;
        }
    }

    TextEditorWidget::keyPressEvent(event);
}

void ChatInputEdit::updateHeight()
{
    auto block = document()->firstBlock();
    int lineCount = 0;
    while (block.isValid() && lineCount < 5) {
        editorLayout()->ensureBlockLayout(block);
        lineCount += editorLayout()->blockLineCount(block);
        block = block.next();
    }
    if (lineCount == 0)
        lineCount = 1; // Ensure at least one line height
    const bool limitHeight = lineCount > 5;
    if (limitHeight)
        lineCount = 5; // Limit to 5 lines for height calculation
    setVerticalScrollBarPolicy(limitHeight ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    const QMargins cm = contentsMargins();
    const int docMargin = static_cast<int>(document()->documentMargin());
    const int h = fontMetrics().lineSpacing() * lineCount
                  + cm.top() + cm.bottom() + 2 * docMargin + 2 * frameWidth();
    setFixedHeight(h);
}

void ChatInputEdit::updateSuggestion()
{
    // Don't show suggestions while browsing history
    if (m_historyIndex != -1) {
        clearSuggestion();
        return;
    }

    const QString currentText = toPlainText();

    // Only suggest for non-empty, single-line input with cursor at the end
    if (currentText.isEmpty() || currentText == "/" || currentText.contains('\n')
        || textCursor().position() != document()->characterCount() - 1) {
        clearSuggestion();
        return;
    }

    const QAbstractItemModel *model = m_history->model();
    const int count = model->rowCount();
    QList<TextEditor::TextSuggestion::Data> suggestions;

    for (int i = 0; i < count; ++i) {
        const QString entry = model->data(model->index(i, 0)).toString();
        if (entry != currentText && entry.startsWith(currentText, Qt::CaseInsensitive)) {
            TextEditor::TextSuggestion::Data data;
            data.range = {Utils::Text::Position{1, 0},
                          Utils::Text::Position{1, int(currentText.length())}};
            data.position = Utils::Text::Position{1, int(currentText.length())};
            data.text = entry;
            suggestions.append(data);
        }
    }

    if (suggestions.isEmpty()) {
        clearSuggestion();
        return;
    }

    insertSuggestion(std::make_unique<TextEditor::CyclicSuggestion>(suggestions, document()));
}

void ChatInputEdit::historyUp()
{
    const QAbstractItemModel *model = m_history->model();
    const int count = model->rowCount();
    if (count == 0)
        return;

    if (m_historyIndex == -1) {
        m_editBuffer = toPlainText();
        m_historyIndex = 0;
    } else if (m_historyIndex < count - 1) {
        ++m_historyIndex;
    } else {
        return;
    }

    setPlainText(model->data(model->index(m_historyIndex, 0)).toString());
    moveCursor(QTextCursor::End);
}

void ChatInputEdit::historyDown()
{
    if (m_historyIndex == -1)
        return;

    if (m_historyIndex > 0) {
        --m_historyIndex;
        const QAbstractItemModel *model = m_history->model();
        setPlainText(model->data(model->index(m_historyIndex, 0)).toString());
    } else {
        m_historyIndex = -1;
        setPlainText(m_editBuffer);
        m_editBuffer.clear();
    }
    moveCursor(QTextCursor::End);
}

} // namespace AcpClient::Internal
