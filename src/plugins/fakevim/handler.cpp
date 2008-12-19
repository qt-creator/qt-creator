/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "handler.h"

#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QObject>
#include <QPlainTextEdit>
#include <QRegExp>
#include <QStack>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextCursor>


using namespace FakeVim::Internal;

#define StartOfLine QTextCursor::StartOfLine
#define EndOfLine QTextCursor::EndOfLine
#define MoveAnchor QTextCursor::MoveAnchor
#define KeepAnchor QTextCursor::KeepAnchor
#define Up QTextCursor::Up
#define Down QTextCursor::Down
#define Right QTextCursor::Right
#define Left QTextCursor::Left


///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////
const int ParagraphSeparator = 0x00002029;

using namespace Qt;

enum Mode { InsertMode, CommandMode, ExMode };

enum SubMode { NoSubMode, RegisterSubMode, ChangeSubMode, DeleteSubMode };

class FakeVimHandler::Private
{
public:
    Private(FakeVimHandler *parent);

    bool eventFilter(QObject *ob, QEvent *ev);

    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    void handleKey(int key);
    void handleInsertMode(int key);
    void handleCommandMode(int key);
    void handleRegisterMode(int key);
    void handleExMode(int key);
    void finishMovement();
    void updateCommandBuffer();
    void search(const QString &needle, bool backwards);
    void showMessage(const QString &msg);

    int count() const { return m_count.isEmpty() ? 1 : m_count.toInt(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEol() const { return m_tc.atBlockEnd() && m_tc.block().length()>1; }

    FakeVimHandler *q;
    Mode m_mode;
    SubMode m_submode;
    QString m_input;
    QPlainTextEdit *m_editor;
    QTextCursor m_tc;
    QHash<int, QString> m_registers;
    int m_register;
    QString m_count;

    QStack<QString> m_undoStack;
    QStack<QString> m_redoStack;

    bool m_fakeEnd;

    int m_commandCode; // ?, /, : ...
    QString m_commandBuffer;

    bool m_lastSearchBackward;
    QString m_lastSearchString;
};

FakeVimHandler::Private::Private(FakeVimHandler *parent)
{
    q = parent;
    m_mode = CommandMode;
    m_fakeEnd = false;
    m_lastSearchBackward = false;
    m_register = '"';
}

bool FakeVimHandler::Private::eventFilter(QObject *ob, QEvent *ev)
{
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
    int key = keyEvent->key();
    if (key == Key_Shift || key == Key_Alt || key == Key_Control
        || key == Key_Alt || key == Key_AltGr || key == Key_Meta)
        return false;
    //qDebug() << "KEY: " << key << Qt::ShiftModifier;

    // Fake "End of line"
    m_editor = qobject_cast<QPlainTextEdit *>(ob);
    if (!m_editor)
        return false;

    m_tc = m_editor->textCursor();
    if (m_fakeEnd) {
        //m_fakeEnd = false;
        m_tc.movePosition(Right, MoveAnchor, 1);
        qDebug() << "Unfake EOL";
    }

    if (key >= Key_A && key <= Key_Z
        && (keyEvent->modifiers() & Qt::ShiftModifier) == 0)
        key += 32;
    if ((keyEvent->modifiers() & Qt::ControlModifier) != 0)
        key += 256;
    handleKey(key);

    // We fake vi-style end-of-line behaviour
    m_fakeEnd = atEol() && m_mode == CommandMode;

    //qDebug() << "POS: " <<  m_tc.position()
    //    << "  BLOCK LEN: " << m_tc.block().length()
    //    << "  LEFT: " << leftDist() << " RIGHT: " << rightDist();

    if (m_fakeEnd) {
        m_tc.movePosition(Left, MoveAnchor, 1);
        qDebug() << "Fake EOL";
    }

    //qDebug() << "POS: " <<  m_tc.position()
    //    << "  BLOCK LEN: " << m_tc.block().length()
    //    << "  LEFT: " << leftDist() << " RIGHT: " << rightDist();

    m_editor->setTextCursor(m_tc);
    m_editor->ensureCursorVisible();
    return true;
}

void FakeVimHandler::Private::handleKey(int key)
{
    if (m_mode == InsertMode)
        handleInsertMode(key);
    else if (m_mode == CommandMode)
        handleCommandMode(key);
    else if (m_mode == ExMode)
        handleExMode(key);
}

void FakeVimHandler::Private::finishMovement()
{
    if (m_submode == ChangeSubMode) {
        m_registers[m_register] = m_tc.selectedText();
        m_tc.removeSelectedText();
        m_mode = InsertMode;
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        m_registers[m_register] = m_tc.selectedText();
        m_tc.removeSelectedText();
        m_submode = NoSubMode;
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
    }
    m_count.clear();
    m_register = '"';
    m_tc.clearSelection();
    updateCommandBuffer();
}

void FakeVimHandler::Private::updateCommandBuffer()
{
    //qDebug() << "CMD" << m_commandBuffer;
    QString msg = QChar(m_commandCode ? m_commandCode : ' ') + m_commandBuffer;
    emit q->commandBufferChanged(msg);
}

void FakeVimHandler::Private::showMessage(const QString &msg)
{
    emit q->commandBufferChanged(msg);
}

void FakeVimHandler::Private::handleCommandMode(int key)
{
    //qDebug() << "-> MODE: " << m_mode << " KEY: " << key;
    if (m_submode == RegisterSubMode) {
        m_register = key;
        m_submode = NoSubMode;
    } else if (m_submode == ChangeSubMode && key == 'c') {
        m_tc.movePosition(StartOfLine, MoveAnchor);
        m_tc.movePosition(Down, KeepAnchor, count());
        m_registers[m_register] = m_tc.selectedText();
        finishMovement();
    } else if (m_submode == DeleteSubMode && key == 'd') {
        m_tc.movePosition(StartOfLine, MoveAnchor);
        m_tc.movePosition(Down, KeepAnchor, count());
        m_registers[m_register] = m_tc.selectedText();
        finishMovement();
    } else if (key >= '0' && key <= '9') {
        if (key == '0' && m_count.isEmpty()) {
            m_tc.movePosition(StartOfLine, KeepAnchor);
            finishMovement();
        } else {
            m_count.append(QChar(key));
        }
    } else if (key == ':' || key == '/' || key == '?') {
        m_commandCode = key;
        m_mode = ExMode;
        updateCommandBuffer();
    } else if (key == '|') {
        m_tc.movePosition(StartOfLine, KeepAnchor);
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()) - 1);
        finishMovement();
    } else if (key == '"') {
        m_submode = RegisterSubMode;
    } else if (key == Key_Return) {
        m_tc.movePosition(StartOfLine);
        m_tc.movePosition(Down);
    } else if (key == Key_Home) {
        m_tc.movePosition(StartOfLine, KeepAnchor);
        finishMovement();
    } else if (key == '$' || key == Key_End) {
        m_tc.movePosition(EndOfLine, KeepAnchor);
        finishMovement();
    } else if (key == 'A') {
        m_tc.movePosition(EndOfLine, MoveAnchor);
        m_mode = InsertMode;
    } else if (key == 'c') {
        m_submode = ChangeSubMode;
    } else if (key == 'C') {
        m_submode = ChangeSubMode;
        m_tc.movePosition(EndOfLine, KeepAnchor);
        finishMovement();
    } else if (key == 'd') {
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
        m_submode = DeleteSubMode;
    } else if (key == 'D') {
        m_submode = DeleteSubMode;
        m_tc.movePosition(EndOfLine, KeepAnchor);
        finishMovement();
    } else if (key == 'h' || key == Key_Left) {
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        m_tc.movePosition(Left, KeepAnchor, n);
        finishMovement();
    } else if (key == 'i') {
        m_mode = InsertMode;
    } else if (key == 'j' || key == Key_Down) {
        m_tc.movePosition(Down, KeepAnchor, count());
        finishMovement();
    } else if (key == 'k' || key == Key_Up) {
        m_tc.movePosition(Up, KeepAnchor, count());
        finishMovement();
    } else if (key == 'l' || key == Key_Right) {
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        finishMovement();
    } else if (key == 'n') {
        search(m_lastSearchString, m_lastSearchBackward);
    } else if (key == 'N') {
        search(m_lastSearchString, !m_lastSearchBackward);
    } else if (key == 'p') {
        QString text = m_registers[m_register];
        int n = text.count(QChar(ParagraphSeparator));
        if (n > 0) {
            m_tc.movePosition(Down);
            m_tc.movePosition(StartOfLine);
            m_tc.insertText(text);
            m_tc.movePosition(Up, MoveAnchor, n);
        } else {
            m_tc.movePosition(Right);
            m_tc.insertText(text);
            m_tc.movePosition(Left);
        }
    } else if (key == control('r')) {
        m_editor->redo();
    } else if (key == 'u') {
        m_editor->undo();
    } else if (key == 'x') { // = "dl"
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
        m_submode = DeleteSubMode;
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        finishMovement();
    } else if (key == 'X') {
        if (leftDist() > 0) {
            m_tc.movePosition(Left, KeepAnchor, qMin(count(), leftDist()));
            m_tc.deleteChar();
        }
        finishMovement();
    } else if (key == Key_Backspace) {
        m_tc.deletePreviousChar();
    } else {
        qDebug() << "Ignored" << key;
    }    
}

void FakeVimHandler::Private::handleInsertMode(int key)
{
    if (key == Key_Escape) {
        m_mode = CommandMode;
        m_tc.movePosition(Left, KeepAnchor, qMin(1, leftDist()));
    } else if (key == Key_Left) {
        m_tc.movePosition(Left, MoveAnchor, 1);
    } else if (key == Key_Down) {
        m_tc.movePosition(Down, MoveAnchor, 1);
    } else if (key == Key_Up) {
        m_tc.movePosition(Up, MoveAnchor, 1);
    } else if (key == Key_Right) {
        m_tc.movePosition(Right, MoveAnchor, 1);
    } else if (key == Key_Return) {
        m_tc.insertBlock();
    } else if (key == Key_Backspace) {
        m_tc.deletePreviousChar();
    } else if (key == Key_Tab) {
        m_tc.insertText(QChar(9));
    } else {
        m_tc.insertText(QChar(key));
    }    
}

void FakeVimHandler::Private::handleExMode(int key)
{
    if (key == Key_Escape) {
        m_commandBuffer.clear();
        m_commandCode = 0;
        m_mode = CommandMode;
    } else if (key == Key_Backspace) {
        if (m_commandBuffer.isEmpty())
            m_mode = CommandMode;
        else
            m_commandBuffer.chop(1);
    } else if (key == Key_Return && m_commandCode == ':') {
        static QRegExp reGoto("^(\\d+)$");
        if (reGoto.indexIn(m_commandBuffer) != -1) {
            int n = reGoto.cap(1).toInt();
            m_tc.setPosition(m_tc.block().document()
                ->findBlockByNumber(n - 1).position());
        } else if (m_commandBuffer == "q!" || m_commandBuffer == "q") {
            q->quitRequested(m_editor);
        }
        m_commandBuffer.clear();
        m_mode = CommandMode;
    } else if (key == Key_Return
            && (m_commandCode == '/' || m_commandCode == '?')) {
        m_lastSearchBackward = (m_commandCode == '?');
        m_lastSearchString = m_commandBuffer;
        search(m_lastSearchString, m_lastSearchBackward);
        m_commandBuffer.clear();
        m_commandCode = 0;
        m_mode = CommandMode;
    } else {
        m_commandBuffer += QChar(key);
    }
    updateCommandBuffer();
}

void FakeVimHandler::Private::search(const QString &needle, bool backwards)
{
    //qDebug() << "NEEDLE " << needle << "BACKWARDS" << backwards;
    //int startPos = m_tc.position();
    QTextDocument::FindFlags flags;
    if (backwards)
        flags = QTextDocument::FindBackward;

    m_tc.movePosition(backwards? Left : Right, MoveAnchor, 1);
    m_editor->setTextCursor(m_tc);
    if (m_editor->find(needle, flags)) {
        m_tc = m_editor->textCursor();
        m_tc.movePosition(Left, MoveAnchor, needle.size());
        return;
    }

    int n = 0;
    if (backwards) {
        QTextBlock block = m_tc.block().document()->lastBlock();
        n = block.position() + block.length() - 1;
    }
    m_tc.setPosition(n);
    m_editor->setTextCursor(m_tc);
    if (m_editor->find(needle, flags)) {
        m_tc = m_editor->textCursor();
        m_tc.movePosition(Left, MoveAnchor, needle.size());
        if (backwards)
            showMessage("search hit TOP, continuing at BOTTOM");
        else
            showMessage("search hit BOTTOM, continuing at TOP");
        return;
    }

    m_tc.movePosition(backwards ? Right : Left, MoveAnchor, 1);
}


///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////

FakeVimHandler::FakeVimHandler(QObject *parent)
    : QObject(parent), d(new Private(this))
{}

FakeVimHandler::~FakeVimHandler()
{
    delete d;
}

bool FakeVimHandler::eventFilter(QObject *ob, QEvent *ev)
{
    if (ev->type() != QEvent::KeyPress)
        return QObject::eventFilter(ob, ev);
    return d->eventFilter(ob, ev);
}

