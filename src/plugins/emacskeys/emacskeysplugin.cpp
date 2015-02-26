/**************************************************************************
**
** Copyright (c) nsf <no.smile.face@gmail.com>
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

#include "emacskeysplugin.h"
#include "emacskeysconstants.h"
#include "emacskeysstate.h"

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

#include <QtPlugin>

QT_BEGIN_NAMESPACE

extern void qt_set_sequence_auto_mnemonic(bool enable);

QT_END_NAMESPACE

using namespace EmacsKeys::Internal;

//---------------------------------------------------------------------------
// EmacsKeysPlugin
//---------------------------------------------------------------------------

EmacsKeysPlugin::EmacsKeysPlugin(): m_currentEditorWidget(0)
{
}

EmacsKeysPlugin::~EmacsKeysPlugin()
{
}

bool EmacsKeysPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // We have to use this hack here at the moment, because it's the only way to
    // disable Qt Creator menu accelerators aka mnemonics. Many of them get into
    // the way of typical emacs keys, such as: Alt+F (File), Alt+B (Build),
    // Alt+W (Window).
    qt_set_sequence_auto_mnemonic(false);

    connect(Core::EditorManager::instance(),
        SIGNAL(editorAboutToClose(Core::IEditor*)),
        this,
        SLOT(editorAboutToClose(Core::IEditor*)));
    connect(Core::EditorManager::instance(),
        SIGNAL(currentEditorChanged(Core::IEditor*)),
        this,
        SLOT(currentEditorChanged(Core::IEditor*)));

    registerAction(Constants::DELETE_CHARACTER,
        SLOT(deleteCharacter()), tr("Delete Character"));
    registerAction(Constants::KILL_WORD,
        SLOT(killWord()), tr("Kill Word"));
    registerAction(Constants::KILL_LINE,
        SLOT(killLine()), tr("Kill Line"));
    registerAction(Constants::INSERT_LINE_AND_INDENT,
        SLOT(insertLineAndIndent()), tr("Insert New Line and Indent"));

    registerAction(Constants::GOTO_FILE_START,
        SLOT(gotoFileStart()), tr("Go to File Start"));
    registerAction(Constants::GOTO_FILE_END,
        SLOT(gotoFileEnd()), tr("Go to File End"));
    registerAction(Constants::GOTO_LINE_START,
        SLOT(gotoLineStart()), tr("Go to Line Start"));
    registerAction(Constants::GOTO_LINE_END,
        SLOT(gotoLineEnd()), tr("Go to Line End"));
    registerAction(Constants::GOTO_NEXT_LINE,
        SLOT(gotoNextLine()), tr("Go to Next Line"));
    registerAction(Constants::GOTO_PREVIOUS_LINE,
        SLOT(gotoPreviousLine()), tr("Go to Previous Line"));
    registerAction(Constants::GOTO_NEXT_CHARACTER,
        SLOT(gotoNextCharacter()), tr("Go to Next Character"));
    registerAction(Constants::GOTO_PREVIOUS_CHARACTER,
        SLOT(gotoPreviousCharacter()), tr("Go to Previous Character"));
    registerAction(Constants::GOTO_NEXT_WORD,
        SLOT(gotoNextWord()), tr("Go to Next Word"));
    registerAction(Constants::GOTO_PREVIOUS_WORD,
        SLOT(gotoPreviousWord()), tr("Go to Previous Word"));

    registerAction(Constants::MARK,
        SLOT(mark()), tr("Mark"));
    registerAction(Constants::EXCHANGE_CURSOR_AND_MARK,
        SLOT(exchangeCursorAndMark()), tr("Exchange Cursor and Mark"));
    registerAction(Constants::COPY,
        SLOT(copy()), tr("Copy"));
    registerAction(Constants::CUT,
        SLOT(cut()), tr("Cut"));
    registerAction(Constants::YANK,
        SLOT(yank()), tr("Yank"));

    registerAction(Constants::SCROLL_HALF_DOWN,
        SLOT(scrollHalfDown()), tr("Scroll Half Screen Down"));
    registerAction(Constants::SCROLL_HALF_UP,
        SLOT(scrollHalfUp()), tr("Scroll Half Screen Up"));
    return true;
}

void EmacsKeysPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag EmacsKeysPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void EmacsKeysPlugin::editorAboutToClose(Core::IEditor *editor)
{
    QPlainTextEdit *w = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!w)
        return;

    if (m_stateMap.contains(w)) {
        delete m_stateMap[w];
        m_stateMap.remove(w);
    }
}

void EmacsKeysPlugin::currentEditorChanged(Core::IEditor *editor)
{
    if (!editor) {
        m_currentEditorWidget = 0;
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
    QApplication::clipboard()->setText(cursor.selectedText());
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
    QApplication::clipboard()->setText(cursor.selectedText());
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
            QApplication::clipboard()->text() + cursor.selectedText());
    } else {
        QApplication::clipboard()->setText(cursor.selectedText());
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
            QApplication::clipboard()->text() + cursor.selectedText());
    } else {
        QApplication::clipboard()->setText(cursor.selectedText());
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
    if (m_currentBaseTextEditorWidget != 0)
        m_currentBaseTextEditorWidget->textDocument()->autoIndent(cursor);
    cursor.endEditBlock();
    m_currentEditorWidget->setTextCursor(cursor);
    m_currentState->endOwnAction(KeysActionOther);
}

QAction *EmacsKeysPlugin::registerAction(Core::Id id, const char *slot,
    const QString &title)
{
    QAction *result = new QAction(title, this);
    Core::ActionManager::registerAction(result, id,
        Core::Context(Core::Constants::C_GLOBAL), true);

    connect(result, SIGNAL(triggered(bool)), this, slot);
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
    if (abortAssist && m_currentBaseTextEditorWidget != 0)
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
