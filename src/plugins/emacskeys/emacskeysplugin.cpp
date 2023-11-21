// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "emacskeysplugin.h"

#include "emacskeysconstants.h"
#include "emacskeysstate.h"
#include "emacskeystr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/qtcassert.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <QAction>
#include <QPlainTextEdit>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>

QT_BEGIN_NAMESPACE
extern void qt_set_sequence_auto_mnemonic(bool enable);
QT_END_NAMESPACE

using namespace Core;
using namespace Utils;

namespace {
QString plainSelectedText(const QTextCursor &cursor)
{
    // selectedText() returns U+2029 (PARAGRAPH SEPARATOR) instead of newline
    return cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
}
}

namespace EmacsKeys {
namespace Internal {

//---------------------------------------------------------------------------
// EmacsKeysPlugin
//---------------------------------------------------------------------------

EmacsKeysPlugin::EmacsKeysPlugin() = default;

EmacsKeysPlugin::~EmacsKeysPlugin() = default;

void EmacsKeysPlugin::initialize()
{
    // We have to use this hack here at the moment, because it's the only way to
    // disable Qt Creator menu accelerators aka mnemonics. Many of them get into
    // the way of typical emacs keys, such as: Alt+F (File), Alt+B (Build),
    // Alt+W (Window).
    qt_set_sequence_auto_mnemonic(false);

    connect(EditorManager::instance(), &EditorManager::editorAboutToClose,
            this, &EmacsKeysPlugin::editorAboutToClose);
    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &EmacsKeysPlugin::currentEditorChanged);

    registerAction(Constants::DELETE_CHARACTER,
        &EmacsKeysPlugin::deleteCharacter, Tr::tr("Delete Character"));
    registerAction(Constants::KILL_WORD,
        &EmacsKeysPlugin::killWord, Tr::tr("Kill Word"));
    registerAction(Constants::KILL_LINE,
        &EmacsKeysPlugin::killLine, Tr::tr("Kill Line"));
    registerAction(Constants::INSERT_LINE_AND_INDENT,
        &EmacsKeysPlugin::insertLineAndIndent, Tr::tr("Insert New Line and Indent"));

    registerAction(Constants::GOTO_FILE_START,
        &EmacsKeysPlugin::gotoFileStart, Tr::tr("Go to File Start"));
    registerAction(Constants::GOTO_FILE_END,
        &EmacsKeysPlugin::gotoFileEnd, Tr::tr("Go to File End"));
    registerAction(Constants::GOTO_LINE_START,
        &EmacsKeysPlugin::gotoLineStart, Tr::tr("Go to Line Start"));
    registerAction(Constants::GOTO_LINE_END,
        &EmacsKeysPlugin::gotoLineEnd, Tr::tr("Go to Line End"));
    registerAction(Constants::GOTO_NEXT_LINE,
        &EmacsKeysPlugin::gotoNextLine, Tr::tr("Go to Next Line"));
    registerAction(Constants::GOTO_PREVIOUS_LINE,
        &EmacsKeysPlugin::gotoPreviousLine, Tr::tr("Go to Previous Line"));
    registerAction(Constants::GOTO_NEXT_CHARACTER,
        &EmacsKeysPlugin::gotoNextCharacter, Tr::tr("Go to Next Character"));
    registerAction(Constants::GOTO_PREVIOUS_CHARACTER,
        &EmacsKeysPlugin::gotoPreviousCharacter, Tr::tr("Go to Previous Character"));
    registerAction(Constants::GOTO_NEXT_WORD,
        &EmacsKeysPlugin::gotoNextWord, Tr::tr("Go to Next Word"));
    registerAction(Constants::GOTO_PREVIOUS_WORD,
        &EmacsKeysPlugin::gotoPreviousWord, Tr::tr("Go to Previous Word"));

    registerAction(Constants::MARK,
        &EmacsKeysPlugin::mark, Tr::tr("Mark"));
    registerAction(Constants::EXCHANGE_CURSOR_AND_MARK,
        &EmacsKeysPlugin::exchangeCursorAndMark, Tr::tr("Exchange Cursor and Mark"));
    registerAction(Constants::COPY,
        &EmacsKeysPlugin::copy, Tr::tr("Copy"));
    registerAction(Constants::CUT,
        &EmacsKeysPlugin::cut, Tr::tr("Cut"));
    registerAction(Constants::YANK,
        &EmacsKeysPlugin::yank, Tr::tr("Yank"));

    registerAction(Constants::SCROLL_HALF_DOWN,
        &EmacsKeysPlugin::scrollHalfDown, Tr::tr("Scroll Half Screen Down"));
    registerAction(Constants::SCROLL_HALF_UP,
        &EmacsKeysPlugin::scrollHalfUp, Tr::tr("Scroll Half Screen Up"));
}

void EmacsKeysPlugin::extensionsInitialized()
{
}

void EmacsKeysPlugin::editorAboutToClose(IEditor *editor)
{
    auto w = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!w)
        return;

    if (m_stateMap.contains(w)) {
        delete m_stateMap[w];
        m_stateMap.remove(w);
    }
}

void EmacsKeysPlugin::currentEditorChanged(IEditor *editor)
{
    if (!editor) {
        m_currentEditorWidget = nullptr;
        return;
    }
    m_currentEditorWidget = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!m_currentEditorWidget)
        return;

    if (!m_stateMap.contains(m_currentEditorWidget)) {
        m_stateMap[m_currentEditorWidget] = new EmacsKeysState(m_currentEditorWidget);
    }
    m_currentState = m_stateMap[m_currentEditorWidget];
    m_currentBaseTextEditorWidget =
        qobject_cast<TextEditor::TextEditorWidget*>(editor->widget());
}

void EmacsKeysPlugin::gotoFileStart()         { genericGoto(QTextCursor::Start); }
void EmacsKeysPlugin::gotoFileEnd()           { genericGoto(QTextCursor::End); }
void EmacsKeysPlugin::gotoLineStart()         { genericGoto(QTextCursor::StartOfLine); }
void EmacsKeysPlugin::gotoLineEnd()           { genericGoto(QTextCursor::EndOfLine); }
void EmacsKeysPlugin::gotoNextLine()          { genericGoto(QTextCursor::Down, false); }
void EmacsKeysPlugin::gotoPreviousLine()      { genericGoto(QTextCursor::Up, false); }
void EmacsKeysPlugin::gotoNextCharacter()     { genericGoto(QTextCursor::Right); }
void EmacsKeysPlugin::gotoPreviousCharacter() { genericGoto(QTextCursor::Left); }
void EmacsKeysPlugin::gotoNextWord()          { genericGoto(QTextCursor::NextWord); }
void EmacsKeysPlugin::gotoPreviousWord()      { genericGoto(QTextCursor::PreviousWord); }

void EmacsKeysPlugin::mark()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    if (m_currentState->mark() == cursor.position()) {
        m_currentState->setMark(-1);
    } else {
        cursor.clearSelection();
        m_currentState->setMark(cursor.position());
        m_currentEditorWidget->setTextCursor(cursor);
    }
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::exchangeCursorAndMark()
{
    if (!m_currentEditorWidget)
        return;

    QTextCursor cursor = m_currentEditorWidget->textCursor();
    if (m_currentState->mark() == -1 || m_currentState->mark() == cursor.position())
        return;

    m_currentState->beginOwnAction();
    int position = cursor.position();
    cursor.clearSelection();
    cursor.setPosition(m_currentState->mark(), QTextCursor::KeepAnchor);
    m_currentState->setMark(position);
    m_currentEditorWidget->setTextCursor(cursor);
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::copy()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    QApplication::clipboard()->setText(plainSelectedText(cursor));
    cursor.clearSelection();
    m_currentEditorWidget->setTextCursor(cursor);
    m_currentState->setMark(-1);
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::cut()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    QApplication::clipboard()->setText(plainSelectedText(cursor));
    cursor.removeSelectedText();
    m_currentState->setMark(-1);
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::yank()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    m_currentEditorWidget->paste();
    m_currentState->setMark(-1);
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::scrollHalfDown() { genericVScroll(1); }
void EmacsKeysPlugin::scrollHalfUp() { genericVScroll(-1); }

void EmacsKeysPlugin::deleteCharacter()
{
    if (!m_currentEditorWidget)
        return;
    m_currentState->beginOwnAction();
    m_currentEditorWidget->textCursor().deleteChar();
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::killWord()
{
    if (!m_currentEditorWidget)
        return;
    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    if (m_currentState->lastAction() == KeysActionKillWord) {
        QApplication::clipboard()->setText(
            QApplication::clipboard()->text() + plainSelectedText(cursor));
    } else {
        QApplication::clipboard()->setText(plainSelectedText(cursor));
    }
    cursor.removeSelectedText();
    m_currentState->endOwnAction(KeysActionKillWord);
}

void EmacsKeysPlugin::killLine()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    int position = cursor.position();
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    if (cursor.position() == position) {
        // empty line
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    if (m_currentState->lastAction() == KeysActionKillLine) {
        QApplication::clipboard()->setText(
            QApplication::clipboard()->text() + plainSelectedText(cursor));
    } else {
        QApplication::clipboard()->setText(plainSelectedText(cursor));
    }
    cursor.removeSelectedText();
    m_currentState->endOwnAction(KeysActionKillLine);
}

void EmacsKeysPlugin::insertLineAndIndent()
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    cursor.beginEditBlock();
    cursor.insertBlock();
    if (m_currentBaseTextEditorWidget)
        m_currentBaseTextEditorWidget->textDocument()->autoIndent(cursor);
    cursor.endEditBlock();
    m_currentEditorWidget->setTextCursor(cursor);
    m_currentState->endOwnAction(KeysActionOther);
}

QAction *EmacsKeysPlugin::registerAction(Id id, void (EmacsKeysPlugin::*callback)(),
                                         const QString &title)
{
    auto result = new QAction(title, this);
    ActionManager::registerAction(result, id, Context(Core::Constants::C_GLOBAL), true);
    connect(result, &QAction::triggered, this, callback);
    return result;
}

void EmacsKeysPlugin::genericGoto(QTextCursor::MoveOperation op, bool abortAssist)
{
    if (!m_currentEditorWidget)
        return;
    m_currentState->beginOwnAction();
    QTextCursor cursor = m_currentEditorWidget->textCursor();
    cursor.movePosition(op, m_currentState->mark() != -1 ?
        QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
    m_currentEditorWidget->setTextCursor(cursor);
    if (abortAssist && m_currentBaseTextEditorWidget)
        m_currentBaseTextEditorWidget->abortAssist();
    m_currentState->endOwnAction(KeysActionOther);
}

void EmacsKeysPlugin::genericVScroll(int direction)
{
    if (!m_currentEditorWidget)
        return;

    m_currentState->beginOwnAction();
    QScrollBar *verticalScrollBar = m_currentEditorWidget->verticalScrollBar();
    const int value = verticalScrollBar->value();
    const int halfPageStep = verticalScrollBar->pageStep() / 2;
    const int newValue = value + (direction > 0 ? halfPageStep : -halfPageStep);
    verticalScrollBar->setValue(newValue);

    // adjust cursor if it's out of screen
    const QRect viewportRect = m_currentEditorWidget->viewport()->rect();
    const QTextCursor::MoveMode mode =
        m_currentState->mark() != -1 ?
        QTextCursor::KeepAnchor :
        QTextCursor::MoveAnchor ;
    const QTextCursor::MoveOperation op =
        m_currentEditorWidget->cursorRect().y() < 0 ?
        QTextCursor::Down :
        QTextCursor::Up ;

    QTextCursor cursor = m_currentEditorWidget->textCursor();
    while (!m_currentEditorWidget->cursorRect(cursor).intersects(viewportRect)) {
        const int previousPosition = cursor.position();
        cursor.movePosition(op, mode);
        if (previousPosition == cursor.position())
            break;
    }
    m_currentEditorWidget->setTextCursor(cursor);
    m_currentState->endOwnAction(KeysActionOther);
}

} // namespace Internal
} // namespace EmacsKeys
