/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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
//   A current "region of interest"
//   spans between anchor(), (i.e. the character below anchor()), and
//   position(). The character below position() is not included
//   if the last movement command was exclusive (MoveExclusive).
//

#include "fakevimhandler.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QRegExp>
#include <QTextStream>
#include <QTimer>
#include <QtAlgorithms>
#include <QStack>

#include <QApplication>
#include <QClipboard>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextEdit>

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
#   define UNDO_DEBUG(s) qDebug() << << document()->availableUndoSteps() << s
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

enum {
#ifdef Q_OS_MAC
    RealControlModifier = Qt::MetaModifier
#else
    RealControlModifier = Qt::ControlModifier
#endif
};
// Enforce use of RealControlModifier by breaking the compilation.
#define MetaModifier     // Use RealControlModifier instead
#define ControlModifier  // Use RealControlModifier instead

const int ParagraphSeparator = 0x00002029;
typedef QLatin1String _;

using namespace Qt;

/*! A \e Mode represents one of the basic modes of operation of FakeVim.
*/

enum Mode
{
    InsertMode,
    ReplaceMode,
    CommandMode,
    ExMode
};

/*! A \e SubMode is used for things that require one more data item
    and are 'nested' behind a \l Mode.
*/
enum SubMode
{
    NoSubMode,
    ChangeSubMode,       // Used for c
    DeleteSubMode,       // Used for d
    FilterSubMode,       // Used for !
    IndentSubMode,       // Used for =
    RegisterSubMode,     // Used for "
    ShiftLeftSubMode,    // Used for <
    ShiftRightSubMode,   // Used for >
    TransformSubMode,    // Used for ~/gu/gU
    WindowSubMode,       // Used for Ctrl-w
    YankSubMode,         // Used for y
    ZSubMode,            // Used for z
    CapitalZSubMode,     // Used for Z
    ReplaceSubMode,      // Used for r
    OpenSquareSubMode,   // Used for [
    CloseSquareSubMode   // Used for ]
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
    TextObjectSubSubMode, // Used for thing like iw, aW, as etc.
    SearchSubSubMode
};

enum VisualMode
{
    NoVisualMode,
    VisualCharMode,
    VisualLineMode,
    VisualBlockMode
};

enum MoveType
{
    MoveExclusive,
    MoveInclusive,
    MoveLineWise
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

QDebug operator<<(QDebug ts, const Column &col)
{
    return ts << "(p: " << col.physical << ", l: " << col.logical << ")";
}

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
    Register(const QString &c) : contents(c), rangemode(RangeCharMode) {}
    Register(const QString &c, RangeMode m) : contents(c), rangemode(m) {}
    QString contents;
    RangeMode rangemode;
};

QDebug operator<<(QDebug ts, const Register &reg)
{
    return ts << reg.contents;
}

struct SearchData
{
    SearchData()
    {
        forward = true;
        mustMove = true;
        highlightMatches = true;
        highlightCursor = true;
    }

    QString needle;
    bool forward;
    bool mustMove;
    bool highlightMatches;
    bool highlightCursor;
};

static QRegExp vimPatternToQtPattern(QString needle, bool smartcase)
{
    /* Trasformations (Vim regexp -> QRegExp):
     *   \a -> [A-Za-z]
     *   \A -> [^A-Za-z]
     *   \h -> [A-Za-z_]
     *   \H -> [^A-Za-z_]
     *   \l -> [a-z]
     *   \L -> [^a-z]
     *   \o -> [0-7]
     *   \O -> [^0-7]
     *   \u -> [A-Z]
     *   \U -> [^A-Z]
     *   \x -> [0-9A-Fa-f]
     *   \X -> [^0-9A-Fa-f]
     *
     *   \< -> \b
     *   \> -> \b
     *   [] -> \[\]
     *   \= -> ?
     *
     *   (...)  <-> \(...\)
     *   {...}  <-> \{...\}
     *   |      <-> \|
     *   ?      <-> \?
     *   +      <-> \+
     *   \{...} -> {...}
     *
     *   \c - set ignorecase for rest
     *   \C - set noignorecase for rest
     */
    bool ignorecase = smartcase && !needle.contains(QRegExp("[A-Z]"));
    QString pattern;
    pattern.reserve(2 * needle.size());

    bool escape = false;
    bool brace = false;
    bool curly = false;
    foreach (const QChar &c, needle) {
        if (brace) {
            brace = false;
            if (c == ']') {
                pattern.append(_("\\[\\]"));
                continue;
            } else {
                pattern.append('[');
            }
        }
        if (QString("(){}+|?").indexOf(c) != -1) {
            if (c == '{') {
                curly = escape;
            } else if (c == '}' && curly) {
                curly = false;
                escape = true;
            }

            if (escape)
                escape = false;
            else
                pattern.append('\\');
            pattern.append(c);
        } else if (escape) {
            // escape expression
            escape = false;
            if (c == '<' || c == '>')
                pattern.append(_("\\b"));
            else if (c == 'a')
                pattern.append(_("[a-zA-Z]"));
            else if (c == 'A')
                pattern.append(_("[^a-zA-Z]"));
            else if (c == 'h')
                pattern.append(_("[A-Za-z_]"));
            else if (c == 'H')
                pattern.append(_("[^A-Za-z_]"));
            else if (c == 'c' || c == 'C')
                ignorecase = (c == 'c');
            else if (c == 'l')
                pattern.append(_("[a-z]"));
            else if (c == 'L')
                pattern.append(_("[^a-z]"));
            else if (c == 'o')
                pattern.append(_("[0-7]"));
            else if (c == 'O')
                pattern.append(_("[^0-7]"));
            else if (c == 'u')
                pattern.append(_("[A-Z]"));
            else if (c == 'U')
                pattern.append(_("[^A-Z]"));
            else if (c == 'x')
                pattern.append(_("[0-9A-Fa-f]"));
            else if (c == 'X')
                pattern.append(_("[^0-9A-Fa-f]"));
            else if (c == '=')
                pattern.append(_("?"));
            else
                pattern.append('\\' + c);
        } else {
            // unescaped expression
            if (c == '\\')
                escape = true;
            else if (c == '[')
                brace = true;
            else if (c.isLetter() && ignorecase)
                pattern.append('[' + c.toLower() + c.toUpper() + ']');
            else
                pattern.append(c);
        }
    }
    if (escape)
        pattern.append('\\');
    else if (brace)
        pattern.append('[');

    return QRegExp(pattern);
}


Range::Range()
    : beginPos(-1), endPos(-1), rangemode(RangeCharMode)
{}

Range::Range(int b, int e, RangeMode m)
    : beginPos(qMin(b, e)), endPos(qMax(b, e)), rangemode(m)
{}

QString Range::toString() const
{
    return QString("%1-%2 (mode: %3)").arg(beginPos).arg(endPos)
        .arg(rangemode);
}

QDebug operator<<(QDebug ts, const Range &range)
{
    return ts << '[' << range.beginPos << ',' << range.endPos << ']';
}


ExCommand::ExCommand(const QString &c, const QString &a, const Range &r)
    : cmd(c), hasBang(false), args(a), range(r), count(1)
{}

bool ExCommand::matches(const QString &min, const QString &full) const
{
    return cmd.startsWith(min) && full.startsWith(cmd);
}

void ExCommand::setContentsFromLine(const QString &line)
{
    cmd = line.section(' ', 0, 0);
    args = line.mid(cmd.size() + 1).trimmed();
    while (cmd.startsWith(QLatin1Char(':')))
        cmd.remove(0, 1);
    hasBang = cmd.endsWith('!');
    if (hasBang)
        cmd.chop(1);
}

QDebug operator<<(QDebug ts, const ExCommand &cmd)
{
    return ts << cmd.cmd << ' ' << cmd.args << ' ' << cmd.range;
}

QDebug operator<<(QDebug ts, const QList<QTextEdit::ExtraSelection> &sels)
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
            res += _("<CR>");
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

inline QString msgMarkNotSet(const QString &text)
{
    return FakeVimHandler::tr("Mark '%1' not set").arg(text);
}

class Input
{
public:
    // Remove some extra "information" on Mac.
    static int cleanModifier(int m)  { return m & ~Qt::KeypadModifier; }

    Input()
        : m_key(0), m_xkey(0), m_modifiers(0) {}

    explicit Input(QChar x)
        : m_key(x.unicode()), m_xkey(x.unicode()), m_modifiers(0), m_text(x) {}

    Input(int k, int m, const QString &t)
        : m_key(k), m_modifiers(cleanModifier(m)), m_text(t)
    {
        // On Mac, QKeyEvent::text() returns non-empty strings for
        // cursor keys. This breaks some of the logic later on
        // relying on text() being empty for "special" keys.
        // FIXME: Check the real conditions.
        if (m_text.size() == 1 && m_text.at(0).unicode() < ' ')
            m_text.clear();
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

    bool isBackspace() const
    {
        return m_key == Key_Backspace || isControl('h');
    }

    bool isReturn() const
    {
        return m_key == Key_Return || m_key == Key_Enter;
    }

    bool isEscape() const
    {
        return isKey(Key_Escape) || isKey(27) || isControl('c')
            || isControl(Key_BracketLeft);
    }

    bool is(int c) const
    {
        return m_xkey == c && m_modifiers != RealControlModifier;
    }

    bool isControl(int c) const
    {
        return m_modifiers == RealControlModifier
            && (m_xkey == c || m_xkey + 32 == c || m_xkey + 64 == c || m_xkey + 96 == c);
    }

    bool isShift(int c) const
    {
        return m_modifiers == Qt::ShiftModifier && m_xkey == c;
    }

    bool operator==(const Input &a) const
    {
        return a.m_key == m_key && a.m_modifiers == m_modifiers
            && m_text == a.m_text;
    }

    // Ignore e.g. ShiftModifier, which is not available in sourced data.
    bool matchesForMap(const Input &a) const
    {
        return (a.m_key == m_key || a.m_xkey == m_xkey) && m_text == a.m_text;
    }

    bool operator!=(const Input &a) const { return !operator==(a); }

    QString text() const { return m_text; }

    QChar asChar() const
    {
        return (m_text.size() == 1 ? m_text.at(0) : QChar());
    }

    int key() const { return m_key; }

    QChar raw() const
    {
        if (m_key == Key_Tab)
            return '\t';
        if (m_key == Key_Return)
            return '\n';
        return m_key;
    }

    QDebug dump(QDebug ts) const
    {
        return ts << m_key << '-' << m_modifiers << '-'
            << quoteUnprintable(m_text);
    }
private:
    int m_key;
    int m_xkey;
    int m_modifiers;
    QString m_text;
};

QDebug operator<<(QDebug ts, const Input &input) { return input.dump(ts); }

class Inputs : public QVector<Input>
{
public:
    Inputs() {}
    explicit Inputs(const QString &str) { parseFrom(str); }
    void parseFrom(const QString &str);
};

static bool iss(char a, char b)
{
    if (a >= 'a')
        a -= 'a' - 'A';
    if (b >= 'a')
        b -= 'a' - 'A';
    return a == b;
}

void Inputs::parseFrom(const QString &str)
{
    const int n = str.size();
    for (int i = 0; i < n; ++i) {
        uint c0 = str.at(i).unicode(), c1 = 0, c2 = 0, c3 = 0, c4 = 0;
        if (i + 1 < n)
            c1 = str.at(i + 1).unicode();
        if (i + 2 < n)
            c2 = str.at(i + 2).unicode();
        if (i + 3 < n)
            c3 = str.at(i + 3).unicode();
        if (i + 4 < n)
            c4 = str.at(i + 4).unicode();
        if (c0 == '<') {
            if (iss(c1, 'C') && c2 == '-' && c4 == '>') {
                uint c = (c3 < 90 ? c3 : c3 - 32);
                append(Input(c, RealControlModifier, QString(QChar(c - 64))));
                i += 4;
            } else if (iss(c1, 'C') && iss(c2, 'R') && c3 == '>') {
                append(Input(Key_Return, Qt::NoModifier, QString(QChar(13))));
                i += 3;
            } else if (iss(c1, 'E') && iss(c2, 'S') && iss(c3, 'C') && c4 == '>') {
                append(Input(Key_Escape, Qt::NoModifier, QString(QChar(27))));
                i += 4;
            } else {
                append(Input(QLatin1Char(c0)));
            }
        } else {
            append(Input(QLatin1Char(c0)));
        }
    }
}

// This wraps a string and a "cursor position".
class CommandBuffer
{
public:
    CommandBuffer() : m_pos(0) {}

    void clear() { m_buffer.clear(); m_pos = 0; }
    void setContents(const QString &s) { m_buffer = s; m_pos = s.size(); }
    QString contents() const { return m_buffer; }
    bool isEmpty() const { return m_buffer.isEmpty(); }
    int cursorPos() const { return m_pos; }

    void insertChar(QChar c) { m_buffer.insert(m_pos++, c); }
    void insertText(const QString &s) { m_buffer.insert(m_pos, s); m_pos += s.size(); }
    void deleteChar() { if (m_pos) m_buffer.remove(--m_pos, 1); }

    void moveLeft() { if (m_pos) --m_pos; }
    void moveRight() { if (m_pos < m_buffer.size()) ++m_pos; }
    void moveStart() { m_pos = 0; }
    void moveEnd() { m_pos = m_buffer.size(); }

    QString display() const
    {
        QString msg;
        for (int i = 0; i != m_buffer.size(); ++i) {
            const QChar c = m_buffer.at(i);
            if (c.unicode() < 32) {
                msg += '^';
                msg += QChar(c.unicode() + 64);
            } else {
                msg += c;
            }
        }
        return msg;
    }

    bool handleInput(const Input &input)
    {
        if (input.isKey(Key_Left)) {
            moveLeft();
        } else if (input.isKey(Key_Right)) {
            moveRight();
        } else if (input.isKey(Key_Home)) {
            moveStart();
        } else if (input.isKey(Key_End)) {
            moveEnd();
        } else if (input.isKey(Key_Delete)) {
            if (m_pos < m_buffer.size())
                m_buffer.remove(m_pos, 1);
        } else if (!input.text().isEmpty()) {
            insertText(input.text());
        } else {
            return false;
        }
        return true;
    }

private:
    QString m_buffer;
    int m_pos;
};


class History
{
public:
    History() : m_index(0) {}
    void append(const QString &item);
    void down() { m_index = qMin(m_index + 1, m_items.size()); }
    void up() { m_index = qMax(m_index - 1, 0); }
    void restart() { m_index = m_items.size(); }
    QString current() const { return m_items.value(m_index, QString()); }
    QStringList items() const { return m_items; }

private:
    QStringList m_items;
    int m_index;
};

void History::append(const QString &item)
{
    if (item.isEmpty())
        return;
    m_items.removeAll(item);
    m_items.append(item); m_index = m_items.size() - 1;
}

// Mappings for a specific mode.
class ModeMapping : public QList<QPair<Inputs, Inputs> >
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

    // Returns 'false' if more input is needed to decide whether a mapping
    // needs to be applied. If a decision can be made, return 'true',
    // and replace *input with the mapped data.
    bool mappingDone(Inputs *inputs) const
    {
        // FIXME: inefficient.
        for (int i = 0; i != size(); ++i) {
            const Inputs &haystack = at(i).first;
            // A mapping
            if (couldTriggerMap(haystack, *inputs)) {
                if (haystack.size() != inputs->size())
                    return false; // This can be extended.
                // Actual mapping.
                *inputs = at(i).second;
                return true;
            }
        }
        // No extensible mapping found. Use inputs as-is.
        return true;
    }

private:
    static bool couldTriggerMap(const Inputs &haystack, const Inputs &needle)
    {
        // Input is already too long.
        if (needle.size() > haystack.size())
            return false;
        for (int i = 0; i != needle.size(); ++i) {
            if (!needle.at(i).matchesForMap(haystack.at(i)))
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
    EventResult handleCommandMode1(const Input &);
    EventResult handleCommandMode2(const Input &);
    EventResult handleRegisterMode(const Input &);
    EventResult handleExMode(const Input &);
    EventResult handleOpenSquareSubMode(const Input &);
    EventResult handleCloseSquareSubMode(const Input &);
    EventResult handleSearchSubSubMode(const Input &);
    EventResult handleCommandSubSubMode(const Input &);
    void finishMovement(const QString &dotCommand = QString());
    void finishMovement(const QString &dotCommand, int count);
    void resetCommandMode();
    void search(const SearchData &sd);
    void searchBalanced(bool forward, QChar needle, QChar other);
    void highlightMatches(const QString &needle);
    void stopIncrementalFind();

    int mvCount() const { return m_mvcount.isEmpty() ? 1 : m_mvcount.toInt(); }
    int opCount() const { return m_opcount.isEmpty() ? 1 : m_opcount.toInt(); }
    int count() const { return mvCount() * opCount(); }
    QTextBlock block() const { return cursor().block(); }
    int leftDist() const { return position() - block().position(); }
    int rightDist() const { return block().length() - leftDist() - 1; }
    bool atBlockEnd() const { return cursor().atBlockEnd(); }
    bool atEndOfLine() const { return atBlockEnd() && block().length() > 1; }

    int lastPositionInDocument() const; // Returns last valid position in doc.
    int firstPositionInLine(int line) const; // 1 based line, 0 based pos
    int lastPositionInLine(int line) const; // 1 based line, 0 based pos
    int lineForPosition(int pos) const;  // 1 based line, 0 based pos
    QString lineContents(int line) const; // 1 based line
    void setLineContents(int line, const QString &contents); // 1 based line
    int blockBoundary(const QString &left, const QString &right,
        bool end, int count) const; // end or start position of current code block

    int linesOnScreen() const;
    int columnsOnScreen() const;
    int linesInDocument() const;

    // The following use all zero-based counting.
    int cursorLineOnScreen() const;
    int cursorLine() const;
    int physicalCursorColumn() const; // as stored in the data
    int logicalCursorColumn() const; // as visible on screen
    int physicalToLogicalColumn(int physical, const QString &text) const;
    int logicalToPhysicalColumn(int logical, const QString &text) const;
    Column cursorColumn() const; // as visible on screen
    int firstVisibleLine() const;
    void scrollToLine(int line);
    void scrollUp(int count);
    void scrollDown(int count) { scrollUp(-count); }

    CursorPosition cursorPosition() const
        { return CursorPosition(position(), firstVisibleLine()); }
    void setCursorPosition(const CursorPosition &p)
        { setPosition(p.position); scrollToLine(p.scrollLine); }

    // Helper functions for indenting/
    bool isElectricCharacter(QChar c) const;
    void indentSelectedText(QChar lastTyped = QChar());
    void indentText(const Range &range, QChar lastTyped = QChar());
    void shiftRegionLeft(int repeat = 1);
    void shiftRegionRight(int repeat = 1);

    void moveToFirstNonBlankOnLine();
    void moveToTargetColumn();
    void setTargetColumn() {
        m_targetColumn = logicalCursorColumn();
        m_visualTargetColumn = m_targetColumn;
        //qDebug() << "TARGET: " << m_targetColumn;
    }
    void moveToNextWord(bool simple, bool deleteWord = false);
    void moveToMatchingParanthesis();
    void moveToWordBoundary(bool simple, bool forward, bool changeWord = false);

    // Convenience wrappers to reduce line noise.
    void moveToStartOfLine();
    void moveToEndOfLine();
    void moveBehindEndOfLine();
    void moveUp(int n = 1) { moveDown(-n); }
    void moveDown(int n = 1);
    void dump(const char *msg) const {
        qDebug() << msg << "POS: " << anchor() << position()
            << "EXT: " << m_oldExternalAnchor << m_oldExternalPosition
            << "INT: " << m_oldInternalAnchor << m_oldInternalPosition
            << "VISUAL: " << m_visualMode;
    }
    void moveRight(int n = 1) {
        //dump("RIGHT 1");
        QTextCursor tc = cursor();
        tc.movePosition(Right, KeepAnchor, n);
        setCursor(tc);
        //dump("RIGHT 2");
    }
    void moveLeft(int n = 1) {
        QTextCursor tc = cursor();
        tc.movePosition(Left, KeepAnchor, n);
        setCursor(tc);
    }
    void setAnchor() {
        QTextCursor tc = cursor();
        tc.setPosition(tc.position(), MoveAnchor);
        setCursor(tc);
    }
    void setAnchor(int position) {
        QTextCursor tc = cursor();
        tc.setPosition(tc.anchor(), MoveAnchor);
        tc.setPosition(position, KeepAnchor);
        setCursor(tc);
    }
    void setPosition(int position) {
        QTextCursor tc = cursor();
        tc.setPosition(position, KeepAnchor);
        setCursor(tc);
    }
    void setAnchorAndPosition(int anchor, int position) {
        QTextCursor tc = cursor();
        tc.setPosition(anchor, MoveAnchor);
        tc.setPosition(position, KeepAnchor);
        setCursor(tc);
    }
    QTextCursor cursor() const { return EDITOR(textCursor()); }
    void setCursor(const QTextCursor &tc) { EDITOR(setTextCursor(tc)); }

    bool handleFfTt(QString key);

    // Helper function for handleExCommand returning 1 based line index.
    int readLineCode(QString &cmd);

    void enterInsertMode();
    void enterReplaceMode();
    void enterCommandMode();
    void enterExMode();
    void showRedMessage(const QString &msg);
    void showBlackMessage(const QString &msg);
    void notImplementedYet();
    void updateMiniBuffer();
    void updateSelection();
    void updateCursorShape();
    QWidget *editor() const;
    QTextDocument *document() const { return EDITOR(document()); }
    QChar characterAtCursor() const
        { return document()->characterAt(position()); }
    void beginEditBlock()
        { UNDO_DEBUG("BEGIN EDIT BLOCK"); cursor().beginEditBlock(); }
    void endEditBlock()
        { UNDO_DEBUG("END EDIT BLOCK"); cursor().endEditBlock(); }
    void joinPreviousEditBlock()
        { UNDO_DEBUG("JOIN"); cursor().joinPreviousEditBlock(); }
    void breakEditBlock() {
        QTextCursor tc = cursor();
        tc.clearSelection();
        tc.beginEditBlock();
        tc.insertText("x");
        tc.deletePreviousChar();
        tc.endEditBlock();
        setCursor(tc);
    }

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
    void changeNumberTextObject(bool doIncrement);
    void selectQuotedStringTextObject(bool inner, const QString &quote);

    Q_SLOT void importSelection();
    void exportSelection();
    void insertInInsertMode(const QString &text);

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    bool m_wasReadOnly; // saves read-only state of document

    FakeVimHandler *q;
    Mode m_mode;
    bool m_passing; // let the core see the next event
    bool m_firstKeyPending;
    SubMode m_submode;
    SubSubMode m_subsubmode;
    Input m_subsubdata;
    int m_oldExternalPosition; // copy from last event to check for external changes
    int m_oldExternalAnchor;
    int m_oldInternalPosition; // copy from last event to check for external changes
    int m_oldInternalAnchor;
    int m_oldPosition; // FIXME: Merge with above.
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
    CommandBuffer m_commandBuffer;
    QString m_currentFileName;
    QString m_currentMessage;

    bool m_lastSearchForward;
    bool m_findPending;
    int m_findStartPosition;
    QString m_lastInsertion;
    QString m_lastDeletion;

    int anchor() const { return cursor().anchor(); }
    int position() const { return cursor().position(); }

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

    void insertText(const Register &reg);
    void removeText(const Range &range);
    void removeTransform(TransformationData *td);

    void invertCase(const Range &range);
    void invertCaseTransform(TransformationData *td);

    void upCase(const Range &range);
    void upCaseTransform(TransformationData *td);

    void downCase(const Range &range);
    void downCaseTransform(TransformationData *td);

    void replaceText(const Range &range, const QString &str);
    void replaceByStringTransform(TransformationData *td);
    void replaceByCharTransform(TransformationData *td);

    QString selectText(const Range &range) const;
    void setCurrentRange(const Range &range);
    Range currentRange() const { return Range(position(), anchor(), m_rangemode); }
    Range rangeFromCurrentLine() const;

    void yankText(const Range &range, int toregister = '"');

    void pasteText(bool afterCursor);

    // undo handling
    void undo();
    void redo();
    void setUndoPosition();
    QMap<int, int> m_undoCursorPosition; // revision -> position

    // extra data for '.'
    void replay(const QString &text, int count);
    void setDotCommand(const QString &cmd) { g.dotCommand = cmd; }
    void setDotCommand(const QString &cmd, int n) { g.dotCommand = cmd.arg(n); }

    // extra data for ';'
    QString m_semicolonCount;
    Input m_semicolonType;  // 'f', 'F', 't', 'T'
    QString m_semicolonKey;

    // visual modes
    void toggleVisualMode(VisualMode visualMode);
    void leaveVisualMode();
    VisualMode m_visualMode;
    VisualMode m_oldVisualMode;

    // marks as lines
    int mark(int code) const;
    void setMark(int code, int position);
    typedef QHash<int, QTextCursor> Marks;
    typedef QHashIterator<int, QTextCursor> MarksIterator;
    Marks m_marks;

    // vi style configuration
    QVariant config(int code) const { return theFakeVimSetting(code)->value(); }
    bool hasConfig(int code) const { return config(code).toBool(); }
    bool hasConfig(int code, const char *value) const // FIXME
        { return config(code).toString().contains(value); }

    int m_targetColumn; // -1 if past end of line
    int m_visualTargetColumn; // 'l' can move past eol in visual mode only

    // auto-indent
    QString tabExpand(int len) const;
    Column indentation(const QString &line) const;
    void insertAutomaticIndentation(bool goingDown);
    bool removeAutomaticIndentation(); // true if something removed
    // number of autoindented characters
    int m_justAutoIndented;
    void handleStartOfLine();

    // register handling
    QString registerContents(int reg) const;
    void setRegisterContents(int reg, const QString &contents);
    RangeMode registerRangeMode(int reg) const;
    void setRegisterRangeMode(int reg, RangeMode mode);
    void getRegisterType(int reg, bool *isClipboard, bool *isSelection) const;

    void recordJump();
    QVector<CursorPosition> m_jumpListUndo;
    QVector<CursorPosition> m_jumpListRedo;
    int m_lastChangePosition;

    QList<QTextEdit::ExtraSelection> m_searchSelections;
    QTextCursor m_searchCursor;
    QString m_oldNeedle;
    QString m_lastSubstituteFlags;
    QRegExp m_lastSubstitutePattern;
    QString m_lastSubstituteReplacement;

    bool handleExCommandHelper(const ExCommand &cmd); // Returns success.
    bool handleExPluginCommand(const ExCommand &cmd); // Handled by plugin?
    bool handleExBangCommand(const ExCommand &cmd);
    bool handleExDeleteCommand(const ExCommand &cmd);
    bool handleExGotoCommand(const ExCommand &cmd);
    bool handleExHistoryCommand(const ExCommand &cmd);
    bool handleExRegisterCommand(const ExCommand &cmd);
    bool handleExMapCommand(const ExCommand &cmd);
    bool handleExNohlsearchCommand(const ExCommand &cmd);
    bool handleExNormalCommand(const ExCommand &cmd);
    bool handleExReadCommand(const ExCommand &cmd);
    bool handleExRedoCommand(const ExCommand &cmd);
    bool handleExSetCommand(const ExCommand &cmd);
    bool handleExShiftCommand(const ExCommand &cmd);
    bool handleExSourceCommand(const ExCommand &cmd);
    bool handleExSubstituteCommand(const ExCommand &cmd);
    bool handleExWriteCommand(const ExCommand &cmd);
    bool handleExEchoCommand(const ExCommand &cmd);

    void timerEvent(QTimerEvent *ev);

    void setupCharClass();
    int charClass(QChar c, bool simple) const;
    signed char m_charClass[256];
    bool m_ctrlVActive;

    static struct GlobalData
    {
        GlobalData()
        {
            inputTimer = -1;
        }

        // Input.
        Inputs pendingInput;
        int inputTimer;

        // Repetition.
        QString dotCommand;

        // History for searches.
        History searchHistory;

        // History for :ex commands.
        History commandHistory;

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
    //new Highlighter(document(), &pythonRules);
    init();
}

void FakeVimHandler::Private::init()
{
    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_passing = false;
    m_firstKeyPending = false;
    m_findPending = false;
    m_findStartPosition = -1;
    m_fakeEnd = false;
    m_positionPastEnd = false;
    m_anchorPastEnd = false;
    m_lastSearchForward = true;
    m_register = '"';
    m_gflag = false;
    m_visualMode = NoVisualMode;
    m_oldVisualMode = NoVisualMode;
    m_targetColumn = 0;
    m_visualTargetColumn = 0;
    m_movetype = MoveInclusive;
    m_justAutoIndented = 0;
    m_rangemode = RangeCharMode;
    m_ctrlVActive = false;
    m_oldInternalAnchor = -1;
    m_oldInternalPosition = -1;
    m_oldExternalAnchor = -1;
    m_oldExternalPosition = -1;
    m_oldPosition = -1;
    m_lastChangePosition = -1;

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
        // Not sure this feels good. People often hit Esc several times.
        if (isNoVisualMode()
                && m_mode == CommandMode
                && m_submode == NoSubMode
                && m_opcount.isEmpty()
                && m_mvcount.isEmpty())
            return false;
        return true;
    }

    // We are interested in overriding most Ctrl key combinations.
    if (mods == RealControlModifier
            && !config(ConfigPassControlKey).toBool()
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

    // Let other shortcuts trigger.
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

    bool inSnippetMode = false;
    QMetaObject::invokeMethod(editor(),
        "inSnippetMode", Q_ARG(bool *, &inSnippetMode));

    if (inSnippetMode)
        return EventPassedToCore;

    // Fake "End of line"
    //m_tc = cursor();

    //bool hasBlock = false;
    //emit q->requestHasBlockSelection(&hasBlock);
    //qDebug() << "IMPORT BLOCK 2:" << hasBlock;

    //if (0 && hasBlock) {
    //    (pos > anc) ? --pos : --anc;

    importSelection();

    // Position changed externally, e.g. by code completion.
    if (position() != m_oldPosition) {
        setTargetColumn();
        //qDebug() << "POSITION CHANGED EXTERNALLY";
        if (m_mode == InsertMode) {
            int dist = position() - m_oldPosition;
            // Try to compensate for code completion
            if (dist > 0 && dist <= physicalCursorColumn()) {
                Range range(m_oldPosition, position());
                m_lastInsertion.append(selectText(range));
            }
        } else if (!isVisualMode()) {
            if (atEndOfLine())
                moveLeft();
        }
    }

    QTextCursor tc = cursor();
    tc.setVisualNavigation(true);
    setCursor(tc);

    if (m_firstKeyPending) {
        m_firstKeyPending = false;
        recordJump();
    }

    if (m_fakeEnd)
        moveRight();

    //if ((mods & RealControlModifier) != 0) {
    //    if (key >= Key_A && key <= Key_Z)
    //        key = shift(key); // make it lower case
    //    key = control(key);
    //} else if (key >= Key_A && key <= Key_Z && (mods & Qt::ShiftModifier) == 0) {
    //    key = shift(key);
    //}

    //QTC_ASSERT(m_mode == InsertMode || m_mode == ReplaceMode
    //        || !atBlockEnd() || block().length() <= 1,
    //    qDebug() << "Cursor at EOL before key handler");

    EventResult result = handleKey(Input(key, mods, ev->text()));

    // The command might have destroyed the editor.
    if (m_textedit || m_plaintextedit) {
        // We fake vi-style end-of-line behaviour
        m_fakeEnd = atEndOfLine() && m_mode == CommandMode && !isVisualBlockMode();

        //QTC_ASSERT(m_mode == InsertMode || m_mode == ReplaceMode
        //        || !atBlockEnd() || block().length() <= 1,
        //    qDebug() << "Cursor at EOL after key handler");
        if (m_fakeEnd)
            moveLeft();

        m_oldPosition = position();
        if (hasConfig(ConfigShowMarks))
            updateSelection();

        exportSelection();
        updateCursorShape();
    }

    return result;
}

void FakeVimHandler::Private::installEventFilter()
{
    EDITOR(viewport()->installEventFilter(q));
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
    m_firstKeyPending = true;

    updateEditor();
    importSelection();
    updateMiniBuffer();
    updateCursorShape();
}

void FakeVimHandler::Private::exportSelection()
{
    int pos = position();
    int anc = anchor();
    m_oldInternalPosition = pos;
    m_oldInternalAnchor = anc;
    if (isVisualMode()) {
        if (pos >= anc)
            setAnchorAndPosition(anc, pos + 1);
        else
            setAnchorAndPosition(anc + 1, pos);

        if (m_visualMode == VisualBlockMode) {
            emit q->requestSetBlockSelection(false);
            emit q->requestSetBlockSelection(true);
        } else if (m_visualMode == VisualLineMode) {
            const int posLine = lineForPosition(pos);
            const int ancLine = lineForPosition(anc);
            if (anc < pos) {
                pos = lastPositionInLine(posLine);
                anc = firstPositionInLine(ancLine);
            } else {
                pos = firstPositionInLine(posLine);
                anc = lastPositionInLine(ancLine);
            }
            setAnchorAndPosition(anc, pos);
        } else if (m_visualMode == VisualCharMode) {
            /* Nothing */
        } else {
            QTC_CHECK(false);
        }
    } else {
        setAnchorAndPosition(pos, pos);
    }
    m_oldExternalPosition = position();
    m_oldExternalAnchor = anchor();
    m_oldVisualMode = m_visualMode;
}

void FakeVimHandler::Private::importSelection()
{
    bool hasBlock = false;
    emit q->requestHasBlockSelection(&hasBlock);

    if (position() == m_oldExternalPosition
            && anchor() == m_oldExternalAnchor) {
        // Undo drawing correction.
        m_visualMode = m_oldVisualMode;
        setAnchorAndPosition(m_oldInternalAnchor, m_oldInternalPosition);
        //setMark('<', m_oldInternalAnchor);
        //setMark('>', m_oldInternalPosition);
    } else {
        // Import new selection.
        Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
        if (cursor().hasSelection()) {
            if (mods & RealControlModifier)
                m_visualMode = VisualBlockMode;
            else if (mods & Qt::AltModifier)
                m_visualMode = VisualBlockMode;
            else if (mods & Qt::ShiftModifier)
                m_visualMode = VisualLineMode;
            else
                m_visualMode = VisualCharMode;
        } else {
            m_visualMode = NoVisualMode;
        }
        //dump("IS @");
        //setMark('<', tc.anchor());
        //setMark('>', tc.position());
    }
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
    m_visualMode = NoVisualMode;
    // Force "ordinary" cursor.
    m_mode = InsertMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    updateCursorShape();
    updateSelection();
}

EventResult FakeVimHandler::Private::handleKey(const Input &input)
{
    KEY_DEBUG("HANDLE INPUT: " << input << " MODE: " << mode);
    if (m_mode == ExMode)
        return handleExMode(input);
    if (m_subsubmode == SearchSubSubMode)
        return handleSearchSubSubMode(input);
    if (m_mode == InsertMode || m_mode == ReplaceMode || m_mode == CommandMode) {
        g.pendingInput.append(input);
        const char code = m_mode == InsertMode ? 'i' : 'n';
        if (g.mappings.value(code).mappingDone(&g.pendingInput))
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
    Inputs pendingInput = g.pendingInput;
    g.pendingInput.clear();
    if (m_mode == InsertMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, pendingInput) {
            EventResult r = handleInsertMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
        return result;
    }
    if (m_mode == ReplaceMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, pendingInput) {
            EventResult r = handleReplaceMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
        return result;
    }
    if (m_mode == CommandMode) {
        EventResult result = EventUnhandled;
        foreach (const Input &in, pendingInput) {
            EventResult r = handleCommandMode(in);
            if (r == EventHandled)
                result = EventHandled;
        }
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
        QTextCursor tc = cursor();
        setAnchorAndPosition(m_findStartPosition, tc.selectionStart());
        finishMovement();
        setAnchor();
    }
}

void FakeVimHandler::Private::setUndoPosition()
{
    int pos = qMin(position(), anchor());
    if (m_visualMode == VisualLineMode)
        pos = firstPositionInLine(lineForPosition(pos));
    const int rev = document()->availableUndoSteps();
    m_undoCursorPosition[rev] = pos;
}

void FakeVimHandler::Private::moveDown(int n)
{
#if 0
    // does not work for "hidden" documents like in the autotests
    tc.movePosition(Down, MoveAnchor, n);
#else
    const int col = position() - block().position();
    const int lastLine = document()->lastBlock().blockNumber();
    const int targetLine = qMax(0, qMin(lastLine, block().blockNumber() + n));
    const QTextBlock &block = document()->findBlockByNumber(targetLine);
    const int pos = block.position();
    setPosition(pos + qMax(0, qMin(block.length() - 2, col)));
    moveToTargetColumn();
#endif
}

void FakeVimHandler::Private::moveToEndOfLine()
{
#if 0
    // does not work for "hidden" documents like in the autotests
    tc.movePosition(EndOfLine, MoveAnchor);
#else
    const int pos = block().position() + block().length() - 2;
    setPosition(qMax(block().position(), pos));
#endif
}

void FakeVimHandler::Private::moveBehindEndOfLine()
{
    int pos = qMin(block().position() + block().length() - 1,
        lastPositionInDocument());
    setPosition(pos);
}

void FakeVimHandler::Private::moveToStartOfLine()
{
#if 0
    // does not work for "hidden" documents like in the autotests
    tc.movePosition(StartOfLine, MoveAnchor);
#else
    setPosition(block().position());
#endif
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand, int count)
{
    finishMovement(dotCommand.arg(count));
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommand)
{
    //dump("FINISH MOVEMENT");
    if (m_submode == FilterSubMode) {
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        setPosition(qMin(anchor(), position()));
        enterExMode();
        m_currentMessage.clear();
        m_commandBuffer.setContents(QString(".,+%1!").arg(qAbs(endLine - beginLine)));
        //g.commandHistory.append(QString());
        updateMiniBuffer();
        return;
    }

    //if (isVisualMode())
    //    setMark('>', position());

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
                if (!cursor().atBlockEnd())
                    setPosition(position() + 1); // correction
            } else {
                setAnchorAndPosition(anchor() + 1, position());
            }
        }

        if (m_positionPastEnd) {
            const int anc = anchor();
            moveBehindEndOfLine();
            moveRight();
            setAnchorAndPosition(anc, position());
        }

        if (m_anchorPastEnd) {
            setAnchorAndPosition(anchor() + 1, position());
        }

        if (m_submode != TransformSubMode) {
            yankText(currentRange(), m_register);
            if (m_movetype == MoveLineWise)
                setRegisterRangeMode(m_register, RangeLineMode);
        }

        m_positionPastEnd = m_anchorPastEnd = false;
    }

    if (m_submode == ChangeSubMode) {
        if (m_rangemode == RangeLineMode)
            m_rangemode = RangeLineModeExclusive;
        removeText(currentRange());
        if (!dotCommand.isEmpty())
            setDotCommand(QLatin1Char('c') + dotCommand);
        if (m_movetype == MoveLineWise)
            insertAutomaticIndentation(true);
        endEditBlock();
        enterInsertMode();
        m_submode = NoSubMode;
    } else if (m_submode == DeleteSubMode) {
        removeText(currentRange());
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
            setPosition(mark(m_register));
            moveToStartOfLine();
        } else {
            if (anchor() <= position())
                setPosition(anchor());
        }
        if (la != lp)
            showBlackMessage(QString("%1 lines yanked").arg(qAbs(la - lp) + 1));
    } else if (m_submode == TransformSubMode) {
        if (m_subsubmode == InvertCaseSubSubMode) {
            invertCase(currentRange());
            if (!dotCommand.isEmpty())
                setDotCommand(QLatin1Char('~') + dotCommand);
        } else if (m_subsubmode == UpCaseSubSubMode) {
            upCase(currentRange());
            if (!dotCommand.isEmpty())
                setDotCommand("gU" + dotCommand);
        } else if (m_subsubmode == DownCaseSubSubMode) {
            downCase(currentRange());
            if (!dotCommand.isEmpty())
                setDotCommand("gu" + dotCommand);
        }
        m_submode = NoSubMode;
        m_subsubmode = NoSubSubMode;
        setPosition(qMin(anchor(), position()));
        if (m_movetype == MoveLineWise)
            handleStartOfLine();
        endEditBlock();
    } else if (m_submode == IndentSubMode) {
        setUndoPosition();
        recordJump();
        indentSelectedText();
        m_submode = NoSubMode;
    } else if (m_submode == ShiftRightSubMode) {
        setUndoPosition();
        recordJump();
        shiftRegionRight(1);
        m_submode = NoSubMode;
    } else if (m_submode == ShiftLeftSubMode) {
        setUndoPosition();
        recordJump();
        shiftRegionLeft(1);
        m_submode = NoSubMode;
    }

    resetCommandMode();
    updateSelection();
    updateMiniBuffer();
}

void FakeVimHandler::Private::resetCommandMode()
{
    m_movetype = MoveInclusive;
    m_mvcount.clear();
    m_opcount.clear();
    m_gflag = false;
    m_register = '"';
    //m_tc.clearSelection();
    m_rangemode = RangeCharMode;
}

void FakeVimHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections = m_searchSelections;
    if (!m_searchCursor.isNull()) {
        QTextEdit::ExtraSelection sel;
        sel.cursor = m_searchCursor;
        sel.format = m_searchCursor.blockCharFormat();
        sel.format.setForeground(Qt::white);
        sel.format.setBackground(Qt::black);
        selections.append(sel);
    }
    if (hasConfig(ConfigShowMarks)) {
        for (MarksIterator it(m_marks); it.hasNext(); ) {
            it.next();
            QTextEdit::ExtraSelection sel;
            const int pos = it.value().position();
            sel.cursor = cursor();
            sel.cursor.setPosition(pos, MoveAnchor);
            sel.cursor.setPosition(pos + 1, KeepAnchor);
            sel.format = cursor().blockCharFormat();
            sel.format.setForeground(Qt::blue);
            sel.format.setBackground(Qt::green);
            selections.append(sel);
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
    int cursorPos = -1;
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
    } else if (!m_commandPrefix.isEmpty()) {
        //QTC_ASSERT(m_mode == ExMode || m_subsubmode == SearchSubSubMode,
        //    qDebug() << "MODE: " << m_mode << m_subsubmode);
        msg = m_commandPrefix + m_commandBuffer.display();
        if (m_mode != CommandMode)
            cursorPos = m_commandPrefix.size() + m_commandBuffer.cursorPos();
    } else {
        QTC_CHECK(m_mode == CommandMode && m_subsubmode != SearchSubSubMode);
        msg = "-- COMMAND --";
    }

    emit q->commandBufferChanged(msg, cursorPos);

    int linesInDoc = linesInDocument();
    int l = cursorLine();
    QString status;
    const QString pos = QString::fromLatin1("%1,%2")
        .arg(l + 1).arg(physicalCursorColumn() + 1);
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
    m_commandBuffer.setContents(msg);
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
        || submode == ChangeSubMode
        || submode == IndentSubMode
        || submode == ShiftLeftSubMode
        || submode == ShiftRightSubMode;
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
        else if (input.is('{') || input.is('}') || input.is('B'))
            selectBlockTextObject(m_subsubdata.is('i'), '{', '}');
        else if (input.is('"') || input.is('\'') || input.is('`'))
            selectQuotedStringTextObject(m_subsubdata.is('i'), input.asChar());
        m_subsubmode = NoSubSubMode;
        finishMovement(QString("%1%2%3")
                       .arg(count())
                       .arg(m_subsubdata.text())
                       .arg(input.text()));
    } else if (m_subsubmode == MarkSubSubMode) {
        setMark(input.asChar().unicode(), position());
        m_subsubmode = NoSubSubMode;
    } else if (m_subsubmode == BackTickSubSubMode
            || m_subsubmode == TickSubSubMode) {
        int m = mark(input.asChar().unicode());
        if (m != -1) {
            setPosition(m);
            if (m_subsubmode == TickSubSubMode)
                moveToFirstNonBlankOnLine();
            finishMovement();
        } else {
            showRedMessage(msgMarkNotSet(input.text()));
        }
        m_subsubmode = NoSubSubMode;
    } else {
        handled = EventUnhandled;
    }
    return handled;
}

EventResult FakeVimHandler::Private::handleOpenSquareSubMode(const Input &input)
{
    EventResult handled = EventHandled;
    m_submode = NoSubMode;
    if (input.is('{')) {
        searchBalanced(false, '{', '}');
    } else if (input.is('(')) {
        searchBalanced(false, '(', ')');
    } else {
        handled = EventUnhandled;
    }
    return handled;
}

EventResult FakeVimHandler::Private::handleCloseSquareSubMode(const Input &input)
{
    EventResult handled = EventHandled;
    m_submode = NoSubMode;
    if (input.is('}')) {
        searchBalanced(true, '}', '{');
    } else if (input.is(')')) {
        searchBalanced(true, ')', '(');
    } else {
        handled = EventUnhandled;
    }
    return handled;
}

EventResult FakeVimHandler::Private::handleCommandMode(const Input &input)
{
    EventResult handled = EventHandled;

    if (input.isEscape()) {
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
    } else if (m_submode == OpenSquareSubMode) {
        handled = handleOpenSquareSubMode(input);
    } else if (m_submode == CloseSquareSubMode) {
        handled = handleCloseSquareSubMode(input);
    } else if (m_submode == WindowSubMode) {
        emit q->windowCommandRequested(input.key());
        m_submode = NoSubMode;
    } else if (m_submode == RegisterSubMode) {
        m_register = input.asChar().unicode();
        m_submode = NoSubMode;
        m_rangemode = RangeLineMode;
    } else if (m_submode == ReplaceSubMode) {
        if (isVisualMode()) {
            setUndoPosition();
            m_lastChangePosition = position();
            if (isVisualLineMode())
                m_rangemode = RangeLineMode;
            else if (isVisualBlockMode())
                m_rangemode = RangeBlockMode;
            else
                m_rangemode = RangeCharMode;
            leaveVisualMode();
            Range range = currentRange();
            Transformation tr =
                &FakeVimHandler::Private::replaceByCharTransform;
            transformText(range, tr, input.asChar());
            setPosition(range.beginPos);
        } else if (count() <= rightDist()) {
            setUndoPosition();
            m_lastChangePosition = position();
            setAnchor();
            moveRight(count());
            Range range = currentRange();
            if (input.isReturn()) {
                beginEditBlock();
                replaceText(range, QString());
                insertText(QString("\n"));
                endEditBlock();
            } else {
                replaceText(range, QString(count(), input.asChar()));
                moveLeft();
            }
            setTargetColumn();
            setDotCommand("%1r" + input.text(), count());
        }
        m_submode = NoSubMode;
        finishMovement();
    } else if (m_submode == ChangeSubMode && input.is('c')) { // tested
        setUndoPosition();
        m_lastChangePosition = position();
        moveToStartOfLine();
        setAnchor();
        moveDown(count() - 1);
        moveToEndOfLine();
        m_movetype = MoveLineWise;
        m_lastInsertion.clear();
        setDotCommand("%1cc", count());
        finishMovement();
    } else if (m_submode == DeleteSubMode && input.is('d')) { // tested
        setUndoPosition();
        m_lastChangePosition = position();
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
        m_lastChangePosition = position();
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1<<", count());
        finishMovement();
    } else if (m_submode == ShiftRightSubMode && input.is('>')) {
        m_lastChangePosition = position();
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1>>", count());
        finishMovement();
    } else if (m_submode == IndentSubMode && input.is('=')) {
        m_lastChangePosition = position();
        setAnchor();
        moveDown(count() - 1);
        m_movetype = MoveLineWise;
        setDotCommand("%1==", count());
        finishMovement();
    } else if (m_submode == ZSubMode) {
        //qDebug() << "Z_MODE " << cursorLine() << linesOnScreen();
        if (input.isReturn() || input.is('t')) {
            // Cursor line to top of window.
            if (!m_mvcount.isEmpty())
                setPosition(firstPositionInLine(count()));
            scrollUp(- cursorLineOnScreen());
            if (input.isReturn())
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
    } else {
        handled = handleCommandMode1(input);
    }

    return handled;
}

EventResult FakeVimHandler::Private::handleCommandMode1(const Input &input)
{
    EventResult handled = EventHandled;

    if (input.is('^') || input.is('_')) {
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
    } else if (input.is('&')) {
        handleExCommand(m_gflag ? "%s//~/&" : "s");
    } else if (input.is(':')) {
        enterExMode();
        g.commandHistory.restart();
        m_currentMessage.clear();
        m_commandBuffer.clear();
        if (isVisualMode())
            m_commandBuffer.setContents("'<,'>");
        updateMiniBuffer();
    } else if (input.is('/') || input.is('?')) {
        m_lastSearchForward = input.is('/');
        g.searchHistory.restart();
        if (hasConfig(ConfigUseCoreSearch)) {
            // re-use the core dialog.
            m_findPending = true;
            m_findStartPosition = position();
            m_movetype = MoveExclusive;
            setAnchor(); // clear selection: otherwise, search is restricted to selection
            emit q->findRequested(!m_lastSearchForward);
        } else {
            // FIXME: make core find dialog sufficiently flexible to
            // produce the "default vi" behaviour too. For now, roll our own.
            m_currentMessage.clear();
            m_movetype = MoveExclusive;
            m_subsubmode = SearchSubSubMode;
            m_commandPrefix = QLatin1Char(m_lastSearchForward ? '/' : '?');
            m_commandBuffer.clear();
            updateMiniBuffer();
        }
    } else if (input.is('`')) {
        m_subsubmode = BackTickSubSubMode;
        if (m_submode != NoSubMode)
            m_movetype = MoveLineWise;
    } else if (input.is('#') || input.is('*')) {
        // FIXME: That's not proper vim behaviour
        QString needle;
        QTextCursor tc = cursor();
        tc.select(QTextCursor::WordUnderCursor);
        needle = "\\<" + tc.selection().toPlainText() + "\\>";
        setAnchorAndPosition(tc.position(), tc.anchor());
        g.searchHistory.append(needle);
        m_lastSearchForward = input.is('*');
        m_currentMessage.clear();
        m_commandPrefix = QLatin1Char(m_lastSearchForward ? '/' : '?');
        m_commandBuffer.setContents(needle);
        SearchData sd;
        sd.needle = needle;
        sd.forward = m_lastSearchForward;
        sd.highlightCursor = false;
        sd.highlightMatches = true;
        search(sd);
        //m_searchCursor = QTextCursor();
        //updateSelection();
        //updateMiniBuffer();
    } else if (input.is('\'')) {
        m_subsubmode = TickSubSubMode;
        if (m_submode != NoSubMode)
            m_movetype = MoveLineWise;
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
        m_commandBuffer.setContents("'<,'>!");
        //g.commandHistory.append(QString());
        updateMiniBuffer();
    } else if (input.is('"')) {
        m_submode = RegisterSubMode;
    } else if (input.isReturn()) {
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
    } else if (input.is('<')) {
        setUndoPosition();
        if (isNoVisualMode()) {
            m_submode = ShiftLeftSubMode;
        } else {
            shiftRegionLeft(count());
            leaveVisualMode();
        }
    } else if (input.is('>')) {
        setUndoPosition();
        if (isNoVisualMode()) {
            m_submode = ShiftRightSubMode;
        } else {
            shiftRegionRight(count());
            leaveVisualMode();
        }
    } else if (input.is('=')) {
        setUndoPosition();
        if (isNoVisualMode()) {
            m_submode = IndentSubMode;
        } else {
            indentSelectedText();
            leaveVisualMode();
        }
    } else if (input.is('%')) {
        moveToMatchingParanthesis();
        finishMovement();
    } else if ((!isVisualMode() && input.is('a')) || (isVisualMode() && input.is('A'))) {
        leaveVisualMode();
        setUndoPosition();
        breakEditBlock();
        enterInsertMode();
        m_lastInsertion.clear();
        if (!atEndOfLine())
            moveRight();
        updateMiniBuffer();
    } else if (input.is('A')) {
        setUndoPosition();
        breakEditBlock();
        moveBehindEndOfLine();
        setAnchor();
        enterInsertMode();
        setDotCommand(QString(QLatin1Char('A')));
        m_lastInsertion.clear();
        updateMiniBuffer();
    } else if (input.isControl('a')) {
        changeNumberTextObject(true);
    } else if (input.is('b') || input.isShift(Key_Left)) {
        m_movetype = MoveExclusive;
        moveToWordBoundary(false, false);
        setTargetColumn();
        finishMovement();
    } else if (input.is('B')) {
        m_movetype = MoveExclusive;
        moveToWordBoundary(true, false);
        setTargetColumn();
        finishMovement();
    } else if (input.is('c') && isNoVisualMode()) {
        setUndoPosition();
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
            int pos = position();
            moveToEndOfLine();
            setAnchor();
            setPosition(pos);
        } else {
            setAnchor();
        }
        m_opcount = m_mvcount;
        m_mvcount.clear();
        m_submode = DeleteSubMode;
    } else if ((input.is('d') || input.is('x') || input.isKey(Key_Delete))
            && isVisualMode()) {
        if (isVisualCharMode()) {
            leaveVisualMode();
            m_submode = DeleteSubMode;
            finishMovement();
        } else if (isVisualLineMode()) {
            leaveVisualMode();
            m_rangemode = RangeLineMode;
            yankText(currentRange(), m_register);
            removeText(currentRange());
            handleStartOfLine();
        } else if (isVisualBlockMode()) {
            leaveVisualMode();
            m_rangemode = RangeBlockMode;
            yankText(currentRange(), m_register);
            removeText(currentRange());
            setPosition(qMin(position(), anchor()));
        }
    } else if (input.is('D') && isNoVisualMode()) {
        if (atEndOfLine())
            moveLeft();
        m_submode = DeleteSubMode;
        setAnchor();
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
        yankText(currentRange(), m_register);
        removeText(currentRange());
        moveToFirstNonBlankOnLine();
    } else if ((input.is('D') || input.is('X')) && isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockAndTailMode;
        yankText(currentRange(), m_register);
        removeText(currentRange());
        setPosition(qMin(position(), anchor()));
    } else if (input.isControl('d')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLine(cursorLine() - sline);
        finishMovement();
    } else if (input.is('e') || input.isShift(Key_Right)) {
        m_movetype = MoveInclusive;
        moveToWordBoundary(false, true);
        setTargetColumn();
        finishMovement("%1e", count());
    } else if (input.is('E')) {
        m_movetype = MoveInclusive;
        moveToWordBoundary(true, true);
        setTargetColumn();
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
            setPosition(firstPositionInLine(n));
            handleStartOfLine();
        } else {
            m_movetype = MoveLineWise;
            m_rangemode = RangeLineMode;
            setAnchor();
            setPosition(firstPositionInLine(n));
        }
        finishMovement(dotCommand);
    } else if (input.is('h') || input.isKey(Key_Left) || input.isBackspace()) {
        m_movetype = MoveExclusive;
        int n = qMin(count(), leftDist());
        if (m_fakeEnd && block().length() > 1)
            ++n;
        moveLeft(n);
        setTargetColumn();
        finishMovement("%1h", count());
    } else if (input.is('H')) {
        setCursor(EDITOR(cursorForPosition(QPoint(0, 0))));
        moveDown(qMax(count() - 1, 0));
        handleStartOfLine();
        finishMovement();
    } else if (!isVisualMode() && (input.is('i') || input.isKey(Key_Insert))) {
        setDotCommand(QString(QLatin1Char('i'))); // setDotCommand("%1i", count());
        setUndoPosition();
        breakEditBlock();
        enterInsertMode();
        updateMiniBuffer();
        if (atEndOfLine())
            moveLeft();
    } else {
        handled = handleCommandMode2(input);
    }
    return handled;
}

EventResult FakeVimHandler::Private::handleCommandMode2(const Input &input)
{
    EventResult handled = EventHandled;

    if (input.is('I')) {
        setUndoPosition();
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
            //m_tc.clearSelection();
        }
        breakEditBlock();
        enterInsertMode();
    } else if (input.isControl('i')) {
        if (!m_jumpListRedo.isEmpty()) {
            m_jumpListUndo.append(cursorPosition());
            setCursorPosition(m_jumpListRedo.last());
            m_jumpListRedo.pop_back();
        }
    } else if (input.is('j') || input.isKey(Key_Down)
            || input.isControl('j') || input.isControl('n')) {
        m_movetype = MoveLineWise;
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
                    removeText(currentRange());
                } else {
                    while (characterAtCursor() == ' '
                        || characterAtCursor() == '\t')
                        moveRight();
                    removeText(currentRange());
                    cursor().insertText(QString(QLatin1Char(' ')));
                }
            }
            if (!m_gflag)
                moveLeft();
        }
        endEditBlock();
        finishMovement();
    } else if (input.is('k') || input.isKey(Key_Up) || input.isControl('p')) {
        m_movetype = MoveLineWise;
        moveUp(count());
        finishMovement("%1k", count());
    } else if (input.is('l') || input.isKey(Key_Right) || input.is(' ')) {
        m_movetype = MoveExclusive;
        bool pastEnd = count() >= rightDist() - 1;
        moveRight(qMax(0, qMin(count(), rightDist() - (m_submode == NoSubMode))));
        setTargetColumn();
        if (pastEnd && isVisualMode())
            m_visualTargetColumn = -1;
        finishMovement("%1l", count());
    } else if (input.is('L')) {
        QTextCursor tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()))));
        setCursor(tc);
        moveUp(qMax(count(), 1));
        handleStartOfLine();
        finishMovement();
    } else if (input.isControl('l')) {
        // screen redraw. should not be needed
    } else if (input.is('m')) {
        if (m_gflag) {
            moveToStartOfLine();
            moveRight(qMin(columnsOnScreen() / 2, rightDist()) - 1);
            setTargetColumn();
            finishMovement();
        } else {
            m_subsubmode = MarkSubSubMode;
        }
    } else if (input.is('M')) {
        QTextCursor tc = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        setCursor(tc);
        handleStartOfLine();
        finishMovement();
    } else if (input.is('n') || input.is('N')) {
        if (hasConfig(ConfigUseCoreSearch)) {
            bool forward = (input.is('n')) ? m_lastSearchForward : !m_lastSearchForward;
            int pos = position();
            emit q->findNextRequested(!forward);
            if (forward && pos == cursor().selectionStart()) {
                // if cursor is already positioned at the start of a find result, this is returned
                emit q->findNextRequested(false);
            }
            setPosition(cursor().selectionStart());
        } else {
            SearchData sd;
            sd.needle = g.searchHistory.current();
            sd.forward = input.is('n') ? m_lastSearchForward : !m_lastSearchForward;
            sd.highlightCursor = false;
            sd.highlightMatches = true;
            search(sd);
        }
    } else if (isVisualMode() && (input.is('o') || input.is('O'))) {
        int pos = position();
        setAnchorAndPosition(pos, anchor());
        std::swap(m_marks['<'], m_marks['>']);
        std::swap(m_positionPastEnd, m_anchorPastEnd);
        setTargetColumn();
        if (m_positionPastEnd)
            m_visualTargetColumn = -1;
        updateSelection();
    } else if (input.is('o')) {
        setDotCommand("%1o", count());
        setUndoPosition();
        breakEditBlock();
        enterInsertMode();
        beginEditBlock();
        moveToFirstNonBlankOnLine();
        moveBehindEndOfLine();
        insertText(QString("\n"));
        insertAutomaticIndentation(true);
        endEditBlock();
    } else if (input.is('O')) {
        setDotCommand("%1O", count());
        setUndoPosition();
        breakEditBlock();
        enterInsertMode();
        beginEditBlock();
        moveToFirstNonBlankOnLine();
        moveToStartOfLine();
        insertText(QString("\n"));
        moveUp();
        insertAutomaticIndentation(false);
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
        setUndoPosition();
        m_submode = ReplaceSubMode;
    } else if (!isVisualMode() && input.is('R')) {
        setUndoPosition();
        breakEditBlock();
        enterReplaceMode();
        updateMiniBuffer();
    } else if (input.isControl('r')) {
        redo();
    } else if (input.is('s') && isVisualBlockMode()) {
        setUndoPosition();
        Range range(position(), anchor(), RangeBlockMode);
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        m_visualInsertCount = qAbs(endLine - beginLine);
        setPosition(qMin(position(), anchor()));
        yankText(range, m_register);
        removeText(range);
        setDotCommand("%1s", count());
        breakEditBlock();
        enterInsertMode();
    } else if (input.is('s')) {
        setUndoPosition();
        leaveVisualMode();
        if (atEndOfLine())
            moveLeft();
        setAnchor();
        moveRight(qMin(count(), rightDist()));
        yankText(currentRange(), m_register);
        removeText(currentRange());
        setDotCommand("%1s", count());
        m_opcount.clear();
        m_mvcount.clear();
        breakEditBlock();
        enterInsertMode();
    } else if (input.is('S')) {
        setUndoPosition();
        beginEditBlock();
        if (!isVisualMode()) {
            const int line = cursorLine() + 1;
            const int anc = firstPositionInLine(line);
            const int pos = lastPositionInLine(line + count() - 1);
            setAnchorAndPosition(anc, pos);
        }
        setDotCommand("%1S", count());
        breakEditBlock();
        enterInsertMode();
        m_submode = ChangeSubMode;
        m_movetype = MoveLineWise;
        endEditBlock();
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
        scrollToLine(cursorLine() - sline);
        finishMovement();
    } else if (input.is('v')) {
        toggleVisualMode(VisualCharMode);
    } else if (input.is('V')) {
        toggleVisualMode(VisualLineMode);
    } else if (input.isControl('v')) {
        toggleVisualMode(VisualBlockMode);
    } else if (input.is('w')) { // tested
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank - except if the cursor is on the last
        // character of a word: only the current word will be changed
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(false, true, true);
            setTargetColumn();
            m_movetype = MoveInclusive;
        } else {
            moveToNextWord(false, m_submode == DeleteSubMode);
            m_movetype = MoveExclusive;
        }
        finishMovement("%1w", count());
    } else if (input.is('W')) {
        if (m_submode == ChangeSubMode) {
            moveToWordBoundary(true, true, true);
            setTargetColumn();
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
        m_submode = DeleteSubMode;
        const int n = qMin(count(), rightDist());
        setAnchorAndPosition(position(), position() + n);
        setDotCommand("%1x", count());
        finishMovement();
    } else if (input.isControl('x')) {
        changeNumberTextObject(false);
    } else if (input.is('X')) {
        if (leftDist() > 0) {
            setAnchor();
            moveLeft(qMin(count(), leftDist()));
            yankText(currentRange(), m_register);
            removeText(currentRange());
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
    } else if (input.isControl('y')) {
        // FIXME: this should use the "scroll" option, and "count"
        if (cursorLineOnScreen() == linesOnScreen() - 1)
            moveUp(1);
        scrollUp(1);
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
        yankText(currentRange(), m_register);
        setPosition(qMin(position(), anchor()));
        moveToStartOfLine();
        leaveVisualMode();
        finishMovement();
    } else if ((input.is('y') || input.is('Y')) && isVisualBlockMode()) {
        m_rangemode = RangeBlockMode;
        yankText(currentRange(), m_register);
        setPosition(qMin(position(), anchor()));
        leaveVisualMode();
        finishMovement();
    } else if (input.is('z')) {
        m_submode = ZSubMode;
    } else if (input.is('Z')) {
        m_submode = CapitalZSubMode;
    } else if (!m_gflag && input.is('~') && !isVisualMode()) {
        m_movetype = MoveExclusive;
        if (!atEndOfLine()) {
            beginEditBlock();
            setAnchor();
            moveRight(qMin(count(), rightDist()));
            if (input.is('~')) {
                invertCase(currentRange());
                setDotCommand("%1~", count());
            } else if (input.is('u')) {
                downCase(currentRange());
                setDotCommand("%1gu", count());
            } else if (input.is('U')) {
                upCase(currentRange());
                setDotCommand("%1gU", count());
            }
            endEditBlock();
        }
        finishMovement();
    } else if ((m_gflag && input.is('~') && !isVisualMode())
        || (m_gflag && input.is('u') && !isVisualMode())
        || (m_gflag && input.is('U') && !isVisualMode())) {
        m_gflag = false;
        m_movetype = MoveExclusive;
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
        m_movetype = MoveExclusive;
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
    } else if (input.is('[')) {
        m_submode = OpenSquareSubMode;
    } else if (input.is(']')) {
        m_submode = CloseSquareSubMode;
    } else if (input.isKey(Key_PageDown) || input.isControl('f')) {
        moveDown(count() * (linesOnScreen() - 2) - cursorLineOnScreen());
        scrollToLine(cursorLine());
        handleStartOfLine();
        finishMovement();
    } else if (input.isKey(Key_PageUp) || input.isControl('b')) {
        moveUp(count() * (linesOnScreen() - 2) + cursorLineOnScreen());
        scrollToLine(cursorLine() + linesOnScreen() - 2);
        handleStartOfLine();
        finishMovement();
    } else if (input.isKey(Key_Delete)) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        removeText(currentRange());
        if (atEndOfLine())
            moveLeft();
    } else if (input.isKey(Key_BracketLeft) || input.isKey(Key_BracketRight)) {

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
    if (input.isEscape()) {
        moveLeft(qMin(1, leftDist()));
        setTargetColumn();
        m_submode = NoSubMode;
        m_mode = CommandMode;
        finishMovement();
    } else if (input.isKey(Key_Left)) {
        breakEditBlock();
        moveLeft(1);
        setTargetColumn();
    } else if (input.isKey(Key_Right)) {
        breakEditBlock();
        moveRight(1);
        setTargetColumn();
    } else if (input.isKey(Key_Up)) {
        breakEditBlock();
        moveUp(1);
        setTargetColumn();
    } else if (input.isKey(Key_Down)) {
        breakEditBlock();
        moveDown(1);
    } else {
        joinPreviousEditBlock();
        if (!atEndOfLine()) {
            setAnchor();
            moveRight();
            m_lastDeletion += selectText(Range(position(), anchor()));
            removeText(currentRange());
        }
        const QString text = input.text();
        m_lastInsertion += text;
        setAnchor();
        insertText(text);
        endEditBlock();
        setTargetColumn();
    }
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleInsertMode(const Input &input)
{
    //const int key = input.key;
    //const QString &text = input.text;

    if (input.isEscape()) {
        if (isVisualBlockMode() && !m_lastInsertion.contains('\n')) {
            leaveVisualMode();
            joinPreviousEditBlock();
            moveLeft(m_lastInsertion.size());
            setAnchor();
            int pos = position();
            setTargetColumn();
            for (int i = 0; i < m_visualInsertCount; ++i) {
                moveDown();
                insertText(m_lastInsertion);
            }
            moveLeft(1);
            Range range(pos, position(), RangeBlockMode);
            yankText(range);
            setPosition(pos);
            setDotCommand("p");
            endEditBlock();
        } else {
            // Normal insertion. Start with '1', as one instance was
            // already physically inserted while typing.
            QString data;
            for (int i = 1; i < count(); ++i)
                data += m_lastInsertion;
            insertText(data);
            moveLeft(qMin(1, leftDist()));
            setTargetColumn();
            leaveVisualMode();
            breakEditBlock();
        }
        g.dotCommand += m_lastInsertion;
        g.dotCommand += QChar(27);
        enterCommandMode();
        m_submode = NoSubMode;
        m_ctrlVActive = false;
        m_opcount.clear();
        m_mvcount.clear();
    } else if (m_ctrlVActive) {
        insertInInsertMode(input.raw());
    } else if (input.isControl('v')) {
        m_ctrlVActive = true;
    } else if (input.isControl('w')) {
        int endPos = position();
        moveToWordBoundary(false, false, false);
        setTargetColumn();
        int beginPos = position();
        Range range(beginPos, endPos, RangeCharMode);
        removeText(range);
    } else if (input.isKey(Key_Insert)) {
        if (m_mode == ReplaceMode)
            m_mode = InsertMode;
        else
            m_mode = ReplaceMode;
    } else if (input.isKey(Key_Left)) {
        moveLeft(count());
        setTargetColumn();
        m_lastInsertion.clear();
    } else if (input.isControl(Key_Left)) {
        moveToWordBoundary(false, false);
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
    } else if (input.isControl(Key_Right)) {
        moveToWordBoundary(false, true);
        moveRight(); // we need one more move since we are in insert mode
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
    } else if (input.isReturn()) {
        joinPreviousEditBlock();
        m_submode = NoSubMode;
        insertText(QString("\n"));
        m_lastInsertion += '\n';
        insertAutomaticIndentation(true);
        setTargetColumn();
        endEditBlock();
    } else if (input.isBackspace()) {
        joinPreviousEditBlock();
        m_justAutoIndented = 0;
        if (!m_lastInsertion.isEmpty()
                || hasConfig(ConfigBackspace, "start")
                || hasConfig(ConfigBackspace, "2")) {
            const int line = cursorLine() + 1;
            const Column col = cursorColumn();
            QString data = lineContents(line);
            const Column ind = indentation(data);
            if (col.logical <= ind.logical && col.logical
                    && startsWithWhitespace(data, col.physical)) {
                const int ts = config(ConfigTabStop).toInt();
                const int newl = col.logical - 1 - (col.logical - 1) % ts;
                const QString prefix = tabExpand(newl);
                setLineContents(line, prefix + data.mid(col.physical));
                moveToStartOfLine();
                moveRight(prefix.size());
                m_lastInsertion.clear(); // FIXME
            } else {
                setAnchor();
                cursor().deletePreviousChar();
                m_lastInsertion.chop(1);
            }
            setTargetColumn();
        }
        endEditBlock();
    } else if (input.isKey(Key_Delete)) {
        setAnchor();
        cursor().deleteChar();
        m_lastInsertion.clear();
    } else if (input.isKey(Key_PageDown) || input.isControl('f')) {
        removeAutomaticIndentation();
        moveDown(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (input.isKey(Key_PageUp) || input.isControl('b')) {
        removeAutomaticIndentation();
        moveUp(count() * (linesOnScreen() - 2));
        m_lastInsertion.clear();
    } else if (input.isKey(Key_Tab)) {
        m_justAutoIndented = 0;
        if (hasConfig(ConfigExpandTab)) {
            const int ts = config(ConfigTabStop).toInt();
            const int col = logicalCursorColumn();
            QString str = QString(ts - col % ts, ' ');
            m_lastInsertion.append(str);
            insertText(str);
            setTargetColumn();
        } else {
            insertInInsertMode(input.raw());
        }
    } else if (input.isControl('d')) {
        // remove one level of indentation from the current line
        int shift = config(ConfigShiftWidth).toInt();
        int tab = config(ConfigTabStop).toInt();
        int line = cursorLine() + 1;
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
    } else if (input.isControl('p') || input.isControl('n')) {
        QTextCursor tc = EDITOR(textCursor());
        moveToWordBoundary(false, false);
        QString str = selectText(Range(position(), tc.position()));
        EDITOR(setTextCursor(tc));
        emit q->simpleCompletionRequested(str, input.isControl('n'));
    } else if (!input.text().isEmpty()) {
        insertInInsertMode(input.text());
    } else {
        // We don't want fancy stuff in insert mode.
        return EventHandled;
    }
    updateMiniBuffer();
    return EventHandled;
}

void FakeVimHandler::Private::insertInInsertMode(const QString &text)
{
    joinPreviousEditBlock();
    m_justAutoIndented = 0;
    m_lastInsertion.append(text);
    insertText(text);
    if (hasConfig(ConfigSmartIndent) && isElectricCharacter(text.at(0))) {
        const QString leftText = block().text()
               .left(position() - 1 - block().position());
        if (leftText.simplified().isEmpty()) {
            Range range(position(), position(), m_rangemode);
            indentText(range, text.at(0));
        }
    }
    setTargetColumn();
    endEditBlock();
    m_ctrlVActive = false;
}

EventResult FakeVimHandler::Private::handleExMode(const Input &input)
{
    if (input.isEscape()) {
        m_commandBuffer.clear();
        enterCommandMode();
        updateMiniBuffer();
        m_ctrlVActive = false;
    } else if (m_ctrlVActive) {
        m_commandBuffer.insertChar(input.raw());
        m_ctrlVActive = false;
    } else if (input.isControl('v')) {
        m_ctrlVActive = true;
    } else if (input.isBackspace()) {
        if (m_commandBuffer.isEmpty()) {
            m_commandPrefix.clear();
            enterCommandMode();
        } else {
            m_commandBuffer.deleteChar();
        }
        updateMiniBuffer();
    } else if (input.isKey(Key_Tab)) {
        QStringList completions;
        foreach (const QString &entry, g.commandHistory.items()) {
            if (entry.startsWith(m_commandBuffer.contents()))
                completions.append(entry);
        }
        qDebug() << completions;
    } else if (input.isKey(Key_Left)) {
        m_commandBuffer.moveLeft();
        updateMiniBuffer();
    } else if (input.isReturn()) {
        if (!m_commandBuffer.isEmpty()) {
            //g.commandHistory.takeLast();
            g.commandHistory.append(m_commandBuffer.contents());
            handleExCommand(m_commandBuffer.contents());
            if (m_textedit || m_plaintextedit)
                leaveVisualMode();
        }
        updateMiniBuffer();
    } else if (input.isKey(Key_Up) || input.isKey(Key_PageUp)) {
        g.commandHistory.up();
        m_commandBuffer.setContents(g.commandHistory.current());
        updateMiniBuffer();
    } else if (input.isKey(Key_Down) || input.isKey(Key_PageDown)) {
        g.commandHistory.down();
        m_commandBuffer.setContents(g.commandHistory.current());
        updateMiniBuffer();
    } else if (m_commandBuffer.handleInput(input)) {
        updateMiniBuffer();
    } else {
        qDebug() << "IGNORED IN EX-MODE: " << input.key() << input.text();
        return EventUnhandled;
    }
    return EventHandled;
}

EventResult FakeVimHandler::Private::handleSearchSubSubMode(const Input &input)
{
    if (input.isEscape()) {
        m_commandBuffer.clear();
        g.searchHistory.append(m_searchCursor.selectedText());
        m_searchCursor = QTextCursor();
        updateSelection();
        enterCommandMode();
        updateMiniBuffer();
    } else if (input.isBackspace()) {
        if (m_commandBuffer.isEmpty()) {
            m_commandPrefix.clear();
            m_searchCursor = QTextCursor();
            enterCommandMode();
        } else {
            m_commandBuffer.deleteChar();
        }
        updateMiniBuffer();
    } else if (input.isKey(Key_Left)) {
        m_commandBuffer.moveLeft();
        updateMiniBuffer();
    } else if (input.isKey(Key_Right)) {
        m_commandBuffer.moveRight();
        updateMiniBuffer();
    } else if (input.isReturn()) {
        m_searchCursor = QTextCursor();
        QString needle = m_commandBuffer.contents();
        if (!needle.isEmpty()) {
            g.searchHistory.append(needle);
            if (!hasConfig(ConfigIncSearch)) {
                SearchData sd;
                sd.needle = needle;
                sd.forward = m_lastSearchForward;
                sd.highlightCursor = false;
                sd.highlightMatches = true;
                search(sd);
            }
            finishMovement(m_commandPrefix + needle + '\n');
        }
        enterCommandMode();
        highlightMatches(needle);
        updateMiniBuffer();
    } else if (input.isKey(Key_Up) || input.isKey(Key_PageUp)) {
        // FIXME: This and the three cases below are wrong as vim
        // takes only matching entries in the history into account.
        g.searchHistory.up();
        showBlackMessage(g.searchHistory.current());
    } else if (input.isKey(Key_Down) || input.isKey(Key_PageDown)) {
        g.searchHistory.down();
        showBlackMessage(g.searchHistory.current());
    } else if (input.isKey(Key_Tab)) {
        m_commandBuffer.insertChar(QChar(9));
        updateMiniBuffer();
    } else if (m_commandBuffer.handleInput(input)) {
        updateMiniBuffer();
    }

    if (hasConfig(ConfigIncSearch) && !input.isReturn() && !input.isEscape()) {
        SearchData sd;
        sd.needle = m_commandBuffer.contents();
        sd.forward = m_lastSearchForward;
        sd.mustMove = false;
        sd.highlightCursor = true;
        sd.highlightMatches = false;
        search(sd);
    }

    //else {
    //   qDebug() << "IGNORED IN SEARCH MODE: " << input.key() << input.text();
    //   return EventUnhandled;
    //}
    return EventHandled;
}

// This uses 1 based line counting.
int FakeVimHandler::Private::readLineCode(QString &cmd)
{
    //qDebug() << "CMD: " << cmd;
    if (cmd.isEmpty())
        return -1;
    QChar c = cmd.at(0);
    cmd = cmd.mid(1);
    if (c == '.') {
        if (cmd.isEmpty())
            return cursorLine() + 1;
        QChar c1 = cmd.at(0);
        if (c1 == '+' || c1 == '-') {
            // Repeat for things like  .+4
            cmd = cmd.mid(1);
            return cursorLine() + readLineCode(cmd);
        }
        return cursorLine() + 1;
    }
    if (c == '$')
        return linesInDocument();
    if (c == '\'' && !cmd.isEmpty()) {
        if (cmd.isEmpty()) {
            showRedMessage(msgMarkNotSet(QString()));
            return -1;
        }
        int m = mark(cmd.at(0).unicode());
        if (m == -1) {
            showRedMessage(msgMarkNotSet(cmd.at(0)));
            cmd = cmd.mid(1);
            return -1;
        }
        cmd = cmd.mid(1);
        return lineForPosition(m);
    }
    if (c == '-') {
        int n = readLineCode(cmd);
        return cursorLine() + 1 - (n == -1 ? 1 : n);
    }
    if (c == '+') {
        int n = readLineCode(cmd);
        return cursorLine() + 1 + (n == -1 ? 1 : n);
    }
    if (c == '\'' && !cmd.isEmpty()) {
        int pos = mark(cmd.at(0).unicode());
        if (pos == -1) {
            showRedMessage(msgMarkNotSet(cmd.at(0)));
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
    // Parsing failed.
    cmd = c + cmd;
    return -1;
}

void FakeVimHandler::Private::setCurrentRange(const Range &range)
{
    setAnchorAndPosition(range.beginPos, range.endPos);
    m_rangemode = range.rangemode;
}

Range FakeVimHandler::Private::rangeFromCurrentLine() const
{
    Range range;
    int line = cursorLine() + 1;
    range.beginPos = firstPositionInLine(line);
    range.endPos = lastPositionInLine(line);
    return range;
}

// use handleExCommand for invoking commands that might move the cursor
void FakeVimHandler::Private::handleCommand(const QString &cmd)
{
    handleExCommand(cmd);
}

bool FakeVimHandler::Private::handleExSubstituteCommand(const ExCommand &cmd)
    // :substitute
{
    QString flags;
    QRegExp pattern;
    QString replacement;
    int count = 0;

    if (cmd.cmd.startsWith("&&")) {
        flags = cmd.cmd.mid(2);
        if (flags.isEmpty())
            flags = m_lastSubstituteFlags;
        pattern = m_lastSubstitutePattern;
        replacement = m_lastSubstituteReplacement;
        count = cmd.args.section(QLatin1Char(' '), 1, 1).toInt();
    } else if (cmd.cmd.startsWith(QLatin1Char('&'))) {
        flags = cmd.cmd.mid(1);
        if (flags.isEmpty())
            flags = m_lastSubstituteFlags;
        pattern = m_lastSubstitutePattern;
        replacement = m_lastSubstituteReplacement;
        count = cmd.args.section(QLatin1Char(' '), 1, 1).toInt();
    } else if (cmd.matches("s", "substitute")) {
        flags = m_lastSubstituteFlags;
        if (flags.isEmpty())
            flags = m_lastSubstituteFlags;
        pattern = m_lastSubstitutePattern;
        replacement = m_lastSubstituteReplacement;
        count = cmd.args.section(QLatin1Char(' '), 2, 2).toInt();
    } else {
        QString line = cmd.cmd + ' ' + cmd.args;
        line = line.trimmed();
        if (line.startsWith(_("substitute")))
            line = line.mid(10);
        else if (line.startsWith('s') && line.size() > 1
                && !isalpha(line.at(1).unicode()))
            line = line.mid(1);
        else
            return false;
        // we have /{pattern}/{string}/[flags]  now
        if (line.isEmpty())
            return false;
        const QChar separator = line.at(0);
        int pos1 = -1;
        int pos2 = -1;
        int i;
        for (i = 1; i < line.size(); ++i) {
            if (line.at(i) == separator && line.at(i - 1) != '\\') {
                pos1 = i;
                break;
            }
        }
        if (pos1 == -1)
            return false;
        for (++i; i < line.size(); ++i) {
            if (line.at(i) == separator && line.at(i - 1) != '\\') {
                pos2 = i;
                break;
            }
        }
        if (pos2 == -1)
            pos2 = line.size();

        QString needle = line.mid(1, pos1 - 1);
        replacement = line.mid(pos1 + 1, pos2 - pos1 - 1);
        flags = line.mid(pos2 + 1);

        needle.replace('$', '\n');
        needle.replace("\\\n", "\\$");
        pattern = vimPatternToQtPattern(needle, hasConfig(ConfigSmartCase));

        m_lastSubstituteFlags = flags;
        m_lastSubstitutePattern = pattern;
        m_lastSubstituteReplacement = replacement;
    }

    if (count == 0)
        count = 1;

    if (flags.contains('i'))
        pattern.setCaseSensitivity(Qt::CaseInsensitive);

    beginEditBlock();
    const bool global = flags.contains('g');
    for (int a = 0; a != count; ++a) {
        const Range range = cmd.range.endPos == 0 ? rangeFromCurrentLine() : cmd.range;
        const int beginLine = lineForPosition(range.beginPos);
        const int endLine = lineForPosition(range.endPos);
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
                repl.replace("\\&", "&");
                text = text.left(pos) + repl + text.mid(pos + matched.size());
                pos += repl.size();
                if (!global)
                    break;
            }
            if (text != origText)
                setLineContents(line, text);
        }
    }
    moveToStartOfLine();
    setTargetColumn();
    endEditBlock();
    return true;
}

bool FakeVimHandler::Private::handleExMapCommand(const ExCommand &cmd0) // :map
{
    QByteArray modes;
    enum Type { Map, Noremap, Unmap } type;

    QByteArray cmd = cmd0.cmd.toLatin1();

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

    const int pos = cmd0.args.indexOf(QLatin1Char(' '));
    if (pos == -1) {
        // FIXME: Dump mappings here.
        //qDebug() << g.mappings;
        return true;
    }

    QString lhs = cmd0.args.left(pos);
    QString rhs = cmd0.args.mid(pos + 1);
    Inputs key;
    key.parseFrom(lhs);
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
            Inputs inputs(rhs);
            foreach (char c, modes)
                g.mappings[c].insert(key, inputs);
            break;
        }
    }
    return true;
}

bool FakeVimHandler::Private::handleExHistoryCommand(const ExCommand &cmd)
{
    // :his[tory]
    if (!cmd.matches("his", "history"))
        return false;

    if (cmd.args.isEmpty()) {
        QString info;
        info += "#  command history\n";
        int i = 0;
        foreach (const QString &item, g.commandHistory.items()) {
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

bool FakeVimHandler::Private::handleExRegisterCommand(const ExCommand &cmd)
{
    // :reg[isters] and :di[splay]
    if (!cmd.matches("reg", "registers") && !cmd.matches("di", "display"))
        return false;

    QByteArray regs = cmd.args.toLatin1();
    if (regs.isEmpty()) {
        regs = "\"0123456789";
        QHashIterator<int, Register> it(g.registers);
        while (it.hasNext()) {
            it.next();
            if (it.key() > '9')
                regs += char(it.key());
        }
    }
    QString info;
    info += "--- Registers ---\n";
    foreach (char reg, regs) {
        QString value = quoteUnprintable(registerContents(reg));
        info += QString("\"%1   %2\n").arg(reg).arg(value);
    }
    emit q->extraInformationChanged(info);
    updateMiniBuffer();
    return true;
}

bool FakeVimHandler::Private::handleExSetCommand(const ExCommand &cmd)
{
    // :se[t]
    if (!cmd.matches("se", "set"))
        return false;

    showBlackMessage(QString());
    SavedAction *act = theFakeVimSettings()->item(cmd.args);
    QTC_CHECK(!cmd.args.isEmpty()); // Handled by plugin.
    if (act && act->value().type() == QVariant::Bool) {
        // Boolean config to be switched on.
        bool oldValue = act->value().toBool();
        if (oldValue == false)
            act->setValue(true);
        else if (oldValue == true)
            {} // nothing to do
    } else if (act) {
        // Non-boolean to show.
        showBlackMessage(cmd.args + '=' + act->value().toString());
    } else if (cmd.args.startsWith(_("no"))
            && (act = theFakeVimSettings()->item(cmd.args.mid(2)))) {
        // Boolean config to be switched off.
        bool oldValue = act->value().toBool();
        if (oldValue == true)
            act->setValue(false);
        else if (oldValue == false)
            {} // nothing to do
    } else if (cmd.args.contains('=')) {
        // Non-boolean config to set.
        int p = cmd.args.indexOf('=');
        QString error = theFakeVimSettings()
                ->trySetValue(cmd.args.left(p), cmd.args.mid(p + 1));
        if (!error.isEmpty())
            showRedMessage(error);
    } else {
        showRedMessage(FakeVimHandler::tr("Unknown option: ") + cmd.args);
    }
    updateMiniBuffer();
    updateEditor();
    return true;
}

bool FakeVimHandler::Private::handleExNormalCommand(const ExCommand &cmd)
{
    // :norm[al]
    if (!cmd.matches("norm", "normal"))
        return false;
    //qDebug() << "REPLAY NORMAL: " << quoteUnprintable(reNormal.cap(3));
    replay(cmd.args, 1);
    return true;
}

bool FakeVimHandler::Private::handleExDeleteCommand(const ExCommand &cmd)
{
    // :d[elete]
    if (!cmd.matches("d", "delete"))
        return false;

    Range range = cmd.range.endPos == 0 ? rangeFromCurrentLine() : cmd.range;
    setCurrentRange(range);
    QString reg = cmd.args;
    QString text = selectText(range);
    removeText(currentRange());
    if (!reg.isEmpty()) {
        const int r = reg.at(0).unicode();
        setRegisterContents(r, text);
        setRegisterRangeMode(r, RangeLineMode);
    }
    return true;
}

bool FakeVimHandler::Private::handleExWriteCommand(const ExCommand &cmd)
{
    // :w, :x, :wq, ...
    //static QRegExp reWrite("^[wx]q?a?!?( (.*))?$");
    if (cmd.cmd != "w" && cmd.cmd != "x" && cmd.cmd != "wq")
        return false;

    int beginLine = lineForPosition(cmd.range.beginPos);
    int endLine = lineForPosition(cmd.range.endPos);
    const bool noArgs = (beginLine == -1);
    if (beginLine == -1)
        beginLine = 0;
    if (endLine == -1)
        endLine = linesInDocument();
    //qDebug() << "LINES: " << beginLine << endLine;
    //QString prefix = cmd.args;
    const bool forced = cmd.hasBang;
    //const bool quit = prefix.contains(QChar('q')) || prefix.contains(QChar('x'));
    //const bool quitAll = quit && prefix.contains(QChar('a'));
    QString fileName = cmd.args;
    if (fileName.isEmpty())
        fileName = m_currentFileName;
    QFile file1(fileName);
    const bool exists = file1.exists();
    if (exists && !forced && !noArgs) {
        showRedMessage(FakeVimHandler::tr
            ("File \"%1\" exists (add ! to override)").arg(fileName));
    } else if (file1.open(QIODevice::ReadWrite)) {
        // Nobody cared, so act ourselves.
        file1.close();
        Range range(firstPositionInLine(beginLine),
            firstPositionInLine(endLine), RangeLineMode);
        QString contents = selectText(range);
        QFile::remove(fileName);
        QFile file2(fileName);
        if (file2.open(QIODevice::ReadWrite)) {
            QTextStream ts(&file2);
            ts << contents;
        } else {
            showRedMessage(FakeVimHandler::tr
               ("Cannot open file \"%1\" for writing").arg(fileName));
        }
        // Check result by reading back.
        QFile file3(fileName);
        file3.open(QIODevice::ReadOnly);
        QByteArray ba = file3.readAll();
        showBlackMessage(FakeVimHandler::tr("\"%1\" %2 %3L, %4C written")
            .arg(fileName).arg(exists ? " " : tr(" [New] "))
            .arg(ba.count('\n')).arg(ba.size()));
        //if (quitAll)
        //    passUnknownExCommand(forced ? "qa!" : "qa");
        //else if (quit)
        //    passUnknownExCommand(forced ? "q!" : "q");
    } else {
        showRedMessage(FakeVimHandler::tr
            ("Cannot open file \"%1\" for reading").arg(fileName));
    }
    return true;
}

bool FakeVimHandler::Private::handleExReadCommand(const ExCommand &cmd)
{
    // :r[ead]
    if (!cmd.matches("r", "read"))
        return false;

    beginEditBlock();
    moveToStartOfLine();
    setTargetColumn();
    moveDown();
    m_currentFileName = cmd.args;
    QFile file(m_currentFileName);
    file.open(QIODevice::ReadOnly);
    QTextStream ts(&file);
    QString data = ts.readAll();
    insertText(data);
    endEditBlock();
    showBlackMessage(FakeVimHandler::tr("\"%1\" %2L, %3C")
        .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));
    return true;
}

bool FakeVimHandler::Private::handleExBangCommand(const ExCommand &cmd) // :!
{
    if (!cmd.cmd.startsWith(QLatin1Char('!')))
        return false;

    setCurrentRange(cmd.range);
    int targetPosition = firstPositionInLine(lineForPosition(cmd.range.beginPos));
    QString command = QString(cmd.cmd.mid(1) + ' ' + cmd.args).trimmed();
    QString text = selectText(cmd.range);
    QProcess proc;
    proc.start(command);
    proc.waitForStarted();
#ifdef Q_OS_WIN
    text.replace(_("\n"), _("\r\n"));
#endif
    proc.write(text.toUtf8());
    proc.closeWriteChannel();
    proc.waitForFinished();
    QString result = QString::fromUtf8(proc.readAllStandardOutput());
    if (text.isEmpty()) {
        emit q->extraInformationChanged(result);
    } else {
        beginEditBlock();
        removeText(currentRange());
        insertText(result);
        setPosition(targetPosition);
        endEditBlock();
        leaveVisualMode();
        //qDebug() << "FILTER: " << command;
        showBlackMessage(FakeVimHandler::tr("%n lines filtered", 0,
            text.count('\n')));
    }
    return true;
}

bool FakeVimHandler::Private::handleExShiftCommand(const ExCommand &cmd)
{
    if (cmd.cmd != "<" && cmd.cmd != ">")
        return false;

    Range range = cmd.range;
    if (cmd.range.endPos == 0)
        range = rangeFromCurrentLine();
    setCurrentRange(range);
    int count = qMax(1, cmd.args.toInt());
    if (cmd.cmd == "<")
        shiftRegionLeft(count);
    else
        shiftRegionRight(count);
    leaveVisualMode();
    const int beginLine = lineForPosition(range.beginPos);
    const int endLine = lineForPosition(range.endPos);
    showBlackMessage(FakeVimHandler::tr("%n lines %1ed %2 time", 0,
        (endLine - beginLine + 1)).arg(cmd.cmd).arg(count));
    return true;
}

bool FakeVimHandler::Private::handleExNohlsearchCommand(const ExCommand &cmd)
{
    // :nohlsearch
    if (!cmd.cmd.startsWith("noh"))
        return false;

    m_searchSelections.clear();
    updateSelection();
    return true;
}

bool FakeVimHandler::Private::handleExRedoCommand(const ExCommand &cmd)
{
    // :redo
    if (cmd.cmd != "red" && cmd.cmd != "redo")
        return false;

    redo();
    updateMiniBuffer();
    return true;
}

bool FakeVimHandler::Private::handleExGotoCommand(const ExCommand &cmd)
{
    // :<nr>
    if (!cmd.cmd.isEmpty())
        return false;

    const int beginLine = lineForPosition(cmd.range.beginPos);
    setPosition(firstPositionInLine(beginLine));
    showBlackMessage(QString());
    return true;
}

bool FakeVimHandler::Private::handleExSourceCommand(const ExCommand &cmd)
{
    // :source
    if (cmd.cmd != "so" && cmd.cmd != "source")
        return false;

    QString fileName = cmd.args;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showRedMessage(FakeVimHandler::tr("Cannot open file %1").arg(fileName));
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
            ExCommand cmd;
            cmd.setContentsFromLine(QString::fromLocal8Bit(line));
            handleExCommandHelper(cmd);
        }
    }
    file.close();
    return true;
}

bool FakeVimHandler::Private::handleExEchoCommand(const ExCommand &cmd)
{
    // :echo
    if (cmd.cmd != "echo")
        return false;
    m_currentMessage = cmd.args;
    return true;
}

void FakeVimHandler::Private::handleExCommand(const QString &line0)
{
    QString line = line0; // Make sure we have a copy to prevent aliasing.

    if (line.endsWith(QLatin1Char('%'))) {
        line.chop(1);
        int percent = line.toInt();
        setPosition(firstPositionInLine(percent * linesInDocument() / 100));
        showBlackMessage(QString());
        return;
    }

    // FIXME: that seems to be different for %w and %s
    if (line.startsWith(QLatin1Char('%')))
        line = "1,$" + line.mid(1);

    const int beginLine = readLineCode(line);
    int endLine = -1;
    if (line.startsWith(',')) {
        line = line.mid(1);
        endLine = readLineCode(line);
    }
    if (beginLine != -1 && endLine == -1)
        endLine = beginLine;
    ExCommand cmd;
    cmd.setContentsFromLine(line);
    if (beginLine != -1) {
        const int beginPos = firstPositionInLine(beginLine);
        const int endPos = lastPositionInLine(endLine);
        cmd.range = Range(beginPos, endPos, RangeLineMode);
        cmd.count = beginLine;
    }
    //qDebug() << "CMD: " << cmd;

    enterCommandMode();
    showBlackMessage(QString());
    if (!handleExCommandHelper(cmd))
        showRedMessage(tr("Not an editor command: %1").arg(cmd.cmd));
}

bool FakeVimHandler::Private::handleExCommandHelper(const ExCommand &cmd)
{
    return handleExPluginCommand(cmd)
        || handleExGotoCommand(cmd)
        || handleExBangCommand(cmd)
        || handleExHistoryCommand(cmd)
        || handleExRegisterCommand(cmd)
        || handleExDeleteCommand(cmd)
        || handleExMapCommand(cmd)
        || handleExNohlsearchCommand(cmd)
        || handleExNormalCommand(cmd)
        || handleExReadCommand(cmd)
        || handleExRedoCommand(cmd)
        || handleExSetCommand(cmd)
        || handleExShiftCommand(cmd)
        || handleExSourceCommand(cmd)
        || handleExSubstituteCommand(cmd)
        || handleExWriteCommand(cmd)
        || handleExEchoCommand(cmd);
}

bool FakeVimHandler::Private::handleExPluginCommand(const ExCommand &cmd)
{
    bool handled = false;
    emit q->handleExCommandRequested(&handled, cmd);
    //qDebug() << "HANDLER REQUEST: " << cmd.cmd << handled;
    return handled;
}

void FakeVimHandler::Private::searchBalanced(bool forward, QChar needle, QChar other)
{
    int level = 1;
    int pos = position();
    const int npos = forward ? lastPositionInDocument() : 0;
    while (true) {
        if (forward)
            ++pos;
        else
            --pos;
        if (pos == npos)
            return;
        QChar c = document()->characterAt(pos);
        if (c == other)
            ++level;
        else if (c == needle)
            --level;
        if (level == 0) {
            const int oldLine = cursorLine() - cursorLineOnScreen();
            // Making this unconditional feels better, but is not "vim like".
            if (oldLine != cursorLine() - cursorLineOnScreen())
                scrollToLine(cursorLine() - linesOnScreen() / 2);
            setPosition(pos);
            setTargetColumn();
            updateSelection();
            recordJump();
            return;
        }
    }
}

void FakeVimHandler::Private::search(const SearchData &sd)
{
    if (sd.needle.isEmpty())
        return;

    const bool incSearch = hasConfig(ConfigIncSearch);
    QTextDocument::FindFlags flags = QTextDocument::FindCaseSensitively;
    if (!sd.forward)
        flags |= QTextDocument::FindBackward;

    QRegExp needleExp = vimPatternToQtPattern(sd.needle, hasConfig(ConfigSmartCase));

    const int oldLine = cursorLine() - cursorLineOnScreen();

    int startPos = position();
    if (sd.mustMove)
        sd.forward ? ++startPos : --startPos;

    m_searchCursor = QTextCursor();
    QTextCursor tc = document()->find(needleExp, startPos, flags);
    if (tc.isNull()) {
        int startPos = sd.forward ? 0 : lastPositionInDocument();
        tc = document()->find(needleExp, startPos, flags);
        if (tc.isNull()) {
            if (!incSearch) {
                highlightMatches(QString());
                showRedMessage(FakeVimHandler::tr("Pattern not found: %1")
                    .arg(needleExp.pattern()));
            }
            updateSelection();
            return;
        }
        if (!incSearch) {
            QString msg = sd.forward
                ? FakeVimHandler::tr("search hit BOTTOM, continuing at TOP")
                : FakeVimHandler::tr("search hit TOP, continuing at BOTTOM");
            showRedMessage(msg);
        }
    }

    // Set Cursor. In contrast to the main editor we have the cursor
    // position before the anchor position.
    setAnchorAndPosition(tc.position(), tc.anchor());

    // Making this unconditional feels better, but is not "vim like".
    if (oldLine != cursorLine() - cursorLineOnScreen())
        scrollToLine(cursorLine() - linesOnScreen() / 2);

    if (incSearch && sd.highlightCursor)
        m_searchCursor = cursor();

    setTargetColumn();

    if (sd.highlightMatches)
        highlightMatches(sd.needle);
    updateSelection();
    recordJump();
}

void FakeVimHandler::Private::highlightMatches(const QString &needle)
{
    if (!hasConfig(ConfigHlSearch))
        return;
    if (needle == m_oldNeedle)
        return;
    m_oldNeedle = needle;
    m_searchSelections.clear();
    if (!needle.isEmpty()) {
        QTextCursor tc = cursor();
        tc.movePosition(StartOfDocument, MoveAnchor);

        QRegExp needleExp = vimPatternToQtPattern(needle, hasConfig(ConfigSmartCase));
        if (!needleExp.isValid()) {
            QString error = needleExp.errorString();
            showRedMessage(
                FakeVimHandler::tr("Invalid regular expression: %1").arg(error));
            return;
        }

        while (!tc.atEnd()) {
            tc = tc.document()->find(needleExp, tc.position());
            if (tc.isNull())
                break;
            if (!tc.hasSelection())
                tc.movePosition(Right, KeepAnchor, 1);
            QTextEdit::ExtraSelection sel;
            sel.cursor = tc;
            sel.format = tc.blockCharFormat();
            sel.format.setBackground(QColor(177, 177, 0));
            m_searchSelections.append(sel);
            if (document()->characterAt(tc.position()) == ParagraphSeparator)
                tc.movePosition(Right, MoveAnchor);
        }
    }
    updateSelection();
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine()
{
    QTextDocument *doc = document();
    int firstPos = block().position();
    for (int i = firstPos, n = firstPos + block().length(); i < n; ++i) {
        if (!doc->characterAt(i).isSpace() || i == n - 1) {
            setPosition(i);
            return;
        }
    }
    setPosition(block().position());
}

void FakeVimHandler::Private::indentSelectedText(QChar typedChar)
{
    beginEditBlock();
    setTargetColumn();
    int beginLine = qMin(lineForPosition(position()), lineForPosition(anchor()));
    int endLine = qMax(lineForPosition(position()), lineForPosition(anchor()));

    Range range(anchor(), position(), m_rangemode);
    indentText(range, typedChar);

    setPosition(firstPositionInLine(beginLine));
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1==", endLine - beginLine + 1);
    endEditBlock();
}

void FakeVimHandler::Private::indentText(const Range &range, QChar typedChar)
{
    int beginLine = lineForPosition(range.beginPos);
    int endLine = lineForPosition(range.endPos);
    if (beginLine > endLine)
        qSwap(beginLine, endLine);

    // LineForPosition has returned 1-based line numbers.
    emit q->indentRegion(beginLine - 1, endLine - 1, typedChar);
    if (beginLine != endLine)
        showBlackMessage("MARKS ARE OFF NOW");
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

    const int sw = config(ConfigShiftWidth).toInt();
    beginEditBlock();
    for (int line = beginLine; line <= endLine; ++line) {
        QString data = lineContents(line);
        const Column col = indentation(data);
        data = tabExpand(col.logical + sw * repeat) + data.mid(col.physical);
        setLineContents(line, data);
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

    beginEditBlock();
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
    const QTextBlock &bl = block();
    //Column column = cursorColumn();
    //int logical = logical
    const int maxcol = bl.length() - 2;
    if (m_targetColumn == -1) {
        setPosition(bl.position() + qMax(0, maxcol));
        return;
    }
    const int physical = logicalToPhysicalColumn(m_targetColumn, bl.text());
    //qDebug() << "CORRECTING COLUMN FROM: " << logical << "TO" << m_targetColumn;
    if (physical >= maxcol)
        setPosition(bl.position() + qMax(0, maxcol));
    else
        setPosition(bl.position() + physical);
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
    QTextDocument *doc = document();
    int n = forward ? lastPositionInDocument() : 0;
    int lastClass = -1;
    if (changeWord) {
        lastClass = charClass(characterAtCursor(), simple);
        --repeat;
        if (changeWord && block().length() == 1) // empty line
            --repeat;
    }
    while (repeat >= 0) {
        QChar c = doc->characterAt(position() + (forward ? 1 : -1));
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass && (lastClass != 0 || changeWord))
            --repeat;
        if (repeat == -1)
            break;
        lastClass = thisClass;
        if (position() == n)
            break;
        forward ? moveRight() : moveLeft();
        if (changeWord && block().length() == 1) // empty line
            --repeat;
        if (repeat == -1)
            break;
    }
}

bool FakeVimHandler::Private::handleFfTt(QString key)
{
    int key0 = key.size() == 1 ? key.at(0).unicode() : 0;
    int oldPos = position();
    // m_subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = m_subsubdata.is('f') || m_subsubdata.is('t');
    int repeat = count();
    QTextDocument *doc = document();
    int n = block().position();
    if (forward)
        n += block().length();
    int pos = position();
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
                moveRight(pos - position());
            else
                moveLeft(position() - pos);
            break;
        }
    }
    if (repeat == 0) {
        setTargetColumn();
        return true;
    }
    setPosition(oldPos);
    return false;
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
            if (atBlockEnd())
                --repeat;
        } else {
            if (block().length() == 1) // empty line
                --repeat;
        }
        if (repeat == 0)
            break;
        if (position() == n)
            break;
    }
    setTargetColumn();
}

void FakeVimHandler::Private::moveToMatchingParanthesis()
{
    bool moved = false;
    bool forward = false;

    const int anc = anchor();
    QTextCursor tc = cursor();
    emit q->moveToMatchingParenthesis(&moved, &forward, &tc);
    if (moved && forward)
        tc.movePosition(Left, KeepAnchor, 1);
    setAnchorAndPosition(anc, tc.position());
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

int FakeVimHandler::Private::cursorLine() const
{
    return block().blockNumber();
}

int FakeVimHandler::Private::physicalCursorColumn() const
{
    return position() - block().position();
}

int FakeVimHandler::Private::physicalToLogicalColumn
    (const int physical, const QString &line) const
{
    const int ts = config(ConfigTabStop).toInt();
    int p = 0;
    int logical = 0;
    while (p < physical) {
        QChar c = line.at(p);
        //if (c == QLatin1Char(' '))
        //    ++logical;
        //else
        if (c == QLatin1Char('\t'))
            logical += ts - logical % ts;
        else
            ++logical;
            //break;
        ++p;
    }
    return logical;
}

int FakeVimHandler::Private::logicalToPhysicalColumn
    (const int logical, const QString &line) const
{
    const int ts = config(ConfigTabStop).toInt();
    int physical = 0;
    for (int l = 0; l < logical && physical < line.size(); ++physical) {
        QChar c = line.at(physical);
        if (c == QLatin1Char('\t'))
            l += ts - l % ts;
        else
            ++l;
    }
    return physical;
}

int FakeVimHandler::Private::logicalCursorColumn() const
{
    const int physical = physicalCursorColumn();
    const QString line = block().text();
    return physicalToLogicalColumn(physical, line);
}

Column FakeVimHandler::Private::cursorColumn() const
{
    return Column(physicalCursorColumn(), logicalCursorColumn());
}

int FakeVimHandler::Private::linesInDocument() const
{
    if (cursor().isNull())
        return 0;
    const int count = document()->blockCount();
    // Qt inserts an empty line if the last character is a '\n',
    // but that's not how vi does it.
    return document()->lastBlock().length() <= 1 ? count - 1 : count;
}

void FakeVimHandler::Private::scrollToLine(int line)
{
    // FIXME: works only for QPlainTextEdit
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    //qDebug() << "SCROLL: " << scrollBar->value() << line;
    scrollBar->setValue(line);
    //QTC_CHECK(firstVisibleLine() == line);
}

int FakeVimHandler::Private::firstVisibleLine() const
{
    QScrollBar *scrollBar = EDITOR(verticalScrollBar());
    if (0 && scrollBar->value() != cursorLine() - cursorLineOnScreen()) {
        qDebug() << "SCROLLBAR: " << scrollBar->value()
            << "CURSORLINE IN DOC" << cursorLine()
            << "CURSORLINE ON SCREEN" << cursorLineOnScreen();
    }
    //return scrollBar->value();
    return cursorLine() - cursorLineOnScreen();
}

void FakeVimHandler::Private::scrollUp(int count)
{
    scrollToLine(cursorLine() - cursorLineOnScreen() - count);
}

int FakeVimHandler::Private::lastPositionInDocument() const
{
    QTextBlock block = document()->lastBlock();
    return block.position() + block.length() - 1;
}

QString FakeVimHandler::Private::selectText(const Range &range) const
{
    if (range.rangemode == RangeCharMode) {
        QTextCursor tc = cursor();
        tc.setPosition(range.beginPos, MoveAnchor);
        tc.setPosition(range.endPos, KeepAnchor);
        return tc.selection().toPlainText();
    }
    if (range.rangemode == RangeLineMode) {
        QTextCursor tc = cursor();
        int firstPos = firstPositionInLine(lineForPosition(range.beginPos));
        int lastLine = lineForPosition(range.endPos);
        bool endOfDoc = lastLine == document()->lastBlock().blockNumber() + 1;
        int lastPos = endOfDoc ? lastPositionInDocument() : firstPositionInLine(lastLine + 1);
        tc.setPosition(firstPos, MoveAnchor);
        tc.setPosition(lastPos, KeepAnchor);
        return tc.selection().toPlainText() + QString((endOfDoc? "\n" : ""));
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
    QTextBlock block = document()->findBlockByNumber(beginLine - 1);
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

void FakeVimHandler::Private::yankText(const Range &range, int reg)
{
    setRegisterContents(reg, selectText(range));
    setRegisterRangeMode(reg, range.rangemode);
}

void FakeVimHandler::Private::transformText(const Range &range,
    Transformation transformFunc, const QVariant &extra)
{
    QTextCursor tc = cursor();
    switch (range.rangemode) {
        case RangeCharMode: {
            // This can span multiple lines.
            beginEditBlock();
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            TransformationData td(tc.selectedText(), extra);
            (this->*transformFunc)(&td);
            tc.removeSelectedText();
            tc.insertText(td.to);
            endEditBlock();
            return;
        }
        case RangeLineMode:
        case RangeLineModeExclusive: {
            beginEditBlock();
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
            tc.insertText(td.to);
            endEditBlock();
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
            QTextBlock block = document()->findBlockByNumber(endLine - 1);
            beginEditBlock();
            for (int i = beginLine; i <= endLine && block.isValid(); ++i) {
                int bCol = qMin(beginColumn, block.length() - 1);
                int eCol = qMin(endColumn + 1, block.length() - 1);
                tc.setPosition(block.position() + bCol, MoveAnchor);
                tc.setPosition(block.position() + eCol, KeepAnchor);
                TransformationData td(tc.selectedText(), extra);
                (this->*transformFunc)(&td);
                tc.removeSelectedText();
                tc.insertText(td.to);
                block = block.previous();
            }
            endEditBlock();
        }
    }
}

void FakeVimHandler::Private::insertText(const Register &reg)
{
    QTC_ASSERT(reg.rangemode == RangeCharMode,
        qDebug() << "WRONG INSERT MODE: " << reg.rangemode; return);
    setAnchor();
    cursor().insertText(reg.contents);
    m_lastChangePosition = cursor().position();
    //dump("AFTER INSERT");
}

void FakeVimHandler::Private::removeText(const Range &range)
{
    //qDebug() << "REMOVE: " << range;
    transformText(range, &FakeVimHandler::Private::removeTransform);
}

void FakeVimHandler::Private::removeTransform(TransformationData *td)
{
    Q_UNUSED(td);
}

void FakeVimHandler::Private::downCase(const Range &range)
{
    transformText(range, &FakeVimHandler::Private::downCaseTransform);
}

void FakeVimHandler::Private::downCaseTransform(TransformationData *td)
{
    td->to = td->from.toLower();
}

void FakeVimHandler::Private::upCase(const Range &range)
{
    transformText(range, &FakeVimHandler::Private::upCaseTransform);
}

void FakeVimHandler::Private::upCaseTransform(TransformationData *td)
{
    td->to = td->from.toUpper();
}

void FakeVimHandler::Private::invertCase(const Range &range)
{
    transformText(range, &FakeVimHandler::Private::invertCaseTransform);
}

void FakeVimHandler::Private::invertCaseTransform(TransformationData *td)
{
    foreach (QChar c, td->from)
        td->to += c.isUpper() ? c.toLower() : c.toUpper();
}

void FakeVimHandler::Private::replaceText(const Range &range, const QString &str)
{
    Transformation tr = &FakeVimHandler::Private::replaceByStringTransform;
    transformText(range, tr, str);
}

void FakeVimHandler::Private::replaceByStringTransform(TransformationData *td)
{
    td->to = td->extraData.toString();
}

void FakeVimHandler::Private::replaceByCharTransform(TransformationData *td)
{
    td->to = QString(td->from.size(), td->extraData.toChar());
}

void FakeVimHandler::Private::pasteText(bool afterCursor)
{
    const QString text = registerContents(m_register);
    const QStringList lines = text.split(QChar('\n'));

    beginEditBlock();

    if (isVisualCharMode()) {
        leaveVisualMode();
        m_submode = DeleteSubMode;
        finishMovement();
    } else if (isVisualLineMode()) {
        leaveVisualMode();
        m_rangemode = RangeLineMode;
        yankText(currentRange(), m_register);
        removeText(currentRange());
        handleStartOfLine();
    } else if (isVisualBlockMode()) {
        leaveVisualMode();
        m_rangemode = RangeBlockMode;
        yankText(currentRange(), m_register);
        removeText(currentRange());
        setPosition(qMin(position(), anchor()));
    }

    switch (registerRangeMode(m_register)) {
        case RangeCharMode: {
            m_targetColumn = 0;
            for (int i = count(); --i >= 0; ) {
                if (afterCursor && rightDist() > 0)
                    moveRight();
                setAnchor();
                insertText(text);
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
            QTextCursor tc = cursor();
            for (int i = count(); --i >= 0; ) {
                bool lastLine = document()->lastBlock() == this->block();
                if (afterCursor) {
                    if (lastLine) {
                        tc.movePosition(EndOfLine, MoveAnchor);
                        tc.insertBlock();
                    }
                    moveDown();
                }
                setAnchor();
                insertText(text);
                if (afterCursor && lastLine) {
                    tc.movePosition(Left, KeepAnchor);
                    tc.removeSelectedText();
                    setAnchor();
                    moveUp(lines.size() - 2);
                } else {
                    moveUp(lines.size() - 1);
                }
            }
            moveToFirstNonBlankOnLine();
            break;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            QTextBlock block = this->block();
            if (afterCursor)
                moveRight();
            QTextCursor tc = cursor();
            const int col = tc.position() - block.position();
            //for (int i = lines.size(); --i >= 0; ) {
            for (int i = 0; i < lines.size(); ++i) {
                const QString line = lines.at(i);
                tc.movePosition(StartOfLine, MoveAnchor);
                if (col >= block.length()) {
                    tc.movePosition(EndOfLine, MoveAnchor);
                    tc.insertText(QString(col - line.size() + 1, QChar(' ')));
                } else {
                    tc.movePosition(Right, MoveAnchor, col - 1 + afterCursor);
                }
                //qDebug() << "INSERT " << line << " AT " << tc.position()
                //    << "COL: " << col;
                tc.insertText(line);
                tc.movePosition(StartOfLine, MoveAnchor);
                tc.movePosition(Down, MoveAnchor, 1);
                if (tc.position() >= lastPositionInDocument() - 1) {
                    tc.insertText(QString(QChar('\n')));
                    tc.movePosition(Up, MoveAnchor, 1);
                }
                block = block.next();
            }
            moveLeft();
            break;
        }
    }

    endEditBlock();
}

QString FakeVimHandler::Private::lineContents(int line) const
{
    return document()->findBlockByNumber(line - 1).text();
}

void FakeVimHandler::Private::setLineContents(int line, const QString &contents)
{
    QTextBlock block = document()->findBlockByNumber(line - 1);
    QTextCursor tc = cursor();
    const int begin = block.position();
    const int len = block.length();
    tc.setPosition(begin);
    tc.setPosition(begin + len - 1, KeepAnchor);
    tc.removeSelectedText();
    tc.insertText(contents);
}

int FakeVimHandler::Private::blockBoundary(const QString &left,
    const QString &right, bool closing, int count) const
{
    const QString &begin = closing ? left : right;
    const QString &end = closing ? right : left;

    // shift cursor if it is already on opening/closing string
    QTextCursor tc1 = cursor();
    int pos = tc1.position();
    int max = document()->characterCount();
    int sz = left.size();
    int from = qMax(pos - sz + 1, 0);
    int to = qMin(pos + sz, max);
    tc1.setPosition(from);
    tc1.setPosition(to, KeepAnchor);
    int i = tc1.selectedText().indexOf(left);
    if (i != -1) {
        // - on opening string:
        tc1.setPosition(from + i + sz);
    } else {
        sz = right.size();
        from = qMax(pos - sz + 1, 0);
        to = qMin(pos + sz, max);
        tc1.setPosition(from);
        tc1.setPosition(to, KeepAnchor);
        i = tc1.selectedText().indexOf(right);
        if (i != -1) {
            // - on closing string:
            tc1.setPosition(from + i);
        } else {
            tc1 = cursor();
        }
    }

    QTextCursor tc2 = tc1;
    QTextDocument::FindFlags flags(closing ? 0 : QTextDocument::FindBackward);
    int level = 0;
    int counter = 0;
    while (true) {
        tc2 = document()->find(end, tc2, flags);
        if (tc2.isNull())
            return -1;
        if (!tc1.isNull())
            tc1 = document()->find(begin, tc1, flags);

        while (!tc1.isNull() && (closing ? (tc1 < tc2) : (tc2 < tc1))) {
            ++level;
            tc1 = document()->find(begin, tc1, flags);
        }

        while (level > 0
               && (tc1.isNull() || (closing ? (tc2 < tc1) : (tc1 < tc2)))) {
            --level;
            tc2 = document()->find(end, tc2, flags);
            if (tc2.isNull())
                return -1;
        }

        if (level == 0
            && (tc1.isNull() || (closing ? (tc2 < tc1) : (tc1 < tc2)))) {
            ++counter;
            if (counter >= count)
                break;
        }
    }

    return tc2.position() - end.size();
}


int FakeVimHandler::Private::firstPositionInLine(int line) const
{
    return document()->findBlockByNumber(line - 1).position();
}

int FakeVimHandler::Private::lastPositionInLine(int line) const
{
    QTextBlock block = document()->findBlockByNumber(line - 1);
    return block.position() + block.length() - 1;
}

int FakeVimHandler::Private::lineForPosition(int pos) const
{
    QTextCursor tc = cursor();
    tc.setPosition(pos);
    return tc.block().blockNumber() + 1;
}

void FakeVimHandler::Private::toggleVisualMode(VisualMode visualMode)
{
    if (isVisualMode()) {
        leaveVisualMode();
    } else {
        m_positionPastEnd = false;
        m_anchorPastEnd = false;
        m_visualMode = visualMode;
        const int pos = position();
        //setMark('<', pos);
        //setMark('>', pos + 1);
        setAnchorAndPosition(pos, pos);
        updateMiniBuffer();
        updateSelection();
    }
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
    // FIXME: That's only an approximaxtion. The real solution might
    // be to store marks and old userData with QTextBlock setUserData
    // and retrieve them afterward.
    const int current = document()->availableUndoSteps();
    EDITOR(undo());
    const int rev = document()->availableUndoSteps();
    if (current == rev)
        showBlackMessage(FakeVimHandler::tr("Already at oldest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        setPosition(m_undoCursorPosition[rev]);
    setTargetColumn();
    if (atEndOfLine())
        moveLeft();
}

void FakeVimHandler::Private::redo()
{
    const int current = document()->availableUndoSteps();
    EDITOR(redo());
    const int rev = document()->availableUndoSteps();
    if (rev == current)
        showBlackMessage(FakeVimHandler::tr("Already at newest change"));
    else
        showBlackMessage(QString());

    if (m_undoCursorPosition.contains(rev))
        setPosition(m_undoCursorPosition[rev]);
    setTargetColumn();
}

void FakeVimHandler::Private::updateCursorShape()
{
    bool thinCursor = m_mode == ExMode
            || m_subsubmode == SearchSubSubMode
            || m_mode == InsertMode
            || isVisualMode()
            || cursor().hasSelection();
    EDITOR(setOverwriteMode(!thinCursor));
}

void FakeVimHandler::Private::enterReplaceMode()
{
    m_mode = ReplaceMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_commandPrefix.clear();
    m_lastInsertion.clear();
    m_lastDeletion.clear();
}

void FakeVimHandler::Private::enterInsertMode()
{
    m_mode = InsertMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_commandPrefix.clear();
    m_lastInsertion.clear();
    m_lastDeletion.clear();
}

void FakeVimHandler::Private::enterCommandMode()
{
    if (atEndOfLine())
        moveLeft();
    m_mode = CommandMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_commandPrefix.clear();
}

void FakeVimHandler::Private::enterExMode()
{
    m_mode = ExMode;
    m_submode = NoSubMode;
    m_subsubmode = NoSubSubMode;
    m_commandPrefix = ':';
}

void FakeVimHandler::Private::recordJump()
{
    m_jumpListUndo.append(cursorPosition());
    m_jumpListRedo.clear();
    UNDO_DEBUG("jumps: " << m_jumpListUndo);
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
        QTextBlock bl = block();
        Range range(bl.position(), bl.position());
        const int oldSize = bl.text().size();
        indentText(range, QLatin1Char('\n'));
        m_justAutoIndented = bl.text().size() - oldSize;
    } else {
        QTextBlock bl = goingDown ? block().previous() : block().next();
        QString text = bl.text();
        int pos = 0;
        int n = text.size();
        while (pos < n && text.at(pos).isSpace())
            ++pos;
        text.truncate(pos);
        // FIXME: handle 'smartindent' and 'cindent'
        insertText(text);
        m_justAutoIndented = text.size();
    }
}

bool FakeVimHandler::Private::removeAutomaticIndentation()
{
    if (!hasConfig(ConfigAutoIndent) || m_justAutoIndented == 0)
        return false;
/*
    m_tc.movePosition(StartOfLine, KeepAnchor);
    m_tc.removeSelectedText();
    m_lastInsertion.chop(m_justAutoIndented);
*/
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
    for (int i = n; --i >= 0; ) {
        foreach (QChar c, command) {
            //qDebug() << "  REPLAY: " << c.unicode();
            handleKey(Input(c));
        }
    }
}

void FakeVimHandler::Private::selectWordTextObject(bool inner)
{
    Q_UNUSED(inner); // FIXME
    m_movetype = MoveExclusive;
    moveToWordBoundary(false, false, true);
    setAnchor();
    // FIXME: Rework the 'anchor' concept.
    //if (isVisualMode())
    //    setMark('<', cursor().position());
    moveToWordBoundary(false, true, true);
    setTargetColumn();
    m_movetype = MoveInclusive;
}

void FakeVimHandler::Private::selectWORDTextObject(bool inner)
{
    Q_UNUSED(inner); // FIXME
    m_movetype = MoveExclusive;
    moveToWordBoundary(true, false, true);
    setAnchor();
    // FIXME: Rework the 'anchor' concept.
    //if (isVisualMode())
    //    setMark('<', cursor().position());
    moveToWordBoundary(true, true, true);
    setTargetColumn();
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

void FakeVimHandler::Private::selectBlockTextObject(bool inner,
    char left, char right)
{
    QString sleft = QString(QLatin1Char(left));
    QString sright = QString(QLatin1Char(right));

    int p1 = blockBoundary(sleft, sright, false, count());
    if (p1 == -1)
        return;

    int p2 = blockBoundary(sleft, sright, true, count());
    if (p2 == -1)
        return;

    if (inner) {
        p1 += sleft.size();
        --p2;
    } else {
        p2 -= sright.size() - 1;
    }

    setAnchorAndPosition(p1, p2);
    m_movetype = MoveInclusive;
}

static bool isSign(const QChar c)
{
    return c.unicode() == '-' || c.unicode() == '+';
}

void FakeVimHandler::Private::changeNumberTextObject(bool doIncrement)
{
    QTextCursor tc = cursor();
    int pos = tc.position();
    const int n = lastPositionInDocument();
    QTextDocument *doc = document();
    QChar c = doc->characterAt(pos);
    if (!c.isNumber()) {
        if (pos == n || !isSign(c))
            return;
        ++pos;
        c = doc->characterAt(pos);
        if (!c.isNumber())
            return;
    }
    int p1 = pos;
    while (p1 >= 1 && doc->characterAt(p1 - 1).isNumber())
        --p1;
    if (p1 >= 1 && isSign(doc->characterAt(p1 - 1)))
        --p1;
    int p2 = pos;
    while (p2 <= n - 1 && doc->characterAt(p2 + 1).isNumber())
        ++p2;
    ++p2;
    setAnchorAndPosition(p2, p1);

    QString orig = selectText(currentRange());
    int value = orig.toInt();
    value = doIncrement ? value + 1 : value - 1;
    QString repl = QString::fromLatin1("%1").arg(value, orig.size(), 10, QLatin1Char('0'));
    replaceText(currentRange(), repl);
    moveLeft();
}

void FakeVimHandler::Private::selectQuotedStringTextObject(bool inner,
    const QString &quote)
{
    QTextCursor tc = cursor();
    int sz = quote.size();

    QTextCursor tc1;
    QTextCursor tc2(document());
    while (tc2 <= tc) {
        tc1 = document()->find(quote, tc2);
        if (tc1.isNull() || tc1.anchor() > tc.position())
            return;
        tc2 = document()->find(quote, tc1);
        if (tc2.isNull())
            return;
    }

    int p1 = tc1.position();
    int p2 = tc2.position();
    if (inner) {
        p2 -= sz + 1;
        if (document()->characterAt(p1) == ParagraphSeparator)
            ++p1;
    } else {
        p1 -= sz;
        p2 -= sz;
    }

    setAnchorAndPosition(p1, p2);
    m_movetype = MoveInclusive;
}

int FakeVimHandler::Private::mark(int code) const
{
    // FIXME: distinguish local and global marks.
    //qDebug() << "MARK: " << code << m_marks.value(code, -1) << m_marks;
    if (isVisualMode()) {
        if (code == '<')
            return position();
        if (code == '>')
            return anchor();
    }
    if (code == '.')
        return m_lastChangePosition;
    QTextCursor tc = m_marks.value(code);
    return tc.isNull() ? -1 : tc.position();
}

void FakeVimHandler::Private::setMark(int code, int position)
{
    // FIXME: distinguish local and global marks.
    QTextCursor tc = cursor();
    tc.setPosition(position, MoveAnchor);
    m_marks[code] = tc;
}

void FakeVimHandler::Private::setRegisterRangeMode(int reg, RangeMode mode)
{
    g.registers[reg].rangemode = mode;
}

RangeMode FakeVimHandler::Private::registerRangeMode(int reg) const
{
    bool isClipboard;
    bool isSelection;
    getRegisterType(reg, &isClipboard, &isSelection);

    // If register content is clipboard:
    //  - return RangeLineMode if text ends with new line char,
    //  - return RangeCharMode otherwise.
    if (isClipboard || isSelection) {
        QString text = QApplication::clipboard()->text(
            isClipboard ? QClipboard::Clipboard : QClipboard::Selection);
        return (text.endsWith('\n') || text.endsWith('\r')) ? RangeLineMode : RangeCharMode;
    }

    return g.registers[reg].rangemode;
}

void FakeVimHandler::Private::setRegisterContents(int reg, const QString &contents)
{
    bool copyToClipboard;
    bool copyToSelection;
    getRegisterType(reg, &copyToClipboard, &copyToSelection);

    if (copyToClipboard || copyToSelection) {
        QClipboard *clipboard = QApplication::clipboard();
        if (copyToClipboard)
            clipboard->setText(contents, QClipboard::Clipboard);
        if (copyToSelection)
            clipboard->setText(contents, QClipboard::Selection);
    } else {
        g.registers[reg].contents = contents;
    }
}

QString FakeVimHandler::Private::registerContents(int reg) const
{
    bool copyFromClipboard;
    bool copyFromSelection;
    getRegisterType(reg, &copyFromClipboard, &copyFromSelection);

    if (copyFromClipboard || copyFromSelection) {
        QClipboard *clipboard = QApplication::clipboard();
        if (copyFromClipboard)
            return clipboard->text(QClipboard::Clipboard);
        if (copyFromSelection)
            return clipboard->text(QClipboard::Selection);
    }

    return g.registers[reg].contents;
}

void FakeVimHandler::Private::getRegisterType(int reg, bool *isClipboard, bool *isSelection) const
{
    bool clipboard = false;
    bool selection = false;

    if (reg == '"') {
        QStringList list = config(ConfigClipboard).toString().split(',');
        clipboard = list.contains(QString("unnamedplus"));
        selection = list.contains(QString("unnamed"));
    } else if (reg == '+') {
        clipboard = true;
    } else if (reg == '*') {
        selection = true;
    }

    // selection (primary) is clipboard on systems without selection support
    if (selection && !QApplication::clipboard()->supportsSelection()) {
        clipboard = true;
        selection = false;
    }

    if (isClipboard != 0)
        *isClipboard = clipboard;
    if (isSelection != 0)
        *isSelection = selection;
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

    // Catch mouse events on the viewport.
    QWidget *viewport = 0;
    if (d->m_plaintextedit)
        viewport = d->m_plaintextedit->viewport();
    else if (d->m_textedit)
        viewport = d->m_textedit->viewport();
    if (ob == viewport) {
        if (active && ev->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mev = static_cast<QMouseEvent *>(ev);
            if (mev->button() == Qt::LeftButton) {
                d->importSelection();
                //return true;
            }
        }
        if (active && ev->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mev = static_cast<QMouseEvent *>(ev);
            if (mev->button() == Qt::LeftButton) {
                d->m_visualMode = NoVisualMode;
                d->updateSelection();
            }
        }
        return QObject::eventFilter(ob, ev);
    }

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
        //if (d->m_mode == InsertMode)
        //    emit completionRequested();
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

void FakeVimHandler::handleReplay(const QString &keys)
{
    d->replay(keys, 1);
}

void FakeVimHandler::handleInput(const QString &keys)
{
    Mode oldMode = d->m_mode;
    d->m_mode = CommandMode;
    Inputs inputs;
    inputs.parseFrom(keys);
    foreach (const Input &input, inputs)
        d->handleKey(input);
    d->m_mode = oldMode;
}

void FakeVimHandler::setCurrentFileName(const QString &fileName)
{
   d->m_currentFileName = fileName;
}

QString FakeVimHandler::currentFileName() const
{
    return d->m_currentFileName;
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
