/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "configurationeditor.h"

#include "abstractsettings.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>

#include <QAbstractItemView>
#include <QCompleter>
#include <QScrollBar>
#include <QStringListModel>

namespace Beautifier {
namespace Internal {

ConfigurationSyntaxHighlighter::ConfigurationSyntaxHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    m_formatKeyword = fs.toTextCharFormat(TextEditor::C_FIELD);
    m_formatComment = fs.toTextCharFormat(TextEditor::C_COMMENT);

    m_expressionComment.setPattern(QLatin1String("#[^\\n]*"));
    m_expressionComment.setMinimal(false);
}

void ConfigurationSyntaxHighlighter::setKeywords(const QStringList &keywords)
{
    if (keywords.isEmpty())
        return;

    // Check for empty keywords since they can cause an endless loop in highlightBlock().
    QStringList pattern;
    foreach (const QString &word, keywords) {
        if (!word.isEmpty())
            pattern << QRegExp::escape(word);
    }

    m_expressionKeyword.setPattern(QLatin1String("(?:\\s|^)(") + pattern.join(QLatin1Char('|'))
                                   + QLatin1String(")(?=\\s|\\:|\\=|\\,|$)"));
}

void ConfigurationSyntaxHighlighter::setCommentExpression(const QRegExp &rx)
{
    m_expressionComment = rx;
}

void ConfigurationSyntaxHighlighter::highlightBlock(const QString &text)
{
    int pos = 0;
    if (!m_expressionKeyword.isEmpty()) {
        while ((pos = m_expressionKeyword.indexIn(text, pos)) != -1) {
            const int length = m_expressionKeyword.matchedLength();
            setFormat(pos, length, m_formatKeyword);
            pos += length;
        }
    }

    if (!m_expressionComment.isEmpty()) {
        pos = 0;
        while ((pos = m_expressionComment.indexIn(text, pos)) != -1) {
            const int length = m_expressionComment.matchedLength();
            setFormat(pos, length, m_formatComment);
            pos += length;
        }
    }
}

ConfigurationEditor::ConfigurationEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_settings(0)
    , m_completer(new QCompleter(this))
    , m_model(new QStringListModel(QStringList(), m_completer))
    , m_highlighter(new ConfigurationSyntaxHighlighter(document()))
{
    m_completer->setModel(m_model);
    m_completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    m_completer->setWrapAround(false);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->popup()->installEventFilter(this);

    connect(m_completer, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated),
            this, &ConfigurationEditor::insertCompleterText);
    connect(this, &ConfigurationEditor::cursorPositionChanged,
            this, &ConfigurationEditor::updateDocumentation);
}

void ConfigurationEditor::setSettings(AbstractSettings *settings)
{
    QTC_ASSERT(settings, return);
    m_settings = settings;

    QStringList keywords = m_settings->options();
    m_highlighter->setKeywords(keywords);
    keywords << m_settings->completerWords();
    keywords.sort(Qt::CaseInsensitive);
    m_model->setStringList(keywords);
}

void ConfigurationEditor::setCommentExpression(const QRegExp &rx)
{
    m_highlighter->setCommentExpression(rx);
}

// Workaround for handling "ESC" right when popup is shown.
bool ConfigurationEditor::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        const QKeyEvent *key = static_cast<const QKeyEvent *>(event);
        if (key->key() == Qt::Key_Escape) {
            event->accept();
            m_completer->popup()->hide();
            return true;
        }
    }
    return QObject::eventFilter(object, event);
}

void ConfigurationEditor::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    if (key == Qt::Key_Escape) {
        event->ignore();
        return;
    }

    if (m_completer->popup()->isVisible()) {
        switch (key) {
        case Qt::Key_Backtab:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Tab:
            event->ignore();
            return;
        default:
            break;
        }
    }

    const bool isShortcut = ((event->modifiers() & Qt::ControlModifier) && key == Qt::Key_Space);
    if (!isShortcut)
        QPlainTextEdit::keyPressEvent(event);

    const int cursorPosition = textCursor().position();
    const QTextCursor cursor = cursorForTextUnderCursor();
    const QString prefix = cursor.selectedText();

    if (!isShortcut && (prefix.length() < 2 || cursorPosition != cursor.position())) {
        m_completer->popup()->hide();
        return;
    }

    if (prefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(prefix);
        m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    }

    if (m_completer->completionCount() == 1 && prefix == m_completer->currentCompletion()) {
        m_completer->popup()->hide();
        return;
    }

    QRect popupRect = cursorRect();
    popupRect.setWidth(m_completer->popup()->sizeHintForColumn(0)
                       + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(popupRect);
}

// Hack because '-' and '=' are treated as End/StartOfWord
QTextCursor ConfigurationEditor::cursorForTextUnderCursor(QTextCursor tc) const
{
    if (tc.isNull())
        tc = textCursor();

    tc.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    QChar ch = document()->characterAt(tc.position() - 1);
    while (!(ch.isNull() || ch.isSpace() || ch == QLatin1Char(':') || ch == QLatin1Char(','))) {
        tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor);
        ch = document()->characterAt(tc.position() - 1);
    }
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    ch = document()->characterAt(tc.position());
    while (!(ch.isNull() || ch.isSpace() || ch == QLatin1Char(':') || ch == QLatin1Char(','))) {
        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        ch = document()->characterAt(tc.position());
    }
    return tc;
}

void ConfigurationEditor::insertCompleterText(const QString &text)
{
   QTextCursor tc = textCursor();
   // Replace entire word to get case sensitivity right.
   tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                   m_completer->completionPrefix().length());
   tc.insertText(text);
   setTextCursor(tc);
}

void ConfigurationEditor::updateDocumentation()
{
    QTC_CHECK(m_settings);
    QTextCursor cursor = textCursor();

    QString word = cursorForTextUnderCursor(cursor).selectedText();
    if (word == m_lastDocumentation)
        return;

    QString doc = m_settings->documentation(word);
    if (!doc.isEmpty()) {
        m_lastDocumentation = word;
        emit documentationChanged(word, doc);
        return;
    }

    // If the documentation was empty, then try to use the line's first word or the first word
    // in front of a colon for providing a documentation.
    cursor.movePosition(QTextCursor::PreviousWord);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    const int pos = cursor.selectedText().lastIndexOf(QLatin1Char(','));
    if (-1 != pos) {
        cursor.setPosition(cursor.position() + pos);
        cursor.movePosition(QTextCursor::NextWord);
    }
    word = cursorForTextUnderCursor(cursor).selectedText();

    if (word == m_lastDocumentation)
        return;

    doc = m_settings->documentation(word);
    if (doc.isEmpty())
        return;

    m_lastDocumentation = word;
    emit documentationChanged(word, doc);
}

} // namespace Internal
} // namespace Beautifier
