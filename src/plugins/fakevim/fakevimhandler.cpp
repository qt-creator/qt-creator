/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fakevimhandler.h"

#include "fakevimconstants.h"

// Please do not add any direct dependencies to other Qt Creator code  here. 
// Instead emit signals and let the FakeVimPlugin channel the information to
// Qt Creator. The idea is to keep this file here in a "clean" state that
// allows easy reuse with any QTextEdit or QPlainTextEdit derived class.

//#include <indenter.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>
#include <QtCore/QtAlgorithms>
#include <QtCore/QStack>

#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QTextEdit>


using namespace FakeVim::Internal;
using namespace FakeVim::Constants;

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
    PassingMode, // lets keyevents to be passed to the main application
};

enum SubMode
{
    NoSubMode,
    RegisterSubMode,
    ChangeSubMode,
    DeleteSubMode,
    FilterSubMode,
    ReplaceSubMode,    // used for R and r
    YankSubMode,
    IndentSubMode,
    ZSubMode,
};

enum SubSubMode
{
    // typically used for things that require one more data item
    // and are 'nested' behind a mode
    NoSubSubMode,
    FtSubSubMode,       // used for f, F, t, T
    MarkSubSubMode,     // used for m
    BackTickSubSubMode, // used for `
    TickSubSubMode,     // used for '
};

enum VisualMode
{
    NoVisualMode,
    VisualCharMode,
    VisualLineMode,
    VisualBlockMode,
};

enum MoveType
{
    MoveExclusive,
    MoveInclusive,
    MoveLineWise,
};

struct EditOperation
{
    EditOperation() : position(-1), itemCount(0) {}
    int position;
    int itemCount; // used to combine several operations
    QString from;
    QString to;
};

QDebug &operator<<(QDebug &ts, const EditOperation &op)
{
    if (op.itemCount > 0) {
        ts << "\n  EDIT BLOCK WITH " << op.itemCount << " ITEMS";
    } else {
        ts << "\n  EDIT AT " << op.position
           << "\n      FROM " << op.from << "\n      TO " << op.to;
    }
    return ts;
}

QDebug &operator<<(QDebug &ts, const QList<QTextEdit::ExtraSelection> &sels)
{
    foreach (QTextEdit::ExtraSelection sel, sels) 
        ts << "SEL: " << sel.cursor.anchor() << sel.cursor.position(); 
    return ts;
}
        
int lineCount(const QString &text)
{
    //return text.count(QChar(ParagraphSeparator));
    return text.count(QChar('\n'));
}

class FakeVimHandler::Private
{
public:
    Private(FakeVimHandler *parent, QWidget *widget);

    bool handleEvent(QKeyEvent *ev);
    void handleExCommand(const QString &cmd);

    void setupWidget();
    void restoreWidget();

private:
    friend class FakeVimHandler;
    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    bool handleKey(int key, int unmodified, const QString &text);
    bool handleInsertMode(int key, int unmodified, const QString &text);
    bool handleCommandMode(int key, int unmodified, const QString &text);
    bool handleRegisterMode(int key, int unmodified, const QString &text);
    bool handleMiniBufferModes(int key, int unmodified, const QString &text);
    void finishMovement(const QString &text = QString());
    void search(const QString &needle, bool forward);

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEndOfLine() const
        { return m_tc.atBlockEnd() && m_tc.block().length() > 1; }

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

    // helper functions for indenting
    bool isElectricCharacter(QChar c) const
        { return c == '{' || c == '}' || c == '#'; }
    int indentDist() const;
    void indentRegion(QTextBlock first, QTextBlock last, QChar typedChar=0);
    void indentCurrentLine(QChar typedChar);

    void moveToFirstNonBlankOnLine();
    void moveToDesiredColumn();
    void moveToNextWord(bool simple);
    void moveToMatchingParanthesis();
    void moveToWordBoundary(bool simple, bool forward);

    // to reduce line noise
    typedef QTextCursor::MoveOperation MoveOperation;
    typedef QTextCursor::MoveMode MoveMode;
    void moveToEndOfDocument() { m_tc.movePosition(EndOfDocument, MoveAnchor); }
    void moveToStartOfLine() { m_tc.movePosition(StartOfLine, MoveAnchor); }
    void moveToEndOfLine() { m_tc.movePosition(EndOfLine, MoveAnchor); }
    void moveUp(int n = 1) { m_tc.movePosition(Up, MoveAnchor, n); }
    void moveDown(int n = 1) { m_tc.movePosition(Down, MoveAnchor, n); }
    void moveRight(int n = 1) { m_tc.movePosition(Right, MoveAnchor, n); }
    void moveLeft(int n = 1) { m_tc.movePosition(Left, MoveAnchor, n); }
    void setAnchor() { m_anchor = m_tc.position(); }

    QString selectedText() const;

    void handleFfTt(int key);

    // helper function for handleCommand. return 1 based line index.
    int readLineCode(QString &cmd);
    void selectRange(int beginLine, int endLine);

    void enterInsertMode();
    void enterCommandMode();
    void enterExMode();
    void showRedMessage(const QString &msg);
    void showBlackMessage(const QString &msg);
    void notImplementedYet();
    void updateMiniBuffer();
    void updateSelection();
    void quit();
    QWidget *editor() const;

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    bool m_wasReadOnly; // saves read-only state of document

    FakeVimHandler *q;
    Mode m_mode;
    SubMode m_submode;
    SubSubMode m_subsubmode;
    int m_subsubdata;
    QString m_input;
    QTextCursor m_tc;
    int m_anchor; 
    QHash<int, QString> m_registers;
    int m_register;
    QString m_mvcount;
    QString m_opcount;
    MoveType m_moveType;

    bool m_fakeEnd;

    bool isSearchMode() const
        { return m_mode == SearchForwardMode || m_mode == SearchBackwardMode; }
    int m_gflag;  // whether current command started with 'g'

    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    QString m_lastInsertion;

    // undo handling
    void recordOperation(const EditOperation &op);
    void recordInsert(int position, const QString &data);
    void recordRemove(int position, const QString &data);
    void recordRemove(int position, int length);

    void recordRemoveNextChar();
    void recordInsertText(const QString &data);
    QString recordRemoveSelectedText();
    void recordMove();
    void recordBeginGroup();
    void recordEndGroup();
    int anchor() const { return m_anchor; }
    int position() const { return m_tc.position(); }

    void undo();
    void redo();
    QStack<EditOperation> m_undoStack;
    QStack<EditOperation> m_redoStack;
    QStack<int> m_undoGroupStack;

    // extra data for '.'
    QString m_dotCommand;

    // extra data for ';'
    QString m_semicolonCount;
    int m_semicolonType;  // 'f', 'F', 't', 'T'
    int m_semicolonKey;

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

    // for restoring cursor position
    int m_savedYankPosition;
    int m_desiredColumn;

    QPointer<QObject> m_extraData;
    int m_cursorWidth;

    void recordJump();
    QList<int> m_jumpListUndo;
    QList<int> m_jumpListRedo;
};

FakeVimHandler::Private::Private(FakeVimHandler *parent, QWidget *widget)
{
    q = parent;

    m_textedit = qobject_cast<QTextEdit *>(widget);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);

    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_fakeEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_visualMode = NoVisualMode;
    m_desiredColumn = 0;
    m_moveType = MoveInclusive;
    m_anchor = 0;
    m_savedYankPosition = 0;
    m_cursorWidth = EDITOR(cursorWidth());

    m_config[ConfigStartOfLine] = ConfigOn;
    m_config[ConfigTabStop]     = "8";
    m_config[ConfigSmartTab]    = ConfigOff;
    m_config[ConfigShiftWidth]  = "8";
    m_config[ConfigExpandTab]   = ConfigOff;
    m_config[ConfigAutoIndent]  = ConfigOff;
}

bool FakeVimHandler::Private::handleEvent(QKeyEvent *ev)
{
    int key = ev->key();
    const int um = key; // keep unmodified key around

    // FIXME
    if (m_mode == PassingMode && key != Qt::Key_Control && key != Qt::Key_Shift) {
        if (key == ',') { // use ',,' to leave, too.
            quit();
            return true;
        }
        m_mode = CommandMode;
        updateMiniBuffer();
        return false;
    }

    if (key == Key_Shift || key == Key_Alt || key == Key_Control
        || key == Key_Alt || key == Key_AltGr || key == Key_Meta)
        return false;

    // Fake "End of line"
    m_tc = EDITOR(textCursor());
    m_tc.setVisualNavigation(true);

    if (m_fakeEnd)
        moveRight();

    if ((ev->modifiers() & Qt::ControlModifier) != 0) {
        key += 256;
        key += 32; // make it lower case
    } else if (key >= Key_A && key <= Key_Z
        && (ev->modifiers() & Qt::ShiftModifier) == 0) {
        key += 32;
    }
    bool handled = handleKey(key, um, ev->text());

    // We fake vi-style end-of-line behaviour
    m_fakeEnd = (atEndOfLine() && m_mode == CommandMode);

    if (m_fakeEnd)
        moveLeft();

    EDITOR(setTextCursor(m_tc));
    EDITOR(ensureCursorVisible());
    return handled;
}

void FakeVimHandler::Private::setupWidget()
{
    enterCommandMode();
    EDITOR(installEventFilter(q));
    //EDITOR(setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
    if (m_textedit) {
        m_textedit->setLineWrapMode(QTextEdit::NoWrap);
    } else if (m_plaintextedit) {
        m_plaintextedit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    m_wasReadOnly = EDITOR(isReadOnly());
    //EDITOR(setReadOnly(true)); 
    showBlackMessage("vi emulation mode.");
    updateMiniBuffer();
}

void FakeVimHandler::Private::restoreWidget()
{
    //showBlackMessage(QString());
    //updateMiniBuffer();
    EDITOR(removeEventFilter(q));
    EDITOR(setReadOnly(m_wasReadOnly));
}

bool FakeVimHandler::Private::handleKey(int key, int unmodified, const QString &text)
{
    //qDebug() << "KEY: " << key << text << "POS: " << m_tc.position();
    //qDebug() << "\nUNDO: " << m_undoStack << "\nREDO: " << m_redoStack;
    if (m_mode == InsertMode)
        return handleInsertMode(key, unmodified, text);
    if (m_mode == CommandMode)
        return handleCommandMode(key, unmodified, text);
    if (m_mode == ExMode || m_mode == SearchForwardMode
            || m_mode == SearchBackwardMode)
        return handleMiniBufferModes(key, unmodified, text);
    return false;
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand)
{
    //qDebug() << "ANCHOR: " << m_anchor;
    if (m_submode == FilterSubMode) {
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        m_tc.setPosition(qMin(anchor(), position()));
        enterExMode();
        m_commandBuffer = QString(".,+%1!").arg(qAbs(endLine - beginLine));
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
        return;
    }

    if (m_visualMode != NoVisualMode)
        m_marks['>'] = m_tc.position();

    if (m_submode == ChangeSubMode) {
        if (m_moveType == MoveInclusive)
            moveRight(); // correction
        if (!dotCommand.isEmpty())
            m_dotCommand = "c" + dotCommand;
        QString text = recordRemoveSelectedText();
        //qDebug() << "CHANGING TO INSERT MODE" << text;
        m_registers[m_register] = text;
        m_mode = InsertMode;
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        if (m_moveType == MoveInclusive)
            moveRight(); // correction
        if (!dotCommand.isEmpty())
            m_dotCommand = "d" + dotCommand;
        m_registers[m_register] = recordRemoveSelectedText();
        recordEndGroup();
        m_submode = NoSubMode;
        if (atEndOfLine())
            moveLeft();
    } else if (m_submode == YankSubMode) {
        m_registers[m_register] = selectedText();
        m_tc.setPosition(m_savedYankPosition);
        m_submode = NoSubMode;
    } else if (m_submode == ReplaceSubMode) {
        m_submode = NoSubMode;
    } else if (m_submode == IndentSubMode) {
        QTextDocument *doc = EDITOR(document());
        int start = m_tc.selectionStart();
        int end = m_tc.selectionEnd();
        if (start > end)
            qSwap(start, end);
        QTextBlock startBlock = doc->findBlock(start);
        indentRegion(doc->findBlock(start), doc->findBlock(end).next());
        m_tc.setPosition(startBlock.position());
        moveToFirstNonBlankOnLine();
        m_submode = NoSubMode;
    }
    m_moveType = MoveInclusive;
    m_mvcount.clear();
    m_opcount.clear();
    m_gflag = false;
    m_register = '"';
    m_tc.clearSelection();

    updateSelection();
    updateMiniBuffer();
    m_desiredColumn = leftDist();
}

void FakeVimHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections;
    if (m_visualMode != NoVisualMode) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_tc;
        sel.format = m_tc.blockCharFormat();
#if 0
        sel.format.setFontWeight(QFont::Bold);
        sel.format.setFontUnderline(true);
#else
        sel.format.setForeground(Qt::white);
        sel.format.setBackground(Qt::black);
#endif
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
    //qDebug() << "SELECTION: " << selections;
    emit q->selectionChanged(selections);
}

void FakeVimHandler::Private::updateMiniBuffer()
{
    QString msg;
    if (m_mode == PassingMode) {
        msg = "-- PASSING --";
    } else if (!m_currentMessage.isEmpty()) {
        msg = m_currentMessage;
        m_currentMessage.clear();
    } else if (m_mode == CommandMode && m_visualMode != NoVisualMode) {
        if (m_visualMode == VisualCharMode) {
            msg = "-- VISUAL --";
        } else if (m_visualMode == VisualLineMode) {
            msg = "-- VISUAL LINE --";
        } else if (m_visualMode == VisualBlockMode) {
            msg = "-- VISUAL BLOCK --";
        }
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
        if (!msg.isEmpty() && m_mode != CommandMode)
            msg += QChar(10073); // '|'; // FIXME: Use a real "cursor"
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

void FakeVimHandler::Private::showRedMessage(const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    m_currentMessage = msg;
    updateMiniBuffer();
}

void FakeVimHandler::Private::showBlackMessage(const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    m_commandBuffer = msg;
    updateMiniBuffer();
}

void FakeVimHandler::Private::notImplementedYet()
{
    showRedMessage("Not implemented in FakeVim");
    updateMiniBuffer();
}

bool FakeVimHandler::Private::handleCommandMode(int key, int unmodified,
    const QString &text)
{
    bool handled = true;

    if (m_submode == RegisterSubMode) {
        m_register = key;
        m_submode = NoSubMode;
    } else if (m_submode == ChangeSubMode && key == 'c') {
        moveToStartOfLine();
        setAnchor();
        moveDown(count());
        m_moveType = MoveLineWise;
        finishMovement("c");
    } else if (m_submode == DeleteSubMode && key == 'd') {
        moveToStartOfLine();
        setAnchor();
        moveDown(count());
        m_moveType = MoveLineWise;
        finishMovement("d");
    } else if (m_submode == YankSubMode && key == 'y') {
        moveToStartOfLine();
        setAnchor();
        moveDown(count());
        m_moveType = MoveLineWise;
        finishMovement("y");
    } else if (m_submode == IndentSubMode && key == '=') {
        indentRegion(m_tc.block(), m_tc.block().next());
        finishMovement();
    } else if (m_submode == ZSubMode) {
        //qDebug() << "Z_MODE " << cursorLineInDocument() << linesOnScreen();
        if (key == Key_Return || key == 't') { // cursor line to top of window
            if (!m_mvcount.isEmpty())
                m_tc.setPosition(positionForLine(count()));
            scrollToLineInDocument(cursorLineInDocument());
            if (key == Key_Return)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '.' || key == 'z') { // cursor line to center of window
            if (!m_mvcount.isEmpty())
                m_tc.setPosition(positionForLine(count()));
            scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
            if (key == '.')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '-' || key == 'b') { // cursor line to bottom of window
            if (!m_mvcount.isEmpty())
                m_tc.setPosition(positionForLine(count()));
            scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() - 1);
            if (key == '-')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            qDebug() << "IGNORED Z_MODE " << key << text;
        }
        m_submode = NoSubMode;
    } else if (m_subsubmode == FtSubSubMode) {
        m_semicolonType = m_subsubdata;
        m_semicolonKey = key;
        handleFfTt(key);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (m_submode == ReplaceSubMode) {
        if (count() < rightDist() && text.size() == 1
                && (text.at(0).isPrint() || text.at(0).isSpace())) {
            recordBeginGroup();
            setAnchor();
            moveRight(count());
            recordRemoveSelectedText();
            recordInsertText(QString(count(), text.at(0)));
            recordEndGroup();
            m_moveType = MoveExclusive;
            m_submode = NoSubMode;
            m_dotCommand = QString("%1r%2").arg(count()).arg(text);
            finishMovement();
        } else {
            m_submode = NoSubMode;
        }
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
            showRedMessage(tr("E20: Mark '%1' not set").arg(text));
        }
        m_subsubmode = NoSubSubMode;
    } else if (key >= '0' && key <= '9') {
        if (key == '0' && m_mvcount.isEmpty()) {
            moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            m_mvcount.append(QChar(key));
        }
    } else if (0 && key == ',') {
        // FIXME: fakevim uses ',' by itself, so it is incompatible
        m_subsubmode = FtSubSubMode;
        // HACK: toggle 'f' <-> 'F', 't' <-> 'T'
        m_subsubdata = m_semicolonType ^ 32;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (key == ';') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = m_semicolonType;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (key == ':') {
        enterExMode();
        m_commandBuffer.clear();
        if (m_visualMode != NoVisualMode)
            m_commandBuffer = "'<,'>";
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '/' || key == '?') {
        enterExMode(); // to get the cursor disabled
        m_mode = (key == '/') ? SearchForwardMode : SearchBackwardMode;
        m_commandBuffer.clear();
        m_searchHistory.append(QString());
        m_searchHistoryIndex = m_searchHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '`') {
        m_subsubmode = BackTickSubSubMode;
    } else if (key == '#' || key == '*') {
        // FIXME: That's not proper vim behaviour
        m_tc.select(QTextCursor::WordUnderCursor);
        QString needle = "\\<" + m_tc.selection().toPlainText() + "\\>";
        m_searchHistory.append(needle);
        m_lastSearchForward = (key == '*');
        updateMiniBuffer();
        search(needle, m_lastSearchForward);
        recordJump();
    } else if (key == '\'') {
        m_subsubmode = TickSubSubMode;
    } else if (key == '|') {
        setAnchor();
        moveToStartOfLine();
        moveRight(qMin(count(), rightDist()) - 1);
        finishMovement();
    } else if (key == '!' && m_visualMode == NoVisualMode) {
        m_submode = FilterSubMode;
    } else if (key == '!' && m_visualMode == VisualLineMode) {
        enterExMode();
        m_commandBuffer = "'<,'>!";
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '"') {
        m_submode = RegisterSubMode;
    } else if (unmodified == Key_Return) {
        moveToStartOfLine();
        moveDown();
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == Key_Home) {
        moveToStartOfLine();
        finishMovement();
    } else if (key == '$' || key == Key_End) {
        int submode = m_submode;
        moveToEndOfLine();
        finishMovement();
        if (submode == NoSubMode)
            m_desiredColumn = -1;
    } else if (key == ',') {
        // FIXME: use some other mechanism
        m_mode = PassingMode;
        updateMiniBuffer();
    } else if (key == '.') {
        qDebug() << "REPEATING" << m_dotCommand;
        for (int i = count(); --i >= 0; )
            foreach (QChar c, m_dotCommand)
                handleKey(c.unicode(), c.unicode(), QString(c));
    } else if (key == '=') {
        m_submode = IndentSubMode;
    } else if (key == '%') {
        moveToMatchingParanthesis();
        finishMovement();
    } else if (key == 'a') {
        m_mode = InsertMode;
        recordBeginGroup();
        m_lastInsertion.clear();
        if (!atEndOfLine())
            moveRight();
        updateMiniBuffer();
    } else if (key == 'A') {
        m_mode = InsertMode;
        moveToEndOfLine();
        recordBeginGroup();
        m_lastInsertion.clear();
    } else if (key == 'b') {
        m_moveType = MoveExclusive;
        moveToWordBoundary(false, false);
        finishMovement();
    } else if (key == 'B') {
        m_moveType = MoveExclusive;
        moveToWordBoundary(true, false);
        finishMovement();
    } else if (key == 'c') {
        setAnchor();
        recordBeginGroup();
        m_submode = ChangeSubMode;
    } else if (key == 'C') {
        setAnchor();
        recordBeginGroup();
        moveToEndOfLine();
        m_registers[m_register] = recordRemoveSelectedText();
        m_mode = InsertMode;
        finishMovement();
    } else if (key == 'd' && m_visualMode == NoVisualMode) {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        recordBeginGroup();
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if (key == 'd' && m_visualMode == VisualLineMode) {
        leaveVisualMode();
        int beginLine = lineForPosition(m_marks['<']);
        int endLine = lineForPosition(m_marks['>']);
        selectRange(beginLine, endLine);
        m_registers[m_register] = recordRemoveSelectedText();
    } else if (key == 'D') {
        setAnchor();
        recordBeginGroup();
        m_submode = DeleteSubMode;
        moveDown(qMax(count() - 1, 0));
        moveRight(rightDist());
        finishMovement();
    } else if (key == control('d')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        moveToFirstNonBlankOnLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'e') {
        m_moveType = MoveInclusive;
        moveToWordBoundary(false, true);
        finishMovement();
    } else if (key == 'E') {
        m_moveType = MoveInclusive;
        moveToWordBoundary(true, true);
        finishMovement();
    } else if (key == 'f' || key == 'F') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'g') {
        m_gflag = true;
    } else if (key == 'G') {
        int n = m_mvcount.isEmpty() ? linesInDocument() : count();
        m_tc.setPosition(positionForLine(n), KeepAnchor);
        if (m_config[ConfigStartOfLine] == ConfigOn)
            moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'h' || key == Key_Left
            || key == Key_Backspace || key == control('h')) {
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        moveLeft(n);
        finishMovement();
    } else if (key == 'H') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, 0)));
        moveDown(qMax(count() - 1, 0));
        moveToFirstNonBlankOnLine();
        finishMovement();
    } else if (key == 'i') {
        recordBeginGroup();
        enterInsertMode();
        updateMiniBuffer();
        if (atEndOfLine())
            moveLeft();
    } else if (key == 'I') {
        recordBeginGroup();
        setAnchor();
        enterInsertMode();
        if (m_gflag)
            moveToStartOfLine();
        else
            moveToFirstNonBlankOnLine();
    } else if (key == control('i')) {
        if (!m_jumpListRedo.isEmpty()) {
            m_jumpListUndo.append(position());
            m_tc.setPosition(m_jumpListRedo.takeLast());
        }
    } else if (key == 'j' || key == Key_Down) {
        int savedColumn = m_desiredColumn;
        if (m_submode == NoSubMode || m_submode == ZSubMode
                || m_submode == RegisterSubMode) {
            moveDown(count());
            moveToDesiredColumn();
        } else {
            moveToStartOfLine();
            moveDown(count() + 1);
        }
        finishMovement();
        m_desiredColumn = savedColumn;
    } else if (key == 'J') {
        recordBeginGroup();
        if (m_submode == NoSubMode) {
            for (int i = qMax(count(), 2) - 1; --i >= 0; ) {
                moveToEndOfLine();
                recordRemoveNextChar();
                if (!m_gflag)
                    recordInsertText(" ");
            }
            if (!m_gflag)
                moveLeft();
        }
        recordEndGroup();
    } else if (key == 'k' || key == Key_Up) {
        int savedColumn = m_desiredColumn;
        if (m_submode == NoSubMode || m_submode == ZSubMode
                || m_submode == RegisterSubMode) {
            moveUp(count());
            moveToDesiredColumn();
        } else {
            moveToStartOfLine();
            moveDown();
            moveUp(count() + 1);
        }
        finishMovement();
        m_desiredColumn = savedColumn;
    } else if (key == 'l' || key == Key_Right) {
        m_moveType = MoveExclusive;
        moveRight(qMin(count(), rightDist()));
        finishMovement();
    } else if (key == 'L') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        moveUp(qMax(count(), 1));
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
        recordJump();
    } else if (key == 'N') {
        search(lastSearchString(), !m_lastSearchForward);
        recordJump();
    } else if (key == 'o' || key == 'O') {
        recordBeginGroup();
        recordMove();
        enterInsertMode();
        moveToFirstNonBlankOnLine();
        int numSpaces = leftDist();
        if (key == 'O')
            moveUp();
        moveToEndOfLine();
        recordInsertText("\n");
        moveToStartOfLine();
        if (m_config[ConfigAutoIndent] == ConfigOn)
            recordInsertText(QString(indentDist(), ' '));
        else
            recordInsertText(QString(numSpaces, ' '));
    } else if (key == control('o')) {
        if (!m_jumpListUndo.isEmpty()) {
            m_jumpListRedo.append(position());
            m_tc.setPosition(m_jumpListUndo.takeLast());
        }
    } else if (key == 'p' || key == 'P') {
        recordBeginGroup();
        QString text = m_registers[m_register];
        int n = lineCount(text);
        //qDebug() << "REGISTERS: " << m_registers << "MOVE: " << m_moveType;
        //qDebug() << "LINES: " << n << text << m_register;
        if (n > 0) {
            recordMove();
            moveToStartOfLine();
            m_desiredColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (key == 'p')
                    moveDown();
                recordInsertText(text);
                moveUp(n);
            }
        } else {
            m_desiredColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (key == 'p')
                    moveRight();
                recordInsertText(text);
                moveLeft();
            }
        }
        recordEndGroup();
        m_dotCommand = QString("%1p").arg(count());
        finishMovement();
    } else if (key == 'r') {
        m_submode = ReplaceSubMode;
        m_dotCommand = "r";
    } else if (key == 'R') {
        // FIXME: right now we repeat the insertion count() times, 
        // but not the deletion
        recordBeginGroup();
        m_lastInsertion.clear();
        m_mode = InsertMode;
        m_submode = ReplaceSubMode;
        m_dotCommand = "R";
    } else if (key == control('r')) {
        redo();
    } else if (key == 's') {
        recordBeginGroup();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        m_registers[m_register] = recordRemoveSelectedText();
        //m_dotCommand = QString("%1s").arg(count());
        m_opcount.clear();
        m_mvcount.clear();
        enterInsertMode();
    } else if (key == 't' || key == 'T') {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == 'u') {
        undo();
    } else if (key == control('u')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveUp(linesOnScreen() / 2);
        moveToFirstNonBlankOnLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'v') {
        enterVisualMode(VisualCharMode);
    } else if (key == 'V') {
        enterVisualMode(VisualLineMode);
    } else if (key == control('v')) {
        enterVisualMode(VisualBlockMode);
    } else if (key == 'w') {
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank.
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(false, true);
            m_moveType = MoveInclusive;
        } else {
            moveToNextWord(false);
            m_moveType = MoveExclusive;
        }
        finishMovement("w");
    } else if (key == 'W') {
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(true, true);
            m_moveType = MoveInclusive;
        } else {
            moveToNextWord(true);
            m_moveType = MoveExclusive;
        }
        finishMovement("W");
    } else if (key == 'x') { // = "dl"
        m_moveType = MoveExclusive;
        if (atEndOfLine())
            moveLeft();
        recordBeginGroup();
        setAnchor();
        m_submode = DeleteSubMode;
        moveRight(qMin(count(), rightDist()));
        finishMovement("l");
    } else if (key == 'X') {
        if (leftDist() > 0) {
            setAnchor();
            moveLeft(qMin(count(), leftDist()));
            m_registers[m_register] = recordRemoveSelectedText();
        }
        finishMovement();
    } else if (key == 'y' && m_visualMode == NoVisualMode) {
        m_savedYankPosition = m_tc.position();
        if (atEndOfLine())
            moveLeft();
        recordBeginGroup();
        setAnchor();
        m_submode = YankSubMode;
    } else if (key == 'y' && m_visualMode == VisualLineMode) {
        int beginLine = lineForPosition(m_marks['<']);
        int endLine = lineForPosition(m_marks['>']);
        selectRange(beginLine, endLine);
        m_registers[m_register] = selectedText();
        m_tc.setPosition(qMin(position(), anchor()));
        moveToStartOfLine();
        leaveVisualMode();
        updateSelection();
    } else if (key == 'Y') {
        moveToStartOfLine();
        setAnchor();
        moveDown(count());
        m_moveType = MoveLineWise;
        finishMovement();
    } else if (key == 'z') {
        recordBeginGroup();
        m_submode = ZSubMode;
    } else if (key == '~' && !atEndOfLine()) {
        recordBeginGroup();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        QString str = recordRemoveSelectedText();
        for (int i = str.size(); --i >= 0; ) {
            QChar c = str.at(i);
            str[i] = c.isUpper() ? c.toLower() : c.toUpper();
        }
        recordInsertText(str);
        recordEndGroup();
    } else if (key == Key_PageDown || key == control('f')) {
        moveDown(count() * (linesOnScreen() - 2));
        finishMovement();
    } else if (key == Key_PageUp || key == control('b')) {
        moveUp(count() * (linesOnScreen() - 2));
        finishMovement();
    } else if (key == Key_Delete) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        recordRemoveSelectedText();
    } else if (key == Key_Escape) {
        if (m_visualMode != NoVisualMode)
            leaveVisualMode();
    } else {
        qDebug() << "IGNORED IN COMMAND MODE: " << key << text;
        if (text.isEmpty())
            handled = false;
    }

    return handled;
}

bool FakeVimHandler::Private::handleInsertMode(int key, int, const QString &text)
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
        recordEndGroup();
        //qDebug() << "UNDO: " << m_undoStack;
        moveLeft(qMin(1, leftDist()));
        enterCommandMode();
    } else if (key == Key_Left) {
        moveLeft(count());
        m_lastInsertion.clear();
    } else if (key == Key_Down) {
        m_submode = NoSubMode;
        moveDown(count());
        m_lastInsertion.clear();
    } else if (key == Key_Up) {
        m_submode = NoSubMode;
        moveUp(count());
        m_lastInsertion.clear();
    } else if (key == Key_Right) {
        moveRight(count());
        m_lastInsertion.clear();
    } else if (key == Key_Return) {
        m_submode = NoSubMode;
        m_tc.insertBlock();
        m_lastInsertion += "\n";
        indentRegion(m_tc.block(), m_tc.block().next());
    } else if (key == Key_Backspace || key == control('h')) {
        m_tc.deletePreviousChar();
        m_lastInsertion = m_lastInsertion.left(m_lastInsertion.size() - 1);
    } else if (key == Key_Delete) {
        m_tc.deleteChar();
        m_lastInsertion.clear();
    } else if (key == Key_PageDown || key == control('f')) {
        moveDown(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_PageUp || key == control('b')) {
        moveUp(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_Tab && m_config[ConfigExpandTab] == ConfigOn) {
        QString str = QString(m_config[ConfigTabStop].toInt(), ' ');
        m_lastInsertion.append(str);
        m_tc.insertText(str);
    } else if (key >= control('a') && key <= control('z')) {
        // ignore these
    } else if (!text.isEmpty()) {
        m_lastInsertion.append(text);
        if (m_submode == ReplaceSubMode) {
            if (atEndOfLine())
                m_submode = NoSubMode;
            else
                m_tc.deleteChar();
        }
        m_tc.insertText(text);
        if (m_config[ConfigAutoIndent] == ConfigOn
                && isElectricCharacter(text.at(0))) {
            const QString leftText = m_tc.block().text()
                .left(m_tc.position() - 1 - m_tc.block().position());
            if (leftText.simplified().isEmpty()) {
                if (m_tc.hasSelection()) {
                    QTextDocument *doc = EDITOR(document());
                    QTextBlock block = doc->findBlock(qMin(m_tc.selectionStart(),
                                m_tc.selectionEnd()));
                    const QTextBlock end = doc->findBlock(qMax(m_tc.selectionStart(),
                                m_tc.selectionEnd())).next();
                    indentRegion(block, end, text.at(0));
                } else {
                    indentCurrentLine(text.at(0));
                }
            }
        }
    } else {
        return false;
    }
    updateMiniBuffer();
    return true;
}

bool FakeVimHandler::Private::handleMiniBufferModes(int key, int unmodified,
    const QString &text)
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
    } else if (key == Key_Left) {
        // FIXME:
        if (!m_commandBuffer.isEmpty())
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (unmodified == Key_Return && m_mode == ExMode) {
        if (!m_commandBuffer.isEmpty()) {
            m_commandHistory.takeLast();
            m_commandHistory.append(m_commandBuffer);
            handleExCommand(m_commandBuffer);
            leaveVisualMode();
        }
    } else if (unmodified == Key_Return && isSearchMode()) {
        if (!m_commandBuffer.isEmpty()) {
            m_searchHistory.takeLast();
            m_searchHistory.append(m_commandBuffer);
            m_lastSearchForward = (m_mode == SearchForwardMode);
            search(lastSearchString(), m_lastSearchForward);
            recordJump();
        }
        enterCommandMode();
        updateMiniBuffer();
    } else if (key == Key_Up && isSearchMode()) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entires in the history into account.
        if (m_searchHistoryIndex > 0) {
            --m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if (key == Key_Up && m_mode == ExMode) {
        if (m_commandHistoryIndex > 0) {
            --m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if (key == Key_Down && isSearchMode()) {
        if (m_searchHistoryIndex < m_searchHistory.size() - 1) {
            ++m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if (key == Key_Down && m_mode == ExMode) {
        if (m_commandHistoryIndex < m_commandHistory.size() - 1) {
            ++m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if (key == Key_Tab) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else {
        m_commandBuffer += QChar(key);
        updateMiniBuffer();
    }
    return true;
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
    if (c == '\'' && !cmd.isEmpty()) {
        int mark = m_marks.value(cmd.at(0).unicode());
        if (!mark) { 
            showRedMessage(tr("E20: Mark '%1' not set").arg(cmd.at(0)));
            cmd = cmd.mid(1);
            return -1;
        }
        cmd = cmd.mid(1);
        QTextCursor tc = m_tc;
        tc.setPosition(mark);
        return tc.block().blockNumber() + 1;
    }
    if (c == '-') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 - (n == -1 ? 1 : n);
    }
    if (c == '+') {
        int n = readLineCode(cmd);
        return cursorLineInDocument() + 1 + (n == -1 ? 1 : n);
    }
    if (c == '\'' && !cmd.isEmpty()) {
        int pos = m_marks.value(cmd.at(0).unicode(), -1);
        //qDebug() << " MARK: " << cmd.at(0) << pos << lineForPosition(pos);
        if (pos == -1) {
            showRedMessage(tr("E20: Mark '%1' not set").arg(cmd.at(0)));
            cmd = cmd.mid(1);
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

void FakeVimHandler::Private::selectRange(int beginLine, int endLine)
{
    m_anchor = positionForLine(beginLine);
    if (endLine == linesInDocument()) {
        m_tc.setPosition(positionForLine(endLine), MoveAnchor);
        m_tc.movePosition(EndOfLine, MoveAnchor);
    } else {
        m_tc.setPosition(positionForLine(endLine + 1), MoveAnchor);
    }
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

    //qDebug() << "RANGE: " << beginLine << endLine << cmd << cmd0 << m_marks;

    static QRegExp reWrite("^w!?( (.*))?$");
    static QRegExp reDelete("^d( (.*))?$");
    static QRegExp reSet("^set?( (.*))?$");
    static QRegExp reHistory("^his(tory)?( (.*))?$");

    if (cmd.isEmpty()) {
        m_tc.setPosition(positionForLine(beginLine));
        showBlackMessage(QString());
    } else if (cmd == "q!" || cmd == "q") { // :q
        quit();
    } else if (reDelete.indexIn(cmd) != -1) { // :d
        if (beginLine == -1)
            beginLine = cursorLineInDocument();
        if (endLine == -1)
            endLine = cursorLineInDocument();
        selectRange(beginLine, endLine);
        QString reg = reDelete.cap(2);
        QString text = recordRemoveSelectedText(); 
        if (!reg.isEmpty())
            m_registers[reg.at(0).unicode()] = text;
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
        QFile file1(fileName);
        bool exists = file1.exists();
        if (exists && !forced && !noArgs) {
            showRedMessage(tr("File '%1' exists (add ! to override)").arg(fileName));
        } else if (file1.open(QIODevice::ReadWrite)) {
            file1.close();
            QTextCursor tc = m_tc;
            selectRange(beginLine, endLine);
            QString contents = selectedText(); 
            m_tc = tc;
            qDebug() << "LINES: " << beginLine << endLine;
            bool handled = false;
            emit q->writeFileRequested(&handled, fileName, contents);
            // nobody cared, so act ourselves
            if (!handled) {
                //qDebug() << "HANDLING MANUAL SAVE TO " << fileName;
                QFile::remove(fileName);
                QFile file2(fileName);
                if (file2.open(QIODevice::ReadWrite)) {
                    QTextStream ts(&file2);
                    ts << contents;
                } else {
                    showRedMessage(tr("Cannot open file '%1' for writing").arg(fileName));
                }
            }
            // check result by reading back
            QFile file3(fileName);
            file3.open(QIODevice::ReadOnly);
            QByteArray ba = file3.readAll();
            showBlackMessage(tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(exists ? " " : " [New] ")
                .arg(ba.count('\n')).arg(ba.size()));
        } else {
            showRedMessage(tr("Cannot open file '%1' for reading").arg(fileName));
        }
    } else if (cmd.startsWith("r ")) { // :r
        m_currentFileName = cmd.mid(2);
        QFile file(m_currentFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        QString data = ts.readAll();
        EDITOR(setPlainText(data));
        enterCommandMode();
        showBlackMessage(tr("\"%1\" %2L, %3C")
            .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));
    } else if (cmd.startsWith("!")) {
        if (beginLine == -1)
            beginLine = cursorLineInDocument();
        if (endLine == -1)
            endLine = cursorLineInDocument();
        selectRange(beginLine, endLine);
        QString command = cmd.mid(1).trimmed();
        recordBeginGroup();
        QString text = recordRemoveSelectedText();
        QProcess proc;
        proc.start(cmd.mid(1));
        proc.waitForStarted();
        proc.write(text.toUtf8());
        proc.closeWriteChannel();
        proc.waitForFinished();
        QString result = QString::fromUtf8(proc.readAllStandardOutput());
        recordInsertText(result);
        recordEndGroup();
        leaveVisualMode();

        m_tc.setPosition(positionForLine(beginLine));
        EditOperation op;
        // FIXME: broken for "upward selection"
        op.position = m_tc.position();
        op.from = text;
        op.to = result;
        recordOperation(op);

        enterCommandMode();
        //qDebug() << "FILTER: " << command;
        showBlackMessage(tr("%1 lines filtered").arg(text.count('\n')));
    } else if (cmd == "red" || cmd == "redo") { // :redo
        redo();
        enterCommandMode();
        updateMiniBuffer();
    } else if (reSet.indexIn(cmd) != -1) { // :set
        QString arg = reSet.cap(2);
        if (arg.isEmpty()) {
            QString info;
            foreach (const QString &key, m_config.keys())
                info += key + ": " + m_config.value(key) + "\n";
            emit q->extraInformationChanged(info);
        } else {
            notImplementedYet();
        }
        enterCommandMode();
        updateMiniBuffer();
    } else if (reHistory.indexIn(cmd) != -1) { // :history
        QString arg = reSet.cap(3);
        if (arg.isEmpty()) {
            QString info;
            info += "#  command history\n";
            int i = 0;
            foreach (const QString &item, m_commandHistory) {
                ++i;
                info += QString("%1 %2\n").arg(i, -8).arg(item);
            }
            emit q->extraInformationChanged(info);
        } else {
            notImplementedYet();
        }
        enterCommandMode();
        updateMiniBuffer();
    } else {
        showRedMessage("E492: Not an editor command: " + cmd0);
    }
}

void FakeVimHandler::Private::search(const QString &needle0, bool forward)
{
    showBlackMessage((forward ? '/' : '?') + needle0);
    QTextCursor orig = m_tc;
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (!forward)
        flags |= QTextDocument::FindBackward;

    // FIXME: Rough mapping of a common case
    QString needle = needle0;
    if (needle.startsWith("\\<") && needle.endsWith("\\>"))
        flags |= QTextDocument::FindWholeWords;
    needle.replace("\\<", ""); // start of word
    needle.replace("\\>", ""); // end of word

    //qDebug() << "NEEDLE " << needle0 << needle << "FORWARD" << forward << flags;

    if (forward)
        m_tc.movePosition(Right, MoveAnchor, 1);

    int oldLine = cursorLineInDocument() - cursorLineOnScreen();

    EDITOR(setTextCursor(m_tc));
    if (EDITOR(find(needle, flags))) {
        m_tc = EDITOR(textCursor());
        m_tc.setPosition(m_tc.anchor());
        // making this unconditional feels better, but is not "vim like"
        if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
            scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
        return;
    }

    m_tc.setPosition(forward ? 0 : lastPositionInDocument() - 1);
    EDITOR(setTextCursor(m_tc));
    if (EDITOR(find(needle, flags))) {
        m_tc = EDITOR(textCursor());
        m_tc.setPosition(m_tc.anchor());
        if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
            scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
        if (forward)
            showRedMessage("search hit BOTTOM, continuing at TOP");
        else
            showRedMessage("search hit TOP, continuing at BOTTOM");
        return;
    }

    m_tc = orig;
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine()
{
    QTextBlock block = m_tc.block();
    QTextDocument *doc = m_tc.document();
    m_tc.movePosition(StartOfLine, KeepAnchor);
    int firstPos = m_tc.position();
    for (int i = firstPos, n = firstPos + block.length(); i < n; ++i) {
        if (!doc->characterAt(i).isSpace()) {
            m_tc.setPosition(i, KeepAnchor);
            return;
        }
    }
}

int FakeVimHandler::Private::indentDist() const
{
#if 0
    // FIXME: Make independent of TextEditor
    if (!m_texteditor)
        return 0;

    TextEditor::TabSettings ts = m_texteditor->tabSettings();
    typedef SharedTools::Indenter<TextEditor::TextBlockIterator> Indenter;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(ts.m_indentSize);
    indenter.setTabSize(ts.m_tabSize);

    QTextDocument *doc = EDITOR(document());
    const TextEditor::TextBlockIterator current(m_tc.block());
    const TextEditor::TextBlockIterator begin(doc->begin());
    const TextEditor::TextBlockIterator end(m_tc.block().next());
    return indenter.indentForBottomLine(current, begin, end, QChar(' '));
#endif
    return 0;
}

void FakeVimHandler::Private::indentRegion(QTextBlock begin, QTextBlock end, QChar typedChar)
{
#if 0
    // FIXME: Make independent of TextEditor
    if (!m_texteditor)
        return 0;
    typedef SharedTools::Indenter<TextEditor::TextBlockIterator> Indenter;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(m_config[ConfigShiftWidth].toInt());
    indenter.setTabSize(m_config[ConfigTabStop].toInt());

    QTextDocument *doc = EDITOR(document());
    const TextEditor::TextBlockIterator docStart(doc->begin());
    for(QTextBlock cur = begin; cur != end; cur = cur.next()) {
        if (typedChar != 0 && cur.text().simplified().isEmpty()) {
            m_tc.setPosition(cur.position(), KeepAnchor);
            while (!m_tc.atBlockEnd())
                m_tc.deleteChar();
        } else {
            const TextEditor::TextBlockIterator current(cur);
            const TextEditor::TextBlockIterator next(cur.next());
            const int indent = indenter.indentForBottomLine(current, docStart, next, typedChar);
            ts.indentLine(cur, indent);
        }
    }
#endif
    Q_UNUSED(begin);
    Q_UNUSED(end);
    Q_UNUSED(typedChar);
}

void FakeVimHandler::Private::indentCurrentLine(QChar typedChar)
{
    indentRegion(m_tc.block(), m_tc.block().next(), typedChar);
}

void FakeVimHandler::Private::moveToDesiredColumn()
{
   if (m_desiredColumn == -1 || m_tc.block().length() <= m_desiredColumn)
       m_tc.movePosition(EndOfLine, KeepAnchor);
   else
       m_tc.setPosition(m_tc.block().position() + m_desiredColumn, KeepAnchor);
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
    int lastClass = -1;
    while (true) {
        QChar c = doc->characterAt(m_tc.position() + (forward ? 1 : -1));
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && lastClass != 0)
            --repeat;
        if (repeat == -1)
            break;
        lastClass = thisClass;
        if (m_tc.position() == n)
            break;
        forward ? moveRight() : moveLeft();
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
        moveRight();
        if (m_tc.position() == n)
            break;
    }
}

void FakeVimHandler::Private::moveToMatchingParanthesis()
{
#if 0
    // FIXME: remove TextEditor dependency
    bool undoFakeEOL = false;
    if (atEndOfLine()) {
        m_tc.movePosition(Left, KeepAnchor, 1);
        undoFakeEOL = true;
    }
    TextEditor::TextBlockUserData::MatchType match
        = TextEditor::TextBlockUserData::matchCursorForward(&m_tc);
    if (match == TextEditor::TextBlockUserData::Match) {
        if (m_submode == NoSubMode || m_submode == ZSubMode || m_submode == RegisterSubMode)
            m_tc.movePosition(Left, KeepAnchor, 1);
    } else {
        if (undoFakeEOL)
            m_tc.movePosition(Right, KeepAnchor, 1);
        if (match == TextEditor::TextBlockUserData::NoMatch) {
            // backward matching is according to the character before the cursor
            bool undoMove = false;
            if (!m_tc.atBlockEnd()) {
                m_tc.movePosition(Right, KeepAnchor, 1);
                undoMove = true;
            }
            match = TextEditor::TextBlockUserData::matchCursorBackward(&m_tc);
            if (match != TextEditor::TextBlockUserData::Match && undoMove)
                m_tc.movePosition(Left, KeepAnchor, 1);
        }
    }
#endif
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

QString FakeVimHandler::Private::selectedText() const
{
    QTextCursor tc = m_tc;
    tc.setPosition(m_anchor, KeepAnchor);
    return tc.selection().toPlainText();
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
    setAnchor();
    m_visualMode = visualMode;
    m_marks['<'] = m_tc.position();
    m_marks['>'] = m_tc.position();
    updateMiniBuffer();
    updateSelection();
}

void FakeVimHandler::Private::leaveVisualMode()
{
    m_visualMode = NoVisualMode;
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
    if (m_undoStack.isEmpty()) {
        showBlackMessage(tr("Already at oldest change"));
    } else {
        EditOperation op = m_undoStack.pop();
        //qDebug() << "UNDO " << op;
        if (op.itemCount > 0) {
            for (int i = op.itemCount; --i >= 0; )
                undo();
        } else {
            m_tc.setPosition(op.position, MoveAnchor);
            if (!op.to.isEmpty()) {
                m_tc.setPosition(op.position + op.to.size(), KeepAnchor);
                m_tc.removeSelectedText();
            }
            if (!op.from.isEmpty())
                m_tc.insertText(op.from);
            m_tc.setPosition(op.position, MoveAnchor);
        }
        m_redoStack.push(op);
        showBlackMessage(QString());
    }
}

void FakeVimHandler::Private::redo()
{
    if (m_redoStack.isEmpty()) {
        showBlackMessage(tr("Already at newest change"));
    } else {
        EditOperation op = m_redoStack.pop();
        //qDebug() << "REDO " << op;
        if (op.itemCount > 0) {
            for (int i = op.itemCount; --i >= 0; )
                redo();
        } else {
            m_tc.setPosition(op.position, MoveAnchor);
            if (!op.from.isEmpty()) {
                m_tc.setPosition(op.position + op.from.size(), KeepAnchor);
                m_tc.removeSelectedText();
            }
            if (!op.to.isEmpty())
                m_tc.insertText(op.to);
            m_tc.setPosition(op.position, MoveAnchor);
        }
        m_undoStack.push(op);
        showBlackMessage(QString());
    }
}

void FakeVimHandler::Private::recordBeginGroup()
{
    //qDebug() << "PUSH";
    m_undoGroupStack.push(m_undoStack.size());
    EditOperation op;
    op.position = m_tc.position();
    recordOperation(op);
}

void FakeVimHandler::Private::recordEndGroup()
{
    if (m_undoGroupStack.isEmpty()) {
        qWarning("fakevim: undo groups not balanced.\n");
        return;
    }
    EditOperation op;
    op.itemCount = m_undoStack.size() - m_undoGroupStack.pop();
    //qDebug() << "POP " << op.itemCount << m_undoStack;
    recordOperation(op);
}

QString FakeVimHandler::Private::recordRemoveSelectedText()
{
    EditOperation op;
    //qDebug() << "POS: " << position() << " ANCHOR: " << anchor() << m_tc.anchor();
    int pos = m_tc.position();
    if (pos == anchor())
        return QString();
    m_tc.setPosition(anchor(), MoveAnchor);
    m_tc.setPosition(pos, KeepAnchor);
    op.position = qMin(pos, anchor());
    op.from = m_tc.selection().toPlainText();
    //qDebug() << "OP: " << op;
    recordOperation(op);
    m_tc.removeSelectedText();
    return op.from;
}

void FakeVimHandler::Private::recordRemoveNextChar()
{
    m_anchor = position(); 
    moveRight();
    recordRemoveSelectedText();
}

void FakeVimHandler::Private::recordInsertText(const QString &data)
{
    EditOperation op;
    op.position = m_tc.position();
    op.to = data;
    recordOperation(op);
    m_tc.insertText(data);
}

void FakeVimHandler::Private::recordMove()
{
    EditOperation op;
    op.position = m_tc.position();
    m_undoStack.push(op);
    m_redoStack.clear();
    //qDebug() << "MOVE: " << op;
    //qDebug() << "\nSTACK: " << m_undoStack;
}

void FakeVimHandler::Private::recordOperation(const EditOperation &op)
{
    //qDebug() << "OP: " << op;
    // No need to record operations that actually do not change anything.
    if (op.from.isEmpty() && op.to.isEmpty() && op.itemCount == 0)
        return;
    // No need to create groups with only one member.
    if (op.itemCount == 1)
        return;
    m_undoStack.push(op);
    m_redoStack.clear();
    //qDebug() << "\nSTACK: " << m_undoStack;
}

void FakeVimHandler::Private::recordInsert(int position, const QString &data)
{
    EditOperation op;
    op.position = position;
    op.to = data;
    recordOperation(op);
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
    op.position = position;
    op.from = data;
    recordOperation(op);
}

void FakeVimHandler::Private::enterInsertMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));
    m_mode = InsertMode;
    m_lastInsertion.clear();
}

void FakeVimHandler::Private::enterCommandMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(true));
    m_mode = CommandMode;
}

void FakeVimHandler::Private::enterExMode()
{
    EDITOR(setCursorWidth(0));
    EDITOR(setOverwriteMode(false));
    m_mode = ExMode;
}

void FakeVimHandler::Private::quit()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));
    q->quitRequested();
}


void FakeVimHandler::Private::recordJump()
{
    m_jumpListUndo.append(position());
    m_jumpListRedo.clear();
    //qDebug() << m_jumpListUndo;
}

///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////

FakeVimHandler::FakeVimHandler(QWidget *widget, QObject *parent)
    : QObject(parent), d(new Private(this, widget))
{}

FakeVimHandler::~FakeVimHandler()
{
    delete d;
}

bool FakeVimHandler::eventFilter(QObject *ob, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress && ob == d->editor())
        return d->handleEvent(static_cast<QKeyEvent *>(ev));

    if (ev->type() == QEvent::ShortcutOverride && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        int key = kev->key();
        int mods = kev->modifiers();
        bool handleIt = (key == Qt::Key_Escape)
            || (key >= Key_A && key <= Key_Z && mods == Qt::ControlModifier);
        if (handleIt && d->handleEvent(kev)) {
            d->enterCommandMode();
            ev->accept();
            return true;
        }
    }

    return QObject::eventFilter(ob, ev);
}

void FakeVimHandler::setupWidget()
{
    d->setupWidget();
}

void FakeVimHandler::restoreWidget()
{
    d->restoreWidget();
}

void FakeVimHandler::handleCommand(const QString &cmd)
{
    d->handleExCommand(cmd);
}

void FakeVimHandler::setConfigValue(const QString &key, const QString &value)
{
    d->m_config[key] = value;
}

void FakeVimHandler::quit()
{
    d->quit();
}

void FakeVimHandler::setCurrentFileName(const QString &fileName)
{
   d->m_currentFileName = fileName;
}

QWidget *FakeVimHandler::widget()
{
    return d->editor();
}

void FakeVimHandler::setExtraData(QObject *data)
{
    d->m_extraData = data;
}

QObject *FakeVimHandler::extraData() const
{
    return d->m_extraData;
}

