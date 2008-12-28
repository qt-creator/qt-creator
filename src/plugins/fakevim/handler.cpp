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

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QStack>

#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextEdit>


using namespace FakeVim::Internal;

#define StartOfLine QTextCursor::StartOfLine
#define EndOfLine   QTextCursor::EndOfLine
#define MoveAnchor  QTextCursor::MoveAnchor
#define KeepAnchor  QTextCursor::KeepAnchor
#define Up          QTextCursor::Up
#define Down        QTextCursor::Down
#define Right       QTextCursor::Right
#define Left        QTextCursor::Left


///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////

#define EDITOR(s) (m_textedit ? m_textedit->s : m_plaintextedit->s)
#define THE_EDITOR (m_textedit ? (QWidget*)m_textedit : (QWidget*)m_plaintextedit)

const int ParagraphSeparator = 0x00002029;

using namespace Qt;

enum Mode
{
    InsertMode,
    CommandMode,
    ExMode
};

enum SubMode
{
    NoSubMode,
    RegisterSubMode,
    ChangeSubMode,
    DeleteSubMode,
    ZSubMode
};

enum SubSubMode
{
    NoSubSubMode,
    FtSubSubMode,       // used for f, F, t, T
    MarkSubSubMode,     // used for m
    BackTickSubSubMode, // used for ` 
    TickSubSubMode      // used for '
};

static const QString ConfigStartOfLine = "startofline";
static const QString ConfigOn = "on";

class FakeVimHandler::Private
{
public:
    Private(FakeVimHandler *parent);

    bool eventFilter(QObject *ob, QEvent *ev);

    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    void handleKey(int key, const QString &text);
    void handleInsertMode(int key, const QString &text);
    void handleCommandMode(int key, const QString &text);
    void handleRegisterMode(int key, const QString &text);
    void handleExMode(int key, const QString &text);
    void finishMovement();
    void updateMiniBuffer();
    void search(const QString &needle, bool forward);
    void showMessage(const QString &msg);

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEol() const { return m_tc.atBlockEnd() && m_tc.block().length()>1; }

    int lastPositionInDocument() const;

	// all zero-based counting
	int cursorLineOnScreen() const;
	int linesOnScreen() const;
	int columnsOnScreen() const;
	int cursorLineInDocument() const;
	int cursorColumnInDocument() const;
    int linesInDocument() const;
	void scrollToLineInDocument(int line);

    void moveToFirstNonBlankOnLine();
    void moveToNextWord(bool simple);
    void moveToWordBoundary(bool simple, bool forward);
    void handleFfTt(int key);
    void handleCommand(const QString &cmd);

    // helper function for handleCommand. return 1 based line index.
    int readLineCode(QString &cmd);

    FakeVimHandler *q;
    Mode m_mode;
    SubMode m_submode;
    SubSubMode m_subsubmode;
    int m_subsubdata;
    QString m_input;
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    QTextCursor m_tc;
    QHash<int, QString> m_registers;
    int m_register;
    QString m_mvcount;
    QString m_opcount;

    QStack<QString> m_undoStack;
    QStack<QString> m_redoStack;

    bool m_fakeEnd;

    bool isSearchCommand() const
        { return m_commandCode == '?' || m_commandCode == '/'; }
    int m_commandCode; // ?, /, : ...
    int m_gflag;  // whether current command started with 'g'
    
    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    QString m_lastInsertion;

    // History for '/'
    QString lastSearchString() const;
    QStringList m_searchHistory;
    int m_searchHistoryIndex;

    // History for ':'
    QStringList m_commandHistory;
    int m_commandHistoryIndex;

    // marks as lines
    QHash<int, int> m_marks;

    // vi style configuration
    QHash<QString, QString> m_config;
};

FakeVimHandler::Private::Private(FakeVimHandler *parent)
{
    q = parent;

    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_commandCode = 0;
    m_fakeEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_textedit = 0;
    m_plaintextedit = 0;

    m_config[ConfigStartOfLine] = ConfigOn;
}

bool FakeVimHandler::Private::eventFilter(QObject *ob, QEvent *ev)
{
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
    int key = keyEvent->key();
    if (key == Key_Shift || key == Key_Alt || key == Key_Control
        || key == Key_Alt || key == Key_AltGr || key == Key_Meta)
        return false;

    // Fake "End of line"
    m_textedit = qobject_cast<QTextEdit *>(ob);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(ob);
    if (!m_textedit && !m_plaintextedit)
        return false;

    m_tc = EDITOR(textCursor());

    if (m_fakeEnd)
        m_tc.movePosition(Right, MoveAnchor, 1);

    if (key >= Key_A && key <= Key_Z
        && (keyEvent->modifiers() & Qt::ShiftModifier) == 0)
        key += 32;
    if ((keyEvent->modifiers() & Qt::ControlModifier) != 0)
        key += 256;
    handleKey(key, keyEvent->text());

    // We fake vi-style end-of-line behaviour
    m_fakeEnd = (atEol() && m_mode == CommandMode);

    if (m_fakeEnd)
        m_tc.movePosition(Left, MoveAnchor, 1);

    EDITOR(setTextCursor(m_tc));
    EDITOR(ensureCursorVisible());
    return true;
}

void FakeVimHandler::Private::handleKey(int key, const QString &text)
{
    if (m_mode == InsertMode)
        handleInsertMode(key, text);
    else if (m_mode == CommandMode)
        handleCommandMode(key, text);
    else if (m_mode == ExMode)
        handleExMode(key, text);
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
    m_mvcount.clear();
    m_opcount.clear();
    m_gflag = false;
    m_register = '"';
    m_tc.clearSelection();
    updateMiniBuffer();
}

void FakeVimHandler::Private::updateMiniBuffer()
{
    if (m_tc.isNull())
        return;
    QString msg;
    if (m_currentMessage.isEmpty()) {
        msg = QChar(m_commandCode ? m_commandCode : ' ');
        for (int i = 0; i != m_commandBuffer.size(); ++i) {
            QChar c = m_commandBuffer.at(i);
            if (c.unicode() < 32) {
                msg += '^';
                msg += QChar(c.unicode() + 64);
            } else {
                msg += c;
            }
        }
    } else {
        msg = m_currentMessage;
        m_currentMessage.clear();
    }
    int l = cursorLineInDocument();
    int w = columnsOnScreen();
    msg += QString(w, ' ');
    msg = msg.left(w - 20);
    QString pos = tr("%1,%2").arg(l + 1).arg(cursorColumnInDocument() + 1);
    msg += tr("%1").arg(pos, -12);
    // FIXME: physical "-" logical
    msg += tr("%1").arg(l * 100 / (m_tc.document()->blockCount() - 1), 4);
    msg += '%';
    emit q->commandBufferChanged(msg);
}

void FakeVimHandler::Private::showMessage(const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    m_currentMessage = msg;
    updateMiniBuffer();
}

void FakeVimHandler::Private::handleCommandMode(int key, const QString &text)
{
    Q_UNUSED(text)

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
    } else if (m_submode == ZSubMode) {
        if (key == Key_Return) {
            // cursor line to top of window, cursor on first non-blank
			scrollToLineInDocument(cursorLineInDocument());
            moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            qDebug() << "Ignored z + " << key << text;
        }
        m_submode = NoSubMode;
    } else if (m_subsubmode == FtSubSubMode) {
        handleFfTt(key);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (m_subsubmode == MarkSubSubMode) {
        m_marks[key] = m_tc.position();
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        if (m_marks.contains(key)) {
            m_tc.setPosition(m_marks[key]);
            if (m_subsubmode == TickSubSubMode)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            showMessage(tr("E20: Mark '%1' not set").arg(text));
        }
        m_subsubmode = NoSubSubMode;
    } else if (key >= '0' && key <= '9') {
        if (key == '0' && m_mvcount.isEmpty()) {
            moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            m_mvcount.append(QChar(key));
        }
    } else if (key == ':' || key == '/' || key == '?') {
        m_commandCode = key;
        m_mode = ExMode;
        if (isSearchCommand()) {
            m_searchHistory.append(QString());
            m_searchHistoryIndex = m_searchHistory.size() - 1;
        } else {
            m_commandHistory.append(QString());
            m_commandHistoryIndex = m_commandHistory.size() - 1;
        }
        updateMiniBuffer();
    } else if (key == '`') {
        m_subsubmode = BackTickSubSubMode;
    } else if (key == '\'') {
        m_subsubmode = TickSubSubMode;
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
    } else if (key == 'a') {
        m_mode = InsertMode;
        m_lastInsertion.clear();
        m_tc.beginEditBlock();
        m_tc.movePosition(Right, MoveAnchor, 1);
    } else if (key == 'A') {
        m_mode = InsertMode;
        m_tc.beginEditBlock();
        m_tc.movePosition(EndOfLine, MoveAnchor);
        m_lastInsertion.clear();
    } else if (key == 'b') {
        moveToWordBoundary(false, false);
        finishMovement();
    } else if (key == 'B') {
        moveToWordBoundary(true, false);
        finishMovement();
    } else if (key == 'c') {
        m_submode = ChangeSubMode;
        m_tc.beginEditBlock();
    } else if (key == 'C') {
        m_submode = ChangeSubMode;
        m_tc.beginEditBlock();
        m_tc.movePosition(EndOfLine, KeepAnchor);
        finishMovement();
    } else if (key == 'd') {
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if (key == 'D') {
        m_submode = DeleteSubMode;
        m_tc.movePosition(Down, KeepAnchor, qMax(count() - 1, 0));
        m_tc.movePosition(Right, KeepAnchor, rightDist());
        finishMovement();
    } else if (key == 'e') {
        moveToWordBoundary(false, true);
        finishMovement();
    } else if (key == 'E') {
        moveToWordBoundary(true, true);
        finishMovement();
    } else if (key == 'f' || key == 'F') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'g') {
        m_gflag = true;
    } else if (key == 'G') {
        int n = m_mvcount.isEmpty() ? linesInDocument() : count();
        m_tc.setPosition(m_tc.document()->findBlockByNumber(n - 1).position());
        if (m_config.contains(ConfigStartOfLine))
            moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'h' || key == Key_Left) {
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        m_tc.movePosition(Left, KeepAnchor, n);
        finishMovement();
    } else if (key == 'H') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, 0)));
        m_tc.movePosition(Down, KeepAnchor, qMax(count() - 1, 0));
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'i') {
        m_mode = InsertMode;
        m_tc.beginEditBlock();
        m_lastInsertion.clear();
    } else if (key == 'I') {
        m_mode = InsertMode;
        if (m_gflag)
            m_tc.movePosition(StartOfLine, KeepAnchor);
        else
            moveToFirstNonBlankOnLine();
        m_tc.clearSelection();
        m_tc.beginEditBlock();
        m_lastInsertion.clear();
    } else if (key == 'j' || key == Key_Down) {
        m_tc.movePosition(Down, KeepAnchor, count());
        finishMovement();
    } else if (key == 'J') {
        m_tc.beginEditBlock();
        if (m_submode == NoSubMode) {
            for (int i = qMax(count(), 2) - 1; --i >= 0; ) {
                m_tc.movePosition(EndOfLine);
                m_tc.deleteChar();
                if (!m_gflag)
                    m_tc.insertText(" ");
            }
            if (!m_gflag)
                m_tc.movePosition(Left, MoveAnchor, 1);
        }
        m_tc.endEditBlock();
    } else if (key == 'k' || key == Key_Up) {
        m_tc.movePosition(Up, KeepAnchor, count());
        finishMovement();
    } else if (key == 'l' || key == Key_Right) {
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        finishMovement();
    } else if (key == 'L') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        m_tc.movePosition(Up, KeepAnchor, qMax(count(), 1));
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'm') {
        m_subsubmode = MarkSubSubMode;
    } else if (key == 'M') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'n') {
        search(lastSearchString(), m_lastSearchForward);
    } else if (key == 'N') {
        search(lastSearchString(), !m_lastSearchForward);
    } else if (key == 'o') {
        m_mode = InsertMode;
        m_lastInsertion.clear();
        //m_tc.beginEditBlock();  // FIXME: unusable due to drawing errors
        m_tc.movePosition(EndOfLine, MoveAnchor);
        m_tc.insertText("\n");
    } else if (key == 'O') {
        m_mode = InsertMode;
        m_lastInsertion.clear();
        //m_tc.beginEditBlock();  // FIXME: unusable due to drawing errors
        m_tc.movePosition(StartOfLine, MoveAnchor);
        m_tc.movePosition(Left, MoveAnchor, 1);
        m_tc.insertText("\n");
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
        EDITOR(redo());
    } else if (key == 's') {
        m_submode = ChangeSubMode;
        m_tc.beginEditBlock();
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        finishMovement();
    } else if (key == 't' || key == 'T') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'u') {
        EDITOR(undo());
    } else if (key == 'w') {
        moveToNextWord(false);
        finishMovement();
    } else if (key == 'W') {
        moveToNextWord(true);
        finishMovement();
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
    } else if (key == 'z') {
        m_submode = ZSubMode;
    } else if (key == '~' && !atEol()) {
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        QString str = m_tc.selectedText();
        for (int i = str.size(); --i >= 0; ) {
            QChar c = str.at(i);
            str[i] = c.isUpper() ? c.toLower() : c.toUpper();
        }
        m_tc.deleteChar();
        m_tc.insertText(str);
    } else if (key == Key_PageDown || key == control('f')) {
        m_tc.movePosition(Down, KeepAnchor, count() * (linesOnScreen() - 2));
        finishMovement();
    } else if (key == Key_PageUp || key == control('b')) {
        m_tc.movePosition(Up, KeepAnchor, count() * (linesOnScreen() - 2));
        finishMovement();
    } else if (key == Key_Backspace) {
        m_tc.deletePreviousChar();
    } else if (key == Key_Delete) {
        m_tc.deleteChar();
    } else if (key == Key_Escape) {
        // harmless
    } else {
        qDebug() << "Ignored" << key << text;
    }    
}

void FakeVimHandler::Private::handleInsertMode(int key, const QString &text)
{
    if (key == Key_Escape) {
        for (int i = 1; i < count(); ++i)
            m_tc.insertText(m_lastInsertion);
        m_tc.movePosition(Left, MoveAnchor, qMin(1, leftDist()));
        m_mode = CommandMode;
        m_tc.endEditBlock();
    } else if (key == Key_Left) {
        m_tc.movePosition(Left, MoveAnchor, 1);
        m_lastInsertion.clear();
    } else if (key == Key_Down) {
        m_tc.movePosition(Down, MoveAnchor, 1);
        m_lastInsertion.clear();
    } else if (key == Key_Up) {
        m_tc.movePosition(Up, MoveAnchor, 1);
        m_lastInsertion.clear();
    } else if (key == Key_Right) {
        m_tc.movePosition(Right, MoveAnchor, 1);
        m_lastInsertion.clear();
    } else if (key == Key_Return) {
        m_tc.insertBlock();
        m_lastInsertion.clear();
    } else if (key == Key_Backspace) {
        m_tc.deletePreviousChar();
        m_lastInsertion = m_lastInsertion.left(m_lastInsertion.size() - 1);
    } else if (key == Key_Delete) {
        m_tc.deleteChar();
        m_lastInsertion.clear();
    } else if (key == Key_PageDown || key == control('f')) {
        m_tc.movePosition(Down, KeepAnchor, count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_Backspace) {
        finishMovement();
    } else if (key == Key_PageUp || key == control('b')) {
        m_tc.movePosition(Up, KeepAnchor, count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_Backspace) {
        finishMovement();
    } else {
        m_lastInsertion.append(text);
        m_tc.insertText(text);
    }    
}

void FakeVimHandler::Private::handleExMode(int key, const QString &text)
{
    Q_UNUSED(text)

    if (key == Key_Escape) {
        m_commandBuffer.clear();
        m_commandCode = 0;
        m_mode = CommandMode;
        updateMiniBuffer();
    } else if (key == Key_Backspace) {
        if (m_commandBuffer.isEmpty())
            m_mode = CommandMode;
        else
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (key == Key_Return && m_commandCode == ':') {
        if (!m_commandBuffer.isEmpty()) {
            m_commandHistory.takeLast();
            m_commandHistory.append(m_commandBuffer);
            handleCommand(m_commandBuffer);
            m_commandBuffer.clear();
            m_commandCode = 0;
        }
        m_mode = CommandMode;
    } else if (key == Key_Return && isSearchCommand()) {
        if (!m_commandBuffer.isEmpty()) {
            m_searchHistory.takeLast();
            m_searchHistory.append(m_commandBuffer);
            m_lastSearchForward = (m_commandCode == '/');
            search(lastSearchString(), m_lastSearchForward);
            m_commandBuffer.clear();
            m_commandCode = 0;
        }
        m_mode = CommandMode;
        updateMiniBuffer();
    } else if (key == Key_Up && isSearchCommand()) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entires in the history into account.
        if (m_searchHistoryIndex > 0) {
            --m_searchHistoryIndex;
            m_commandBuffer = m_searchHistory.at(m_searchHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Up && m_commandCode == ':') {
        if (m_commandHistoryIndex > 0) {
            --m_commandHistoryIndex;
            m_commandBuffer = m_commandHistory.at(m_commandHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Down && isSearchCommand()) {
        if (m_searchHistoryIndex < m_searchHistory.size() - 1) {
            ++m_searchHistoryIndex;
            m_commandBuffer = m_searchHistory.at(m_searchHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Down && m_commandCode == ':') {
        if (m_commandHistoryIndex < m_commandHistory.size() - 1) {
            ++m_commandHistoryIndex;
            m_commandBuffer = m_commandHistory.at(m_commandHistoryIndex); 
            updateMiniBuffer();
        }
    } else if (key == Key_Tab) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else {
        m_commandBuffer += QChar(key);
        updateMiniBuffer();
    }
}

// 1 based.
int FakeVimHandler::Private::readLineCode(QString &cmd)
{
    //qDebug() << "CMD: " << cmd;
    if (cmd.isEmpty())
        return -1;
    QChar c = cmd.at(0);
    cmd = cmd.mid(1);
    if (c == '.')
        return cursorLineInDocument() + 1;
    if (c == '$')
        return linesInDocument();
    if (c == '-') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 - (n == -1 ? 1 : n);
    }
    if (c == '+') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 + (n == -1 ? 1 : n);
    }
    if (c.isDigit()) {
        int n = c.unicode() - '0';
        while (!cmd.isEmpty()) {
            c = cmd.at(0);
            if (!c.isDigit())
                break;
            cmd = cmd.mid(1);
            n = n * 10 + (c.unicode() - '0');
        }
        //qDebug() << "N: " << n;
        return n;
    }
    // not parsed
    cmd = c + cmd;
    return -1; 
}

void FakeVimHandler::Private::handleCommand(const QString &cmd0)
{
    QString cmd = cmd0;
    if (cmd.startsWith("%"))
        cmd = "1,$" + cmd.mid(1);

    int beginLine = 0;
    int endLine = linesInDocument();

    int line = readLineCode(cmd);
    if (line != -1)
        beginLine = line;
    
    if (cmd.startsWith(',')) {
        cmd = cmd.mid(1);
        line = readLineCode(cmd);
        if (line != -1)
            endLine = line;
    }

    //qDebug() << "RANGE: " << beginLine << endLine << cmd << cmd0;

    static QRegExp reWrite("^w!?( (.*))?$");

    if (cmd.isEmpty()) {
        m_tc.setPosition(m_tc.block().document()
            ->findBlockByNumber(beginLine - 1).position());
        showMessage(QString());
    } else if (cmd == "q!" || cmd == "q") {
        q->quitRequested(THE_EDITOR);
        showMessage(QString());
    } else if (reWrite.indexIn(cmd) != -1) {
        bool forced = cmd.startsWith("w!");
        QString fileName = reWrite.cap(2);
        if (fileName.isEmpty())
            fileName = m_currentFileName;
        QFile file(fileName);
        bool exists = file.exists();
        if (exists && !forced) {
            showMessage("E13: File exists (add ! to override)");
        } else {
            file.open(QIODevice::ReadWrite);
            QTextStream ts(&file);
            ts << EDITOR(toPlainText());
            file.close();
            file.open(QIODevice::ReadWrite);
            QByteArray ba = file.readAll();
            showMessage(tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(exists ? " " : " [New] ")
                .arg(ba.count('\n')).arg(ba.size()));
        }
    } else if (cmd.startsWith("r ")) {
        m_currentFileName = cmd.mid(2);
        QFile file(m_currentFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        EDITOR(setPlainText(ts.readAll()));
        showMessage(QString());
    } else {
        showMessage("E492: Not an editor command: " + cmd0);
    }
}

void FakeVimHandler::Private::search(const QString &needle, bool forward)
{
    //qDebug() << "NEEDLE " << needle << "BACKWARDS" << backwards;
    QTextCursor orig = m_tc;
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (!forward)
        flags = QTextDocument::FindBackward;

    if (forward)
        m_tc.movePosition(Right, MoveAnchor, 1);

    EDITOR(setTextCursor(m_tc));
    if (EDITOR(find(needle, flags))) {
        m_tc = EDITOR(textCursor());
        // the qMax seems to be needed for QPlainTextEdit only
        m_tc.movePosition(Left, MoveAnchor, qMax(1, needle.size() - 1));
        return;
    }

    m_tc.setPosition(forward ? 0 : lastPositionInDocument() - 1);
    EDITOR(setTextCursor(m_tc));
    if (EDITOR(find(needle, flags))) {
        m_tc = EDITOR(textCursor());
        // the qMax seems to be needed for QPlainTextEdit only
        m_tc.movePosition(Left, MoveAnchor, qMax(1, needle.size() - 1));
        if (forward)
            showMessage("search hit BOTTOM, continuing at TOP");
        else
            showMessage("search hit TOP, continuing at BOTTOM");
        return;
    }

    m_tc = orig;
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine()
{
    QTextBlock block = m_tc.block();
    QTextDocument *doc = m_tc.document();
    m_tc.movePosition(StartOfLine);
    int firstPos = m_tc.position();
    for (int i = firstPos, n = firstPos + block.length(); i < n; ++i) {
        if (!doc->characterAt(i).isSpace()) {
            m_tc.setPosition(i, KeepAnchor);
            return;
        }
    }
}

static int charClass(QChar c, bool simple)
{
    if (simple)
        return c.isSpace() ? 0 : 1;
    if (c.isLetterOrNumber() || c.unicode() == '_')
        return 2;
    return c.isSpace() ? 0 : 1;
}

void FakeVimHandler::Private::moveToWordBoundary(bool simple, bool forward)
{
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    int n = forward ? lastPositionInDocument() - 1 : 0;
    int lastClass = 0;
    while (true) {
        m_tc.movePosition(forward ? Right : Left, KeepAnchor, 1);
        QChar c = doc->characterAt(m_tc.position());
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && lastClass != 0)
            --repeat;
        if (repeat == -1) {
            m_tc.movePosition(forward ? Left : Right, KeepAnchor, 1);
            break;
        }
        lastClass = thisClass;
        if (m_tc.position() == n)
            break;
    }
}

void FakeVimHandler::Private::handleFfTt(int key)
{
    // m_subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = m_subsubdata == 'f' || m_subsubdata == 't';
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    QTextBlock block = m_tc.block();
    int n = block.position();
    if (forward)
        n += block.length();
    int pos = m_tc.position();
    while (true) {
        pos += forward ? 1 : -1;
        if (pos == n)
            break;
        int uc = doc->characterAt(pos).unicode();
        if (uc == ParagraphSeparator)
            break;
        if (uc == key)
            --repeat;
        if (repeat == 0) {
            if (m_subsubdata == 't')
                --pos;
            else if (m_subsubdata == 'T')
                ++pos;
            // FIXME: strange correction...
            if (m_submode == DeleteSubMode && m_subsubdata == 'f')
                ++pos;
            if (m_submode == DeleteSubMode && m_subsubdata == 't')
                ++pos;

            if (forward)
                m_tc.movePosition(Right, KeepAnchor, pos - m_tc.position());
            else
                m_tc.movePosition(Left, KeepAnchor, m_tc.position() - pos);
            break;
        }
    }
}

void FakeVimHandler::Private::moveToNextWord(bool simple)
{
    // FIXME: 'w' should stop on empty lines, too
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    int n = lastPositionInDocument() - 1;
    QChar c = doc->characterAt(m_tc.position());
    int lastClass = charClass(c, simple);
    while (true) {
        c = doc->characterAt(m_tc.position());
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && thisClass != 0)
            --repeat;
        if (repeat == 0)
            break;
        lastClass = thisClass;
        m_tc.movePosition(Right, KeepAnchor, 1);
        if (m_tc.position() == n)
            break;
    }
}

int FakeVimHandler::Private::cursorLineOnScreen() const
{
	QRect rect = EDITOR(cursorRect());
	return rect.y() / rect.height();
}

int FakeVimHandler::Private::linesOnScreen() const
{
	QRect rect = EDITOR(cursorRect());
	return EDITOR(height()) / rect.height();
}

int FakeVimHandler::Private::columnsOnScreen() const
{
	QRect rect = EDITOR(cursorRect());
	return EDITOR(width()) / rect.width();
}

int FakeVimHandler::Private::cursorLineInDocument() const
{
	return m_tc.block().blockNumber();
}

int FakeVimHandler::Private::cursorColumnInDocument() const
{
	return m_tc.position() - m_tc.block().position();
}

int FakeVimHandler::Private::linesInDocument() const
{
    return m_tc.isNull() ? 0 : m_tc.document()->blockCount();
}

void FakeVimHandler::Private::scrollToLineInDocument(int line)
{
	// FIXME: works only for QPlainTextEdit
	QScrollBar *scrollBar = EDITOR(verticalScrollBar());
	scrollBar->setValue(line);
}

int FakeVimHandler::Private::lastPositionInDocument() const
{
    QTextBlock block = m_tc.block().document()->lastBlock();
    return block.position() + block.length();
}

QString FakeVimHandler::Private::lastSearchString() const
{
     return m_searchHistory.empty() ? QString() : m_searchHistory.back();
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

void FakeVimHandler::addWidget(QWidget *widget)
{
    widget->installEventFilter(this);
    QFont font = widget->font();
    //: -misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1
    //font.setFamily("Misc");
    font.setFamily("Monospace");
    font.setStretch(QFont::SemiCondensed);
    widget->setFont(font);
    if (QTextEdit *ed = qobject_cast<QTextEdit *>(widget)) {
        ed->setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
        ed->setLineWrapMode(QTextEdit::NoWrap);
    } else if (QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(widget)) {
        ed->setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
        ed->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
}

void FakeVimHandler::removeWidget(QWidget *widget)
{
    widget->removeEventFilter(this);
}

void FakeVimHandler::handleCommand(QWidget *widget, const QString &cmd)
{
    d->m_textedit = qobject_cast<QTextEdit *>(widget);
    d->m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    d->handleCommand(cmd);
}

