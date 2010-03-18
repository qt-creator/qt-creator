/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "fakevimhandler.h"

//
// ATTENTION:
//
// 1 Please do not add any direct dependencies to other Qt Creator code here.
//   Instead emit signals and let the FakeVimPlugin channel the information to
//   Qt Creator. The idea is to keep this file here in a "clean" state that
//   allows easy reuse with any QTextEdit or QPlainTextEdit derived class.
//
// 2 There are a few auto tests located in ../../../tests/auto/fakevim.
//   Commands that are covered there are marked as "// tested" below.
//
// 3 Some conventions:
//
//   Use 1 based line numbers and 0 based column numbers. Even though
//   the 1 based line are not nice it matches vim's and QTextEdit's 'line'
//   concepts.
//
//   Do not pass QTextCursor etc around unless really needed. Convert
//   early to  line/column.
//
//   A QTextCursor is always between characters, whereas vi's cursor is always
//   over a character. FakeVim interprets the QTextCursor to be over the character
//   to the right of the QTextCursor's position().
//
//   There is always a "current" cursor (m_tc). A current "region of interest"
//   spans between m_anchor (== anchor()), i.e. the character below anchor()), and
//   m_tc.position() (== position()). The character below position() is not included
//   if the last movement command was exclusive (MoveExclusive).
//   The value of m_tc.anchor() is not used.

#include <utils/qtcassert.h>

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

#include <algorithm>
#include <climits>
#include <ctype.h>

//#define DEBUG_KEY  1
#if DEBUG_KEY
#   define KEY_DEBUG(s) qDebug() << s
#else
#   define KEY_DEBUG(s)
#endif

//#define DEBUG_UNDO  1
#if DEBUG_UNDO
#   define UNDO_DEBUG(s) qDebug() << << m_tc.document()->availableUndoSteps() << s
#else
#   define UNDO_DEBUG(s)
#endif

using namespace Utils;

namespace FakeVim {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// FakeVimHandler
//
///////////////////////////////////////////////////////////////////////

#define StartOfLine     QTextCursor::StartOfLine
#define EndOfLine       QTextCursor::EndOfLine
#define MoveAnchor      QTextCursor::MoveAnchor
#define KeepAnchor      QTextCursor::KeepAnchor
#define Up              QTextCursor::Up
#define Down            QTextCursor::Down
#define Right           QTextCursor::Right
#define Left            QTextCursor::Left
#define EndOfDocument   QTextCursor::End
#define StartOfDocument QTextCursor::Start

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
    ChangeSubMode,     // used for c
    DeleteSubMode,     // used for d
    FilterSubMode,     // used for !
    IndentSubMode,     // used for =
    RegisterSubMode,   // used for "
    ReplaceSubMode,    // used for R and r
    ShiftLeftSubMode,  // used for <
    ShiftRightSubMode, // used for >
    TransformSubMode,  // used for ~/gu/gU
    WindowSubMode,     // used for Ctrl-w
    YankSubMode,       // used for y
    ZSubMode,          // used for z
    CapitalZSubMode    // used for Z
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
    InvertCaseSubSubMode, // used for ~
    DownCaseSubSubMode, // used for gu
    UpCaseSubSubMode,   // used for gU
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

enum RangeMode
{
    RangeCharMode,  // v
    RangeLineMode,  // V
    RangeLineModeExclusive, // like above, but keep one newline when deleting
    RangeBlockMode, // Ctrl-v
    RangeBlockAndTailMode, // Ctrl-v for D and X
};

enum EventResult
{
    EventHandled,
    EventUnhandled,
    EventPassedToCore
};

struct Column
{
    Column(int p, int l) : physical(p), logical(l) {}
    int physical; // number of characters in the data
    int logical; // column on screen
};

struct CursorPosition
{
    // for jump history
    CursorPosition() : position(-1), scrollLine(-1) {}
    CursorPosition(int pos, int line) : position(pos), scrollLine(line) {}
    int position; // Position in document
    int scrollLine; // First visible line
};

struct Register
{
    Register() : rangemode(RangeCharMode) {}
    Register(const QString &c, RangeMode m) : contents(c), rangemode(m) {}
    QString contents;
    RangeMode rangemode;
};

struct Range
{
    Range()
        : beginPos(-1), endPos(-1), rangemode(RangeCharMode)
    {}

    Range(int b, int e, RangeMode m = RangeCharMode)
        : beginPos(qMin(b, e)), endPos(qMax(b, e)), rangemode(m)
    {}

    QString toString() const
    {
        return QString("%1-%2 (mode: %3)").arg(beginPos).arg(endPos)
            .arg(rangemode);
    }

    int beginPos;
    int endPos;
    RangeMode rangemode;
};

QDebug &operator<<(QDebug &ts, const QList<QTextEdit::ExtraSelection> &sels)
{
    foreach (const QTextEdit::ExtraSelection &sel, sels)
        ts << "SEL: " << sel.cursor.anchor() << sel.cursor.position();
    return ts;
}

QString quoteUnprintable(const QString &ba)
{
    QString res;
    for (int i = 0, n = ba.size(); i != n; ++i) {
        QChar c = ba.at(i);
        if (c.isPrint())
            res += c;
        else
            res += QString("\\x%1").arg(c.unicode(), 2, 16);
    }
    return res;
}

static bool startsWithWhitespace(const QString &str, int col)
{
    QTC_ASSERT(str.size() >= col, return false);
    for (int i = 0; i < col; ++i) {
        uint u = str.at(i).unicode();
        if (u != ' ' && u != '\t')
            return false;
    }
    return true;
}

inline QString msgE20MarkNotSet(const QString &text)
{
    return FakeVimHandler::tr("E20: Mark '%1' not set").arg(text);
}

class FakeVimHandler::Private
{
public:
    Private(FakeVimHandler *parent, QWidget *widget);

    EventResult handleEvent(QKeyEvent *ev);
    bool wantsOverride(QKeyEvent *ev);
    void handleCommand(const QString &cmd); // sets m_tc + handleExCommand
    void handleExCommand(const QString &cmd);
    void fixMarks(int positionAction, int positionChange); //Updates marks positions by the difference in positionChange

    void installEventFilter();
    void passShortcuts(bool enable);
    void setupWidget();
    void restoreWidget();

    friend class FakeVimHandler;
    static int shift(int key) { return key + 32; }
    static int control(int key) { return key + 256; }

    void init();
    EventResult handleKey(int key, int unmodified, const QString &text);
    EventResult handleInsertMode(int key, int unmodified, const QString &text);
    EventResult handleCommandMode(int key, int unmodified, const QString &text);
    EventResult handleRegisterMode(int key, int unmodified, const QString &text);
    EventResult handleMiniBufferModes(int key, int unmodified, const QString &text);
    void finishMovement(const QString &dotCommand = QString());
    void finishMovement(const QString &dotCommand, int count);
    void resetCommandMode();
    void search(const QString &needle, bool forward);
    void highlightMatches(const QString &needle);
    void stopIncrementalFind();

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    int leftDist() const { return m_tc.position() - m_tc.block().position(); }
    int rightDist() const { return m_tc.block().length() - leftDist() - 1; }
    bool atEndOfLine() const
        { return m_tc.atBlockEnd() && m_tc.block().length() > 1; }

    int lastPositionInDocument() const; // last valid pos in doc
    int firstPositionInLine(int line) const; // 1 based line, 0 based pos
    int lastPositionInLine(int line) const; // 1 based line, 0 based pos
    int lineForPosition(int pos) const;  // 1 based line, 0 based pos
    QString lineContents(int line) const; // 1 based line
    void setLineContents(int line, const QString &contents) const; // 1 based line

    int linesOnScreen() const;
    int columnsOnScreen() const;
    int linesInDocument() const;

    // all zero-based counting
    int cursorLineOnScreen() const;
    int cursorLineInDocument() const;
    int physicalCursorColumnInDocument() const; // as stored in the data
    int logicalCursorColumnInDocument() const; // as visible on screen
    Column cursorColumnInDocument() const; // as visible on screen
    int firstVisibleLineInDocument() const;
    void scrollToLineInDocument(int line);
    void scrollUp(int count);
    void scrollDown(int count) { scrollUp(-count); }

    CursorPosition cursorPosition() const
        { return CursorPosition(position(), firstVisibleLineInDocument()); }
    void setCursorPosition(const CursorPosition &p)
        { setPosition(p.position); scrollToLineInDocument(p.scrollLine); }

    // helper functions for indenting
    bool isElectricCharacter(QChar c) const;
    void indentSelectedText(QChar lastTyped = QChar());
    int indentText(const Range &range, QChar lastTyped = QChar());
    void shiftRegionLeft(int repeat = 1);
    void shiftRegionRight(int repeat = 1);

    void moveToFirstNonBlankOnLine();
    void moveToTargetColumn();
    void setTargetColumn() {
        m_targetColumn = leftDist();
        m_visualTargetColumn = m_targetColumn;
        //qDebug() << "TARGET: " << m_targetColumn;
    }
    void moveToNextWord(bool simple, bool deleteWord = false);
    void moveToMatchingParanthesis();
    void moveToWordBoundary(bool simple, bool forward, bool changeWord = false);

    // to reduce line noise
    void moveToEndOfDocument() { m_tc.movePosition(EndOfDocument, MoveAnchor); }
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveBehindEndOfLine();
    void moveUp(int n = 1) { moveDown(-n); }
    void moveDown(int n = 1); // { m_tc.movePosition(Down, MoveAnchor, n); }
    void moveRight(int n = 1) { m_tc.movePosition(Right, MoveAnchor, n); }
    void moveLeft(int n = 1) { m_tc.movePosition(Left, MoveAnchor, n); }
    void setAnchor() { if (!isVisualMode()) m_anchor = m_tc.position(); }
    void setAnchor(int position) { if (!isVisualMode()) m_anchor = position; }
    void setPosition(int position) { m_tc.setPosition(position, MoveAnchor); }

    bool handleFfTt(int key);

    // helper function for handleExCommand. return 1 based line index.
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
    QWidget *editor() const;
    QChar characterAtCursor() const
        { return m_tc.document()->characterAt(m_tc.position()); }
    void beginEditBlock() { UNDO_DEBUG("BEGIN EDIT BLOCK"); m_tc.beginEditBlock(); }
    void beginEditBlock(int pos) { setUndoPosition(pos); beginEditBlock(); }
    void endEditBlock() { UNDO_DEBUG("END EDIT BLOCK"); m_tc.endEditBlock(); }
    void joinPreviousEditBlock() { UNDO_DEBUG("JOIN EDIT BLOCK"); m_tc.joinPreviousEditBlock(); }

    // this asks the layer above (e.g. the fake vim plugin or the
    // stand-alone test application to handle the command)
    void passUnknownExCommand(const QString &cmd);
    // this asks the layer above (e.g. the fake vim plugin or the
    // stand-alone test application to handle the set command)
    void passUnknownSetCommand(const QString &cmd);

    bool isVisualMode() const { return m_visualMode != NoVisualMode; }
    bool isNoVisualMode() const { return m_visualMode == NoVisualMode; }
    bool isVisualCharMode() const { return m_visualMode == VisualCharMode; }
    bool isVisualLineMode() const { return m_visualMode == VisualLineMode; }
    bool isVisualBlockMode() const { return m_visualMode == VisualBlockMode; }
    void updateEditor();

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    bool m_wasReadOnly; // saves read-only state of document

    FakeVimHandler *q;
    Mode m_mode;
    bool m_passing; // let the core see the next event
    SubMode m_submode;
    SubSubMode m_subsubmode;
    int m_subsubdata;
    QString m_input;
    QTextCursor m_tc;
    int m_oldPosition; // copy from last event to check for external changes
    int m_anchor;
    static QHash<int, Register> m_registers;
    int m_register;
    QString m_mvcount;
    QString m_opcount;
    MoveType m_movetype;
    RangeMode m_rangemode;
    int m_visualInsertCount;

    bool m_fakeEnd;
    bool m_anchorPastEnd;
    bool m_positionPastEnd; // '$' & 'l' in visual mode can move past eol

    bool isSearchMode() const
        { return m_mode == SearchForwardMode || m_mode == SearchBackwardMode; }
    int m_gflag;  // whether current command started with 'g'

    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    bool m_findPending;
    QString m_lastInsertion;

    int anchor() const { return m_anchor; }
    int position() const { return m_tc.position(); }

    typedef void (FakeVimHandler::Private::*Transformation)(int, QTextCursor *);
    void transformText(const Range &range, Transformation transformation);

    void removeSelectedText(bool exclusive = false);
    void removeText(const Range &range);
    void removeTransform(int, QTextCursor *);

    void invertCaseSelectedText();
    void invertCaseTransform(int, QTextCursor *);

    void upCaseSelectedText();
    void upCaseTransform(int, QTextCursor *);

    void downCaseSelectedText();
    void downCaseTransform(int, QTextCursor *);

    QString selectedText() const { return text(Range(position(), anchor())); }
    QString text(const Range &range) const;

    void yankSelectedText();
    void yankText(const Range &range, int toregister = '"');

    void pasteText(bool afterCursor);

    // undo handling
    void undo();
    void redo();
    void setUndoPosition(int pos);
    QMap<int, int> m_undoCursorPosition; // revision -> position
    bool m_beginEditBlock;

    // extra data for '.'
    void replay(const QString &text, int count);
    void setDotCommand(const QString &cmd) { m_dotCommand = cmd; }
    void setDotCommand(const QString &cmd, int n) { m_dotCommand = cmd.arg(n); }
    QString m_dotCommand;
    bool m_inReplay; // true if we are executing a '.'

    // extra data for ';'
    QString m_semicolonCount;
    int m_semicolonType;  // 'f', 'F', 't', 'T'
    int m_semicolonKey;

    // history for '/'
    QString lastSearchString() const;
    static QStringList m_searchHistory;
    int m_searchHistoryIndex;

    // history for ':'
    static QStringList m_commandHistory;
    int m_commandHistoryIndex;

    // visual line mode
    void enterVisualMode(VisualMode visualMode);
    void leaveVisualMode();
    VisualMode m_visualMode;

    // marks as lines
    QHash<int, int> m_marks;
    QString m_oldNeedle;

    // vi style configuration
    QVariant config(int code) const { return theFakeVimSetting(code)->value(); }
    bool hasConfig(int code) const { return config(code).toBool(); }
    bool hasConfig(int code, const char *value) const // FIXME
        { return config(code).toString().contains(value); }

    int m_targetColumn; // -1 if past end of line
    int m_visualTargetColumn; // 'l' can move past eol in visual mode only

    int m_cursorWidth;

    // auto-indent
    QString tabExpand(int len) const;
    Column indentation(const QString &line) const;
    void insertAutomaticIndentation(bool goingDown);
    bool removeAutomaticIndentation(); // true if something removed
    // number of autoindented characters
    int m_justAutoIndented;
    void handleStartOfLine();

    void recordJump();
    void recordNewUndo();
    QVector<CursorPosition> m_jumpListUndo;
    QVector<CursorPosition> m_jumpListRedo;

    QList<QTextEdit::ExtraSelection> m_searchSelections;
};

QStringList FakeVimHandler::Private::m_searchHistory;
QStringList FakeVimHandler::Private::m_commandHistory;
QHash<int, Register> FakeVimHandler::Private::m_registers;

FakeVimHandler::Private::Private(FakeVimHandler *parent, QWidget *widget)
{
    q = parent;
    m_textedit = qobject_cast<QTextEdit *>(widget);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    init();
}

void FakeVimHandler::Private::init()
{
    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_passing = false;
    m_findPending = false;
    m_fakeEnd = false;
    m_positionPastEnd = m_anchorPastEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_visualMode = NoVisualMode;
    m_targetColumn = 0;
    m_visualTargetColumn = 0;
    m_movetype = MoveInclusive;
    m_anchor = 0;
    m_cursorWidth = EDITOR(cursorWidth());
    m_inReplay = false;
    m_justAutoIndented = 0;
    m_rangemode = RangeCharMode;
    m_beginEditBlock = true;
}

bool FakeVimHandler::Private::wantsOverride(QKeyEvent *ev)
{
    const int key = ev->key();
    const int mods = ev->modifiers();
    KEY_DEBUG("SHORTCUT OVERRIDE" << key << "  PASSING: " << m_passing);

    if (key == Key_Escape) {
        // Not sure this feels good. People often hit Esc several times
        if (isNoVisualMode() && m_mode == CommandMode)
            return false;
        return true;
    }

    // We are interested in overriding  most Ctrl key combinations
    if (mods == Qt::ControlModifier
            && ((key >= Key_A && key <= Key_Z && key != Key_K)
                || key == Key_BracketLeft || key == Key_BracketRight)) {
        // Ctrl-K is special as it is the Core's default notion of Locator
        if (m_passing) {
            KEY_DEBUG(" PASSING CTRL KEY");
            // We get called twice on the same key
            //m_passing = false;
            return false;
        }
        KEY_DEBUG(" NOT PASSING CTRL KEY");
        //updateMiniBuffer();
        return true;
    }

    // Let other shortcuts trigger
    return false;
}

EventResult FakeVimHandler::Private::handleEvent(QKeyEvent *ev)
{
    int key = ev->key();
    const int um = key; // keep unmodified key around
    const int mods = ev->modifiers();

    if (key == Key_Shift || key == Key_Alt || key == Key_Control
            || key == Key_Alt || key == Key_AltGr || key == Key_Meta)
    {
        KEY_DEBUG("PLAIN MODIFIER");
        return EventUnhandled;
    }

    if (m_passing) {
        passShortcuts(false);
        KEY_DEBUG("PASSING PLAIN KEY..." << ev->key() << ev->text());
        //if (key == ',') { // use ',,' to leave, too.
        //    qDebug() << "FINISHED...";
        //    return EventHandled;
        //}
        m_passing = false;
        updateMiniBuffer();
        KEY_DEBUG("   PASS TO CORE");
        return EventPassedToCore;
    }

    // Fake "End of line"
    m_tc = EDITOR(textCursor());

    // Position changed externally
    if (m_tc.position() != m_oldPosition) {
        setTargetColumn();
        if (m_mode == InsertMode) {
            int dist = m_tc.position() - m_oldPosition;
            // Try to compensate for code completion
            if (dist > 0 && dist <= physicalCursorColumnInDocument()) {
                Range range(m_oldPosition, m_tc.position());
                m_lastInsertion.append(text(range));
            }
        } else if (!isVisualMode()) {
            if (atEndOfLine())
                moveLeft();
        }
    }

    m_tc.setVisualNavigation(true);

    if (m_fakeEnd)
        moveRight();

    if ((mods & Qt::ControlModifier) != 0) {
        if (key >= Key_A && key <= Key_Z)
            key = shift(key); // make it lower case
        key = control(key);
    } else if (key >= Key_A && key <= Key_Z && (mods & Qt::ShiftModifier) == 0) {
        key = shift(key);
    }

    QTC_ASSERT(!(m_mode != InsertMode && m_tc.atBlockEnd() && m_tc.block().length() > 1),
               qDebug() << "Cursor at EOL before key handler");

    //if (m_mode == InsertMode)
    //    joinPreviousEditBlock();
    //else
    //    beginEditBlock();
    EventResult result = handleKey(key, um, ev->text());
    //endEditBlock();

    // the command might have destroyed the editor
    if (m_textedit || m_plaintextedit) {
        // We fake vi-style end-of-line behaviour
        m_fakeEnd = atEndOfLine() && m_mode == CommandMode && !isVisualBlockMode();

        QTC_ASSERT(!(m_mode != InsertMode && m_tc.atBlockEnd() && m_tc.block().length() > 1),
                   qDebug() << "Cursor at EOL after key handler");

        if (m_fakeEnd)
            moveLeft();

        EDITOR(setTextCursor(m_tc));
        m_oldPosition = m_tc.position();
    }
    return result;
}

void FakeVimHandler::Private::installEventFilter()
{
    EDITOR(installEventFilter(q));
    updateEditor();
}

void FakeVimHandler::Private::setupWidget()
{
    enterCommandMode();
    //EDITOR(setCursorWidth(QFontMetrics(ed->font()).width(QChar('x')));
    if (m_textedit) {
        m_textedit->setLineWrapMode(QTextEdit::NoWrap);
    } else if (m_plaintextedit) {
        m_plaintextedit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    m_wasReadOnly = EDITOR(isReadOnly());
    //EDITOR(setReadOnly(true));

    updateEditor();

    QTextCursor tc = EDITOR(textCursor());
    if (tc.hasSelection()) {
        int pos = tc.position();
        int anc = tc.anchor();
        m_marks['<'] = anc;
        m_marks['>'] = pos;
        m_anchor = anc;
        m_visualMode = VisualCharMode;
        tc.clearSelection();
        EDITOR(setTextCursor(tc));
        m_tc = tc; // needed in updateSelection
        updateSelection();
    }

    //showBlackMessage("vi emulation mode. Type :q to leave. Use , Ctrl-R to trigger run.");
    updateMiniBuffer();
}

void FakeVimHandler::Private::updateEditor()
{
    const int charWidth = QFontMetrics(EDITOR(font())).width(QChar(' '));
    EDITOR(setTabStopWidth(charWidth * config(ConfigTabStop).toInt()));
}

void FakeVimHandler::Private::restoreWidget()
{
    //showBlackMessage(QString());
    //updateMiniBuffer();
    //EDITOR(removeEventFilter(q));
    EDITOR(setReadOnly(m_wasReadOnly));
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));

    if (isVisualLineMode()) {
        m_tc = EDITOR(textCursor());
        int beginLine = lineForPosition(m_marks['<']);
        int endLine = lineForPosition(m_marks['>']);
        m_tc.setPosition(firstPositionInLine(beginLine), MoveAnchor);
        m_tc.setPosition(lastPositionInLine(endLine), KeepAnchor);
        EDITOR(setTextCursor(m_tc));
    } else if (isVisualCharMode()) {
        m_tc = EDITOR(textCursor());
        m_tc.setPosition(m_marks['<'], MoveAnchor);
        m_tc.setPosition(m_marks['>'], KeepAnchor);
        EDITOR(setTextCursor(m_tc));
    }

    m_visualMode = NoVisualMode;
    updateSelection();
}

EventResult FakeVimHandler::Private::handleKey(int key, int unmodified,
    const QString &text)
{
    setUndoPosition(m_tc.position());
    if (m_mode == InsertMode)
        return handleInsertMode(key, unmodified, text);
    if (m_mode == CommandMode)
        return handleCommandMode(key, unmodified, text);
    if (m_mode == ExMode || m_mode == SearchForwardMode
            || m_mode == SearchBackwardMode)
        return handleMiniBufferModes(key, unmodified, text);
    return EventUnhandled;
}

void FakeVimHandler::Private::stopIncrementalFind()
{
    if (m_findPending) {
        m_findPending = false;
        QTextCursor tc = EDITOR(textCursor());
        tc.setPosition(tc.selectionStart());
        EDITOR(setTextCursor(tc));
    }
}

void FakeVimHandler::Private::setUndoPosition(int pos)
{
    //qDebug() << " CURSOR POS: " << m_undoCursorPosition;
    m_undoCursorPosition[m_tc.document()->availableUndoSteps()] = pos;
}

void FakeVimHandler::Private::moveDown(int n)
{
#if 0
    // does not work for "hidden" documents like in the autotests
    m_tc.movePosition(Down, MoveAnchor, n);
#else
    const int col = m_tc.position() - m_tc.block().position();
    const int lastLine = m_tc.document()->lastBlock().blockNumber();
    const int targetLine = qMax(0, qMin(lastLine, m_tc.block().blockNumber() + n));
    const QTextBlock &block = m_tc.document()->findBlockByNumber(targetLine);
    const int pos = block.position();
    setPosition(pos + qMax(0, qMin(block.length() - 2, col)));
    moveToTargetColumn();
#endif
}

void FakeVimHandler::Private::moveToEndOfLine()
{
#if 0
    // does not work for "hidden" documents like in the autotests
    m_tc.movePosition(EndOfLine, MoveAnchor);
#else
    const QTextBlock &block = m_tc.block();
    const int pos = block.position() + block.length() - 2;
    setPosition(qMax(block.position(), pos));
#endif
}

void FakeVimHandler::Private::moveBehindEndOfLine()
{
    const QTextBlock &block = m_tc.block();
    int pos = qMin(block.position() + block.length() - 1, lastPositionInDocument());
    setPosition(pos);
}

void FakeVimHandler::Private::moveToStartOfLine()
{
#if 0
    // does not work for "hidden" documents like in the autotests
    m_tc.movePosition(StartOfLine, MoveAnchor);
#else
    const QTextBlock &block = m_tc.block();
    setPosition(block.position());
#endif
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand, int count)
{
    finishMovement(dotCommand.arg(count));
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand)
{
    //qDebug() << "ANCHOR: " << position() << anchor();
    if (m_submode == FilterSubMode) {
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        setPosition(qMin(anchor(), position()));
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer = QString(".,+%1!").arg(qAbs(endLine - beginLine));
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
        return;
    }

    if (isVisualMode())
        m_marks['>'] = m_tc.position();

    if (m_submode == ChangeSubMode
        || m_submode == DeleteSubMode
        || m_submode == YankSubMode
        || m_submode == TransformSubMode) {
        if (m_submode != YankSubMode)
            beginEditBlock();
        if (m_movetype == MoveLineWise)
            m_rangemode = (m_submode == ChangeSubMode)
                ? RangeLineModeExclusive
                : RangeLineMode;

        if (m_movetype == MoveInclusive) {
            if (anchor() <= position()) {
                if ( !m_tc.atBlockEnd())
                    moveRight(); // correction
            } else {
                m_anchor++;
            }
        }

        if (m_positionPastEnd) {
            moveBehindEndOfLine();
            moveRight();
        }

        if (m_anchorPastEnd) {
            m_anchor++;
        }

        if (m_submode != TransformSubMode) {
            yankSelectedText();
            if (m_movetype == MoveLineWise)
                m_registers[m_register].rangemode = RangeLineMode;
        }

        m_positionPastEnd = m_anchorPastEnd = false;
    }

    if (m_submode == ChangeSubMode) {
        removeSelectedText(true);
        if (!dotCommand.isEmpty())
            setDotCommand(QLatin1Char('c') + dotCommand);
        if (m_movetype == MoveLineWise) {
            insertAutomaticIndentation(true);
        }
        endEditBlock();
        enterInsertMode();
        m_beginEditBlock = false;
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        removeSelectedText();
        if (!dotCommand.isEmpty())
            setDotCommand(QLatin1Char('d') + dotCommand);
        if (m_movetype == MoveLineWise)
            handleStartOfLine();
        m_submode = NoSubMode;
        if (atEndOfLine())
            moveLeft();
        else
            setTargetColumn();
        endEditBlock();
    } else if (m_submode == YankSubMode) {
        m_submode = NoSubMode;
        const int la = lineForPosition(anchor());
        const int lp = lineForPosition(position());
        if (m_register != '"') {
            setPosition(m_marks[m_register]);
            moveToStartOfLine();
        } else {
            if (anchor() <= position())
                setPosition(anchor());
        }
        if (la != lp)
            showBlackMessage(QString("%1 lines yanked").arg(qAbs(la - lp) + 1));
    } else if (m_submode == TransformSubMode) {
        if (m_subsubmode == InvertCaseSubSubMode) {
            invertCaseSelectedText();
            if (!dotCommand.isEmpty())
                setDotCommand(QLatin1Char('~') + dotCommand);
        } else if (m_subsubmode == UpCaseSubSubMode) {
            upCaseSelectedText();
            if (!dotCommand.isEmpty())
                setDotCommand("gU" + dotCommand);
        } else if (m_subsubmode == DownCaseSubSubMode) {
            downCaseSelectedText();
            if (!dotCommand.isEmpty())
                setDotCommand("gu" + dotCommand);
        }
        m_submode = NoSubMode;
        m_subsubmode = NoSubSubMode;
        setPosition(qMin(anchor(), position()));
        if (m_movetype == MoveLineWise)
            handleStartOfLine();
        endEditBlock();
    } else if (m_submode == ReplaceSubMode) {
        m_submode = NoSubMode;
    } else if (m_submode == IndentSubMode) {
        recordJump();
        beginEditBlock();
        indentSelectedText();
        endEditBlock();
        m_submode = NoSubMode;
        updateMiniBuffer();
    } else if (m_submode == ShiftRightSubMode) {
        recordJump();
        shiftRegionRight(1);
        m_submode = NoSubMode;
        updateMiniBuffer();
    } else if (m_submode == ShiftLeftSubMode) {
        recordJump();
        shiftRegionLeft(1);
        m_submode = NoSubMode;
        updateMiniBuffer();
    }

    resetCommandMode();
}

void FakeVimHandler::Private::resetCommandMode()
{
    m_movetype = MoveInclusive;
    m_mvcount.clear();
    m_opcount.clear();
    m_gflag = false;
    m_register = '"';
    m_tc.clearSelection();
    m_rangemode = RangeCharMode;

    updateSelection();
    updateMiniBuffer();
}

void FakeVimHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections = m_searchSelections;
    if (isVisualMode()) {
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
        if (isVisualCharMode()) {
            sel.cursor.setPosition(qMin(cursorPos, anchorPos), MoveAnchor);
            sel.cursor.setPosition(qMax(cursorPos, anchorPos) + 1, KeepAnchor);
            selections.append(sel);
        } else if (isVisualLineMode()) {
            sel.cursor.setPosition(qMin(cursorPos, anchorPos), MoveAnchor);
            sel.cursor.movePosition(StartOfLine, MoveAnchor);
            sel.cursor.setPosition(qMax(cursorPos, anchorPos), KeepAnchor);
            sel.cursor.movePosition(EndOfLine, KeepAnchor);
            selections.append(sel);
        } else if (isVisualBlockMode()) {
            QTextCursor tc = m_tc;
            tc.setPosition(anchorPos);
            int anchorColumn = tc.columnNumber();
            int cursorColumn = m_tc.columnNumber();
            int anchorRow    = tc.blockNumber();
            int cursorRow    = m_tc.blockNumber();
            int startColumn  = qMin(anchorColumn, cursorColumn);
            int endColumn    = qMax(anchorColumn, cursorColumn);
            int diffRow      = cursorRow - anchorRow;
            if (anchorRow > cursorRow) {
                tc.setPosition(cursorPos);
                diffRow = -diffRow;
            }
            tc.movePosition(StartOfLine, MoveAnchor);
            for (int i = 0; i <= diffRow; ++i) {
                if (startColumn < tc.block().length() - 1) {
                    int last = qMin(tc.block().length(), endColumn + 1);
                    int len = last - startColumn;
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
    if (!m_textedit && !m_plaintextedit)
        return;

    QString msg;
    if (m_passing) {
        msg = "-- PASSING --  ";
    } else if (!m_currentMessage.isEmpty()) {
        msg = m_currentMessage;
    } else if (m_mode == CommandMode && isVisualMode()) {
        if (isVisualCharMode()) {
            msg = "-- VISUAL --";
        } else if (isVisualLineMode()) {
            msg = "-- VISUAL LINE --";
        } else if (isVisualBlockMode()) {
            msg = "-- VISUAL BLOCK --";
        }
    } else if (m_mode == InsertMode) {
        if (m_submode == ReplaceSubMode)
            msg = "-- REPLACE --";
        else
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
    const QString pos = QString::fromLatin1("%1,%2").arg(l + 1).arg(physicalCursorColumnInDocument() + 1);
    // FIXME: physical "-" logical
    if (linesInDoc != 0) {
        status = FakeVimHandler::tr("%1%2%").arg(pos, -10).arg(l * 100 / linesInDoc, 4);
    } else {
        status = FakeVimHandler::tr("%1All").arg(pos, -10);
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
    qDebug() << "Not implemented in FakeVim";
    showRedMessage(FakeVimHandler::tr("Not implemented in FakeVim"));
    updateMiniBuffer();
}

void FakeVimHandler::Private::passShortcuts(bool enable)
{
    m_passing = enable;
    updateMiniBuffer();
    if (enable)
        QCoreApplication::instance()->installEventFilter(q);
    else
        QCoreApplication::instance()->removeEventFilter(q);
}

EventResult FakeVimHandler::Private::handleCommandMode(int key, int unmodified,
    const QString &text)
{
    EventResult handled = EventHandled;

    if (m_subsubmode == FtSubSubMode) {
        m_semicolonType = m_subsubdata;
        m_semicolonKey = key;
        bool valid = handleFfTt(key);
        m_subsubmode = NoSubSubMode;
        if (!valid) {
            m_submode = NoSubMode;
            finishMovement();
        } else {
            finishMovement(QString("%1%2%3")
                           .arg(count())
                           .arg(QChar(m_semicolonType))
                           .arg(QChar(m_semicolonKey)));
        }
    } else if (m_submode == WindowSubMode) {
        emit q->windowCommandRequested(key);
        m_submode = NoSubMode;
    } else if (m_submode == RegisterSubMode) {
        m_register = key;
        m_submode = NoSubMode;
        m_rangemode = RangeLineMode;
    } else if (m_submode == ChangeSubMode && key == 'c') { // tested
        moveToStartOfLine();
        setAnchor();
        moveDown(count() - 1);
        moveToEndOfLine();
        m_movetype = MoveLineWise;
        m_lastInsertion.clear();
        setDotCommand("%1cc", count());
        finishMovement();
    } else if (m_submode == DeleteSubMode && key == 'd') { // tested
        m_movetype = MoveLineWise;
        int endPos = firstPositionInLine(lineForPosition(position()) + count() - 1);
        Range range(position(), endPos, RangeLineMode);
        yankText(range);
        removeText(range);
        setDotCommand("%1dd", count());
        m_submode = NoSubMode;
        handleStartOfLine();
        setTargetColumn();
        finishMovement();
    } else if (m_submode == ShiftLeftSubMode && key == '<') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1<<", count());
        finishMovement();
    } else if (m_submode == ShiftRightSubMode && key == '>') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1>>", count());
        finishMovement();
    } else if (m_submode == IndentSubMode && key == '=') {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1==", count());
        finishMovement();
    } else if (m_submode == ZSubMode) {
        //qDebug() << "Z_MODE " << cursorLineInDocument() << linesOnScreen();
        if (key == Key_Return || key == 't') { // cursor line to top of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(- cursorLineOnScreen());
            if (key == Key_Return)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '.' || key == 'z') { // cursor line to center of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() / 2 - cursorLineOnScreen());
            if (key == '.')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (key == '-' || key == 'b') { // cursor line to bottom of window
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() - cursorLineOnScreen());
            if (key == '-')
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            qDebug() << "IGNORED Z_MODE " << key << text;
        }
        m_submode = NoSubMode;
    } else if (m_submode == CapitalZSubMode) {
        // Recognize ZZ and ZQ as aliases for ":x" and ":q!".
        m_submode = NoSubMode;
        if (key == 'Z')
            handleExCommand(QString(QLatin1Char('x')));
        else if (key == 'Q')
            handleExCommand("q!");
    } else if (m_submode == ReplaceSubMode) {
        if (count() <= (rightDist() + atEndOfLine()) && text.size() == 1
                && (text.at(0).isPrint() || text.at(0).isSpace())) {
            if (atEndOfLine())
                moveLeft();
            setAnchor();
            moveRight(count());
            removeSelectedText();
            m_tc.insertText(QString(count(), text.at(0)));
            m_movetype = MoveExclusive;
            setDotCommand("%1r" + text, count());
            moveLeft();
        }
        setTargetColumn();
        m_submode = NoSubMode;
        finishMovement();
    } else if (m_subsubmode == MarkSubSubMode) {
        m_marks[key] = m_tc.position();
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        if (m_marks.contains(key)) {
            setPosition(m_marks[key]);
            if (m_subsubmode == TickSubSubMode)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            showRedMessage(msgE20MarkNotSet(text));
        }
        m_subsubmode = NoSubSubMode;
    } else if (key >= '0' && key <= '9') {
        if (key == '0' && m_mvcount.isEmpty()) {
            m_movetype = MoveExclusive;
            moveToStartOfLine();
            setTargetColumn();
            finishMovement(QString(QLatin1Char('0')));
        } else {
            m_mvcount.append(QChar(key));
        }
    } else if (key == '^' || key == '_') {
        moveToFirstNonBlankOnLine();
        setTargetColumn();
        m_movetype = MoveExclusive;
        finishMovement(QString(QLatin1Char(key)));
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
        m_currentMessage.clear();
        m_commandBuffer.clear();
        if (isVisualMode())
            m_commandBuffer = "'<,'>";
        m_commandHistory.append(QString());
        m_commandHistoryIndex = m_commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (key == '/' || key == '?') {
        if (hasConfig(ConfigIncSearch)) {
            // re-use the core dialog.
            m_findPending = true;
            m_lastSearchForward = (key == '/');
            EDITOR(setTextCursor(m_tc));
            emit q->findRequested(!m_lastSearchForward);
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.selectionStart());
        } else {
            // FIXME: make core find dialog sufficiently flexible to
            // produce the "default vi" behaviour too. For now, roll our own.
            enterExMode(); // to get the cursor disabled
            m_currentMessage.clear();
            m_mode = (key == '/') ? SearchForwardMode : SearchBackwardMode;
            m_commandBuffer.clear();
            m_searchHistory.append(QString());
            m_searchHistoryIndex = m_searchHistory.size() - 1;
            updateMiniBuffer();
        }
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
        moveToStartOfLine();
        moveRight(qMin(count(), rightDist()) - 1);
        setTargetColumn();
        finishMovement();
    } else if (key == '!' && isNoVisualMode()) {
        m_submode = FilterSubMode;
    } else if (key == '!' && isVisualMode()) {
        enterExMode();
        m_currentMessage.clear();
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
        m_movetype = MoveLineWise;
        finishMovement("%1j", count());
    } else if (key == '-') {
        moveToStartOfLine();
        moveUp(count());
        moveToFirstNonBlankOnLine();
        m_movetype = MoveLineWise;
        finishMovement("%1-", count());
    } else if (key == '+') {
        moveToStartOfLine();
        moveDown(count());
        moveToFirstNonBlankOnLine();
        m_movetype = MoveLineWise;
        finishMovement("%1+", count());
    } else if (key == Key_Home) {
        moveToStartOfLine();
        setTargetColumn();
        finishMovement();
    } else if (key == '$' || key == Key_End) {
        if (count() > 1)
            moveDown(count() - 1);
        moveToEndOfLine();
        m_movetype = MoveInclusive;
        setTargetColumn();
        if (m_submode == NoSubMode)
            m_targetColumn = -1;
        if (isVisualMode())
            m_visualTargetColumn = -1;
        finishMovement("%1$", count());
    } else if (key == ',') {
        passShortcuts(true);
    } else if (key == '.') {
        //qDebug() << "REPEATING" << quoteUnprintable(m_dotCommand) << count();
        QString savedCommand = m_dotCommand;
        m_dotCommand.clear();
        replay(savedCommand, count());
        enterCommandMode();
        m_dotCommand = savedCommand;
    } else if (key == '<' && isNoVisualMode()) {
        m_submode = ShiftLeftSubMode;
    } else if (key == '<' && isVisualMode()) {
        shiftRegionLeft(1);
        leaveVisualMode();
    } else if (key == '>' && isNoVisualMode()) {
        m_submode = ShiftRightSubMode;
    } else if (key == '>' && isVisualMode()) {
        shiftRegionRight(1);
        leaveVisualMode();
    } else if (key == '=' && isNoVisualMode()) {
        m_submode = IndentSubMode;
    } else if (key == '=' && isVisualMode()) {
        beginEditBlock();
        indentSelectedText();
        endEditBlock();
        leaveVisualMode();
    } else if (key == '%') {
        setAnchor();
        moveToMatchingParanthesis();
        finishMovement();
    } else if (key == 'a') {
        enterInsertMode();
        m_lastInsertion.clear();
        if (!atEndOfLine())
            moveRight();
        updateMiniBuffer();
    } else if (key == 'A') {
        enterInsertMode();
        moveBehindEndOfLine();
        setDotCommand(QString(QLatin1Char('A')));
        m_lastInsertion.clear();
        updateMiniBuffer();
    } else if (key == control('a')) {
        // FIXME: eat it to prevent the global "select all" shortcut to trigger
    } else if (key == 'b') {
        m_movetype = MoveExclusive;
        moveToWordBoundary(false, false);
        finishMovement();
    } else if (key == 'B') {
        m_movetype = MoveExclusive;
        moveToWordBoundary(true, false);
        finishMovement();
    } else if (key == 'c' && isNoVisualMode()) {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = ChangeSubMode;
    } else if (key == 'c' && (isVisualCharMode() || isVisualLineMode())) {
        m_rangemode = isVisualCharMode() ? RangeCharMode : RangeLineMode;
        if (isVisualLineMode()) {
            m_movetype =  MoveLineWise;
        }
        leaveVisualMode();
        m_submode = ChangeSubMode;
        finishMovement();
    } else if (key == 'C') {
        setAnchor();
        moveToEndOfLine();
        m_submode = ChangeSubMode;
        setDotCommand(QString(QLatin1Char('C')));
        finishMovement();
    } else if (key == control('c')) {
        if (isNoVisualMode())
            showBlackMessage("Type Alt-v,Alt-v  to quit FakeVim mode");
        else
            leaveVisualMode();
    } else if (key == 'd' && isNoVisualMode()) {
        if (m_rangemode == RangeLineMode) {
            int pos = m_tc.position();
            moveToEndOfLine();
            setAnchor();
            setPosition(pos);
        } else {
            setAnchor();
        }
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if ((key == 'd' || key == 'x') && isVisualCharMode()) {
        leaveVisualMode();
        m_submode = DeleteSubMode;
        finishMovement();
    } else if ((key == 'd' || key == 'x') && isVisualLineMode()) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        yankSelectedText();
        removeSelectedText();
        handleStartOfLine();
    } else if ((key == 'd' || key == 'x') && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (key == 'D' && isNoVisualMode()) {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = DeleteSubMode;
        moveDown(qMax(count() - 1, 0));
        m_movetype = MoveInclusive;
        moveToEndOfLine();
        setDotCommand(QString(QLatin1Char('D')));
        finishMovement();
    } else if ((key == 'D' || key == 'X') &&
         (isVisualCharMode() || isVisualLineMode())) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        m_submode = NoSubMode;
        yankSelectedText();
        removeSelectedText();
        moveToFirstNonBlankOnLine();
    } else if ((key == 'D' || key == 'X') && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockAndTailMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (key == control('d')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'e') { // tested
        m_movetype = MoveInclusive;
        moveToWordBoundary(false, true);
        finishMovement("%1e", count());
    } else if (key == 'E') {
        m_movetype = MoveInclusive;
        moveToWordBoundary(true, true);
        finishMovement("%1E", count());
    } else if (key == control('e')) {
        // FIXME: this should use the "scroll" option, and "count"
        if (cursorLineOnScreen() == 0)
            moveDown(1);
        scrollDown(1);
        finishMovement();
    } else if (key == 'f') {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveInclusive;
        m_subsubdata = key;
    } else if (key == 'F') {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveExclusive;
        m_subsubdata = key;
    } else if (key == 'g' && !m_gflag) {
        m_gflag = true;
    } else if (key == 'g' || key == 'G') {
        QString dotCommand = QString("%1G").arg(count());
        if (key == 'G' && m_mvcount.isEmpty())
            dotCommand = QString(QLatin1Char('G'));
        if (key == 'g')
            m_gflag = false;
        int n = (key == 'g') ? 1 : linesInDocument();
        n = m_mvcount.isEmpty() ? n : count();
        if (m_submode == NoSubMode || m_submode == ZSubMode
                || m_submode == CapitalZSubMode || m_submode == RegisterSubMode) {
            m_tc.setPosition(firstPositionInLine(n), KeepAnchor);
            handleStartOfLine();
        } else {
            m_movetype = MoveLineWise;
            m_rangemode = RangeLineMode;
            setAnchor();
            m_tc.setPosition(firstPositionInLine(n), KeepAnchor);
        }
        finishMovement(dotCommand);
    } else if (key == 'h' || key == Key_Left
            || key == Key_Backspace || key == control('h')) {
        m_movetype = MoveExclusive;
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        moveLeft(n);
        setTargetColumn();
        finishMovement("%1h", count());
    } else if (key == 'H') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, 0)));
        moveDown(qMax(count() - 1, 0));
        handleStartOfLine();
        finishMovement();
    } else if (key == 'i' || key == Key_Insert) {
        setDotCommand(QString(QLatin1Char('i'))); // setDotCommand("%1i", count());
        enterInsertMode();
        updateMiniBuffer();
        if (atEndOfLine())
            moveLeft();
    } else if (key == 'I') {
        setDotCommand(QString(QLatin1Char('I'))); // setDotCommand("%1I", count());
        if (isVisualMode()) {
            int beginLine = lineForPosition(anchor());
            int endLine = lineForPosition(position());
            m_visualInsertCount = qAbs(endLine - beginLine);
            setPosition(qMin(position(), anchor()));
        } else {
            if (m_gflag)
                moveToStartOfLine();
            else
                moveToFirstNonBlankOnLine();
            m_gflag = false;
            m_tc.clearSelection();
        }
        enterInsertMode();
    } else if (key == control('i')) {
        if (!m_jumpListRedo.isEmpty()) {
            m_jumpListUndo.append(cursorPosition());
            setCursorPosition(m_jumpListRedo.last());
            m_jumpListRedo.pop_back();
        }
    } else if (key == 'j' || key == Key_Down) {
        m_movetype = MoveLineWise;
        setAnchor();
        moveDown(count());
        finishMovement("%1j", count());
    } else if (key == 'J') {
        setDotCommand("%1J", count());
        beginEditBlock();
        if (m_submode == NoSubMode) {
            for (int i = qMax(count(), 2) - 1; --i >= 0; ) {
                moveBehindEndOfLine();
                setAnchor();
                moveRight();
                if (m_gflag) {
                    removeSelectedText();
                } else {
                    while (characterAtCursor() == ' '
                        || characterAtCursor() == '\t')
                        moveRight();
                    removeSelectedText();
                    m_tc.insertText(QString(QLatin1Char(' ')));
                }
            }
            if (!m_gflag)
                moveLeft();
        }
        endEditBlock();
        finishMovement();
    } else if (key == 'k' || key == Key_Up) {
        m_movetype = MoveLineWise;
        setAnchor();
        moveUp(count());
        finishMovement("%1k", count());
    } else if (key == 'l' || key == Key_Right || key == ' ') {
        m_movetype = MoveExclusive;
        setAnchor();
        bool pastEnd = count() >= rightDist() - 1;
        moveRight(qMax(0, qMin(count(), rightDist() - (m_submode==NoSubMode))));
        setTargetColumn();
        if (pastEnd && isVisualMode()) {
            m_visualTargetColumn = -1;
        }
        finishMovement("%1l", count());
    } else if (key == 'L') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        moveUp(qMax(count(), 1));
        handleStartOfLine();
        finishMovement();
    } else if (key == control('l')) {
        // screen redraw. should not be needed
    } else if (key == 'm') {
        m_subsubmode = MarkSubSubMode;
    } else if (key == 'M') {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        handleStartOfLine();
        finishMovement();
    } else if (key == 'n' || key == 'N') {
        bool forward = (key == 'n') ? m_lastSearchForward : !m_lastSearchForward;
        if (hasConfig(ConfigIncSearch)) {
            int pos = position();
            emit q->findNextRequested(!forward);
            if (forward && pos == EDITOR(textCursor()).selectionStart()) {
                // if cursor is already positioned at the start of a find result, this is returned
                emit q->findNextRequested(false);
            }
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.selectionStart());
        } else
            search(lastSearchString(), m_lastSearchForward);
        recordJump();
    } else if (isVisualMode() && (key == 'o' || key == 'O')) {
        int pos = position();
        setPosition(anchor());
        m_anchor = pos;
        std::swap(m_marks['<'], m_marks['>']);
        std::swap(m_positionPastEnd, m_anchorPastEnd);
        setTargetColumn();
        if (m_positionPastEnd)
            m_visualTargetColumn = -1;
        updateSelection();
    } else if (key == 'o' || key == 'O') {
        beginEditBlock();
        setDotCommand("%1o", count());
        enterInsertMode();
        m_beginEditBlock = false;
        moveToFirstNonBlankOnLine();
        if (key == 'O')
            moveToStartOfLine();
        else
            moveBehindEndOfLine();
        m_tc.insertText("\n");
        if (key == 'O')
            moveUp();
        insertAutomaticIndentation(key == 'o');
        endEditBlock();
    } else if (key == control('o')) {
        if (!m_jumpListUndo.isEmpty()) {
            m_jumpListRedo.append(cursorPosition());
            setCursorPosition(m_jumpListUndo.last());
            m_jumpListUndo.pop_back();
        }
    } else if (key == 'p' || key == 'P') {
        pasteText(key == 'p');
        setTargetColumn();
        setDotCommand("%1p", count());
        finishMovement();
    } else if (key == 'r') {
        m_submode = ReplaceSubMode;
        setDotCommand(QString(QLatin1Char('r')));
    } else if (key == 'R') {
        // FIXME: right now we repeat the insertion count() times,
        // but not the deletion
        m_lastInsertion.clear();
        enterInsertMode();
        m_submode = ReplaceSubMode;
        setDotCommand(QString(QLatin1Char('R')));
        updateMiniBuffer();
    } else if (key == control('r')) {
        redo();
    } else if (key == 's') {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        yankSelectedText();
        removeSelectedText();
        setDotCommand("%1s", count());
        m_opcount.clear();
        m_mvcount.clear();
        enterInsertMode();
    } else if (key == 'S') {
        const int line = cursorLineInDocument() + 1;
        setAnchor(firstPositionInLine(line));
        setPosition(lastPositionInLine(line + count() - 1));
        setDotCommand("%1S", count());
        enterInsertMode();
        m_beginEditBlock = false;
        m_submode = ChangeSubMode;
        m_movetype = MoveLineWise;
        finishMovement();
    } else if (m_gflag && key == 't') {
        m_gflag = false;
        handleExCommand("tabnext");
    } else if (key == 't') {
        m_movetype = MoveInclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (m_gflag && key == 'T') {
        m_gflag = false;
        handleExCommand("tabprev");
    } else if (key == 'T') {
        m_movetype = MoveExclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = key;
    } else if (key == control('t')) {
        handleExCommand("pop");
    } else if (!m_gflag && key == 'u') {
        undo();
    } else if (key == control('u')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveUp(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (key == 'v') {
        enterVisualMode(VisualCharMode);
    } else if (key == 'V') {
        enterVisualMode(VisualLineMode);
    } else if (key == control('v')) {
        enterVisualMode(VisualBlockMode);
    } else if (key == 'w') { // tested
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank - except if the cursor is on the last character of a word:
        // only the current word will be changed
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(false, true, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(false, m_submode==DeleteSubMode);
            m_movetype = MoveExclusive;
        }
        finishMovement("%1w", count());
    } else if (key == 'W') {
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(true, true, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(true, m_submode==DeleteSubMode);
            m_movetype = MoveExclusive;
        }
        finishMovement("%1W", count());
    } else if (key == control('w')) {
        m_submode = WindowSubMode;
    } else if (key == 'x' && isNoVisualMode()) { // = "dl"
        m_movetype = MoveExclusive;
        setAnchor();
        m_submode = DeleteSubMode;
        moveRight(qMin(count(), rightDist()));
        setDotCommand("%1x", count());
        finishMovement();
    } else if (key == 'X') {
        if (leftDist() > 0) {
            setAnchor();
            moveLeft(qMin(count(), leftDist()));
            yankSelectedText();
            removeSelectedText();
        }
        finishMovement();
    } else if ((m_submode == YankSubMode && key == 'y')
            || (key == 'Y' && isNoVisualMode()))  {
        setAnchor();
        if (count() > 1)
            moveDown(count()-1);
        m_rangemode = RangeLineMode;
        m_movetype = MoveLineWise;
        m_submode = YankSubMode;
        finishMovement();
    } else if (key == 'y' && isNoVisualMode()) {
        setAnchor();
        m_submode = YankSubMode;
    } else if (key == 'y' && isVisualCharMode()) {
        Range range(position(), anchor(), RangeCharMode);
        range.endPos++; // MoveInclusive
        yankText(range, m_register);
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if ((key == 'y' && isVisualLineMode())
            || (key == 'Y' && isVisualLineMode())
            || (key == 'Y' && isVisualCharMode())) {
        m_rangemode = RangeLineMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        moveToStartOfLine();
        leaveVisualMode();
        finishMovement();
    } else if ((key == 'y' || key == 'Y') && isVisualBlockMode()) {
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if (key == 'z') {
        m_submode = ZSubMode;
    } else if (key == 'Z') {
        m_submode = CapitalZSubMode;
    } else if (!m_gflag && key == '~' && !isVisualMode()) {
        if (!atEndOfLine()) {
            beginEditBlock();
            setAnchor();
            moveRight(qMin(count(), rightDist()));
            if (key == '~') {
                invertCaseSelectedText();
                setDotCommand("%1~", count());
            } else if (key == 'u') {
                downCaseSelectedText();
                setDotCommand("%1gu", count());
            } else if (key == 'U') {
                upCaseSelectedText();
                setDotCommand("%1gU", count());
            }
            endEditBlock();
        }
        finishMovement();
    } else if ((m_gflag && key == '~' && !isVisualMode())
        || (m_gflag && key == 'u' && !isVisualMode())
        || (m_gflag && key == 'U' && !isVisualMode())) {
        m_gflag = false;
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = TransformSubMode;
        if (key == '~')
            m_subsubmode = InvertCaseSubSubMode;
        if (key == 'u')
            m_subsubmode = DownCaseSubSubMode;
        else if (key == 'U')
            m_subsubmode = UpCaseSubSubMode;
    } else if ((key == '~' && isVisualMode())
        || (m_gflag && key == 'u' && isVisualMode())
        || (m_gflag && key == 'U' && isVisualMode())) {
        m_gflag = false;
        if (isVisualLineMode())
            m_rangemode = RangeLineMode;
        else if (isVisualBlockMode())
            m_rangemode = RangeBlockMode;
        leaveVisualMode();
        m_submode = TransformSubMode;
        if (key == '~')
            m_subsubmode = InvertCaseSubSubMode;
        else if (key == 'u')
            m_subsubmode = DownCaseSubSubMode;
        else if (key == 'U')
            m_subsubmode = UpCaseSubSubMode;
        finishMovement();
    } else if (key == Key_PageDown || key == control('f')) {
        moveDown(count() * (linesOnScreen() - 2) - cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument());
        handleStartOfLine();
        finishMovement();
    } else if (key == Key_PageUp || key == control('b')) {
        moveUp(count() * (linesOnScreen() - 2) + cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument() + linesOnScreen() - 2);
        handleStartOfLine();
        finishMovement();
    } else if (key == Key_Delete) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        removeSelectedText();
    } else if (key == Key_BracketLeft || key == Key_BracketRight) {

    } else if (key == control(Key_BracketRight)) {
        handleExCommand("tag");
    } else if (key == Key_Escape || key == control(Key_BracketLeft)) {
        if (isVisualMode()) {
            leaveVisualMode();
        } else if (m_submode != NoSubMode) {
            m_submode = NoSubMode;
            m_subsubmode = NoSubSubMode;
            finishMovement();
        } else {
            resetCommandMode();
        }
    } else {
        //qDebug() << "IGNORED IN COMMAND MODE: " << key << text
        //    << " VISUAL: " << m_visualMode;

        // if a key which produces text was pressed, don't mark it as unhandled
        // - otherwise the text would be inserted while being in command mode
        if (text.isEmpty()) {
            handled = EventUnhandled;
        }
    }

    m_positionPastEnd = (m_visualTargetColumn == -1) && isVisualMode();

    return handled;
}

EventResult FakeVimHandler::Private::handleInsertMode(int key, int,
    const QString &text)
{
    if (key == Key_Escape || key == 27 || key == control('c') ||
         key == control(Key_BracketLeft)) {
        if (isVisualBlockMode() && !m_lastInsertion.contains('\n')) {
            leaveVisualMode();
            joinPreviousEditBlock();
            moveLeft(m_lastInsertion.size());
            setAnchor();
            int pos = position();
            setTargetColumn();
            for (int i = 0; i < m_visualInsertCount; ++i) {
                moveDown();
                m_tc.insertText(m_lastInsertion);
            }
            moveLeft(1);
            Range range(pos, position(), RangeBlockMode);
            yankText(range);
            setPosition(pos);
            setDotCommand("p");
            endEditBlock();
        } else {
            // normal insertion. start with '1', as one instance was
            // already physically inserted while typing
            QString data = m_lastInsertion;
            for (int i = 1; i < count(); ++i) {
                m_tc.insertText(m_lastInsertion);
                data += m_lastInsertion;
            }
            moveLeft(qMin(1, leftDist()));
            setTargetColumn();
            leaveVisualMode();
            recordNewUndo();
        }
        m_dotCommand += m_lastInsertion;
        m_dotCommand += QChar(27);
        enterCommandMode();
        m_submode = NoSubMode;
    } else if (key == Key_Insert) {
        if (m_submode == ReplaceSubMode) {
            EDITOR(setCursorWidth(m_cursorWidth));
            EDITOR(setOverwriteMode(false));
            m_submode = NoSubMode;
        } else {
            EDITOR(setCursorWidth(m_cursorWidth));
            EDITOR(setOverwriteMode(true));
            m_submode = ReplaceSubMode;
        }
    } else if (key == Key_Left) {
        moveLeft(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (key == Key_Down) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveDown(count());
        m_lastInsertion.clear();
    } else if (key == Key_Up) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveUp(count());
        m_lastInsertion.clear();
    } else if (key == Key_Right) {
        moveRight(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (key == Key_Return) {
        m_submode = NoSubMode;
        m_tc.insertBlock();
        m_lastInsertion += "\n";
        insertAutomaticIndentation(true);
        setTargetColumn();
    } else if (key == Key_Backspace || key == control('h')) {
        joinPreviousEditBlock();
        m_justAutoIndented = 0;
        if (!m_lastInsertion.isEmpty() || hasConfig(ConfigBackspace, "start")) {
            const int line = cursorLineInDocument() + 1;
            const Column col = cursorColumnInDocument();
            QString data = lineContents(line);
            const Column ind = indentation(data);
            if (col.logical <= ind.logical && col.logical
                    && startsWithWhitespace(data, col.physical)) {
                const int ts = config(ConfigTabStop).toInt();
                const int newcol = col.logical - 1 - (col.logical - 1) % ts;
                data.remove(0, col.physical);
                setLineContents(line, tabExpand(newcol).append(data));
                m_lastInsertion.clear(); // FIXME
            } else {
                m_tc.deletePreviousChar();
                m_lastInsertion.chop(1);
            }
            setTargetColumn();
        }
        endEditBlock();
    } else if (key == Key_Delete) {
        m_tc.deleteChar();
        m_lastInsertion.clear();
    } else if (key == Key_PageDown || key == control('f')) {
        removeAutomaticIndentation();
        moveDown(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_PageUp || key == control('b')) {
        removeAutomaticIndentation();
        moveUp(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (key == Key_Tab && hasConfig(ConfigExpandTab)) {
        m_justAutoIndented = 0;
        const int ts = config(ConfigTabStop).toInt();
        const int col = physicalCursorColumnInDocument();
        QString str = QString(ts - col % ts, ' ');
        m_lastInsertion.append(str);
        m_tc.insertText(str);
        setTargetColumn();
    } else if (key == control('d')) {
        // remove one level of indentation from the current line
        int shift = config(ConfigShiftWidth).toInt();
        int tab = config(ConfigTabStop).toInt();
        int line = cursorLineInDocument() + 1;
        int pos = firstPositionInLine(line);
        QString text = lineContents(line);
        int amount = 0;
        int i = 0;
        for (; i < text.size() && amount < shift; ++i) {
            if (text.at(i) == ' ')
                ++amount;
            else if (text.at(i) == '\t')
                amount += tab; // FIXME: take position into consideration
            else
                break;
        }
        removeText(Range(pos, pos+i));
    } else if (key >= control('a') && key <= control('z')) {
        // ignore these
    } else if (!text.isEmpty()) {
        if (m_beginEditBlock) {
            beginEditBlock();
            m_beginEditBlock = false;
        } else {
            joinPreviousEditBlock();
        }
        m_justAutoIndented = 0;
        m_lastInsertion.append(text);
        if (m_submode == ReplaceSubMode) {
            if (atEndOfLine())
                m_submode = NoSubMode;
            else
                m_tc.deleteChar();
        }
        m_tc.insertText(text);
        if (hasConfig(ConfigSmartIndent) && isElectricCharacter(text.at(0))) {
            const QString leftText = m_tc.block().text()
                   .left(m_tc.position() - 1 - m_tc.block().position());
            if (leftText.simplified().isEmpty()) {
                Range range(position(), position());
                range.rangemode = m_rangemode;
                indentText(range, text.at(0));
            }
        }

        if (!m_inReplay)
            emit q->completionRequested();
        setTargetColumn();
        endEditBlock();
    } else {
        return EventUnhandled;
    }
    updateMiniBuffer();
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleMiniBufferModes(int key, int unmodified,
    const QString &text)
{
    Q_UNUSED(text)

    if (key == Key_Escape || key == control('c') || key == control(Key_BracketLeft)) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
    } else if (key == Key_Backspace) {
        if (m_commandBuffer.isEmpty()) {
            enterCommandMode();
        } else {
            m_commandBuffer.chop(1);
        }
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
            if (m_textedit || m_plaintextedit) {
                leaveVisualMode();
            }
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
    } else if ((key == Key_Up || key == Key_PageUp) && isSearchMode()) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entries in the history into account.
        if (m_searchHistoryIndex > 0) {
            --m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if ((key == Key_Up || key == Key_PageUp) && m_mode == ExMode) {
        if (m_commandHistoryIndex > 0) {
            --m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if ((key == Key_Down || key == Key_PageDown) && isSearchMode()) {
        if (m_searchHistoryIndex < m_searchHistory.size() - 1) {
            ++m_searchHistoryIndex;
            showBlackMessage(m_searchHistory.at(m_searchHistoryIndex));
        }
    } else if ((key == Key_Down || key == Key_PageDown) && m_mode == ExMode) {
        if (m_commandHistoryIndex < m_commandHistory.size() - 1) {
            ++m_commandHistoryIndex;
            showBlackMessage(m_commandHistory.at(m_commandHistoryIndex));
        }
    } else if (key == Key_Tab) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else if (QChar(key).isPrint()) {
        m_commandBuffer += QChar(key);
        updateMiniBuffer();
    } else {
        qDebug() << "IGNORED IN MINIBUFFER MODE: " << key << text;
        return EventUnhandled;
    }
    return EventHandled;
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
            showRedMessage(msgE20MarkNotSet(cmd.at(0)));
            cmd = cmd.mid(1);
            return -1;
        }
        cmd = cmd.mid(1);
        return lineForPosition(mark);
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
            showRedMessage(msgE20MarkNotSet(cmd.at(0)));
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
    if (beginLine == -1)
        beginLine = cursorLineInDocument();
    if (endLine == -1)
        endLine = cursorLineInDocument();
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    setAnchor(firstPositionInLine(beginLine));
    if (endLine == linesInDocument())
       setPosition(lastPositionInLine(endLine));
    else
       setPosition(firstPositionInLine(endLine + 1));
}

// use handleExCommand for invoking commands that might move the cursor
void FakeVimHandler::Private::handleCommand(const QString &cmd)
{
    m_tc = EDITOR(textCursor());
    handleExCommand(cmd);
    EDITOR(setTextCursor(m_tc));
}

// result: (needle, replacement, opions)
static bool isSubstitution(const QString &cmd0, QStringList *result)
{
    QString cmd;
    if (cmd0.startsWith(QLatin1String("substitute")))
        cmd = cmd0.mid(10);
    else if (cmd0.startsWith('s') && cmd0.size() > 1
            && !isalpha(cmd0.at(1).unicode()))
        cmd = cmd0.mid(1);
    else
        return false;
    // we have /{pattern}/{string}/[flags]  now
    if (cmd.isEmpty())
        return false;
    const QChar separator = cmd.at(0);
    int pos1 = -1;
    int pos2 = -1;
    int i;
    for (i = 1; i < cmd.size(); ++i) {
        if (cmd.at(i) == separator && cmd.at(i - 1) != '\\') {
            pos1 = i;
            break;
        }
    }
    if (pos1 == -1)
        return false;
    for (++i; i < cmd.size(); ++i) {
        if (cmd.at(i) == separator && cmd.at(i - 1) != '\\') {
            pos2 = i;
            break;
        }
    }
    if (pos2 == -1)
        pos2 = cmd.size();

    result->append(cmd.mid(1, pos1 - 1));
    result->append(cmd.mid(pos1 + 1, pos2 - pos1 - 1));
    result->append(cmd.mid(pos2 + 1));
    return true;
}

void FakeVimHandler::Private::handleExCommand(const QString &cmd0)
{
    QString cmd = cmd0;
    // FIXME: that seems to be different for %w and %s
    if (cmd.startsWith(QLatin1Char('%')))
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

    static QRegExp reQuit("^qa?!?$");
    static QRegExp reDelete("^d( (.*))?$");
    static QRegExp reHistory("^his(tory)?( (.*))?$");
    static QRegExp reNormal("^norm(al)?( (.*))?$");
    static QRegExp reSet("^set?( (.*))?$");
    static QRegExp reWrite("^[wx]q?a?!?( (.*))?$");
    QStringList arguments;

    enterCommandMode();
    showBlackMessage(QString());

    if (cmd.isEmpty()) {
        setPosition(firstPositionInLine(beginLine));
        showBlackMessage(QString());
    } else if (reDelete.indexIn(cmd) != -1) { // :d
        selectRange(beginLine, endLine);
        QString reg = reDelete.cap(2);
        QString text = selectedText();
        removeSelectedText();
        if (!reg.isEmpty()) {
            Register &r = m_registers[reg.at(0).unicode()];
            r.contents = text;
            r.rangemode = RangeLineMode;
        }
    } else if (reWrite.indexIn(cmd) != -1) { // :w and :x
        bool noArgs = (beginLine == -1);
        if (beginLine == -1)
            beginLine = 0;
        if (endLine == -1)
            endLine = linesInDocument();
        //qDebug() << "LINES: " << beginLine << endLine;
        int indexOfSpace = cmd.indexOf(QChar(' '));
        QString prefix;
        if (indexOfSpace < 0)
            prefix = cmd;
        else
            prefix = cmd.left(indexOfSpace);
        bool forced = prefix.contains(QChar('!'));
        bool quit = prefix.contains(QChar('q')) || prefix.contains(QChar('x'));
        bool quitAll = quit && prefix.contains(QChar('a'));
        QString fileName = reWrite.cap(2);
        if (fileName.isEmpty())
            fileName = m_currentFileName;
        QFile file1(fileName);
        bool exists = file1.exists();
        if (exists && !forced && !noArgs) {
            showRedMessage(FakeVimHandler::tr
                ("File '%1' exists (add ! to override)").arg(fileName));
        } else if (file1.open(QIODevice::ReadWrite)) {
            file1.close();
            QTextCursor tc = m_tc;
            Range range(firstPositionInLine(beginLine),
                firstPositionInLine(endLine), RangeLineMode);
            QString contents = text(range);
            m_tc = tc;
            //qDebug() << "LINES: " << beginLine << endLine;
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
                    showRedMessage(FakeVimHandler::tr
                       ("Cannot open file '%1' for writing").arg(fileName));
                }
            }
            // check result by reading back
            QFile file3(fileName);
            file3.open(QIODevice::ReadOnly);
            QByteArray ba = file3.readAll();
            showBlackMessage(FakeVimHandler::tr("\"%1\" %2 %3L, %4C written")
                .arg(fileName).arg(exists ? " " : " [New] ")
                .arg(ba.count('\n')).arg(ba.size()));
            if (quitAll)
                passUnknownExCommand(forced ? "qa!" : "qa");
            else if (quit)
                passUnknownExCommand(forced ? "q!" : "q");
        } else {
            showRedMessage(FakeVimHandler::tr
                ("Cannot open file '%1' for reading").arg(fileName));
        }
    } else if (cmd.startsWith(QLatin1String("r "))) { // :r
        beginEditBlock();
        moveToStartOfLine();
        setTargetColumn();
        moveDown();
        m_currentFileName = cmd.mid(2);
        QFile file(m_currentFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream ts(&file);
        QString data = ts.readAll();
        m_tc.insertText(data);
        showBlackMessage(FakeVimHandler::tr("\"%1\" %2L, %3C")
            .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));
        endEditBlock();
    } else if (cmd.startsWith(QLatin1Char('!'))) {
        selectRange(beginLine, endLine);
        QString command = cmd.mid(1).trimmed();
        QString text = selectedText();
        removeSelectedText();
        QProcess proc;
        proc.start(cmd.mid(1));
        proc.waitForStarted();
        proc.write(text.toUtf8());
        proc.closeWriteChannel();
        proc.waitForFinished();
        QString result = QString::fromUtf8(proc.readAllStandardOutput());
        m_tc.insertText(result);
        leaveVisualMode();
        setPosition(firstPositionInLine(beginLine));
        //qDebug() << "FILTER: " << command;
        showBlackMessage(FakeVimHandler::tr("%n lines filtered", 0,
            text.count('\n')));
    } else if (cmd.startsWith(QLatin1Char('>'))) {
        m_anchor = firstPositionInLine(beginLine);
        setPosition(firstPositionInLine(endLine));
        shiftRegionRight(1);
        leaveVisualMode();
        showBlackMessage(FakeVimHandler::tr("%n lines >ed %1 time", 0,
            (endLine - beginLine + 1)).arg(1));
    } else if (cmd == "red" || cmd == "redo") { // :redo
        redo();
        updateMiniBuffer();
    } else if (reNormal.indexIn(cmd) != -1) { // :normal
        //qDebug() << "REPLAY: " << reNormal.cap(3);
        replay(reNormal.cap(3), 1);
    } else if (isSubstitution(cmd, &arguments)) { // :substitute
        QString needle = arguments.at(0);
        const QString replacement = arguments.at(1);
        QString flags = arguments.at(2);
        needle.replace('$', '\n');
        needle.replace("\\\n", "\\$");
        QRegExp pattern(needle);
        if (flags.contains('i'))
            pattern.setCaseSensitivity(Qt::CaseInsensitive);
        const bool global = flags.contains('g');
        beginEditBlock();
        for (int line = endLine; line >= beginLine; --line) {
            QString origText = lineContents(line);
            QString text = origText;
            int pos = 0;
            while (true) {
                pos = pattern.indexIn(text, pos, QRegExp::CaretAtZero);
                if (pos == -1)
                    break;
                if (pattern.cap(0).isEmpty())
                    break;
                QStringList caps = pattern.capturedTexts();
                QString matched = text.mid(pos, caps.at(0).size());
                QString repl = replacement;
                for (int i = 1; i < caps.size(); ++i)
                    repl.replace("\\" + QString::number(i), caps.at(i));
                for (int i = 0; i < repl.size(); ++i) {
                    if (repl.at(i) == '&' && (i == 0 || repl.at(i - 1) != '\\')) {
                        repl.replace(i, 1, caps.at(0));
                        i += caps.at(0).size();
                    }
                }
                text = text.left(pos) + repl + text.mid(pos + matched.size());
                pos += matched.size();
                if (!global)
                    break;
            }
            if (text != origText)
                setLineContents(line, text);
        }
        endEditBlock();
    } else if (reSet.indexIn(cmd) != -1) { // :set
        showBlackMessage(QString());
        QString arg = reSet.cap(2);
        SavedAction *act = theFakeVimSettings()->item(arg);
        if (arg.isEmpty()) {
            theFakeVimSetting(SettingsDialog)->trigger(QVariant());
        } else if (act && act->value().type() == QVariant::Bool) {
            // boolean config to be switched on
            bool oldValue = act->value().toBool();
            if (oldValue == false)
                act->setValue(true);
            else if (oldValue == true)
                {} // nothing to do
        } else if (act) {
            // non-boolean to show
            showBlackMessage(arg + '=' + act->value().toString());
        } else if (arg.startsWith(QLatin1String("no"))
                && (act = theFakeVimSettings()->item(arg.mid(2)))) {
            // boolean config to be switched off
            bool oldValue = act->value().toBool();
            if (oldValue == true)
                act->setValue(false);
            else if (oldValue == false)
                {} // nothing to do
        } else if (arg.contains('=')) {
            // non-boolean config to set
            int p = arg.indexOf('=');
            act = theFakeVimSettings()->item(arg.left(p));
            if (act)
                act->setValue(arg.mid(p + 1));
        } else {
            passUnknownSetCommand(arg);
        }
        updateMiniBuffer();
        updateEditor();
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
        updateMiniBuffer();
    } else {
        passUnknownExCommand(cmd);
    }
}

void FakeVimHandler::Private::passUnknownExCommand(const QString &cmd)
{
    EDITOR(setTextCursor(m_tc));
    emit q->handleExCommandRequested(cmd);
    if (m_plaintextedit || m_textedit)
        m_tc = EDITOR(textCursor());
}

void FakeVimHandler::Private::passUnknownSetCommand(const QString &arg)
{
    bool handled = false;
    emit q->handleSetCommandRequested(&handled, arg);
    if (!handled) {
        showRedMessage(FakeVimHandler::tr("E512: Unknown option: ") + arg);
    }
}

static void vimPatternToQtPattern(QString *needle, QTextDocument::FindFlags *flags)
{
    // FIXME: Rough mapping of a common case
    if (needle->startsWith(QLatin1String("\\<")) && needle->endsWith(QLatin1String("\\>")))
        (*flags) |= QTextDocument::FindWholeWords;
    needle->remove(QLatin1String("\\<")); // start of word
    needle->remove(QLatin1String("\\>")); // end of word
    //qDebug() << "NEEDLE " << needle0 << needle;
}

void FakeVimHandler::Private::search(const QString &needle0, bool forward)
{
    showBlackMessage((forward ? '/' : '?') + needle0);
    CursorPosition origPosition = cursorPosition();
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (!forward)
        flags |= QTextDocument::FindBackward;

    QString needle = needle0;
    vimPatternToQtPattern(&needle, &flags);

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
        highlightMatches(needle);
    } else {
        m_tc.setPosition(forward ? 0 : lastPositionInDocument());
        EDITOR(setTextCursor(m_tc));
        if (EDITOR(find(needle, flags))) {
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.anchor());
            if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
                scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
            if (forward)
                showRedMessage(FakeVimHandler::tr("search hit BOTTOM, continuing at TOP"));
            else
                showRedMessage(FakeVimHandler::tr("search hit TOP, continuing at BOTTOM"));
            highlightMatches(needle);
        } else {
            highlightMatches(QString());
            setCursorPosition(origPosition);
            showRedMessage(FakeVimHandler::tr("Pattern not found: ") + needle);
        }
    }
}

void FakeVimHandler::Private::highlightMatches(const QString &needle0)
{
    if (!hasConfig(ConfigHlSearch))
        return;
    if (needle0 == m_oldNeedle)
        return;
    m_oldNeedle = needle0;
    m_searchSelections.clear();

    if (!needle0.isEmpty()) {
        QTextCursor tc = m_tc;
        tc.movePosition(StartOfDocument, MoveAnchor);

        QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
        QString needle = needle0;
        vimPatternToQtPattern(&needle, &flags);


        EDITOR(setTextCursor(tc));
        while (EDITOR(find(needle, flags))) {
            tc = EDITOR(textCursor());
            QTextEdit::ExtraSelection sel;
            sel.cursor = tc;
            sel.format = tc.blockCharFormat();
            sel.format.setBackground(QColor(177, 177, 0));
            m_searchSelections.append(sel);
            tc.movePosition(Right, MoveAnchor);
            EDITOR(setTextCursor(tc));
        }
    }
    updateSelection();
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine()
{
    QTextDocument *doc = m_tc.document();
    const QTextBlock &block = m_tc.block();
    int firstPos = block.position();
    for (int i = firstPos, n = firstPos + block.length(); i < n; ++i) {
        if (!doc->characterAt(i).isSpace() || i == n - 1) {
            setPosition(i);
            return;
        }
    }
    setPosition(block.position());
}

void FakeVimHandler::Private::indentSelectedText(QChar typedChar)
{
    setTargetColumn();
    int beginLine = qMin(lineForPosition(position()), lineForPosition(anchor()));
    int endLine = qMax(lineForPosition(position()), lineForPosition(anchor()));

    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    indentText(range, typedChar);

    setPosition(firstPositionInLine(beginLine));
    moveToTargetColumn();
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1==", endLine - beginLine + 1);
}

int FakeVimHandler::Private::indentText(const Range &range, QChar typedChar)
{
    int beginLine = lineForPosition(range.beginPos);
    int endLine = lineForPosition(range.endPos);
    if (beginLine > endLine)
        qSwap(beginLine, endLine);

    int amount = 0;
    // lineForPosition has returned 1-based line numbers
    emit q->indentRegion(&amount, beginLine-1, endLine-1, typedChar);

    return amount;
}

bool FakeVimHandler::Private::isElectricCharacter(QChar c) const
{
    bool result = false;
    emit q->checkForElectricCharacter(&result, c);
    return result;
}

void FakeVimHandler::Private::shiftRegionRight(int repeat)
{
    setTargetColumn();
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    int len = config(ConfigShiftWidth).toInt() * repeat;
    QString indent(len, ' ');
    int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);
    for (int line = beginLine; line <= endLine; ++line) {
        setPosition(firstPositionInLine(line));
        m_tc.insertText(indent);
    }
    endEditBlock();

    setPosition(firstPos);
    moveToTargetColumn();
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1>>", endLine - beginLine + 1);
}

void FakeVimHandler::Private::shiftRegionLeft(int repeat)
{
    setTargetColumn();
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    if (beginLine > endLine)
        qSwap(beginLine, endLine);
    const int shift = config(ConfigShiftWidth).toInt() * repeat;
    const int tab = config(ConfigTabStop).toInt();
    const int firstPos = firstPositionInLine(beginLine);

    beginEditBlock(firstPos);
    for (int line = endLine; line >= beginLine; --line) {
        int pos = firstPositionInLine(line);
        const QString text = lineContents(line);
        int amount = 0;
        int i = 0;
        for (; i < text.size() && amount < shift; ++i) {
            if (text.at(i) == ' ')
                amount++;
            else if (text.at(i) == '\t')
                amount += tab; // FIXME: take position into consideration
            else
                break;
        }
        removeText(Range(pos, pos + i));
        setPosition(pos);
    }
    endEditBlock();

    setPosition(firstPos);
    moveToTargetColumn();
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1<<", endLine - beginLine + 1);
}

void FakeVimHandler::Private::moveToTargetColumn()
{
    const QTextBlock &block = m_tc.block();
    int col = m_tc.position() - block.position();
    if (col == m_targetColumn && m_targetColumn < block.length() - 2)
        return;
    //qDebug() << "CORRECTING COLUMN FROM: " << col << "TO" << m_targetColumn;
    if (m_targetColumn == -1 || m_targetColumn >= block.length() - 2)
        m_tc.setPosition(block.position() + qMax(0, block.length() - 2), MoveAnchor);
    else
        m_tc.setPosition(block.position() + m_targetColumn, MoveAnchor);
}

/* if simple is given:
 *  class 0: spaces
 *  class 1: non-spaces
 * else
 *  class 0: spaces
 *  class 1: non-space-or-letter-or-number
 *  class 2: letter-or-number
 */
static int charClass(QChar c, bool simple)
{
    if (simple)
        return c.isSpace() ? 0 : 1;
    if (c.isLetterOrNumber() || c.unicode() == '_')
        return 2;
    return c.isSpace() ? 0 : 1;
}

void FakeVimHandler::Private::moveToWordBoundary(bool simple, bool forward, bool changeWord)
{
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    int n = forward ? lastPositionInDocument() : 0;
    int lastClass = -1;
    if (changeWord) {
        lastClass = charClass(characterAtCursor(), simple);
        --repeat;
        if (changeWord && m_tc.block().length() == 1) // empty line
            --repeat;
    }
    while (repeat >= 0) {
        QChar c = doc->characterAt(m_tc.position() + (forward ? 1 : -1));
        //qDebug() << "EXAMINING: " << c << " AT " << position();
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && (lastClass != 0 || changeWord))
            --repeat;
        if (repeat == -1)
            break;
        lastClass = thisClass;
        if (m_tc.position() == n)
            break;
        forward ? moveRight() : moveLeft();
        if (changeWord && m_tc.block().length() == 1) // empty line
            --repeat;
        if (repeat == -1)
            break;
    }
    setTargetColumn();
}

bool FakeVimHandler::Private::handleFfTt(int key)
{
    int oldPos = position();
    // m_subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = m_subsubdata == 'f' || m_subsubdata == 't';
    int repeat = count();
    QTextDocument *doc = m_tc.document();
    QTextBlock block = m_tc.block();
    int n = block.position();
    if (forward)
        n += block.length();
    int pos = m_tc.position();
    while (pos != n) {
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

            if (forward)
                m_tc.movePosition(Right, KeepAnchor, pos - m_tc.position());
            else
                m_tc.movePosition(Left, KeepAnchor, m_tc.position() - pos);
            break;
        }
    }
    if (repeat == 0) {
        setTargetColumn();
        return true;
    } else {
        setPosition(oldPos);
        return false;
    }
}

void FakeVimHandler::Private::moveToNextWord(bool simple, bool deleteWord)
{
    int repeat = count();
    int n = lastPositionInDocument();
    int lastClass = charClass(characterAtCursor(), simple);
    while (true) {
        QChar c = characterAtCursor();
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && thisClass != 0)
            --repeat;
        if (repeat == 0)
            break;
        lastClass = thisClass;
        moveRight();
        if (deleteWord) {
            if (m_tc.atBlockEnd())
                --repeat;
        } else {
            if (m_tc.block().length() == 1) // empty line
                --repeat;
        }
        if (repeat == 0)
            break;
        if (m_tc.position() == n)
            break;
    }
    setTargetColumn();
}

void FakeVimHandler::Private::moveToMatchingParanthesis()
{
    bool moved = false;
    bool forward = false;

    emit q->moveToMatchingParenthesis(&moved, &forward, &m_tc);

    if (moved && forward) {
        m_tc.movePosition(Left, KeepAnchor, 1);
    }
    setTargetColumn();
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

int FakeVimHandler::Private::physicalCursorColumnInDocument() const
{
    return m_tc.position() - m_tc.block().position();
}

int FakeVimHandler::Private::logicalCursorColumnInDocument() const
{
    const int ncols = physicalCursorColumnInDocument();
    const QString line = m_tc.block().text();
    const int ts = config(ConfigTabStop).toInt();
    int physical = 0;
    int logical = 0;
    while (physical < ncols) {
        QChar c = line.at(physical);
        if (c == QLatin1Char(' '))
            ++logical;
        else if (c == QLatin1Char('\t'))
            logical += ts - logical % ts;
        else
            break;
        ++physical;
    }
    return logical;
}

Column FakeVimHandler::Private::cursorColumnInDocument() const
{
    return Column(physicalCursorColumnInDocument(), logicalCursorColumnInDocument());
}

int FakeVimHandler::Private::linesInDocument() const
{
    if (m_tc.isNull())
        return 0;
    const QTextDocument *doc = m_tc.document();
    const int count = doc->blockCount();
    // Qt inserts an empty line if the last character is a '\n',
    // but that's not how vi does it.
    return doc->lastBlock().length() <= 1 ? count - 1 : count;
}

void FakeVimHandler::Private::scrollToLineInDocument(int line)
{
    // FIXME: works only for QPlainTextEdit
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    //qDebug() << "SCROLL: " << scrollBar->value() << line;
    scrollBar->setValue(line);
    //QTC_ASSERT(firstVisibleLineInDocument() == line, /**/);
}

int FakeVimHandler::Private::firstVisibleLineInDocument() const
{
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    if (0 && scrollBar->value() != cursorLineInDocument() - cursorLineOnScreen()) {
        qDebug() << "SCROLLBAR: " << scrollBar->value()
            << "CURSORLINE IN DOC" << cursorLineInDocument()
            << "CURSORLINE ON SCREEN" << cursorLineOnScreen();
    }
    //return scrollBar->value();
    return cursorLineInDocument() - cursorLineOnScreen();
}

void FakeVimHandler::Private::scrollUp(int count)
{
    scrollToLineInDocument(cursorLineInDocument() - cursorLineOnScreen() - count);
}

int FakeVimHandler::Private::lastPositionInDocument() const
{
    QTextBlock block = m_tc.document()->lastBlock();
    return block.position() + block.length() - 1;
}

QString FakeVimHandler::Private::lastSearchString() const
{
     return m_searchHistory.empty() ? QString() : m_searchHistory.back();
}

QString FakeVimHandler::Private::text(const Range &range) const
{
    if (range.rangemode == RangeCharMode) {
        QTextCursor tc = m_tc;
        tc.setPosition(range.beginPos, MoveAnchor);
        tc.setPosition(range.endPos, KeepAnchor);
        return tc.selection().toPlainText();
    }
    if (range.rangemode == RangeLineMode) {
        QTextCursor tc = m_tc;
        int firstPos = firstPositionInLine(lineForPosition(range.beginPos));
        int lastLine = lineForPosition(range.endPos);
        int lastPos = lastLine == m_tc.document()->lastBlock().blockNumber() + 1
            ? lastPositionInDocument() : firstPositionInLine(lastLine + 1);
        tc.setPosition(firstPos, MoveAnchor);
        tc.setPosition(lastPos, KeepAnchor);
        return tc.selection().toPlainText();
    }
    // FIXME: Performance?
    int beginLine = lineForPosition(range.beginPos);
    int endLine = lineForPosition(range.endPos);
    int beginColumn = 0;
    int endColumn = INT_MAX;
    if (range.rangemode == RangeBlockMode) {
        int column1 = range.beginPos - firstPositionInLine(beginLine);
        int column2 = range.endPos - firstPositionInLine(endLine);
        beginColumn = qMin(column1, column2);
        endColumn = qMax(column1, column2);
    }
    int len = endColumn - beginColumn + 1;
    QString contents;
    QTextBlock block = m_tc.document()->findBlockByNumber(beginLine - 1);
    for (int i = beginLine; i <= endLine && block.isValid(); ++i) {
        QString line = block.text();
        if (range.rangemode == RangeBlockMode) {
            line = line.mid(beginColumn, len);
            if (line.size() < len)
                line += QString(len - line.size(), QChar(' '));
        }
        contents += line;
        if (!contents.endsWith('\n'))
            contents += '\n';
        block = block.next();
    }
    //qDebug() << "SELECTED: " << contents;
    return contents;
}

void FakeVimHandler::Private::yankSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    yankText(range, m_register);
}

void FakeVimHandler::Private::yankText(const Range &range, int toregister)
{
    Register &reg = m_registers[toregister];
    reg.contents = text(range);
    reg.rangemode = range.rangemode;
    //qDebug() << "YANKED: " << reg.contents;
}

void FakeVimHandler::Private::transformText(const Range &range,
    Transformation transformFunc)
{
    QTextCursor tc = m_tc;
    switch (range.rangemode) {
        case RangeCharMode: {
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            (this->*transformFunc)(range.beginPos, &tc);
            return;
        }
        case RangeLineMode:
        case RangeLineModeExclusive: {
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.movePosition(StartOfLine, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            tc.movePosition(EndOfLine, KeepAnchor);
            if (range.rangemode != RangeLineModeExclusive) {
                // make sure that complete lines are removed
                // - also at the beginning and at the end of the document
                if (tc.atEnd()) {
                    tc.setPosition(range.beginPos, MoveAnchor);
                    tc.movePosition(StartOfLine, MoveAnchor);
                    if (!tc.atStart()) {
                        // also remove first line if it is the only one
                        tc.movePosition(Left, MoveAnchor, 1);
                        tc.movePosition(EndOfLine, MoveAnchor, 1);
                    }
                    tc.setPosition(range.endPos, KeepAnchor);
                    tc.movePosition(EndOfLine, KeepAnchor);
                } else {
                    tc.movePosition(Right, KeepAnchor, 1);
                }
            }
            (this->*transformFunc)(range.beginPos, &tc);
            return;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            int beginLine = lineForPosition(range.beginPos);
            int endLine = lineForPosition(range.endPos);
            int column1 = range.beginPos - firstPositionInLine(beginLine);
            int column2 = range.endPos - firstPositionInLine(endLine);
            int beginColumn = qMin(column1, column2);
            int endColumn = qMax(column1, column2);
            if (range.rangemode == RangeBlockAndTailMode)
                endColumn = INT_MAX - 1;
            QTextBlock block = m_tc.document()->findBlockByNumber(endLine - 1);
            beginEditBlock(range.beginPos);
            for (int i = beginLine; i <= endLine && block.isValid(); ++i) {
                int bCol = qMin(beginColumn, block.length() - 1);
                int eCol = qMin(endColumn + 1, block.length() - 1);
                tc.setPosition(block.position() + bCol, MoveAnchor);
                tc.setPosition(block.position() + eCol, KeepAnchor);
                (this->*transformFunc)(block.position() + bCol, &tc);
                block = block.previous();
            }
            endEditBlock();
        }
    }
}

void FakeVimHandler::Private::removeSelectedText(bool exclusive)
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    if (exclusive && m_rangemode == RangeLineMode)
        range.rangemode = RangeLineModeExclusive;
    removeText(range);
}

void FakeVimHandler::Private::removeText(const Range &range)
{
    transformText(range, &FakeVimHandler::Private::removeTransform);
}

void FakeVimHandler::Private::removeTransform(int updateMarksAfter, QTextCursor *tc)
{
    fixMarks(updateMarksAfter, tc->selectionStart() - tc->selectionEnd());
    tc->removeSelectedText();
}

void FakeVimHandler::Private::downCaseSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    transformText(range, &FakeVimHandler::Private::downCaseTransform);
}

void FakeVimHandler::Private::downCaseTransform(int updateMarksAfter, QTextCursor *tc)
{
    Q_UNUSED(updateMarksAfter);
    QString str = tc->selectedText();
    tc->removeSelectedText();
    for (int i = str.size(); --i >= 0; ) {
        QChar c = str.at(i);
        str[i] = c.toLower();
    }
    tc->insertText(str);
}

void FakeVimHandler::Private::upCaseSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    transformText(range, &FakeVimHandler::Private::upCaseTransform);
}

void FakeVimHandler::Private::upCaseTransform(int updateMarksAfter, QTextCursor *tc)
{
    Q_UNUSED(updateMarksAfter);
    QString str = tc->selectedText();
    tc->removeSelectedText();
    for (int i = str.size(); --i >= 0; ) {
        QChar c = str.at(i);
        str[i] = c.toUpper();
    }
    tc->insertText(str);
}

void FakeVimHandler::Private::invertCaseSelectedText()
{
    Range range(anchor(), position());
    range.rangemode = m_rangemode;
    transformText(range, &FakeVimHandler::Private::invertCaseTransform);
}

void FakeVimHandler::Private::invertCaseTransform(int updateMarksAfter, QTextCursor *tc)
{
    Q_UNUSED(updateMarksAfter);
    QString str = tc->selectedText();
    tc->removeSelectedText();
    for (int i = str.size(); --i >= 0; ) {
        QChar c = str.at(i);
        str[i] = c.isUpper() ? c.toLower() : c.toUpper();
    }
    tc->insertText(str);
}

void FakeVimHandler::Private::pasteText(bool afterCursor)
{
    const QString text = m_registers[m_register].contents;
    const QStringList lines = text.split(QChar('\n'));
    switch (m_registers[m_register].rangemode) {
        case RangeCharMode: {
            m_targetColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (afterCursor && rightDist() > 0)
                    moveRight();
                fixMarks(position(), text.length());
                m_tc.insertText(text);
                if (!afterCursor && atEndOfLine())
                    moveLeft();
                moveLeft();
            }
            break;
        }
        case RangeLineMode:
        case RangeLineModeExclusive: {
            moveToStartOfLine();
            m_targetColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (afterCursor)
                    moveDown();
                fixMarks(position(), text.length());
                m_tc.insertText(text);
                moveUp(lines.size() - 1);
            }
            moveToFirstNonBlankOnLine();
            break;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            beginEditBlock();
            QTextBlock block = m_tc.block();
            if (afterCursor)
                moveRight();
            QTextCursor tc = m_tc;
            const int col = tc.position() - block.position();
            //for (int i = lines.size(); --i >= 0; ) {
            for (int i = 0; i < lines.size(); ++i) {
                const QString line = lines.at(i);
                tc.movePosition(StartOfLine, MoveAnchor);
                if (col >= block.length()) {
                    tc.movePosition(EndOfLine, MoveAnchor);
                    fixMarks(position(), col - line.size() + 1);
                    tc.insertText(QString(col - line.size() + 1, QChar(' ')));
                } else {
                    tc.movePosition(Right, MoveAnchor, col - 1 + afterCursor);
                }
                qDebug() << "INSERT " << line << " AT " << tc.position()
                    << "COL: " << col;
                fixMarks(position(), line.length());
                tc.insertText(line);
                tc.movePosition(StartOfLine, MoveAnchor);
                tc.movePosition(Down, MoveAnchor, 1);
                if (tc.position() >= lastPositionInDocument() - 1) {
                    fixMarks(position(), 1);
                    tc.insertText(QString(QChar('\n')));
                    tc.movePosition(Up, MoveAnchor, 1);
                }
                block = block.next();
            }
            moveLeft();
            endEditBlock();
            break;
        }
    }
}

//FIXME: This needs to called after undo/insert
void FakeVimHandler::Private::fixMarks(int positionAction, int positionChange)
{
    QHashIterator<int, int> i(m_marks);
    while (i.hasNext()) {
        i.next();
        if (i.value() >= positionAction) {
            if (i.value() + positionChange > 0)
                m_marks[i.key()] = i.value() + positionChange;
            else
                m_marks.remove(i.key());
        }
    }
}

QString FakeVimHandler::Private::lineContents(int line) const
{
    return m_tc.document()->findBlockByNumber(line - 1).text();
}

void FakeVimHandler::Private::setLineContents(int line, const QString &contents) const
{
    QTextBlock block = m_tc.document()->findBlockByNumber(line - 1);
    QTextCursor tc = m_tc;
    tc.setPosition(block.position());
    tc.setPosition(block.position() + block.length() - 1, KeepAnchor);
    tc.removeSelectedText();
    tc.insertText(contents);
}

int FakeVimHandler::Private::firstPositionInLine(int line) const
{
    return m_tc.document()->findBlockByNumber(line - 1).position();
}

int FakeVimHandler::Private::lastPositionInLine(int line) const
{
    QTextBlock block = m_tc.document()->findBlockByNumber(line - 1);
    return block.position() + block.length() - 1;
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
    m_positionPastEnd = m_anchorPastEnd = false;
    m_visualMode = visualMode;
    m_marks['<'] = m_tc.position();
    m_marks['>'] = m_tc.position();
    updateMiniBuffer();
    updateSelection();
}

void FakeVimHandler::Private::leaveVisualMode()
{
    if (isVisualLineMode())
        m_movetype = MoveLineWise;
    else if (isVisualCharMode())
        m_movetype = MoveInclusive;

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
    //qDebug() << " CURSOR POS: " << m_undoCursorPosition;
    int current = m_tc.document()->availableUndoSteps();
    //endEditBlock();
    EDITOR(undo());
    //beginEditBlock();
    int rev = m_tc.document()->availableUndoSteps();
    if (current == rev)
        showBlackMessage(FakeVimHandler::tr("Already at oldest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        m_tc.setPosition(m_undoCursorPosition[rev]);
    if (atEndOfLine())
        moveLeft();
}

void FakeVimHandler::Private::redo()
{
    int current = m_tc.document()->availableUndoSteps();
    //endEditBlock();
    EDITOR(redo());
    //beginEditBlock();
    int rev = m_tc.document()->availableUndoSteps();
    if (rev == current)
        showBlackMessage(FakeVimHandler::tr("Already at newest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        m_tc.setPosition(m_undoCursorPosition[rev]);
}

void FakeVimHandler::Private::enterInsertMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(false));
    //leaveVisualMode();
    m_mode = InsertMode;
    m_lastInsertion.clear();
    m_beginEditBlock = true;
}

void FakeVimHandler::Private::enterCommandMode()
{
    EDITOR(setCursorWidth(m_cursorWidth));
    EDITOR(setOverwriteMode(true));
    if (atEndOfLine())
        moveLeft();
    m_mode = CommandMode;
}

void FakeVimHandler::Private::enterExMode()
{
    EDITOR(setCursorWidth(0));
    EDITOR(setOverwriteMode(false));
    m_mode = ExMode;
}

void FakeVimHandler::Private::recordJump()
{
    m_jumpListUndo.append(cursorPosition());
    m_jumpListRedo.clear();
    UNDO_DEBUG("jumps: " << m_jumpListUndo);
}

void FakeVimHandler::Private::recordNewUndo()
{
    //endEditBlock();
    UNDO_DEBUG("---- BREAK ----");
    //beginEditBlock();
}

Column FakeVimHandler::Private::indentation(const QString &line) const
{
    int ts = config(ConfigTabStop).toInt();
    int physical = 0;
    int logical = 0;
    int n = line.size();
    while (physical < n) {
        QChar c = line.at(physical);
        if (c == QLatin1Char(' '))
            ++logical;
        else if (c == QLatin1Char('\t'))
            logical += ts - logical % ts;
        else
            break;
        ++physical;
    }
    return Column(physical, logical);
}

QString FakeVimHandler::Private::tabExpand(int n) const
{
    int ts = config(ConfigTabStop).toInt();
    if (hasConfig(ConfigExpandTab) || ts < 1)
        return QString(n, QLatin1Char(' '));
    return QString(n / ts, QLatin1Char('\t'))
         + QString(n % ts, QLatin1Char(' '));
}

void FakeVimHandler::Private::insertAutomaticIndentation(bool goingDown)
{
    if (!hasConfig(ConfigAutoIndent) && !hasConfig(ConfigSmartIndent))
        return;

    if (hasConfig(ConfigSmartIndent)) {
        Range range(m_tc.block().position(), m_tc.block().position());
        m_justAutoIndented = indentText(range, QLatin1Char('\n'));
    } else {
        QTextBlock block = goingDown ? m_tc.block().previous() : m_tc.block().next();
        QString text = block.text();
        int pos = 0;
        int n = text.size();
        while (pos < n && text.at(pos).isSpace())
            ++pos;
        text.truncate(pos);
        // FIXME: handle 'smartindent' and 'cindent'
        m_tc.insertText(text);
        m_justAutoIndented = text.size();
    }
}

bool FakeVimHandler::Private::removeAutomaticIndentation()
{
    if (!hasConfig(ConfigAutoIndent) || m_justAutoIndented == 0)
        return false;
    m_tc.movePosition(StartOfLine, KeepAnchor);
    m_tc.removeSelectedText();
    m_lastInsertion.chop(m_justAutoIndented);
    m_justAutoIndented = 0;
    return true;
}

void FakeVimHandler::Private::handleStartOfLine()
{
    if (hasConfig(ConfigStartOfLine))
        moveToFirstNonBlankOnLine();
}

void FakeVimHandler::Private::replay(const QString &command, int n)
{
    //qDebug() << "REPLAY: " << command;
    m_inReplay = true;
    for (int i = n; --i >= 0; ) {
        foreach (QChar c, command) {
            //qDebug() << "  REPLAY: " << QString(c);
            handleKey(c.unicode(), c.unicode(), QString(c));
        }
    }
    m_inReplay = false;
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

// gracefully handle that the parent editor is deleted
void FakeVimHandler::disconnectFromEditor()
{
    d->m_textedit = 0;
    d->m_plaintextedit = 0;
}

bool FakeVimHandler::eventFilter(QObject *ob, QEvent *ev)
{
    bool active = theFakeVimSetting(ConfigUseFakeVim)->value().toBool();

    if (active && ev->type() == QEvent::Shortcut) {
        d->passShortcuts(false);
        return false;
    }

    if (active && ev->type() == QEvent::KeyPress && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        KEY_DEBUG("KEYPRESS" << kev->key());
        EventResult res = d->handleEvent(kev);
        // returning false core the app see it
        //KEY_DEBUG("HANDLED CODE:" << res);
        //return res != EventPassedToCore;
        //return true;
        return res == EventHandled;
    }

    if (active && ev->type() == QEvent::ShortcutOverride && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        if (d->wantsOverride(kev)) {
            KEY_DEBUG("OVERRIDING SHORTCUT" << kev->key());
            ev->accept(); // accepting means "don't run the shortcuts"
            return true;
        }
        KEY_DEBUG("NO SHORTCUT OVERRIDE" << kev->key());
        return true;
    }

    if (active && ev->type() == QEvent::FocusIn && ob == d->editor()) {
        d->stopIncrementalFind();
    }

    return QObject::eventFilter(ob, ev);
}

void FakeVimHandler::installEventFilter()
{
    d->installEventFilter();
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
    d->handleCommand(cmd);
}

void FakeVimHandler::setCurrentFileName(const QString &fileName)
{
   d->m_currentFileName = fileName;
}

void FakeVimHandler::showBlackMessage(const QString &msg)
{
   d->showBlackMessage(msg);
}

void FakeVimHandler::showRedMessage(const QString &msg)
{
   d->showRedMessage(msg);
}


QWidget *FakeVimHandler::widget()
{
    return d->editor();
}

// Test only
int FakeVimHandler::physicalIndentation(const QString &line) const
{
    Column ind = d->indentation(line);
    return ind.physical;
}

int FakeVimHandler::logicalIndentation(const QString &line) const
{
    Column ind = d->indentation(line);
    return ind.logical;
}

QString FakeVimHandler::tabExpand(int n) const
{
    return d->tabExpand(n);
}


} // namespace Internal
} // namespace FakeVim
