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
//

#include "fakevimhandler.h"
#include "fakevimsyntax.h"

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
#include <QtGui/QInputMethodEvent>
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

/*! A \e Mode represents one of the basic modes of operation of FakeVim.
*/

enum Mode
{
    InsertMode,
    ReplaceMode,
    CommandMode,
    ExMode,
};

/*! A \e SubMode is used for things that require one more data item
    and are 'nested' behind a \l Mode.
*/
enum SubMode
{
    NoSubMode,
    ChangeSubMode,     // Used for c
    DeleteSubMode,     // Used for d
    FilterSubMode,     // Used for !
    IndentSubMode,     // Used for =
    RegisterSubMode,   // Used for "
    ShiftLeftSubMode,  // Used for <
    ShiftRightSubMode, // Used for >
    TransformSubMode,  // Used for ~/gu/gU
    WindowSubMode,     // Used for Ctrl-w
    YankSubMode,       // Used for y
    ZSubMode,          // Used for z
    CapitalZSubMode    // Used for Z
};

/*! A \e SubSubMode is used for things that require one more data item
    and are 'nested' behind a \l SubMode.
*/
enum SubSubMode
{
    NoSubSubMode,
    FtSubSubMode,         // Used for f, F, t, T.
    MarkSubSubMode,       // Used for m.
    BackTickSubSubMode,   // Used for `.
    TickSubSubMode,       // Used for '.
    InvertCaseSubSubMode, // Used for ~.
    DownCaseSubSubMode,   // Used for gu.
    UpCaseSubSubMode,     // Used for gU.
    ReplaceSubSubMode,    // Used for r after visual mode.
    TextObjectSubSubMode, // Used for thing like iw, aW, as etc.
    SearchSubSubMode,
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

/*!
    \enum RangeMode

    The \e RangeMode serves as a means to define how the "Range" between
    the \l cursor and the \l anchor position is to be interpreted.

    \value RangeCharMode   Entered by pressing \key v. The range includes
                           all characters between cursor and anchor.
    \value RangeLineMode   Entered by pressing \key V. The range includes
                           all lines between the line of the cursor and
                           the line of the anchor.
    \value RangeLineModeExclusice Like \l RangeLineMode, but keeps one
                           newline when deleting.
    \value RangeBlockMode  Entered by pressing \key Ctrl-v. The range includes
                           all characters with line and column coordinates
                           between line and columns coordinates of cursor and
                           anchor.
    \value RangeBlockAndTailMode Like \l RangeBlockMode, but also includes
                           all characters in the affected lines up to the end
                           of these lines.
*/
enum RangeMode
{
    RangeCharMode,         // v
    RangeLineMode,         // V
    RangeLineModeExclusive,
    RangeBlockMode,        // Ctrl-v
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
    int physical; // Number of characters in the data.
    int logical; // Column on screen.
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
        const QChar c = ba.at(i);
        const int cc = c.unicode();
        if (c.isPrint())
            res += c;
        else if (cc == '\n')
            res += QLatin1String("<CR>");
        else
            res += QString("\\x%1").arg(c.unicode(), 2, 16, QLatin1Char('0'));
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

class Input
{
public:
    Input()
        : m_key(0), m_xkey(0), m_modifiers(0) {}

    explicit Input(QChar x)
        : m_key(x.unicode()), m_xkey(x.unicode()), m_modifiers(0), m_text(x) {}

    Input(int k, int m, QString t)
        : m_key(k), m_modifiers(m), m_text(t)
    {
        // m_xkey is only a cache.
        m_xkey = (m_text.size() == 1 ? m_text.at(0).unicode() : m_key);
    }

    bool isDigit() const
    {
        return m_xkey >= '0' && m_xkey <= '9';
    }
    bool isKey(int c) const
    {
        return !m_modifiers && m_key == c;
    }

    bool is(int c) const
    {
        return m_xkey == c && (m_modifiers == 0 || m_modifiers == Qt::ShiftModifier);
    }

    bool isControl(int c) const
    {
        return m_modifiers == Qt::ControlModifier &&
            (m_xkey == c || m_xkey + 32 == c || m_xkey + 64 == c || m_xkey + 96 == c);
    }

    bool isShift(int c) const
    {
        return m_modifiers == Qt::ShiftModifier && m_xkey == c;
    }

    bool operator==(const Input &a) const
    {
        return a.m_key == m_key && m_text == a.m_text;
    }

    QString text() const { return m_text; }

    int key() const { return m_key; }

private:
    int m_key;
    int m_xkey;
    int m_modifiers;
    QString m_text;
};

QDebug &operator<<(QDebug &ts, const Input &input)
{
    return ts << input.text() << input.key();
}

typedef QVector<Input> Inputs;

// Mappings for a specific mode.
class ModeMapping : private QList<QPair<Inputs, Inputs> >
{
public:
    ModeMapping() { test(); }

    void test()
    {
        //insert(Inputs() << Input('A') << Input('A'),
        //       Inputs() << Input('x') << Input('x'));
    }

    void insert(const Inputs &from, const Inputs &to)
    {
        for (int i = 0; i != size(); ++i)
            if (at(i).first == from) {
                (*this)[i].second = to;
                return;
            }
        append(QPair<Inputs, Inputs>(from, to));
    }

    void remove(const Inputs &from)
    {
        for (int i = 0; i != size(); ++i)
            if (at(i).first == from) {
                removeAt(i);
                return;
            }
    }

    // Returns 'false' if more input input is needed to decide whether a
    // mapping needs to be applied. If a decision can be made, return 'true',
    // and replace *input with the mapped data.
    bool mappingDone(Inputs *input) const
    {
        Q_UNUSED(input);
        // FIXME: inefficient.
        for (int i = 0; i != size(); ++i) {
            // A mapping
            if (startsWith(at(i).first, *input)) {
                if (at(i).first.size() != input->size())
                    return false; // This can be extended.
                // Actual mapping.
                *input = at(i).second;
                return true;
            }
        }
        // No extensible mapping found. Use input as-is.
        return true;
    }

private:
    static bool startsWith(const Inputs &haystack, const Inputs &needle)
    {
        // Input is already too long.
        if (needle.size() > haystack.size())
            return false;
        for (int i = 0; i != needle.size(); ++i) {
            if (needle.at(i).text() != haystack.at(i).text())
                return false;
        }
        return true;
    }
};


class FakeVimHandler::Private : public QObject
{
    Q_OBJECT

public:
    Private(FakeVimHandler *parent, QWidget *widget);

    EventResult handleEvent(QKeyEvent *ev);
    bool wantsOverride(QKeyEvent *ev);
    void handleCommand(const QString &cmd); // Sets m_tc + handleExCommand
    void handleExCommand(const QString &cmd);

    // updates marks positions by the difference in positionChange
    void fixMarks(int positionAction, int positionChange);

    void installEventFilter();
    void passShortcuts(bool enable);
    void setupWidget();
    void restoreWidget(int tabSize);

    friend class FakeVimHandler;

    void init();
    EventResult handleKey(const Input &);
    Q_SLOT EventResult handleKey2();
    EventResult handleInsertMode(const Input &);
    EventResult handleReplaceMode(const Input &);
    EventResult handleCommandMode(const Input &);
    EventResult handleRegisterMode(const Input &);
    EventResult handleExMode(const Input &);
    EventResult handleSearchSubSubMode(const Input &);
    EventResult handleCommandSubSubMode(const Input &);
    void finishMovement(const QString &dotCommand = QString());
    void finishMovement(const QString &dotCommand, int count);
    void resetCommandMode();
    void search(const QString &needle, bool forward, bool incSearch = false);
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
    void setAnchor();
    void setAnchor(int position) { if (!isVisualMode()) m_anchor = position; }
    void setPosition(int position) { m_tc.setPosition(position, MoveAnchor); }

    bool handleFfTt(QString key);

    // helper function for handleExCommand. return 1 based line index.
    int readLineCode(QString &cmd);
    void selectRange(int beginLine, int endLine);

    void enterInsertMode();
    void enterReplaceMode();
    void enterCommandMode();
    void enterExMode();
    void showRedMessage(const QString &msg);
    void showBlackMessage(const QString &msg);
    void notImplementedYet();
    void updateMiniBuffer();
    void updateSelection();
    void updateCursor();
    QWidget *editor() const;
    QChar characterAtCursor() const
        { return m_tc.document()->characterAt(m_tc.position()); }
    void beginEditBlock() { UNDO_DEBUG("BEGIN EDIT BLOCK"); m_tc.beginEditBlock(); }
    void beginEditBlock(int pos) { setUndoPosition(pos); beginEditBlock(); }
    void endEditBlock() { UNDO_DEBUG("END EDIT BLOCK"); m_tc.endEditBlock(); }
    void joinPreviousEditBlock() { UNDO_DEBUG("JOIN"); m_tc.joinPreviousEditBlock(); }
    void breakEditBlock()
        { m_tc.beginEditBlock(); m_tc.insertText("x");
          m_tc.deletePreviousChar(); m_tc.endEditBlock(); }

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

    void selectWordTextObject(bool inner);
    void selectWORDTextObject(bool inner);
    void selectSentenceTextObject(bool inner);
    void selectParagraphTextObject(bool inner);
    void selectBlockTextObject(bool inner, char left, char right);
    void selectQuotedStringTextObject(bool inner, int type);

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    bool m_wasReadOnly; // saves read-only state of document

    FakeVimHandler *q;
    Mode m_mode;
    bool m_passing; // let the core see the next event
    SubMode m_submode;
    SubSubMode m_subsubmode;
    Input m_subsubdata;
    QTextCursor m_tc;
    int m_oldPosition; // copy from last event to check for external changes
    int m_anchor;
    int m_register;
    QString m_mvcount;
    QString m_opcount;
    MoveType m_movetype;
    RangeMode m_rangemode;
    int m_visualInsertCount;

    bool m_fakeEnd;
    bool m_anchorPastEnd;
    bool m_positionPastEnd; // '$' & 'l' in visual mode can move past eol

    int m_gflag;  // whether current command started with 'g'

    QString m_commandPrefix;
    QString m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    bool m_findPending;
    QString m_lastInsertion;
    QString m_lastDeletion;

    int anchor() const { return m_anchor; }
    int position() const { return m_tc.position(); }

    struct TransformationData
    {
        TransformationData(const QString &s, const QVariant &d)
            : from(s), extraData(d) {}
        QString from;
        QString to;
        QVariant extraData;
    };
    typedef void (Private::*Transformation)(TransformationData *td);
    void transformText(const Range &range, Transformation transformation,
        const QVariant &extraData = QVariant());

    void removeSelectedText();
    void removeText(const Range &range);
    void removeTransform(TransformationData *td);

    void invertCaseSelectedText();
    void invertCaseTransform(TransformationData *td);

    void upCaseSelectedText();
    void upCaseTransform(TransformationData *td);

    void downCaseSelectedText();
    void downCaseTransform(TransformationData *td);

    QString m_replacement;
    void replaceSelectedText(); // replace each character with m_replacement
    void replaceTransform(TransformationData *td);

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

    // extra data for '.'
    void replay(const QString &text, int count);
    void setDotCommand(const QString &cmd) { g.dotCommand = cmd; }
    void setDotCommand(const QString &cmd, int n) { g.dotCommand = cmd.arg(n); }

    // extra data for ';'
    QString m_semicolonCount;
    Input m_semicolonType;  // 'f', 'F', 't', 'T'
    QString m_semicolonKey;

    // history for '/'
    QString lastSearchString() const;

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

    bool handleExCommandHelper(const QString &cmd); // Returns success.
    QString extractCommand(const QString &line, int *beginLine, int *endLine);
    bool handleExBangCommand(const QString &line);
    bool handleExDeleteCommand(const QString &line);
    bool handleExGotoCommand(const QString &line);
    bool handleExHistoryCommand(const QString &line);
    bool handleExMapCommand(const QString &line);
    bool handleExNormalCommand(const QString &line);
    bool handleExReadCommand(const QString &line);
    bool handleExRedoCommand(const QString &line);
    bool handleExSetCommand(const QString &line);
    bool handleExShiftRightCommand(const QString &line);
    bool handleExSourceCommand(const QString &line);
    bool handleExSubstituteCommand(const QString &line);
    bool handleExWriteCommand(const QString &line);

    void timerEvent(QTimerEvent *ev);

    void setupCharClass();
    int charClass(QChar c, bool simple) const;
    signed char m_charClass[256];

    static struct GlobalData
    {
        GlobalData()
        {
            inReplay = false;
            inputTimer = -1;
            searchHistoryIndex = 0;
            commandHistoryIndex = 0;
        }

        // Input.
        QVector<Input> pendingInput;
        int inputTimer;

        // Repetition.
        QString dotCommand;
        bool inReplay; // true if we are executing a '.'

        // Histories.
        QStringList searchHistory;
        int searchHistoryIndex;
        QStringList commandHistory;
        int commandHistoryIndex;

        QHash<int, Register> registers;

        // All mappings.
        typedef QHash<char, ModeMapping> Mappings;
        Mappings mappings;
    } g;
};

FakeVimHandler::Private::GlobalData FakeVimHandler::Private::g;

FakeVimHandler::Private::Private(FakeVimHandler *parent, QWidget *widget)
{
    //static PythonHighlighterRules pythonRules;
    q = parent;
    m_textedit = qobject_cast<QTextEdit *>(widget);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    //new Highlighter(EDITOR(document()), &pythonRules);
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
    m_justAutoIndented = 0;
    m_rangemode = RangeCharMode;

    setupCharClass();
}

bool FakeVimHandler::Private::wantsOverride(QKeyEvent *ev)
{
    const int key = ev->key();
    const int mods = ev->modifiers();
    KEY_DEBUG("SHORTCUT OVERRIDE" << key << "  PASSING: " << m_passing);

    if (key == Key_Escape) {
        if (m_subsubmode == SearchSubSubMode)
            return true;
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
    const int key = ev->key();
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
        //if (input.is(',')) { // use ',,' to leave, too.
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

    //if ((mods & Qt::ControlModifier) != 0) {
    //    if (key >= Key_A && key <= Key_Z)
    //        key = shift(key); // make it lower case
    //    key = control(key);
    //} else if (key >= Key_A && key <= Key_Z && (mods & Qt::ShiftModifier) == 0) {
    //    key = shift(key);
    //}

    QTC_ASSERT(m_mode == InsertMode || m_mode == ReplaceMode
            || !m_tc.atBlockEnd() || m_tc.block().length() <= 1,
        qDebug() << "Cursor at EOL before key handler");

    EventResult result = handleKey(Input(key, mods, ev->text()));

    // the command might have destroyed the editor
    if (m_textedit || m_plaintextedit) {
        // We fake vi-style end-of-line behaviour
        m_fakeEnd = atEndOfLine() && m_mode == CommandMode && !isVisualBlockMode();

        QTC_ASSERT(m_mode == InsertMode || m_mode == ReplaceMode
                || !m_tc.atBlockEnd() || m_tc.block().length() <= 1,
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
}

void FakeVimHandler::Private::setupWidget()
{
    enterCommandMode();
    if (m_textedit) {
        m_textedit->setLineWrapMode(QTextEdit::NoWrap);
    } else if (m_plaintextedit) {
        m_plaintextedit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    m_wasReadOnly = EDITOR(isReadOnly());

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

    updateMiniBuffer();
    updateCursor();
}

void FakeVimHandler::Private::updateEditor()
{
    const int charWidth = QFontMetrics(EDITOR(font())).width(QChar(' '));
    EDITOR(setTabStopWidth(charWidth * config(ConfigTabStop).toInt()));

    setupCharClass();
}

void FakeVimHandler::Private::restoreWidget(int tabSize)
{
    //showBlackMessage(QString());
    //updateMiniBuffer();
    //EDITOR(removeEventFilter(q));
    //EDITOR(setReadOnly(m_wasReadOnly));
    const int charWidth = QFontMetrics(EDITOR(font())).width(QChar(' '));
    EDITOR(setTabStopWidth(charWidth * tabSize));

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
    // Force "ordinary" cursor.
    m_mode = InsertMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    updateCursor();
    updateSelection();
}

EventResult FakeVimHandler::Private::handleKey(const Input &input)
{
    if (m_mode == ExMode)
        return handleExMode(input);
    if (m_subsubmode == SearchSubSubMode)
        return handleSearchSubSubMode(input);
    if (m_mode == InsertMode || m_mode == ReplaceMode || m_mode == CommandMode) {
        g.pendingInput.append(input);
        const char code = m_mode == InsertMode ? 'i' : 'n';
        if (g.mappings[code].mappingDone(&g.pendingInput))
            return handleKey2();
        if (g.inputTimer != -1)
            killTimer(g.inputTimer);
        g.inputTimer = startTimer(1000);
        return EventHandled;
    }
    return EventUnhandled;
}

EventResult FakeVimHandler::Private::handleKey2()
{
    setUndoPosition(m_tc.position());
    if (m_mode == InsertMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, g.pendingInput) {
            EventResult r = handleInsertMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
        g.pendingInput.clear();
        return result;
    }
    if (m_mode == ReplaceMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, g.pendingInput) {
            EventResult r = handleReplaceMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
        g.pendingInput.clear();
        return result;
    }
    if (m_mode == CommandMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, g.pendingInput) {
            EventResult r = handleCommandMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
        g.pendingInput.clear();
        return result;
    }
    return EventUnhandled;
}

void FakeVimHandler::Private::timerEvent(QTimerEvent *ev)
{
    Q_UNUSED(ev);
    handleKey2();
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

void FakeVimHandler::Private::setAnchor()
{
    // FIXME: This indicates that the concept of 'Anchor' is broken.
    if (!isVisualMode()) {
        m_anchor = m_tc.position();
    } else {
    //    m_marks['<'] = m_tc.position();
    }
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
        m_commandPrefix = QChar();
        m_commandBuffer = QString(".,+%1!").arg(qAbs(endLine - beginLine));
        g.commandHistory.append(QString());
        g.commandHistoryIndex = g.commandHistory.size() - 1;
        updateMiniBuffer();
        updateCursor();
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
                if (!m_tc.atBlockEnd())
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
                g.registers[m_register].rangemode = RangeLineMode;
        }

        m_positionPastEnd = m_anchorPastEnd = false;
    }

    if (m_submode == ChangeSubMode) {
        if (m_rangemode == RangeLineMode)
            m_rangemode = RangeLineModeExclusive;
        removeSelectedText();
        if (!dotCommand.isEmpty())
            setDotCommand(QLatin1Char('c') + dotCommand);
        if (m_movetype == MoveLineWise)
            insertAutomaticIndentation(true);
        endEditBlock();
        enterInsertMode();
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
        } else if (m_subsubmode == ReplaceSubSubMode) {
            replaceSelectedText();
            if (!dotCommand.isEmpty())
                setDotCommand("r" + dotCommand);
        }
        m_submode = NoSubMode;
        m_subsubmode = NoSubSubMode;
        setPosition(qMin(anchor(), position()));
        if (m_movetype == MoveLineWise)
            handleStartOfLine();
        endEditBlock();
    } else if (m_submode == IndentSubMode) {
        recordJump();
        beginEditBlock();
        indentSelectedText();
        endEditBlock();
        m_submode = NoSubMode;
    } else if (m_submode == ShiftRightSubMode) {
        recordJump();
        shiftRegionRight(1);
        m_submode = NoSubMode;
    } else if (m_submode == ShiftLeftSubMode) {
        recordJump();
        shiftRegionLeft(1);
        m_submode = NoSubMode;
    }

    resetCommandMode();
    updateSelection();
    updateMiniBuffer();
    updateCursor();
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
        const int cursorPos = m_tc.position();
        const int anchorPos = m_marks['<'];
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
        msg = "-- INSERT --";
    } else if (m_mode == ReplaceMode) {
        msg = "-- REPLACE --";
    } else {
        if (m_subsubmode == SearchSubSubMode)
            msg += m_commandPrefix;
        else if (m_mode == ExMode)
            msg += m_commandPrefix;
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

static bool subModeCanUseTextObjects(int submode)
{
    return submode == DeleteSubMode
        || submode == YankSubMode
        || submode == ChangeSubMode;
}

EventResult FakeVimHandler::Private::handleCommandSubSubMode(const Input &input)
{
    //const int key = input.key;
    EventResult handled = EventHandled;
    if (m_subsubmode == FtSubSubMode) {
        m_semicolonType = m_subsubdata;
        m_semicolonKey = input.text();
        bool valid = handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        if (!valid) {
            m_submode = NoSubMode;
            finishMovement();
        } else {
            finishMovement(QString("%1%2%3")
                           .arg(count())
                           .arg(m_semicolonType.text())
                           .arg(m_semicolonKey));
        }
    } else if (m_subsubmode == TextObjectSubSubMode) {
        if (input.is('w'))
            selectWordTextObject(m_subsubdata.is('i'));
        else if (input.is('W'))
            selectWORDTextObject(m_subsubdata.is('i'));
        else if (input.is('s'))
            selectSentenceTextObject(m_subsubdata.is('i'));
        else if (input.is('p'))
            selectParagraphTextObject(m_subsubdata.is('i'));
        else if (input.is('[') || input.is(']'))
            selectBlockTextObject(m_subsubdata.is('i'), '[', ']');
        else if (input.is('(') || input.is(')') || input.is('b'))
            selectBlockTextObject(m_subsubdata.is('i'), '(', ')');
        else if (input.is('<') || input.is('>'))
            selectBlockTextObject(m_subsubdata.is('i'), '<', '>');
        else if (input.is('"') || input.is('\'') || input.is('`'))
            selectQuotedStringTextObject(m_subsubdata.is('i'), input.key());
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (m_submode == TransformSubMode && m_subsubmode == ReplaceSubSubMode) {
        if (isVisualLineMode())
            m_rangemode = RangeLineMode;
        else if (isVisualBlockMode())
            m_rangemode = RangeBlockMode;
        // FIXME: Consolidate.
        if (isVisualMode()) {
            leaveVisualMode();
            m_replacement = input.text();
            finishMovement();
        } else {
            Range range(position(), position() + count(), RangeCharMode);
            m_replacement = input.text();
            Transformation tr = &FakeVimHandler::Private::replaceTransform;
            transformText(range, tr);
            m_subsubmode = NoSubSubMode;
            m_submode = NoSubMode;
            setDotCommand("%1r" + input.text(), count());
            finishMovement();
        }
    } else if (m_subsubmode == MarkSubSubMode) {
        m_marks[input.key()] = m_tc.position();
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        if (m_marks.contains(input.key())) {
            setPosition(m_marks[input.key()]);
            if (m_subsubmode == TickSubSubMode)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            showRedMessage(msgE20MarkNotSet(input.text()));
        }
        m_subsubmode = NoSubSubMode;
    } else {
        handled = EventUnhandled;
    }
    return handled;
}

EventResult FakeVimHandler::Private::handleCommandMode(const Input &input)
{
    EventResult handled = EventHandled;

    if (input.isKey(Key_Escape) || input.isControl(Key_BracketLeft)) {
        if (isVisualMode()) {
            leaveVisualMode();
        } else if (m_submode != NoSubMode) {
            m_submode = NoSubMode;
            m_subsubmode = NoSubSubMode;
            finishMovement();
        } else {
            resetCommandMode();
            updateSelection();
            updateMiniBuffer();
        }
    } else if (m_subsubmode != NoSubSubMode) {
        handleCommandSubSubMode(input);
    } else if (m_submode == WindowSubMode) {
        emit q->windowCommandRequested(input.key());
        m_submode = NoSubMode;
    } else if (m_submode == RegisterSubMode) {
        m_register = input.key();
        m_submode = NoSubMode;
        m_rangemode = RangeLineMode;
    } else if (m_submode == ChangeSubMode && input.is('c')) { // tested
        moveToStartOfLine();
        setAnchor();
        moveDown(count() - 1);
        moveToEndOfLine();
        m_movetype = MoveLineWise;
        m_lastInsertion.clear();
        setDotCommand("%1cc", count());
        finishMovement();
    } else if (m_submode == DeleteSubMode && input.is('d')) { // tested
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
    } else if ((subModeCanUseTextObjects(m_submode) || isVisualMode())
            && (input.is('a') || input.is('i'))) {
        m_subsubmode = TextObjectSubSubMode;
        m_subsubdata = input;
    } else if (m_submode == ShiftLeftSubMode && input.is('<')) {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1<<", count());
        finishMovement();
    } else if (m_submode == ShiftRightSubMode && input.is('>')) {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1>>", count());
        finishMovement();
    } else if (m_submode == IndentSubMode && input.is('=')) {
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1==", count());
        finishMovement();
    } else if (m_submode == ZSubMode) {
        //qDebug() << "Z_MODE " << cursorLineInDocument() << linesOnScreen();
        if (input.isKey(Key_Return) || input.is('t')) {
            // Cursor line to top of window.
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(- cursorLineOnScreen());
            if (input.isKey(Key_Return))
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (input.is('.') || input.is('z')) {
            // Cursor line to center of window.
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() / 2 - cursorLineOnScreen());
            if (input.is('.'))
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else if (input.is('-') || input.is('b')) {
            // Cursor line to bottom of window.
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(linesOnScreen() - cursorLineOnScreen());
            if (input.is('-'))
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            qDebug() << "IGNORED Z_MODE " << input.key() << input.text();
        }
        m_submode = NoSubMode;
    } else if (m_submode == CapitalZSubMode) {
        // Recognize ZZ and ZQ as aliases for ":x" and ":q!".
        m_submode = NoSubMode;
        if (input.is('Z'))
            handleExCommand(QString(QLatin1Char('x')));
        else if (input.is('Q'))
            handleExCommand("q!");
    } else if (input.isDigit()) {
        if (input.is('0') && m_mvcount.isEmpty()) {
            m_movetype = MoveExclusive;
            moveToStartOfLine();
            setTargetColumn();
            finishMovement(QString(QLatin1Char('0')));
        } else {
            m_mvcount.append(input.text());
        }
    } else if (input.is('^') || input.is('_')) {
        moveToFirstNonBlankOnLine();
        setTargetColumn();
        m_movetype = MoveExclusive;
        finishMovement(input.text());
    } else if (0 && input.is(',')) {
        // FIXME: fakevim uses ',' by itself, so it is incompatible
        m_subsubmode = FtSubSubMode;
        // HACK: toggle 'f' <-> 'F', 't' <-> 'T'
        //m_subsubdata = m_semicolonType ^ 32;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (input.is(';')) {
        m_subsubmode = FtSubSubMode;
        m_subsubdata = m_semicolonType;
        handleFfTt(m_semicolonKey);
        m_subsubmode = NoSubSubMode;
        finishMovement();
    } else if (input.is(':')) {
        enterExMode();
        m_currentMessage.clear();
        m_commandPrefix = input.text().at(0);
        m_commandBuffer.clear();
        if (isVisualMode())
            m_commandBuffer = "'<,'>";
        g.commandHistory.append(QString());
        g.commandHistoryIndex = g.commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (input.is('/') || input.is('?')) {
        m_lastSearchForward = input.is('/');
        if (hasConfig(ConfigUseCoreSearch)) {
            // re-use the core dialog.
            m_findPending = true;
            EDITOR(setTextCursor(m_tc));
            emit q->findRequested(!m_lastSearchForward);
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.selectionStart());
        } else {
            // FIXME: make core find dialog sufficiently flexible to
            // produce the "default vi" behaviour too. For now, roll our own.
            m_currentMessage.clear();
            m_movetype = MoveExclusive;
            m_subsubmode = SearchSubSubMode;
            m_commandPrefix = QLatin1Char(m_lastSearchForward ? '/' : '?');
            m_commandBuffer = QString();
            g.searchHistory.append(QString());
            g.searchHistoryIndex = g.searchHistory.size() - 1;
            updateCursor();
            updateMiniBuffer();
        }
    } else if (input.is('`')) {
        m_subsubmode = BackTickSubSubMode;
    } else if (input.is('#') || input.is('*')) {
        // FIXME: That's not proper vim behaviour
        m_tc.select(QTextCursor::WordUnderCursor);
        QString needle = "\\<" + m_tc.selection().toPlainText() + "\\>";
        g.searchHistory.append(needle);
        m_lastSearchForward = input.is('*');
        m_currentMessage.clear();
        m_commandPrefix = QLatin1Char(m_lastSearchForward ? '/' : '?');
        m_commandBuffer = needle;
        updateMiniBuffer();
        search(needle, m_lastSearchForward);
        recordJump();
    } else if (input.is('\'')) {
        m_subsubmode = TickSubSubMode;
    } else if (input.is('|')) {
        moveToStartOfLine();
        moveRight(qMin(count(), rightDist()) - 1);
        setTargetColumn();
        finishMovement();
    } else if (input.is('!') && isNoVisualMode()) {
        m_submode = FilterSubMode;
    } else if (input.is('!') && isVisualMode()) {
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer = "'<,'>!";
        g.commandHistory.append(QString());
        g.commandHistoryIndex = g.commandHistory.size() - 1;
        updateMiniBuffer();
    } else if (input.is('"')) {
        m_submode = RegisterSubMode;
    } else if (input.isKey(Key_Return)) {
        moveToStartOfLine();
        moveDown();
        moveToFirstNonBlankOnLine();
        m_movetype = MoveLineWise;
        finishMovement("%1j", count());
    } else if (input.is('-')) {
        moveToStartOfLine();
        moveUp(count());
        moveToFirstNonBlankOnLine();
        m_movetype = MoveLineWise;
        finishMovement("%1-", count());
    } else if (input.is('+')) {
        moveToStartOfLine();
        moveDown(count());
        moveToFirstNonBlankOnLine();
        m_movetype = MoveLineWise;
        finishMovement("%1+", count());
    } else if (input.isKey(Key_Home)) {
        moveToStartOfLine();
        setTargetColumn();
        finishMovement();
    } else if (input.is('$') || input.isKey(Key_End)) {
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
    } else if (input.is(',')) {
        passShortcuts(true);
    } else if (input.is('.')) {
        //qDebug() << "REPEATING" << quoteUnprintable(g.dotCommand) << count()
        //    << input;
        QString savedCommand = g.dotCommand;
        g.dotCommand.clear();
        replay(savedCommand, count());
        enterCommandMode();
        g.dotCommand = savedCommand;
    } else if (input.is('<') && isNoVisualMode()) {
        m_submode = ShiftLeftSubMode;
    } else if (input.is('<') && isVisualMode()) {
        shiftRegionLeft(1);
        leaveVisualMode();
    } else if (input.is('>') && isNoVisualMode()) {
        m_submode = ShiftRightSubMode;
    } else if (input.is('>') && isVisualMode()) {
        shiftRegionRight(1);
        leaveVisualMode();
    } else if (input.is('=') && isNoVisualMode()) {
        m_submode = IndentSubMode;
    } else if (input.is('=') && isVisualMode()) {
        beginEditBlock();
        indentSelectedText();
        endEditBlock();
        leaveVisualMode();
    } else if (input.is('%')) {
        setAnchor();
        moveToMatchingParanthesis();
        finishMovement();
    } else if ((!isVisualMode() && input.is('a')) || (isVisualMode() && input.is('A'))) {
        leaveVisualMode();
        enterInsertMode();
        m_lastInsertion.clear();
        if (!atEndOfLine())
            moveRight();
        updateMiniBuffer();
    } else if (input.is('A')) {
        enterInsertMode();
        moveBehindEndOfLine();
        setDotCommand(QString(QLatin1Char('A')));
        m_lastInsertion.clear();
        updateMiniBuffer();
    } else if (input.isControl('a')) {
        // FIXME: eat it to prevent the global "select all" shortcut to trigger
    } else if (input.is('b') || input.isShift(Key_Left)) {
        m_movetype = MoveExclusive;
        moveToWordBoundary(false, false);
        finishMovement();
    } else if (input.is('B')) {
        m_movetype = MoveExclusive;
        moveToWordBoundary(true, false);
        finishMovement();
    } else if (input.is('c') && isNoVisualMode()) {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = ChangeSubMode;
    } else if ((input.is('c') || input.is('C') || input.is('s') || input.is('R'))
          && (isVisualCharMode() || isVisualLineMode())) {
        if ((input.is('c')|| input.is('s')) && isVisualCharMode()) {
            leaveVisualMode();
            m_rangemode = RangeCharMode;
        } else {
            leaveVisualMode();
            m_rangemode = RangeLineMode;
            // leaveVisualMode() has set this to MoveInclusive for visual character mode
            m_movetype =  MoveLineWise;
        }
        m_submode = ChangeSubMode;
        finishMovement();
    } else if (input.is('C')) {
        setAnchor();
        moveToEndOfLine();
        m_submode = ChangeSubMode;
        setDotCommand(QString(QLatin1Char('C')));
        finishMovement();
    } else if (input.isControl('c')) {
        if (isNoVisualMode())
            showBlackMessage("Type Alt-v,Alt-v  to quit FakeVim mode");
        else
            leaveVisualMode();
    } else if (input.is('d') && isNoVisualMode()) {
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
    } else if ((input.is('d') || input.is('x')) && isVisualCharMode()) {
        leaveVisualMode();
        m_submode = DeleteSubMode;
        finishMovement();
    } else if ((input.is('d') || input.is('x')) && isVisualLineMode()) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        yankSelectedText();
        removeSelectedText();
        handleStartOfLine();
    } else if ((input.is('d') || input.is('x')) && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (input.is('D') && isNoVisualMode()) {
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = DeleteSubMode;
        moveDown(qMax(count() - 1, 0));
        m_movetype = MoveInclusive;
        moveToEndOfLine();
        setDotCommand(QString(QLatin1Char('D')));
        finishMovement();
    } else if ((input.is('D') || input.is('X')) &&
         (isVisualCharMode() || isVisualLineMode())) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        m_submode = NoSubMode;
        yankSelectedText();
        removeSelectedText();
        moveToFirstNonBlankOnLine();
    } else if ((input.is('D') || input.is('X')) && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockAndTailMode;
        yankSelectedText();
        removeSelectedText();
        setPosition(qMin(position(), anchor()));
    } else if (input.isControl('d')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (input.is('e') || input.isShift(Key_Right)) {
        m_movetype = MoveInclusive;
        moveToWordBoundary(false, true);
        finishMovement("%1e", count());
    } else if (input.is('E')) {
        m_movetype = MoveInclusive;
        moveToWordBoundary(true, true);
        finishMovement("%1E", count());
    } else if (input.isControl('e')) {
        // FIXME: this should use the "scroll" option, and "count"
        if (cursorLineOnScreen() == 0)
            moveDown(1);
        scrollDown(1);
        finishMovement();
    } else if (input.is('f')) {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveInclusive;
        m_subsubdata = input;
    } else if (input.is('F')) {
        m_subsubmode = FtSubSubMode;
        m_movetype = MoveExclusive;
        m_subsubdata = input;
    } else if (input.is('g') && !m_gflag) {
        m_gflag = true;
    } else if (input.is('g') || input.is('G')) {
        QString dotCommand = QString("%1G").arg(count());
        if (input.is('G') && m_mvcount.isEmpty())
            dotCommand = QString(QLatin1Char('G'));
        if (input.is('g'))
            m_gflag = false;
        int n = (input.is('g')) ? 1 : linesInDocument();
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
    } else if (input.is('h') || input.isKey(Key_Left)
            || input.isKey(Key_Backspace) || input.isControl('h')) {
        m_movetype = MoveExclusive;
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && m_tc.block().length() > 1)
            ++n;
        moveLeft(n);
        setTargetColumn();
        finishMovement("%1h", count());
    } else if (input.is('H')) {
        m_tc = EDITOR(cursorForPosition(QPoint(0, 0)));
        moveDown(qMax(count() - 1, 0));
        handleStartOfLine();
        finishMovement();
    } else if (!isVisualMode() && (input.is('i') || input.isKey(Key_Insert))) {
        setDotCommand(QString(QLatin1Char('i'))); // setDotCommand("%1i", count());
        enterInsertMode();
        updateMiniBuffer();
        if (atEndOfLine())
            moveLeft();
    } else if (input.is('I')) {
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
    } else if (input.isControl('i')) {
        if (!m_jumpListRedo.isEmpty()) {
            m_jumpListUndo.append(cursorPosition());
            setCursorPosition(m_jumpListRedo.last());
            m_jumpListRedo.pop_back();
        }
    } else if (input.is('j') || input.isKey(Key_Down)) {
        m_movetype = MoveLineWise;
        setAnchor();
        moveDown(count());
        finishMovement("%1j", count());
    } else if (input.is('J')) {
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
    } else if (input.is('k') || input.isKey(Key_Up)) {
        m_movetype = MoveLineWise;
        setAnchor();
        moveUp(count());
        finishMovement("%1k", count());
    } else if (input.is('l') || input.isKey(Key_Right) || input.is(' ')) {
        m_movetype = MoveExclusive;
        setAnchor();
        bool pastEnd = count() >= rightDist() - 1;
        moveRight(qMax(0, qMin(count(), rightDist() - (m_submode == NoSubMode))));
        setTargetColumn();
        if (pastEnd && isVisualMode()) {
            m_visualTargetColumn = -1;
        }
        finishMovement("%1l", count());
    } else if (input.is('L')) {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        moveUp(qMax(count(), 1));
        handleStartOfLine();
        finishMovement();
    } else if (input.isControl('l')) {
        // screen redraw. should not be needed
    } else if (input.is('m')) {
        m_subsubmode = MarkSubSubMode;
    } else if (input.is('M')) {
        m_tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        handleStartOfLine();
        finishMovement();
    } else if (input.is('n')) {
        search(lastSearchString(), m_lastSearchForward);
        recordJump();
    } else if (input.is('N')) {
        search(lastSearchString(), !m_lastSearchForward);
        recordJump();
    } else if (isVisualMode() && (input.is('o') || input.is('O'))) {
        int pos = position();
        setPosition(anchor());
        m_anchor = pos;
        std::swap(m_marks['<'], m_marks['>']);
        std::swap(m_positionPastEnd, m_anchorPastEnd);
        setTargetColumn();
        if (m_positionPastEnd)
            m_visualTargetColumn = -1;
        updateSelection();
    } else if (input.is('o') || input.is('O')) {
        beginEditBlock();
        setDotCommand("%1o", count());
        enterInsertMode();
        moveToFirstNonBlankOnLine();
        if (input.is('O'))
            moveToStartOfLine();
        else
            moveBehindEndOfLine();
        m_tc.insertText("\n");
        if (input.is('O'))
            moveUp();
        insertAutomaticIndentation(input.is('o'));
        endEditBlock();
    } else if (input.isControl('o')) {
        if (!m_jumpListUndo.isEmpty()) {
            m_jumpListRedo.append(cursorPosition());
            setCursorPosition(m_jumpListUndo.last());
            m_jumpListUndo.pop_back();
        }
    } else if (input.is('p') || input.is('P')) {
        pasteText(input.is('p'));
        setTargetColumn();
        setDotCommand("%1p", count());
        finishMovement();
    } else if (input.is('r')) {
        m_submode = TransformSubMode;
        m_subsubmode = ReplaceSubSubMode;
    } else if (!isVisualMode() && input.is('R')) {
        enterReplaceMode();
        updateMiniBuffer();
    } else if (input.isControl('r')) {
        redo();
    } else if (input.is('s')) {
        leaveVisualMode();
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
    } else if (input.is('S')) {
        if (!isVisualMode()) {
            const int line = cursorLineInDocument() + 1;
            setAnchor(firstPositionInLine(line));
            setPosition(lastPositionInLine(line + count() - 1));
        }
        setDotCommand("%1S", count());
        enterInsertMode();
        m_submode = ChangeSubMode;
        m_movetype = MoveLineWise;
        finishMovement();
    } else if (m_gflag && input.is('t')) {
        m_gflag = false;
        handleExCommand("tabnext");
    } else if (input.is('t')) {
        m_movetype = MoveInclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = input;
    } else if (m_gflag && input.is('T')) {
        m_gflag = false;
        handleExCommand("tabprev");
    } else if (input.is('T')) {
        m_movetype = MoveExclusive;
        m_subsubmode = FtSubSubMode;
        m_subsubdata = input;
    } else if (input.isControl('t')) {
        handleExCommand("pop");
    } else if (!m_gflag && input.is('u')) {
        undo();
    } else if (input.isControl('u')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveUp(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLineInDocument(cursorLineInDocument() - sline);
        finishMovement();
    } else if (input.is('v')) {
        enterVisualMode(VisualCharMode);
    } else if (input.is('V')) {
        enterVisualMode(VisualLineMode);
    } else if (input.isControl('v')) {
        enterVisualMode(VisualBlockMode);
    } else if (input.is('w')) { // tested
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank - except if the cursor is on the last
        // character of a word: only the current word will be changed
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(false, true, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(false, m_submode == DeleteSubMode);
            m_movetype = MoveExclusive;
        }
        finishMovement("%1w", count());
    } else if (input.is('W')) {
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(true, true, true);
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(true, m_submode == DeleteSubMode);
            m_movetype = MoveExclusive;
        }
        finishMovement("%1W", count());
    } else if (input.isControl('w')) {
        m_submode = WindowSubMode;
    } else if (input.is('x') && isNoVisualMode()) { // = "dl"
        m_movetype = MoveExclusive;
        setAnchor();
        m_submode = DeleteSubMode;
        moveRight(qMin(count(), rightDist()));
        setDotCommand("%1x", count());
        finishMovement();
    } else if (input.is('X')) {
        if (leftDist() > 0) {
            setAnchor();
            moveLeft(qMin(count(), leftDist()));
            yankSelectedText();
            removeSelectedText();
        }
        finishMovement();
    } else if ((m_submode == YankSubMode && input.is('y'))
            || (input.is('Y') && isNoVisualMode()))  {
        setAnchor();
        if (count() > 1)
            moveDown(count()-1);
        m_rangemode = RangeLineMode;
        m_movetype = MoveLineWise;
        m_submode = YankSubMode;
        finishMovement();
    } else if (input.is('y') && isNoVisualMode()) {
        setAnchor();
        m_submode = YankSubMode;
    } else if (input.is('y') && isVisualCharMode()) {
        Range range(position(), anchor(), RangeCharMode);
        range.endPos++; // MoveInclusive
        yankText(range, m_register);
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if ((input.is('y') && isVisualLineMode())
            || (input.is('Y') && isVisualLineMode())
            || (input.is('Y') && isVisualCharMode())) {
        m_rangemode = RangeLineMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        moveToStartOfLine();
        leaveVisualMode();
        finishMovement();
    } else if ((input.is('y') || input.is('Y')) && isVisualBlockMode()) {
        m_rangemode = RangeBlockMode;
        yankSelectedText();
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if (input.is('z')) {
        m_submode = ZSubMode;
    } else if (input.is('Z')) {
        m_submode = CapitalZSubMode;
    } else if (!m_gflag && input.is('~') && !isVisualMode()) {
        if (!atEndOfLine()) {
            beginEditBlock();
            setAnchor();
            moveRight(qMin(count(), rightDist()));
            if (input.is('~')) {
                invertCaseSelectedText();
                setDotCommand("%1~", count());
            } else if (input.is('u')) {
                downCaseSelectedText();
                setDotCommand("%1gu", count());
            } else if (input.is('U')) {
                upCaseSelectedText();
                setDotCommand("%1gU", count());
            }
            endEditBlock();
        }
        finishMovement();
    } else if ((m_gflag && input.is('~') && !isVisualMode())
        || (m_gflag && input.is('u') && !isVisualMode())
        || (m_gflag && input.is('U') && !isVisualMode())) {
        m_gflag = false;
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        m_submode = TransformSubMode;
        if (input.is('~'))
            m_subsubmode = InvertCaseSubSubMode;
        if (input.is('u'))
            m_subsubmode = DownCaseSubSubMode;
        else if (input.is('U'))
            m_subsubmode = UpCaseSubSubMode;
    } else if ((input.is('~') && isVisualMode())
        || (m_gflag && input.is('u') && isVisualMode())
        || (m_gflag && input.is('U') && isVisualMode())) {
        m_gflag = false;
        if (isVisualLineMode())
            m_rangemode = RangeLineMode;
        else if (isVisualBlockMode())
            m_rangemode = RangeBlockMode;
        leaveVisualMode();
        m_submode = TransformSubMode;
        if (input.is('~'))
            m_subsubmode = InvertCaseSubSubMode;
        else if (input.is('u'))
            m_subsubmode = DownCaseSubSubMode;
        else if (input.is('U'))
            m_subsubmode = UpCaseSubSubMode;
        finishMovement();
    } else if (input.is(Key_PageDown) || input.isControl('f')) {
        moveDown(count() * (linesOnScreen() - 2) - cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument());
        handleStartOfLine();
        finishMovement();
    } else if (input.is(Key_PageUp) || input.isControl('b')) {
        moveUp(count() * (linesOnScreen() - 2) + cursorLineOnScreen());
        scrollToLineInDocument(cursorLineInDocument() + linesOnScreen() - 2);
        handleStartOfLine();
        finishMovement();
    } else if (input.is(Key_Delete)) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        removeSelectedText();
    } else if (input.is(Key_BracketLeft) || input.is(Key_BracketRight)) {

    } else if (input.isControl(Key_BracketRight)) {
        handleExCommand("tag");
    } else {
        //qDebug() << "IGNORED IN COMMAND MODE: " << key << text
        //    << " VISUAL: " << m_visualMode;

        // if a key which produces text was pressed, don't mark it as unhandled
        // - otherwise the text would be inserted while being in command mode
        if (input.text().isEmpty()) {
            handled = EventUnhandled;
        }
    }

    m_positionPastEnd = (m_visualTargetColumn == -1) && isVisualMode();

    return handled;
}

EventResult FakeVimHandler::Private::handleReplaceMode(const Input &input)
{
    if (input.isKey(Key_Escape) || input.isKey(27) || input.isControl('c') ||
         input.isControl(Key_BracketLeft)) {
        moveLeft(qMin(1, leftDist()));
        setTargetColumn();
        m_submode = NoSubMode;
        m_mode = CommandMode;
        finishMovement();
    } else {
        joinPreviousEditBlock();
        if (!atEndOfLine()) {
            setAnchor();
        //    m_tc.deleteChar();
            moveRight();
            m_lastDeletion += selectedText();
            removeSelectedText();
        }
        const QString text = input.text();
        m_lastInsertion += text;
        m_tc.insertText(text);
        //qDebug() << "REM/INS: " << m_lastDeletion << m_lastInsertion;
        endEditBlock();
        //   m_movetype = MoveExclusive;
        //   setDotCommand("%1r" + text, count());
    }
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleInsertMode(const Input &input)
{
    //const int key = input.key;
    //const QString &text = input.text;

    if (input.isKey(Key_Escape) || input.isKey(27) || input.isControl('c') ||
         input.isControl(Key_BracketLeft)) {
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
        g.dotCommand += m_lastInsertion;
        g.dotCommand += QChar(27);
        enterCommandMode();
        m_submode = NoSubMode;
    } else if (input.isKey(Key_Insert)) {
        if (m_mode == ReplaceMode)
            m_mode = InsertMode;
        else
            m_mode = ReplaceMode;
        updateCursor();
    } else if (input.isKey(Key_Left)) {
        moveLeft(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Down)) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveDown(count());
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Up)) {
        //removeAutomaticIndentation();
        m_submode = NoSubMode;
        moveUp(count());
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Right)) {
        moveRight(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Home)) {
        moveToStartOfLine();
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_End)) {
        if (count() > 1)
            moveDown(count() - 1);
        moveBehindEndOfLine();
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Return)) {
        m_submode = NoSubMode;
        m_tc.insertBlock();
        m_lastInsertion += "\n";
        insertAutomaticIndentation(true);
        setTargetColumn();
    } else if (input.isKey(Key_Backspace) || input.isControl('h')) {
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
                moveToStartOfLine();
                moveRight(newcol);
                m_lastInsertion.clear(); // FIXME
            } else {
                m_tc.deletePreviousChar();
                m_lastInsertion.chop(1);
            }
            setTargetColumn();
        }
        endEditBlock();
    } else if (input.isKey(Key_Delete)) {
        m_tc.deleteChar();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_PageDown) || input.isControl('f')) {
        removeAutomaticIndentation();
        moveDown(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (input.isKey(Key_PageUp) || input.isControl('b')) {
        removeAutomaticIndentation();
        moveUp(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Tab) && hasConfig(ConfigExpandTab)) {
        m_justAutoIndented = 0;
        const int ts = config(ConfigTabStop).toInt();
        const int col = physicalCursorColumnInDocument();
        QString str = QString(ts - col % ts, ' ');
        m_lastInsertion.append(str);
        m_tc.insertText(str);
        setTargetColumn();
    } else if (input.isControl('d')) {
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
    //} else if (key >= control('a') && key <= control('z')) {
    //    // ignore these
    } else if (!input.text().isEmpty()) {
        joinPreviousEditBlock();
        m_justAutoIndented = 0;
        const QString text = input.text();
        m_lastInsertion.append(text);
        m_tc.insertText(text);
        if (hasConfig(ConfigSmartIndent) && isElectricCharacter(text.at(0))) {
            const QString leftText = m_tc.block().text()
                   .left(m_tc.position() - 1 - m_tc.block().position());
            if (leftText.simplified().isEmpty()) {
                Range range(position(), position(), m_rangemode);
                indentText(range, text.at(0));
            }
        }

        if (!g.inReplay)
            emit q->completionRequested();
        setTargetColumn();
        endEditBlock();
    } else {
        return EventUnhandled;
    }
    updateMiniBuffer();
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleExMode(const Input &input)
{
    if (input.isKey(Key_Escape) || input.isControl('c')
            || input.isControl(Key_BracketLeft)) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
    } else if (input.isKey(Key_Backspace)) {
        if (m_commandBuffer.isEmpty()) {
            enterCommandMode();
        } else {
            m_commandBuffer.chop(1);
        }
        updateMiniBuffer();
    } else if (input.isKey(Key_Left)) {
        // FIXME:
        if (!m_commandBuffer.isEmpty())
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (input.isKey(Key_Return)) {
        if (!m_commandBuffer.isEmpty()) {
            g.commandHistory.takeLast();
            g.commandHistory.append(m_commandBuffer);
            handleExCommand(m_commandBuffer);
            if (m_textedit || m_plaintextedit)
                leaveVisualMode();
        }
    } else if (input.isKey(Key_Up) || input.isKey(Key_PageUp)) {
        if (g.commandHistoryIndex > 0) {
            --g.commandHistoryIndex;
            showBlackMessage(g.commandHistory.at(g.commandHistoryIndex));
        }
    } else if (input.isKey(Key_Down) || input.isKey(Key_PageDown)) {
        if (g.commandHistoryIndex < g.commandHistory.size() - 1) {
            ++g.commandHistoryIndex;
            showBlackMessage(g.commandHistory.at(g.commandHistoryIndex));
        }
    } else if (input.isKey(Key_Tab)) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else if (!input.text().isEmpty()) {
        m_commandBuffer += input.text();
        updateMiniBuffer();
    } else {
        qDebug() << "IGNORED IN EX-MODE: " << input.key() << input.text();
        return EventUnhandled;
    }
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleSearchSubSubMode(const Input &input)
{
    if (input.isKey(Key_Escape) || input.isControl('c')
            || input.isControl(Key_BracketLeft)) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
    } else if (input.isKey(Key_Backspace)) {
        if (m_commandBuffer.isEmpty())
            enterCommandMode();
        else
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (input.isKey(Key_Left)) {
        if (!m_commandBuffer.isEmpty())
            m_commandBuffer.chop(1);
        updateMiniBuffer();
    } else if (input.isKey(Key_Return) && !hasConfig(ConfigIncSearch)) {
        QString needle = m_commandBuffer;
        if (!needle.isEmpty()) {
            g.searchHistory.takeLast();
            g.searchHistory.append(needle);
            search(lastSearchString(), m_lastSearchForward);
            recordJump();
        }
        finishMovement(m_commandPrefix + needle + "\n");
        enterCommandMode();
        highlightMatches(needle);
        updateMiniBuffer();
    } else if (input.isKey(Key_Up) || input.isKey(Key_PageUp)) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entries in the history into account.
        if (g.searchHistoryIndex > 0) {
            --g.searchHistoryIndex;
            showBlackMessage(g.searchHistory.at(g.searchHistoryIndex));
        }
    } else if (input.isKey(Key_Down) || input.isKey(Key_PageDown)) {
        if (g.searchHistoryIndex < g.searchHistory.size() - 1) {
            ++g.searchHistoryIndex;
            showBlackMessage(g.searchHistory.at(g.searchHistoryIndex));
        }
    } else if (input.isKey(Key_Tab)) {
        m_commandBuffer += QChar(9);
        updateMiniBuffer();
    } else if (input.isKey(Key_Return) && hasConfig(ConfigIncSearch)) {
        QString needle = m_commandBuffer;
        enterCommandMode();
        highlightMatches(needle);
        updateMiniBuffer();
    } else if (hasConfig(ConfigIncSearch)) {
        QString needle = m_commandBuffer + input.text();
        search(needle, m_lastSearchForward, true);
        updateMiniBuffer();
        recordJump();
    } else if (!input.text().isEmpty()) {
        m_commandBuffer += input.text();
        updateMiniBuffer();
    } else {
        qDebug() << "IGNORED IN SEARCH MODE: " << input.key() << input.text();
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

QString FakeVimHandler::Private::extractCommand(const QString &line,
    int *beginLine, int *endLine)
{
    QString cmd = line;
    *beginLine = -1;
    *endLine = -1;

    // FIXME: that seems to be different for %w and %s
    if (cmd.startsWith(QLatin1Char('%')))
        cmd = "1,$" + cmd.mid(1);

    int lineNumber = readLineCode(cmd);
    if (lineNumber != -1)
        *beginLine = lineNumber;

    if (cmd.startsWith(',')) {
        cmd = cmd.mid(1);
        lineNumber = readLineCode(cmd);
        if (lineNumber != -1)
            *endLine = lineNumber;
    }
    //qDebug() << "RANGE: " << beginLine << endLine << cmd << lineNumber << m_marks;
    return cmd;
}

bool FakeVimHandler::Private::handleExSubstituteCommand(const QString &line)
    // :substitute
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    if (cmd.startsWith(QLatin1String("substitute")))
        cmd = cmd.mid(10);
    else if (cmd.startsWith('s') && line.size() > 1
            && !isalpha(cmd.at(1).unicode()))
        cmd = cmd.mid(1);
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

    QString needle = cmd.mid(1, pos1 - 1);
    const QString replacement = cmd.mid(pos1 + 1, pos2 - pos1 - 1);
    QString flags = cmd.mid(pos2 + 1);

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
    return true;
}

bool FakeVimHandler::Private::handleExMapCommand(const QString &line) // :map
{
    int pos1 = line.indexOf(QLatin1Char(' '));
    if (pos1 == -1)
        return false;
    int pos2 = line.indexOf(QLatin1Char(' '), pos1 + 1);
    if (pos2 == -1)
        return false;

    QByteArray modes;
    enum Type { Map, Noremap, Unmap } type;

    QByteArray cmd = line.left(pos1).toLatin1();

    // Strange formatting. But everything else is even uglier.
    if (cmd == "map") { modes = "nvo"; type = Map; } else
    if (cmd == "nm" || cmd == "nmap") { modes = "n"; type = Map; } else
    if (cmd == "vm" || cmd == "vmap") { modes = "v"; type = Map; } else
    if (cmd == "xm" || cmd == "xmap") { modes = "x"; type = Map; } else
    if (cmd == "smap") { modes = "s"; type = Map; } else
    if (cmd == "map!") { modes = "ic"; type = Map; } else
    if (cmd == "im" || cmd == "imap") { modes = "i"; type = Map; } else
    if (cmd == "lm" || cmd == "lmap") { modes = "l"; type = Map; } else
    if (cmd == "cm" || cmd == "cmap") { modes = "c"; type = Map; } else

    if (cmd == "no" || cmd == "noremap") { modes = "nvo"; type = Noremap; } else
    if (cmd == "nn" || cmd == "nnoremap") { modes = "n"; type = Noremap; } else
    if (cmd == "vn" || cmd == "vnoremap") { modes = "v"; type = Noremap; } else
    if (cmd == "xn" || cmd == "xnoremap") { modes = "x"; type = Noremap; } else
    if (cmd == "snor" || cmd == "snoremap") { modes = "s"; type = Noremap; } else
    if (cmd == "ono" || cmd == "onoremap") { modes = "o"; type = Noremap; } else
    if (cmd == "no!" || cmd == "noremap!") { modes = "ic"; type = Noremap; } else
    if (cmd == "ino" || cmd == "inoremap") { modes = "i"; type = Noremap; } else
    if (cmd == "ln" || cmd == "lnoremap") { modes = "l"; type = Noremap; } else
    if (cmd == "cno" || cmd == "cnoremap") { modes = "c"; type = Noremap; } else

    if (cmd == "unm" || cmd == "unmap") { modes = "nvo"; type = Unmap; } else
    if (cmd == "nun" || cmd == "nunmap") { modes = "n"; type = Unmap; } else
    if (cmd == "vu" || cmd == "vunmap") { modes = "v"; type = Unmap; } else
    if (cmd == "xu" || cmd == "xunmap") { modes = "x"; type = Unmap; } else
    if (cmd == "sunm" || cmd == "sunmap") { modes = "s"; type = Unmap; } else
    if (cmd == "ou" || cmd == "ounmap") { modes = "o"; type = Unmap; } else
    if (cmd == "unm!" || cmd == "unmap!") { modes = "ic"; type = Unmap; } else
    if (cmd == "iu" || cmd == "iunmap") { modes = "i"; type = Unmap; } else
    if (cmd == "lu" || cmd == "lunmap") { modes = "l"; type = Unmap; } else
    if (cmd == "cu" || cmd == "cunmap") { modes = "c"; type = Unmap; }

    else
        return false;

    QString lhs = line.mid(pos1 + 1, pos2 - pos1 - 1);
    QString rhs = line.mid(pos2 + 1);
    Inputs key;
    foreach (QChar c, lhs)
        key.append(Input(c));
    //qDebug() << "MAPPING: " << modes << lhs << rhs;
    switch (type) {
        case Unmap:
            foreach (char c, modes)
                if (g.mappings.contains(c))
                    g.mappings[c].remove(key);
            break;
        case Map:
            rhs = rhs; // FIXME: expand rhs.
            // Fall through.
        case Noremap: {
            Inputs inputs;
            foreach (QChar c, rhs)
                inputs.append(Input(c));
            foreach (char c, modes)
                g.mappings[c].insert(key, inputs);
            break;
        }
    }
    return true;
}

bool FakeVimHandler::Private::handleExHistoryCommand(const QString &cmd) // :history
{
    static QRegExp reHistory("^his(tory)?( (.*))?$");
    if (reHistory.indexIn(cmd) == -1)
        return false;

    QString arg = reHistory.cap(3);
    if (arg.isEmpty()) {
        QString info;
        info += "#  command history\n";
        int i = 0;
        foreach (const QString &item, g.commandHistory) {
            ++i;
            info += QString("%1 %2\n").arg(i, -8).arg(item);
        }
        emit q->extraInformationChanged(info);
    } else {
        notImplementedYet();
    }
    updateMiniBuffer();
    return true;
}

bool FakeVimHandler::Private::handleExSetCommand(const QString &cmd) // :set
{
    static QRegExp reSet("^set?( (.*))?$");
    if (reSet.indexIn(cmd) == -1)
        return false;

    showBlackMessage(QString());
    QString arg = reSet.cap(2).trimmed();
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
    return true;
}

bool FakeVimHandler::Private::handleExNormalCommand(const QString &cmd) // :normal
{
    static QRegExp reNormal("^norm(al)?( (.*))?$");
    if (reNormal.indexIn(cmd) == -1)
        return false;
    //qDebug() << "REPLAY NORMAL: " << quoteUnprintable(reNormal.cap(3));
    replay(reNormal.cap(3), 1);
    return true;
}

bool FakeVimHandler::Private::handleExDeleteCommand(const QString &line) // :d
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    static QRegExp reDelete("^d( (.*))?$");
    if (reDelete.indexIn(cmd) != -1)
        return false;

    selectRange(beginLine, endLine);
    QString reg = reDelete.cap(2);
    QString text = selectedText();
    removeSelectedText();
    if (!reg.isEmpty()) {
        Register &r = g.registers[reg.at(0).unicode()];
        r.contents = text;
        r.rangemode = RangeLineMode;
    }
    return true;
}

bool FakeVimHandler::Private::handleExWriteCommand(const QString &line)
    // :w, :x, :q, :wq, ...
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    static QRegExp reWrite("^[wx]q?a?!?( (.*))?$");
    if (reWrite.indexIn(cmd) == -1) // :w and :x
        return false;

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
    return true;
}

bool FakeVimHandler::Private::handleExReadCommand(const QString &line) // :r
{
    if (!line.startsWith(QLatin1String("r ")))
        return false;

    beginEditBlock();
    moveToStartOfLine();
    setTargetColumn();
    moveDown();
    m_currentFileName = line.mid(2);
    QFile file(m_currentFileName);
    file.open(QIODevice::ReadOnly);
    QTextStream ts(&file);
    QString data = ts.readAll();
    m_tc.insertText(data);
    endEditBlock();
    showBlackMessage(FakeVimHandler::tr("\"%1\" %2L, %3C")
        .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));
    return true;
}

bool FakeVimHandler::Private::handleExBangCommand(const QString &line) // :!
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    if (!cmd.startsWith(QLatin1Char('!')))
        return false;

    selectRange(beginLine, endLine);
    int targetPosition = firstPositionInLine(beginLine);
    QString command = cmd.mid(1).trimmed();
    QString text = selectedText();
    QProcess proc;
    proc.start(command);
    proc.waitForStarted();
#ifdef Q_OS_WIN
    text.replace(QLatin1String("\n"), QLatin1String("\r\n"));
#endif
    proc.write(text.toUtf8());
    proc.closeWriteChannel();
    proc.waitForFinished();
    QString result = QString::fromUtf8(proc.readAllStandardOutput());
    beginEditBlock(targetPosition);
    removeSelectedText();
    m_tc.insertText(result);
    setPosition(targetPosition);
    endEditBlock();
    leaveVisualMode();
    //qDebug() << "FILTER: " << command;
    showBlackMessage(FakeVimHandler::tr("%n lines filtered", 0,
        text.count('\n')));
    return true;
}

bool FakeVimHandler::Private::handleExShiftRightCommand(const QString &line) // :>
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    if (!cmd.startsWith(QLatin1Char('>')))
        return false;

    m_anchor = firstPositionInLine(beginLine);
    setPosition(firstPositionInLine(endLine));
    shiftRegionRight(1);
    leaveVisualMode();
    showBlackMessage(FakeVimHandler::tr("%n lines >ed %1 time", 0,
        (endLine - beginLine + 1)).arg(1));
    return true;
}

bool FakeVimHandler::Private::handleExRedoCommand(const QString &line) // :redo
{
    if (line != "red" && line != "redo")
        return false;

    redo();
    updateMiniBuffer();
    return true;
}

bool FakeVimHandler::Private::handleExGotoCommand(const QString &line) // :<nr>
{
    int beginLine, endLine;
    QString cmd = extractCommand(line, &beginLine, &endLine);

    if (!cmd.isEmpty())
        return false;

    setPosition(firstPositionInLine(beginLine));
    showBlackMessage(QString());
    return true;
}

bool FakeVimHandler::Private::handleExSourceCommand(const QString &line) // :source
{
    int pos = line.indexOf(' ');
    if (line.leftRef(pos) != "so" && line.leftRef(pos) != "source")
        return false;

    QString fileName = line.mid(pos + 1);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showRedMessage(FakeVimHandler::tr("Can't open file %1").arg(fileName));
        return true;
    }

    bool inFunction = false;
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        line = line.trimmed();
        if (line.startsWith("function")) {
            //qDebug() << "IGNORING FUNCTION" << line;
            inFunction = true;
        } else if (inFunction && line.startsWith("endfunction")) {
            inFunction = false;
        } else if (line.startsWith("function")) {
            //qDebug() << "IGNORING FUNCTION" << line;
            inFunction = true;
        } else if (line.startsWith('"')) {
            // A comment.
        } else if (!line.isEmpty() && !inFunction) {
            //qDebug() << "EXECUTING: " << line;
            handleExCommandHelper(QString::fromUtf8(line));
        }
    }
    file.close();
    return true;
}

void FakeVimHandler::Private::handleExCommand(const QString &line0)
{
    QString line = line0; // Make sure we have a copy to prevent aliasing.
    enterCommandMode();
    showBlackMessage(QString());
    if (handleExCommandHelper(line))
        return;
    int beginLine, endLine;
    passUnknownExCommand(extractCommand(line, &beginLine, &endLine));
}

bool FakeVimHandler::Private::handleExCommandHelper(const QString &line)
{
    return handleExGotoCommand(line)
        || handleExBangCommand(line)
        || handleExHistoryCommand(line)
        || handleExMapCommand(line)
        || handleExNormalCommand(line)
        || handleExReadCommand(line)
        || handleExRedoCommand(line)
        || handleExSetCommand(line)
        || handleExShiftRightCommand(line)
        || handleExSourceCommand(line)
        || handleExSubstituteCommand(line)
        || handleExWriteCommand(line);
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

void FakeVimHandler::Private::search(const QString &needle0, bool forward,
    bool incSearch)
{
    showBlackMessage(QLatin1Char(forward ? '/' : '?') + needle0);
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
        if (!incSearch)
            highlightMatches(needle);
    } else {
        m_tc.setPosition(forward ? 0 : lastPositionInDocument());
        EDITOR(setTextCursor(m_tc));
        if (EDITOR(find(needle, flags))) {
            m_tc = EDITOR(textCursor());
            m_tc.setPosition(m_tc.anchor());
            if (oldLine != cursorLineInDocument() - cursorLineOnScreen())
                scrollToLineInDocument(cursorLineInDocument() - linesOnScreen() / 2);
            if (!incSearch) {
                if (forward)
                    showRedMessage(FakeVimHandler::tr("search hit BOTTOM, continuing at TOP"));
                else
                    showRedMessage(FakeVimHandler::tr("search hit TOP, continuing at BOTTOM"));
                highlightMatches(needle);
            }
        } else {
            if (!incSearch)
                highlightMatches(QString());
            setCursorPosition(origPosition);
            if (!incSearch)
                showRedMessage(FakeVimHandler::tr("Pattern not found: ") + needle);
        }
    }
    if (incSearch) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_tc;
        sel.format = m_tc.blockCharFormat();
        sel.format.setForeground(Qt::white);
        sel.format.setBackground(Qt::black);
        sel.cursor.setPosition(m_tc.position(), MoveAnchor);
        sel.cursor.setPosition(m_tc.position() + needle.size(), KeepAnchor);
        QList<QTextEdit::ExtraSelection> selections = m_searchSelections;
        selections.append(sel);
        emit q->selectionChanged(selections);
    }
    setTargetColumn();
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

    Range range(anchor(), position(), m_rangemode);
    indentText(range, typedChar);

    setPosition(firstPositionInLine(beginLine));
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
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    int targetPos = anchor();
    if (beginLine > endLine) {
        qSwap(beginLine, endLine);
        targetPos = position();
    }
    if (hasConfig(ConfigStartOfLine))
        targetPos = firstPositionInLine(beginLine);

    int len = config(ConfigShiftWidth).toInt() * repeat;
    QString indent(len, ' ');

    beginEditBlock(targetPos);
    for (int line = beginLine; line <= endLine; ++line) {
        setPosition(firstPositionInLine(line));
        m_tc.insertText(indent);
    }
    endEditBlock();

    setPosition(targetPos);
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1>>", endLine - beginLine + 1);
}

void FakeVimHandler::Private::shiftRegionLeft(int repeat)
{
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    int targetPos = anchor();
    if (beginLine > endLine) {
        qSwap(beginLine, endLine);
        targetPos = position();
    }
    const int shift = config(ConfigShiftWidth).toInt() * repeat;
    const int tab = config(ConfigTabStop).toInt();
    if (hasConfig(ConfigStartOfLine))
        targetPos = firstPositionInLine(beginLine);

    beginEditBlock(targetPos);
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

    setPosition(targetPos);
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


int FakeVimHandler::Private::charClass(QChar c, bool simple) const
{
    if (simple)
        return c.isSpace() ? 0 : 1;
    // FIXME: This means that only characters < 256 in the
    // ConfigIsKeyword setting are handled properly.
    if (c.unicode() < 256) {
        //int old = (c.isLetterOrNumber() || c.unicode() == '_') ? 2
        //    :  c.isSpace() ? 0 : 1;
        //qDebug() << c.unicode() << old << m_charClass[c.unicode()];
        return m_charClass[c.unicode()];
    }
    if (c.isLetterOrNumber() || c.unicode() == '_')
        return 2;
    return c.isSpace() ? 0 : 1;
}

// Helper to parse a-z,A-Z,48-57,_
static int someInt(const QString &str)
{
    if (str.toInt())
        return str.toInt();
    if (str.size())
        return str.at(0).unicode();
    return 0;
}

void FakeVimHandler::Private::setupCharClass()
{
    for (int i = 0; i < 256; ++i) {
        const QChar c = QChar(QLatin1Char(i));
        m_charClass[i] = c.isSpace() ? 0 : 1;
    }
    const QString conf = config(ConfigIsKeyword).toString();
    foreach (const QString &part, conf.split(QLatin1Char(','))) {
        if (part.contains(QLatin1Char('-'))) {
            const int from = someInt(part.section(QLatin1Char('-'), 0, 0));
            const int to = someInt(part.section(QLatin1Char('-'), 1, 1));
            for (int i = qMax(0, from); i <= qMin(255, to); ++i)
                m_charClass[i] = 2;
        } else {
            m_charClass[qMin(255, someInt(part))] = 2;
        }
    }
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

bool FakeVimHandler::Private::handleFfTt(QString key)
{
    int key0 = key.size() == 1 ? key.at(0).unicode() : 0;
    int oldPos = position();
    // m_subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = m_subsubdata.is('f') || m_subsubdata.is('t');
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
        if (uc == key0)
            --repeat;
        if (repeat == 0) {
            if (m_subsubdata.is('t'))
                --pos;
            else if (m_subsubdata.is('T'))
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
     return g.searchHistory.empty() ? QString() : g.searchHistory.back();
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
    Range range(anchor(), position(), m_rangemode);
    yankText(range, m_register);
}

void FakeVimHandler::Private::yankText(const Range &range, int toregister)
{
    Register &reg = g.registers[toregister];
    reg.contents = text(range);
    reg.rangemode = range.rangemode;
    //qDebug() << "YANKED: " << reg.contents;
}

void FakeVimHandler::Private::transformText(const Range &range,
    Transformation transformFunc, const QVariant &extra)
{
    QTextCursor tc = m_tc;
    switch (range.rangemode) {
        case RangeCharMode: {
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            TransformationData td(tc.selectedText(), extra);
            (this->*transformFunc)(&td);
            tc.removeSelectedText();
            fixMarks(range.beginPos, td.to.size() - td.from.size());
            tc.insertText(td.to);
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
            TransformationData td(tc.selectedText(), extra);
            (this->*transformFunc)(&td);
            tc.removeSelectedText();
            fixMarks(range.beginPos, td.to.size() - td.from.size());
            tc.insertText(td.to);
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
                TransformationData td(tc.selectedText(), extra);
                (this->*transformFunc)(&td);
                tc.removeSelectedText();
                fixMarks(block.position() + bCol, td.to.size() - td.from.size());
                tc.insertText(td.to);
                block = block.previous();
            }
            endEditBlock();
        }
    }
}

void FakeVimHandler::Private::removeSelectedText()
{
    Range range(anchor(), position(), m_rangemode);
    removeText(range);
}

void FakeVimHandler::Private::removeText(const Range &range)
{
    transformText(range, &FakeVimHandler::Private::removeTransform);
}

void FakeVimHandler::Private::removeTransform(TransformationData *td)
{
    Q_UNUSED(td);
}

void FakeVimHandler::Private::downCaseSelectedText()
{
    Range range(anchor(), position(), m_rangemode);
    transformText(range, &FakeVimHandler::Private::downCaseTransform);
}

void FakeVimHandler::Private::downCaseTransform(TransformationData *td)
{
    td->to = td->from.toLower();
}

void FakeVimHandler::Private::upCaseSelectedText()
{
    Range range(anchor(), position(), m_rangemode);
    transformText(range, &FakeVimHandler::Private::upCaseTransform);
}

void FakeVimHandler::Private::upCaseTransform(TransformationData *td)
{
    td->to = td->from.toUpper();
}

void FakeVimHandler::Private::invertCaseSelectedText()
{
    Range range(anchor(), position(), m_rangemode);
    transformText(range, &FakeVimHandler::Private::invertCaseTransform, QVariant());
}

void FakeVimHandler::Private::invertCaseTransform(TransformationData *td)
{
    foreach (QChar c, td->from)
        td->to += c.isUpper() ? c.toLower() : c.toUpper();
}

void FakeVimHandler::Private::replaceSelectedText()
{
    Range range(anchor(), position(), m_rangemode);
    transformText(range, &FakeVimHandler::Private::replaceTransform, QVariant());
}

void FakeVimHandler::Private::replaceTransform(TransformationData *td)
{
    for (int i = td->from.size(); --i >= 0; ) {
        QChar c = td->from.at(i);
        if (c.unicode() == '\n' || c.unicode() == '\0')
            td->to += QLatin1Char('\n');
        else
            td->to += m_replacement;
    }
}

void FakeVimHandler::Private::pasteText(bool afterCursor)
{
    const QString text = g.registers[m_register].contents;
    const QStringList lines = text.split(QChar('\n'));
    switch (g.registers[m_register].rangemode) {
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

void FakeVimHandler::Private::updateCursor()
{
    if (m_mode == ExMode) {
        EDITOR(setCursorWidth(0));
        EDITOR(setOverwriteMode(false));
    } else if (m_mode == InsertMode) {
        EDITOR(setCursorWidth(m_cursorWidth));
        EDITOR(setOverwriteMode(false));
    } else {
        EDITOR(setCursorWidth(m_cursorWidth));
        EDITOR(setOverwriteMode(true));
    }
}

void FakeVimHandler::Private::enterReplaceMode()
{
    m_mode = ReplaceMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_lastInsertion.clear();
    m_lastDeletion.clear();
    setUndoPosition(position());
    breakEditBlock();
    updateCursor();
}

void FakeVimHandler::Private::enterInsertMode()
{
    m_mode = InsertMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_lastInsertion.clear();
    setUndoPosition(position());
    breakEditBlock();
    updateCursor();
}

void FakeVimHandler::Private::enterCommandMode()
{
    if (atEndOfLine())
        moveLeft();
    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    updateCursor();
}

void FakeVimHandler::Private::enterExMode()
{
    m_mode = ExMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    updateCursor();
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
    //qDebug() << "REPLAY: " << quoteUnprintable(command);
    g.inReplay = true;
    for (int i = n; --i >= 0; ) {
        foreach (QChar c, command) {
            //qDebug() << "  REPLAY: " << QString(c);
            handleKey(Input(c));
        }
    }
    g.inReplay = false;
}

void FakeVimHandler::Private::selectWordTextObject(bool inner)
{
    Q_UNUSED(inner); // FIXME
    m_movetype = MoveExclusive;
    moveToWordBoundary(false, false, true);
    setAnchor();
    // FIXME: Rework the 'anchor' concept.
    if (isVisualMode())
        m_marks['<'] = m_tc.position();
    moveToWordBoundary(false, true, true);
    m_movetype = MoveInclusive;
}

void FakeVimHandler::Private::selectWORDTextObject(bool inner)
{
    Q_UNUSED(inner); // FIXME
    m_movetype = MoveExclusive;
    moveToWordBoundary(true, false, true);
    setAnchor();
    // FIXME: Rework the 'anchor' concept.
    if (isVisualMode())
        m_marks['<'] = m_tc.position();
    moveToWordBoundary(true, true, true);
    m_movetype = MoveInclusive;
}

void FakeVimHandler::Private::selectSentenceTextObject(bool inner)
{
    Q_UNUSED(inner);
}

void FakeVimHandler::Private::selectParagraphTextObject(bool inner)
{
    Q_UNUSED(inner);
}

void FakeVimHandler::Private::selectBlockTextObject(bool inner, char left, char right)
{
    Q_UNUSED(inner);
    Q_UNUSED(left);
    Q_UNUSED(right);
}

void FakeVimHandler::Private::selectQuotedStringTextObject(bool inner, int type)
{
    Q_UNUSED(inner);
    Q_UNUSED(type);
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

    if (active && ev->type() == QEvent::InputMethod && ob == d->editor()) {
        // This handles simple dead keys. The sequence of events is
        // KeyRelease-InputMethod-KeyRelease  for dead keys instead of
        // KeyPress-KeyRelease as for simple keys. As vi acts on key presses,
        // we have to act on the InputMethod event.
        // FIXME: A first approximation working for e.g. ^ on a German keyboard
        QInputMethodEvent *imev = static_cast<QInputMethodEvent *>(ev);
        KEY_DEBUG("INPUTMETHOD" << imev->commitString() << imev->preeditString());
        QString commitString = imev->commitString();
        int key = commitString.size() == 1 ? commitString.at(0).unicode() : 0;
        QKeyEvent kev(QEvent::KeyPress, key, Qt::KeyboardModifiers(), commitString);
        EventResult res = d->handleEvent(&kev);
        return res == EventHandled;
    }

    if (active && ev->type() == QEvent::KeyPress && ob == d->editor()) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
        KEY_DEBUG("KEYPRESS" << kev->key() << kev->text() << QChar(kev->key()));
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

void FakeVimHandler::restoreWidget(int tabSize)
{
    d->restoreWidget(tabSize);
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

#include "fakevimhandler.moc"
