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
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QTextEdit>


using namespace FakeVim::Internal;

#define StartOfLine    QTextCursor::StartOfLine
#define EndOfLine      QTextCursor::EndOfLine
#define MoveAnchor     QTextCursor::MoveAnchor
#define KeepAnchor     QTextCursor::KeepAnchor
#define Up             QTextCursor::Up
#define Down           QTextCursor::Down
#define Right          QTextCursor::Right
#define Left           QTextCursor::Left
#define EndOfDocument  QTextCursor::End


///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////

#define EDITOR(s) (m_textedit ? m_textedit->s : m_plaintextedit->s)

const int ParagraphSeparator = 0x00002029;

using namespace Qt;

enum Mode
{
    InsertMode,
    CommandMode,
    ExMode,
    SearchForwardMode,
    SearchBackwardMode,
};

enum SubMode
{
    NoSubMode,
    RegisterSubMode,
    ChangeSubMode,
    DeleteSubMode,
    ZSubMode,
};

enum SubSubMode
{
    NoSubSubMode,
    FtSubSubMode,       // used for f, F, t, T
    MarkSubSubMode,     // used for m
    BackTickSubSubMode, // used for ` 
    TickSubSubMode      // used for '
};

enum VisualMode
{
    NoVisualMode,
    VisualCharMode,
    VisualLineMode,
    VisualBlockMode,
};

static const QString ConfigStartOfLine = "startofline";
static const QString ConfigOn = "on";

struct EditOperation
{
    EditOperation() : m_position(-1), m_itemCount(0) {}
    int m_position;
    int m_itemCount; // used to combine several operations
    QString m_from;
    QString m_to;
};

QDebug &operator<<(QDebug &ts, const EditOperation &op)
{
    if (op.m_itemCount > 0) {
        ts << "EDIT BLOCK WITH" << op.m_itemCount << "ITEMS";
    } else {
        ts << "EDIT AT " << op.m_position
           << " FROM " << op.m_from << " TO " << op.m_to;
    }
    return ts;
}

class FakeVimHandler::Private
{
public:
    Private(FakeVimHandler *parent);

    bool eventFilter(QObject *ob, QEvent *ev);
    void handleExCommand(const QString &cmd);

private:
    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    void handleKey(int key, const QString &text);
    void handleInsertMode(int key, const QString &text);
    void handleCommandMode(int key, const QString &text);
    void handleRegisterMode(int key, const QString &text);
    void handleMiniBufferModes(int key, const QString &text);
    void finishMovement(const QString &text = QString());
    void search(const QString &needle, bool forward);

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEol() const { return m_tc.atBlockEnd() && m_tc.block().length()>1; }

    int lastPositionInDocument() const;
    int positionForLine(int line) const; // 1 based line, 0 based pos
    int lineForPosition(int pos) const;  // 1 based line, 0 based pos

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

    // helper function for handleCommand. return 1 based line index.
    int readLineCode(QString &cmd);
    QTextCursor selectRange(int beginLine, int endLine);

public:
    void enterInsertMode();
    void enterCommandMode();
    void showMessage(const QString &msg);
    void updateMiniBuffer();
    void updateSelection();
    void quit();

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;

private:
    FakeVimHandler *q;
    Mode m_mode;
    SubMode m_submode;
    SubSubMode m_subsubmode;
    int m_subsubdata;
    QString m_input;
    QTextCursor m_tc;
    QHash<int, QString> m_registers;
    int m_register;
    QString m_mvcount;
    QString m_opcount;

    bool m_fakeEnd;

    QWidget *editor() const;
    bool isSearchMode() const
        { return m_mode == SearchForwardMode || m_mode == SearchBackwardMode; }
    int m_gflag;  // whether current command started with 'g'
    
    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    QString m_lastInsertion;

    // undo handling
    void recordInsert(int position, const QString &data);
    void recordRemove(int position, const QString &data);
    void recordRemove(int position, int length);
    void undo();
    void redo();
    QStack<EditOperation> m_undoStack;
    QStack<EditOperation> m_redoStack;

    // extra data for '.'
    QString m_dotCount;
    QString m_dotCommand;

    // history for '/'
    QString lastSearchString() const;
    QStringList m_searchHistory;
    int m_searchHistoryIndex;

    // history for ':'
    QStringList m_commandHistory;
    int m_commandHistoryIndex;

    // visual line mode
    void enterVisualMode(VisualMode visualMode);
    void leaveVisualMode();
    VisualMode m_visualMode;

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
    m_fakeEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_textedit = 0;
    m_plaintextedit = 0;
    m_visualMode = NoVisualMode;

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
    //qDebug() << "KEY: " << key << text << "POS: " << m_tc.position();
    //qDebug() << "\nUNDO: " << m_undoStack << "\nREDO: " << m_redoStack;
    if (m_mode == InsertMode)
        handleInsertMode(key, text);
    else if (m_mode == CommandMode)
        handleCommandMode(key, text);
    else if (m_mode == ExMode || m_mode == SearchForwardMode
            || m_mode == SearchBackwardMode)
        handleMiniBufferModes(key, text);
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand)
{
    if (m_submode == ChangeSubMode) {
        if (!dotCommand.isEmpty())
            m_dotCommand = "c" + dotCommand;
        m_registers[m_register] = m_tc.selectedText();
        m_tc.removeSelectedText();
        m_mode = InsertMode;
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        if (!dotCommand.isEmpty())
            m_dotCommand = "d" + dotCommand;
        recordRemove(qMin(m_tc.position(), m_tc.anchor()), m_tc.selectedText());
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

    updateSelection();
    updateMiniBuffer();
}

void FakeVimHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (m_visualMode != NoVisualMode) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_tc;
        sel.format = m_tc.blockCharFormat();
        sel.format.setFontWeight(QFont::Bold);
        sel.format.setFontUnderline(true);
        int cursorPos = m_tc.position();
        int anchorPos = m_marks['<'];
        //qDebug() << "POS: " << cursorPos << " ANCHOR: " << anchorPos;
        if (m_visualMode == VisualCharMode) {
            sel.cursor.setPosition(anchorPos, KeepAnchor);
            selections.append(sel);
        } else if (m_visualMode == VisualLineMode) {
            sel.cursor.setPosition(qMin(cursorPos, anchorPos), MoveAnchor);
            sel.cursor.movePosition(StartOfLine, MoveAnchor);
            sel.cursor.setPosition(qMax(cursorPos, anchorPos), KeepAnchor);
            sel.cursor.movePosition(EndOfLine, KeepAnchor);
            selections.append(sel);
        } else if (m_visualMode == VisualBlockMode) {
            QTextCursor tc = m_tc;
            tc.setPosition(anchorPos);
            tc.movePosition(StartOfLine, MoveAnchor);
            QTextBlock anchorBlock = tc.block();
            QTextBlock cursorBlock = m_tc.block();
            int anchorColumn = anchorPos - anchorBlock.position();
            int cursorColumn = cursorPos - cursorBlock.position();
            int startColumn = qMin(anchorColumn, cursorColumn);
            int endColumn = qMax(anchorColumn, cursorColumn);
            int endPos = cursorBlock.position();
            while (tc.position() <= endPos) {
                if (startColumn < tc.block().length() - 1) {
                    int last = qMin(tc.block().length() - 1, endColumn);
                    int len = last - startColumn + 1;
                    sel.cursor = tc;
                    sel.cursor.movePosition(Right, MoveAnchor, startColumn);
                    sel.cursor.movePosition(Right, KeepAnchor, len);
                    selections.append(sel);
                }
                tc.movePosition(Down, MoveAnchor, 1);
            }
        }
    }
    EDITOR(setExtraSelections(selections));
}

void FakeVimHandler::Private::updateMiniBuffer()
{
    QString msg;
    if (!m_currentMessage.isEmpty()) {
        msg = m_currentMessage;
        m_currentMessage.clear();
    } else if (m_visualMode == VisualCharMode) {
        msg = "-- VISUAL --";
    } else if (m_visualMode == VisualLineMode) {
        msg = "-- VISUAL LINE --";
    } else if (m_visualMode == VisualBlockMode) {
        msg = "-- VISUAL BLOCK --";
    } else if (m_mode == InsertMode) {
        msg = "-- INSERT --";
    } else {
        if (m_mode == SearchForwardMode) 
            msg += '/';
        else if (m_mode == SearchBackwardMode) 
            msg += '?';
        else if (m_mode == ExMode) 
            msg += ':';
        foreach (QChar c, m_commandBuffer) {
            if (c.unicode() < 32) {
                msg += '^';
                msg += QChar(c.unicode() + 64);
            } else {
                msg += c;
            }
        }
    }
    emit q->commandBufferChanged(msg);

    int linesInDoc = linesInDocument();
    int l = cursorLineInDocument();
    QString status;
    QString pos = tr("%1,%2").arg(l + 1).arg(cursorColumnInDocument() + 1);
    status += tr("%1").arg(pos, -10);
    // FIXME: physical "-" logical
    if (linesInDoc != 0) {
        status += tr("%1").arg(l * 100 / linesInDoc, 4);
        status += "%";
    } else {
        status += "All";
    }
    emit q->statusDataChanged(status);
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
        finishMovement("c");
    } else if (m_submode == DeleteSubMode && key == 'd') {
        m_tc.movePosition(StartOfLine, MoveAnchor);
        m_tc.movePosition(Down, KeepAnchor, count());
        m_registers[m_register] = m_tc.selectedText();
        finishMovement("d");
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
        finishMovement(QString(QChar(m_subsubdata)) + QChar(key));
    } else if (m_subsubmode == MarkSubSubMode) {
        m_marks[key] = m_tc.position();
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        if (m_marks.contains(key)) {
            m_tc.setPosition(m_marks[key], MoveAnchor);
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
    } else if (key == ':') {
        m_mode = ExMode;
        m_commandBuffer.clear();
        if (m_visualMode != NoVisualMode) {
            m_commandBuffer = "'<,'>";
            leaveVisualMode();
        }
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '/' || key == '?') {
        m_mode = (key == '/') ? SearchForwardMode : SearchBackwardMode;
        m_commandBuffer.clear();
        m_searchHistory.append(QString());
        m_searchHistoryIndex = m_searchHistory.size() - 1;
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
    } else if (key == '.') {
        qDebug() << "REPEATING" << m_dotCommand;
        for (int i = count(); --i >= 0; )
            foreach (QChar c, m_dotCommand)
                handleKey(c.unicode(), QString(c));
    } else if (key == 'a') {
        m_mode = InsertMode;
        m_lastInsertion.clear();
        m_tc.movePosition(Right, MoveAnchor, 1);
        updateMiniBuffer();
    } else if (key == 'A') {
        m_mode = InsertMode;
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
    } else if (key == 'C') {
        m_submode = ChangeSubMode;
        m_tc.movePosition(EndOfLine, KeepAnchor);
        finishMovement();
    } else if (key == 'd' && m_visualMode == NoVisualMode) {
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if (key == 'd') {
        leaveVisualMode();
        int beginLine = lineForPosition(m_marks['<']);
        int endLine = lineForPosition(m_marks['>']);
        m_tc = selectRange(beginLine, endLine);
        m_tc.removeSelectedText();
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
        m_tc.setPosition(positionForLine(n), MoveAnchor);
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
        enterInsertMode();
        updateMiniBuffer();
    } else if (key == 'I') {
        enterInsertMode();
        if (m_gflag)
            m_tc.movePosition(StartOfLine, KeepAnchor);
        else
            moveToFirstNonBlankOnLine();
    } else if (key == 'j' || key == Key_Down) {
        m_tc.movePosition(Down, KeepAnchor, count());
        finishMovement();
    } else if (key == 'J') {
        EditOperation op;
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
        enterInsertMode();
        m_tc.movePosition(EndOfLine, MoveAnchor);
        m_tc.insertText("\n");
    } else if (key == 'O') {
        enterInsertMode();
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
        redo();
    } else if (key == 's') {
        m_submode = ChangeSubMode;
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
    } else if (key == 't' || key == 'T') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'u') {
        undo();
    } else if (key == 'v') {
        enterVisualMode(VisualCharMode);
    } else if (key == 'V') {
        enterVisualMode(VisualLineMode);
    } else if (key == control('v')) {
        enterVisualMode(VisualBlockMode);
    } else if (key == 'w') {
        moveToNextWord(false);
        finishMovement("w");
    } else if (key == 'W') {
        moveToNextWord(true);
        finishMovement("W");
    } else if (key == 'x') { // = "dl"
        if (atEol())
            m_tc.movePosition(Left, MoveAnchor, 1);
        m_submode = DeleteSubMode;
        m_tc.movePosition(Right, KeepAnchor, qMin(count(), rightDist()));
        finishMovement("l");
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
        if (m_visualMode != NoVisualMode)
            leaveVisualMode();
    } else {
        qDebug() << "Ignored" << key << text;
    }    
}

void FakeVimHandler::Private::handleInsertMode(int key, const QString &text)
{
    if (key == Key_Escape) {
        // start with '1', as one instance was already physically inserted
        // while typing
        QString data = m_lastInsertion;
        for (int i = 1; i < count(); ++i) {
            m_tc.insertText(m_lastInsertion);
            data += m_lastInsertion;
        }
        recordInsert(m_tc.position() - m_lastInsertion.size(), data);
        m_tc.movePosition(Left, MoveAnchor, qMin(1, leftDist()));
        enterCommandMode();
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
        m_lastInsertion += "\n";
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
    updateMiniBuffer();
}

void FakeVimHandler::Private::handleMiniBufferModes(int key, const QString &text)
{
    Q_UNUSED(text)

    if (key == Key_Escape) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
    } else if (key == Key_Backspace) {
        if (m_commandBuffer.isEmpty())
            enterCommandMode();
        else
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (key == Key_Return && m_mode == ExMode) {
        if (!m_commandBuffer.isEmpty()) {
            m_commandHistory.takeLast();
            m_commandHistory.append(m_commandBuffer);
            handleExCommand(m_commandBuffer);
        }
    } else if (key == Key_Return && isSearchMode()) {
        if (!m_commandBuffer.isEmpty()) {
            m_searchHistory.takeLast();
            m_searchHistory.append(m_commandBuffer);
            m_lastSearchForward = (m_mode == SearchForwardMode);
            search(lastSearchString(), m_lastSearchForward);
        }
        enterCommandMode();
        updateMiniBuffer();
    } else if (key == Key_Up && isSearchMode()) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entires in the history into account.
        if (m_searchHistoryIndex > 0) {
            --m_searchHistoryIndex;
            m_commandBuffer = m_searchHistory.at(m_searchHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Up && m_mode == ExMode) {
        if (m_commandHistoryIndex > 0) {
            --m_commandHistoryIndex;
            m_commandBuffer = m_commandHistory.at(m_commandHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Down && isSearchMode()) {
        if (m_searchHistoryIndex < m_searchHistory.size() - 1) {
            ++m_searchHistoryIndex;
            m_commandBuffer = m_searchHistory.at(m_searchHistoryIndex);
            updateMiniBuffer();
        }
    } else if (key == Key_Down && m_mode == ExMode) {
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
    if (c == '\'' && !cmd.isEmpty()) {
        int pos = m_marks.value(cmd.at(0).unicode());
        qDebug() << " MARK: " << cmd.at(0) << pos << lineForPosition(pos);
        if (!pos) {
             showMessage(tr("E20: Mark '%1' not set").arg(cmd.at(0)));
             return -1;
        }
        cmd = cmd.mid(1);
        return lineForPosition(pos);
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

QTextCursor FakeVimHandler::Private::selectRange(int beginLine, int endLine)
{
    QTextCursor tc = m_tc;
    tc.setPosition(positionForLine(beginLine), MoveAnchor);
    if (endLine == linesInDocument()) {
        tc.setPosition(positionForLine(endLine), KeepAnchor);
        tc.movePosition(EndOfLine, KeepAnchor);
    } else {
        tc.setPosition(positionForLine(endLine + 1), KeepAnchor);
    }
    return tc;
}

void FakeVimHandler::Private::handleExCommand(const QString &cmd0)
{
    QString cmd = cmd0;
    if (cmd.startsWith("%"))
        cmd = "1,$" + cmd.mid(1);

    int beginLine = -1;
    int endLine = -1;

    int line = readLineCode(cmd);
    if (line != -1)
        beginLine = line;
    
    if (cmd.startsWith(',')) {
        cmd = cmd.mid(1);
        line = readLineCode(cmd);
        if (line != -1)
            endLine = line;
    }

    qDebug() << "RANGE: " << beginLine << endLine << cmd << cmd0;

    static QRegExp reWrite("^w!?( (.*))?$");
    static QRegExp reDelete("^d( (.*))?$");

    if (cmd.isEmpty()) {
        m_tc.setPosition(positionForLine(beginLine));
        showMessage(QString());
    } else if (cmd == "q!" || cmd == "q") { // :q
        quit();
    } else if (reDelete.indexIn(cmd) != -1) { // :d
        if (beginLine == -1)
            beginLine = cursorLineInDocument();
        if (endLine == -1)
            endLine = cursorLineInDocument();
        QTextCursor tc = selectRange(beginLine, endLine);
        QString reg = reDelete.cap(2);
        if (!reg.isEmpty())
            m_registers[reg.at(0).unicode()] = tc.selection().toPlainText();
        tc.removeSelectedText();
    } else if (reWrite.indexIn(cmd) != -1) { // :w
        enterCommandMode();
        bool noArgs = (beginLine == -1);
        if (beginLine == -1)
            beginLine = 0;
        if (endLine == -1)
            endLine = linesInDocument();
        qDebug() << "LINES: " << beginLine << endLine;
        bool forced = cmd.startsWith("w!");
        QString fileName = reWrite.cap(2);
        if (fileName.isEmpty())
            fileName = m_currentFileName;
        QFile file(fileName);
        bool exists = file.exists();
        if (exists && !forced && !noArgs) {
            showMessage(tr("File '%1' exists (add ! to override)").arg(fileName));
        } else if (file.open(QIODevice::ReadWrite)) {
            QTextCursor tc = selectRange(beginLine, endLine);
            qDebug() << "ANCHOR: " << tc.position() << tc.anchor()
                << tc.selection().toPlainText();
            { QTextStream ts(&file); ts << tc.selection().toPlainText(); }
            file.close();
            file.open(QIODevice::ReadOnly);
            QByteArray ba = file.readAll();
            m_commandBuffer = QString("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(exists ? " " : " [New] ")
                .arg(ba.count('\n')).arg(ba.size());
            updateMiniBuffer();
        } else {
            showMessage(tr("Cannot open file '%1' for reading").arg(fileName));
        }
    } else if (cmd.startsWith("r ")) { // :r
        m_currentFileName = cmd.mid(2);
        QFile file(m_currentFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        QString data = ts.readAll();
        EDITOR(setPlainText(data));
        m_commandBuffer = QString("\"%1\" %2L, %3C")
            .arg(m_currentFileName).arg(data.count('\n')).arg(data.size());
        enterCommandMode();
        updateMiniBuffer();
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
    if (!editor())
        return 0;
    QRect rect = EDITOR(cursorRect());
    return rect.y() / rect.height();
}

int FakeVimHandler::Private::linesOnScreen() const
{
    if (!editor())
        return 1;
    QRect rect = EDITOR(cursorRect());
    return EDITOR(height()) / rect.height();
}

int FakeVimHandler::Private::columnsOnScreen() const
{
    if (!editor())
        return 1;
    QRect rect = EDITOR(cursorRect());
    // qDebug() << "WID: " << EDITOR(width()) << "RECT: " << rect;
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

int FakeVimHandler::Private::positionForLine(int line) const
{
    return m_tc.block().document()->findBlockByNumber(line - 1).position();
}

int FakeVimHandler::Private::lineForPosition(int pos) const
{
    QTextCursor tc = m_tc;
    tc.setPosition(pos);
    return tc.block().blockNumber() + 1;
}

void FakeVimHandler::Private::enterVisualMode(VisualMode visualMode)
{
    m_visualMode = visualMode;
    m_marks['<'] = m_tc.position();
    updateMiniBuffer();
    updateSelection();
}

void FakeVimHandler::Private::leaveVisualMode()
{
    m_visualMode = NoVisualMode;
    m_marks['>'] = m_tc.position();
    updateMiniBuffer();
    updateSelection();
}

QWidget *FakeVimHandler::Private::editor() const
{
    return m_textedit
        ? static_cast<QWidget *>(m_textedit)
        : static_cast<QWidget *>(m_plaintextedit);
}

void FakeVimHandler::Private::undo()
{
#if 0
    EDITOR(undo());
#else
    if (m_undoStack.isEmpty())
        return;
    EditOperation op = m_undoStack.pop();
    //qDebug() << "UNDO " << op;
    if (op.m_itemCount > 0) {
        for (int i = op.m_itemCount; --i >= 0; )
            undo();
    } else {
        m_tc.setPosition(op.m_position, MoveAnchor);
        if (!op.m_to.isEmpty()) {
            m_tc.setPosition(op.m_position + op.m_to.size(), KeepAnchor);
            m_tc.deleteChar();
        }
        if (!op.m_from.isEmpty())
            m_tc.insertText(op.m_from);
        m_tc.setPosition(op.m_position, MoveAnchor);
    }
    m_redoStack.push(op);
#endif
}

void FakeVimHandler::Private::redo()
{
#if 0
    EDITOR(redo());
#else
    if (m_redoStack.isEmpty())
        return;
    EditOperation op = m_redoStack.pop();
    //qDebug() << "REDO " << op;
    if (op.m_itemCount > 0) {
        for (int i = op.m_itemCount; --i >= 0; )
            redo();
    } else {
        m_tc.setPosition(op.m_position, MoveAnchor);
        if (!op.m_from.isEmpty()) {
            m_tc.setPosition(op.m_position + op.m_from.size(), KeepAnchor);
            m_tc.deleteChar();
        }
        if (!op.m_to.isEmpty())
            m_tc.insertText(op.m_to);
        m_tc.setPosition(op.m_position, MoveAnchor);
    }
    m_undoStack.push(op);
#endif
}

void FakeVimHandler::Private::recordInsert(int position, const QString &data)
{
    EditOperation op;
    op.m_position = position;
    op.m_to = data;
    m_undoStack.push(op);
    m_redoStack.clear();
}

void FakeVimHandler::Private::recordRemove(int position, int length)
{
    QTextCursor tc = m_tc;
    tc.setPosition(position, MoveAnchor);
    tc.setPosition(position + length, KeepAnchor);
    recordRemove(position, tc.selection().toPlainText());
}

void FakeVimHandler::Private::recordRemove(int position, const QString &data)
{
    EditOperation op;
    op.m_position = position;
    op.m_from = data;
    m_undoStack.push(op);
    m_redoStack.clear();
}

void FakeVimHandler::Private::enterInsertMode()
{
    EDITOR(setOverwriteMode(false));
    m_mode = InsertMode;
    m_lastInsertion.clear();
}

void FakeVimHandler::Private::enterCommandMode()
{
    if (editor())
        EDITOR(setOverwriteMode(true));
    m_mode = CommandMode;
}

void FakeVimHandler::Private::quit()
{
    showMessage(QString());
    EDITOR(setOverwriteMode(false));
    q->quitRequested(editor());
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
    d->enterCommandMode();
    if (QTextEdit *ed = qobject_cast<QTextEdit *>(widget)) {
        //ed->setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
        ed->setLineWrapMode(QTextEdit::NoWrap);
    } else if (QPlainTextEdit *ed = qobject_cast<QPlainTextEdit *>(widget)) {
        //ed->setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
        ed->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    d->showMessage("vi emulation mode. Hit <Shift+Esc>:q<Return> to quit");
    d->updateMiniBuffer();
}

void FakeVimHandler::removeWidget(QWidget *widget)
{
    d->showMessage(QString());
    d->updateMiniBuffer();
    widget->removeEventFilter(this);
}

void FakeVimHandler::handleCommand(QWidget *widget, const QString &cmd)
{
    d->m_textedit = qobject_cast<QTextEdit *>(widget);
    d->m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    d->handleExCommand(cmd);
}


void FakeVimHandler::quit()
{
    d->quit();
}
