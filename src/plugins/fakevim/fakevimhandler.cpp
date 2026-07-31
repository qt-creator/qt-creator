// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include "fakevimactions.h"
#include "fakevimtr.h"

#include <QDebug>
#include <QFile>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <QStack>

#include <QApplication>
#include <QClipboard>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStringConverter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextEdit>
#include <QMimeData>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

#include <algorithm>
#include <climits>
#include <ctime>
#include <functional>
#include <optional>

#ifdef FAKEVIM_STANDALONE
namespace Utils {
using PlainTextEdit = QPlainTextEdit;
}
#else
#include <utils/filepath.h>
#include <utils/plaintextedit/plaintextedit.h>
#endif
#include <utils/hostosinfo.h>

//#define DEBUG_KEY  1
#if DEBUG_KEY
#   define KEY_DEBUG(s) qDebug() << s
#else
#   define KEY_DEBUG(s)
#endif

//#define DEBUG_UNDO  1
#if DEBUG_UNDO
#   define UNDO_DEBUG(s) qDebug() << "REV" << revision() << s
#else
#   define UNDO_DEBUG(s)
#endif

namespace FakeVim::Internal {

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
#define NextBlock       QTextCursor::NextBlock

#define ParagraphSeparator QChar::ParagraphSeparator

#define EDITOR(s) (m_textedit ? m_textedit->s : m_plaintextedit ? m_plaintextedit->s : m_qcPlainTextEdit->s)

/* Clipboard MIME types used by Vim. */
static const QString vimMimeText = "_VIM_TEXT";
static const QString vimMimeTextEncoded = "_VIMENC_TEXT";

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

enum BlockInsertMode
{
    NoneBlockInsertMode,
    AppendBlockInsertMode,
    AppendToEndOfLineBlockInsertMode,
    InsertBlockInsertMode,
    ChangeBlockInsertMode
};

/*! A \e SubMode is used for things that require one more data item
    and are 'nested' behind a \l Mode.
*/
enum SubMode
{
    NoSubMode,
    ChangeSubMode,              // Used for c
    DeleteSubMode,              // Used for d
    ExchangeSubMode,            // Used for cx
    DeleteSurroundingSubMode,   // Used for ds
    ChangeSurroundingSubMode,   // Used for cs
    AddSurroundingSubMode,      // Used for ys
    FilterSubMode,              // Used for !
    IndentSubMode,              // Used for =
    RegisterSubMode,            // Used for "
    ShiftLeftSubMode,           // Used for <
    ShiftRightSubMode,          // Used for >
    CommentSubMode,             // Used for gc
    ReplaceWithRegisterSubMode, // Used for gr
    InvertCaseSubMode,          // Used for g~
    DownCaseSubMode,            // Used for gu
    UpCaseSubMode,              // Used for gU
    ReflowSubMode,              // Used for gq
    ReflowKeepCursorSubMode,    // Used for gw
    OperatorFuncSubMode,        // Used for g@
    WindowSubMode,              // Used for Ctrl-w
    YankSubMode,                // Used for y
    ZSubMode,                   // Used for z
    CapitalZSubMode,            // Used for Z
    ReplaceSubMode,             // Used for r
    MacroRecordSubMode,         // Used for q
    MacroExecuteSubMode,        // Used for @
    CtrlVSubMode,               // Used for Ctrl-v in insert mode
    CtrlRSubMode                // Used for Ctrl-r in insert mode
};

/*! A \e SubSubMode is used for things that require one more data item
    and are 'nested' behind a \l SubMode.
*/
enum SubSubMode
{
    NoSubSubMode,
    FtSubSubMode,                   // Used for f, F, t, T.
    MarkSubSubMode,                 // Used for m.
    BackTickSubSubMode,             // Used for `.
    TickSubSubMode,                 // Used for '.
    TextObjectSubSubMode,           // Used for thing like iw, aW, as etc.
    ZSubSubMode,                    // Used for zj, zk
    OpenSquareSubSubMode,           // Used for [{, {(, [z
    CloseSquareSubSubMode,          // Used for ]}, ]), ]z
    SearchSubSubMode,               // Used for /, ?
    SurroundSubSubMode,             // Used for cs, ds, ys
    SurroundWithFunctionSubSubMode, // Used for ys{motion}f
    CtrlVUnicodeSubSubMode,         // Used for Ctrl-v based unicode input
    ExpressionSubSubMode            // Used for the "=" expression register (CTRL-R =)
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
    \value RangeLineModeExclusive Like \l RangeLineMode, but keeps one
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
    EventCancelled, // Event is handled but a sub mode was cancelled.
    EventPassedToCore
};

struct CursorPosition
{
    CursorPosition() = default;
    CursorPosition(int block, int column) : line(block), column(column) {}
    explicit CursorPosition(const QTextCursor &tc)
        : line(tc.block().blockNumber()), column(tc.positionInBlock()) {}
    CursorPosition(const QTextDocument *document, int position)
    {
        QTextBlock block = document->findBlock(position);
        line = block.blockNumber();
        column = position - block.position();
    }
    bool isValid() const { return line >= 0 && column >= 0; }
    bool operator>(const CursorPosition &other) const
        { return line > other.line || column > other.column; }
    bool operator==(const CursorPosition &other) const
        { return line == other.line && column == other.column; }
    bool operator!=(const CursorPosition &other) const { return !operator==(other); }

    int line = -1; // Line in document (from 0, folded lines included).
    int column = -1; // Position on line.
};

[[maybe_unused]] static QDebug operator<<(QDebug ts, const CursorPosition &pos)
{
    return ts << "(line: " << pos.line << ", column: " << pos.column << ")";
}

// vi style configuration

class Mark
{
public:
    Mark(const CursorPosition &pos = CursorPosition(), const QString &fileName = QString())
        : m_position(pos), m_fileName(fileName) {}

    bool isValid() const { return m_position.isValid(); }

    bool isLocal(const QString &localFileName) const
    {
        return m_fileName.isEmpty() || m_fileName == localFileName;
    }

    /* Return position of mark within given document.
     * If saved line number is too big, mark position is at the end of document.
     * If line number is in document but column is too big, mark position is at the end of line.
     */
    CursorPosition position(const QTextDocument *document) const
    {
        QTextBlock block = document->findBlockByNumber(m_position.line);
        CursorPosition pos;
        if (block.isValid()) {
            pos.line = m_position.line;
            pos.column = qMax(0, qMin(m_position.column, block.length() - 2));
        } else if (document->isEmpty()) {
            pos.line = 0;
            pos.column = 0;
        } else {
            pos.line = document->blockCount() - 1;
            pos.column = qMax(0, document->lastBlock().length() - 2);
        }
        return pos;
    }

    const QString &fileName() const { return m_fileName; }

    void setFileName(const QString &fileName) { m_fileName = fileName; }

private:
    CursorPosition m_position;
    QString m_fileName;
};
using Marks = QHash<QChar, Mark>;

struct State
{
    State() = default;
    State(int revision, const CursorPosition &position, const Marks &marks,
        VisualMode lastVisualMode, bool lastVisualModeInverted) : revision(revision),
        position(position), marks(marks), lastVisualMode(lastVisualMode),
        lastVisualModeInverted(lastVisualModeInverted) {}

    bool isValid() const { return position.isValid(); }

    int revision = -1;
    CursorPosition position;
    Marks marks;
    VisualMode lastVisualMode = NoVisualMode;
    bool lastVisualModeInverted = false;
};

struct Column
{
    Column(int p, int l) : physical(p), logical(l) {}
    int physical; // Number of characters in the data.
    int logical; // Column on screen.
};

[[maybe_unused]] static QDebug operator<<(QDebug ts, const Column &col)
{
    return ts << "(p: " << col.physical << ", l: " << col.logical << ")";
}

struct Register
{
    Register() = default;
    Register(const QString &c) : contents(c) {}
    Register(const QString &c, RangeMode m) : contents(c), rangemode(m) {}
    QString contents;
    RangeMode rangemode = RangeCharMode;
};

[[maybe_unused]] static QDebug operator<<(QDebug ts, const Register &reg)
{
    return ts << reg.contents;
}

struct SearchData
{
    QString needle;
    bool forward = true;
    bool highlightMatches = true;
};

static QString replaceTildeWithHome(QString str)
{
#ifdef FAKEVIM_STANDALONE
    // Standalone FakeVim has no Utils::FilePath; expand only a leading "~".
    // A tilde anywhere else (e.g. "/tmp/~foo") is a literal character.
    if (str == "~" || str.startsWith("~/"))
        str.replace(0, 1, QDir::homePath());
    return str;
#else
    // Reuse the canonical expansion, which also handles the "~user" form.
    return Utils::FilePath::fromUserInput(str).toFSPathString();
#endif
}

// If string begins with given prefix remove it with trailing spaces and return true.
static bool eatString(const QString &prefix, QString *str)
{
    if (!str->startsWith(prefix))
        return false;
    *str = str->mid(prefix.size()).trimmed();
    return true;
}

static QRegularExpression vimPatternToQtPattern(const QString &needle)
{
    /* Transformations (Vim regexp -> QRegularExpression):
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

    // FIXME: Option smartcase should be used only if search was typed by user.
    const bool smartCaseOption = settings().smartCase();
    static const QRegularExpression regexp("[A-Z]");
    const bool initialIgnoreCase = settings().ignoreCase()
        && !(smartCaseOption && needle.contains(regexp));

    bool ignorecase = initialIgnoreCase;

    QString pattern;
    pattern.reserve(2 * needle.size());

    bool escape = false;
    bool brace = false;
    bool embraced = false;
    bool range = false;
    bool curly = false;
    bool zmark = false; // saw "\z", waiting for the "s" or "e"
    bool lookahead = false; // a "\ze" opened a "(?=" that has to be closed
    // How much punctuation carries meaning, set by "\v", "\m", "\M" and "\V".
    // Very magic is close to what QRegularExpression already reads, magic is
    // Vim's default and wants a backslash on "(){}+|?", and the nomagic levels
    // take more and more of it literally.
    enum MagicLevel { VeryMagic, Magic, NoMagic, VeryNoMagic };
    MagicLevel magic = Magic;
    bool percent = false; // saw "%" in very magic, waiting for the "("
    for (const QChar &c : needle) {
        if (zmark) {
            zmark = false;
            if (c == 's') { // \zs: the match starts here
                pattern.append("\\K");
                continue;
            }
            if (c == 'e') { // \ze: the match ends here
                pattern.append("(?=");
                lookahead = true;
                continue;
            }
            pattern.append("\\z"); // not \zs or \ze, keep it and handle c below
        }
        if (percent) {
            percent = false;
            if (c == '(') { // "%(" groups without capturing
                pattern.append("(?:");
                continue;
            }
            pattern.append("%"); // a "%" of its own, then handle c below
        }
        if (brace) {
            brace = false;
            if (c == ']') {
                pattern.append("\\[\\]");
                continue;
            }
            pattern.append('[');
            escape = true;
            embraced = true;
        }
        if (embraced) {
            if (range) {
                QChar c2 = pattern[pattern.size() - 2];
                pattern.remove(pattern.size() - 2, 2);
                pattern.append(c2.toUpper() + '-' + c.toUpper());
                pattern.append(c2.toLower() + '-' + c.toLower());
                range = false;
            } else if (escape) {
                escape = false;
                pattern.append(c);
            } else if (c == '\\') {
                escape = true;
            } else if (c == ']') {
                pattern.append(']');
                embraced = false;
            } else if (c == '-') {
                range = ignorecase && pattern[pattern.size() - 1].isLetter();
                pattern.append('-');
            } else if (c.isLetter() && ignorecase) {
                pattern.append(c.toLower()).append(c.toUpper());
            } else {
                pattern.append(c);
            }
        } else if (QString("(){}+|?").indexOf(c) != -1) {
            // Magic wants a backslash on these to make them mean something,
            // very magic wants one to take them literally, and the nomagic
            // levels never give them a meaning.
            bool special;
            if (magic == VeryMagic) {
                special = !escape;
                if (c == '{')
                    curly = special;
                else if (c == '}' && curly)
                    curly = false;
            } else if (magic == Magic) {
                if (c == '{') {
                    curly = escape;
                } else if (c == '}' && curly) {
                    curly = false;
                    escape = true;
                }
                special = escape;
            } else {
                special = false;
            }
            escape = false;
            if (!special)
                pattern.append('\\');
            pattern.append(c);
        } else if (escape) {
            // escape expression
            escape = false;
            if (c == '<' || c == '>')
                pattern.append("\\b");
            else if (c == 'a')
                pattern.append("[a-zA-Z]");
            else if (c == 'A')
                pattern.append("[^a-zA-Z]");
            else if (c == 'h')
                pattern.append("[A-Za-z_]");
            else if (c == 'H')
                pattern.append("[^A-Za-z_]");
            else if (c == 'c' || c == 'C')
                ignorecase = (c == 'c');
            else if (c == 'l')
                pattern.append("[a-z]");
            else if (c == 'L')
                pattern.append("[^a-z]");
            else if (c == 'o')
                pattern.append("[0-7]");
            else if (c == 'O')
                pattern.append("[^0-7]");
            else if (c == 'u')
                pattern.append("[A-Z]");
            else if (c == 'U')
                pattern.append("[^A-Z]");
            else if (c == 'x')
                pattern.append("[0-9A-Fa-f]");
            else if (c == 'X')
                pattern.append("[^0-9A-Fa-f]");
            // The uppercase form of these is the same class without the
            // digits, not its negation.
            else if (c == 'i' || c == 'k') // identifier, keyword
                pattern.append("[0-9A-Za-z_]");
            else if (c == 'I' || c == 'K')
                pattern.append("[A-Za-z_]");
            else if (c == 'f') // file name
                pattern.append("[0-9A-Za-z_/.,+=~$%#-]");
            else if (c == 'F')
                pattern.append("[A-Za-z_/.,+=~$%#-]");
            else if (c == 'p') // printable
                pattern.append("[[:print:]]");
            else if (c == 'P') // no set intersection here, so spell it out
                pattern.append("[^\\x00-\\x1f\\x7f0-9]");
            else if (c == '=')
                pattern.append("?");
            else if (c == 'z')
                zmark = true;
            else if (c == 'v')
                magic = VeryMagic;
            else if (c == 'm')
                magic = Magic;
            else if (c == 'M')
                magic = NoMagic;
            else if (c == 'V')
                magic = VeryNoMagic;
            else {
                pattern.append('\\');
                pattern.append(c);
            }
        } else {
            // unescaped expression
            if (c == '\\')
                escape = true;
            else if (magic == VeryMagic && c == '%')
                percent = true; // "%(" is a group that does not capture
            else if (magic == VeryMagic && (c == '<' || c == '>'))
                pattern.append("\\b");
            else if (magic == VeryMagic && c == '=')
                pattern.append('?');
            else if (c == '[' && magic != VeryNoMagic)
                brace = true;
            else if (c.isLetter() && ignorecase)
                pattern.append('[').append(c.toLower()).append(c.toUpper()).append(']');
            else if (magic >= NoMagic && QString(".*[]~").indexOf(c) != -1)
                pattern.append('\\').append(c); // no meaning at these levels
            else
                pattern.append(c);
        }
    }
    if (escape)
        pattern.append('\\');
    else if (brace)
        pattern.append("\\["); // a trailing "[" opens no class; Vim takes it literally
    if (lookahead)
        pattern.append(')');

    return QRegularExpression(pattern, initialIgnoreCase ? QRegularExpression::CaseInsensitiveOption
                                                         : QRegularExpression::NoPatternOption);
}

static bool afterEndOfLine(const QTextDocument *doc, int position)
{
    return doc->characterAt(position) == ParagraphSeparator
        && doc->findBlock(position).length() > 1;
}

static void searchForward(QTextCursor *tc, const QRegularExpression &needleExp, int *repeat)
{
    const QTextDocument *doc = tc->document();
    const int startPos = tc->position();

    QTextDocument::FindFlags flags = {};
    if (!(needleExp.patternOptions() & QRegularExpression::CaseInsensitiveOption))
        flags |= QTextDocument::FindCaseSensitively;

    // Search from beginning of line so that matched text is the same.
    tc->movePosition(StartOfLine);

    // forward to current position
    *tc = doc->find(needleExp, *tc, flags);
    while (!tc->isNull() && tc->anchor() < startPos) {
        if (!tc->hasSelection())
            tc->movePosition(Right);
        if (tc->atBlockEnd())
            tc->movePosition(NextBlock);
        *tc = doc->find(needleExp, *tc, flags);
    }

    if (tc->isNull())
        return;

    --*repeat;

    while (*repeat > 0) {
        if (!tc->hasSelection())
            tc->movePosition(Right);
        if (tc->atBlockEnd())
            tc->movePosition(NextBlock);
        *tc = doc->find(needleExp, *tc, flags);
        if (tc->isNull())
            return;
        --*repeat;
    }

    if (!tc->isNull() && afterEndOfLine(doc, tc->anchor()))
        tc->movePosition(Left);
}

static void searchBackward(QTextCursor *tc, const QRegularExpression &needleExp, int *repeat)
{
    // Search from beginning of line so that matched text is the same.
    QTextBlock block = tc->block();
    QString line = block.text();

    QRegularExpressionMatch match;
    int i = line.indexOf(needleExp, 0, &match);
    while (i != -1 && i < tc->positionInBlock()) {
        --*repeat;
        const int offset = i + qMax(1, match.capturedLength());
        i = line.indexOf(needleExp, offset, &match);
        if (i == line.size())
            i = -1;
    }

    if (i == tc->positionInBlock())
        --*repeat;

    while (*repeat > 0) {
        block = block.previous();
        if (!block.isValid())
            break;
        line = block.text();
        i = line.indexOf(needleExp, 0, &match);
        while (i != -1) {
            --*repeat;
            const int offset = i + qMax(1, match.capturedLength());
            i = line.indexOf(needleExp, offset, &match);
            if (i == line.size())
                i = -1;
        }
    }

    if (!block.isValid()) {
        *tc = QTextCursor();
        return;
    }

    i = line.indexOf(needleExp, 0, &match);
    while (*repeat < 0) {
        const int offset = i + qMax(1, match.capturedLength());
        i = line.indexOf(needleExp, offset, &match);
        ++*repeat;
    }
    tc->setPosition(block.position() + i);
    tc->setPosition(tc->position() + match.capturedLength(), KeepAnchor);
}

// Commands [[, []
static void bracketSearchBackward(QTextCursor *tc, const QString &needleExp, int repeat)
{
    const QRegularExpression re(needleExp);
    QTextCursor tc2 = *tc;
    tc2.setPosition(tc2.position() - 1);
    searchBackward(&tc2, re, &repeat);
    if (repeat <= 1)
        tc->setPosition(tc2.isNull() ? 0 : tc2.position(), KeepAnchor);
}

// Commands ][, ]]
// When ]] is used after an operator, then also stops below a '}' in the first column.
static void bracketSearchForward(QTextCursor *tc, const QString &needleExp, int repeat,
                                 bool searchWithCommand)
{
    static const QRegularExpression reWithCommand("^\\}|^\\{");
    const QRegularExpression reNeedle(needleExp);
    QTextCursor tc2 = *tc;
    tc2.setPosition(tc2.position() + 1);
    searchForward(&tc2, searchWithCommand ? reWithCommand : reNeedle, &repeat);
    if (repeat <= 1) {
        if (tc2.isNull()) {
            tc->setPosition(tc->document()->characterCount() - 1, KeepAnchor);
        } else {
            tc->setPosition(tc2.position() - 1, KeepAnchor);
            if (searchWithCommand && tc->document()->characterAt(tc->position()).unicode() == '}') {
                QTextBlock block = tc->block().next();
                if (block.isValid())
                    tc->setPosition(block.position(), KeepAnchor);
            }
        }
    }
}

static char backslashed(char t)
{
    switch (t) {
        case 'e': return 27;
        case 't': return '\t';
        case 'r': return '\r';
        case 'n': return '\n';
        case 'b': return 8;
    }
    return t;
}

enum class Modifier {NONE,UPPERCASE,LOWERCASE};

static QString applyReplacementLetterCases(QString repl,
                                           Modifier &toggledModifier,
                                           Modifier &nextCharacterModifier)
{
    if (toggledModifier == Modifier::UPPERCASE)
        repl = repl.toUpper();
    else if (toggledModifier == Modifier::LOWERCASE)
        repl = repl.toLower();

    if (nextCharacterModifier == Modifier::UPPERCASE) {
        repl.replace(0, 1, repl.at(0).toUpper());
        nextCharacterModifier = Modifier::NONE;
    } else if (nextCharacterModifier == Modifier::LOWERCASE) {
        repl.replace(0, 1, repl.at(0).toLower());
        nextCharacterModifier = Modifier::NONE;
    }
    return repl;
}

static QChar applyReplacementLetterCases(QChar repl,
                                         Modifier &toggledModifier,
                                         Modifier &nextCharacterModifier)
{
    if (nextCharacterModifier == Modifier::UPPERCASE){
        nextCharacterModifier = Modifier::NONE;
        return repl.toUpper();
    }
    else if (nextCharacterModifier == Modifier::LOWERCASE) {
        nextCharacterModifier = Modifier::NONE;
        return repl.toLower();
    }
    else if (toggledModifier == Modifier::UPPERCASE)
        return repl.toUpper();
    else if (toggledModifier == Modifier::LOWERCASE)
        return repl.toLower();
    else
        return repl;
}

static bool substituteText(QString *text,
                           const QRegularExpression &pattern,
                           const QString &replacement,
                           bool global)
{
    bool substituted = false;
    int pos = 0;
    int right = -1;
    while (true) {
        const QRegularExpressionMatch match = pattern.match(*text, pos);
        if (!match.hasMatch())
            break;

        pos = match.capturedStart();

        // ensure that substitution is advancing towards end of line
        if (right == text->size() - pos) {
            ++pos;
            if (pos == text->size())
                break;
            continue;
        }

        right = text->size() - pos;

        substituted = true;
        QString matched = text->mid(pos, match.captured(0).size());
        QString repl;
        bool escape = false;
        Modifier toggledModifier = Modifier::NONE;
        Modifier nextCharacterModifier = Modifier::NONE;
        // insert captured texts
        for (int i = 0; i < replacement.size(); ++i) {
            const QChar &c = replacement[i];
            if (escape) {
                escape = false;
                if (c.isDigit()) {
                    if (c.digitValue() <= match.lastCapturedIndex()) {
                        repl += applyReplacementLetterCases(match.captured(c.digitValue()),
                                                            toggledModifier,
                                                            nextCharacterModifier);

                    }
                } else if (c == 'u') {
                    nextCharacterModifier = Modifier::UPPERCASE;
                } else if (c == 'l') {
                    nextCharacterModifier = Modifier::LOWERCASE;
                } else if (c == 'U') {
                    toggledModifier = Modifier::UPPERCASE;
                } else if (c == 'L') {
                    toggledModifier = Modifier::LOWERCASE;
                } else if (c == 'e' || c == 'E') {
                    nextCharacterModifier = Modifier::NONE;
                    toggledModifier = Modifier::NONE;
                } else {
                    repl += backslashed(c.unicode());
                }
            } else {
                if (c == '\\')
                    escape = true;
                else if (c == '&')
                    repl += applyReplacementLetterCases(match.captured(0),
                                                        toggledModifier,
                                                        nextCharacterModifier);
                else
                    repl += applyReplacementLetterCases(c, toggledModifier, nextCharacterModifier);
            }
        }
        text->replace(pos, matched.size(), repl);
        pos += (repl.isEmpty() && matched.isEmpty()) ? 1 : repl.size();

        if (pos >= text->size() || !global)
            break;
    }

    return substituted;
}

static int findUnescaped(QChar c, const QString &line, int from)
{
    bool singleBackSlashBefore = false;
    for (int i = from; i < line.size(); ++i) {
        const QChar currentChar = line.at(i);
        if (currentChar == '\\') {
           singleBackSlashBefore = !singleBackSlashBefore;
           continue;
        }

        if (currentChar == c && !singleBackSlashBefore)
            return i;

        singleBackSlashBefore = false;
    }
    return -1;
}

static void setClipboardData(const QString &content, RangeMode mode,
    QClipboard::Mode clipboardMode)
{
    QClipboard *clipboard = QApplication::clipboard();
    char vimRangeMode = mode;

    QByteArray bytes1;
    bytes1.append(vimRangeMode);
    bytes1.append(content.toUtf8());

    QByteArray bytes2;
    bytes2.append(vimRangeMode);
    bytes2.append("utf-8");
    bytes2.append('\0');
    bytes2.append(content.toUtf8());

    auto data = new QMimeData;
    data->setText(content);
    data->setData(vimMimeText, bytes1);
    data->setData(vimMimeTextEncoded, bytes2);
    clipboard->setMimeData(data, clipboardMode);
}

static const QMap<QString, int> &vimKeyNames()
{
    static const QMap<QString, int> k = {
        // FIXME: Should be value of mapleader.
        {"LEADER", Key_Backslash},

        {"SPACE", Key_Space},
        {"TAB", Key_Tab},
        {"NL", Key_Return},
        {"NEWLINE", Key_Return},
        {"LINEFEED", Key_Return},
        {"LF", Key_Return},
        {"CR", Key_Return},
        {"RETURN", Key_Return},
        {"ENTER", Key_Return},
        {"BS", Key_Backspace},
        {"BACKSPACE", Key_Backspace},
        {"ESC", Key_Escape},
        {"BAR", Key_Bar},
        {"BSLASH", Key_Backslash},
        {"DEL", Key_Delete},
        {"DELETE", Key_Delete},
        {"KDEL", Key_Delete},
        {"UP", Key_Up},
        {"DOWN", Key_Down},
        {"LEFT", Key_Left},
        {"RIGHT", Key_Right},

        {"LT", Key_Less},
        {"GT", Key_Greater},

        {"F1", Key_F1},
        {"F2", Key_F2},
        {"F3", Key_F3},
        {"F4", Key_F4},
        {"F5", Key_F5},
        {"F6", Key_F6},
        {"F7", Key_F7},
        {"F8", Key_F8},
        {"F9", Key_F9},
        {"F10", Key_F10},

        {"F11", Key_F11},
        {"F12", Key_F12},
        {"F13", Key_F13},
        {"F14", Key_F14},
        {"F15", Key_F15},
        {"F16", Key_F16},
        {"F17", Key_F17},
        {"F18", Key_F18},
        {"F19", Key_F19},
        {"F20", Key_F20},

        {"F21", Key_F21},
        {"F22", Key_F22},
        {"F23", Key_F23},
        {"F24", Key_F24},
        {"F25", Key_F25},
        {"F26", Key_F26},
        {"F27", Key_F27},
        {"F28", Key_F28},
        {"F29", Key_F29},
        {"F30", Key_F30},

        {"F31", Key_F31},
        {"F32", Key_F32},
        {"F33", Key_F33},
        {"F34", Key_F34},
        {"F35", Key_F35},

        {"INSERT", Key_Insert},
        {"INS", Key_Insert},
        {"KINSERT", Key_Insert},
        {"HOME", Key_Home},
        {"END", Key_End},
        {"PAGEUP", Key_PageUp},
        {"PAGEDOWN", Key_PageDown},

        {"KPLUS", Key_Plus},
        {"KMINUS", Key_Minus},
        {"KDIVIDE", Key_Slash},
        {"KMULTIPLY", Key_Asterisk},
        {"KENTER", Key_Enter},
        {"KPOINT", Key_Period},

        {"CAPS", Key_CapsLock},
        {"NUM", Key_NumLock},
        {"SCROLL", Key_ScrollLock},
        {"ALTGR", Key_AltGr}
    };

    return k;
}

static bool isOnlyControlModifier(const Qt::KeyboardModifiers &mods)
{
    return (mods ^ Utils::HostOsInfo::controlModifier()) == Qt::NoModifier;
}

static bool isAcceptableModifier(const Qt::KeyboardModifiers &mods)
{
    if (Utils::HostOsInfo::isMacHost() && (mods & Qt::ControlModifier)) {
        // We want to have Cmd+S as save and not as 's' action
        // See QTCREATORBUG-13392
        return false;
    }

    if (mods & Utils::HostOsInfo::controlModifier()) {
        // Generally, CTRL is not fine, except in combination with ALT.
        // See QTCREATORBUG-24673
        return mods & AltModifier;
    }
    return true;
}


Range::Range(int b, int e, RangeMode m)
    : beginPos(qMin(b, e)), endPos(qMax(b, e)), rangemode(m)
{}

bool Range::isValid() const
{
    return beginPos >= 0 && endPos >= 0;
}

static QDebug operator<<(QDebug ts, const Range &range)
{
    return ts << '[' << range.beginPos << ',' << range.endPos << ']';
}


bool ExCommand::matches(const QString &min, const QString &full) const
{
    return cmd.startsWith(min) && full.startsWith(cmd);
}

static QDebug operator<<(QDebug ts, const ExCommand &cmd)
{
    return ts << cmd.cmd << ' ' << cmd.args << ' ' << cmd.range;
}

static QString quoteUnprintable(const QString &ba)
{
    QString res;
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const QChar c = ba.at(i);
        const int cc = c.unicode();
        if (c.isPrint())
            res += c;
        else if (cc == '\n')
            res += "<CR>";
        else
            res += QString("\\x%1").arg(int16_t(c.unicode()), 2, 16, QLatin1Char('0'));
    }
    return res;
}

static bool startsWithWhitespace(const QString &str, int col)
{
    if (col > str.size()) {
        qWarning("Wrong column");
        return false;
    }
    for (int i = 0; i < col; ++i) {
        uint u = str.at(i).unicode();
        if (u != ' ' && u != '\t')
            return false;
    }
    return true;
}

inline QString msgMarkNotSet(const QString &text)
{
    return Tr::tr("Mark \"%1\" not set.").arg(text);
}

class Input
{
public:
    // Remove some extra "information" on Mac.
    static Qt::KeyboardModifiers cleanModifier(Qt::KeyboardModifiers m)
    {
        return m & ~Qt::KeypadModifier;
    }

    Input() = default;
    explicit Input(QChar x)
        : m_key(x.unicode()), m_xkey(x.unicode()), m_text(x)
    {
        if (x.isUpper())
            m_modifiers = Qt::ShiftModifier;
        else if (x.isLower())
            m_key = x.toUpper().unicode();
    }

    Input(int k, Qt::KeyboardModifiers m, const QString &t = QString())
        : m_key(k), m_modifiers(cleanModifier(m)), m_text(t)
    {
        if (m_text.size() == 1) {
            QChar x = m_text.at(0);

            // On Mac, QKeyEvent::text() returns non-empty strings for
            // cursor keys. This breaks some of the logic later on
            // relying on text() being empty for "special" keys.
            // FIXME: Check the real conditions.
            if (x.unicode() < ' ' && x.unicode() != 27)
                m_text.clear();
            else if (x.isLetter())
                m_key = x.toUpper().unicode();
        }

        // Set text only if input is ascii key without control modifier.
        if (m_text.isEmpty() && k >= 0 && k <= 0x7f && (m & Utils::HostOsInfo::controlModifier()) == 0) {
            QChar c = QChar(k);
            if (c.isLetter())
                m_text = isShift() ? c.toUpper() : c;
            else if (!isShift())
                m_text = c;
        }

        // Normalize <S-TAB>.
        if (m_key == Qt::Key_Backtab) {
            m_key = Qt::Key_Tab;
            m_modifiers |= Qt::ShiftModifier;
        }

        // m_xkey is only a cache.
        m_xkey = (m_text.size() == 1 ? m_text.at(0).unicode() : m_key);
    }

    bool isValid() const
    {
        return m_key != 0 || !m_text.isNull();
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
        return m_key == '\n' || m_key == Key_Return || m_key == Key_Enter;
    }

    bool isEscape() const
    {
        return isKey(Key_Escape) || isShift(Key_Escape) || isKey(27) || isShift(27) || isControl('c')
            || isControl(Key_BracketLeft);
    }

    bool is(int c) const
    {
        return m_xkey == c && isAcceptableModifier(m_modifiers);
    }

    bool isControl() const
    {
        return isOnlyControlModifier(m_modifiers);
    }

    bool isControl(int c) const
    {
        return isControl()
            && (m_xkey == c || m_xkey + 32 == c || m_xkey + 64 == c || m_xkey + 96 == c);
    }

    bool isShift() const
    {
        return m_modifiers & Qt::ShiftModifier;
    }

    bool isShift(int c) const
    {
        return isShift() && m_xkey == c;
    }

    bool operator<(const Input &a) const
    {
        if (m_key != a.m_key)
            return m_key < a.m_key;
        // Text for some mapped key cannot be determined (e.g. <C-J>) so if text is not set for
        // one of compared keys ignore it.
        if (!m_text.isEmpty() && !a.m_text.isEmpty() && m_text != " ")
            return m_text < a.m_text;
        return m_modifiers < a.m_modifiers;
    }

    bool operator==(const Input &a) const
    {
        return !(*this < a || a < *this);
    }

    bool operator!=(const Input &a) const { return !operator==(a); }

    QString text() const { return m_text; }

    QChar asChar() const
    {
        return (m_text.size() == 1 ? m_text.at(0) : QChar());
    }

    int toInt(bool *ok, int base) const
    {
        const int uc = asChar().unicode();
        int res;
        if ('0' <= uc && uc <= '9')
            res = uc -'0';
        else if ('a' <= uc && uc <= 'z')
            res = 10 + uc - 'a';
        else if ('A' <= uc && uc <= 'Z')
            res = 10 + uc - 'A';
        else
            res = base;
        *ok = res < base;
        return *ok ? res : 0;
    }

    int key() const { return m_key; }

    Qt::KeyboardModifiers modifiers() const { return m_modifiers; }

    // Return raw character for macro recording or dot command.
    QChar raw() const
    {
        if (m_key == Key_Tab)
            return '\t';
        if (m_key == Key_Return)
            return '\n';
        if (m_key == Key_Escape)
            return QChar(27);
        return QChar(m_xkey & 0xffff); // FIXME
    }

    QString toString() const
    {
        if (!m_text.isEmpty())
            return QString(m_text).replace("<", "<LT>");

        QString key = vimKeyNames().key(m_key);
        bool namedKey = !key.isEmpty();

        if (!namedKey) {
            if (m_xkey == '<')
                key = "<LT>";
            else if (m_xkey == '>')
                key = "<GT>";
            else
                key = QChar(m_xkey & 0xffff);  // FIXME
        }

        bool shift = isShift();
        bool ctrl = isControl();
        if (shift)
            key.prepend("S-");
        if (ctrl)
            key.prepend("C-");

        if (namedKey || shift || ctrl) {
            key.prepend('<');
            key.append('>');
        }

        return key;
    }

    QDebug dump(QDebug ts) const
    {
        return ts << m_key << '-' << m_modifiers << '-'
            << quoteUnprintable(m_text);
    }

    friend size_t qHash(const Input &i)
    {
        return ::qHash(i.m_key);
    }

private:
    int m_key = 0;
    int m_xkey = 0;
    Qt::KeyboardModifiers m_modifiers = NoModifier;
    QString m_text;
};

// mapping to <Nop> (do nothing)
static const Input Nop(-1, Qt::KeyboardModifiers(-1), QString());
// mapping to <PASS> (hand the key to Qt Creator) (QTCREATORBUG-14413)
static const Input Pass(-2, Qt::KeyboardModifiers(-1), QString());

static SubMode letterCaseModeFromInput(const Input &input)
{
    if (input.is('~'))
        return InvertCaseSubMode;
    if (input.is('u'))
        return DownCaseSubMode;
    if (input.is('U'))
        return UpCaseSubMode;

    return NoSubMode;
}

static SubMode indentModeFromInput(const Input &input)
{
    if (input.is('<'))
        return ShiftLeftSubMode;
    if (input.is('>'))
        return ShiftRightSubMode;
    if (input.is('='))
        return IndentSubMode;

    return NoSubMode;
}

static SubMode changeDeleteYankModeFromInput(const Input &input)
{
    if (input.is('c'))
        return ChangeSubMode;
    if (input.is('d'))
        return DeleteSubMode;
    if (input.is('y'))
        return YankSubMode;

    return NoSubMode;
}

static QString dotCommandFromSubMode(SubMode submode)
{
    if (submode == ChangeSubMode)
        return QLatin1String("c");
    if (submode == DeleteSubMode)
        return QLatin1String("d");
    if (submode == CommentSubMode)
        return QLatin1String("gc");
    if (submode == DeleteSurroundingSubMode)
        return QLatin1String("ds");
    if (submode == ChangeSurroundingSubMode)
        return QLatin1String("c");
    if (submode == AddSurroundingSubMode)
        return QLatin1String("y");
    if (submode == ExchangeSubMode)
        return QLatin1String("cx");
    if (submode == ReplaceWithRegisterSubMode)
        return QLatin1String("gr");
    if (submode == InvertCaseSubMode)
        return QLatin1String("g~");
    if (submode == DownCaseSubMode)
        return QLatin1String("gu");
    if (submode == UpCaseSubMode)
        return QLatin1String("gU");
    if (submode == ReflowSubMode)
        return QLatin1String("gq");
    if (submode == ReflowKeepCursorSubMode)
        return QLatin1String("gw");
    if (submode == OperatorFuncSubMode)
        return QLatin1String("g@");
    if (submode == IndentSubMode)
        return QLatin1String("=");
    if (submode == ShiftRightSubMode)
        return QLatin1String(">");
    if (submode == ShiftLeftSubMode)
        return QLatin1String("<");

    return QString();
}

[[maybe_unused]] static QDebug operator<<(QDebug ts, const Input &input) { return input.dump(ts); }

class Inputs : public QVector<Input>
{
public:
    Inputs() = default;

    explicit Inputs(const QString &str, bool noremap = true, bool silent = false,
                    bool expression = false, bool vim9 = false)
        : m_noremap(noremap), m_silent(silent), m_expression(expression), m_vim9(vim9)
    {
        if (expression) {
            m_expr = str; // evaluated on use; not parsed as keys (:map <expr>)
            squeeze();
            return;
        }
        // "<Cmd>{command}<CR>" runs an ex command without leaving the current
        // mode, so the right-hand side is not keys either. "<ScriptCmd>" only
        // differs in which script the command belongs to, which does not matter
        // while everything shares one namespace.
        static const QRegularExpression cmdRe("^<(?:Cmd|ScriptCmd)>",
                                              QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch cmd = cmdRe.match(str);
        if (cmd.hasMatch()) {
            QString rest = str.mid(cmd.capturedLength());
            // The command ends at the <CR>; whatever follows is keys again.
            static const QRegularExpression endRe("<(?:CR|Return|Enter)>",
                                                  QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch end = endRe.match(rest);
            if (end.hasMatch()) {
                m_exCommand = rest.left(end.capturedStart());
                rest = rest.mid(end.capturedEnd());
            } else {
                m_exCommand = rest;
                rest.clear();
            }
            parseFrom(rest);
            squeeze();
            return;
        }
        parseFrom(str);
        squeeze();
    }

    // The keys of an existing sequence, without whatever else it carried.
    Inputs(const QVector<Input> &keys, bool noremap, bool silent)
        : QVector<Input>(keys), m_noremap(noremap), m_silent(silent)
    {}

    bool noremap() const { return m_noremap; }

    bool silent() const { return m_silent; }

    bool isExpression() const { return m_expression; }
    QString expression() const { return m_expr; }

    bool isExCommand() const { return !m_exCommand.isEmpty(); }
    QString exCommand() const { return m_exCommand; }
    // "<ScriptCmd>" runs in the script that defined the mapping, so a command
    // from a Vim9 script is read with Vim9 rules.
    bool vim9() const { return m_vim9; }

    // An <expr> or <Cmd> mapping may hold no parsed keys but is still a real
    // mapping.
    bool isEmpty() const
    {
        return m_expression || isExCommand() ? false : QVector<Input>::isEmpty();
    }

private:
    void parseFrom(const QString &str);

    bool m_noremap = true;
    bool m_silent = false;
    bool m_expression = false;
    bool m_vim9 = false;
    QString m_expr;
    QString m_exCommand;
};

static Input parseVimKeyName(const QString &keyName)
{
    if (keyName.size() == 1)
        return Input(keyName.at(0));

    const QStringList keys = keyName.split('-');
    const int len = keys.length();

    if (len == 1 && keys.at(0).toUpper() == "NOP")
        return Nop;

    if (len == 1 && keys.at(0).toUpper() == "PASS")
        return Pass;

    Qt::KeyboardModifiers mods = NoModifier;
    for (int i = 0; i < len - 1; ++i) {
        const QString &key = keys[i].toUpper();
        if (key == "S")
            mods |= Qt::ShiftModifier;
        else if (key == "C")
            mods |= Utils::HostOsInfo::controlModifier();
        else
            return Input();
    }

    if (!keys.isEmpty()) {
        const QString key = keys.last();
        if (key.size() == 1) {
            // simple character
            QChar c = key.at(0).toUpper();
            return Input(c.unicode(), mods);
        }

        // find key name
        QMap<QString, int>::ConstIterator it = vimKeyNames().constFind(key.toUpper());
        if (it != vimKeyNames().end())
            return Input(*it, mods);
    }

    return Input();
}

void Inputs::parseFrom(const QString &str)
{
    const int n = str.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = str.at(i);
        if (c == '<') {
            int j = str.indexOf('>', i);
            Input input;
            if (j != -1) {
                const QString key = str.mid(i+1, j - i - 1);
                if (!key.contains('<'))
                    input = parseVimKeyName(key);
            }
            if (input.isValid()) {
                append(input);
                i = j;
            } else {
                append(Input(c));
            }
        } else {
            append(Input(c));
        }
    }
}

class History
{
public:
    History() : m_items(QString()) {}
    void append(const QString &item);
    const QString &move(QStringView prefix, int skip);
    const QString &current() const { return m_items[m_index]; }
    const QStringList &items() const { return m_items; }
    void restart() { m_index = m_items.size() - 1; }

private:
    // Last item is always empty or current search prefix.
    QStringList m_items;
    int m_index = 0;
};

void History::append(const QString &item)
{
    if (item.isEmpty())
        return;
    m_items.pop_back();
    m_items.removeAll(item);
    m_items << item << QString();
    restart();
}

const QString &History::move(QStringView prefix, int skip)
{
    if (!current().startsWith(prefix))
        restart();

    if (m_items.last() != prefix)
        m_items[m_items.size() - 1] = prefix.toString();

    int i = m_index + skip;
    if (!prefix.isEmpty())
        for (; i >= 0 && i < m_items.size() && !m_items[i].startsWith(prefix); i += skip)
            ;
    if (i >= 0 && i < m_items.size())
        m_index = i;

    return current();
}

// Command line buffer with prompt (i.e. :, / or ? characters), text contents and cursor position.
class CommandBuffer
{
public:
    void setPrompt(const QChar &prompt) { m_prompt = prompt; }
    void setContents(const QString &s) { m_buffer = s; m_anchor = m_pos = s.size(); }

    void setContents(const QString &s, int pos, int anchor = -1)
    {
        m_buffer = s; m_pos = m_userPos = pos; m_anchor = anchor >= 0 ? anchor : pos;
    }

    QStringView userContents() const { return QStringView{m_buffer}.left(m_userPos); }
    const QChar &prompt() const { return m_prompt; }
    const QString &contents() const { return m_buffer; }
    bool isEmpty() const { return m_buffer.isEmpty(); }
    int cursorPos() const { return m_pos; }
    int anchorPos() const { return m_anchor; }
    bool hasSelection() const { return m_pos != m_anchor; }

    void insertChar(QChar c) { m_buffer.insert(m_pos++, c); m_anchor = m_userPos = m_pos; }
    void insertText(const QString &s)
    {
        m_buffer.insert(m_pos, s); m_anchor = m_userPos = m_pos = m_pos + s.size();
    }
    void deleteChar() { if (m_pos) m_buffer.remove(--m_pos, 1); m_anchor = m_userPos = m_pos; }

    void moveLeft() { if (m_pos) m_userPos = --m_pos; }
    void moveRight() { if (m_pos < m_buffer.size()) m_userPos = ++m_pos; }
    void moveStart() { m_userPos = m_pos = 0; }
    void moveEnd() { m_userPos = m_pos = m_buffer.size(); }

    void setHistoryAutoSave(bool autoSave) { m_historyAutoSave = autoSave; }
    bool userContentsValid() const { return m_userPos >= 0 && m_userPos <= m_buffer.size(); }
    void historyDown() { if (userContentsValid()) setContents(m_history.move(userContents(), 1)); }
    void historyUp() { if (userContentsValid()) setContents(m_history.move(userContents(), -1)); }
    const QStringList &historyItems() const { return m_history.items(); }
    void historyPush(const QString &item = QString())
    {
        m_history.append(item.isNull() ? contents() : item);
    }

    void clear()
    {
        if (m_historyAutoSave)
            historyPush();
        m_buffer.clear();
        m_anchor = m_userPos = m_pos = 0;
    }

    QString display() const
    {
        QString msg(m_prompt);
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

    void deleteSelected()
    {
        if (m_pos < m_anchor) {
            m_buffer.remove(m_pos, m_anchor - m_pos);
            m_anchor = m_pos;
        } else {
            m_buffer.remove(m_anchor, m_pos - m_anchor);
            m_pos = m_anchor;
        }
    }

    bool handleInput(const Input &input)
    {
        if (input.isShift(Key_Left)) {
            moveLeft();
        } else if (input.isShift(Key_Right)) {
            moveRight();
        } else if (input.isShift(Key_Home)) {
            moveStart();
        } else if (input.isShift(Key_End)) {
            moveEnd();
        } else if (input.isKey(Key_Left)) {
            moveLeft();
            m_anchor = m_pos;
        } else if (input.isKey(Key_Right)) {
            moveRight();
            m_anchor = m_pos;
        } else if (input.isKey(Key_Home)) {
            moveStart();
            m_anchor = m_pos;
        } else if (input.isKey(Key_End)) {
            moveEnd();
            m_anchor = m_pos;
        } else if (input.isKey(Key_Up) || input.isKey(Key_PageUp)) {
            historyUp();
        } else if (input.isKey(Key_Down) || input.isKey(Key_PageDown)) {
            historyDown();
        } else if (input.isKey(Key_Delete)) {
            if (hasSelection()) {
                deleteSelected();
            } else {
                if (m_pos < m_buffer.size())
                    m_buffer.remove(m_pos, 1);
                else
                    deleteChar();
            }
        } else if (!input.text().isEmpty()) {
            if (hasSelection())
                deleteSelected();
            insertText(input.text());
        } else {
            return false;
        }
        return true;
    }

private:
    QString m_buffer;
    QChar m_prompt;
    History m_history;
    int m_pos = 0;
    int m_anchor = 0;
    int m_userPos = 0; // last position of inserted text (for retrieving history items)
    bool m_historyAutoSave = true; // store items to history on clear()?
};

// Mappings for a specific mode (trie structure)
class ModeMapping : public QHash<Input, ModeMapping>
{
public:
    const Inputs &value() const { return m_value; }
    void setValue(const Inputs &value) { m_value = value; }
private:
    Inputs m_value;
};

// Mappings for all modes
using Mappings = QHash<char, ModeMapping>;

// Iterator for mappings
class MappingsIterator : public QVector<ModeMapping::Iterator>
{
public:
    MappingsIterator(Mappings *mappings, char mode = -1, const Inputs &inputs = Inputs())
        : m_parent(mappings)
    {
        reset(mode);
        walk(inputs);
    }

    // Reset iterator state. Keep previous mode if 0.
    void reset(char mode = 0)
    {
        clear();
        m_lastValid = -1;
        m_currentInputs.clear();
        if (mode != 0) {
            m_mode = mode;
            if (mode != -1)
                m_modeMapping = m_parent->find(mode);
        }
    }

    bool isValid() const { return !empty(); }

    // Return true if mapping can be extended.
    bool canExtend() const { return isValid() && !last()->empty(); }

    // Return true if this mapping can be used.
    bool isComplete() const { return m_lastValid != -1; }

    // Return size of current map.
    int mapLength() const { return m_lastValid + 1; }

    bool walk(const Input &input)
    {
        m_currentInputs.append(input);

        if (m_modeMapping == m_parent->end())
            return false;

        ModeMapping::Iterator it;
        if (isValid()) {
            it = last()->find(input);
            if (it == last()->end())
                return false;
        } else {
            it = m_modeMapping->find(input);
            if (it == m_modeMapping->end())
                return false;
        }

        if (!it->value().isEmpty())
            m_lastValid = size();
        append(it);

        return true;
    }

    bool walk(const Inputs &inputs)
    {
        for (const Input &input : inputs) {
            if (!walk(input))
                return false;
        }
        return true;
    }

    // Return current mapped value. Iterator must be valid.
    const Inputs &inputs() const
    {
        return at(m_lastValid)->value();
    }

    void remove()
    {
        if (isValid()) {
            if (canExtend()) {
                last()->setValue(Inputs());
            } else {
                if (size() > 1) {
                    while (last()->empty()) {
                        at(size() - 2)->erase(last());
                        pop_back();
                        if (size() == 1 || !last()->value().isEmpty())
                            break;
                    }
                    if (last()->empty() && last()->value().isEmpty())
                        m_modeMapping->erase(last());
                } else if (last()->empty() && !last()->value().isEmpty()) {
                    m_modeMapping->erase(last());
                }
            }
        }
    }

    void setInputs(const Inputs &key, const Inputs &inputs, bool unique = false)
    {
        ModeMapping *current = &(*m_parent)[m_mode];
        for (const Input &input : key)
            current = &(*current)[input];
        if (!unique || current->value().isEmpty())
            current->setValue(inputs);
    }

    const Inputs &currentInputs() const { return m_currentInputs; }

private:
    Mappings *m_parent;
    Mappings::Iterator m_modeMapping;
    int m_lastValid = -1;
    char m_mode = 0;
    Inputs m_currentInputs;
};

// state of current mapping
struct MappingState {
    MappingState() = default;
    MappingState(bool noremap, bool silent, bool editBlock)
        : noremap(noremap), silent(silent), editBlock(editBlock) {}
    bool noremap = false;
    bool silent = false;
    bool editBlock = false;
};

struct VimFunc;

// A Vimscript value: scalars (Number/Float/String), List, Dictionary and
// Funcref/lambda. Containers and funcs are held by a shared pointer, so copies
// share one instance as in Vim.
class VimValue
{
public:
    enum Type { Number, Float, String, List, Dict, Func };

    VimValue() = default;
    explicit VimValue(qlonglong n) : m_type(Number), m_number(n) {}
    explicit VimValue(double f) : m_type(Float), m_float(f) {}
    explicit VimValue(const QString &s) : m_type(String), m_string(s) {}

    static VimValue list(const QList<VimValue> &items = {})
    {
        VimValue v;
        v.m_type = List;
        v.m_list = std::make_shared<QList<VimValue>>(items);
        return v;
    }

    static VimValue dict(const QMap<QString, VimValue> &items = {})
    {
        VimValue v;
        v.m_type = Dict;
        v.m_dict = std::make_shared<QMap<QString, VimValue>>(items);
        return v;
    }

    // A named function reference; defined out-of-line (needs VimFunc).
    static VimValue func(const QString &name);
    // A lambda with captured (closure) scope; defined out-of-line.
    static VimValue lambda(const QStringList &params, const QString &body,
                           const QHash<QString, VimValue> &captured);

    Type type() const { return m_type; }
    bool isString() const { return m_type == String; }
    bool isList() const { return m_type == List; }
    bool isDict() const { return m_type == Dict; }
    bool isFunc() const { return m_type == Func; }
    QList<VimValue> *listData() const { return m_list.get(); }
    QMap<QString, VimValue> *dictData() const { return m_dict.get(); }
    VimFunc *funcData() const { return m_func.get(); }

    qlonglong toNumber() const
    {
        switch (m_type) {
        case Number: return m_number;
        case Float:  return qlonglong(m_float);
        case String: {
            // Vim uses the leading (optionally signed) decimal digits, 0 else.
            static const QRegularExpression re("^\\s*(-?\\d+)");
            const QRegularExpressionMatch m = re.match(m_string);
            return m.hasMatch() ? m.captured(1).toLongLong() : 0;
        }
        case List:
        case Dict:
        case Func:
            return 0;
        }
        return 0;
    }

    double toFloat() const
    {
        return m_type == Float ? m_float : double(toNumber());
    }

    bool toBool() const
    {
        if (m_type == Float)
            return m_float != 0;
        if (m_type == List)
            return m_list && !m_list->isEmpty();
        if (m_type == Dict)
            return m_dict && !m_dict->isEmpty();
        return toNumber() != 0;
    }

    QString toString() const
    {
        switch (m_type) {
        case Number: return QString::number(m_number);
        case Float:  return QString::number(m_float, 'g', 6);
        case String: return m_string;
        case List:
        case Dict:
        case Func:   return repr(*this);
        }
        return QString();
    }

    // Vim's string() / repr form: like toString() but strings are quoted.
    QString reprString() const { return repr(*this); }

private:
    static QString repr(const VimValue &v)
    {
        switch (v.m_type) {
        case Number: return QString::number(v.m_number);
        case Float:  return QString::number(v.m_float, 'g', 6);
        case String: return '\'' + QString(v.m_string).replace('\'', "''") + '\'';
        case List: {
            QStringList parts;
            if (v.m_list) {
                for (const VimValue &e : *v.m_list)
                    parts.append(repr(e));
            }
            return '[' + parts.join(", ") + ']';
        }
        case Dict: {
            QStringList parts;
            if (v.m_dict) {
                for (auto it = v.m_dict->cbegin(), end = v.m_dict->cend(); it != end; ++it) {
                    parts.append('\'' + QString(it.key()).replace('\'', "''")
                                 + "': " + repr(it.value()));
                }
            }
            return '{' + parts.join(", ") + '}';
        }
        case Func:
            return v.m_string; // precomputed display form, e.g. function('Foo')
        }
        return QString();
    }

    Type m_type = Number;
    qlonglong m_number = 0;
    double m_float = 0;
    QString m_string;
    std::shared_ptr<VimFunc> m_func;
    std::shared_ptr<QList<VimValue>> m_list;
    std::shared_ptr<QMap<QString, VimValue>> m_dict;
};

// A function reference: either a named function or a lambda with a captured
// (closure) scope.
struct VimFunc
{
    QString name;
    QStringList params;
    QString body;
    QHash<QString, VimValue> captured;
    bool isLambda = false;
};

VimValue VimValue::func(const QString &name)
{
    VimValue v;
    v.m_type = Func;
    v.m_func = std::make_shared<VimFunc>();
    v.m_func->name = name;
    v.m_string = "function('" + name + "')";
    return v;
}

VimValue VimValue::lambda(const QStringList &params, const QString &body,
                          const QHash<QString, VimValue> &captured)
{
    VimValue v;
    v.m_type = Func;
    v.m_func = std::make_shared<VimFunc>();
    v.m_func->isLambda = true;
    v.m_func->params = params;
    v.m_func->body = body;
    v.m_func->captured = captured;
    v.m_string = "function('<lambda>')";
    return v;
}

// Recursive-descent evaluator for Vimscript expressions; defined below.
class VimExpr;

class FakeVimHandler::Private : public QObject
{
public:
    Private(FakeVimHandler *parent, QWidget *widget);

    EventResult handleEvent(QKeyEvent *ev);
    bool wantsOverride(QKeyEvent *ev);
    bool parseExCommand(QString *line, ExCommand *cmd);
    bool parseLineRange(QString *line, ExCommand *cmd);
    int parseLineAddress(QString *cmd, bool *hasAddress = nullptr);
    void parseRangeCount(const QString &line, Range *range) const;
    void handleCommand(const QString &cmd); // Sets m_tc + handleExCommand
    void handleExCommand(const QString &cmd);

    void installEventFilter();
    void removeEventFilter();
    void passShortcuts(bool enable);
    void setupWidget();
    void restoreWidget(int tabSize);

    friend class FakeVimHandler;

    void init();
    void focus();
    void unfocus();
    void fixExternalCursor(bool focus);
    void fixExternalCursorPosition(bool focus);

    // Call before any FakeVim processing (import cursor position from editor)
    void enterFakeVim();
    // Call after any FakeVim processing
    // (if needUpdate is true, export cursor position to editor and scroll)
    void leaveFakeVim(bool needUpdate = true);
    void leaveFakeVim(EventResult eventResult);

    EventResult handleKey(const Input &input);
    EventResult handleDefaultKey(const Input &input);
    bool handleCommandBufferPaste(const Input &input);
    EventResult handleCurrentMapAsDefault();
    void prependInputs(const QVector<Input> &inputs); // Handle inputs.
    void prependMapping(const Inputs &inputs); // Handle inputs as mapping.
    bool expandCompleteMapping(); // Return false if current mapping is not complete.
    bool extendMapping(const Input &input); // Return false if no suitable mappig found.
    void endMapping();
    bool canHandleMapping();
    void clearPendingInput();
    void waitForMapping();
    EventResult stopWaitForMapping(bool hasInput);
    EventResult handleInsertOrReplaceMode(const Input &);
    void handleInsertMode(const Input &);
    void handleReplaceMode(const Input &);
    void finishInsertMode();

    EventResult handleCommandMode(const Input &);

    // return true only if input in current mode and sub-mode was correctly handled
    bool handleEscape();
    bool handleNoSubMode(const Input &);
    bool handleChangeDeleteYankSubModes(const Input &);
    void handleChangeDeleteYankSubModes();
    bool handleReplaceSubMode(const Input &);
    bool handleCommentSubMode(const Input &);
    bool handleReplaceWithRegisterSubMode(const Input &);
    bool handleExchangeSubMode(const Input &);
    bool handleDeleteChangeSurroundingSubMode(const Input &);
    bool handleAddSurroundingSubMode(const Input &);
    bool handleFilterSubMode(const Input &);
    bool handleRegisterSubMode(const Input &);
    bool handleShiftSubMode(const Input &);
    bool handleChangeCaseSubMode(const Input &);
    bool handleReflowSubMode(const Input &);
    bool handleWindowSubMode(const Input &);
    bool handleZSubMode(const Input &);
    bool handleCapitalZSubMode(const Input &);
    bool handleMacroRecordSubMode(const Input &);
    bool handleMacroExecuteSubMode(const Input &);

    bool handleCount(const Input &); // Handle count for commands (return false if input isn't count).
    bool handleMovement(const Input &);

    EventResult handleExMode(const Input &);
    EventResult handleSearchSubSubMode(const Input &);
    bool handleCommandSubSubMode(const Input &);
    // vim-unimpaired emulation for the "[x" / "]x" bracket commands.
    bool handleVimUnimpaired(bool close, const Input &input);
    void fixSelection(); // Fix selection according to current range, move and command modes.
    bool finishSearch();
    void finishMovement(const QString &dotCommandMovement = QString());

    // Returns to insert/replace mode after <C-O> command in insert mode,
    // otherwise returns to command mode.
    void leaveCurrentMode();

    // Clear data for current (possibly incomplete) command in current mode.
    // I.e. clears count, register, g flag, sub-modes etc.
    void clearCurrentMode();

    QTextCursor search(const SearchData &sd, int startPos, int count, bool showMessages);
    void search(const SearchData &sd, bool showMessages = true);
    bool searchNext(bool forward = true);
    void searchBalanced(bool forward, QChar needle, QChar other);
    void highlightMatches(const QString &needle);
    void stopIncrementalFind();
    void updateFind(bool isComplete);

    void resetCount();
    bool isInputCount(const Input &) const; // Return true if input can be used as count for commands.
    int mvCount() const { return qMax(1, g.mvcount); }
    int opCount() const { return qMax(1, g.opcount); }
    int count() const { return mvCount() * opCount(); }
    QTextBlock block() const { return m_cursor.block(); }
    int leftDist() const { return position() - block().position(); }
    int rightDist() const { return block().length() - leftDist() - (isVisualCharMode() ? 0 : 1); }
    bool atBlockStart() const { return m_cursor.atBlockStart(); }
    bool atBlockEnd() const { return m_cursor.atBlockEnd(); }
    bool atEndOfLine() const { return atBlockEnd() && block().length() > 1; }
    bool atDocumentEnd() const { return position() >= lastPositionInDocument(true); }
    bool atDocumentStart() const { return m_cursor.atStart(); }

    bool atEmptyLine(int pos) const;
    bool atEmptyLine(const QTextCursor &tc) const;
    bool atEmptyLine() const;
    bool atBoundary(bool end, bool simple, bool onlyWords = false,
        const QTextCursor &tc = QTextCursor()) const;
    bool atWordBoundary(bool end, bool simple, const QTextCursor &tc = QTextCursor()) const;
    bool atWordStart(bool simple, const QTextCursor &tc = QTextCursor()) const;
    bool atWordEnd(bool simple, const QTextCursor &tc = QTextCursor()) const;
    bool isFirstNonBlankOnLine(int pos);

    int lastPositionInDocument(bool ignoreMode = false) const; // Returns last valid position in doc.
    int firstPositionInLine(int line, bool onlyVisibleLines = true) const; // 1 based line, 0 based pos
    int lastPositionInLine(int line, bool onlyVisibleLines = true) const; // 1 based line, 0 based pos
    int lineForPosition(int pos) const;  // 1 based line, 0 based pos
    QString lineContents(int line) const; // 1 based line
    QString textAt(int from, int to) const;
    void setLineContents(int line, const QString &contents); // 1 based line
    int blockBoundary(const QString &left, const QString &right,
        bool end, int count) const; // end or start position of current code block
    int lineNumber(const QTextBlock &block) const;

    int columnAt(int pos) const;
    int blockNumberAt(int pos) const;
    QTextBlock blockAt(int pos) const;
    QTextBlock nextLine(const QTextBlock &block) const; // following line (respects wrapped parts)
    QTextBlock previousLine(const QTextBlock &block) const; // previous line (respects wrapped parts)

    int linesOnScreen() const;
    int linesInDocument() const;

    // The following use all zero-based counting.
    int cursorLineOnScreen() const;
    int cursorLine() const;
    int cursorBlockNumber() const; // "." address
    int physicalCursorColumn() const; // as stored in the data
    int logicalCursorColumn() const; // as visible on screen
    int physicalToLogicalColumn(int physical, const QString &text) const;
    int logicalToPhysicalColumn(int logical, const QString &text) const;
    // Fetches the editor tab/indent settings when useEditorTabSettings is on
    // and a handler for them is wired (QTCREATORBUG-14273).
    bool editorTabSettings(int *tabSize, int *indentSize, bool *spacesForTabs) const
    {
        if (!s.useEditorTabSettings())
            return false;
        int ts = -1;
        int is = -1;
        bool spaces = true;
        q->tabSettingsRequested(&ts, &is, &spaces);
        if (ts <= 0) // no editor settings available (e.g. standalone)
            return false;
        if (tabSize)
            *tabSize = ts;
        if (indentSize)
            *indentSize = is;
        if (spacesForTabs)
            *spacesForTabs = spaces;
        return true;
    }
    // Effective tab stop, never below 1. Guards the column arithmetic below
    // against a stray "tabstop=0" (e.g. from a hand-edited settings file)
    // dividing by zero (QTCREATORBUG-29376).
    int tabStop() const
    {
        int ts;
        if (editorTabSettings(&ts, nullptr, nullptr))
            return qMax(1, ts);
        return qMax(1, s.tabStop());
    }
    int shiftWidth() const
    {
        int sw;
        if (editorTabSettings(nullptr, &sw, nullptr))
            return sw;
        return s.shiftWidth();
    }
    bool expandTab() const
    {
        bool et;
        if (editorTabSettings(nullptr, nullptr, &et))
            return et;
        return s.expandTab();
    }
    int windowScrollOffset() const; // return scrolloffset but max half the current window height
    Column cursorColumn() const; // as visible on screen
    void updateFirstVisibleLine();
    int firstVisibleLine() const;
    int lastVisibleLine() const;
    int lineOnTop(int count = 1) const; // [count]-th line from top reachable without scrolling
    int lineOnBottom(int count = 1) const; // [count]-th line from bottom reachable without scrolling
    void scrollToLine(int line);
    void scrollUp(int count);
    void scrollDown(int count) { scrollUp(-count); }
    void updateScrollOffset();
    void alignViewportToCursor(Qt::AlignmentFlag align, int line = -1,
        bool moveToNonBlank = false);

    int lineToBlockNumber(int line) const;

    void setCursorPosition(const CursorPosition &p);
    void setCursorPosition(QTextCursor *tc, const CursorPosition &p);

    // Helper functions for indenting/
    bool isElectricCharacter(QChar c) const;
    void indentSelectedText(QChar lastTyped = QChar());
    void indentText(const Range &range, QChar lastTyped = QChar());
    void shiftRegionLeft(int repeat = 1);
    void shiftRegionRight(int repeat = 1);

    void moveToFirstNonBlankOnLine();
    void moveToFirstNonBlankOnLine(QTextCursor *tc);
    void moveToFirstNonBlankOnLineVisually();
    void moveToNonBlankOnLine(QTextCursor *tc);
    void moveToTargetColumn();
    void setTargetColumn();
    void moveToMatchingParanthesis();
    int vimMatchingParenthesis(int pos) const; // textual match, -1 if none
    void moveToBoundary(bool simple, bool forward = true);
    void moveToNextBoundary(bool end, int count, bool simple, bool forward);
    void moveToNextBoundaryStart(int count, bool simple, bool forward = true);
    void moveToNextBoundaryEnd(int count, bool simple, bool forward = true);
    void moveToBoundaryStart(int count, bool simple, bool forward = true);
    void moveToBoundaryEnd(int count, bool simple, bool forward = true);
    void moveToNextWord(bool end, int count, bool simple, bool forward, bool emptyLines);
    void moveToNextWordStart(int count, bool simple, bool forward = true, bool emptyLines = true);
    void moveToNextWordEnd(int count, bool simple, bool forward = true, bool emptyLines = true);
    void moveToWordStart(int count, bool simple, bool forward = true, bool emptyLines = true);
    void moveToWordEnd(int count, bool simple, bool forward = true, bool emptyLines = true);

    // Convenience wrappers to reduce line noise.
    void moveToStartOfLine();
    void moveToStartOfLineVisually();
    void moveToEndOfLine();
    void moveToEndOfLineVisually();
    void moveToEndOfLineVisually(QTextCursor *tc);
    void moveBehindEndOfLine();
    void moveUp(int n = 1) { moveDown(-n); }
    void moveDown(int n = 1);
    void moveUpVisually(int n = 1) { moveDownVisually(-n); }
    void moveDownVisually(int n = 1);
    void moveVertically(int n = 1) {
        if (g.gflag) {
            g.movetype = MoveExclusive;
            moveDownVisually(n);
        } else {
            g.movetype = MoveLineWise;
            moveDown(n);
        }
    }
    void movePageDown(int count = 1);
    void movePageUp(int count = 1) { movePageDown(-count); }
    void dump(const char *msg) const {
        qDebug() << msg << "POS: " << anchor() << position()
            << "VISUAL: " << g.visualMode;
    }
    void moveRight(int n = 1) {
        if (isVisualCharMode()) {
            const QTextBlock currentBlock = block();
            const int max = currentBlock.position() + currentBlock.length() - 1;
            const int pos = position() + n;
            setPosition(qMin(pos, max));
        } else {
            m_cursor.movePosition(Right, KeepAnchor, n);
        }
        if (atEndOfLine())
            q->fold(1, false);
        setTargetColumn();
    }
    void moveLeft(int n = 1) {
        m_cursor.movePosition(Left, KeepAnchor, n);
        setTargetColumn();
    }
    void moveToNextCharacter() {
        moveRight();
        if (atEndOfLine())
            moveRight();
    }
    void moveToPreviousCharacter() {
        moveLeft();
        if (atBlockStart())
            moveLeft();
    }
    void setAnchor() {
        m_cursor.setPosition(position(), MoveAnchor);
    }
    void setAnchor(int position) {
        m_cursor.setPosition(position, KeepAnchor);
    }
    void setPosition(int position) {
        m_cursor.setPosition(position, KeepAnchor);
    }
    void setAnchorAndPosition(int anchor, int position) {
        m_cursor.setPosition(anchor, MoveAnchor);
        m_cursor.setPosition(position, KeepAnchor);
    }

    // Set cursor in text editor widget.
    void commitCursor();

    // Restore cursor from editor widget.
    // Update selection, record jump and target column if cursor position
    // changes externally (e.g. by code completion).
    void pullCursor();

    QTextCursor editorCursor() const;

    // Values to save when starting FakeVim processing.
    int m_firstVisibleLine;
    QTextCursor m_cursor;
    bool m_cursorNeedsUpdate;

    // Cursor position (block, column) to restore after the "gw" reflow.
    int m_reflowSavedLine = 0;
    int m_reflowSavedColumn = 0;

    bool moveToPreviousParagraph(int count = 1) { return moveToNextParagraph(-count); }
    bool moveToNextParagraph(int count = 1);
    void moveToParagraphStartOrEnd(int direction = 1);

    bool handleFfTt(const QString &key, bool repeats = false);

    void enterVisualInsertMode(QChar command);
    void enterReplaceMode();
    void enterInsertMode();
    void enterInsertOrReplaceMode(Mode mode);
    void enterCommandMode(Mode returnToMode = CommandMode);
    void enterExMode(const QString &contents = QString());
    void showMessage(MessageLevel level, const QString &msg);
    void clearMessage() { showMessage(MessageInfo, QString()); }
    void notImplementedYet();
    void updateMiniBuffer();
    void updateSelection();
    void updateHighlights();
    void updateCursorShape();
    void setThinCursor(bool enable = true);
    bool hasThinCursor() const;
    QWidget *editor() const;
    QTextDocument *document() const { return EDITOR(document()); }
    QChar characterAt(int pos) const { return document()->characterAt(pos); }
    QChar characterAtCursor() const { return characterAt(position()); }

    void joinPreviousEditBlock();
    void beginEditBlock(bool largeEditBlock = false);
    void beginLargeEditBlock() { beginEditBlock(true); }
    void endEditBlock();
    void breakEditBlock() { m_buffer->breakEditBlock = true; }

    bool canModifyBufferData() const { return m_buffer->currentHandler.data() == this; }

    void onContentsChanged(int position, int charsRemoved, int charsAdded);
    void onCursorPositionChanged();
    void onUndoCommandAdded();

    void onInputTimeout();
    void onFixCursorTimeout();
    void onAutocmdTimeout();
    void queueChangeAutocmds(bool textChanged, bool cursorMoved);

    bool isCommandLineMode() const { return g.mode == ExMode || g.subsubmode == SearchSubSubMode; }
    bool isInsertMode() const { return g.mode == InsertMode || g.mode == ReplaceMode; }
    // Waiting for movement operator.
    bool isOperatorPending() const {
        return g.submode == ChangeSubMode
            || g.submode == DeleteSubMode
            || g.submode == ExchangeSubMode
            || g.submode == CommentSubMode
            || g.submode == ReplaceWithRegisterSubMode
            || g.submode == AddSurroundingSubMode
            || g.submode == FilterSubMode
            || g.submode == IndentSubMode
            || g.submode == ShiftLeftSubMode
            || g.submode == ShiftRightSubMode
            || g.submode == InvertCaseSubMode
            || g.submode == DownCaseSubMode
            || g.submode == UpCaseSubMode
            || g.submode == ReflowSubMode
            || g.submode == ReflowKeepCursorSubMode
            || g.submode == OperatorFuncSubMode
            || g.submode == YankSubMode; }

    bool isVisualMode() const { return g.visualMode != NoVisualMode; }
    bool isNoVisualMode() const { return g.visualMode == NoVisualMode; }
    bool isVisualCharMode() const { return g.visualMode == VisualCharMode; }
    bool isVisualLineMode() const { return g.visualMode == VisualLineMode; }
    bool isVisualBlockMode() const { return g.visualMode == VisualBlockMode; }
    char currentModeCode() const;
    // True if the key is mapped to <PASS> in the current mode, i.e. should be
    // handed to Qt Creator instead of handled by FakeVim (QTCREATORBUG-14413).
    bool isPassthroughKey(const Input &input) const;
    void updateEditor();

    void selectTextObject(bool simple, bool inner);
    void selectWordTextObject(bool inner);
    void selectWORDTextObject(bool inner);
    void selectSentenceTextObject(bool inner);
    void selectParagraphTextObject(bool inner);
    bool changeNumberTextObject(int count);
    // return true only if cursor is in a block delimited with correct characters
    bool selectBlockTextObject(bool inner, QChar left, QChar right);
    bool selectQuotedStringTextObject(bool inner, const QString &quote);
    bool selectArgumentTextObject(bool inner);
    bool selectTagTextObject(bool inner);

    void commitInsertState();
    void invalidateInsertState();
    bool isInsertStateValid() const;
    void clearLastInsertion();
    void ensureCursorVisible();
    void insertInInsertMode(const QString &text);

    // Macro recording
    bool startRecording(const Input &input);
    void record(const Input &input);
    void stopRecording();
    bool executeRegister(int reg);

    // Handle current command as synonym
    void handleAs(const QString &command);

public:
    QTextEdit *m_textedit;
    QPlainTextEdit *m_plaintextedit;
    Utils::PlainTextEdit *m_qcPlainTextEdit;
    bool hasValidEditor();

    bool m_inFakeVim; // true if currently processing a key press or a command

    FakeVimHandler *q;
    int m_register;
    BlockInsertMode m_visualBlockInsert;

    // Characters overwritten during the current Replace mode session, so that
    // <BS> can restore them (QTCREATORBUG-12120). A newline sentinel marks a
    // character that was appended past the end of the line (nothing to
    // restore, just remove it again).
    QString m_replacedChars;

    // Block number of a freshly auto-indented line that has not received any
    // typed content yet. Leaving such a line untouched removes the automatic
    // indentation again, matching Vim (QTCREATORBUG-15009). -1 means none.
    int m_autoIndentBlock = -1;

    bool m_anchorPastEnd;
    bool m_positionPastEnd; // '$' & 'l' in visual mode can move past eol

    QString m_currentFileName;

    // Buffer-ish Vimscript variables (b:/w:/t:, kept with their prefix). The
    // session-wide ones live in g.variables, see variableStore().
    QHash<QString, VimValue> m_variables;
    // User functions and the local (a:/l:) scope stack, one frame per active
    // call.
    struct UserFunction {
        QStringList params;
        QStringList defaults; // parallel to params; "" if no default value
        QString varargName; // Vim9 "...name": binds the extra arguments as a list
        QList<ExCommand> body;
        bool vim9 = false; // a :def function (bare-name args, Vim9 semantics)
    };
    struct AutoCommand {
        QString group; // the ":augroup" it belongs to, empty for none
        QString event;
        QString pattern;
        QString command;
    };
    bool m_noAutocmd = false; // ":noautocmd" is suppressing autocommands
    int m_autocmdDepth = 0; // nesting level of running autocommands
    // Whether the FileType event has fired for this buffer from a file type
    // that was not just a guess, which is what ":setf" checks so that the first
    // rule recognizing a buffer wins. Reset when the buffer is read again.
    bool m_didFileType = false;
    QString m_fileType; // matched by FileType autocommands
    // Buffer-local 'commentstring'; empty means derive it from the file type.
    QString m_commentString;
    // The pieces of the current match, while a "\=" replacement is being
    // worked out, for submatch() to reach.
    QStringList m_subMatches;
    // What bufnr() reports for this buffer, taken on first use.
    int m_bufferNumber = 0;
    // Syntax item names seen so far; a name's position here is its synID().
    QStringList m_syntaxNames;
    bool m_vim9 = false; // Vim9-script semantics are active
    QStringList m_scriptFileStack; // scripts currently sourcing, innermost last
    QStringList m_callStack; // user functions currently running, for <stack>
    QSet<QString> m_sourcesInFlight; // guards against :source/:import cycles
    QList<QHash<QString, VimValue>> m_localScopes;
    bool m_returning = false;
    VimValue m_returnValue;
    bool m_throwing = false; // an exception raised by :throw is in flight
    QString m_exception;
    bool m_finishing = false; // :finish - stop running the current command list
    int m_messageSilence = 0; // >0 while inside :silent
    bool m_silenceErrors = false; // :silent! also suppresses errors

    int m_findStartPosition;

    int anchor() const { return m_cursor.anchor(); }
    int position() const { return m_cursor.position(); }

    // Transform text selected by cursor in current visual mode.
    using Transformation = std::function<QString(const QString &)>;
    void transformText(const Range &range, QTextCursor &tc, const std::function<void()> &transform) const;
    void transformText(const Range &range, const Transformation &transform);

    void insertText(QTextCursor &tc, const QString &text);
    void insertText(const Register &reg);
    void removeText(const Range &range);

    void invertCase(const Range &range);

    void toggleComment(const Range &range);

    void callOperatorFunc(const Range &range);

    void exchangeRange(const Range &range);

    void replaceWithRegister(const Range &range);

    void surroundCurrentRange(const Input &input, const QString &prefix = {});

    void upCase(const Range &range);

    void downCase(const Range &range);

    void reflowText(const Range &range);

    void replaceText(const Range &range, const QString &str);

    QString selectText(const Range &range) const;
    void setCurrentRange(const Range &range);
    Range currentRange() const { return Range(position(), anchor(), g.rangemode); }

    void yankText(const Range &range, int toregister);

    void pasteText(bool afterCursor);

    void cutSelectedText(int reg = 0);

    void joinLines(int count, bool preserveSpace = false);

    void insertNewLine();

    bool handleInsertInEditor(const Input &input);
    bool passEventToEditor(QEvent &event, QTextCursor &tc); // Pass event to editor widget without filtering. Returns true if event was processed.

    // undo handling
    int revision() const { return document()->availableUndoSteps(); }
    void undoRedo(bool undo);
    void undo();
    void redo();
    void pushUndoState(bool overwrite = true);

    // extra data for '.'
    void replay(const QString &text, int repeat = 1);
    void setDotCommand(const QString &cmd) { g.dotCommand = cmd; }
    void setDotCommand(const QString &cmd, int n) { g.dotCommand = cmd.arg(n); }
    QString visualDotCommand() const;

    // visual modes
    void toggleVisualMode(VisualMode visualMode);
    void leaveVisualMode();
    void saveLastVisualMode();

    // marks
    Mark mark(QChar code) const;
    void setMark(QChar code, CursorPosition position);
    void removeMark(QChar code);
    // jump to valid mark return true if mark is valid and local
    bool jumpToMark(QChar mark, bool backTickMode);
    // update marks on undo/redo
    void updateMarks(const Marks &newMarks);
    CursorPosition markLessPosition() const { return mark('<').position(document()); }
    CursorPosition markGreaterPosition() const { return mark('>').position(document()); }

    int m_targetColumn; // -1 if past end of line
    int m_visualTargetColumn; // 'l' can move past eol in visual mode only
    int m_targetColumnWrapped; // column in current part of wrapped line

    // auto-indent
    QString tabExpand(int len) const;
    Column indentation(const QString &line) const;
    void insertAutomaticIndentation(bool goingDown, bool forceAutoIndent = false);
    // If the current line is a freshly auto-indented, still whitespace-only
    // line, remove that automatic indentation again (QTCREATORBUG-15009).
    void clearUntouchedAutoIndentation();
    void handleStartOfLine();

    // register handling
    QString registerContents(int reg) const;
    void setRegister(int reg, const QString &contents, RangeMode mode);
    RangeMode registerRangeMode(int reg) const;
    void getRegisterType(int *reg, bool *isClipboard, bool *isSelection, bool *append = nullptr) const;

    void recordJump(int position = -1);
    void jump(int distance);

    QList<QTextEdit::ExtraSelection> m_extraSelections;
    QTextCursor m_searchCursor;
    int m_searchStartPosition;
    int m_searchFromScreenLine;
    QString m_highlighted; // currently highlighted text

    bool handleExCommandHelper(ExCommand &cmd);
    void runExCommandLine(const QString &line); // keeps the current mode
    bool handleExPluginCommand(const ExCommand &cmd); // Handled by plugin?
    bool handleExBangCommand(const ExCommand &cmd);
    bool handleExDelMarksCommand(const ExCommand &cmd);
    bool handleExYankDeleteCommand(const ExCommand &cmd);
    bool handleExChangeCommand(const ExCommand &cmd);
    bool handleExMoveCommand(const ExCommand &cmd);
    bool handleExJoinCommand(const ExCommand &cmd);
    bool handleExGotoCommand(const ExCommand &cmd);
    bool handleExHistoryCommand(const ExCommand &cmd);
    bool handleExRegisterCommand(const ExCommand &cmd);
    bool handleExMapCommand(const ExCommand &cmd);
    bool handleExMultiRepeatCommand(const ExCommand &cmd);
    bool handleExNohlsearchCommand(const ExCommand &cmd);
    bool handleExNormalCommand(const ExCommand &cmd);
    bool handleExReadCommand(const ExCommand &cmd);
    bool handleExUndoRedoCommand(const ExCommand &cmd);
    bool handleExSetCommand(const ExCommand &cmd);
    bool handleExSortCommand(const ExCommand &cmd);
    bool handleExShiftCommand(const ExCommand &cmd);
    bool handleExSourceCommand(const ExCommand &cmd);
    bool handleExImportCommand(const ExCommand &cmd);
    bool handleExSubstituteCommand(const ExCommand &cmd);
    bool handleExTabNextCommand(const ExCommand &cmd);
    bool handleExTabPreviousCommand(const ExCommand &cmd);
    bool handleExTagCommand(const ExCommand &cmd);
    bool handleExWriteCommand(const ExCommand &cmd);
    bool handleExEchoCommand(const ExCommand &cmd);

    // Vimscript expression evaluation.
    friend class VimExpr;
    bool evaluateExpression(const QString &expr, VimValue *result, QString *error);
    bool variableValue(const QString &name, VimValue *result);
    void setVariable(const QString &name, const VimValue &value);
    bool unsetVariable(const QString &name);
    QHash<QString, VimValue> *variableStore(const QString &name, QString *key);
    void collectFunction(const QList<ExCommand> &cmds, int &index, bool active,
                         const ExCommand &header);
    VimValue callUserFunction(const QString &name, const UserFunction &fn,
                              const QList<VimValue> &args);
    bool optionValue(const QString &name, VimValue *result);
    QString commentString() const;
    bool setOption(const QString &name, const VimValue &value);
    bool callFunction(const QString &name, const QList<VimValue> &args,
                      VimValue *result, QString *error);
    bool searchFunction(const QList<VimValue> &args, VimValue *result, QString *error);
    CursorPosition lineColArg(const QString &spec) const;
    static VimValue deepCopy(const VimValue &value);
    static QString applyFileNameModifiers(const QString &fileName, const QString &mods);
    int bufferNumber();
    bool loadAutoloadScript(const QString &functionName);
    QString expandKeyword(const QString &what) const;
    bool invokeCallable(const VimValue &callable, const QList<VimValue> &args,
                        VimValue *result, QString *error);
    bool handleExLetCommand(const ExCommand &cmd);
    bool letAssignIndexed(const QString &args);
    bool handleExUnletCommand(const ExCommand &cmd);
    bool handleExCallCommand(const ExCommand &cmd);
    bool handleExExecuteCommand(const ExCommand &cmd);

    // Vimscript control flow: interpret a sequence of ex-commands honoring
    // :if/:elseif/:else/:endif.
    void runExCommands(const QList<ExCommand> &cmds);
    void execSequence(const QList<ExCommand> &cmds, int &index, bool active);
    void execIf(const QList<ExCommand> &cmds, int &index, bool active, bool condition);
    void execWhile(const QList<ExCommand> &cmds, int &index, bool active,
                   const QString &condition);
    void execFor(const QList<ExCommand> &cmds, int &index, bool active,
                 const QString &spec);
    void execTry(const QList<ExCommand> &cmds, int &index, bool active);
    bool evalCondition(const QString &expr);
    bool interpreterInterrupted() const
    {
        return m_loopSignal != NoSignal || m_returning || m_throwing || m_finishing;
    }
    bool handleExSilentCommand(const ExCommand &cmd);
    bool handleExModifierCommand(const ExCommand &cmd);
    bool handleExAutocmdCommand(const ExCommand &cmd);
    bool handleExAugroupCommand(const ExCommand &cmd);
    bool handleExDoAutocmdCommand(const ExCommand &cmd);
    bool handleExCommandDefCommand(const ExCommand &cmd);
    bool handleExUserCommand(const ExCommand &cmd);
    bool handleExSetFileTypeCommand(const ExCommand &cmd);
    void setFileType(const QString &type, bool fallback = false);
    bool didFileType() const;
    void processModelines();
    void applyModeline(const QString &line);
    void triggerAutocmd(const QString &event);
    enum LoopSignal { NoSignal, BreakSignal, ContinueSignal };
    LoopSignal m_loopSignal = NoSignal;

    void setTabSize(int tabSize);
    void setupCharClass();
    int charClass(QChar c, bool simple) const;
    signed char m_charClass[256];

    int m_ctrlVAccumulator;
    int m_ctrlVLength;
    int m_ctrlVBase;

    QTimer m_fixCursorTimer;
    QTimer m_inputTimer;
    // TextChanged and CursorMoved are reported while the document or the cursor
    // is still being changed, so their rules run from a zero timer: a run of
    // changes then fires once, and a rule may edit the buffer safely.
    QTimer m_autocmdTimer;
    // Which mode the change happened in is recorded here rather than read back
    // when the timer runs, since the mode may have been left by then.
    bool m_pendingTextChanged = false;
    bool m_pendingTextChangedI = false;
    bool m_pendingCursorMoved = false;
    bool m_pendingCursorMovedI = false;

    void miniBufferTextEdited(const QString &text, int cursorPos, int anchorPos);

    // Data shared among editors with same document.
    struct BufferData
    {
        QStack<State> undo;
        QStack<State> redo;
        State undoState;
        int lastRevision = 0;

        int editBlockLevel = 0; // current level of edit blocks
        bool breakEditBlock = false; // if true, joinPreviousEditBlock() starts new edit block

        QStack<CursorPosition> jumpListUndo;
        QStack<CursorPosition> jumpListRedo;

        VisualMode lastVisualMode = NoVisualMode;
        bool lastVisualModeInverted = false;

        Marks marks;

        // Insert state to get last inserted text.
        struct InsertState {
            int pos1;
            int pos2;
            int backspaces;
            int deletes;
            QSet<int> spaces;
            bool insertingSpaces;
            QString textBeforeCursor;
            bool newLineBefore;
            bool newLineAfter;
        } insertState;

        QString lastInsertion;

        // If there are multiple editors with same document,
        // only the handler with last focused editor can change buffer data.
        QPointer<FakeVimHandler::Private> currentHandler;
    };

    using BufferDataPtr = std::shared_ptr<BufferData>;
    void pullOrCreateBufferData();
    BufferDataPtr m_buffer;

    // Data shared among all editors.
    static struct GlobalData
    {
        GlobalData()
            : mappings()
            , currentMap(&mappings)
        {
            commandBuffer.setPrompt(':');
        }

        // Current state.
        bool passing = false; // let the core see the next event
        Mode mode = CommandMode;
        SubMode submode = NoSubMode;
        SubSubMode subsubmode = NoSubSubMode;
        Input subsubdata;
        VisualMode visualMode = NoVisualMode;
        Input minibufferData;

        // [count] for current command, 0 if no [count] available
        int mvcount = 0;
        int opcount = 0;

        MoveType movetype = MoveInclusive;
        RangeMode rangemode = RangeCharMode;
        bool gflag = false;  // whether current command started with 'g'

        // Extra data for ';'.
        Input semicolonType;  // 'f', 'F', 't', 'T'
        QString semicolonKey;

        // Repetition.
        QString dotCommand;

        QHash<int, Register> registers;

        // All mappings.
        Mappings mappings;

        // Input.
        QList<Input> pendingInput;
        MappingsIterator currentMap;
        QStack<MappingState> mapStates;
        int mapDepth = 0;

        // Command line buffers.
        CommandBuffer commandBuffer;
        CommandBuffer searchBuffer;

        // Current mini buffer message.
        QString currentMessage;
        MessageLevel currentMessageLevel = MessageInfo;
        QString currentCommand;

        // Search state.
        QString lastSearch; // last search expression as entered by user
        QString lastNeedle; // last search expression translated with vimPatternToQtPattern()
        bool lastSearchForward = false; // last search command was '/' or '*'
        bool highlightsCleared = false; // ':nohlsearch' command is active until next search
        bool findPending = false; // currently searching using external tool (until editor is focused again)

        // Last substitution command.
        QString lastSubstituteFlags;
        QString lastSubstitutePattern;
        QString lastSubstituteReplacement;

        // Global marks.
        Marks marks;

        // Return to insert/replace mode after single command (<C-O>).
        Mode returnToMode = CommandMode;

        // Currently recorded macro
        bool isRecording = false;
        QString recorded;
        int currentRegister = 0;
        int lastExecutedRegister = 0;

        // If empty, cx{motion} will store the range defined by {motion} here.
        // If non-empty, cx{motion} replaces the {motion} with selectText(*exchangeData)
        std::optional<Range> exchangeRange;

        bool surroundUpperCaseS; // True for yS and cS, false otherwise
        QString surroundFunction; // Used for storing the function name provided to ys{motion}f

        // Vimscript state that Vim keeps for the whole session, not per buffer.
        // It has to outlive a single handler because the vimrc is read into a
        // throwaway one (see FakeVimPlugin::maybeReadVimRc), while the mappings
        // it installs are used from every editor.
        QHash<QString, VimValue> variables; // g:/s:/v: and script-level names
        QHash<QString, UserFunction> userFunctions;
        QList<AutoCommand> autoCommands;
        QHash<QString, QString> userCommands; // :command Name -> replacement
        QString currentAutoGroup; // group ":augroup" left current
        int lastBufferNumber = 0; // hands out the number bufnr() reports
        // Scripts already looked for, so a fruitless search is made only once.
        QSet<QString> autoloadTried;
        // Vim9 ":export"ed names, keyed by canonical script path.
        QHash<QString, QMap<QString, VimValue>> moduleExports;
    } g;

    FakeVimSettings &s = settings();
};

static void initSingleShotTimer(QTimer *timer,
                                int interval,
                                FakeVimHandler::Private *receiver,
                                void (FakeVimHandler::Private::*slot)())
{
    timer->setSingleShot(true);
    timer->setInterval(interval);
    QObject::connect(timer, &QTimer::timeout, receiver, slot);
}

FakeVimHandler::Private::GlobalData FakeVimHandler::Private::g;

FakeVimHandler::Private::Private(FakeVimHandler *parent, QWidget *widget)
{
    q = parent;
    m_textedit = qobject_cast<QTextEdit *>(widget);
    m_plaintextedit = qobject_cast<QPlainTextEdit *>(widget);
    m_qcPlainTextEdit = qobject_cast<Utils::PlainTextEdit *>(widget);

    init();

    if (editor()) {
        connect(EDITOR(document()), &QTextDocument::contentsChange,
                this, &Private::onContentsChanged);
        connect(EDITOR(document()), &QTextDocument::undoCommandAdded,
                this, &Private::onUndoCommandAdded);
        m_buffer->lastRevision = revision();
    }

#ifndef FAKEVIM_STANDALONE
    connect(&s.showMarks, &FvBaseAspect::changed,
            this, &FakeVimHandler::Private::updateSelection);
#endif
}

void FakeVimHandler::Private::init()
{
    m_cursor = QTextCursor(document());
    m_cursorNeedsUpdate = true;
    m_inFakeVim = false;
    m_findStartPosition = -1;
    m_visualBlockInsert = NoneBlockInsertMode;
    m_positionPastEnd = false;
    m_anchorPastEnd = false;
    m_register = '"';
    m_targetColumn = 0;
    m_visualTargetColumn = 0;
    m_targetColumnWrapped = 0;
    m_searchStartPosition = 0;
    m_searchFromScreenLine = 0;
    m_firstVisibleLine = 0;
    m_ctrlVAccumulator = 0;
    m_ctrlVLength = 0;
    m_ctrlVBase = 0;

    initSingleShotTimer(&m_fixCursorTimer, 0, this, &FakeVimHandler::Private::onFixCursorTimeout);
    initSingleShotTimer(&m_autocmdTimer, 0, this, &FakeVimHandler::Private::onAutocmdTimeout);
    initSingleShotTimer(&m_inputTimer, 1000, this, &FakeVimHandler::Private::onInputTimeout);

    pullOrCreateBufferData();
    setupCharClass();
}

void FakeVimHandler::Private::focus()
{
    m_buffer->currentHandler = this;

    enterFakeVim();

    stopIncrementalFind();
    if (isCommandLineMode()) {
        if (g.subsubmode == SearchSubSubMode) {
            setPosition(m_searchStartPosition);
            scrollToLine(m_searchFromScreenLine);
        } else {
            leaveVisualMode();
            setPosition(qMin(position(), anchor()));
        }
        leaveCurrentMode();
        setTargetColumn();
        setAnchor();
        commitCursor();
    } else {
        clearCurrentMode();
    }
    fixExternalCursor(true);
    updateHighlights();

    leaveFakeVim(false);
}

void FakeVimHandler::Private::unfocus()
{
    fixExternalCursor(false);
}

void FakeVimHandler::Private::fixExternalCursor(bool focus)
{
    m_fixCursorTimer.stop();

    if (isVisualCharMode() && !focus && !hasThinCursor()) {
        // Select the character under thick cursor for external operations with text selection.
        fixExternalCursorPosition(false);
    } else if (isVisualCharMode() && focus && hasThinCursor()) {
        // Fix cursor position if changing its shape.
        // The fix is postponed so context menu action can be finished.
        m_fixCursorTimer.start();
    } else {
        updateCursorShape();
    }
}

void FakeVimHandler::Private::fixExternalCursorPosition(bool focus)
{
    QTextCursor tc = editorCursor();
    if (tc.anchor() < tc.position()) {
        tc.movePosition(focus ? Left : Right, KeepAnchor);
        EDITOR(setTextCursor(tc));
    }

    setThinCursor(!focus);
}

void FakeVimHandler::Private::enterFakeVim()
{
    if (m_inFakeVim) {
        qWarning("enterFakeVim() shouldn't be called recursively!");
        return;
    }

    if (!m_buffer->currentHandler)
        m_buffer->currentHandler = this;

    pullOrCreateBufferData();

    m_inFakeVim = true;

    removeEventFilter();

    pullCursor();

    updateFirstVisibleLine();
}

void FakeVimHandler::Private::leaveFakeVim(bool needUpdate)
{
    if (!m_inFakeVim) {
        qWarning("enterFakeVim() not called before leaveFakeVim()!");
        return;
    }

    // The command might have destroyed the editor.
    if (hasValidEditor()) {
        if (s.showMarks())
            updateSelection();

        updateMiniBuffer();

        if (needUpdate) {
            // Move cursor line to middle of screen if it's not visible.
            const int line = cursorLine();
            if (line < firstVisibleLine() || line > firstVisibleLine() + linesOnScreen())
                scrollToLine(qMax(0, line - linesOnScreen() / 2));
            else
                scrollToLine(firstVisibleLine());
            updateScrollOffset();

            commitCursor();
        }

        installEventFilter();
    }

    m_inFakeVim = false;
}

void FakeVimHandler::Private::leaveFakeVim(EventResult eventResult)
{
    leaveFakeVim(eventResult == EventHandled || eventResult == EventCancelled);
}

bool FakeVimHandler::Private::wantsOverride(QKeyEvent *ev)
{
    const int key = ev->key();
    const Qt::KeyboardModifiers mods = ev->modifiers();
    KEY_DEBUG("SHORTCUT OVERRIDE" << key << "  PASSING: " << g.passing);

    // If the user has mapped this key in the current mode, claim it so the
    // mapping is applied instead of letting a Qt Creator shortcut (or the
    // key's native meaning) take over (QTCREATORBUG-20998). A key mapped to
    // <PASS> is the exception: leave it for Qt Creator (QTCREATORBUG-14413).
    if (!g.passing) {
        const Input input(key, mods, ev->text());
        if (input.isValid() && g.mappings.value(currentModeCode()).contains(input))
            return !isPassthroughKey(input);
    }

    if (key == Key_Escape) {
        if (g.subsubmode == SearchSubSubMode)
            return true;
        // Not sure this feels good. People often hit Esc several times.
        if (isNoVisualMode()
                && g.mode == CommandMode
                && g.submode == NoSubMode
                && g.currentCommand.isEmpty()
                && g.returnToMode == CommandMode)
            return false;
        return true;
    }

    // We are interested in overriding most Ctrl key combinations.
    if (isOnlyControlModifier(mods)
            && !s.passControlKey()
            && ((key >= Key_A && key <= Key_Z && key != Key_K)
                || key == Key_BracketLeft || key == Key_BracketRight)) {
        // Ctrl-K is special as it is the Core's default notion of Locator
        if (g.passing) {
            KEY_DEBUG(" PASSING CTRL KEY");
            // We get called twice on the same key
            //g.passing = false;
            return false;
        }
        KEY_DEBUG(" NOT PASSING CTRL KEY");
        return true;
    }

    // Let other shortcuts trigger.
    return false;
}

EventResult FakeVimHandler::Private::handleEvent(QKeyEvent *ev)
{
    const int key = ev->key();
    const Qt::KeyboardModifiers mods = ev->modifiers();

    if (key == Key_Shift || key == Key_Alt || key == Key_Control
            || key == Key_AltGr || key == Key_Meta)
    {
        KEY_DEBUG("PLAIN MODIFIER");
        return EventUnhandled;
    }

    // Some layout modifiers (e.g. ISO_Level5_Shift) have no Qt key code and
    // arrive with a single null character as text. Do not treat that as
    // insertable input - it would insert a stray NUL (QTCREATORBUG-26818).
    if (ev->text().size() == 1 && ev->text().at(0).isNull())
        return EventUnhandled;

    if (g.passing) {
        passShortcuts(false);
        KEY_DEBUG("PASSING PLAIN KEY..." << ev->key() << ev->text());
        //if (input.is(',')) { // use ',,' to leave, too.
        //    qDebug() << "FINISHED...";
        //    return EventHandled;
        //}
        KEY_DEBUG("   PASS TO CORE");
        return EventPassedToCore;
    }

#ifndef FAKEVIM_STANDALONE
    bool inSnippetMode = false;
    QMetaObject::invokeMethod(editor(),
        "inSnippetMode", Q_ARG(bool *, &inSnippetMode));

    if (inSnippetMode)
        return EventPassedToCore;

    // Let an inline rename (e.g. "Rename Symbol Under Cursor") consume keys
    // itself, so Esc/Enter finish it instead of being handled by Vim
    // (QTCREATORBUG-20619).
    bool inInlineRename = false;
    QMetaObject::invokeMethod(editor(),
        "inInlineRename", Q_ARG(bool *, &inInlineRename));

    if (inInlineRename)
        return EventPassedToCore;
#endif

    // Fake "End of line"
    //m_tc = m_cursor;

    //bool hasBlock = false;
    //q->requestHasBlockSelection(&hasBlock);
    //qDebug() << "IMPORT BLOCK 2:" << hasBlock;

    //if (0 && hasBlock) {
    //    (pos > anc) ? --pos : --anc;

    //if ((mods & RealControlModifier) != 0) {
    //    if (key >= Key_A && key <= Key_Z)
    //        key = shift(key); // make it lower case
    //    key = control(key);
    //} else if (key >= Key_A && key <= Key_Z && (mods & Qt::ShiftModifier) == 0) {
    //    key = shift(key);
    //}

    //QTC_ASSERT(g.mode == InsertMode || g.mode == ReplaceMode
    //        || !atBlockEnd() || block().length() <= 1,
    //    qDebug() << "Cursor at EOL before key handler");

    const Input input(key, mods, ev->text());
    if (!input.isValid())
        return EventUnhandled;

    // A <PASS>-mapped key reaching here (no Qt Creator shortcut consumed it) is
    // handed to the editor unchanged rather than run through FakeVim
    // (QTCREATORBUG-14413).
    if (isPassthroughKey(input))
        return EventPassedToCore;

    enterFakeVim();
    EventResult result = handleKey(input);
    leaveFakeVim(result);
    return result;
}

void FakeVimHandler::Private::installEventFilter()
{
    EDITOR(installEventFilter(q));
}

void FakeVimHandler::Private::removeEventFilter()
{
    EDITOR(removeEventFilter(q));
}

void FakeVimHandler::Private::setupWidget()
{
    m_cursorNeedsUpdate = true;
    if (m_textedit) {
        connect(m_textedit, &QTextEdit::cursorPositionChanged,
                this, &FakeVimHandler::Private::onCursorPositionChanged, Qt::UniqueConnection);
    } else if (m_plaintextedit) {
        connect(m_plaintextedit, &QPlainTextEdit::cursorPositionChanged,
                this, &FakeVimHandler::Private::onCursorPositionChanged, Qt::UniqueConnection);
    } else {
        connect(m_qcPlainTextEdit, &Utils::PlainTextEdit::cursorPositionChanged,
                this, &FakeVimHandler::Private::onCursorPositionChanged, Qt::UniqueConnection);
    }

    enterFakeVim();

    leaveCurrentMode();

    updateEditor();

    leaveFakeVim();
}

void FakeVimHandler::Private::commitInsertState()
{
    if (!isInsertStateValid())
        return;

    QString &lastInsertion = m_buffer->lastInsertion;
    BufferData::InsertState &insertState = m_buffer->insertState;

    // An insert-mode command that shrinks the line (e.g. CTRL-D/CTRL-T
    // re-indentation) can leave the recorded positions past the current end of
    // the document; clamp them before reading the inserted text.
    const int last = lastPositionInDocument();
    if (last < 0)
        return; // empty document: nothing was inserted
    const int pos1 = qBound(0, insertState.pos1, last);
    const int pos2 = qBound(0, insertState.pos2, last);

    // Get raw inserted text.
    lastInsertion = textAt(pos1, pos2);

    // Escape special characters and spaces inserted by user (not by auto-indentation).
    for (int i = lastInsertion.size() - 1; i >= 0; --i) {
        const int pos = pos1 + i;
        const QChar c = characterAt(pos);
        if (c == '<') {
            lastInsertion.replace(i, 1, "<LT>");
        } else if (c == ' ' && insertState.spaces.contains(pos)) {
            lastInsertion.replace(i, 1, QLatin1String("<SPACE>"));
        } else if (c == '\t' && insertState.spaces.contains(pos)) {
            // Escape a user-typed tab as a literal (^V) insert so it survives
            // the auto-indentation stripping below and, when replayed (e.g.
            // for a block insert or the dot command), is inserted verbatim
            // instead of re-triggering the editor's Tab handling
            // (QTCREATORBUG-24094).
            lastInsertion.replace(i, 1, QLatin1String("<C-v>\t"));
        }
    }

    // Remove unnecessary backspaces.
    while (insertState.backspaces > 0 && !lastInsertion.isEmpty() && lastInsertion[0].isSpace())
        --insertState.backspaces;

    // backspaces in front of inserted text
    lastInsertion.prepend(QString("<BS>").repeated(insertState.backspaces));
    // deletes after inserted text
    lastInsertion.prepend(QString("<DELETE>").repeated(insertState.deletes));

    // Remove indentation.
    static const QRegularExpression regexp("(^|\n)[\\t ]+");
    lastInsertion.replace(regexp, "\\1");
}

void FakeVimHandler::Private::invalidateInsertState()
{
    BufferData::InsertState &insertState = m_buffer->insertState;
    insertState.pos1 = -1;
    insertState.pos2 = position();
    insertState.backspaces = 0;
    insertState.deletes = 0;
    insertState.spaces.clear();
    insertState.insertingSpaces = false;
    insertState.textBeforeCursor = textAt(block().position(), position());
    insertState.newLineBefore = false;
    insertState.newLineAfter = false;
}

bool FakeVimHandler::Private::isInsertStateValid() const
{
    return m_buffer->insertState.pos1 != -1;
}

void FakeVimHandler::Private::clearLastInsertion()
{
    invalidateInsertState();
    m_buffer->lastInsertion.clear();
    m_buffer->insertState.pos1 = m_buffer->insertState.pos2;
}

void FakeVimHandler::Private::ensureCursorVisible()
{
    int pos = position();
    int anc = isVisualMode() ? anchor() : position();

    // fix selection so it is outside folded block
    int start = qMin(pos, anc);
    int end = qMax(pos, anc) + 1;
    QTextBlock block = blockAt(start);
    QTextBlock block2 = blockAt(end);
    if (!block.isVisible() || !block2.isVisible()) {
        // FIXME: Moving cursor left/right or unfolding block immediately after block is folded
        //        should restore cursor position inside block.
        // Changing cursor position after folding is not Vim behavior so at least record the jump.
        if (block.isValid() && !block.isVisible())
            recordJump();

        pos = start;
        while (block.isValid() && !block.isVisible())
            block = block.previous();
        if (block.isValid())
            pos = block.position() + qMin(m_targetColumn, block.length() - 2);

        if (isVisualMode()) {
            anc = end;
            while (block2.isValid() && !block2.isVisible()) {
                anc = block2.position() + block2.length() - 2;
                block2 = block2.next();
            }
        }

        setAnchorAndPosition(anc, pos);
    }
}

void FakeVimHandler::Private::updateEditor()
{
    setTabSize(tabStop());
    setupCharClass();
}

void FakeVimHandler::Private::setTabSize(int tabSize)
{
    const int charWidth = QFontMetrics(EDITOR(font())).horizontalAdvance(' ');
    const int width = charWidth * tabSize;
    EDITOR(setTabStopDistance(width));
}

void FakeVimHandler::Private::restoreWidget(int tabSize)
{
    //EDITOR(removeEventFilter(q));
    setTabSize(tabSize);
    g.visualMode = NoVisualMode;
    // Force "ordinary" cursor.
    setThinCursor();
    updateSelection();
    updateHighlights();
    if (m_textedit) {
        disconnect(m_textedit, &QTextEdit::cursorPositionChanged,
                   this, &FakeVimHandler::Private::onCursorPositionChanged);
    } else if (m_plaintextedit) {
        disconnect(m_plaintextedit, &QPlainTextEdit::cursorPositionChanged,
                   this, &FakeVimHandler::Private::onCursorPositionChanged);
    } else {
        disconnect(m_qcPlainTextEdit, &Utils::PlainTextEdit::cursorPositionChanged,
                   this, &FakeVimHandler::Private::onCursorPositionChanged);
    }
}

EventResult FakeVimHandler::Private::handleKey(const Input &input)
{
    KEY_DEBUG("HANDLE INPUT: " << input);

    bool hasInput = input.isValid();

    // Waiting on input to complete mapping?
    EventResult r = stopWaitForMapping(hasInput);

    if (hasInput) {
        record(input);
        g.pendingInput.append(input);
    }

    // Process pending input.
    // Note: Pending input is global state and can be extended by:
    //         1. handling a user input (though handleKey() is not called recursively),
    //         2. expanding a user mapping or
    //         3. executing a register.
    while (!g.pendingInput.isEmpty() && r == EventHandled) {
        const Input in = g.pendingInput.takeFirst();

        // invalid input is used to pop mapping state
        if (!in.isValid()) {
            endMapping();
        } else {
            // Handle user mapping.
            if (canHandleMapping()) {
                if (extendMapping(in)) {
                    if (!hasInput || !g.currentMap.canExtend())
                        expandCompleteMapping();
                } else if (!expandCompleteMapping()) {
                    r = handleCurrentMapAsDefault();
                }
            } else {
                r = handleDefaultKey(in);
            }
        }
    }

    if (g.currentMap.canExtend()) {
        waitForMapping();
        return EventHandled;
    }

    if (r != EventHandled)
        clearPendingInput();

    return r;
}

bool FakeVimHandler::Private::handleCommandBufferPaste(const Input &input)
{
    const bool inCommandLine = g.subsubmode == SearchSubSubMode || g.mode == ExMode;
    if (inCommandLine && input.isControl('v')) {
        // OS-style paste of the clipboard into the command line, in addition to
        // the Vim way with Ctrl-R (QTCREATORBUG-23785). Only the first line is
        // used, since the command line is single-line.
        CommandBuffer &buffer = (g.subsubmode == SearchSubSubMode)
            ? g.searchBuffer : g.commandBuffer;
        QString text = QApplication::clipboard()->text();
        const int newline = text.indexOf('\n');
        if (newline != -1)
            text.truncate(newline);
        text.remove('\r');
        buffer.insertText(text);
        updateMiniBuffer();
        return true;
    }
    if (input.isControl('r') && inCommandLine) {
        g.minibufferData = input;
        return true;
    }
    if (g.minibufferData.isControl('r')) {
        g.minibufferData = Input();
        if (input.isEscape())
            return true;
        CommandBuffer &buffer = (g.subsubmode == SearchSubSubMode)
            ? g.searchBuffer : g.commandBuffer;
        if (input.isControl('w')) {
            QTextCursor tc = m_cursor;
            tc.select(QTextCursor::WordUnderCursor);
            QString word = tc.selectedText();
            buffer.insertText(word);
        } else {
            QString r = registerContents(input.asChar().unicode());
            buffer.insertText(r);
        }
        updateMiniBuffer();
        return true;
    }
    return false;
}

EventResult FakeVimHandler::Private::handleDefaultKey(const Input &input)
{
    if (g.passing) {
        passShortcuts(false);
        QKeyEvent event(QEvent::KeyPress, input.key(), input.modifiers(), input.text());
        bool accepted = QApplication::sendEvent(editor()->window(), &event);
        if (accepted || (!hasValidEditor()))
            return EventHandled;
    }

    if (input == Nop)
        return EventHandled;
    else if (g.subsubmode == SearchSubSubMode)
        return handleSearchSubSubMode(input);
    else if (g.mode == CommandMode)
        return handleCommandMode(input);
    else if (g.mode == InsertMode || g.mode == ReplaceMode)
        return handleInsertOrReplaceMode(input);
    else if (g.mode == ExMode)
        return handleExMode(input);
    return EventUnhandled;
}

EventResult FakeVimHandler::Private::handleCurrentMapAsDefault()
{
    // If mapping has failed take the first input from it and try default command.
    const Inputs &inputs = g.currentMap.currentInputs();
    if (inputs.isEmpty())
        return EventHandled;

    Input in = inputs.front();
    if (inputs.size() > 1)
        prependInputs(inputs.mid(1));
    g.currentMap.reset();

    return handleDefaultKey(in);
}

void FakeVimHandler::Private::prependInputs(const QVector<Input> &inputs)
{
    for (int i = inputs.size() - 1; i >= 0; --i)
        g.pendingInput.prepend(inputs[i]);
}

void FakeVimHandler::Private::prependMapping(const Inputs &inputs)
{
    // FIXME: Implement Vim option maxmapdepth (default value is 1000).
    if (g.mapDepth >= 1000) {
        const int i = qMax(0, g.pendingInput.lastIndexOf(Input()));
        const QList<Input> inputs = g.pendingInput.mid(i);
        clearPendingInput();
        g.pendingInput.append(inputs);
        showMessage(MessageError, Tr::tr("Recursive mapping"));
        return;
    }

    ++g.mapDepth;
    g.pendingInput.prepend(Input());
    prependInputs(inputs);
    g.commandBuffer.setHistoryAutoSave(false);

    // start new edit block (undo/redo) only if necessary
    bool editBlock = m_buffer->editBlockLevel == 0 && !(isInsertMode() && isInsertStateValid());
    if (editBlock)
        beginLargeEditBlock();
    g.mapStates << MappingState(inputs.noremap(), inputs.silent(), editBlock);
}

bool FakeVimHandler::Private::expandCompleteMapping()
{
    if (!g.currentMap.isComplete())
        return false;

    const Inputs &inputs = g.currentMap.inputs();
    int usedInputs = g.currentMap.mapLength();
    prependInputs(g.currentMap.currentInputs().mid(usedInputs));
    if (inputs.isExCommand()) {
        // "<Cmd>{command}<CR>": run the command, staying in the current mode.
        // Keys behind the <CR> are keys again and follow the command.
        if (!QVector<Input>(inputs).isEmpty())
            prependMapping(Inputs(inputs, inputs.noremap(), inputs.silent()));
        // A command standing in for a motion must not see the operator waiting
        // for it. Vim runs it as if nothing were pending, so "v" means what it
        // means in normal mode, and the selection the command leaves behind is
        // the range the operator then works on. That is how a script defines a
        // text object.
        const bool forOperator = isOperatorPending();
        const SubMode savedSubmode = g.submode;
        const SubSubMode savedSubsubmode = g.subsubmode;
        const MoveType savedMovetype = g.movetype;
        const RangeMode savedRangemode = g.rangemode;
        const int savedAnchor = anchor();
        if (forOperator) {
            g.submode = NoSubMode;
            g.subsubmode = NoSubSubMode;
        }

        const bool savedVim9 = m_vim9;
        m_vim9 = inputs.vim9();
        runExCommandLine(inputs.exCommand());
        m_vim9 = savedVim9;

        if (forOperator) {
            const bool selected = isVisualMode();
            const VisualMode visual = g.visualMode;
            int from = 0;
            int to = 0;
            if (selected) {
                from = qMin(anchor(), position());
                to = qMax(anchor(), position());
                leaveVisualMode();
            }
            g.submode = savedSubmode;
            g.subsubmode = savedSubsubmode;
            if (selected) {
                // The selection replaces whatever range the operator had.
                g.movetype = visual == VisualLineMode ? MoveLineWise : MoveInclusive;
                g.rangemode = visual == VisualBlockMode ? RangeBlockMode
                            : visual == VisualLineMode ? RangeLineMode : RangeCharMode;
                setAnchorAndPosition(from, to);
            } else {
                // Nothing was selected, so the command acted as a plain motion.
                g.movetype = savedMovetype;
                g.rangemode = savedRangemode;
                setAnchorAndPosition(savedAnchor, position());
            }
            finishMovement();
        }
    } else if (inputs.isExpression()) {
        // ":map <expr>": the right-hand side is an expression whose string
        // result is used as the typed keys.
        VimValue value;
        QString error, keys;
        if (evaluateExpression(inputs.expression(), &value, &error))
            keys = value.toString();
        else
            showMessage(MessageError, error);
        prependMapping(Inputs(keys, inputs.noremap(), inputs.silent()));
    } else {
        prependMapping(inputs);
    }
    g.currentMap.reset();

    return true;
}

bool FakeVimHandler::Private::extendMapping(const Input &input)
{
    if (!g.currentMap.isValid())
        g.currentMap.reset(currentModeCode());
    return g.currentMap.walk(input);
}

void FakeVimHandler::Private::endMapping()
{
    if (!g.currentMap.canExtend())
        --g.mapDepth;
    if (g.mapStates.isEmpty())
        return;
    if (g.mapStates.last().editBlock)
        endEditBlock();
    g.mapStates.pop_back();
    if (g.mapStates.isEmpty())
        g.commandBuffer.setHistoryAutoSave(true);
}

bool FakeVimHandler::Private::canHandleMapping()
{
    // Don't handle user mapping in sub-modes that cannot be followed by movement and in "noremap".
    return g.subsubmode == NoSubSubMode
        && g.submode != RegisterSubMode
        && g.submode != WindowSubMode
        && g.submode != ZSubMode
        && g.submode != CapitalZSubMode
        && g.submode != ReplaceSubMode
        && g.submode != MacroRecordSubMode
        && g.submode != MacroExecuteSubMode
        && (g.mapStates.isEmpty() || !g.mapStates.last().noremap);
}

void FakeVimHandler::Private::clearPendingInput()
{
    // Clear pending input on interrupt or bad mapping.
    g.pendingInput.clear();
    g.mapStates.clear();
    g.mapDepth = 0;

    // Clear all started edit blocks.
    while (m_buffer->editBlockLevel > 0)
        endEditBlock();
}

void FakeVimHandler::Private::waitForMapping()
{
    g.currentCommand.clear();
    for (const Input &input : g.currentMap.currentInputs())
        g.currentCommand.append(input.toString());

    // Wait for the user to press another key. With 'timeout' set, complete the
    // mapping automatically after 'timeoutlen' milliseconds; without it, wait
    // indefinitely for the next key (as Vim does for 'notimeout')
    // (QTCREATORBUG-29162).
    if (s.timeout())
        m_inputTimer.start(qMax(0, int(s.timeoutlen())));
}

EventResult FakeVimHandler::Private::stopWaitForMapping(bool hasInput)
{
    if (!hasInput || m_inputTimer.isActive()) {
        m_inputTimer.stop();
        g.currentCommand.clear();
        if (!hasInput && !expandCompleteMapping()) {
            // Cannot complete mapping so handle the first input from it as default command.
            return handleCurrentMapAsDefault();
        }
    }

    return EventHandled;
}

void FakeVimHandler::Private::stopIncrementalFind()
{
    if (g.findPending) {
        g.findPending = false;
        setAnchorAndPosition(m_findStartPosition, m_cursor.selectionStart());
        finishMovement();
        setAnchor();
    }
}

void FakeVimHandler::Private::updateFind(bool isComplete)
{
    if (!isComplete && !s.incSearch())
        return;

    g.currentMessage.clear();

    const QString &needle = g.searchBuffer.contents();
    if (isComplete) {
        setPosition(m_searchStartPosition);
        if (!needle.isEmpty())
            recordJump();
    }

    SearchData sd;
    sd.needle = needle;
    sd.forward = g.lastSearchForward;
    sd.highlightMatches = isComplete;
    search(sd, isComplete);
}

void FakeVimHandler::Private::resetCount()
{
    g.mvcount = 0;
    g.opcount = 0;
}

bool FakeVimHandler::Private::isInputCount(const Input &input) const
{
    return input.isDigit() && (!input.is('0') || g.mvcount > 0);
}

bool FakeVimHandler::Private::atEmptyLine(int pos) const
{
    return blockAt(pos).length() == 1;
}

bool FakeVimHandler::Private::atEmptyLine(const QTextCursor &tc) const
{
    return atEmptyLine(tc.position());
}

bool FakeVimHandler::Private::atEmptyLine() const
{
    return atEmptyLine(position());
}

bool FakeVimHandler::Private::atBoundary(bool end, bool simple, bool onlyWords,
    const QTextCursor &tc) const
{
    if (tc.isNull())
        return atBoundary(end, simple, onlyWords, m_cursor);
    if (atEmptyLine(tc))
        return true;
    int pos = tc.position();
    QChar c1 = characterAt(pos);
    QChar c2 = characterAt(pos + (end ? 1 : -1));
    int thisClass = charClass(c1, simple);
    return (!onlyWords || thisClass != 0)
        && (c2.isNull() || c2 == ParagraphSeparator || thisClass != charClass(c2, simple));
}

bool FakeVimHandler::Private::atWordBoundary(bool end, bool simple, const QTextCursor &tc) const
{
    return atBoundary(end, simple, true, tc);
}

bool FakeVimHandler::Private::atWordStart(bool simple, const QTextCursor &tc) const
{
    return atWordBoundary(false, simple, tc);
}

bool FakeVimHandler::Private::atWordEnd(bool simple, const QTextCursor &tc) const
{
    return atWordBoundary(true, simple, tc);
}

bool FakeVimHandler::Private::isFirstNonBlankOnLine(int pos)
{
    for (int i = blockAt(pos).position(); i < pos; ++i) {
        if (!document()->characterAt(i).isSpace())
            return false;
    }
    return true;
}

void FakeVimHandler::Private::pushUndoState(bool overwrite)
{
    if (m_buffer->editBlockLevel != 0 && m_buffer->undoState.isValid())
        return; // No need to save undo state for inner edit blocks.

    if (m_buffer->undoState.isValid() && !overwrite)
        return;

    UNDO_DEBUG("PUSH UNDO");
    int pos = position();
    if (!isInsertMode()) {
        if (isVisualMode() || g.submode == DeleteSubMode
            || (g.submode == ChangeSubMode && g.movetype != MoveLineWise)) {
            pos = qMin(pos, anchor());
            if (isVisualLineMode())
                pos = firstPositionInLine(lineForPosition(pos));
            else if (isVisualBlockMode())
                pos = blockAt(pos).position() + qMin(columnAt(anchor()), columnAt(position()));
        } else if (g.movetype == MoveLineWise && s.startOfLine()) {
            QTextCursor tc = m_cursor;
            if (g.submode == ShiftLeftSubMode || g.submode == ShiftRightSubMode
                || g.submode == IndentSubMode) {
                pos = qMin(pos, anchor());
            }
            tc.setPosition(pos);
            moveToFirstNonBlankOnLine(&tc);
            pos = qMin(pos, tc.position());
        }
    }

    CursorPosition lastChangePosition(document(), pos);
    setMark('.', lastChangePosition);

    m_buffer->redo.clear();
    m_buffer->undoState = State(
                revision(), lastChangePosition, m_buffer->marks,
                m_buffer->lastVisualMode, m_buffer->lastVisualModeInverted);
}

void FakeVimHandler::Private::moveDown(int n)
{
    if (n == 0)
        return;

    QTextBlock block = m_cursor.block();
    const int col = position() - block.position();

    int lines = qAbs(n);
    int position = 0;
    while (block.isValid()) {
        position = block.position() + qMax(0, qMin(block.length() - 2, col));
        if (block.isVisible()) {
            --lines;
            if (lines < 0)
                break;
        }
        block = n > 0 ? nextLine(block) : previousLine(block);
    }

    setPosition(position);
    moveToTargetColumn();
    updateScrollOffset();
}

void FakeVimHandler::Private::moveDownVisually(int n)
{
    const QTextCursor::MoveOperation moveOperation = (n > 0) ? Down : Up;
    int count = qAbs(n);
    int oldPos = m_cursor.position();

    while (count > 0) {
        m_cursor.movePosition(moveOperation, KeepAnchor, 1);
        if (oldPos == m_cursor.position())
            break;
        oldPos = m_cursor.position();
        QTextBlock block = m_cursor.block();
        if (block.isVisible())
            --count;
    }

    QTextCursor tc = m_cursor;
    tc.movePosition(StartOfLine);
    const int minPos = tc.position();
    moveToEndOfLineVisually(&tc);
    const int maxPos = tc.position();

    if (m_targetColumn == -1) {
        setPosition(maxPos);
    } else {
        setPosition(qMin(maxPos, minPos + m_targetColumnWrapped));
        const int targetColumn = m_targetColumnWrapped;
        setTargetColumn();
        m_targetColumnWrapped = targetColumn;
    }

    if (!isInsertMode() && atEndOfLine())
        m_cursor.movePosition(Left, KeepAnchor);

    updateScrollOffset();
}

void FakeVimHandler::Private::movePageDown(int count)
{
    const int scrollOffset = windowScrollOffset();
    const int screenLines = linesOnScreen();
    const int offset = count > 0 ? scrollOffset - 2 : screenLines - scrollOffset + 2;
    const int value = count * screenLines - cursorLineOnScreen() + offset;
    moveDown(value);

    if (count > 0)
        scrollToLine(cursorLine());
    else
        scrollToLine(qMax(0, cursorLine() - screenLines + 1));
}

void FakeVimHandler::Private::commitCursor()
{
    QTextCursor tc = m_cursor;

    if (isVisualMode()) {
        int pos = tc.position();
        int anc = tc.anchor();

        if (isVisualBlockMode()) {
            const int col1 = columnAt(anc);
            const int col2 = columnAt(pos);
            if (col1 > col2)
                ++anc;
            else if (!tc.atBlockEnd())
                ++pos;
        } else if (isVisualLineMode()) {
            const int posLine = lineForPosition(pos);
            const int ancLine = lineForPosition(anc);
            if (anc < pos) {
                pos = lastPositionInLine(posLine);
                anc = firstPositionInLine(ancLine);
            } else {
                pos = firstPositionInLine(posLine);
                anc = lastPositionInLine(ancLine) + 1;
            }
            // putting cursor on folded line will unfold the line, so move the cursor a bit
            if (!blockAt(pos).isVisible())
                ++pos;
        } else if (isVisualCharMode()) {
            if (anc > pos)
                ++anc;
            else if (!editor()->hasFocus() || isCommandLineMode())
                m_fixCursorTimer.start();
        }

        tc.setPosition(anc);
        tc.setPosition(pos, KeepAnchor);
    } else if (g.subsubmode == SearchSubSubMode && !m_searchCursor.isNull()) {
        tc = m_searchCursor;
    } else {
        tc.clearSelection();
    }

    updateCursorShape();

    if (isVisualBlockMode()) {
        q->requestSetBlockSelection(tc, m_visualTargetColumn == -1);
    } else  {
        q->requestDisableBlockSelection();
        if (editor())
            EDITOR(setTextCursor(tc));
    }
}

void FakeVimHandler::Private::pullCursor()
{
    if (!m_cursorNeedsUpdate)
        return;

    m_cursorNeedsUpdate = false;

    QTextCursor oldCursor = m_cursor;

    bool visualBlockMode = false;
    q->requestHasBlockSelection(&visualBlockMode);

    if (visualBlockMode)
        q->requestBlockSelection(&m_cursor);
    else if (editor())
        m_cursor = editorCursor();

    // Cursor should be always valid.
    if (m_cursor.isNull())
        m_cursor = QTextCursor(document());

    if (visualBlockMode)
        g.visualMode = VisualBlockMode;
    else if (m_cursor.hasSelection())
        g.visualMode = VisualCharMode;
    else
        g.visualMode = NoVisualMode;

    // Keep visually the text selection same.
    // With thick text cursor, the character under cursor is treated as selected.
    if (isVisualCharMode() && hasThinCursor())
        moveLeft();

    // Cursor position can be after the end of line only in some modes.
    if (atEndOfLine() && !isVisualMode() && !isInsertMode())
        moveLeft();

    // Record external jump to different line.
    if (lineForPosition(position()) != lineForPosition(oldCursor.position()))
        recordJump(oldCursor.position());

    setTargetColumn();
}

QTextCursor FakeVimHandler::Private::editorCursor() const
{
    QTextCursor tc = EDITOR(textCursor());
    tc.setVisualNavigation(false);
    return tc;
}

bool FakeVimHandler::Private::moveToNextParagraph(int count)
{
    const bool forward = count > 0;
    int repeat = forward ? count : -count;
    QTextBlock block = this->block();

    if (block.isValid() && block.length() == 1)
        ++repeat;

    for (; block.isValid(); block = forward ? block.next() : block.previous()) {
        if (block.length() == 1) {
            if (--repeat == 0)
                break;
            while (block.isValid() && block.length() == 1)
                block = forward ? block.next() : block.previous();
            if (!block.isValid())
                break;
        }
    }

    if (!block.isValid())
        --repeat;

    if (repeat > 0)
        return false;

    if (block.isValid())
        setPosition(block.position());
    else
        setPosition(forward ? lastPositionInDocument() : 0);

    return true;
}

void FakeVimHandler::Private::moveToParagraphStartOrEnd(int direction)
{
    bool emptyLine = atEmptyLine();
    int oldPos = -1;

    while (atEmptyLine() == emptyLine && oldPos != position()) {
        oldPos = position();
        moveDown(direction);
    }

    if (oldPos != position())
        moveUp(direction);
}

void FakeVimHandler::Private::moveToEndOfLine()
{
    // Additionally select (in visual mode) or apply current command on hidden lines following
    // the current line.
    bool onlyVisibleLines = isVisualMode() || g.submode != NoSubMode;
    const int id = onlyVisibleLines ? lineNumber(block()) : block().blockNumber() + 1;
    setPosition(lastPositionInLine(id, onlyVisibleLines));
    setTargetColumn();
}

void FakeVimHandler::Private::moveToEndOfLineVisually()
{
    moveToEndOfLineVisually(&m_cursor);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToEndOfLineVisually(QTextCursor *tc)
{
    // Moving to end of line ends up on following line if the line is wrapped.
    tc->movePosition(StartOfLine);
    const int minPos = tc->position();
    tc->movePosition(EndOfLine);
    int maxPos = tc->position();
    tc->movePosition(StartOfLine);
    if (minPos != tc->position())
        --maxPos;
    tc->setPosition(maxPos);
}

void FakeVimHandler::Private::moveBehindEndOfLine()
{
    q->fold(1, false);
    int pos = qMin(block().position() + block().length() - 1,
        lastPositionInDocument() + 1);
    setPosition(pos);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToStartOfLine()
{
    setPosition(block().position());
    setTargetColumn();
}

void FakeVimHandler::Private::moveToStartOfLineVisually()
{
    m_cursor.movePosition(StartOfLine, KeepAnchor);
    setTargetColumn();
}

void FakeVimHandler::Private::fixSelection()
{
    if (g.rangemode == RangeBlockMode)
         return;

    if (g.movetype == MoveInclusive) {
        // If position or anchor is after end of non-empty line, include line break in selection.
        if (characterAtCursor() == ParagraphSeparator) {
            if (!atEmptyLine() && !atDocumentEnd()) {
                setPosition(position() + 1);
                return;
            }
        } else if (characterAt(anchor()) == ParagraphSeparator) {
            QTextCursor tc = m_cursor;
            tc.setPosition(anchor());
            if (!atEmptyLine(tc)) {
                setAnchorAndPosition(anchor() + 1, position());
                return;
            }
        }
    }

    if (g.movetype == MoveExclusive && g.subsubmode == NoSubSubMode) {
        if (anchor() < position() && atBlockStart()) {
            // Exclusive motion ending at the beginning of line
            // becomes inclusive and end is moved to end of previous line.
            g.movetype = MoveInclusive;
            moveToStartOfLine();
            moveLeft();

            // Exclusive motion ending at the beginning of line and
            // starting at or before first non-blank on a line becomes linewise.
            if (anchor() < block().position() && isFirstNonBlankOnLine(anchor()))
                g.movetype = MoveLineWise;
        }
    }

    if (g.movetype == MoveLineWise)
        g.rangemode = (g.submode == ChangeSubMode)
            ? RangeLineModeExclusive
            : RangeLineMode;

    if (g.movetype == MoveInclusive) {
        if (anchor() <= position()) {
            if (!atBlockEnd())
                setPosition(position() + 1); // correction

            // Omit first character in selection if it's line break on non-empty line.
            int start = anchor();
            int end = position();
            if (afterEndOfLine(document(), start) && start > 0) {
                start = qMin(start + 1, end);
                if (g.submode == DeleteSubMode && !atDocumentEnd())
                    setAnchorAndPosition(start, end + 1);
                else
                    setAnchorAndPosition(start, end);
            }

            // If more than one line is selected and all are selected completely
            // movement becomes linewise.
            if (start < block().position() && isFirstNonBlankOnLine(start) && atBlockEnd()) {
                if (g.submode != ChangeSubMode) {
                    moveRight();
                    if (atEmptyLine())
                        moveRight();
                }
                g.movetype = MoveLineWise;
            }
        } else if (!m_anchorPastEnd) {
            setAnchorAndPosition(anchor() + 1, position());
        }
    }

    if (m_positionPastEnd) {
        moveBehindEndOfLine();
        moveRight();
        setAnchorAndPosition(anchor(), position());
    }

    if (m_anchorPastEnd) {
        const int pos = position();
        setPosition(anchor());
        moveBehindEndOfLine();
        moveRight();
        setAnchorAndPosition(position(), pos);
    }
}

bool FakeVimHandler::Private::finishSearch()
{
    if (g.lastSearch.isEmpty()
        || (!g.currentMessage.isEmpty() && g.currentMessageLevel == MessageError)) {
        return false;
    }
    if (g.submode != NoSubMode)
        setAnchorAndPosition(m_searchStartPosition, position());
    return true;
}

void FakeVimHandler::Private::finishMovement(const QString &dotCommandMovement)
{
    //dump("FINISH MOVEMENT");
    if (g.submode == FilterSubMode) {
        int beginLine = lineForPosition(anchor());
        int endLine = lineForPosition(position());
        setPosition(qMin(anchor(), position()));
        enterExMode(QString(".,+%1!").arg(qAbs(endLine - beginLine)));
        return;
    }

    if (g.submode == ChangeSubMode
        || g.submode == DeleteSubMode
        || g.submode == CommentSubMode
        || g.submode == ExchangeSubMode
        || g.submode == ReplaceWithRegisterSubMode
        || g.submode == AddSurroundingSubMode
        || g.submode == YankSubMode
        || g.submode == InvertCaseSubMode
        || g.submode == DownCaseSubMode
        || g.submode == UpCaseSubMode
        || g.submode == ReflowSubMode
        || g.submode == ReflowKeepCursorSubMode
        || g.submode == OperatorFuncSubMode
        || g.submode == IndentSubMode
        || g.submode == ShiftLeftSubMode
        || g.submode == ShiftRightSubMode)
    {
        fixSelection();

        if (g.submode == ChangeSubMode
            || g.submode == DeleteSubMode
            || g.submode == YankSubMode)
        {
            yankText(currentRange(), m_register);
        }
    }

    if (g.submode == ChangeSubMode) {
        pushUndoState(false);
        beginEditBlock();
        removeText(currentRange());
        if (g.movetype == MoveLineWise)
            insertAutomaticIndentation(true);
        endEditBlock();
        setTargetColumn();
    } else if (g.submode == CommentSubMode) {
        pushUndoState(false);
        beginEditBlock();
        toggleComment(currentRange());
        endEditBlock();
    } else if (g.submode == AddSurroundingSubMode) {
        g.subsubmode = SurroundSubSubMode;
        g.dotCommand = dotCommandMovement;

        // We now only know the region that should be surrounded, but not the actual
        // character that should surround it. We thus do NOT want to finish the
        // movement yet here, so we return early.
        // The next character entered will be used by the SurroundSubSubMode.
        return;
    } else if (g.submode == OperatorFuncSubMode) {
        pushUndoState(false);
        beginEditBlock();
        callOperatorFunc(currentRange());
        endEditBlock();
    } else if (g.submode == ExchangeSubMode) {
        exchangeRange(currentRange());
    } else if (g.submode == ReplaceWithRegisterSubMode && s.emulateReplaceWithRegister()) {
        pushUndoState(false);
        beginEditBlock();
        replaceWithRegister(currentRange());
        endEditBlock();
    } else if (g.submode == DeleteSubMode) {
        pushUndoState(false);
        beginEditBlock();
        const int pos = position();
        // Always delete something (e.g. 'dw' on an empty line deletes the line).
        if (pos == anchor() && g.movetype == MoveInclusive)
            removeText(Range(pos, pos + 1));
        else
            removeText(currentRange());
        if (g.movetype == MoveLineWise)
            handleStartOfLine();
        endEditBlock();
    } else if (g.submode == YankSubMode) {
        bool isVisualModeYank = isVisualMode();
        leaveVisualMode();
        const QTextCursor tc = m_cursor;
        if (g.rangemode == RangeBlockMode) {
            const int pos1 = tc.block().position();
            const int pos2 = blockAt(tc.anchor()).position();
            const int col = qMin(tc.position() - pos1, tc.anchor() - pos2);
            setPosition(qMin(pos1, pos2) + col);
        } else {
            setPosition(qMin(position(), anchor()));
            if (g.rangemode == RangeLineMode) {
                if (isVisualModeYank)
                    moveToStartOfLine();
                else
                    moveToTargetColumn();
            }
        }
        setTargetColumn();
    } else if (g.submode == InvertCaseSubMode
        || g.submode == UpCaseSubMode
        || g.submode == DownCaseSubMode) {
        beginEditBlock();
        if (g.submode == InvertCaseSubMode)
            invertCase(currentRange());
        else if (g.submode == DownCaseSubMode)
            downCase(currentRange());
        else if (g.submode == UpCaseSubMode)
            upCase(currentRange());
        if (g.movetype == MoveLineWise)
            handleStartOfLine();
        endEditBlock();
    } else if (g.submode == IndentSubMode
        || g.submode == ShiftRightSubMode
        || g.submode == ShiftLeftSubMode) {
        recordJump();
        pushUndoState(false);
        if (g.submode == IndentSubMode)
            indentSelectedText();
        else if (g.submode == ShiftRightSubMode)
            shiftRegionRight(1);
        else if (g.submode == ShiftLeftSubMode)
            shiftRegionLeft(1);
    } else if (g.submode == ReflowSubMode || g.submode == ReflowKeepCursorSubMode) {
        const bool keepCursor = g.submode == ReflowKeepCursorSubMode;
        pushUndoState(false);
        beginEditBlock();
        reflowText(currentRange());
        endEditBlock();
        if (keepCursor) {
            // "gw" leaves the cursor where it was before the command.
            const int line = qMin(m_reflowSavedLine, document()->blockCount() - 1);
            const QTextBlock b = document()->findBlockByNumber(line);
            setPosition(b.position() + qMin(m_reflowSavedColumn, qMax(b.length() - 1, 0)));
            setTargetColumn();
        } else if (g.movetype == MoveLineWise) {
            handleStartOfLine();
        }
    }

    if (!dotCommandMovement.isEmpty()) {
        QString dotCommand = dotCommandFromSubMode(g.submode);
        if (!dotCommand.isEmpty()) {
            if (g.submode == ReplaceWithRegisterSubMode)
                dotCommand = QString("\"%1%2").arg(QChar(m_register)).arg(dotCommand);

            setDotCommand(dotCommand + dotCommandMovement);
        }
    }

    // Change command continues in insert mode.
    if (g.submode == ChangeSubMode) {
        clearCurrentMode();
        enterInsertMode();
    } else {
        leaveCurrentMode();
    }
}

void FakeVimHandler::Private::leaveCurrentMode()
{
    if (isVisualMode())
        enterCommandMode(g.returnToMode);
    else if (g.returnToMode == CommandMode)
        enterCommandMode();
    else if (g.returnToMode == InsertMode)
        enterInsertMode();
    else
        enterReplaceMode();

    if (isNoVisualMode())
        setAnchor();
}

void FakeVimHandler::Private::clearCurrentMode()
{
    g.submode = NoSubMode;
    g.subsubmode = NoSubSubMode;
    g.movetype = MoveInclusive;
    g.gflag = false;
    g.surroundUpperCaseS = false;
    g.surroundFunction.clear();
    m_register = '"';
    g.rangemode = RangeCharMode;
    g.currentCommand.clear();
    resetCount();
}

void FakeVimHandler::Private::updateSelection()
{
    QList<QTextEdit::ExtraSelection> selections = m_extraSelections;
    if (s.showMarks()) {
        // Show only user-defined marks (a-z, A-Z). Vim does not paint the
        // automatic positional marks (', `, ., <, >, ...) into the buffer;
        // doing so highlighted positions the user never set and that kept
        // moving while editing (QTCREATORBUG-7790).
        const QPalette pal = EDITOR(palette());
        for (auto it = m_buffer->marks.cbegin(), end = m_buffer->marks.cend(); it != end; ++it) {
            if (!it.key().isLetter())
                continue;
            QTextEdit::ExtraSelection sel;
            sel.cursor = m_cursor;
            setCursorPosition(&sel.cursor, it.value().position(document()));
            sel.cursor.setPosition(sel.cursor.position(), MoveAnchor);
            sel.cursor.movePosition(Right, KeepAnchor);
            sel.format = m_cursor.blockCharFormat();
            sel.format.setForeground(pal.color(QPalette::Base));
            sel.format.setBackground(pal.color(QPalette::Text));
            selections.append(sel);
        }
    }
    //qDebug() << "SELECTION: " << selections;
    q->selectionChanged(selections);
}

void FakeVimHandler::Private::updateHighlights()
{
    if (s.useCoreSearch() || !s.hlSearch() || g.highlightsCleared) {
        if (m_highlighted.isEmpty())
            return;
        m_highlighted.clear();
    } else if (m_highlighted != g.lastNeedle) {
        m_highlighted = g.lastNeedle;
    } else {
        return;
    }

    q->highlightMatches(m_highlighted);
}

void FakeVimHandler::Private::updateMiniBuffer()
{
    if (!hasValidEditor())
        return;

    QString msg;
    int cursorPos = -1;
    int anchorPos = -1;
    MessageLevel messageLevel = MessageMode;

    if (!g.mapStates.isEmpty() && g.mapStates.last().silent && g.currentMessageLevel < MessageInfo)
        g.currentMessage.clear();

    if (g.passing) {
        msg = "PASSING";
    } else if (g.subsubmode == SearchSubSubMode) {
        msg = g.searchBuffer.display();
        if (g.mapStates.isEmpty()) {
            cursorPos = g.searchBuffer.cursorPos() + 1;
            anchorPos = g.searchBuffer.anchorPos() + 1;
        }
    } else if (g.subsubmode == ExpressionSubSubMode) {
        msg = g.commandBuffer.display();
        if (g.mapStates.isEmpty()) {
            cursorPos = g.commandBuffer.cursorPos() + 1;
            anchorPos = g.commandBuffer.anchorPos() + 1;
        }
    } else if (g.mode == ExMode) {
        msg = g.commandBuffer.display();
        if (g.mapStates.isEmpty()) {
            cursorPos = g.commandBuffer.cursorPos() + 1;
            anchorPos = g.commandBuffer.anchorPos() + 1;
        }
    } else if (!g.currentMessage.isEmpty()) {
        msg = g.currentMessage;
        g.currentMessage.clear();
        messageLevel = g.currentMessageLevel;
    } else if (!g.mapStates.isEmpty() && !g.mapStates.last().silent) {
        // Do not reset previous message when after running a mapped command.
        return;
    } else if (g.mode == CommandMode && !g.currentCommand.isEmpty() && s.showCmd()) {
        msg = g.currentCommand;
        messageLevel = MessageShowCmd;
    } else if (g.mode == CommandMode && isVisualMode()) {
        if (isVisualCharMode())
            msg = "-- VISUAL --";
        else if (isVisualLineMode())
            msg = "-- VISUAL LINE --";
        else if (isVisualBlockMode())
            msg = "VISUAL BLOCK";
    } else if (g.mode == InsertMode) {
        msg = "-- INSERT --";
        if (g.submode == CtrlRSubMode)
            msg += " ^R";
        else if (g.submode == CtrlVSubMode)
            msg += " ^V";
    } else if (g.mode == ReplaceMode) {
        msg = "-- REPLACE --";
    } else {
        if (g.returnToMode == CommandMode)
            msg = "-- COMMAND --";
        else if (g.returnToMode == InsertMode)
            msg = "-- (insert) --";
        else
            msg = "-- (replace) --";
    }

    if (g.isRecording && msg.startsWith("--"))
        msg.append(' ').append("Recording");

    q->commandBufferChanged(msg, cursorPos, anchorPos, messageLevel);

    int linesInDoc = linesInDocument();
    int l = cursorLine();
    QString status;
    const QString pos = QString("%1,%2")
        .arg(l + 1).arg(physicalCursorColumn() + 1);
    // FIXME: physical "-" logical
    if (linesInDoc != 0)
        status = Tr::tr("%1%2%").arg(pos, -10).arg(l * 100 / linesInDoc, 4);
    else
        status = Tr::tr("%1All").arg(pos, -10);
    q->statusDataChanged(status);
}

void FakeVimHandler::Private::showMessage(MessageLevel level, const QString &msg)
{
    //qDebug() << "MSG: " << msg;
    // ":silent" suppresses ordinary messages; ":silent!" also suppresses errors.
    if (m_messageSilence > 0 && (level != MessageError || m_silenceErrors))
        return;
    g.currentMessage = msg;
    g.currentMessageLevel = level;
}

void FakeVimHandler::Private::notImplementedYet()
{
    qDebug() << "Not implemented in FakeVim";
    showMessage(MessageError, Tr::tr("Not implemented in FakeVim."));
}

void FakeVimHandler::Private::passShortcuts(bool enable)
{
    g.passing = enable;
    updateMiniBuffer();
    if (enable)
        QCoreApplication::instance()->installEventFilter(q);
    else
        QCoreApplication::instance()->removeEventFilter(q);
}

bool FakeVimHandler::Private::handleCommandSubSubMode(const Input &input)
{
    bool handled = true;

    if (g.subsubmode == FtSubSubMode) {
        g.semicolonType = g.subsubdata;
        g.semicolonKey = input.text();
        handled = handleFfTt(g.semicolonKey);
        g.subsubmode = NoSubSubMode;
        if (handled) {
            finishMovement(QString("%1%2%3")
                           .arg(count())
                           .arg(g.semicolonType.text())
                           .arg(g.semicolonKey));
        }
    } else if (g.subsubmode == TextObjectSubSubMode) {
        // vim-surround treats aw and aW the same as iw and iW, respectively
        if ((input.is('w') || input.is('W'))
                && g.submode == AddSurroundingSubMode && g.subsubdata.is('a'))
            g.subsubdata = Input('i');

        if (input.is('w'))
            selectWordTextObject(g.subsubdata.is('i'));
        else if (input.is('W'))
            selectWORDTextObject(g.subsubdata.is('i'));
        else if (input.is('s'))
            selectSentenceTextObject(g.subsubdata.is('i'));
        else if (input.is('p'))
            selectParagraphTextObject(g.subsubdata.is('i'));
        else if (input.is('[') || input.is(']'))
            handled = selectBlockTextObject(g.subsubdata.is('i'), '[', ']');
        else if (input.is('(') || input.is(')') || input.is('b'))
            handled = selectBlockTextObject(g.subsubdata.is('i'), '(', ')');
        else if (input.is('<') || input.is('>'))
            handled = selectBlockTextObject(g.subsubdata.is('i'), '<', '>');
        else if (input.is('{') || input.is('}') || input.is('B'))
            handled = selectBlockTextObject(g.subsubdata.is('i'), '{', '}');
        else if (input.is('"') || input.is('\'') || input.is('`'))
            handled = selectQuotedStringTextObject(g.subsubdata.is('i'), input.asChar());
        else if (input.is('a') && s.emulateArgTextObj())
            handled = selectArgumentTextObject(g.subsubdata.is('i'));
        else if (input.is('t'))
            handled = selectTagTextObject(g.subsubdata.is('i'));
        else
            handled = false;
        g.subsubmode = NoSubSubMode;
        if (handled) {
            finishMovement(QString("%1%2%3")
                           .arg(count())
                           .arg(g.subsubdata.text())
                           .arg(input.text()));
        }
    } else if (g.subsubmode == MarkSubSubMode) {
        setMark(input.asChar(), CursorPosition(m_cursor));
        g.subsubmode = NoSubSubMode;
    } else if (g.subsubmode == BackTickSubSubMode
            || g.subsubmode == TickSubSubMode) {
        handled = jumpToMark(input.asChar(), g.subsubmode == BackTickSubSubMode);
        if (handled)
            finishMovement();
        g.subsubmode = NoSubSubMode;
    } else if (g.subsubmode == ZSubSubMode) {
        handled = false;
        if (input.is('j') || input.is('k')) {
            int pos = position();
            q->foldGoTo(input.is('j') ? count() : -count(), false);
            if (pos != position()) {
                handled = true;
                finishMovement(QString("%1z%2")
                               .arg(count())
                               .arg(input.text()));
            }
        }
    } else if (g.subsubmode == OpenSquareSubSubMode || g.subsubmode == CloseSquareSubSubMode) {
        if (s.emulateVimUnimpaired()
                && handleVimUnimpaired(g.subsubmode == CloseSquareSubSubMode, input)) {
            g.subsubmode = NoSubSubMode;
            handled = true;
        } else {
            int pos = position();
            if (input.is('{') && g.subsubmode == OpenSquareSubSubMode)
                searchBalanced(false, '{', '}');
            else if (input.is('}') && g.subsubmode == CloseSquareSubSubMode)
                searchBalanced(true, '}', '{');
            else if (input.is('(') && g.subsubmode == OpenSquareSubSubMode)
                searchBalanced(false, '(', ')');
            else if (input.is(')') && g.subsubmode == CloseSquareSubSubMode)
                searchBalanced(true, ')', '(');
            else if (input.is('[') && g.subsubmode == OpenSquareSubSubMode)
                bracketSearchBackward(&m_cursor, "^\\{", count());
            else if (input.is('[') && g.subsubmode == CloseSquareSubSubMode)
                bracketSearchForward(&m_cursor, "^\\}", count(), false);
            else if (input.is(']') && g.subsubmode == OpenSquareSubSubMode)
                bracketSearchBackward(&m_cursor, "^\\}", count());
            else if (input.is(']') && g.subsubmode == CloseSquareSubSubMode)
                bracketSearchForward(&m_cursor, "^\\{", count(), g.submode != NoSubMode);
            else if (input.is('z'))
                q->foldGoTo(g.subsubmode == OpenSquareSubSubMode ? -count() : count(), true);
            handled = pos != position();
            if (handled) {
                if (lineForPosition(pos) != lineForPosition(position()))
                    recordJump(pos);
                finishMovement(QString("%1%2%3")
                               .arg(count())
                               .arg(g.subsubmode == OpenSquareSubSubMode ? '[' : ']')
                               .arg(input.text()));
            }
        }
    } else if (g.subsubmode == SurroundWithFunctionSubSubMode) {
        if (input.isReturn()) {
            pushUndoState(false);
            beginEditBlock();

            const QString dotCommand = "ys" + g.dotCommand + "f" + g.surroundFunction + "<CR>";

            surroundCurrentRange(Input(')'), g.surroundFunction);

            g.dotCommand = dotCommand;

            endEditBlock();
            leaveCurrentMode();
        } else {
            g.surroundFunction += input.asChar();
        }
        return true;
    } else if (g.subsubmode == SurroundSubSubMode) {
        if (input.is('f') && g.submode == AddSurroundingSubMode) {
            g.subsubmode = SurroundWithFunctionSubSubMode;
            g.commandBuffer.setContents("");
            return true;
        }

        pushUndoState(false);
        beginEditBlock();

        surroundCurrentRange(input);

        endEditBlock();
        leaveCurrentMode();
    } else {
        handled = false;
    }
    return handled;
}

bool FakeVimHandler::Private::handleVimUnimpaired(bool close, const Input &input)
{
    // A small subset of tpope's vim-unimpaired, entered via "[" / "]".
    if (input.is(' ')) {
        // "[<Space>" / "]<Space>": add [count] blank lines above/below,
        // leaving the cursor on the current line.
        const QTextBlock b = block();
        pushUndoState(false);
        beginEditBlock();
        QTextCursor tc = m_cursor;
        tc.setPosition(close ? b.position() + b.length() - 1 : b.position());
        tc.insertText(QString(count(), QLatin1Char('\n')));
        endEditBlock();
        setTargetColumn();
        return true;
    }

    if (input.is('e')) {
        // "[e" / "]e": move the current line up/down by [count] lines,
        // keeping the cursor on the moved line and in the same column.
        const int line = cursorLine() + 1; // 1-based
        const int lastLine = document()->blockCount();
        const int target = close ? qMin(line + count(), lastLine)
                                 : qMax(line - count() - 1, 0);
        if ((close && target > line) || (!close && target < line - 1)) {
            const int column = m_cursor.positionInBlock();
            handleExCommand(QString("move %1").arg(target));
            const QTextBlock b = block();
            setPosition(b.position() + qMin(column, qMax(b.length() - 1, 0)));
            setTargetColumn();
        }
        return true;
    }

    return false;
}

bool FakeVimHandler::Private::handleCount(const Input &input)
{
    if (!isInputCount(input))
        return false;
    g.mvcount = g.mvcount * 10 + input.text().toInt();
    return true;
}

bool FakeVimHandler::Private::handleMovement(const Input &input)
{
    bool handled = true;
    int count = this->count();

    if (handleCount(input)) {
        return true;
    } else if (input.is('0')) {
        g.movetype = MoveExclusive;
        if (g.gflag)
            moveToStartOfLineVisually();
        else
            moveToStartOfLine();
        count = 1;
    } else if (input.is('a') || input.is('i')) {
        g.subsubmode = TextObjectSubSubMode;
        g.subsubdata = input;
    } else if (input.is('^') || input.is('_')) {
        if (g.gflag)
            moveToFirstNonBlankOnLineVisually();
        else
            moveToFirstNonBlankOnLine();
        g.movetype = MoveExclusive;
    } else if (!s.commaPassesShortcuts() && input.is(',')) {
        // Repeat the last f/F/t/T in the opposite direction. Only when ',' is
        // not reserved for passing shortcuts (QTCREATORBUG-12115).
        g.subsubmode = FtSubSubMode;
        // Reverse direction by toggling the case: f <-> F, t <-> T.
        const int c = g.semicolonType.asChar().unicode();
        g.subsubdata = Input(QChar(c ? c ^ 0x20 : c));
        handleFfTt(g.semicolonKey, true);
        g.subsubmode = NoSubSubMode;
    } else if (input.is(';')) {
        g.subsubmode = FtSubSubMode;
        g.subsubdata = g.semicolonType;
        handleFfTt(g.semicolonKey, true);
        g.subsubmode = NoSubSubMode;
    } else if (input.is('/') || input.is('?')) {
        g.lastSearchForward = input.is('/');
        // The core search dialog cannot carry out a pending operator (d/, c/,
        // y/, ...), so use the built-in search for those (QTCREATORBUG-24172).
        if (s.useCoreSearch() && g.submode == NoSubMode) {
            // re-use the core dialog.
            g.findPending = true;
            m_findStartPosition = position();
            g.movetype = MoveExclusive;
            setAnchor(); // clear selection: otherwise, search is restricted to selection
            q->findRequested(!g.lastSearchForward);
        } else {
            // FIXME: make core find dialog sufficiently flexible to
            // produce the "default vi" behaviour too. For now, roll our own.
            g.currentMessage.clear();
            g.movetype = MoveExclusive;
            g.subsubmode = SearchSubSubMode;
            g.searchBuffer.setPrompt(g.lastSearchForward ? '/' : '?');
            m_searchStartPosition = position();
            m_searchFromScreenLine = firstVisibleLine();
            m_searchCursor = QTextCursor();
            g.searchBuffer.clear();
        }
    } else if (input.is('`')) {
        g.subsubmode = BackTickSubSubMode;
    } else if (input.is('#') || input.is('*')) {
        // FIXME: That's not proper vim behaviour
        QString needle;
        QTextCursor tc = m_cursor;
        tc.select(QTextCursor::WordUnderCursor);
        needle = QRegularExpression::escape(tc.selection().toPlainText());
        if (!g.gflag) {
            needle.prepend("\\<");
            needle.append("\\>");
        }
        setAnchorAndPosition(tc.position(), tc.anchor());
        g.searchBuffer.historyPush(needle);
        g.lastSearch = needle;
        g.lastSearchForward = input.is('*');
        handled = searchNext();
    } else if (input.is('\'')) {
        g.subsubmode = TickSubSubMode;
        if (g.submode != NoSubMode)
            g.movetype = MoveLineWise;
    } else if (input.is('|')) {
        moveToStartOfLine();
        const int column = count - 1;
        moveRight(qMin(column, rightDist() - 1));
        m_targetColumn = column;
        m_visualTargetColumn = column;
    } else if (input.is('{') || input.is('}')) {
        const int oldPosition = position();
        handled = input.is('}')
            ? moveToNextParagraph(count)
            : moveToPreviousParagraph(count);
        if (handled) {
            recordJump(oldPosition);
            setTargetColumn();
            g.movetype = MoveExclusive;
        }
    } else if (input.isReturn()) {
        moveToStartOfLine();
        moveDown();
        moveToFirstNonBlankOnLine();
    } else if (input.is('-')) {
        moveToStartOfLine();
        moveUp(count);
        moveToFirstNonBlankOnLine();
    } else if (input.is('+')) {
        moveToStartOfLine();
        moveDown(count);
        moveToFirstNonBlankOnLine();
    } else if (input.isKey(Key_Home)) {
        moveToStartOfLine();
    } else if (input.is('$') || input.isKey(Key_End)) {
        if (g.gflag) {
            if (count > 1)
                moveDownVisually(count - 1);
            moveToEndOfLineVisually();
        } else {
            if (count > 1)
                moveDown(count - 1);
            moveToEndOfLine();
        }
        g.movetype = atEmptyLine() ? MoveExclusive : MoveInclusive;
        if (g.submode == NoSubMode)
            m_targetColumn = -1;
        if (isVisualMode())
            m_visualTargetColumn = -1;
    } else if (input.is('%')) {
        recordJump();
        if (g.mvcount == 0) {
            moveToMatchingParanthesis();
            g.movetype = MoveInclusive;
        } else {
            // set cursor position in percentage - formula taken from Vim help
            setPosition(firstPositionInLine((count * linesInDocument() + 99) / 100));
            moveToTargetColumn();
            handleStartOfLine();
            g.movetype = MoveLineWise;
        }
    } else if (input.is('b') || input.isShift(Key_Left)) {
        moveToNextWordStart(count, false, false);
    } else if (input.is('B') || input.isControl(Key_Left)) {
        moveToNextWordStart(count, true, false);
    } else if (input.is('e') && g.gflag) {
        moveToNextWordEnd(count, false, false);
    } else if (input.is('e')) {
        moveToNextWordEnd(count, false, true, false);
    } else if (input.is('E') && g.gflag) {
        moveToNextWordEnd(count, true, false);
    } else if (input.is('E')) {
        moveToNextWordEnd(count, true, true, false);
    } else if (input.isControl('e')) {
        // Scroll the view down, dragging the cursor along so it stays at least
        // 'scrolloff' lines from the top instead of blocking the scroll there
        // (QTCREATORBUG-34074).
        scrollDown(count);
        if (cursorLine() < lineOnTop())
            moveDownVisually(lineOnTop() - cursorLine());
    } else if (input.is('f')) {
        g.subsubmode = FtSubSubMode;
        g.movetype = MoveInclusive;
        g.subsubdata = input;
    } else if (input.is('F')) {
        g.subsubmode = FtSubSubMode;
        g.movetype = MoveExclusive;
        g.subsubdata = input;
    } else if (!g.gflag && input.is('g')) {
        g.gflag = true;
        return true;
    } else if (input.is('g') || input.is('G')) {
        QString dotCommand = QString("%1G").arg(count);
        recordJump();
        if (input.is('G') && g.mvcount == 0)
            dotCommand = "G";
        int n = (input.is('g')) ? 1 : linesInDocument();
        n = g.mvcount == 0 ? n : count;
        if (g.submode == NoSubMode || g.submode == ZSubMode
                || g.submode == CapitalZSubMode || g.submode == RegisterSubMode) {
            setPosition(firstPositionInLine(n, false));
            handleStartOfLine();
        } else {
            g.movetype = MoveLineWise;
            g.rangemode = RangeLineMode;
            setAnchor();
            setPosition(firstPositionInLine(n, false));
        }
        setTargetColumn();
        updateScrollOffset();
    } else if (input.is('h') || input.isKey(Key_Left) || input.isBackspace()) {
        g.movetype = MoveExclusive;
        int n = qMin(count, leftDist());
        moveLeft(n);
    } else if (input.is('H')) {
        const CursorPosition pos(lineToBlockNumber(lineOnTop(count)), 0);
        setCursorPosition(&m_cursor, pos);
        handleStartOfLine();
    } else if (input.is('j') || input.isKey(Key_Down)
            || input.isControl('j') || input.isControl('n')) {
        moveVertically(count);
    } else if (input.is('k') || input.isKey(Key_Up) || input.isControl('p')) {
        moveVertically(-count);
    } else if (input.is('l') || input.isKey(Key_Right) || input.is(' ')) {
        g.movetype = MoveExclusive;
        moveRight(qMax(0, qMin(count, rightDist() - (g.submode == NoSubMode))));
    } else if (input.is('L')) {
        const CursorPosition pos(lineToBlockNumber(lineOnBottom(count)), 0);
        setCursorPosition(&m_cursor, pos);
        handleStartOfLine();
    } else if (g.gflag && input.is('m')) {
        const QPoint pos(EDITOR(viewport()->width()) / 2, EDITOR(cursorRect(m_cursor)).y());
        QTextCursor tc = EDITOR(cursorForPosition(pos));
        if (!tc.isNull()) {
            m_cursor = tc;
            setTargetColumn();
        }
    } else if (input.is('M')) {
        m_cursor = EDITOR(cursorForPosition(QPoint(0, EDITOR(height()) / 2)));
        handleStartOfLine();
    } else if (input.is('n') || input.is('N')) {
        if (s.useCoreSearch()) {
            bool forward = (input.is('n')) ? g.lastSearchForward : !g.lastSearchForward;
            int pos = position();
            q->findNextRequested(!forward);
            if (forward && pos == m_cursor.selectionStart()) {
                // if cursor is already positioned at the start of a find result, this is returned
                q->findNextRequested(false);
            }
            setPosition(m_cursor.selectionStart());
        } else {
            handled = searchNext(input.is('n'));
        }
    } else if (input.is('t')) {
        g.movetype = MoveInclusive;
        g.subsubmode = FtSubSubMode;
        g.subsubdata = input;
    } else if (input.is('T')) {
        g.movetype = MoveExclusive;
        g.subsubmode = FtSubSubMode;
        g.subsubdata = input;
    } else if (input.is('w') || input.is('W')
            || input.isShift(Key_Right) || input.isControl(Key_Right)) { // tested
        // Special case: "cw" and "cW" work the same as "ce" and "cE" if the
        // cursor is on a non-blank - except if the cursor is on the last
        // character of a word: only the current word will be changed
        bool simple = input.is('W') || input.isControl(Key_Right);
        if (g.submode == ChangeSubMode && !characterAtCursor().isSpace()) {
            moveToWordEnd(count, simple, true);
        } else {
            moveToNextWordStart(count, simple, true);
            // Command 'dw' deletes to the next word on the same line or to end of line.
            if (g.submode == DeleteSubMode && count == 1) {
                const QTextBlock currentBlock = blockAt(anchor());
                setPosition(qMin(position(), currentBlock.position() + currentBlock.length()));
            }
        }
    } else if (input.is('z')) {
        g.movetype =  MoveLineWise;
        g.subsubmode = ZSubSubMode;
    } else if (input.is('[')) {
        g.subsubmode = OpenSquareSubSubMode;
    } else if (input.is(']')) {
        g.subsubmode = CloseSquareSubSubMode;
    } else if (input.isKey(Key_PageDown) || input.isControl('f')) {
        movePageDown(count);
        handleStartOfLine();
    } else if (input.isKey(Key_PageUp) || input.isControl('b')) {
        movePageUp(count);
        handleStartOfLine();
    } else {
        handled = false;
    }

    if (handled && g.subsubmode == NoSubSubMode) {
        if (g.submode == NoSubMode) {
            leaveCurrentMode();
        } else {
            // finish movement for sub modes
            const QString dotMovement =
                (count > 1 ? QString::number(count) : QString())
                + QLatin1String(g.gflag ? "g" : "")
                + input.toString();
            finishMovement(dotMovement);
            setTargetColumn();
        }
    }

    return handled;
}

EventResult FakeVimHandler::Private::handleCommandMode(const Input &input)
{
    bool handled = false;

    bool clearGflag = g.gflag;
    bool clearRegister = g.submode != RegisterSubMode;
    bool clearCount = g.submode != RegisterSubMode && !isInputCount(input);

    // Process input for a sub-mode.
    if (input.isEscape()) {
        handled = handleEscape();
    } else if (EDITOR(isReadOnly())) {
        // Queried live, not cached: a document can become writable later (e.g.
        // reloaded with another encoding), and FakeVim must handle keys again
        // (QTCREATORBUG-24237).
        return EventUnhandled;
    } else if (g.subsubmode != NoSubSubMode) {
        handled = handleCommandSubSubMode(input);
    } else if (g.submode == NoSubMode) {
        handled = handleNoSubMode(input);
    } else if (g.submode == ExchangeSubMode) {
        handled = handleExchangeSubMode(input);
    } else if (g.submode == ChangeSubMode && input.is('x') && s.emulateExchange()) {
        // Exchange submode is "cx", so we need to switch over from ChangeSubMode here
        g.submode = ExchangeSubMode;
        handled = true;
    } else if (g.submode == DeleteSurroundingSubMode
               || g.submode == ChangeSurroundingSubMode) {
        handled = handleDeleteChangeSurroundingSubMode(input);
    } else if (g.submode == AddSurroundingSubMode) {
        handled = handleAddSurroundingSubMode(input);
    } else if (g.submode == ChangeSubMode && (input.is('s') || input.is('S'))
               && s.emulateSurround()) {
        g.submode = ChangeSurroundingSubMode;
        g.surroundUpperCaseS = input.is('S');
        handled = true;
    } else if (g.submode == DeleteSubMode && input.is('s') && s.emulateSurround()) {
        g.submode = DeleteSurroundingSubMode;
        handled = true;
    } else if (g.submode == YankSubMode && (input.is('s') || input.is('S'))
               && s.emulateSurround()) {
        g.submode = AddSurroundingSubMode;
        g.movetype = MoveInclusive;
        g.surroundUpperCaseS = input.is('S');
        handled = true;
    } else if (g.submode == ChangeSubMode
        || g.submode == DeleteSubMode
        || g.submode == YankSubMode) {
        handled = handleChangeDeleteYankSubModes(input);
    } else if (g.submode == CommentSubMode && s.emulateVimCommentary()) {
        handled = handleCommentSubMode(input);
    } else if (g.submode == ReplaceWithRegisterSubMode && s.emulateReplaceWithRegister()) {
        handled = handleReplaceWithRegisterSubMode(input);
    } else if (g.submode == ReplaceSubMode) {
        handled = handleReplaceSubMode(input);
    } else if (g.submode == FilterSubMode) {
        handled = handleFilterSubMode(input);
    } else if (g.submode == RegisterSubMode) {
        handled = handleRegisterSubMode(input);
    } else if (g.submode == WindowSubMode) {
        handled = handleWindowSubMode(input);
    } else if (g.submode == ZSubMode) {
        handled = handleZSubMode(input);
    } else if (g.submode == CapitalZSubMode) {
        handled = handleCapitalZSubMode(input);
    } else if (g.submode == MacroRecordSubMode) {
        handled = handleMacroRecordSubMode(input);
    } else if (g.submode == MacroExecuteSubMode) {
        handled = handleMacroExecuteSubMode(input);
    } else if (g.submode == ShiftLeftSubMode
        || g.submode == ShiftRightSubMode
        || g.submode == IndentSubMode) {
        handled = handleShiftSubMode(input);
    } else if (g.submode == InvertCaseSubMode
        || g.submode == DownCaseSubMode
        || g.submode == UpCaseSubMode) {
        handled = handleChangeCaseSubMode(input);
    } else if (g.submode == ReflowSubMode || g.submode == ReflowKeepCursorSubMode) {
        handled = handleReflowSubMode(input);
    }

    if (!handled && isOperatorPending())
       handled = handleMovement(input);

    // Clear state and display incomplete command if necessary.
    if (handled) {
        bool noMode =
            (g.mode == CommandMode && g.submode == NoSubMode && g.subsubmode == NoSubSubMode);
        clearCount = clearCount && noMode && !g.gflag;
        if (clearCount && clearRegister) {
            leaveCurrentMode();
        } else {
            // Use gflag only for next input.
            if (clearGflag)
                g.gflag = false;
            // Clear [count] and [register] if its no longer needed.
            if (clearCount)
                resetCount();
            // Show or clear current command on minibuffer (showcmd).
            if (input.isEscape() || g.mode != CommandMode || clearCount)
                g.currentCommand.clear();
            else
                g.currentCommand.append(input.toString());
        }

        saveLastVisualMode();
    } else {
        leaveCurrentMode();
        //qDebug() << "IGNORED IN COMMAND MODE: " << key << text
        //    << " VISUAL: " << g.visualMode;

        // if a key which produces text was pressed, don't mark it as unhandled
        // - otherwise the text would be inserted while being in command mode
        if (input.text().isEmpty())
            handled = false;
    }

    m_positionPastEnd = (m_visualTargetColumn == -1) && isVisualMode() && !atEmptyLine();

    return handled ? EventHandled : EventCancelled;
}

bool FakeVimHandler::Private::handleEscape()
{
    if (isVisualMode())
        leaveVisualMode();
    leaveCurrentMode();
    return true;
}

bool FakeVimHandler::Private::handleNoSubMode(const Input &input)
{
    bool handled = true;

    const int oldRevision = revision();
    QString dotCommand = visualDotCommand()
            + QLatin1String(g.gflag ? "g" : "")
            + QString::number(count())
            + input.toString();

    if (input.is('&')) {
        handleExCommand(QLatin1String(g.gflag ? "%s//~/&" : "s"));
    } else if (input.is(':')) {
        enterExMode();
    } else if (input.is('!') && isNoVisualMode()) {
        g.submode = FilterSubMode;
    } else if (input.is('!') && isVisualMode()) {
        enterExMode(QString("!"));
    } else if (input.is('"')) {
        g.submode = RegisterSubMode;
    } else if (s.commaPassesShortcuts() && input.is(',')) {
        passShortcuts(true);
    } else if (input.is('.')) {
        //qDebug() << "REPEATING" << quoteUnprintable(g.dotCommand) << count()
        //    << input;
        dotCommand.clear();
        QString savedCommand = g.dotCommand;
        g.dotCommand.clear();
        beginLargeEditBlock();
        replay(savedCommand);
        endEditBlock();
        leaveCurrentMode();
        g.dotCommand = savedCommand;
    } else if (input.is('<') || input.is('>') || input.is('=')) {
        g.submode = indentModeFromInput(input);
        if (isVisualMode()) {
            leaveVisualMode();
            const int repeat = count();
            if (g.submode == ShiftLeftSubMode)
                shiftRegionLeft(repeat);
            else if (g.submode == ShiftRightSubMode)
                shiftRegionRight(repeat);
            else
                indentSelectedText();
            g.submode = NoSubMode;
        } else {
            setAnchor();
        }
    } else if ((!isVisualMode() && input.is('a')) || (isVisualMode() && input.is('A'))) {
        if (isVisualMode()) {
            if (!isVisualBlockMode())
                dotCommand = QString::number(count()) + "a";
            enterVisualInsertMode('A');
        } else {
            moveRight(qMin(rightDist(), 1));
            breakEditBlock();
            enterInsertMode();
        }
    } else if (input.is('A')) {
        breakEditBlock();
        moveBehindEndOfLine();
        setAnchor();
        enterInsertMode();
        setTargetColumn();
    } else if (input.isControl('a')) {
        changeNumberTextObject(count());
    } else if (g.gflag && input.is('c') && s.emulateVimCommentary()) {
        if (isVisualMode()) {
            pushUndoState();

            QTextCursor start(m_cursor);
            QTextCursor end(start);
            end.setPosition(end.anchor());

            const int count = qAbs(start.blockNumber() - end.blockNumber());
            if (count == 0) {
                dotCommand = "gcc";
            } else {
                dotCommand = QString("gc%1j").arg(count);
            }

            leaveVisualMode();
            toggleComment(currentRange());

            g.submode = NoSubMode;
        } else {
            g.movetype = MoveLineWise;
            g.submode = CommentSubMode;
            pushUndoState();
            setAnchor();
        }
    } else if (g.gflag && input.is('r') && s.emulateReplaceWithRegister()) {
        g.submode = ReplaceWithRegisterSubMode;
        if (isVisualMode()) {
            dotCommand = visualDotCommand() + QString::number(count()) + "gr";
            pasteText(true);
        } else {
            setAnchor();
        }
    } else if (g.gflag && (input.is('q') || input.is('w'))) {
        const bool keepCursor = input.is('w');
        const QString op = keepCursor ? "gw" : "gq";
        m_reflowSavedLine = m_cursor.blockNumber();
        m_reflowSavedColumn = m_cursor.positionInBlock();
        if (isVisualMode()) {
            pushUndoState();
            const int lines = qAbs(m_cursor.blockNumber() - blockAt(m_cursor.anchor()).blockNumber());
            dotCommand = lines == 0 ? op + op.right(1) : op + QString("%1j").arg(lines);
            leaveVisualMode();
            g.movetype = MoveLineWise;
            beginEditBlock();
            reflowText(currentRange());
            endEditBlock();
            if (keepCursor) {
                const int line = qMin(m_reflowSavedLine, document()->blockCount() - 1);
                const QTextBlock b = document()->findBlockByNumber(line);
                setPosition(b.position() + qMin(m_reflowSavedColumn, qMax(b.length() - 1, 0)));
                setTargetColumn();
            } else {
                handleStartOfLine();
            }
            g.submode = NoSubMode;
        } else {
            g.movetype = MoveLineWise;
            g.submode = keepCursor ? ReflowKeepCursorSubMode : ReflowSubMode;
            pushUndoState();
            setAnchor();
        }
    } else if (g.gflag && input.is('d')) {
        // gd: go to definition of the symbol under the cursor.
        q->tagJumpRequested();
    } else if ((input.is('c') || input.is('d') || input.is('y')) && isNoVisualMode()) {
        setAnchor();
        g.opcount = g.mvcount;
        g.mvcount = 0;
        g.rangemode = RangeCharMode;
        g.movetype = MoveExclusive;
        g.submode = changeDeleteYankModeFromInput(input);
    } else if ((input.is('c') || input.is('C') || input.is('s') || input.is('R'))
          && (isVisualCharMode() || isVisualLineMode())) {
        leaveVisualMode();
        g.submode = ChangeSubMode;
        finishMovement();
    } else if ((input.is('c') || input.is('s')) && isVisualBlockMode()) {
        resetCount();
        enterVisualInsertMode(input.asChar());
    } else if (input.is('C')) {
        handleAs("%1c$");
    } else if (input.isControl('c')) {
        if (isNoVisualMode()) {
#if defined(Q_OS_MACOS)
            showMessage(MessageInfo,
                        Tr::tr("Type Control-Shift-Y, Control-Shift-Y to quit FakeVim mode."));
#else
            showMessage(MessageInfo, Tr::tr("Type Alt-Y, Alt-Y to quit FakeVim mode."));
#endif
        } else {
            leaveVisualMode();
        }
    } else if ((input.is('d') || input.is('x') || input.isKey(Key_Delete))
            && isVisualMode()) {
        cutSelectedText();
    } else if (input.is('D') && isNoVisualMode()) {
        handleAs("%1d$");
    } else if ((input.is('D') || input.is('X')) && isVisualMode()) {
        if (isVisualCharMode())
            toggleVisualMode(VisualLineMode);
        if (isVisualBlockMode() && input.is('D'))
            m_visualTargetColumn = -1;
        cutSelectedText();
    } else if (input.isControl('d')) {
        const int scrollOffset = windowScrollOffset();
        int sline = cursorLine() < scrollOffset ? scrollOffset : cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveDown(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLine(cursorLine() - sline);
    } else if (input.isControl('g')) {
        // CTRL-G: show file name, position and status, like Vim.
        const int lines = linesInDocument();
        const int line = cursorLine() + 1;
        const int physCol = physicalCursorColumn() + 1;
        const int logCol = logicalCursorColumn() + 1;
        const QString col = physCol == logCol
            ? Tr::tr("col %1").arg(physCol)
            : Tr::tr("col %1-%2").arg(physCol).arg(logCol);
        QString msg = m_currentFileName.isEmpty()
            ? QString("[No Name]") : '"' + m_currentFileName + '"';
        if (document()->isModified())
            msg += Tr::tr(" [Modified]");
        msg += Tr::tr(" line %1 of %2 --%3%-- %4")
            .arg(line).arg(lines).arg(line * 100 / lines).arg(col);
        showMessage(MessageInfo, msg);
    } else if (!g.gflag && input.is('g')) {
        g.gflag = true;
    } else if (!isVisualMode() && (input.is('i') || input.isKey(Key_Insert))) {
        breakEditBlock();
        enterInsertMode();
        if (atEndOfLine())
            moveLeft();
    } else if (input.is('I')) {
        if (isVisualMode()) {
            if (!isVisualBlockMode())
                dotCommand = QString::number(count()) + "i";
            enterVisualInsertMode('I');
        } else {
            if (g.gflag)
                moveToStartOfLine();
            else
                moveToFirstNonBlankOnLine();
            breakEditBlock();
            enterInsertMode();
        }
    } else if (input.isControl('i')) {
        jump(count());
    } else if (input.is('J')) {
        pushUndoState();
        moveBehindEndOfLine();
        beginEditBlock();
        if (g.submode == NoSubMode)
            joinLines(count(), g.gflag);
        endEditBlock();
    } else if (input.isControl('l')) {
        // screen redraw. should not be needed
    } else if (!g.gflag && input.is('m')) {
        g.subsubmode = MarkSubSubMode;
    } else if (isVisualMode() && (input.is('o') || input.is('O'))) {
        int pos = position();
        setAnchorAndPosition(pos, anchor());
        std::swap(m_positionPastEnd, m_anchorPastEnd);
        setTargetColumn();
        if (m_positionPastEnd)
            m_visualTargetColumn = -1;
    } else if (input.is('o') || input.is('O')) {
        bool insertAfter = input.is('o');
        pushUndoState();

        // Prepend line only if on the first line and command is 'O'.
        bool appendLine = true;
        if (!insertAfter) {
            if (block().blockNumber() == 0)
                appendLine = false;
            else
                moveUp();
        }
        // firstPositionInLine()/lastPositionInLine() below expect a visual
        // line number.
        const int line = lineNumber(block());
        // Detecting an accidentally opened fold further down must use the
        // (stable) block number, not the visual line number: the latter shifts
        // when word wrapping adds display lines, which spuriously folded the
        // enclosing block (QTCREATORBUG-24005).
        const int startBlockNumber = block().blockNumber();

        beginEditBlock();
        enterInsertMode();
        setPosition(appendLine ? lastPositionInLine(line) : firstPositionInLine(line));
        clearLastInsertion();
        setAnchor();
        insertNewLine();
        if (appendLine) {
            m_buffer->insertState.newLineBefore = true;
        } else {
            moveUp();
            m_buffer->insertState.pos1 = position();
            m_buffer->insertState.newLineAfter = true;
        }
        setTargetColumn();
        endEditBlock();

        // Close accidentally opened block.
        if (block().blockNumber() > 0) {
            moveUp();
            if (startBlockNumber != block().blockNumber())
                q->fold(1, true);
            moveDown();
        }
    } else if (input.isControl('o')) {
        jump(-count());
    } else if (input.is('p') || input.is('P') || input.isShift(Qt::Key_Insert)) {
        if (isVisualMode()) {
            // Vim's redo for a paste over a visual selection records only the
            // deletion of the selection, not the put text, so "." deletes the
            // re-selected region without inserting anything (QTCREATORBUG-18298).
            dotCommand = visualDotCommand() + "d";
        } else {
            dotCommand = QString("\"%1%2%3").arg(QChar(m_register)).arg(count()).arg(input.asChar());
        }

        pasteText(!input.is('P'));
        setTargetColumn();
        finishMovement();
    } else if (input.is('q')) {
        if (g.isRecording) {
            // Stop recording.
            stopRecording();
        } else {
            // Recording shouldn't work in mapping or while executing register.
            handled = g.mapStates.empty();
            if (handled)
                g.submode = MacroRecordSubMode;
        }
    } else if (input.is('r')) {
        g.submode = ReplaceSubMode;
    } else if (!isVisualMode() && input.is('R')) {
        pushUndoState();
        breakEditBlock();
        enterReplaceMode();
    } else if (input.isControl('r')) {
        dotCommand.clear();
        int repeat = count();
        while (--repeat >= 0)
            redo();
    } else if (input.is('S') && isVisualMode() && s.emulateSurround()) {
        g.submode = AddSurroundingSubMode;
        g.subsubmode = SurroundSubSubMode;
    } else if (input.is('s')) {
        handleAs("c%1l");
    } else if (input.is('S')) {
        handleAs("%1cc");
    } else if (g.gflag && input.is('t')) {
        handleExCommand("tabnext");
    } else if (g.gflag && input.is('T')) {
        handleExCommand("tabprevious");
    } else if (input.isControl('t')) {
        q->tagStackRequested(-count());
    } else if (!g.gflag && input.is('u') && !isVisualMode()) {
        dotCommand.clear();
        int repeat = count();
        while (--repeat >= 0)
            undo();
    } else if (input.isControl('u')) {
        int sline = cursorLineOnScreen();
        // FIXME: this should use the "scroll" option, and "count"
        moveUp(linesOnScreen() / 2);
        handleStartOfLine();
        scrollToLine(cursorLine() - sline);
    } else if (g.gflag && input.is('v')) {
        if (isNoVisualMode()) {
            CursorPosition from = markLessPosition();
            CursorPosition to = markGreaterPosition();
            if (m_buffer->lastVisualModeInverted)
                std::swap(from, to);
            toggleVisualMode(m_buffer->lastVisualMode);
            setCursorPosition(from);
            setAnchor();
            setCursorPosition(to);
            setTargetColumn();
        }
    } else if (input.is('v')) {
        toggleVisualMode(VisualCharMode);
    } else if (input.is('V')) {
        toggleVisualMode(VisualLineMode);
    } else if (input.isControl('v')) {
        toggleVisualMode(VisualBlockMode);
    } else if (input.isControl('w')) {
        g.submode = WindowSubMode;
    } else if (input.is('x') && isNoVisualMode()) {
        handleAs("%1dl");
    } else if (input.isControl('x')) {
        changeNumberTextObject(-count());
    } else if (input.is('X')) {
        handleAs("%1dh");
    } else if (input.is('Y') && isNoVisualMode())  {
        handleAs("%1yy");
    } else if (input.isControl('y')) {
        // Scroll the view up, dragging the cursor along so it stays at least
        // 'scrolloff' lines from the bottom (QTCREATORBUG-34074).
        scrollUp(count());
        if (cursorLine() > lineOnBottom())
            moveUpVisually(cursorLine() - lineOnBottom());
    } else if (input.is('y') && isVisualCharMode()) {
        g.rangemode = RangeCharMode;
        g.movetype = MoveInclusive;
        g.submode = YankSubMode;
        finishMovement();
    } else if ((input.is('y') && isVisualLineMode())
            || (input.is('Y') && isVisualLineMode())
            || (input.is('Y') && isVisualCharMode())) {
        g.rangemode = RangeLineMode;
        g.movetype = MoveLineWise;
        g.submode = YankSubMode;
        finishMovement();
    } else if ((input.is('y') || input.is('Y')) && isVisualBlockMode()) {
        g.rangemode = RangeBlockMode;
        g.movetype = MoveInclusive;
        g.submode = YankSubMode;
        finishMovement();
    } else if (input.is('z')) {
        g.submode = ZSubMode;
    } else if (input.is('Z')) {
        g.submode = CapitalZSubMode;
    } else if ((input.is('~') || input.is('u') || input.is('U'))) {
        g.movetype = MoveExclusive;
        g.submode = letterCaseModeFromInput(input);
        pushUndoState();
        if (isVisualMode()) {
            leaveVisualMode();
            finishMovement();
        } else if (g.gflag || (g.submode == InvertCaseSubMode && s.tildeOp())) {
            if (atEndOfLine())
                moveLeft();
            setAnchor();
        } else {
            const QString movementCommand = QString("%1l%1l").arg(count());
            handleAs("g" + input.toString() + movementCommand);
        }
    } else if (g.gflag && input.is('@')) {
        // g@{motion}: hand the moved-over region to the 'operatorfunc'.
        if (isVisualMode()) {
            const bool blockwise = isVisualBlockMode();
            g.rangemode = blockwise ? RangeBlockMode
                        : isVisualLineMode() ? RangeLineMode : RangeCharMode;
            g.movetype = isVisualLineMode() ? MoveLineWise : MoveInclusive;
            dotCommand = visualDotCommand() + "g@";
            leaveVisualMode();
            // Normalize exactly like the operator-pending path, so the region
            // ends one past its last character in both.
            fixSelection();
            pushUndoState(false);
            beginEditBlock();
            callOperatorFunc(currentRange());
            endEditBlock();
            g.submode = NoSubMode;
        } else {
            g.opcount = g.mvcount;
            g.mvcount = 0;
            g.rangemode = RangeCharMode;
            g.movetype = MoveExclusive;
            g.submode = OperatorFuncSubMode;
            setAnchor();
        }
    } else if (input.is('@')) {
        g.submode = MacroExecuteSubMode;
    } else if (input.isKey(Key_Delete)) {
        setAnchor();
        moveRight(qMin(1, rightDist()));
        removeText(currentRange());
        if (atEndOfLine())
            moveLeft();
    } else if (input.isControl(Key_BracketRight)) {
        q->tagJumpRequested();
    } else if (input.is('K')) {
        q->contextHelpRequested();
    } else if (input.key() == Key_AsciiCircum
               && input.modifiers().testFlag(Utils::HostOsInfo::controlModifier())) {
        // CTRL-^: edit the alternate file. Key_AsciiCircum is the caret key
        // whatever the layout, so Shift needs no special handling.
        q->alternateFileRequested();
    } else if (handleMovement(input)) {
        // movement handled
        dotCommand.clear();
    } else {
        handled = false;
    }

    // Set dot command if the current input changed document or entered insert mode.
    if (handled && !dotCommand.isEmpty() && (oldRevision != revision() || isInsertMode()))
        setDotCommand(dotCommand);

    return handled;
}

bool FakeVimHandler::Private::handleChangeDeleteYankSubModes(const Input &input)
{
    if (g.submode != changeDeleteYankModeFromInput(input))
        return false;

    handleChangeDeleteYankSubModes();

    return true;
}

void FakeVimHandler::Private::handleChangeDeleteYankSubModes()
{
    g.movetype = MoveLineWise;

    const QString dotCommand = dotCommandFromSubMode(g.submode);

    if (!dotCommand.isEmpty())
        pushUndoState();

    const int anc = firstPositionInLine(cursorLine() + 1);
    moveDown(count() - 1);
    const int pos = lastPositionInLine(cursorLine() + 1);
    setAnchorAndPosition(anc, pos);

    if (!dotCommand.isEmpty())
        setDotCommand(QString("%2%1%1").arg(dotCommand), count());

    finishMovement();

    g.submode = NoSubMode;
}

bool FakeVimHandler::Private::handleReplaceSubMode(const Input &input)
{
    bool handled = true;

    const QChar c = input.asChar();
    setDotCommand(visualDotCommand() + 'r' + c);
    if (isVisualMode()) {
        pushUndoState();
        leaveVisualMode();
        Range range = currentRange();
        if (g.rangemode == RangeCharMode)
            ++range.endPos;
        // Replace each character but preserve lines.
        transformText(range, [&c](const QString &text) {
            static const QRegularExpression regexp("[^\\n]");
            return QString(text).replace(regexp, c);
        });
    } else if (count() <= rightDist()) {
        pushUndoState();
        setAnchor();
        moveRight(count());
        Range range = currentRange();
        if (input.isReturn()) {
            beginEditBlock();
            replaceText(range, QString());
            insertNewLine();
            endEditBlock();
        } else {
            replaceText(range, QString(count(), c));
            moveRight(count() - 1);
        }
        setTargetColumn();
        setDotCommand("%1r" + input.text(), count());
    } else {
        handled = false;
    }
    g.submode = NoSubMode;
    finishMovement();

    return handled;
}

bool FakeVimHandler::Private::handleCommentSubMode(const Input &input)
{
    if (!input.is('c'))
        return false;

    g.movetype = MoveLineWise;

    const int anc = firstPositionInLine(cursorLine() + 1);
    moveDown(count() - 1);
    const int pos = lastPositionInLine(cursorLine() + 1);
    setAnchorAndPosition(anc, pos);

    setDotCommand(QString("%1gcc").arg(count()));

    finishMovement();

    g.submode = NoSubMode;

    return true;
}

bool FakeVimHandler::Private::handleReplaceWithRegisterSubMode(const Input &input)
{
    if (!input.is('r'))
        return false;

    pushUndoState(false);
    beginEditBlock();

    const QString movement = (count() == 1)
                             ? QString() : (QString::number(count() - 1) + "j");

    g.dotCommand = "V" + movement + "gr";
    replay(g.dotCommand);

    endEditBlock();

    return true;
}

bool FakeVimHandler::Private::handleExchangeSubMode(const Input &input)
{
    if (input.is('c')) { // cxc
        g.exchangeRange.reset();
        g.submode = NoSubMode;
        return true;
    }

    if (input.is('x')) { // cxx
        setAnchorAndPosition(firstPositionInLine(cursorLine() + 1),
                             lastPositionInLine(cursorLine() + 1) + 1);

        setDotCommand("cxx");

        finishMovement();

        g.submode = NoSubMode;

        return true;
    }

    return false;
}

bool FakeVimHandler::Private::handleDeleteChangeSurroundingSubMode(const Input &input)
{
    if (g.submode != ChangeSurroundingSubMode && g.submode != DeleteSurroundingSubMode)
        return false;

    bool handled = false;

    if (input.is('(') || input.is(')') || input.is('b')) {
        handled = selectBlockTextObject(false, '(', ')');
    } else if (input.is('{') || input.is('}') || input.is('B')) {
        handled = selectBlockTextObject(false, '{', '}');
    } else if (input.is('[') || input.is(']')) {
        handled = selectBlockTextObject(false, '[', ']');
    } else if (input.is('<') || input.is('>') || input.is('t')) {
        handled = selectBlockTextObject(false, '<', '>');
    } else if (input.is('"') || input.is('\'') || input.is('`')) {
        handled = selectQuotedStringTextObject(false, input.asChar());
    }

    if (handled) {
        if (g.submode == DeleteSurroundingSubMode) {
            pushUndoState(false);
            beginEditBlock();

            // Surround is always one character, so just delete the first and last one
            transformText(currentRange(), [](const QString &text) {
                return text.mid(1, text.size() - 2);
            });

            endEditBlock();
            clearCurrentMode();

            g.dotCommand = "ds" + input.asChar();
        } else if (g.submode == ChangeSurroundingSubMode) {
            g.subsubmode = SurroundSubSubMode;
        }
    }

    return handled;
}

bool FakeVimHandler::Private::handleAddSurroundingSubMode(const Input &input)
{
    if (!input.is('s'))
        return false;

    g.subsubmode = SurroundSubSubMode;

    int anc = firstPositionInLine(cursorLine() + 1);
    const int pos = lastPositionInLine(cursorLine() + 1);

    // Ignore leading spaces
    while ((characterAt(anc) == ' ' || characterAt(anc) == '\t') && anc != pos) {
        anc++;
    }

    setAnchorAndPosition(anc, pos);

    finishMovement("s");

    return true;
}

bool FakeVimHandler::Private::handleFilterSubMode(const Input &)
{
    return false;
}

bool FakeVimHandler::Private::handleRegisterSubMode(const Input &input)
{
    bool handled = false;

    QChar reg = input.asChar();
    if (QString("*+.%#:-\"_").contains(reg) || reg.isLetterOrNumber()) {
        m_register = reg.unicode();
        handled = true;
    }
    g.submode = NoSubMode;

    return handled;
}

bool FakeVimHandler::Private::handleShiftSubMode(const Input &input)
{
    if (g.submode != indentModeFromInput(input))
        return false;

    g.movetype = MoveLineWise;
    pushUndoState();
    moveDown(count() - 1);
    setDotCommand(QString("%2%1%1").arg(input.asChar()), count());
    finishMovement();
    g.submode = NoSubMode;

    return true;
}

bool FakeVimHandler::Private::handleChangeCaseSubMode(const Input &input)
{
    if (g.submode != letterCaseModeFromInput(input))
        return false;

    if (!isFirstNonBlankOnLine(position())) {
        moveToStartOfLine();
        moveToFirstNonBlankOnLine();
    }
    setTargetColumn();
    pushUndoState();
    setAnchor();
    setPosition(lastPositionInLine(cursorLine() + count()) + 1);
    finishMovement(QString("%1%2").arg(count()).arg(input.raw()));
    g.submode = NoSubMode;

    return true;
}

bool FakeVimHandler::Private::handleReflowSubMode(const Input &input)
{
    // "gqq"/"gqgq" and "gww"/"gwgw" reflow [count] lines from the current one.
    const QChar op = g.submode == ReflowKeepCursorSubMode ? QLatin1Char('w') : QLatin1Char('q');
    if (!input.is(op.toLatin1()))
        return false;

    g.movetype = MoveLineWise;
    const int anc = firstPositionInLine(cursorLine() + 1);
    moveDown(count() - 1);
    const int pos = lastPositionInLine(cursorLine() + 1);
    setAnchorAndPosition(anc, pos);
    setDotCommand(QString("%1g%2%2").arg(count()).arg(op));
    finishMovement();
    g.submode = NoSubMode;

    return true;
}

bool FakeVimHandler::Private::handleWindowSubMode(const Input &input)
{
    if (handleCount(input))
        return true;

    leaveVisualMode();
    leaveCurrentMode();
    q->windowCommandRequested(input.toString(), count());

    return true;
}

bool FakeVimHandler::Private::handleZSubMode(const Input &input)
{
    bool handled = true;
    bool foldMaybeClosed = false;
    if (input.isReturn() || input.is('t')
        || input.is('-') || input.is('b')
        || input.is('.') || input.is('z')) {
        // Cursor line to top/center/bottom of window.
        Qt::AlignmentFlag align;
        if (input.isReturn() || input.is('t'))
            align = Qt::AlignTop;
        else if (input.is('.') || input.is('z'))
            align = Qt::AlignVCenter;
        else
            align = Qt::AlignBottom;
        const bool moveToNonBlank = (input.is('.') || input.isReturn() || input.is('-'));
        const int line = g.mvcount == 0 ? -1 : firstPositionInLine(count());
        alignViewportToCursor(align, line, moveToNonBlank);
    } else if (input.is('o') || input.is('c')) {
        // Open/close current fold.
        foldMaybeClosed = input.is('c');
        q->fold(count(), foldMaybeClosed);
    } else if (input.is('O') || input.is('C')) {
        // Recursively open/close current fold.
        foldMaybeClosed = input.is('C');
        q->fold(-1, foldMaybeClosed);
    } else if (input.is('a') || input.is('A')) {
        // Toggle current fold.
        foldMaybeClosed = true;
        q->foldToggle(input.is('a') ? count() : -1);
    } else if (input.is('R') || input.is('M')) {
        // Open/close all folds in document.
        foldMaybeClosed = input.is('M');
        q->foldAll(foldMaybeClosed);
    } else if (input.is('i')) {
        // Toggle folding for the whole document (open all if any fold is
        // closed, otherwise close all) - like Vim's zi (QTCREATORBUG-11753).
        foldMaybeClosed = true;
        q->foldToggleAll();
    } else if (input.is('j') || input.is('k')) {
        q->foldGoTo(input.is('j') ? count() : -count(), false);
    } else {
        handled = false;
    }
    if (foldMaybeClosed)
        ensureCursorVisible();
    g.submode = NoSubMode;
    return handled;
}

bool FakeVimHandler::Private::handleCapitalZSubMode(const Input &input)
{
    // Recognize ZZ and ZQ as aliases for ":x" and ":q!".
    bool handled = true;
    if (input.is('Z'))
        handleExCommand("x");
    else if (input.is('Q'))
        handleExCommand("q!");
    else
        handled = false;
    g.submode = NoSubMode;
    return handled;
}

bool FakeVimHandler::Private::handleMacroRecordSubMode(const Input &input)
{
    g.submode = NoSubMode;
    return startRecording(input);
}

bool FakeVimHandler::Private::handleMacroExecuteSubMode(const Input &input)
{
    g.submode = NoSubMode;

    bool result = true;
    int repeat = count();
    while (result && --repeat >= 0)
        result = executeRegister(input.asChar().unicode());

    return result;
}

EventResult FakeVimHandler::Private::handleInsertOrReplaceMode(const Input &input)
{
    if (position() < m_buffer->insertState.pos1 || position() > m_buffer->insertState.pos2) {
        commitInsertState();
        invalidateInsertState();
    }

    if (g.mode == InsertMode)
        handleInsertMode(input);
    else
        handleReplaceMode(input);

    if (!hasValidEditor())
        return EventHandled;

    if (!isInsertMode() || m_buffer->breakEditBlock
            || position() < m_buffer->insertState.pos1 || position() > m_buffer->insertState.pos2) {
        commitInsertState();
        invalidateInsertState();
        breakEditBlock();
        m_visualBlockInsert = NoneBlockInsertMode;
    }

    // We don't want fancy stuff in insert mode.
    return EventHandled;
}

void FakeVimHandler::Private::handleReplaceMode(const Input &input)
{
    if (input.isEscape()) {
        commitInsertState();
        moveLeft(qMin(1, leftDist()));
        enterCommandMode();
        g.dotCommand.append(m_buffer->lastInsertion + "<ESC>");
        m_replacedChars.clear();
    } else if (input.isKey(Key_Left)) {
        moveLeft();
        m_replacedChars.clear();
    } else if (input.isKey(Key_Right)) {
        moveRight();
        m_replacedChars.clear();
    } else if (input.isKey(Key_Up)) {
        moveUp();
        m_replacedChars.clear();
    } else if (input.isKey(Key_Down)) {
        moveDown();
        m_replacedChars.clear();
    } else if (input.isKey(Key_Insert)) {
        g.mode = InsertMode;
        q->modeChanged(isInsertMode());
    } else if (input.isControl('o')) {
        enterCommandMode(ReplaceMode);
    } else if (input.isBackspace()) {
        // Undo the last overwrite: move left, remove the typed character and,
        // unless it was appended past the end of the line, restore the
        // character that was there before.
        joinPreviousEditBlock();
        if (!m_replacedChars.isEmpty()) {
            const QChar original = m_replacedChars.back();
            m_replacedChars.chop(1);
            moveLeft();
            setAnchor();
            moveRight();
            removeText(currentRange());
            if (original != QLatin1Char('\n')) {
                setAnchor();
                insertText(QString(original));
                moveLeft();
            }
        } else {
            moveLeft(qMin(1, leftDist()));
        }
        setTargetColumn();
        endEditBlock();
    } else if (input.isKey(Key_Delete)) {
        // As in insert mode, <Del> removes the character under the cursor.
        joinPreviousEditBlock();
        if (!atEndOfLine()) {
            setAnchor();
            moveRight();
            removeText(currentRange());
        }
        setTargetColumn();
        endEditBlock();
    } else {
        joinPreviousEditBlock();
        if (atEndOfLine()) {
            m_replacedChars.append(QLatin1Char('\n'));
        } else {
            m_replacedChars.append(characterAtCursor());
            setAnchor();
            moveRight();
            removeText(currentRange());
        }
        const QString text = input.text();
        setAnchor();
        insertText(text);
        setTargetColumn();
        endEditBlock();
    }
}

void FakeVimHandler::Private::finishInsertMode()
{
    bool newLineAfter = m_buffer->insertState.newLineAfter;
    bool newLineBefore = m_buffer->insertState.newLineBefore;

    // Repeat insertion [count] times.
    // One instance was already physically inserted while typing.
    if (!m_buffer->breakEditBlock && isInsertStateValid()) {
        commitInsertState();

        QString text = m_buffer->lastInsertion;
        const QString dotCommand = g.dotCommand;
        const int repeat = count() - 1;
        m_buffer->lastInsertion.clear();
        joinPreviousEditBlock();

        if (newLineAfter) {
            text.chop(1);
            text.prepend("<END>\n");
        } else if (newLineBefore) {
            text.prepend("<END>");
        }

        replay(text, repeat);

        if (m_visualBlockInsert != NoneBlockInsertMode && !text.contains('\n')) {
            const CursorPosition lastAnchor = markLessPosition();
            const CursorPosition lastPosition = markGreaterPosition();
            const bool change = m_visualBlockInsert == ChangeBlockInsertMode;
            const int insertColumn = (m_visualBlockInsert == InsertBlockInsertMode || change)
                    ? qMin(lastPosition.column, lastAnchor.column)
                    : qMax(lastPosition.column, lastAnchor.column) + 1;

            CursorPosition pos(lastAnchor.line, insertColumn);

            if (change)
                pos.column = columnAt(m_buffer->insertState.pos1);

            // Cursor position after block insert is on the first selected line,
            // last selected column for 's' command, otherwise first selected column.
            const int endColumn = change ? qMax(0, m_cursor.positionInBlock() - 1)
                                         : qMin(lastPosition.column, lastAnchor.column);

            while (pos.line < lastPosition.line) {
                ++pos.line;
                setCursorPosition(&m_cursor, pos);
                if (m_visualBlockInsert == AppendToEndOfLineBlockInsertMode) {
                    moveToEndOfLine();
                } else if (m_visualBlockInsert == AppendBlockInsertMode) {
                    // Prepend spaces if necessary.
                    int spaces = pos.column - m_cursor.positionInBlock();
                    if (spaces > 0) {
                        setAnchor();
                        m_cursor.insertText(QString(" ").repeated(spaces));
                    }
                } else if (m_cursor.positionInBlock() != pos.column) {
                    continue;
                }
                replay(text, repeat + 1);
            }

            setCursorPosition(CursorPosition(lastAnchor.line, endColumn));
        } else {
            moveLeft(qMin(1, leftDist()));
        }

        endEditBlock();
        breakEditBlock();

        m_buffer->lastInsertion = text;
        g.dotCommand = dotCommand;
    } else {
        moveLeft(qMin(1, leftDist()));
    }

    if (newLineBefore || newLineAfter)
        m_buffer->lastInsertion.remove(0, m_buffer->lastInsertion.indexOf('\n') + 1);
    g.dotCommand.append(m_buffer->lastInsertion + "<ESC>");

    setTargetColumn();
    enterCommandMode();
    triggerAutocmd("InsertLeave");
}

void FakeVimHandler::Private::handleInsertMode(const Input &input)
{
    if (g.subsubmode == ExpressionSubSubMode) {
        // CTRL-R = : collect an expression, then insert its value on Return.
        if (input.isEscape()) {
            g.commandBuffer.clear();
            g.subsubmode = NoSubSubMode;
            updateMiniBuffer();
        } else if (input.isReturn()) {
            const QString expr = g.commandBuffer.contents();
            g.commandBuffer.clear();
            g.subsubmode = NoSubSubMode;
            VimValue value;
            QString error;
            if (evaluateExpression(expr, &value, &error))
                m_cursor.insertText(value.toString());
            else
                showMessage(MessageError, error);
            updateMiniBuffer();
        } else if (input.isBackspace()) {
            if (g.commandBuffer.isEmpty())
                g.subsubmode = NoSubSubMode;
            else
                g.commandBuffer.deleteChar();
            updateMiniBuffer();
        } else if (g.commandBuffer.handleInput(input)) {
            updateMiniBuffer();
        }
        return;
    }

    if (input.isEscape()) {
        if (g.submode == CtrlRSubMode || g.submode == CtrlVSubMode) {
            g.submode = NoSubMode;
            g.subsubmode = NoSubSubMode;
            updateMiniBuffer();
        } else {
            clearUntouchedAutoIndentation();
            finishInsertMode();
        }
    } else if (g.submode == CtrlRSubMode) {
        if (input.is('=')) {
            // Enter the "=" expression register prompt.
            g.submode = NoSubMode;
            g.subsubmode = ExpressionSubSubMode;
            g.commandBuffer.clear();
            g.commandBuffer.setPrompt('=');
            updateMiniBuffer();
        } else {
            m_cursor.insertText(registerContents(input.asChar().unicode()));
            g.submode = NoSubMode;
        }
    } else if (g.submode == CtrlVSubMode) {
        if (g.subsubmode == NoSubSubMode) {
            g.subsubmode = CtrlVUnicodeSubSubMode;
            m_ctrlVAccumulator = 0;
            if (input.is('x') || input.is('X')) {
                // ^VXnn or ^Vxnn with 00 <= nn <= FF
                // BMP Unicode codepoints ^Vunnnn with 0000 <= nnnn <= FFFF
                // any Unicode codepoint ^VUnnnnnnnn with 00000000 <= nnnnnnnn <= 7FFFFFFF
                // ^Vnnn with 000 <= nnn <= 255
                // ^VOnnn or ^Vonnn with 000 <= nnn <= 377
                m_ctrlVLength = 2;
                m_ctrlVBase = 16;
            } else if (input.is('O') || input.is('o')) {
                m_ctrlVLength = 3;
                m_ctrlVBase = 8;
            } else if (input.is('u')) {
                m_ctrlVLength = 4;
                m_ctrlVBase = 16;
            } else if (input.is('U')) {
                m_ctrlVLength = 8;
                m_ctrlVBase = 16;
            } else if (input.isDigit()) {
                bool ok;
                m_ctrlVAccumulator = input.toInt(&ok, 10);
                m_ctrlVLength = 2;
                m_ctrlVBase = 10;
            } else {
                insertInInsertMode(input.raw());
                g.submode = NoSubMode;
                g.subsubmode = NoSubSubMode;
            }
        } else {
            bool ok;
            int current = input.toInt(&ok, m_ctrlVBase);
            if (ok)
                m_ctrlVAccumulator = m_ctrlVAccumulator * m_ctrlVBase + current;
            --m_ctrlVLength;
            if (m_ctrlVLength == 0 || !ok) {
                QString s;
                if (QChar::requiresSurrogates(m_ctrlVAccumulator)) {
                    s.append(QChar(QChar::highSurrogate(m_ctrlVAccumulator)));
                    s.append(QChar(QChar::lowSurrogate(m_ctrlVAccumulator)));
                } else {
                    s.append(QChar(m_ctrlVAccumulator));
                }
                insertInInsertMode(s);
                g.submode = NoSubMode;
                g.subsubmode = NoSubSubMode;

                // Try again without Ctrl-V interpretation.
                if (!ok)
                    handleInsertMode(input);
            }
        }
    } else if (input.isControl('o')) {
        enterCommandMode(InsertMode);
    } else if (input.isControl('v')) {
        g.submode = CtrlVSubMode;
        g.subsubmode = NoSubSubMode;
        updateMiniBuffer();
    } else if (input.isControl('r')) {
        g.submode = CtrlRSubMode;
        g.subsubmode = NoSubSubMode;
        updateMiniBuffer();
    } else if (input.isControl('w')) {
        const int blockNumber = m_cursor.blockNumber();
        const int endPos = position();
        moveToNextWordStart(1, false, false);
        if (blockNumber != m_cursor.blockNumber())
            moveToEndOfLine();
        const int beginPos = position();
        Range range(beginPos, endPos, RangeCharMode);
        removeText(range);
    } else if (input.isControl('u')) {
        const int blockNumber = m_cursor.blockNumber();
        const int endPos = position();
        moveToStartOfLine();
        if (blockNumber != m_cursor.blockNumber())
            moveToEndOfLine();
        const int beginPos = position();
        Range range(beginPos, endPos, RangeCharMode);
        removeText(range);
    } else if (input.isKey(Key_Insert)) {
        g.mode = ReplaceMode;
        q->modeChanged(isInsertMode());
    } else if (input.isKey(Key_Left)) {
        moveLeft();
    } else if (input.isShift(Key_Left) || input.isControl(Key_Left)) {
        moveToNextWordStart(1, false, false);
    } else if (input.isKey(Key_Down)) {
        g.submode = NoSubMode;
        moveDown();
    } else if (input.isKey(Key_Up)) {
        g.submode = NoSubMode;
        moveUp();
    } else if (input.isKey(Key_Right)) {
        moveRight();
    } else if (input.isShift(Key_Right) || input.isControl(Key_Right)) {
        moveToNextWordStart(1, false, true);
    } else if (input.isKey(Key_Home)) {
        moveToStartOfLine();
    } else if (input.isKey(Key_End)) {
        moveBehindEndOfLine();
        m_targetColumn = -1;
    } else if (input.isReturn() || input.isControl('j') || input.isControl('m')) {
        if (!input.isReturn() || !handleInsertInEditor(input)) {
            joinPreviousEditBlock();
            g.submode = NoSubMode;
            insertNewLine();
            endEditBlock();
        }
    } else if (input.isBackspace()) {
        // pass C-h as backspace, too
        if (!handleInsertInEditor(Input(Qt::Key_Backspace, Qt::NoModifier))) {
            joinPreviousEditBlock();
            if (!m_buffer->lastInsertion.isEmpty()
                    || s.backspace().contains("start")
                    || s.backspace().contains("2")) {
                const int line = cursorLine() + 1;
                const Column col = cursorColumn();
                QString data = lineContents(line);
                const Column ind = indentation(data);
                if (col.logical <= ind.logical && col.logical
                        && startsWithWhitespace(data, col.physical)) {
                    const int ts = tabStop();
                    const int newl = col.logical - 1 - (col.logical - 1) % ts;
                    const QString prefix = tabExpand(newl);
                    setLineContents(line, prefix + data.mid(col.physical));
                    moveToStartOfLine();
                    moveRight(prefix.size());
                } else {
                    setAnchor();
                    m_cursor.deletePreviousChar();
                }
            }
            endEditBlock();
        }
    } else if (input.isKey(Key_Delete)) {
        if (!handleInsertInEditor(input)) {
            joinPreviousEditBlock();
            m_cursor.deleteChar();
            endEditBlock();
        }
    } else if (input.isKey(Key_PageDown) || input.isControl('f')) {
        movePageDown();
    } else if (input.isKey(Key_PageUp) || input.isControl('b')) {
        movePageUp();
    } else if (input.isKey(Key_Tab)) {
        const QString tabOut = s.tabOut();
        const int pos = position();
        if (!tabOut.isEmpty() && pos < lastPositionInDocument()
                && tabOut.contains(document()->characterAt(pos))) {
            // Tab jumps over the next closing character instead of inserting a
            // tab (QTCREATORBUG-27441).
            setPosition(pos + 1);
            setTargetColumn();
        } else if (q->tabPressedInInsertMode()) {
            m_buffer->insertState.insertingSpaces = true;
            if (expandTab()) {
                const int ts = tabStop();
                const int col = logicalCursorColumn();
                QString str = QString(ts - col % ts, ' ');
                insertInInsertMode(str);
            } else {
                insertInInsertMode(input.raw());
            }
            m_buffer->insertState.insertingSpaces = false;
        }
    } else if (input.isControl('t')) {
        // Add one level of indentation to the current line (Vim CTRL-T).
        const int pos = firstPositionInLine(cursorLine() + 1);
        setAnchorAndPosition(pos, pos);
        shiftRegionRight(1);
    } else if (input.isControl('d')) {
        // remove one level of indentation from the current line
        const int shift = shiftWidth();
        const int tab = tabStop();
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
    } else if (input.isControl('p') || input.isControl('n')) {
        QTextCursor tc = m_cursor;
        moveToNextWordStart(1, false, false);
        QString str = selectText(Range(position(), tc.position()));
        m_cursor = tc;
        q->simpleCompletionRequested(str, input.isControl('n'));
    } else if (input.isShift(Qt::Key_Insert)) {
        // Insert text from clipboard.
        QClipboard *clipboard = QApplication::clipboard();
        const QMimeData *data = clipboard->mimeData();
        if (data && data->hasText())
            insertInInsertMode(data->text());
    } else {
        // Treat inserted whitespace (space or tab) as user-typed so it is not
        // stripped as auto-indentation, e.g. when block-inserting an indent
        // (QTCREATORBUG-24094).
        const QString toInsert = input.text();
        m_buffer->insertState.insertingSpaces =
            toInsert == QLatin1String(" ") || toInsert == QLatin1String("\t");
        if (!handleInsertInEditor(input)) {
            if (toInsert.isEmpty())
                return;
            insertInInsertMode(toInsert);
        }
        m_buffer->insertState.insertingSpaces = false;
    }
}

void FakeVimHandler::Private::insertInInsertMode(const QString &text)
{
    // Typing on an auto-indented line keeps its indentation.
    m_autoIndentBlock = -1;
    joinPreviousEditBlock();
    insertText(text);
    if (s.smartIndent() && isElectricCharacter(text.at(0))) {
        const QString leftText = block().text()
               .left(position() - 1 - block().position());
        if (leftText.simplified().isEmpty()) {
            Range range(position(), position(), g.rangemode);
            indentText(range, text.at(0));
        }
    }
    setTargetColumn();
    endEditBlock();
    g.submode = NoSubMode;
}

bool FakeVimHandler::Private::startRecording(const Input &input)
{
    QChar reg = input.asChar();
    if (reg == '"' || reg.isLetterOrNumber()) {
        g.currentRegister = reg.unicode();
        g.isRecording = true;
        g.recorded.clear();
        return true;
    }

    return false;
}

void FakeVimHandler::Private::record(const Input &input)
{
    if (g.isRecording)
        g.recorded.append(input.toString());
}

void FakeVimHandler::Private::stopRecording()
{
    // Remove q from end (stop recording command).
    g.isRecording = false;
    g.recorded.chop(1);
    setRegister(g.currentRegister, g.recorded, g.rangemode);
    g.currentRegister = 0;
    g.recorded.clear();
}

void FakeVimHandler::Private::handleAs(const QString &command)
{
    QString cmd = QString("\"%1").arg(QChar(m_register));

    if (command.contains("%1"))
        cmd.append(command.arg(count()));
    else
        cmd.append(command);

    leaveVisualMode();
    beginLargeEditBlock();
    replay(cmd);
    endEditBlock();
}

bool FakeVimHandler::Private::hasValidEditor()
{
    return m_textedit || m_plaintextedit || m_qcPlainTextEdit;
}

bool FakeVimHandler::Private::executeRegister(int reg)
{
    QChar regChar(reg);

    // TODO: Prompt for an expression to execute if register is '='.
    if (reg == '@' && g.lastExecutedRegister != 0)
        reg = g.lastExecutedRegister;
    else if (QString("\".*+").contains(regChar) || regChar.isLetterOrNumber())
        g.lastExecutedRegister = reg;
    else
        return false;

    // FIXME: In Vim it's possible to interrupt recursive macro with <C-c>.
    //        One solution may be to call QApplication::processEvents() and check if <C-c> was
    //        used when a mapping is active.
    // According to Vim, register is executed like mapping.
    prependMapping(Inputs(registerContents(reg), false, false));

    return true;
}

EventResult FakeVimHandler::Private::handleExMode(const Input &input)
{
    // handle C-R, C-R C-W, C-R {register}
    if (handleCommandBufferPaste(input))
        return EventHandled;

    if (input.isEscape()) {
        g.commandBuffer.clear();
        leaveCurrentMode();
        g.submode = NoSubMode;
    } else if (g.submode == CtrlVSubMode) {
        g.commandBuffer.insertChar(input.raw());
        g.submode = NoSubMode;
    } else if (input.isControl('v')) {
        g.submode = CtrlVSubMode;
        g.subsubmode = NoSubSubMode;
        return EventHandled;
    } else if (input.isBackspace()) {
        if (g.commandBuffer.isEmpty()) {
            leaveVisualMode();
            leaveCurrentMode();
        } else if (g.commandBuffer.hasSelection()) {
            g.commandBuffer.deleteSelected();
        } else {
            g.commandBuffer.deleteChar();
        }
    } else if (input.isKey(Key_Tab)) {
        // FIXME: Complete actual commands.
        g.commandBuffer.historyUp();
    } else if (input.isReturn()) {
        showMessage(MessageCommand, g.commandBuffer.display());
        handleExCommand(g.commandBuffer.contents());
        g.commandBuffer.clear();
    } else if (!g.commandBuffer.handleInput(input)) {
        qDebug() << "IGNORED IN EX-MODE: " << input.key() << input.text();
        return EventUnhandled;
    }

    return EventHandled;
}

EventResult FakeVimHandler::Private::handleSearchSubSubMode(const Input &input)
{
    EventResult handled = EventHandled;

    // handle C-R, C-R C-W, C-R {register}
    if (handleCommandBufferPaste(input))
        return handled;

    if (input.isEscape()) {
        g.currentMessage.clear();
        setPosition(m_searchStartPosition);
        scrollToLine(m_searchFromScreenLine);
    } else if (input.isBackspace()) {
        if (g.searchBuffer.isEmpty())
            leaveCurrentMode();
        else if (g.searchBuffer.hasSelection())
            g.searchBuffer.deleteSelected();
        else
            g.searchBuffer.deleteChar();
    } else if (input.isReturn()) {
        const QString &needle = g.searchBuffer.contents();
        if (!needle.isEmpty())
            g.lastSearch = needle;
        else
            g.searchBuffer.setContents(g.lastSearch);

        updateFind(true);

        if (finishSearch()) {
            if (g.submode != NoSubMode)
                finishMovement(g.searchBuffer.prompt() + g.lastSearch + '\n');
            if (g.currentMessage.isEmpty())
                showMessage(MessageCommand, g.searchBuffer.display());
        } else {
            handled = EventCancelled; // Not found so cancel mapping if any.
        }
    } else if (input.isKey(Key_Tab)) {
        g.searchBuffer.insertChar(QChar(9));
    } else if (!g.searchBuffer.handleInput(input)) {
        //qDebug() << "IGNORED IN SEARCH MODE: " << input.key() << input.text();
        return EventUnhandled;
    }

    if (input.isReturn() || input.isEscape()) {
        g.searchBuffer.clear();
        leaveCurrentMode();
    } else {
        updateFind(false);
    }

    return handled;
}

// This uses 0 based line counting (hidden lines included).
int FakeVimHandler::Private::parseLineAddress(QString *cmd, bool *hasAddress)
{
    //qDebug() << "CMD: " << cmd;
    if (cmd->isEmpty())
        return -1;

    if (hasAddress)
        *hasAddress = true;

    int result = -1;
    QChar c = cmd->at(0);
    if (c == '.') { // current line
        result = cursorBlockNumber();
        cmd->remove(0, 1);
    } else if (c == '$') { // last line
        result = document()->blockCount() - 1;
        cmd->remove(0, 1);
    } else if (c == '\'') { // mark
        cmd->remove(0, 1);
        if (cmd->isEmpty()) {
            showMessage(MessageError, msgMarkNotSet(QString()));
            return -1;
        }
        c = cmd->at(0);
        Mark m = mark(c);
        if (!m.isValid() || !m.isLocal(m_currentFileName)) {
            showMessage(MessageError, msgMarkNotSet(c));
            return -1;
        }
        cmd->remove(0, 1);
        result = m.position(document()).line;
    } else if (c.isDigit()) { // line with given number
        result = 0;
    } else if (c == '-' || c == '+') { // add or subtract from current line number
        result = cursorBlockNumber();
    } else if (c == '/' || c == '?'
        || (c == '\\' && cmd->size() > 1 && QString("/?&").contains(cmd->at(1)))) {
        // search for expression
        SearchData sd;
        if (c == '/' || c == '?') {
            const int end = findUnescaped(c, *cmd, 1);
            if (end == -1)
                return -1;
            sd.needle = cmd->mid(1, end - 1);
            cmd->remove(0, end + 1);
        } else {
            c = cmd->at(1);
            cmd->remove(0, 2);
            sd.needle = (c == '&') ? g.lastSubstitutePattern : g.lastSearch;
        }
        sd.forward = (c != '?');
        const QTextBlock b = block();
        const int pos = b.position() + (sd.forward ? b.length() - 1 : 0);
        QTextCursor tc = search(sd, pos, 1, true);
        g.lastSearch = sd.needle;
        if (tc.isNull())
            return -1;
        result = tc.block().blockNumber();
    } else {
        if (hasAddress)
            *hasAddress = false;
        return cursorBlockNumber();
    }

    // basic arithmetic ("-3+5" or "++" means "+2" etc.)
    int n = 0;
    bool add = true;
    int i = 0;
    for (; i < cmd->size(); ++i) {
        c = cmd->at(i);
        if (c == '-' || c == '+') {
            if (n != 0)
                result = result + (add ? n - 1 : -(n - 1));
            add = c == '+';
            result = result + (add ? 1 : -1);
            n = 0;
        } else if (c.isDigit()) {
            n = n * 10 + c.digitValue();
        } else if (!c.isSpace()) {
            break;
        }
    }
    if (n != 0)
        result = result + (add ? n - 1 : -(n - 1));
    *cmd = cmd->mid(i).trimmed();

    return result;
}

void FakeVimHandler::Private::setCurrentRange(const Range &range)
{
    setAnchorAndPosition(range.beginPos, range.endPos);
    g.rangemode = range.rangemode;
}

bool FakeVimHandler::Private::parseExCommand(QString *line, ExCommand *cmd)
{
    *cmd = ExCommand();
    if (line->isEmpty())
        return false;

    // parse range first
    if (!parseLineRange(line, cmd))
        return false;

    // ":normal" reaches to the end of the line: Vim counts a "|" as one of the
    // keys to replay rather than as the start of another command, which is why
    // it cannot be followed by one.
    static const QRegularExpression normalRe("^norm(a(l)?)?(!|\\s|$)");
    if (normalRe.match(*line).hasMatch()) {
        cmd->cmd = line->trimmed();
        static const QRegularExpression nonLetter("(?=[^a-zA-Z])");
        cmd->args = cmd->cmd.section(nonLetter, 1);
        if (!cmd->args.isEmpty()) {
            cmd->cmd.chop(cmd->args.size());
            cmd->hasBang = cmd->args.startsWith('!');
            if (cmd->hasBang)
                cmd->args = cmd->args.mid(1);
            // Only the blank telling the keys from the command name goes.
            if (cmd->args.startsWith(' ') || cmd->args.startsWith('\t'))
                cmd->args = cmd->args.mid(1);
        }
        line->clear();
        return true;
    }

    // get first command from command line
    QChar close;
    bool subst = false;
    int i = 0;
    for (; i < line->size(); ++i) {
        const QChar &c = line->at(i);
        if (c == '\\') {
            ++i; // skip escaped character
        } else if (close.isNull()) {
            if (c == '|') {
                // "||" is the logical-or operator (e.g. in :echo/:if), not a
                // command separator; only a single "|" splits commands.
                if (i + 1 < line->size() && line->at(i + 1) == '|') {
                    ++i;
                    continue;
                }
                // split on |
                break;
            } else if (c == '/') {
                subst = i > 0 && (line->at(i - 1) == 's');
                close = c;
            } else if (c == '"' || c == '\'') {
                close = c;
            }
        } else if (c == close) {
            if (subst)
                subst = false;
            else
                close = QChar();
        }
    }

    cmd->cmd = line->mid(0, i).trimmed();

    // command arguments starts with first non-letter character
    static const QRegularExpression regexp("(?=[^a-zA-Z])");
    cmd->args = cmd->cmd.section(regexp, 1);
    if (!cmd->args.isEmpty()) {
        cmd->cmd.chop(cmd->args.size());
        cmd->args = cmd->args.trimmed();

        // '!' at the end of command
        cmd->hasBang = cmd->args.startsWith('!');
        if (cmd->hasBang)
            cmd->args = cmd->args.mid(1).trimmed();
    }

    // remove the first command from command line
    line->remove(0, i + 1);

    return true;
}

bool FakeVimHandler::Private::parseLineRange(QString *line, ExCommand *cmd)
{
    // remove leading colons and spaces
    static const QRegularExpression regexp("^\\s*(:+\\s*)*");
    line->remove(regexp);

    // special case ':!...' (use invalid range)
    if (line->startsWith('!')) {
        cmd->range = Range();
        return true;
    }

    // FIXME: that seems to be different for %w and %s
    if (line->startsWith('%'))
        line->replace(0, 1, "1,$");

    bool hasAddress = false;
    int beginLine = parseLineAddress(line, &hasAddress);
    int endLine;
    if (line->startsWith(',')) {
        hasAddress = true;
        *line = line->mid(1).trimmed();
        endLine = parseLineAddress(line);
    } else {
        endLine = beginLine;
    }
    if (beginLine == -1 || endLine == -1)
        return false;

    const int beginPos = firstPositionInLine(qMin(beginLine, endLine) + 1, false);
    const int endPos = lastPositionInLine(qMax(beginLine, endLine) + 1, false);
    cmd->range = Range(beginPos, endPos, RangeLineMode);
    cmd->hasRange = hasAddress;
    cmd->count = beginLine;

    return true;
}

void FakeVimHandler::Private::parseRangeCount(const QString &line, Range *range) const
{
    bool ok;
    const int count = qAbs(line.trimmed().toInt(&ok));
    if (ok) {
        const int beginLine = blockAt(range->endPos).blockNumber() + 1;
        const int endLine = qMin(beginLine + count - 1, document()->blockCount());
        range->beginPos = firstPositionInLine(beginLine, false);
        range->endPos = lastPositionInLine(endLine, false);
    }
}

// use handleExCommand for invoking commands that might move the cursor
void FakeVimHandler::Private::handleCommand(const QString &cmd)
{
    handleExCommand(cmd);
}

bool FakeVimHandler::Private::handleExSubstituteCommand(const ExCommand &cmd)
{
    // :[range]s[ubstitute]/{pattern}/{string}/[flags] [count]
    if (!cmd.matches("s", "substitute")
        && !(cmd.cmd.isEmpty() && !cmd.args.isEmpty() && QString("&~").contains(cmd.args[0]))) {
        return false;
    }

    int count = 1;
    QString line = cmd.args;
    static const QRegularExpression regexp("\\d+$");
    const QRegularExpressionMatch match = regexp.match(line);
    if (match.hasMatch()) {
        count = match.captured().toInt();
        line = line.left(match.capturedStart()).trimmed();
    }

    if (cmd.cmd.isEmpty()) {
        // keep previous substitution flags on '&&' and '~&'
        if (line.size() > 1 && line[1] == '&')
            g.lastSubstituteFlags += line.mid(2);
        else
            g.lastSubstituteFlags = line.mid(1);
        if (line[0] == '~')
            g.lastSubstitutePattern = g.lastSearch;
    } else {
        if (line.isEmpty()) {
            g.lastSubstituteFlags.clear();
        } else {
            // we have /{pattern}/{string}/[flags]  now
            const QChar separator = line.at(0);
            int pos1 = findUnescaped(separator, line, 1);
            if (pos1 == -1)
                return false;
            int pos2 = findUnescaped(separator, line, pos1 + 1);
            if (pos2 == -1)
                pos2 = line.size();

            g.lastSubstitutePattern = line.mid(1, pos1 - 1);
            g.lastSubstituteReplacement = line.mid(pos1 + 1, pos2 - pos1 - 1);
            g.lastSubstituteFlags = line.mid(pos2 + 1);
        }
    }

    count = qMax(1, count);
    QString needle = g.lastSubstitutePattern;

    if (g.lastSubstituteFlags.contains('i'))
        needle.prepend("\\c");

    const QRegularExpression pattern = vimPatternToQtPattern(needle);

    QTextBlock lastBlock;
    QTextBlock firstBlock;
    const bool global = g.lastSubstituteFlags.contains('g');
    for (int a = 0; a != count; ++a) {
        for (QTextBlock block = blockAt(cmd.range.endPos);
            block.isValid() && block.position() + block.length() > cmd.range.beginPos;
            block = block.previous()) {
            QString text = block.text();
            if (substituteText(&text, pattern, g.lastSubstituteReplacement, global)) {
                firstBlock = block;
                if (!lastBlock.isValid()) {
                    lastBlock = block;
                    beginEditBlock();
                }
                QTextCursor tc = m_cursor;
                const int pos = block.position();
                const int anchor = pos + block.length() - 1;
                tc.setPosition(anchor);
                tc.setPosition(pos, KeepAnchor);
                tc.insertText(text);
            }
        }
    }

    if (lastBlock.isValid()) {
        m_buffer->undoState.position = CursorPosition(firstBlock.blockNumber(), 0);

        leaveVisualMode();
        setPosition(lastBlock.position());
        setAnchor();
        moveToFirstNonBlankOnLine();

        endEditBlock();
    }

    return true;
}

bool FakeVimHandler::Private::handleExTabNextCommand(const ExCommand &cmd)
{
    if (!cmd.matches("tabn", "tabnext"))
        return false;

    q->tabNextRequested();
    return true;
}

bool FakeVimHandler::Private::handleExTabPreviousCommand(const ExCommand &cmd)
{
    if (!cmd.matches("tabp", "tabprevious"))
        return false;

    q->tabPreviousRequested();
    return true;
}

bool FakeVimHandler::Private::handleExTagCommand(const ExCommand &cmd)
{
    // :ta[g] with an argument follows a (new) symbol; bare :ta[g] moves forward
    // on the tag stack. :po[p] moves back. CTRL-] and CTRL-T map to the first
    // and last of these directly (QTCREATORBUG-11754).
    if (cmd.matches("ta", "tag")) {
        if (cmd.args.isEmpty())
            q->tagStackRequested(count());
        else
            q->tagJumpRequested();
        return true;
    }
    if (cmd.matches("po", "pop")) {
        q->tagStackRequested(-count());
        return true;
    }
    return false;
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
    // Vim's "x" is Visual mode and "v" is Visual plus Select mode. There is no
    // Select mode here, so both are the same and have to use the mode code
    // that currentModeCode() reports for a visual selection.
    if (cmd == "xm" || cmd == "xmap") { modes = "v"; type = Map; } else
    if (cmd == "smap") { modes = "s"; type = Map; } else
    if (cmd == "omap") { modes = "o"; type = Map; } else
    if (cmd == "map!") { modes = "ic"; type = Map; } else
    if (cmd == "im" || cmd == "imap") { modes = "i"; type = Map; } else
    if (cmd == "lm" || cmd == "lmap") { modes = "l"; type = Map; } else
    if (cmd == "cm" || cmd == "cmap") { modes = "c"; type = Map; } else

    if (cmd == "no" || cmd == "noremap") { modes = "nvo"; type = Noremap; } else
    if (cmd == "nn" || cmd == "nnoremap") { modes = "n"; type = Noremap; } else
    if (cmd == "vn" || cmd == "vnoremap") { modes = "v"; type = Noremap; } else
    if (cmd == "xn" || cmd == "xnoremap") { modes = "v"; type = Noremap; } else
    if (cmd == "snor" || cmd == "snoremap") { modes = "s"; type = Noremap; } else
    if (cmd == "ono" || cmd == "onoremap") { modes = "o"; type = Noremap; } else
    if (cmd == "no!" || cmd == "noremap!") { modes = "ic"; type = Noremap; } else
    if (cmd == "ino" || cmd == "inoremap") { modes = "i"; type = Noremap; } else
    if (cmd == "ln" || cmd == "lnoremap") { modes = "l"; type = Noremap; } else
    if (cmd == "cno" || cmd == "cnoremap") { modes = "c"; type = Noremap; } else

    if (cmd == "unm" || cmd == "unmap") { modes = "nvo"; type = Unmap; } else
    if (cmd == "nun" || cmd == "nunmap") { modes = "n"; type = Unmap; } else
    if (cmd == "vu" || cmd == "vunmap") { modes = "v"; type = Unmap; } else
    if (cmd == "xu" || cmd == "xunmap") { modes = "v"; type = Unmap; } else
    if (cmd == "sunm" || cmd == "sunmap") { modes = "s"; type = Unmap; } else
    if (cmd == "ou" || cmd == "ounmap") { modes = "o"; type = Unmap; } else
    if (cmd == "unm!" || cmd == "unmap!") { modes = "ic"; type = Unmap; } else
    if (cmd == "iu" || cmd == "iunmap") { modes = "i"; type = Unmap; } else
    if (cmd == "lu" || cmd == "lunmap") { modes = "l"; type = Unmap; } else
    if (cmd == "cu" || cmd == "cunmap") { modes = "c"; type = Unmap; }

    else
        return false;

    QString args = cmd0.args;
    bool silent = false;
    bool unique = false;
    bool expression = false;
    forever {
        if (eatString("<silent>", &args)) {
            silent = true;
            continue;
        } else if (eatString("<unique>", &args)) {
            continue;
        } else if (eatString("<special>", &args)) {
            continue;
        } else if (eatString("<buffer>", &args)) {
            notImplementedYet();
            continue;
        } else if (eatString("<script>", &args)) {
            notImplementedYet();
            continue;
        } else if (eatString("<expr>", &args)) {
            expression = true;
            continue;
        }
        break;
    }

    static const QRegularExpression regexp("\\s+");
    const QString lhs = args.section(regexp, 0, 0);
    const QString rhs = args.section(regexp, 1);
    if ((rhs.isNull() && type != Unmap) || (!rhs.isNull() && type == Unmap)) {
        // FIXME: Dump mappings here.
        //qDebug() << g.mappings;
        return true;
    }

    Inputs key(lhs);
    //qDebug() << "MAPPING: " << modes << lhs << rhs;
    switch (type) {
        case Unmap:
            for (char c : std::as_const(modes))
                MappingsIterator(&g.mappings, c, key).remove();
            break;
        case Map: Q_FALLTHROUGH();
        case Noremap: {
            const Inputs inputs(rhs, type == Noremap, silent, expression, m_vim9);
            for (char c : std::as_const(modes))
                MappingsIterator(&g.mappings, c).setInputs(key, inputs, unique);
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
        for (const QString &item : g.commandBuffer.historyItems()) {
            ++i;
            info += QString("%1 %2\n").arg(i, -8).arg(item);
        }
        q->extraInformationChanged(info);
    } else {
        notImplementedYet();
    }

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
        for (auto it = g.registers.cbegin(), end = g.registers.cend(); it != end; ++it) {
            if (it.key() > '9')
                regs += char(it.key());
        }
    }
    QString info;
    info += "--- Registers ---\n";
    for (char reg : std::as_const(regs)) {
        QString value = quoteUnprintable(registerContents(reg));
        info += QString("\"%1   %2\n").arg(reg).arg(value);
    }
    q->extraInformationChanged(info);

    return true;
}

// 'commentstring' is buffer-local in Vim, so it is not kept in the settings.
static bool isCommentStringOption(const QString &name)
{
    return name == "commentstring" || name == "cms";
}

// 'filetype' is not stored either; it drives the FileType autocommands.
static bool isFileTypeOption(const QString &name)
{
    return name == "filetype" || name == "ft";
}

bool FakeVimHandler::Private::handleExSetCommand(const ExCommand &cmd)
{
    // :se[t]
    if (!cmd.matches("se", "set"))
        return false;

    clearMessage();

    // "filetype"/"ft" is not a stored option; it drives FileType autocommands.
    if (cmd.args.startsWith("filetype=") || cmd.args.startsWith("ft=")) {
        setFileType(cmd.args.section('=', 1));
        return true;
    }

    static const QRegularExpression addRe("^([a-zA-Z]+)([-+^])=(.*)$");
    const QRegularExpressionMatch add = addRe.match(cmd.args);
    if (add.hasMatch()) {
        // ":set {option}+=" adds to what is there, "-=" takes away and "^="
        // puts in front, which is how a path option is added to.
        const QString optionName = add.captured(1);
        const QString what = add.captured(3);
        VimValue current;
        if (!optionValue(optionName, &current)) {
            showMessage(MessageError, Tr::tr("Unknown option:") + ' ' + optionName);
        } else {
            QString value = current.toString();
            const QChar how = add.captured(2).at(0);
            if (how == '+')
                value += (value.isEmpty() ? QString() : QString(',')) + what;
            else if (how == '^')
                value = what + (value.isEmpty() ? QString() : QString(',')) + value;
            else
                value.remove(QRegularExpression("(^|,)" + QRegularExpression::escape(what)
                                                + "(?=,|$)"));
            setOption(optionName, VimValue(value));
        }
    } else if (cmd.args.contains('=')) {
        // Non-boolean config to set.
        int p = cmd.args.indexOf('=');
        const QString optionName = cmd.args.left(p);
        if (isCommentStringOption(optionName)) {
            m_commentString = cmd.args.mid(p + 1);
            return true;
        }
        QString error = s.trySetValue(optionName, cmd.args.mid(p + 1));
        if (!error.isEmpty())
            showMessage(MessageError, error);
    } else if (cmd.args == "commentstring?" || cmd.args == "cms?") {
        showMessage(MessageInfo, "commentstring=" + commentString());
    } else if (cmd.args.endsWith('&') || cmd.args.endsWith("&vim")) {
        // ":set {option}&" puts an option back to what it started as, which is
        // how a script leaves one as it found it. Vim's "&vim" asks for its own
        // default rather than Vi's; there is only one default here.
        QString optionName = cmd.args;
        optionName.chop(optionName.endsWith("&vim") ? 4 : 1);
        if (FvBaseAspect *act = s.item(Utils::keyFromString(optionName)))
            act->setVariantValue(act->defaultVariantValue());
        else
            showMessage(MessageError, Tr::tr("Unknown option:") + ' ' + cmd.args);
    } else {
        QString optionName = cmd.args;

        bool toggleOption = optionName.endsWith('!');
        bool printOption = !toggleOption && optionName.endsWith('?');
        if (printOption || toggleOption)
            optionName.chop(1);

        bool negateOption = optionName.startsWith("no");
        if (negateOption)
            optionName.remove(0, 2);

        FvBaseAspect *act = s.item(Utils::keyFromString(optionName));
        if (!act) {
            showMessage(MessageError, Tr::tr("Unknown option:") + ' ' + cmd.args);
        } else if (act->defaultVariantValue().typeId() == QMetaType::Bool) {
            bool oldValue = act->variantValue().toBool();
            if (printOption) {
                showMessage(MessageInfo, QLatin1String(oldValue ? "" : "no")
                            + act->settingsKey().toByteArray().toLower());
            } else if (toggleOption || negateOption == oldValue) {
                act->setVariantValue(!oldValue);
            }
        } else if (negateOption && !printOption) {
            showMessage(MessageError, Tr::tr("Invalid argument:") + ' ' + cmd.args);
        } else if (toggleOption) {
            showMessage(MessageError, Tr::tr("Trailing characters:") + ' ' + cmd.args);
        } else {
            showMessage(MessageInfo, act->settingsKey().toByteArray().toLower() + "="
                        + act->variantValue().toString());
        }
    }
    updateEditor();
    updateHighlights();
    return true;
}

bool FakeVimHandler::Private::handleExNormalCommand(const ExCommand &cmd)
{
    // :[range]norm[al][!] {commands}
    if (!cmd.matches("norm", "normal"))
        return false;

    const int beginLine = lineForPosition(cmd.range.beginPos);
    const int endLine = lineForPosition(cmd.range.endPos);

    // Vim aborts a command the keys did not finish, as if <ESC> had been typed,
    // which is what ends a pending insert mode (QTCREATORBUG-25820). A mode the
    // keys did finish is kept, so ":normal! v" leaves a selection behind for
    // whatever runs next to work on.
    const auto finishNormal = [this] {
        if (isInsertMode())
            replay("<ESC>");
    };

    // Without an explicit range Vim runs the commands once at the current
    // cursor position.
    if (!cmd.hasRange) {
        //qDebug() << "REPLAY NORMAL: " << quoteUnprintable(cmd.args);
        replay(cmd.args);
        finishNormal();
        return true;
    }

    // With a range Vim runs the commands once per line, placing the cursor at
    // the start of each line first (this also applies to a single addressed
    // line, e.g. ":3normal A;"). Collect a cursor per line up front so the
    // positions stay valid even when the commands insert or delete lines
    // (e.g. :%normal dd), mirroring the approach used for :global.
    QList<QTextCursor> cursors;
    cursors.reserve(endLine - beginLine + 1);
    for (int line = beginLine; line <= endLine; ++line) {
        QTextCursor tc(document());
        tc.setPosition(firstPositionInLine(line));
        cursors.append(tc);
    }

    beginEditBlock();
    for (const QTextCursor &tc : std::as_const(cursors)) {
        // Each line starts over in normal mode, so a selection one line's keys
        // left behind must not reach the next one.
        if (isVisualMode())
            leaveVisualMode();
        setPosition(tc.position());
        replay(cmd.args);
        finishNormal();
    }
    endEditBlock();

    return true;
}

bool FakeVimHandler::Private::handleExYankDeleteCommand(const ExCommand &cmd)
{
    // :[range]d[elete] [x] [count]
    // :[range]y[ank] [x] [count]
    const bool remove = cmd.matches("d", "delete");
    if (!remove && !cmd.matches("y", "yank"))
        return false;

    // get register from arguments
    const bool hasRegisterArg = !cmd.args.isEmpty() && !cmd.args.at(0).isDigit();
    const int r = hasRegisterArg ? cmd.args.at(0).unicode() : m_register;

    // get [count] from arguments
    Range range = cmd.range;
    parseRangeCount(cmd.args.mid(hasRegisterArg ? 1 : 0).trimmed(), &range);

    yankText(range, r);

    if (remove) {
        leaveVisualMode();
        setPosition(range.beginPos);
        pushUndoState();
        setCurrentRange(range);
        removeText(currentRange());
    }

    return true;
}

bool FakeVimHandler::Private::handleExChangeCommand(const ExCommand &cmd)
{
    // :[range]c[hange]
    if (!cmd.matches("c", "change"))
        return false;

    Range range = cmd.range;
    range.rangemode = RangeLineModeExclusive;
    removeText(range);
    insertAutomaticIndentation(true, cmd.hasBang);

    // FIXME: In Vim same or less number of lines can be inserted and position after insertion is
    //        beginning of last inserted line.
    enterInsertMode();

    return true;
}

bool FakeVimHandler::Private::handleExMoveCommand(const ExCommand &cmd)
{
    // :[range]m[ove] {address}
    if (!cmd.matches("m", "move"))
        return false;

    QString lineCode = cmd.args;

    const int startLine = blockAt(cmd.range.beginPos).blockNumber();
    const int endLine = blockAt(cmd.range.endPos).blockNumber();
    const int lines = endLine - startLine + 1;

    int targetLine = lineCode == "0" ? -1 : parseLineAddress(&lineCode);
    if (targetLine >= startLine && targetLine < endLine) {
        showMessage(MessageError, Tr::tr("Move lines into themselves."));
        return true;
    }

    CursorPosition lastAnchor = markLessPosition();
    CursorPosition lastPosition = markGreaterPosition();

    recordJump();
    setPosition(cmd.range.beginPos);
    pushUndoState();

    setCurrentRange(cmd.range);
    QString text = selectText(cmd.range);
    removeText(currentRange());

    // The last line has no trailing newline, so its linewise selection carries
    // a leading newline instead; normalize to the usual "line\n" form so the
    // text below can be reinserted at any block boundary.
    if (text.startsWith(QLatin1Char('\n')))
        text = text.mid(1) + QLatin1Char('\n');

    const bool insertAtEnd = targetLine == document()->blockCount();
    if (targetLine >= startLine)
        targetLine -= lines;
    QTextBlock block = document()->findBlockByNumber(insertAtEnd ? targetLine : targetLine + 1);
    setPosition(block.position());
    setAnchor();

    if (insertAtEnd) {
        moveBehindEndOfLine();
        text.chop(1);
        insertText(QString("\n"));
    }
    insertText(text);

    if (!insertAtEnd)
        moveUp(1);
    if (s.startOfLine())
        moveToFirstNonBlankOnLine();

    if (lastAnchor.line >= startLine && lastAnchor.line <= endLine)
        lastAnchor.line += targetLine - startLine + 1;
    if (lastPosition.line >= startLine && lastPosition.line <= endLine)
        lastPosition.line += targetLine - startLine + 1;
    setMark('<', lastAnchor);
    setMark('>', lastPosition);

    if (lines > 2)
        showMessage(MessageInfo, Tr::tr("%n lines moved.", nullptr, lines));

    return true;
}

bool FakeVimHandler::Private::handleExJoinCommand(const ExCommand &cmd)
{
    // :[range]j[oin][!] [count]
    // FIXME: Argument [count] can follow immediately.
    if (!cmd.matches("j", "join"))
        return false;

    // get [count] from arguments
    bool ok;
    int count = cmd.args.toInt(&ok);

    if (ok) {
        setPosition(cmd.range.endPos);
    } else {
        setPosition(cmd.range.beginPos);
        const int startLine = blockAt(cmd.range.beginPos).blockNumber();
        const int endLine = blockAt(cmd.range.endPos).blockNumber();
        count = endLine - startLine + 1;
    }

    moveToStartOfLine();
    pushUndoState();
    joinLines(count, cmd.hasBang);

    moveToFirstNonBlankOnLine();

    return true;
}

bool FakeVimHandler::Private::handleExWriteCommand(const ExCommand &cmd)
{
    // Note: The cmd.args.isEmpty() case is handled by handleExPluginCommand.
    // :w, :x, :wq, ...
    //static QRegularExpression reWrite("^[wx]q?a?!?( (.*))?$");
    if (cmd.cmd != "w" && cmd.cmd != "x" && cmd.cmd != "wq")
        return false;

    triggerAutocmd("BufWritePre");

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
    //const bool quit = prefix.contains('q') || prefix.contains('x');
    //const bool quitAll = quit && prefix.contains('a');
    QString fileName = replaceTildeWithHome(cmd.args);
    if (fileName.isEmpty())
        fileName = m_currentFileName;
    QFile file1(fileName);
    const bool exists = file1.exists();
    if (exists && !forced && !noArgs) {
        showMessage(MessageError, Tr::tr
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
            showMessage(MessageError, Tr::tr
               ("Cannot open file \"%1\" for writing").arg(fileName));
        }
        // Check result by reading back.
        QFile file3(fileName);
        if (!file3.open(QIODevice::ReadOnly))
            return false;
        QByteArray ba = file3.readAll();
        showMessage(MessageInfo, Tr::tr("\"%1\" %2 %3L, %4C written.")
            .arg(fileName).arg(exists ? QString(" ") : Tr::tr(" [New] "))
            .arg(ba.count('\n')).arg(ba.size()));
        //if (quitAll)
        //    passUnknownExCommand(forced ? "qa!" : "qa");
        //else if (quit)
        //    passUnknownExCommand(forced ? "q!" : "q");
    } else {
        showMessage(MessageError, Tr::tr
            ("Cannot open file \"%1\" for reading").arg(fileName));
    }
    triggerAutocmd("BufWritePost");
    return true;
}

bool FakeVimHandler::Private::handleExReadCommand(const ExCommand &cmd)
{
    // :r[ead]
    if (!cmd.matches("r", "read"))
        return false;

    beginEditBlock();

    moveToStartOfLine();
    moveDown();
    int pos = position();

    m_currentFileName = replaceTildeWithHome(cmd.args);
    QFile file(m_currentFileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QTextStream ts(&file);
    QString data = ts.readAll();
    insertText(data);

    setAnchorAndPosition(pos, pos);

    endEditBlock();

    showMessage(MessageInfo, Tr::tr("\"%1\" %2L, %3C")
        .arg(m_currentFileName).arg(data.count('\n')).arg(data.size()));

    return true;
}

bool FakeVimHandler::Private::handleExBangCommand(const ExCommand &cmd) // :!
{
    if (!cmd.cmd.isEmpty() || !cmd.hasBang)
        return false;

    bool replaceText = cmd.range.isValid();
    const QString command = QString(cmd.cmd.mid(1) + ' ' + cmd.args).trimmed();
    const QString input = replaceText ? selectText(cmd.range) : QString();

    QString result;
    q->processOutput(command, input, &result);

    if (replaceText) {
        setCurrentRange(cmd.range);
        int targetPosition = firstPositionInLine(lineForPosition(cmd.range.beginPos));
        beginEditBlock();
        removeText(currentRange());
        insertText(result);
        setPosition(targetPosition);
        endEditBlock();
        leaveVisualMode();
        //qDebug() << "FILTER: " << command;
        showMessage(MessageInfo, Tr::tr("%n lines filtered.", nullptr,
            input.count('\n')));
    } else if (!result.isEmpty()) {
        q->extraInformationChanged(result);
    }

    return true;
}

bool FakeVimHandler::Private::handleExDelMarksCommand(const ExCommand &cmd)
{
    // :delm[arks] {marks}   delete the listed marks (a range like "a-z" is
    //                       allowed, spaces are optional separators)
    // :delm[arks]!          delete all lowercase marks of the current buffer
    if (!cmd.matches("delm", "delmarks"))
        return false;

    if (cmd.args.isEmpty()) {
        if (cmd.hasBang) {
            for (char c = 'a'; c <= 'z'; ++c)
                m_buffer->marks.remove(QLatin1Char(c));
        } else {
            showMessage(MessageError, Tr::tr("Argument required."));
        }
        return true;
    }

    const QString marks = cmd.args;
    for (int i = 0; i < marks.size(); ++i) {
        const QChar c = marks.at(i);
        if (c.isSpace())
            continue;
        // A range such as "a-z" removes every mark between the endpoints.
        if (i + 2 < marks.size() && marks.at(i + 1) == QLatin1Char('-')
                && marks.at(i + 2).unicode() >= c.unicode()) {
            for (ushort u = c.unicode(); u <= marks.at(i + 2).unicode(); ++u)
                removeMark(QChar(u));
            i += 2;
        } else {
            removeMark(c);
        }
    }
    return true;
}

bool FakeVimHandler::Private::handleExShiftCommand(const ExCommand &cmd)
{
    // :[range]{<|>}* [count]
    if (!cmd.cmd.isEmpty() || (!cmd.args.startsWith('<') && !cmd.args.startsWith('>')))
        return false;

    const QChar c = cmd.args.at(0);

    // get number of repetition
    int repeat = 1;
    int i = 1;
    for (; i < cmd.args.size(); ++i) {
        const QChar c2 = cmd.args.at(i);
        if (c2 == c)
            ++repeat;
        else if (!c2.isSpace())
            break;
    }

    // get [count] from arguments
    Range range = cmd.range;
    parseRangeCount(cmd.args.mid(i), &range);

    setCurrentRange(range);
    if (c == '<')
        shiftRegionLeft(repeat);
    else
        shiftRegionRight(repeat);

    leaveVisualMode();

    return true;
}

bool FakeVimHandler::Private::handleExMultiRepeatCommand(const ExCommand &cmd)
{
    // :[range]g[lobal]/{pattern}/[cmd]
    // :[range]g[lobal]!/{pattern}/[cmd]
    // :[range]v[globa]!/{pattern}/[cmd]
    const bool hasG = cmd.matches("g", "global");
    const bool hasV = cmd.matches("v", "vglobal");
    if (!hasG && !hasV)
        return false;

    // Force operation on full lines, and full document if only
    // one line (the current one...) is specified
    int beginLine = lineForPosition(cmd.range.beginPos);
    int endLine = lineForPosition(cmd.range.endPos);
    if (beginLine == endLine) {
        beginLine = 0;
        endLine = lineForPosition(lastPositionInDocument());
    }

    const bool negates = hasV || cmd.hasBang;

    const QChar delim = cmd.args.front();
    const QString pattern = cmd.args.section(delim, 1, 1);
    const QRegularExpression re(pattern);

    QString innerCmd = cmd.args.section(delim, 2, 2);
    if (innerCmd.isEmpty())
        innerCmd = "p";

    QList<QTextCursor> matches;

    for (int line = beginLine; line <= endLine; ++line) {
        const int pos = firstPositionInLine(line);
        const Range range(pos, pos, RangeLineMode);
        const QString lineContents = selectText(range);
        const QRegularExpressionMatch match = re.match(lineContents);
        if (match.hasMatch() != negates) {
            QTextCursor tc(document());
            tc.setPosition(pos);
            matches.append(tc);
        }
    }

    beginEditBlock();

    for (const QTextCursor &tc : std::as_const(matches)) {
        setPosition(tc.position());
        handleExCommand(innerCmd);
    }

    endEditBlock();

    return true;
}

bool FakeVimHandler::Private::handleExSortCommand(const ExCommand &cmd)
{
    // :[range]sor[t][!] [b][f][i][n][o][r][u][x] [/{pattern}/]
    // FIXME: Only the ! for reverse is implemented.
    if (!cmd.matches("sor", "sort"))
        return false;

    // Force operation on full lines, and full document if only
    // one line (the current one...) is specified
    int beginLine = lineForPosition(cmd.range.beginPos);
    int endLine = lineForPosition(cmd.range.endPos);
    if (beginLine == endLine) {
        beginLine = 0;
        endLine = lineForPosition(lastPositionInDocument());
    }
    Range range(firstPositionInLine(beginLine),
                firstPositionInLine(endLine), RangeLineMode);

    QString input = selectText(range);
    if (input.endsWith('\n')) // It should always...
        input.chop(1);

    QStringList lines = input.split('\n');
    lines.sort();
    if (cmd.hasBang)
        std::reverse(lines.begin(), lines.end());
    QString res = lines.join('\n') + '\n';

    replaceText(range, res);

    return true;
}

bool FakeVimHandler::Private::handleExNohlsearchCommand(const ExCommand &cmd)
{
    // :noh, :nohl, ..., :nohlsearch
    if (cmd.cmd.size() < 3 || !QString("nohlsearch").startsWith(cmd.cmd))
        return false;

    g.highlightsCleared = true;
    updateHighlights();
    // When Qt Creator's own search is used, matches are highlighted by the
    // find tool, not tracked in m_highlighted, so updateHighlights() above
    // cannot clear them. Hide the find tool bar, which clears its highlights
    // and find scope (QTCREATORBUG-22298).
    if (s.useCoreSearch())
        q->findHideRequested();
    return true;
}

bool FakeVimHandler::Private::handleExUndoRedoCommand(const ExCommand &cmd)
{
    // :undo
    // :redo
    bool undo = cmd.matches("u", "undo");
    if (!undo && !cmd.matches("red", "redo"))
        return false;

    undoRedo(undo);

    return true;
}

bool FakeVimHandler::Private::handleExGotoCommand(const ExCommand &cmd)
{
    // :{address}
    if (!cmd.cmd.isEmpty() || !cmd.args.isEmpty())
        return false;

    const int beginLine = lineForPosition(cmd.range.endPos);
    setPosition(firstPositionInLine(beginLine));
    clearMessage();
    return true;
}

// Rewrite a Vim9 statement into the equivalent legacy ex-command the
// interpreter already understands: "var/const/final x = e" and a plain
// "x = e" become ":let", and a bare "Foo(...)" call becomes ":call".
static QString vim9Statement(const QString &line)
{
    const auto fixConcatAssign = [](QString s) {
        return s.replace("..=", ".="); // Vim9 string concat-assign
    };

    // Declarations: strip the keyword, an optional ": type" and default-init.
    static const QRegularExpression declRe("^(?:var|const|final)\\s+(.+)$");
    const QRegularExpressionMatch decl = declRe.match(line);
    if (decl.hasMatch()) {
        QString rest = decl.captured(1).trimmed();
        if (rest.startsWith('[')) // destructuring: var [a, b] = ...
            return "let " + fixConcatAssign(rest);
        static const QRegularExpression nameRe(
            "^([A-Za-z_][A-Za-z0-9_]*)\\s*(?::\\s*([^=]+?))?\\s*(=\\s*.*)?$");
        const QRegularExpressionMatch nm = nameRe.match(rest);
        if (!nm.hasMatch())
            return "let " + fixConcatAssign(rest);
        const QString name = nm.captured(1);
        if (!nm.captured(3).isEmpty())
            return "let " + name + " " + fixConcatAssign(nm.captured(3));
        const QString type = nm.captured(2).trimmed(); // no initializer -> default
        const QString def = type.startsWith("string") ? QStringLiteral("''")
                          : type.startsWith("list") ? QStringLiteral("[]")
                          : type.startsWith("dict") ? QStringLiteral("{}")
                          : QStringLiteral("0");
        return "let " + name + " = " + def;
    }

    // Plain assignment: lvalue [op]= rhs (no ":let").
    static const QRegularExpression assignRe(
        "^([@&$]?[A-Za-z_][A-Za-z0-9_:]*(?:\\[[^\\]]*\\]|\\.[A-Za-z_][A-Za-z0-9_]*)*)"
        "\\s*((?:\\.\\.|[-+*/%])?=)\\s*(.+)$");
    const QRegularExpressionMatch as = assignRe.match(line);
    if (as.hasMatch()) {
        QString op = as.captured(2);
        if (op == "..=")
            op = ".=";
        return "let " + as.captured(1) + " " + op + " " + as.captured(3);
    }

    // Bare call: in Vim9 a statement may be a plain function call, including a
    // builtin like "add(list, item)". Everything shaped like "name(" is one,
    // except the statement keywords that can also be written without a space.
    static const QRegularExpression callRe(
        "^([A-Za-z_][A-Za-z0-9_:#]*(?:\\.[A-Za-z_][A-Za-z0-9_]*)*)\\s*\\(.*$");
    const QRegularExpressionMatch call = callRe.match(line);
    if (call.hasMatch()) {
        static const QSet<QString> keywords = {
            "if", "elseif", "else", "endif", "while", "endwhile", "for",
            "endfor", "return", "break", "continue", "throw", "try", "catch",
            "finally", "endtry", "def", "enddef", "function", "endfunction",
            "var", "const", "final", "unlet", "echo", "echon", "echomsg",
            "echoerr", "execute", "exe", "eval", "call", "import", "export",
            "silent", "normal", "set", "setlocal", "augroup", "autocmd",
            "command", "source", "delete", "put", "copy", "move", "sort",
            "print", "substitute", "global", "range"
        };
        if (!keywords.contains(call.captured(1)))
            return "call " + line;
    }

    return line;
}

// Net bracket balance of a Vim9 line (skipping strings and a "#" comment),
// used to decide implicit line continuation.
static int vim9BracketDelta(const QString &s)
{
    int depth = 0;
    for (int i = 0; i < s.size(); ++i) {
        const QChar ch = s.at(i);
        if (ch == '"' || ch == '\'') {
            const QChar quote = ch;
            for (++i; i < s.size() && s.at(i) != quote; ++i) {
                if (quote == '"' && s.at(i) == '\\')
                    ++i;
            }
        } else if (ch == '#') {
            break; // rest of the line is a comment
        } else if (ch == '(' || ch == '[' || ch == '{') {
            ++depth;
        } else if (ch == ')' || ch == ']' || ch == '}') {
            --depth;
        }
    }
    return depth;
}

// A continuation line begins with a binary operator (Vim9 operator-first style).
static bool vim9StartsContinuation(const QString &s)
{
    if (s.startsWith("..") || s.startsWith("->") || s.startsWith("&&")
        || s.startsWith("||"))
        return true;
    if (s.isEmpty())
        return false;
    const QChar c = s.at(0);
    return c == '+' || c == '*' || c == '/' || c == '%' || c == '?' || c == ':'
        || c == '.' || c == ')' || c == ']' || c == '}';
}

// A line whose last token is clearly a binary operator continues onto the next.
static bool vim9EndsOpen(const QString &s)
{
    if (s.endsWith("..") || s.endsWith("&&") || s.endsWith("||") || s.endsWith("->"))
        return true;
    if (s.isEmpty())
        return false;
    const QChar c = s.at(s.size() - 1);
    return c == '+' || c == '*' || c == '/' || c == '%' || c == ',';
}

bool FakeVimHandler::Private::handleExSourceCommand(const ExCommand &cmd)
{
    // :source
    if (!cmd.matches("so", "source"))
        return false;

    QString fileName = replaceTildeWithHome(cmd.args);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showMessage(MessageError, Tr::tr("Cannot open file %1").arg(fileName));
        return true;
    }
    const QFileInfo fileInfo(fileName);
    const QString canonicalPath = fileInfo.canonicalFilePath();

    // A script that (directly or through an import) sources itself would
    // recurse until the stack is gone, so refuse a file already in flight.
    if (m_sourcesInFlight.contains(canonicalPath)) {
        showMessage(MessageError, Tr::tr("Recursive :source of %1").arg(fileName));
        return true;
    }

    // Detect the encoding like Vim's 'fileencodings': prefer UTF-8 and fall
    // back to the local 8-bit encoding for invalid byte sequences
    // (QTCREATORBUG-8776).
    const QByteArray data = file.readAll();
    file.close();
    QStringDecoder utf8(QStringDecoder::Utf8);
    QString text = utf8(data);
    if (utf8.hasError())
        text = QString::fromLocal8Bit(data);
    const QStringList rawLines = text.split('\n');

    // A leading "vim9script" command selects Vim9 syntax for the whole file:
    // "#" line comments (not "\"") and Vim9 expression semantics.
    bool fileVim9 = false;
    for (const QString &raw : rawLines) {
        const QString t = raw.trimmed();
        if (t.isEmpty() || t.startsWith('"'))
            continue;
        fileVim9 = t == "vim9script" || t.startsWith("vim9script ");
        break;
    }
    const QChar commentChar = fileVim9 ? '#' : '"';

    // Collect all executable command units first, then interpret them in one
    // pass so control-flow and function blocks can span several lines.
    QList<ExCommand> cmds;
    QString line;
    const auto flush = [&] {
        if (line.isEmpty())
            return;
        ExCommand c;
        QString cl = fileVim9 ? vim9Statement(line) : line;
        while (parseExCommand(&cl, &c))
            cmds.append(c);
        line.clear();
    };
    int depth = 0; // running bracket balance of the accumulated line (Vim9)
    QStringList exportedNames; // Vim9 ":export"ed def/var names of this script
    for (int i = 0; i < rawLines.size(); ++i) {
        QString next = rawLines.at(i).trimmed();
        if (next.startsWith(commentChar))
            continue; // full-line comment (also dropped inside a continuation)
        if (next == "vim9script" || next.startsWith("vim9script "))
            continue; // the mode marker itself is not a command to run
        if (next.startsWith('\\')) { // explicit continuation
            line += next.mid(1);
            depth += vim9BracketDelta(next.mid(1));
            continue;
        }
        // Vim9 "export def/var/const/final NAME ...": strip the keyword and
        // remember NAME so it can be handed to an importing script later.
        // "export" only ever starts a fresh top-level statement, so this
        // does not need to check whether the previous statement was flushed.
        if (fileVim9 && next.startsWith("export ")) {
            static const QRegularExpression exportNameRe(
                "^(?:def|var|const|final)\\s+([A-Za-z_][A-Za-z0-9_]*)");
            const QRegularExpressionMatch em = exportNameRe.match(next.mid(7));
            if (em.hasMatch())
                exportedNames.append(em.captured(1));
            next = next.mid(7);
        }
        // Heredoc: "let VAR =<< [trim] MARKER" gathers the following lines up to
        // MARKER into a list of strings. Only at statement start, not mid-line.
        const int hd = line.isEmpty() ? next.indexOf("=<<") : -1;
        if (hd >= 0) {
            QStringList opts = next.mid(hd + 3).split(' ', Qt::SkipEmptyParts);
            const bool trim = opts.removeAll("trim") > 0;
            opts.removeAll("eval"); // interpolation flag not honored yet
            const QString marker = opts.isEmpty() ? QString() : opts.constLast();
            QStringList body;
            while (++i < rawLines.size() && rawLines.at(i).trimmed() != marker)
                body.append(rawLines.at(i));
            if (trim && !body.isEmpty()) {
                const QString &first = body.first();
                int indent = 0;
                while (indent < first.size()
                       && (first.at(indent) == ' ' || first.at(indent) == '\t'))
                    ++indent;
                for (QString &b : body) {
                    int k = 0;
                    while (k < indent && k < b.size()
                           && (b.at(k) == ' ' || b.at(k) == '\t'))
                        ++k;
                    b = b.mid(k);
                }
            }
            QStringList items;
            for (const QString &b : std::as_const(body))
                items.append('\'' + QString(b).replace('\'', "''") + '\'');
            line = next.left(hd) + "= [" + items.join(", ") + ']';
            flush();
            continue;
        }
        // Vim9 implicit continuation: unclosed brackets, or an operator at the
        // end of the previous line or the start of this one.
        if (fileVim9 && !line.isEmpty()
            && (depth > 0 || vim9EndsOpen(line) || vim9StartsContinuation(next))) {
            line += ' ' + next;
            depth += vim9BracketDelta(next);
            continue;
        }
        flush();
        line = next;
        depth = fileVim9 ? vim9BracketDelta(next) : 0;
    }
    flush();

    const bool savedVim9 = m_vim9;
    m_vim9 = fileVim9;
    m_scriptFileStack.append(canonicalPath);
    m_sourcesInFlight.insert(canonicalPath);
    runExCommands(cmds);
    m_sourcesInFlight.remove(canonicalPath);
    m_scriptFileStack.removeLast();
    m_vim9 = savedVim9;

    if (fileVim9 && !exportedNames.isEmpty() && !canonicalPath.isEmpty()) {
        QMap<QString, VimValue> exports;
        for (const QString &name : std::as_const(exportedNames)) {
            if (g.userFunctions.contains(name))
                exports.insert(name, VimValue::func(name));
            else {
                VimValue v;
                if (variableValue(name, &v))
                    exports.insert(name, v);
            }
        }
        g.moduleExports.insert(canonicalPath, exports);
    }
    return true;
}

bool FakeVimHandler::Private::handleExImportCommand(const ExCommand &cmd)
{
    // :import ["autoload"] 'file' [as Name]  -- Vim9 module import. Sources
    // the file once (if not already imported) and binds its exported names
    // as a dict under Name, defaulting to the file's base name. "autoload"
    // is accepted but imports eagerly; lazy loading is not modeled.
    if (cmd.cmd != "import")
        return false;

    QString args = cmd.args.trimmed();
    const bool autoload = args.startsWith("autoload ");
    if (autoload)
        args = args.mid(9).trimmed();
    if (args.isEmpty() || (args.at(0) != '\'' && args.at(0) != '"')) {
        showMessage(MessageError, Tr::tr("Missing file name in :import"));
        return true;
    }
    const QChar quote = args.at(0);
    const int end = args.indexOf(quote, 1);
    if (end < 0) {
        showMessage(MessageError, Tr::tr("Missing closing quote in :import"));
        return true;
    }
    const QString path = args.mid(1, end - 1);
    const QString rest = args.mid(end + 1).trimmed();
    const QString alias = rest.startsWith("as ")
        ? rest.mid(3).trimmed() : QFileInfo(path).completeBaseName();

    const QString resolved = replaceTildeWithHome(path);
    QString canonicalPath = QFileInfo(resolved).canonicalFilePath();
    if (QFileInfo(resolved).isRelative() && !m_scriptFileStack.isEmpty()) {
        // Vim finds an "import autoload" in the "autoload" directories along
        // the runtimepath. There is no runtimepath here, so look next to the
        // importing script: a packaged plugin has plugin/x.vim beside
        // autoload/x.vim. Plain relative imports are script-relative.
        const QString dir = QFileInfo(m_scriptFileStack.last()).canonicalPath();
        QStringList candidates;
        if (autoload)
            candidates << dir + "/../autoload/" + resolved << dir + "/autoload/" + resolved;
        candidates << dir + '/' + resolved;
        canonicalPath.clear();
        for (const QString &candidate : std::as_const(candidates)) {
            canonicalPath = QFileInfo(candidate).canonicalFilePath();
            if (!canonicalPath.isEmpty())
                break;
        }
    }
    if (canonicalPath.isEmpty()) {
        showMessage(MessageError, Tr::tr("Cannot open file %1").arg(path));
        return true;
    }

    if (!g.moduleExports.contains(canonicalPath)) {
        ExCommand src;
        src.cmd = "source";
        src.args = canonicalPath;
        handleExSourceCommand(src);
    }

    QMap<QString, VimValue> exports = g.moduleExports.value(canonicalPath);
    setVariable(alias, VimValue::dict(exports));
    return true;
}

// Recursive-descent evaluator for a subset of Vimscript expressions. The
// grammar levels follow ":help expression-syntax" (ternary, ||, &&, compare,
// +/-/., */ /%, unary, atom). Only scalar values are handled for now; more
// atoms (variables, options, registers, function calls) are added in later
// steps.
class VimExpr
{
public:
    VimExpr(FakeVimHandler::Private *h, const QString &input)
        : m_h(h), m_in(input)
    {}

    bool vim9() const { return m_h->m_vim9; }

    VimValue parseExpr() { return exprTernary(); }

    void skipBlanks()
    {
        while (m_pos < m_in.size() && (m_in.at(m_pos) == ' ' || m_in.at(m_pos) == '\t'))
            ++m_pos;
    }

    bool atEnd() { skipBlanks(); return m_pos >= m_in.size(); }
    bool ok() const { return m_ok; }
    QString error() const { return m_error; }
    void setTrailingError() { setError(Tr::tr("Trailing characters: %1").arg(m_in.mid(m_pos))); }

private:
    QChar cur() const { return m_pos < m_in.size() ? m_in.at(m_pos) : QChar(); }
    QChar at(int o) const { return m_pos + o < m_in.size() ? m_in.at(m_pos + o) : QChar(); }
    void setError(const QString &msg) { if (m_ok) { m_ok = false; m_error = msg; } }

    bool eatOp(const char *op)
    {
        skipBlanks();
        const int n = int(qstrlen(op));
        for (int i = 0; i < n; ++i) {
            if (at(i) != QLatin1Char(op[i]))
                return false;
        }
        m_pos += n;
        return true;
    }

    VimValue exprTernary()
    {
        VimValue cond = exprOr();
        skipBlanks();
        if (m_ok && cur() == '?' && at(1) == '?') {
            // "a ?? b" is b only when a is falsy.
            m_pos += 2;
            const VimValue fallback = exprTernary();
            return cond.toBool() ? cond : fallback;
        }
        if (m_ok && cur() == '?') {
            ++m_pos;
            VimValue a = exprTernary();
            skipBlanks();
            if (!eatOp(":")) {
                setError(Tr::tr("Missing ':' in ternary expression"));
                return {};
            }
            VimValue b = exprTernary();
            return cond.toBool() ? a : b;
        }
        return cond;
    }

    VimValue exprOr()
    {
        VimValue e = exprAnd();
        while (m_ok && eatOp("||")) {
            // Once true the rest cannot change it, so it is read past rather
            // than worked out: "exists(x) || f(x)" must not call f().
            const bool decided = e.toBool();
            if (decided)
                ++m_skip;
            const VimValue r = exprAnd();
            if (decided)
                --m_skip;
            e = VimValue(qlonglong(decided || r.toBool()));
        }
        return e;
    }

    VimValue exprAnd()
    {
        VimValue e = exprCompare();
        while (m_ok && eatOp("&&")) {
            // Once false the rest cannot change it, which is what lets
            // "exists(x) && x" ask about something that may not be there.
            const bool decided = !e.toBool();
            if (decided)
                ++m_skip;
            const VimValue r = exprCompare();
            if (decided)
                --m_skip;
            e = VimValue(qlonglong(!decided && r.toBool()));
        }
        return e;
    }

    // A word at the current position with a trailing word boundary (used for
    // the "is"/"isnot" operators). Does not skip blanks or advance.
    bool peekWord(const char *w) const
    {
        const int n = int(qstrlen(w));
        for (int i = 0; i < n; ++i) {
            if (at(i) != QLatin1Char(w[i]))
                return false;
        }
        const QChar after = at(n);
        return !(after.isLetterOrNumber() || after == '_');
    }

    static bool identity(const VimValue &a, const VimValue &b)
    {
        if (a.isList() && b.isList())
            return a.listData() == b.listData();
        if (a.isDict() && b.isDict())
            return a.dictData() == b.dictData();
        if (a.isList() || b.isList() || a.isDict() || b.isDict())
            return false;
        return compare(a, "==", b, true); // scalars: identity is equality
    }

    VimValue exprCompare()
    {
        VimValue lhs = exprAdd();
        if (!m_ok)
            return lhs;
        skipBlanks();

        // "is" / "isnot" identity operators.
        if (peekWord("isnot") || peekWord("is")) {
            const bool negate = peekWord("isnot");
            m_pos += negate ? 5 : 2;
            const VimValue rhs = exprAdd();
            if (!m_ok)
                return {};
            return VimValue(qlonglong(identity(lhs, rhs) != negate ? 1 : 0));
        }

        const QChar c = cur();
        if (c != '=' && c != '!' && c != '<' && c != '>')
            return lhs;

        // "=~" / "!~" regular expression match.
        if (eatOp("=~") || eatOp("!~")) {
            const bool negate = m_in.at(m_pos - 1) == '~' && m_in.at(m_pos - 2) == '!';
            if (cur() == '#' || cur() == '?') // accept a case suffix
                ++m_pos;
            const VimValue rhs = exprAdd();
            if (!m_ok)
                return {};
            const bool matched =
                vimPatternToQtPattern(rhs.toString()).match(lhs.toString()).hasMatch();
            return VimValue(qlonglong(matched != negate ? 1 : 0));
        }

        QString op;
        if (eatOp("==")) op = "==";
        else if (eatOp("!=")) op = "!=";
        else if (eatOp("<=")) op = "<=";
        else if (eatOp(">=")) op = ">=";
        else if (eatOp("<")) op = "<";
        else if (eatOp(">")) op = ">";
        else return lhs;

        // Optional case-forcing suffix, only when glued (no leading blank).
        bool caseSensitive = true;
        if (cur() == '#')
            ++m_pos;
        else if (cur() == '?') {
            caseSensitive = false;
            ++m_pos;
        }

        VimValue rhs = exprAdd();
        if (!m_ok)
            return {};
        return VimValue(qlonglong(compare(lhs, op, rhs, caseSensitive)));
    }

    static bool compare(const VimValue &l, const QString &op, const VimValue &r, bool cs)
    {
        int c;
        if (l.isString() && r.isString()) {
            c = QString::compare(l.toString(), r.toString(),
                                 cs ? Qt::CaseSensitive : Qt::CaseInsensitive);
        } else if (l.type() == VimValue::Float || r.type() == VimValue::Float) {
            const double a = l.toFloat(), b = r.toFloat();
            c = a < b ? -1 : a > b ? 1 : 0;
        } else {
            const qlonglong a = l.toNumber(), b = r.toNumber();
            c = a < b ? -1 : a > b ? 1 : 0;
        }
        if (op == "==") return c == 0;
        if (op == "!=") return c != 0;
        if (op == "<")  return c < 0;
        if (op == "<=") return c <= 0;
        if (op == ">")  return c > 0;
        return c >= 0; // ">="
    }

    VimValue exprAdd()
    {
        VimValue e = exprMul();
        while (m_ok) {
            skipBlanks();
            const QChar c = cur();
            if (c == '+' || c == '-') {
                ++m_pos;
                VimValue r = exprMul();
                if (c == '+' && e.isList() && r.isList()) {
                    // Two lists added are the one after the other.
                    QList<VimValue> items = *e.listData();
                    items += *r.listData();
                    e = VimValue::list(items);
                } else if (e.type() == VimValue::Float || r.type() == VimValue::Float)
                    e = VimValue(c == '+' ? e.toFloat() + r.toFloat() : e.toFloat() - r.toFloat());
                else
                    e = VimValue(c == '+' ? e.toNumber() + r.toNumber() : e.toNumber() - r.toNumber());
            } else if (c == '.'
                       && (vim9() ? at(1) == '.'
                                  : (at(1) != QChar() && !at(1).isDigit()))) {
                // In Vim9 only ".." concatenates; a single "." is member access.
                ++m_pos;
                if (cur() == '.')
                    ++m_pos;
                VimValue r = exprMul();
                e = VimValue(e.toString() + r.toString());
            } else {
                break;
            }
        }
        return e;
    }

    VimValue exprMul()
    {
        VimValue e = exprUnary();
        while (m_ok) {
            skipBlanks();
            const QChar c = cur();
            if (c != '*' && c != '/' && c != '%')
                break;
            ++m_pos;
            VimValue r = exprUnary();
            if (c != '%' && (e.type() == VimValue::Float || r.type() == VimValue::Float)) {
                const double b = r.toFloat();
                e = VimValue(c == '*' ? e.toFloat() * b : (b != 0 ? e.toFloat() / b : 0.0));
            } else {
                const qlonglong b = r.toNumber();
                if (c == '*')
                    e = VimValue(e.toNumber() * b);
                else
                    e = VimValue(b != 0 ? (c == '/' ? e.toNumber() / b : e.toNumber() % b) : 0);
            }
        }
        return e;
    }

    VimValue exprUnary()
    {
        skipBlanks();
        const QChar c = cur();
        if (c == '!') {
            ++m_pos;
            return VimValue(qlonglong(!exprUnary().toBool()));
        }
        if (c == '-' || c == '+') {
            ++m_pos;
            VimValue v = exprUnary();
            if (c == '+')
                return v;
            return v.type() == VimValue::Float ? VimValue(-v.toFloat())
                                               : VimValue(-v.toNumber());
        }
        return exprPostfix();
    }

    VimValue exprPostfix()
    {
        VimValue v = exprAtom();
        while (m_ok) {
            if (cur() == '[') { // subscript v[i] or slice v[a:b]
                ++m_pos;
                skipBlanks();
                const bool haveStart = cur() != ':';
                const VimValue start = haveStart ? parseExpr() : VimValue();
                skipBlanks();
                if (cur() == ':') { // slice
                    ++m_pos;
                    skipBlanks();
                    const bool haveEnd = cur() != ']';
                    const VimValue end = haveEnd ? parseExpr() : VimValue();
                    skipBlanks();
                    if (!eatOp("]")) {
                        setError(Tr::tr("Missing ']' in slice"));
                        return {};
                    }
                    v = slice(v, haveStart, start, haveEnd, end);
                } else {
                    if (!eatOp("]")) {
                        setError(Tr::tr("Missing ']' in subscript"));
                        return {};
                    }
                    v = subscript(v, start);
                }
            } else if (cur() == '-' && at(1) == '>') { // method call v->f(...)
                m_pos += 2;
                v = parseMethodCall(v);
            } else if (cur() == '.' && at(1) != '.' && v.isDict()
                       && (at(1).isLetter() || at(1) == '_')) {
                // "d.key" dictionary access: only when v is a dictionary, so a
                // "." after a string/number stays the concatenation operator.
                ++m_pos;
                const int keyStart = m_pos;
                while (cur().isLetterOrNumber() || cur() == '_')
                    ++m_pos;
                v = subscript(v, VimValue(m_in.mid(keyStart, m_pos - keyStart)));
            } else if (cur() == '(' && v.isFunc()) {
                // Calling a funcref value obtained above, e.g. "list[0](x)" or
                // "ns.Func(x)" (module import namespace access).
                v = parseFuncrefCall(v);
            } else {
                break;
            }
        }
        return v;
    }

    VimValue parseFuncrefCall(const VimValue &callable)
    {
        ++m_pos; // '('
        QList<VimValue> args;
        skipBlanks();
        if (cur() != ')') {
            while (m_ok) {
                args.append(parseExpr());
                skipBlanks();
                if (cur() == ',') {
                    ++m_pos;
                    continue;
                }
                break;
            }
        }
        if (!m_ok)
            return {};
        if (!eatOp(")")) {
            setError(Tr::tr("Missing ')' in call"));
            return {};
        }
        VimValue result;
        QString error;
        if (!m_h->invokeCallable(callable, args, &result, &error)) {
            setError(error);
            return {};
        }
        return result;
    }

    // "v->f(args)" is sugar for "f(v, args)".
    VimValue parseMethodCall(const VimValue &piped)
    {
        skipBlanks();
        const QString name = parseName();
        if (!eatOp("(")) {
            setError(Tr::tr("Missing '(' in method call"));
            return {};
        }
        QList<VimValue> args;
        args.append(piped);
        skipBlanks();
        if (cur() != ')') {
            while (m_ok) {
                args.append(parseExpr());
                skipBlanks();
                if (cur() == ',') {
                    ++m_pos;
                    continue;
                }
                break;
            }
        }
        if (!m_ok)
            return {};
        if (!eatOp(")")) {
            setError(Tr::tr("Missing ')' in method call"));
            return {};
        }
        VimValue result;
        QString error;
        if (!m_h->callFunction(name, args, &result, &error)) {
            setError(error);
            return {};
        }
        return result;
    }

    // Vim slices are inclusive of both ends; a missing start is 0, a missing
    // end is the last element, and negative indices count from the end.
    VimValue slice(const VimValue &v, bool haveStart, const VimValue &startVal,
                   bool haveEnd, const VimValue &endVal)
    {
        const bool isList = v.isList();
        if (!isList && v.isDict()) {
            setError(Tr::tr("Cannot slice a dictionary"));
            return {};
        }
        const int n = isList ? v.listData()->size() : v.toString().size();
        int a = haveStart ? int(startVal.toNumber()) : 0;
        int b = haveEnd ? int(endVal.toNumber()) : n - 1;
        if (a < 0)
            a += n;
        if (b < 0)
            b += n;
        a = qMax(a, 0);
        b = qMin(b, n - 1);
        if (isList) {
            QList<VimValue> out;
            for (int i = a; i <= b; ++i)
                out.append(v.listData()->at(i));
            return VimValue::list(out);
        }
        return VimValue(a <= b ? v.toString().mid(a, b - a + 1) : QString());
    }

    VimValue subscript(const VimValue &v, const VimValue &index)
    {
        int i = int(index.toNumber());
        if (v.isList()) {
            QList<VimValue> *l = v.listData();
            const int n = l->size();
            if (i < 0)
                i += n;
            if (i < 0 || i >= n) {
                setError(Tr::tr("List index out of range: %1").arg(index.toNumber()));
                return {};
            }
            return l->at(i);
        }
        if (v.isString()) {
            const QString s = v.toString();
            if (i < 0)
                i += s.size();
            return (i >= 0 && i < s.size()) ? VimValue(QString(s.at(i))) : VimValue(QString());
        }
        if (v.isDict()) {
            const QString key = index.toString();
            QMap<QString, VimValue> *d = v.dictData();
            if (!d->contains(key)) {
                setError(Tr::tr("Key not present in dictionary: %1").arg(key));
                return {};
            }
            return d->value(key);
        }
        setError(Tr::tr("Can only index a list, dictionary or string"));
        return {};
    }

    VimValue exprAtom()
    {
        skipBlanks();
        const QChar c = cur();
        if (c == '(') {
            if (vim9() && looksLikeVim9Lambda())
                return parseVim9Lambda();
            ++m_pos;
            VimValue v = parseExpr();
            skipBlanks();
            if (!eatOp(")"))
                setError(Tr::tr("Missing ')' in expression"));
            return v;
        }
        if (c == '"')
            return parseDoubleQuoted();
        if (c == '\'')
            return parseSingleQuoted();
        if (c == '[')
            return parseListLiteral();
        if (c == '{')
            return parseBraceExpr();
        if (c == '$' && (at(1) == '\'' || at(1) == '"'))
            return parseInterpolatedString();
        if (c == '$')
            return parseEnvVar();
        if (c == '&')
            return parseOption();
        if (c == '@')
            return parseRegister();
        if (c.isDigit() || (c == '.' && at(1).isDigit()))
            return parseNumber();
        if (c.isLetter() || c == '_')
            return parseVariable();

        setError(Tr::tr("Invalid expression: %1").arg(m_in.mid(m_pos)));
        return {};
    }

    // A variable name, optionally with a scope prefix like "g:".
    QString parseName()
    {
        // "#" is part of a name: "a#b" names one thing, which is how a plugin
        // spells a function that is loaded when first needed.
        const int start = m_pos;
        while (cur().isLetterOrNumber() || cur() == '_' || cur() == '#')
            ++m_pos;
        QString name = m_in.mid(start, m_pos - start);
        static const QString scopes = "gbwtslav";
        if (name.size() == 1 && scopes.contains(name.at(0)) && cur() == ':') {
            ++m_pos;
            const int s2 = m_pos;
            while (cur().isLetterOrNumber() || cur() == '_')
                ++m_pos;
            name += ':' + m_in.mid(s2, m_pos - s2);
        }
        return name;
    }

    VimValue parseVariable()
    {
        const QString name = parseName();
        if (cur() == '(') // a function call: name immediately followed by "("
            return parseCall(name);
        if (vim9()) { // Vim9 boolean/null literals
            if (name == "true")
                return VimValue(qlonglong(1));
            if (name == "false" || name == "null")
                return VimValue(qlonglong(0));
        }
        VimValue v;
        if (m_h->variableValue(name, &v))
            return v;
        // In Vim9 the name of a function is a funcref, which is how one is
        // handed to something like search().
        if (vim9() && m_h->g.userFunctions.contains(name))
            return VimValue::func(name);
        if (m_skip)
            return VimValue(qlonglong(0)); // only being read past
        setError(Tr::tr("Undefined variable: %1").arg(name));
        return {};
    }

    VimValue parseCall(const QString &name)
    {
        ++m_pos; // '('
        QList<VimValue> args;
        skipBlanks();
        if (cur() != ')') {
            while (m_ok) {
                args.append(parseExpr());
                skipBlanks();
                if (cur() == ',') {
                    ++m_pos;
                    continue;
                }
                break;
            }
        }
        if (!m_ok)
            return {};
        if (!eatOp(")")) {
            setError(Tr::tr("Missing ')' in call to %1()").arg(name));
            return {};
        }
        if (m_skip)
            return VimValue(qlonglong(0)); // not called, only read past
        VimValue result;
        QString error;
        if (!m_h->callFunction(name, args, &result, &error)) {
            setError(error);
            return {};
        }
        return result;
    }

    VimValue parseNumber()
    {
        const int start = m_pos;
        if (cur() == '0' && (at(1) == 'x' || at(1) == 'X')) {
            m_pos += 2;
            while (isHex(cur()))
                ++m_pos;
            return VimValue(qlonglong(m_in.mid(start + 2, m_pos - start - 2)
                                          .toLongLong(nullptr, 16)));
        }
        if (cur() == '0' && (at(1) == 'b' || at(1) == 'B')) {
            m_pos += 2;
            while (cur() == '0' || cur() == '1')
                ++m_pos;
            return VimValue(qlonglong(m_in.mid(start + 2, m_pos - start - 2)
                                          .toLongLong(nullptr, 2)));
        }
        if (cur() == '0' && (at(1) == 'o' || at(1) == 'O')) {
            m_pos += 2;
            while (cur() >= '0' && cur() <= '7')
                ++m_pos;
            return VimValue(qlonglong(m_in.mid(start + 2, m_pos - start - 2)
                                          .toLongLong(nullptr, 8)));
        }
        while (cur().isDigit())
            ++m_pos;
        bool isFloat = false;
        if (cur() == '.' && at(1).isDigit()) {
            isFloat = true;
            ++m_pos;
            while (cur().isDigit())
                ++m_pos;
        }
        if (cur() == 'e' || cur() == 'E') {
            isFloat = true;
            ++m_pos;
            if (cur() == '+' || cur() == '-')
                ++m_pos;
            while (cur().isDigit())
                ++m_pos;
        }
        const QString num = m_in.mid(start, m_pos - start);
        return isFloat ? VimValue(num.toDouble()) : VimValue(qlonglong(num.toLongLong()));
    }

    VimValue parseDoubleQuoted()
    {
        ++m_pos; // opening quote
        QString s;
        while (m_pos < m_in.size() && cur() != '"') {
            QChar ch = cur();
            if (ch == '\\' && m_pos + 1 < m_in.size()) {
                ++m_pos;
                const QChar e = cur();
                if (e == 'n') s += '\n';
                else if (e == 't') s += '\t';
                else if (e == 'r') s += '\r';
                else if (e == 'e') s += QChar(27);
                else if (e == '\\') s += '\\';
                else if (e == '"') s += '"';
                else s += e;
                ++m_pos;
            } else {
                s += ch;
                ++m_pos;
            }
        }
        if (!eatOp("\""))
            setError(Tr::tr("Missing '\"' in string"));
        return VimValue(s);
    }

    VimValue parseSingleQuoted()
    {
        ++m_pos; // opening quote
        QString s;
        while (m_pos < m_in.size()) {
            if (cur() == '\'') {
                if (at(1) == '\'') { // '' is a literal single quote
                    s += '\'';
                    m_pos += 2;
                    continue;
                }
                ++m_pos;
                return VimValue(s);
            }
            s += cur();
            ++m_pos;
        }
        setError(Tr::tr("Missing \"'\" in string"));
        return VimValue(s);
    }

    VimValue parseEnvVar()
    {
        ++m_pos; // '$'
        const int start = m_pos;
        while (cur().isLetterOrNumber() || cur() == '_')
            ++m_pos;
        const QString name = m_in.mid(start, m_pos - start);
        return VimValue(qEnvironmentVariable(name.toLatin1().constData()));
    }

    // Interpolated string $'...{expr}...' / $"...{expr}..."; "{{"/"}}" are
    // literal braces, and $"..." also honors backslash escapes.
    VimValue parseInterpolatedString()
    {
        ++m_pos; // '$'
        const QChar quote = cur();
        const bool doubleQuoted = quote == '"';
        ++m_pos; // opening quote
        QString out;
        while (m_pos < m_in.size() && cur() != quote) {
            const QChar ch = cur();
            if (ch == '{') {
                if (at(1) == '{') { // literal "{"
                    out += '{';
                    m_pos += 2;
                    continue;
                }
                ++m_pos;
                const VimValue v = exprTernary();
                skipBlanks();
                if (!m_ok)
                    return {};
                if (cur() != '}') {
                    setError(Tr::tr("Missing '}' in interpolation"));
                    return {};
                }
                ++m_pos;
                out += v.toString();
                continue;
            }
            if (ch == '}' && at(1) == '}') { // literal "}"
                out += '}';
                m_pos += 2;
                continue;
            }
            if (doubleQuoted && ch == '\\' && m_pos + 1 < m_in.size()) {
                ++m_pos;
                const QChar e = cur();
                out += e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r'
                     : e == 'e' ? QChar(27) : e;
                ++m_pos;
                continue;
            }
            if (!doubleQuoted && ch == '\'' && at(1) == '\'') { // '' -> '
                out += '\'';
                m_pos += 2;
                continue;
            }
            out += ch;
            ++m_pos;
        }
        if (cur() != quote) {
            setError(Tr::tr("Missing quote in interpolated string"));
            return {};
        }
        ++m_pos; // closing quote
        return VimValue(out);
    }

    VimValue parseListLiteral()
    {
        ++m_pos; // '['
        QList<VimValue> items;
        skipBlanks();
        while (m_ok && cur() != ']') {
            items.append(parseExpr());
            skipBlanks();
            if (cur() != ',')
                break;
            ++m_pos; // ','
            skipBlanks(); // a trailing comma before ']' is allowed
        }
        if (!m_ok)
            return {};
        if (!eatOp("]")) {
            setError(Tr::tr("Missing ']' in list"));
            return {};
        }
        return VimValue::list(items);
    }

    // "{" starts either a lambda "{args -> expr}" or a dictionary. Look ahead
    // past an optional comma-separated identifier list for "->" to decide.
    VimValue parseBraceExpr()
    {
        int p = m_pos + 1;
        while (p < m_in.size()) {
            const QChar ch = m_in.at(p);
            if (ch == ' ' || ch == '\t' || ch == ',' || ch == '_'
                || ch.isLetterOrNumber()) {
                ++p;
            } else {
                break;
            }
        }
        if (p + 1 < m_in.size() && m_in.at(p) == '-' && m_in.at(p + 1) == '>')
            return parseLambda();
        return parseDictLiteral();
    }

    int findMatchingBrace(int from) const
    {
        int depth = 0;
        for (int p = from; p < m_in.size(); ++p) {
            const QChar ch = m_in.at(p);
            if (ch == '"' || ch == '\'') {
                const QChar quote = ch;
                for (++p; p < m_in.size() && m_in.at(p) != quote; ++p) {
                    if (quote == '"' && m_in.at(p) == '\\')
                        ++p;
                }
            } else if (ch == '{' || ch == '[' || ch == '(') {
                ++depth;
            } else if (ch == ')' || ch == ']') {
                --depth;
            } else if (ch == '}') {
                if (depth == 0)
                    return p;
                --depth;
            }
        }
        return -1;
    }

    // Position just past a balanced expression starting at "from", stopping at
    // a top-level ",", ")", "]" or "}".
    int scanExpressionEnd(int from) const
    {
        int depth = 0;
        for (int p = from; p < m_in.size(); ++p) {
            const QChar ch = m_in.at(p);
            if (ch == '"' || ch == '\'') {
                const QChar quote = ch;
                for (++p; p < m_in.size() && m_in.at(p) != quote; ++p) {
                    if (quote == '"' && m_in.at(p) == '\\')
                        ++p;
                }
            } else if (ch == '(' || ch == '[' || ch == '{') {
                ++depth;
            } else if (ch == ')' || ch == ']' || ch == '}') {
                if (depth == 0)
                    return p;
                --depth;
            } else if (ch == ',' && depth == 0) {
                return p;
            }
        }
        return m_in.size();
    }

    // Is the "(" at the cursor the start of a Vim9 lambda "(args) => expr"?
    bool looksLikeVim9Lambda() const
    {
        int depth = 0;
        int p = m_pos;
        for (; p < m_in.size(); ++p) {
            const QChar ch = m_in.at(p);
            if (ch == '"' || ch == '\'') {
                const QChar quote = ch;
                for (++p; p < m_in.size() && m_in.at(p) != quote; ++p) {
                    if (quote == '"' && m_in.at(p) == '\\')
                        ++p;
                }
            } else if (ch == '(') {
                ++depth;
            } else if (ch == ')') {
                if (--depth == 0) {
                    ++p;
                    break;
                }
            }
        }
        while (p < m_in.size() && (m_in.at(p) == ' ' || m_in.at(p) == '\t'))
            ++p;
        return p + 1 < m_in.size() && m_in.at(p) == '=' && m_in.at(p + 1) == '>';
    }

    VimValue parseVim9Lambda()
    {
        ++m_pos; // '('
        QStringList params;
        skipBlanks();
        while (m_ok && cur() != ')') {
            const int s = m_pos;
            while (cur().isLetterOrNumber() || cur() == '_')
                ++m_pos;
            params.append(m_in.mid(s, m_pos - s));
            skipBlanks();
            if (cur() == ':') { // skip a ": type" annotation
                ++m_pos;
                while (m_pos < m_in.size() && cur() != ',' && cur() != ')')
                    ++m_pos;
            }
            skipBlanks();
            if (cur() == ',') {
                ++m_pos;
                skipBlanks();
            }
        }
        eatOp(")");
        skipBlanks();
        if (!eatOp("=>")) {
            setError(Tr::tr("Missing '=>' in lambda"));
            return {};
        }
        skipBlanks();
        const int bodyStart = m_pos;
        const int bodyEnd = scanExpressionEnd(bodyStart);
        const QString body = m_in.mid(bodyStart, bodyEnd - bodyStart).trimmed();
        m_pos = bodyEnd;
        QHash<QString, VimValue> captured;
        if (!m_h->m_localScopes.isEmpty())
            captured = m_h->m_localScopes.last();
        return VimValue::lambda(params, body, captured);
    }

    VimValue parseLambda()
    {
        ++m_pos; // '{'
        QStringList params;
        skipBlanks();
        while (cur().isLetter() || cur() == '_') {
            const int s = m_pos;
            while (cur().isLetterOrNumber() || cur() == '_')
                ++m_pos;
            params.append(m_in.mid(s, m_pos - s));
            skipBlanks();
            if (cur() == ',') {
                ++m_pos;
                skipBlanks();
            }
        }
        if (!eatOp("->")) {
            setError(Tr::tr("Missing '->' in lambda"));
            return {};
        }
        skipBlanks();
        const int bodyStart = m_pos;
        const int bodyEnd = findMatchingBrace(bodyStart);
        if (bodyEnd < 0) {
            setError(Tr::tr("Missing '}' in lambda"));
            return {};
        }
        const QString body = m_in.mid(bodyStart, bodyEnd - bodyStart).trimmed();
        m_pos = bodyEnd + 1;
        // Capture the enclosing local scope for the closure.
        QHash<QString, VimValue> captured;
        if (!m_h->m_localScopes.isEmpty())
            captured = m_h->m_localScopes.last();
        return VimValue::lambda(params, body, captured);
    }

    VimValue parseDictLiteral()
    {
        ++m_pos; // '{'
        QMap<QString, VimValue> items;
        skipBlanks();
        while (m_ok && cur() != '}') {
            const VimValue key = parseExpr();
            skipBlanks();
            if (!eatOp(":")) {
                setError(Tr::tr("Missing ':' in dictionary"));
                return {};
            }
            const VimValue value = parseExpr();
            if (!m_ok)
                return {};
            items.insert(key.toString(), value);
            skipBlanks();
            if (cur() != ',')
                break;
            ++m_pos; // ','
            skipBlanks(); // a trailing comma before '}' is allowed
        }
        if (!m_ok)
            return {};
        if (!eatOp("}")) {
            setError(Tr::tr("Missing '}' in dictionary"));
            return {};
        }
        return VimValue::dict(items);
    }

    VimValue parseOption()
    {
        ++m_pos; // '&'
        if ((cur() == 'g' || cur() == 'l') && at(1) == ':') // &g:opt / &l:opt
            m_pos += 2;
        const int start = m_pos;
        while (cur().isLetter())
            ++m_pos;
        const QString name = m_in.mid(start, m_pos - start);
        VimValue v;
        if (m_h->optionValue(name, &v))
            return v;
        setError(Tr::tr("Unknown option: %1").arg(name));
        return {};
    }

    VimValue parseRegister()
    {
        ++m_pos; // '@'
        if (m_pos >= m_in.size()) {
            setError(Tr::tr("Missing register name"));
            return {};
        }
        const QChar reg = cur();
        ++m_pos;
        return VimValue(m_h->registerContents(reg.unicode()));
    }

    static bool isHex(QChar c)
    {
        return c.isDigit() || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    [[maybe_unused]] FakeVimHandler::Private *m_h; // used by later steps
    const QString m_in;
    int m_pos = 0;
    // Above zero while an expression is only being read past, because what it
    // would work out to can no longer change the answer.
    int m_skip = 0;
    bool m_ok = true;
    QString m_error;
};

bool FakeVimHandler::Private::evaluateExpression(const QString &expr,
    VimValue *result, QString *error)
{
    VimExpr e(this, expr);
    if (e.atEnd()) {
        if (error)
            *error = Tr::tr("Empty expression");
        return false;
    }
    const VimValue v = e.parseExpr();
    if (e.ok() && !e.atEnd())
        e.setTrailingError();
    if (!e.ok()) {
        if (error)
            *error = e.error();
        return false;
    }
    *result = v;
    return true;
}

// Pick the storage map for a variable and its key in that map, taking the
// active function scope into account. Inside a function, "l:x" and an
// unscoped "x" are function-local and "a:x" holds an argument; "g:x" and an
// unscoped name at the top level are global; other scopes (b:/w:/t:/s:/v:)
// live in the global map keyed with their prefix.
QHash<QString, VimValue> *FakeVimHandler::Private::variableStore(const QString &name,
    QString *key)
{
    const bool inFunction = !m_localScopes.isEmpty();
    if (name.startsWith("g:")) {
        *key = name.mid(2);
        return &g.variables;
    }
    if (name.startsWith("a:")) {
        *key = name;
        return inFunction ? &m_localScopes.last() : &g.variables;
    }
    if (name.startsWith("l:")) {
        *key = name.mid(2);
        return inFunction ? &m_localScopes.last() : &g.variables;
    }
    if (name.size() > 1 && name.at(1) == ':') {
        *key = name;
        // b:/w:/t: belong to a buffer, window or tab; s:/v: and anything else
        // to the session, so they have to survive the handler that set them.
        const QChar scope = name.at(0);
        const bool perBuffer = scope == 'b' || scope == 'w' || scope == 't';
        return perBuffer ? &m_variables : &g.variables;
    }
    *key = name;
    return inFunction ? &m_localScopes.last() : &g.variables;
}

bool FakeVimHandler::Private::variableValue(const QString &name, VimValue *result)
{
    if (name == "v:true") { *result = VimValue(qlonglong(1)); return true; }
    if (name == "v:false" || name == "v:null" || name == "v:none") {
        *result = VimValue(qlonglong(0));
        return true;
    }
    if (name == "v:version") { *result = VimValue(qlonglong(900)); return true; }

    // A bare scope name like "g:" is a dictionary of that scope, as used by
    // "get(g:, 'name', default)". This is a snapshot: assigning through it
    // does not reach the variables themselves.
    if (name.size() == 2 && name.at(1) == ':' && QString("gbwtslav").contains(name.at(0))) {
        QMap<QString, VimValue> items;
        if (name == "l:" || name == "a:") {
            if (!m_localScopes.isEmpty()) {
                const QHash<QString, VimValue> &frame = m_localScopes.last();
                const bool wantArgs = name == "a:";
                for (auto it = frame.cbegin(); it != frame.cend(); ++it) {
                    if (it.key().startsWith("a:") == wantArgs)
                        items.insert(wantArgs ? it.key().mid(2) : it.key(), it.value());
                }
            }
        } else {
            const bool global = name == "g:";
            // Scan whichever map variableStore() would have used for the scope.
            QString ignored;
            const QHash<QString, VimValue> *store = variableStore(name, &ignored);
            for (auto it = store->cbegin(); it != store->cend(); ++it) {
                const QString &k = it.key();
                const bool scoped = k.size() > 1 && k.at(1) == ':';
                if (global && !scoped)
                    items.insert(k, it.value());
                else if (!global && scoped && k.startsWith(name))
                    items.insert(k.mid(2), it.value());
            }
        }
        *result = VimValue::dict(items);
        return true;
    }

    QString key;
    QHash<QString, VimValue> *store = variableStore(name, &key);
    const auto it = store->constFind(key);
    if (it == store->constEnd())
        return false;
    *result = *it;
    return true;
}

void FakeVimHandler::Private::setVariable(const QString &name, const VimValue &value)
{
    QString key;
    variableStore(name, &key)->insert(key, value);
}

bool FakeVimHandler::Private::unsetVariable(const QString &name)
{
    QString key;
    return variableStore(name, &key)->remove(key) > 0;
}

// Apply a compound assignment operator (the char before '=' in += etc.).
static VimValue applyCompound(const VimValue &a, QChar op, const VimValue &b)
{
    if (op == '.')
        return VimValue(a.toString() + b.toString());
    // "+" on lists joins them, which is how a script grows one item by item.
    if (op == '+' && a.isList() && b.isList()) {
        QList<VimValue> items = *a.listData();
        items += *b.listData();
        return VimValue::list(items);
    }
    if (a.type() == VimValue::Float || b.type() == VimValue::Float) {
        const double x = a.toFloat(), y = b.toFloat();
        return VimValue(op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y
                        : y != 0 ? x / y : 0.0);
    }
    const qlonglong x = a.toNumber(), y = b.toNumber();
    return VimValue(op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y
                    : op == '%' ? (y != 0 ? x % y : 0) : (y != 0 ? x / y : 0));
}

// Option name from a ":let &[g:|l:]opt" left-hand side (drops "&" and scope).
static QString optionNameFromLet(const QString &lhs)
{
    QString name = lhs.mid(1);
    if (name.startsWith("g:") || name.startsWith("l:"))
        name = name.mid(2);
    return name;
}

bool FakeVimHandler::Private::handleExLetCommand(const ExCommand &cmd)
{
    // :let {lhs} = {expr} with compound forms (+= -= *= /= %= .=). {lhs} is a
    // variable, an option (&opt), a register (@r) or an environment variable.
    if (cmd.cmd != "let")
        return false;

    // :let [a, b, ...] = listexpr - unpack a list into several variables.
    if (cmd.args.trimmed().startsWith('[')) {
        static const QRegularExpression unpackRe("^\\s*\\[([^\\]]*)\\]\\s*=\\s*(.*)$");
        const QRegularExpressionMatch um = unpackRe.match(cmd.args);
        VimValue list;
        QString error;
        if (!um.hasMatch()) {
            showMessage(MessageError, Tr::tr("Invalid :let expression: %1").arg(cmd.args));
        } else if (!evaluateExpression(um.captured(2), &list, &error)) {
            showMessage(MessageError, error);
        } else if (!list.isList()) {
            showMessage(MessageError, Tr::tr(":let with [...] requires a list"));
        } else {
            const QStringList names =
                um.captured(1).split(QRegularExpression("\\s*,\\s*"), Qt::SkipEmptyParts);
            const QList<VimValue> &items = *list.listData();
            for (int i = 0; i < names.size(); ++i)
                setVariable(names.at(i), i < items.size() ? items.at(i) : VimValue());
        }
        return true;
    }

    static const QRegularExpression re(
        "^\\s*([@&$]?[A-Za-z_][A-Za-z0-9_:]*)\\s*([-+*/%.]?=)\\s*(.*)$");
    const QRegularExpressionMatch m = re.match(cmd.args);
    if (!m.hasMatch()) {
        if (!letAssignIndexed(cmd.args))
            showMessage(MessageError, Tr::tr("Invalid :let expression: %1").arg(cmd.args));
        return true;
    }

    const QString name = m.captured(1);
    const QString op = m.captured(2);

    VimValue value;
    QString error;
    if (!evaluateExpression(m.captured(3), &value, &error)) {
        showMessage(MessageError, error);
        return true;
    }

    const QChar kind = name.at(0);
    if (op != "=") {
        VimValue old;
        bool haveOld = false;
        if (kind == '&')
            haveOld = optionValue(optionNameFromLet(name), &old);
        else if (kind == '@')
            old = VimValue(registerContents(name.at(1).unicode())), haveOld = true;
        else if (kind == '$')
            old = VimValue(qEnvironmentVariable(name.mid(1).toLatin1())), haveOld = true;
        else
            haveOld = variableValue(name, &old);
        if (!haveOld) {
            showMessage(MessageError, Tr::tr("Undefined variable: %1").arg(name));
            return true;
        }
        value = applyCompound(old, op.at(0), value);
    }

    if (kind == '&') {
        if (!setOption(optionNameFromLet(name), value))
            showMessage(MessageError, Tr::tr("Unknown option: %1").arg(optionNameFromLet(name)));
    } else if (kind == '@') {
        setRegister(name.at(1).unicode(), value.toString(), RangeCharMode);
    } else if (kind == '$') {
        qputenv(name.mid(1).toLatin1().constData(), value.toString().toLocal8Bit());
    } else {
        setVariable(name, value);
    }
    return true;
}

bool FakeVimHandler::Private::letAssignIndexed(const QString &args)
{
    // :let {var}[index]... = {expr} or :let {var}.key = {expr}, assigning into
    // a list or dictionary.
    static const QRegularExpression re(
        "^\\s*([A-Za-z_][A-Za-z0-9_:]*"
        "(?:\\[[^\\]]*\\]|\\.[A-Za-z_][A-Za-z0-9_]*)+)\\s*([-+*/%.]?=)\\s*(.*)$");
    const QRegularExpressionMatch m = re.match(args);
    if (!m.hasMatch())
        return false;

    const QString lhs = m.captured(1);
    const QString op = m.captured(2);

    VimValue value;
    QString error;
    if (!evaluateExpression(m.captured(3), &value, &error)) {
        showMessage(MessageError, error);
        return true;
    }

    // Find the last top-level subscript/key segment; the part before it is the
    // (shared) container to assign into.
    int segStart = -1;
    bool bracket = false;
    int depth = 0;
    for (int i = 0; i < lhs.size(); ++i) {
        const QChar ch = lhs.at(i);
        if (ch == '[') {
            if (depth == 0) {
                segStart = i;
                bracket = true;
            }
            ++depth;
        } else if (ch == ']') {
            --depth;
        } else if (ch == '.' && depth == 0) {
            segStart = i;
            bracket = false;
        }
    }

    VimValue container, index;
    if (!evaluateExpression(lhs.left(segStart), &container, &error)) {
        showMessage(MessageError, error);
        return true;
    }
    if (bracket) {
        if (!evaluateExpression(lhs.mid(segStart + 1, lhs.size() - segStart - 2),
                                &index, &error)) {
            showMessage(MessageError, error);
            return true;
        }
    } else {
        index = VimValue(lhs.mid(segStart + 1)); // ".key" -> string key
    }

    if (container.isList()) {
        QList<VimValue> *l = container.listData();
        int i = int(index.toNumber());
        if (i < 0)
            i += l->size();
        if (i < 0 || i >= l->size()) {
            showMessage(MessageError, Tr::tr("List index out of range: %1").arg(index.toNumber()));
            return true;
        }
        (*l)[i] = op == "=" ? value : applyCompound(l->at(i), op.at(0), value);
    } else if (container.isDict()) {
        QMap<QString, VimValue> *d = container.dictData();
        const QString key = index.toString();
        d->insert(key, op == "=" ? value : applyCompound(d->value(key), op.at(0), value));
    } else {
        showMessage(MessageError, Tr::tr("Can only index a list or dictionary"));
    }
    return true;
}

// 'commentstring' by file type. Looked up with the type set by ":setf" first
// and with the file name extension after that, so both "python" and "py" find
// the same entry. The part before "%s" is the comment leader, the part behind
// it the trailer that block styles need.
static QString commentStringForToken(const QString &token)
{
    static const QHash<QString, QString> table = {
        // C family and the languages that borrowed its line comment
        {"c", "// %s"}, {"cpp", "// %s"}, {"cxx", "// %s"}, {"cc", "// %s"},
        {"h", "// %s"}, {"hpp", "// %s"}, {"hxx", "// %s"},
        {"m", "// %s"}, {"mm", "// %s"}, {"objc", "// %s"},
        {"java", "// %s"}, {"cs", "// %s"}, {"go", "// %s"}, {"swift", "// %s"},
        {"rs", "// %s"}, {"rust", "// %s"}, {"kt", "// %s"}, {"kotlin", "// %s"},
        {"js", "// %s"}, {"javascript", "// %s"}, {"ts", "// %s"},
        {"typescript", "// %s"}, {"json", "// %s"}, {"qml", "// %s"},
        {"qbs", "// %s"}, {"php", "// %s"}, {"proto", "// %s"},
        {"glsl", "// %s"}, {"vert", "// %s"}, {"frag", "// %s"},
        // "#" comments
        {"py", "# %s"}, {"python", "# %s"}, {"sh", "# %s"}, {"bash", "# %s"},
        {"zsh", "# %s"}, {"pl", "# %s"}, {"perl", "# %s"}, {"rb", "# %s"},
        {"ruby", "# %s"}, {"pri", "# %s"}, {"pro", "# %s"}, {"prf", "# %s"},
        {"cmake", "# %s"}, {"make", "# %s"}, {"makefile", "# %s"},
        {"yaml", "# %s"}, {"yml", "# %s"}, {"toml", "# %s"}, {"conf", "# %s"},
        {"ini", "# %s"}, {"gitignore", "# %s"}, {"desktop", "# %s"},
        // the rest
        {"vim", "\" %s"}, {"lua", "-- %s"}, {"sql", "-- %s"}, {"hs", "-- %s"},
        {"haskell", "-- %s"}, {"ada", "-- %s"}, {"lisp", "; %s"},
        {"el", "; %s"}, {"scm", "; %s"}, {"asm", "; %s"}, {"s", "; %s"},
        {"bat", "REM %s"}, {"cmd", "REM %s"}, {"tex", "% %s"},
        {"f90", "! %s"}, {"fortran", "! %s"},
        {"css", "/* %s */"}, {"scss", "/* %s */"},
        {"html", "<!-- %s -->"}, {"htm", "<!-- %s -->"},
        {"md", "<!-- %s -->"}, {"markdown", "<!-- %s -->"},
        {"xml", "<!-- %s -->"}, {"ui", "<!-- %s -->"},
        {"qrc", "<!-- %s -->"}, {"svg", "<!-- %s -->"}
    };
    return table.value(token);
}

// The effective 'commentstring' of this buffer: what ":set" put there wins,
// then the file type, then the configured fallback.
QString FakeVimHandler::Private::commentString() const
{
    if (!m_commentString.isEmpty())
        return m_commentString;
    QString cms = commentStringForToken(m_fileType.toLower());
    if (cms.isEmpty())
        cms = commentStringForToken(QFileInfo(m_currentFileName).suffix().toLower());
    return cms.isEmpty() ? s.commentString() : cms;
}

bool FakeVimHandler::Private::optionValue(const QString &name, VimValue *result)
{
    if (isCommentStringOption(name)) {
        *result = VimValue(commentString());
        return true;
    }
    if (isFileTypeOption(name)) {
        *result = VimValue(m_fileType);
        return true;
    }
    FvBaseAspect *act = s.item(Utils::keyFromString(name));
    if (!act)
        return false;
    const QVariant v = act->variantValue();
    if (v.typeId() == QMetaType::Bool)
        *result = VimValue(qlonglong(v.toBool() ? 1 : 0));
    else if (v.typeId() == QMetaType::Int || v.typeId() == QMetaType::LongLong)
        *result = VimValue(qlonglong(v.toLongLong()));
    else
        *result = VimValue(v.toString());
    return true;
}

bool FakeVimHandler::Private::setOption(const QString &name, const VimValue &value)
{
    if (isCommentStringOption(name)) {
        m_commentString = value.toString();
        return true;
    }
    if (isFileTypeOption(name)) {
        setFileType(value.toString());
        return true;
    }
    FvBaseAspect *act = s.item(Utils::keyFromString(name));
    if (!act)
        return false;
    const QVariant v = act->variantValue();
    if (v.typeId() == QMetaType::Bool)
        act->setVariantValue(value.toBool());
    else if (v.typeId() == QMetaType::Int || v.typeId() == QMetaType::LongLong)
        act->setVariantValue(value.toNumber());
    else
        act->setVariantValue(value.toString());
    updateEditor();
    updateHighlights();
    return true;
}

bool FakeVimHandler::Private::handleExCallCommand(const ExCommand &cmd)
{
    // :call {func}({args}) - evaluate a function call for its side effects.
    if (!cmd.matches("cal", "call"))
        return false;
    VimValue result;
    QString error;
    if (!evaluateExpression(cmd.args, &result, &error))
        showMessage(MessageError, error);
    return true;
}

// A subset of Vim's printf(): the conversions d i x X o c f s and %%, with the
// "-" and "0" flags, a field width and a precision (for f and s).
static QString vimPrintf(const QList<VimValue> &args)
{
    const QString fmt = args.isEmpty() ? QString() : args.at(0).toString();
    QString out;
    int ai = 1;
    for (int i = 0; i < fmt.size(); ++i) {
        if (fmt.at(i) != '%') {
            out += fmt.at(i);
            continue;
        }
        int j = i + 1;
        bool left = false;
        bool zero = false;
        for (; j < fmt.size() && QString("-+ 0#").contains(fmt.at(j)); ++j) {
            left = left || fmt.at(j) == '-';
            zero = zero || fmt.at(j) == '0';
        }
        int width = 0;
        for (; j < fmt.size() && fmt.at(j).isDigit(); ++j)
            width = width * 10 + fmt.at(j).digitValue();
        int prec = -1;
        if (j < fmt.size() && fmt.at(j) == '.') {
            prec = 0;
            for (++j; j < fmt.size() && fmt.at(j).isDigit(); ++j)
                prec = prec * 10 + fmt.at(j).digitValue();
        }
        if (j >= fmt.size())
            break;
        const QChar conv = fmt.at(j);
        i = j;
        if (conv == '%') {
            out += '%';
            continue;
        }
        const VimValue a = ai < args.size() ? args.at(ai++) : VimValue();
        QString piece;
        switch (conv.unicode()) {
        case 'd': case 'i': piece = QString::number(a.toNumber()); break;
        case 'x': piece = QString::number(a.toNumber(), 16); break;
        case 'X': piece = QString::number(a.toNumber(), 16).toUpper(); break;
        case 'o': piece = QString::number(a.toNumber(), 8); break;
        case 'c': piece = QString(QChar(uint(a.toNumber()))); break;
        case 'f': piece = QString::number(a.toFloat(), 'f', prec < 0 ? 6 : prec); break;
        case 's': piece = prec >= 0 ? a.toString().left(prec) : a.toString(); break;
        default: piece = QString(conv); break;
        }
        if (piece.size() < width) {
            const int fill = width - piece.size();
            if (left)
                piece += QString(fill, ' ');
            else
                piece.prepend(QString(fill, zero ? '0' : ' '));
        }
        out += piece;
    }
    return out;
}

// Resolve a line()/col()/getpos() position argument: "." is the cursor, "$"
// the last line and "'m" a mark. Both fields are 0-based; the callers add 1.
CursorPosition FakeVimHandler::Private::lineColArg(const QString &spec) const
{
    if (spec == "." || spec == "v")
        return CursorPosition(m_cursor);
    if (spec == "$")
        return CursorPosition(linesInDocument() - 1, 0);
    if (spec.size() == 2 && spec.at(0) == '\'') {
        const Mark m = mark(spec.at(1));
        if (m.isValid())
            return m.position(document());
    }
    return CursorPosition(-1, -1); // unknown: line()/col() report 0
}

// The ":p", ":h", ":t", ":r" and ":e" modifiers a path may carry, applied left
// to right as Vim applies them. Shared by fnamemodify() and expand().
QString FakeVimHandler::Private::applyFileNameModifiers(const QString &fileName,
                                                        const QString &mods)
{
    QString path = fileName;
    int i = 0;
    while (i < mods.size()) {
        if (mods.at(i) != ':')
            break;
        ++i;
        if (i >= mods.size())
            break;
        const QChar what = mods.at(i++);
        if (what == 'p') { // full path
            path = QFileInfo(path).absoluteFilePath();
        } else if (what == 'h') { // head, the directory
            const int slash = path.lastIndexOf('/');
            path = slash > 0 ? path.left(slash) : (slash == 0 ? "/" : QString("."));
        } else if (what == 't') { // tail, the file name
            path = path.mid(path.lastIndexOf('/') + 1);
        } else if (what == 'r') { // root, without the extension
            const int dot = path.lastIndexOf('.');
            if (dot > path.lastIndexOf('/') + 1)
                path = path.left(dot);
        } else if (what == 'e') { // extension only
            const int dot = path.lastIndexOf('.');
            path = dot > path.lastIndexOf('/') + 1 ? path.mid(dot + 1) : QString();
        }
    }
    return path;
}

// A copy that shares nothing with the original, however deeply nested.
VimValue FakeVimHandler::Private::deepCopy(const VimValue &value)
{
    if (value.isList()) {
        QList<VimValue> items;
        for (const VimValue &item : *value.listData())
            items.append(deepCopy(item));
        return VimValue::list(items);
    }
    if (value.isDict()) {
        QMap<QString, VimValue> items;
        const QMap<QString, VimValue> *from = value.dictData();
        for (auto it = from->cbegin(); it != from->cend(); ++it)
            items.insert(it.key(), deepCopy(it.value()));
        return VimValue::dict(items);
    }
    return value; // the rest carries no reference to share
}

// Source the script a "#" name lives in. "a#b" is in "autoload/a.vim" and
// "a#b#c" in "autoload/a/b.vim", looked for along the runtimepath. Returns
// whether a file was read, and looks only once per script.
bool FakeVimHandler::Private::loadAutoloadScript(const QString &functionName)
{
    const int lastHash = functionName.lastIndexOf('#');
    if (lastHash <= 0)
        return false;
    const QString relative = functionName.left(lastHash).replace('#', '/');
    if (g.autoloadTried.contains(relative))
        return false;
    g.autoloadTried.insert(relative);

    const QStringList dirs = s.runtimePath().split(',', Qt::SkipEmptyParts);
    for (const QString &dir : dirs) {
        const QString path = replaceTildeWithHome(dir) + "/autoload/" + relative + ".vim";
        if (QFileInfo::exists(path)) {
            ExCommand source;
            source.cmd = "source";
            source.args = path;
            handleExSourceCommand(source);
            return true;
        }
    }
    return false;
}

// What bufnr() reports for this buffer. Numbers are handed out as they are
// asked for, which is enough for a script to tell one buffer from another.
int FakeVimHandler::Private::bufferNumber()
{
    if (m_bufferNumber == 0)
        m_bufferNumber = ++g.lastBufferNumber;
    return m_bufferNumber;
}

// expand(): the handful of "<...>" keywords and "%" that scripts rely on.
QString FakeVimHandler::Private::expandKeyword(const QString &what) const
{
    // What is being asked about comes first, then any ":" modifiers, so
    // "<afile>:p:h" is the directory of the file the event was for.
    QString base = what;
    QString mods;
    const int modAt = what.startsWith('<') ? what.indexOf(':', what.indexOf('>'))
                                           : what.indexOf(':');
    if (modAt > 0) {
        base = what.left(modAt);
        mods = what.mid(modAt);
    }

    QString value;
    if (base == "<stack>") {
        // Vim reports the source chain, e.g.
        // "command line..function comment#Toggle[2]". Plugins pick the
        // innermost function name out of it with a pattern anchored on the
        // "[", most notably to set 'operatorfunc' to their own function, so
        // the name here has to be one callFunction() can resolve again.
        value = m_scriptFileStack.isEmpty()
            ? QStringLiteral("command line") : m_scriptFileStack.last();
        for (const QString &fn : m_callStack)
            value += "..function " + fn + "[0]";
        return value; // not a path, so no modifiers
    }
    if (base == "<sfile>" || base == "<script>") {
        value = m_scriptFileStack.isEmpty() ? QString() : m_scriptFileStack.last();
    } else if (base == "<abuf>") {
        // The buffer an event was for, which is this one.
        return QString::number(const_cast<Private *>(this)->bufferNumber());
    } else if (base == "%" || base == "<afile>") {
        value = m_currentFileName;
    } else if (base == "<cword>" || base == "<cWORD>") {
        QTextCursor tc = m_cursor;
        tc.select(base == "<cword>" ? QTextCursor::WordUnderCursor
                                    : QTextCursor::BlockUnderCursor);
        if (base == "<cword>")
            return tc.selectedText();
        // <cWORD> is delimited by whitespace only.
        const QString line = tc.selectedText();
        const int col = m_cursor.positionInBlock();
        int from = col;
        while (from > 0 && !line.at(from - 1).isSpace())
            --from;
        int to = col;
        while (to < line.size() && !line.at(to).isSpace())
            ++to;
        return line.mid(from, to - from);
    } else {
        return QString();
    }
    return mods.isEmpty() ? value : applyFileNameModifiers(value, mods);
}

// search({pattern} [, {flags} [, {stopline} [, {timeout} [, {skip}]]]])
// Returns the line the match starts on, or 0 when there is none. {skip} is
// evaluated with the cursor on each candidate and rejects it when true, which
// is how a caller ignores matches in, say, a comment.
bool FakeVimHandler::Private::searchFunction(const QList<VimValue> &args,
    VimValue *result, QString *error)
{
    const auto arg = [&](int i) { return i < args.size() ? args.at(i) : VimValue(); };
    const QString flags = arg(1).toString();
    const bool backward = flags.contains('b');
    const bool acceptAtCursor = flags.contains('c');
    const bool moveToEnd = flags.contains('e');
    const bool keepCursor = flags.contains('n');
    // 'W' forbids wrapping, 'w' demands it, otherwise 'wrapscan' decides.
    const bool wrap = flags.contains('w') || (!flags.contains('W') && s.wrapScan());
    const int stopLine = args.size() > 2 ? int(arg(2).toNumber()) : 0;
    const bool haveSkip = args.size() > 4;
    const VimValue skip = arg(4);

    QRegularExpression re = vimPatternToQtPattern(arg(0).toString());
    if (!re.isValid()) {
        *error = Tr::tr("Invalid pattern: %1").arg(arg(0).toString());
        return false;
    }
    // The whole buffer is matched at once, so "^" and "$" have to mean the ends
    // of a line rather than the ends of the text, as they do for a Vim pattern.
    re.setPatternOptions(re.patternOptions() | QRegularExpression::MultilineOption);

    const QString text = document()->toPlainText();
    const int start = position();

    // Collect the match offsets in the order they are to be visited.
    QList<int> offsets;
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        const int offset = it.next().capturedStart();
        if (backward ? (offset < start || (acceptAtCursor && offset == start))
                     : (offset > start || (acceptAtCursor && offset == start))) {
            offsets.append(offset);
        }
    }
    if (backward)
        std::reverse(offsets.begin(), offsets.end());
    if (wrap) {
        // A wrapped search continues from the far end, so the matches on the
        // other side of the cursor come after the ones ahead of it.
        QList<int> wrapped;
        it = re.globalMatch(text);
        while (it.hasNext()) {
            const int offset = it.next().capturedStart();
            if (backward ? offset > start : offset < start)
                wrapped.append(offset);
        }
        if (backward)
            std::reverse(wrapped.begin(), wrapped.end());
        offsets += wrapped;
    }

    const CursorPosition saved(m_cursor);
    for (const int offset : std::as_const(offsets)) {
        const QTextBlock block = document()->findBlock(offset);
        const int line = block.blockNumber() + 1;
        if (stopLine > 0 && (backward ? line < stopLine : line > stopLine))
            break;

        // Vim evaluates {skip} with the cursor on the candidate.
        setCursorPosition(CursorPosition(block.blockNumber(), offset - block.position()));
        if (haveSkip) {
            VimValue keep;
            bool ok = true;
            if (skip.isFunc())
                ok = invokeCallable(skip, {}, &keep, error);
            else
                ok = evaluateExpression(skip.toString(), &keep, error);
            if (!ok) {
                setCursorPosition(saved);
                return false;
            }
            if (keep.toBool())
                continue;
        }
        if (keepCursor) {
            setCursorPosition(saved);
        } else {
            if (moveToEnd) {
                const QRegularExpressionMatch m = re.match(text, offset);
                if (m.hasMatch() && m.capturedLength() > 0) {
                    const int end = offset + m.capturedLength() - 1;
                    const QTextBlock endBlock = document()->findBlock(end);
                    setCursorPosition(CursorPosition(endBlock.blockNumber(),
                                                     end - endBlock.position()));
                }
            }
            setTargetColumn();
        }
        *result = VimValue(qlonglong(line));
        return true;
    }

    setCursorPosition(saved);
    *result = VimValue(qlonglong(0));
    return true;
}

// The builtins callFunction() below answers to, for exists("*name"). Keep in
// step with it when adding one; a name missing here only means exists() denies
// a function that does work.
static bool isBuiltinFunction(const QString &name)
{
    static const QSet<QString> builtins = {
        "abs", "add", "bufnr", "call", "char2nr", "col", "copy", "count",
        "cursor", "deepcopy", "did_filetype", "empty", "escape", "executable",
        "exists", "expand", "extend", "filereadable", "filter", "fnameescape",
        "fnamemodify", "funcref", "function", "get", "getbufvar", "getcurpos",
        "getcwd", "getline", "getpos", "has", "has_key", "iconv", "indent",
        "index", "insert", "isdirectory", "items", "join", "keys", "len",
        "line", "map", "match", "matchlist", "matchstr", "max", "min",
        "nr2char", "printf", "range", "readfile", "remove", "repeat", "reverse",
        "search", "setbufvar", "setline", "setpos", "shellescape", "sort",
        "split", "str2nr", "strftime", "stridx", "string", "strlen", "strpart",
        "submatch", "substitute", "synID", "synIDattr", "synstack", "system",
        "tolower", "toupper", "trim", "type", "values", "winrestview",
        "winsaveview", "writefile"
    };
    return builtins.contains(name);
}

bool FakeVimHandler::Private::callFunction(const QString &name,
    const QList<VimValue> &args, VimValue *result, QString *error)
{
    const auto arg = [&](int i) { return i < args.size() ? args.at(i) : VimValue(); };

    if (name == "strlen") {
        *result = VimValue(qlonglong(arg(0).toString().size()));
    } else if (name == "len") {
        const VimValue v = arg(0);
        *result = VimValue(qlonglong(v.isList() ? v.listData()->size()
                                     : v.isDict() ? v.dictData()->size()
                                     : v.toString().size()));
    } else if (name == "add") {
        if (!arg(0).isList()) {
            *error = Tr::tr("add() requires a list");
            return false;
        }
        arg(0).listData()->append(arg(1));
        *result = arg(0);
    } else if (name == "get") {
        if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            int i = int(arg(1).toNumber());
            if (i < 0)
                i += l->size();
            *result = (i >= 0 && i < l->size()) ? l->at(i) : arg(2);
        } else if (arg(0).isDict()) {
            QMap<QString, VimValue> *d = arg(0).dictData();
            const QString key = arg(1).toString();
            *result = d->contains(key) ? d->value(key) : arg(2);
        } else {
            *result = arg(2);
        }
    } else if (name == "has_key") {
        *result = VimValue(qlonglong(arg(0).isDict()
                           && arg(0).dictData()->contains(arg(1).toString()) ? 1 : 0));
    } else if (name == "keys") {
        QList<VimValue> items;
        if (arg(0).isDict()) {
            const QList<QString> keys = arg(0).dictData()->keys();
            for (const QString &k : keys)
                items.append(VimValue(k));
        }
        *result = VimValue::list(items);
    } else if (name == "values") {
        QList<VimValue> items;
        if (arg(0).isDict())
            items = arg(0).dictData()->values();
        *result = VimValue::list(items);
    } else if (name == "items") {
        QList<VimValue> items;
        if (arg(0).isDict()) {
            QMap<QString, VimValue> *d = arg(0).dictData();
            for (auto it = d->cbegin(), end = d->cend(); it != end; ++it)
                items.append(VimValue::list({VimValue(it.key()), it.value()}));
        }
        *result = VimValue::list(items);
    } else if (name == "remove") {
        if (arg(0).isDict()) {
            QMap<QString, VimValue> *d = arg(0).dictData();
            const QString key = arg(1).toString();
            *result = d->contains(key) ? d->take(key) : VimValue();
        } else if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            int i = int(arg(1).toNumber());
            if (i < 0)
                i += l->size();
            *result = (i >= 0 && i < l->size()) ? l->takeAt(i) : VimValue();
        } else {
            *result = VimValue();
        }
    } else if (name == "range") {
        const int a = int(arg(0).toNumber());
        const bool haveMax = args.size() > 1;
        const int lo = haveMax ? a : 0;
        const int hi = haveMax ? int(arg(1).toNumber()) : a - 1;
        const int step = args.size() > 2 ? int(arg(2).toNumber()) : 1;
        QList<VimValue> items;
        if (step > 0)
            for (int v = lo; v <= hi; v += step)
                items.append(VimValue(qlonglong(v)));
        else if (step < 0)
            for (int v = lo; v >= hi; v += step)
                items.append(VimValue(qlonglong(v)));
        *result = VimValue::list(items);
    } else if (name == "sort") {
        if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            if (args.size() > 1 && arg(1).isFunc()) {
                // A Funcref/lambda comparator: f(a, b) < 0 means a sorts first.
                const VimValue cmp = arg(1);
                std::stable_sort(l->begin(), l->end(),
                                 [&](const VimValue &a, const VimValue &b) {
                    VimValue r;
                    QString e;
                    invokeCallable(cmp, {a, b}, &r, &e);
                    return r.toNumber() < 0;
                });
            } else {
                const QString how = args.size() > 1 ? arg(1).toString() : QString();
                if (how == "n" || how == "N") {
                    std::stable_sort(l->begin(), l->end(),
                                     [](const VimValue &a, const VimValue &b) {
                        return a.toNumber() < b.toNumber();
                    });
                } else {
                    const Qt::CaseSensitivity cs = how == "i" ? Qt::CaseInsensitive
                                                              : Qt::CaseSensitive;
                    std::stable_sort(l->begin(), l->end(),
                                     [cs](const VimValue &a, const VimValue &b) {
                        return QString::compare(a.toString(), b.toString(), cs) < 0;
                    });
                }
            }
        }
        *result = arg(0);
    } else if (name == "reverse") {
        if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            std::reverse(l->begin(), l->end());
        }
        *result = arg(0);
    } else if (name == "copy") {
        const VimValue v = arg(0);
        *result = v.isList() ? VimValue::list(*v.listData())
                : v.isDict() ? VimValue::dict(*v.dictData()) : v;
    } else if (name == "type") {
        int t = 0;
        switch (arg(0).type()) {
        case VimValue::Number: t = 0; break;
        case VimValue::String: t = 1; break;
        case VimValue::Func:   t = 2; break;
        case VimValue::List:   t = 3; break;
        case VimValue::Dict:   t = 4; break;
        case VimValue::Float:  t = 5; break;
        }
        *result = VimValue(qlonglong(t));
    } else if (name == "max" || name == "min") {
        const bool isMax = name == "max";
        if (arg(0).isList() && !arg(0).listData()->isEmpty()) {
            const QList<VimValue> &l = *arg(0).listData();
            qlonglong acc = l.at(0).toNumber();
            for (const VimValue &e : l) {
                const qlonglong n = e.toNumber();
                if (isMax ? n > acc : n < acc)
                    acc = n;
            }
            *result = VimValue(acc);
        } else {
            *result = VimValue(qlonglong(0));
        }
    } else if (name == "map" || name == "filter") {
        // The second argument is either an expression evaluated per element
        // with v:val/v:key set, or a Funcref/lambda called as f(key, val).
        // map() replaces each element; filter() keeps the ones it is true for.
        const bool isMap = name == "map";
        const VimValue callable = arg(1);
        const bool useFunc = callable.isFunc();
        const QString expr = callable.toString();
        VimValue savedVal, savedKey;
        const bool hadVal = variableValue("v:val", &savedVal);
        const bool hadKey = variableValue("v:key", &savedKey);
        bool ok = true;
        const auto apply = [&](const VimValue &key, const VimValue &val, VimValue *out) {
            if (useFunc)
                return invokeCallable(callable, {key, val}, out, error);
            setVariable("v:key", key);
            setVariable("v:val", val);
            return evaluateExpression(expr, out, error);
        };
        if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            QList<VimValue> kept;
            for (int i = 0; i < l->size() && ok; ++i) {
                VimValue r;
                if (!apply(VimValue(qlonglong(i)), l->at(i), &r))
                    ok = false;
                else if (isMap)
                    (*l)[i] = r;
                else if (r.toBool())
                    kept.append(l->at(i));
            }
            if (ok && !isMap)
                *l = kept;
        } else if (arg(0).isDict()) {
            QMap<QString, VimValue> *d = arg(0).dictData();
            const QList<QString> keys = d->keys();
            for (const QString &k : keys) {
                if (!ok)
                    break;
                VimValue r;
                if (!apply(VimValue(k), d->value(k), &r))
                    ok = false;
                else if (isMap)
                    d->insert(k, r);
                else if (!r.toBool())
                    d->remove(k);
            }
        }
        if (hadVal)
            setVariable("v:val", savedVal);
        else
            unsetVariable("v:val");
        if (hadKey)
            setVariable("v:key", savedKey);
        else
            unsetVariable("v:key");
        if (!ok)
            return false;
        *result = arg(0);
    } else if (name == "extend") {
        if (arg(0).isList() && arg(1).isList()) {
            arg(0).listData()->append(*arg(1).listData());
        } else if (arg(0).isDict() && arg(1).isDict()) {
            const QMap<QString, VimValue> *src = arg(1).dictData();
            for (auto it = src->cbegin(), end = src->cend(); it != end; ++it)
                arg(0).dictData()->insert(it.key(), it.value());
        }
        *result = arg(0);
    } else if (name == "index" || name == "count") {
        const auto equalValues = [](const VimValue &a, const VimValue &b) {
            return a.isString() && b.isString() ? a.toString() == b.toString()
                                                : a.toNumber() == b.toNumber();
        };
        qlonglong n = name == "index" ? -1 : 0;
        if (arg(0).isList()) {
            const QList<VimValue> &l = *arg(0).listData();
            if (name == "index") {
                for (int i = qMax(0, args.size() > 2 ? int(arg(2).toNumber()) : 0);
                     i < l.size(); ++i) {
                    if (equalValues(l.at(i), arg(1))) { n = i; break; }
                }
            } else {
                for (const VimValue &e : l)
                    n += equalValues(e, arg(1)) ? 1 : 0;
            }
        }
        *result = VimValue(n);
    } else if (name == "insert") {
        if (arg(0).isList()) {
            QList<VimValue> *l = arg(0).listData();
            const int idx = args.size() > 2 ? int(arg(2).toNumber()) : 0;
            l->insert(qBound(0, idx, l->size()), arg(1));
        }
        *result = arg(0);
    } else if (name == "repeat") {
        const int n = int(arg(1).toNumber());
        if (arg(0).isList()) {
            QList<VimValue> out;
            for (int i = 0; i < n; ++i)
                out.append(*arg(0).listData());
            *result = VimValue::list(out);
        } else {
            *result = VimValue(arg(0).toString().repeated(qMax(0, n)));
        }
    } else if (name == "trim") {
        *result = VimValue(arg(0).toString().trimmed());
    } else if (name == "nr2char") {
        *result = VimValue(QString(QChar(uint(arg(0).toNumber()))));
    } else if (name == "char2nr") {
        const QString str = arg(0).toString();
        *result = VimValue(qlonglong(str.isEmpty() ? 0 : str.at(0).unicode()));
    } else if (name == "matchstr" || name == "match") {
        const QRegularExpressionMatch mm =
            vimPatternToQtPattern(arg(1).toString()).match(arg(0).toString());
        if (name == "matchstr")
            *result = VimValue(mm.hasMatch() ? mm.captured(0) : QString());
        else
            *result = VimValue(qlonglong(mm.hasMatch() ? mm.capturedStart() : -1));
    } else if (name == "toupper") {
        *result = VimValue(arg(0).toString().toUpper());
    } else if (name == "tolower") {
        *result = VimValue(arg(0).toString().toLower());
    } else if (name == "str2nr") {
        const int base = args.size() > 1 ? int(arg(1).toNumber()) : 10;
        *result = VimValue(qlonglong(arg(0).toString().trimmed().toLongLong(nullptr, base)));
    } else if (name == "stridx") {
        const int start = args.size() > 2 ? int(arg(2).toNumber()) : 0;
        *result = VimValue(qlonglong(arg(0).toString().indexOf(arg(1).toString(), start)));
    } else if (name == "strpart") {
        const QString str = arg(0).toString();
        *result = args.size() > 2 ? VimValue(str.mid(int(arg(1).toNumber()), int(arg(2).toNumber())))
                                  : VimValue(str.mid(int(arg(1).toNumber())));
    } else if (name == "split") {
        const QRegularExpression sep(args.size() > 1 ? arg(1).toString() : QString("\\s+"));
        const QStringList parts = arg(0).toString().split(sep, Qt::SkipEmptyParts);
        QList<VimValue> items;
        for (const QString &p : parts)
            items.append(VimValue(p));
        *result = VimValue::list(items);
    } else if (name == "join") {
        const QString sep = args.size() > 1 ? arg(1).toString() : QString(" ");
        QStringList parts;
        if (arg(0).isList()) {
            for (const VimValue &e : *arg(0).listData())
                parts.append(e.toString());
        }
        *result = VimValue(parts.join(sep));
    } else if (name == "substitute") {
        // Vim patterns via the same translation as search/:s. "\0"/"&" is the
        // whole match and "\1".."\9" the groups; a "g" flag replaces all.
        const QRegularExpression re = vimPatternToQtPattern(arg(1).toString());
        const QString str = arg(0).toString();
        const QString tmpl = arg(2).toString();
        const bool global = args.size() > 3 && arg(3).toString().contains('g');
        // A replacement starting with "\=" is an expression worked out for each
        // match, with submatch() reaching the pieces that matched.
        const bool byExpression = tmpl.startsWith("\\=");
        QString out;
        int last = 0;
        QRegularExpressionMatchIterator it = re.globalMatch(str);
        while (it.hasNext()) {
            const QRegularExpressionMatch mm = it.next();
            out += str.mid(last, mm.capturedStart() - last);
            if (byExpression) {
                const QStringList savedSubMatches = m_subMatches;
                m_subMatches.clear();
                for (int k = 0; k <= 9; ++k)
                    m_subMatches.append(mm.captured(k));
                VimValue replacement;
                QString replaceError;
                if (evaluateExpression(tmpl.mid(2), &replacement, &replaceError))
                    out += replacement.toString();
                else
                    showMessage(MessageError, replaceError);
                m_subMatches = savedSubMatches;
                last = mm.capturedEnd();
                if (!global)
                    break;
                continue;
            }
            for (int k = 0; k < tmpl.size(); ++k) {
                const QChar ch = tmpl.at(k);
                if (ch == '\\' && k + 1 < tmpl.size()) {
                    const QChar n = tmpl.at(++k);
                    out += n.isDigit() ? mm.captured(n.digitValue()) : QString(n);
                } else if (ch == '&') {
                    out += mm.captured(0);
                } else {
                    out += ch;
                }
            }
            last = mm.capturedEnd();
            if (!global)
                break;
        }
        out += str.mid(last);
        *result = VimValue(out);
    } else if (name == "deepcopy") {
        *result = deepCopy(arg(0));
    } else if (name == "winsaveview") {
        // What it takes to put the view back as it was.
        QMap<QString, VimValue> view;
        view.insert("lnum", VimValue(qlonglong(cursorBlockNumber() + 1)));
        view.insert("col", VimValue(qlonglong(physicalCursorColumn())));
        view.insert("coladd", VimValue(qlonglong(0)));
        view.insert("curswant", VimValue(qlonglong(m_targetColumn)));
        view.insert("topline", VimValue(qlonglong(firstVisibleLine() + 1)));
        view.insert("topfill", VimValue(qlonglong(0)));
        view.insert("leftcol", VimValue(qlonglong(0)));
        view.insert("skipcol", VimValue(qlonglong(0)));
        *result = VimValue::dict(view);
    } else if (name == "winrestview") {
        if (arg(0).isDict()) {
            const QMap<QString, VimValue> *view = arg(0).dictData();
            if (view->contains("lnum")) {
                const int line = int(view->value("lnum").toNumber()) - 1;
                const int column = view->contains("col")
                    ? int(view->value("col").toNumber()) : 0;
                setCursorPosition(CursorPosition(line, column));
                if (!isVisualMode())
                    setAnchor();
            }
            if (view->contains("topline"))
                scrollToLine(int(view->value("topline").toNumber()) - 1);
        }
        *result = VimValue(qlonglong(0));
    } else if (name == "executable") {
        const QString found = QStandardPaths::findExecutable(arg(0).toString());
        *result = VimValue(qlonglong(found.isEmpty() ? 0 : 1));
    } else if (name == "system") {
        // The same way ":!" reaches a shell, so it stays out of this file.
        QString output;
        q->processOutput(arg(0).toString(), arg(1).toString(), &output);
        *result = VimValue(output);
    } else if (name == "iconv") {
        // Vim hands the string back untouched when it cannot convert, which is
        // also what happens here for an encoding Qt does not know.
        const QString from = arg(1).toString();
        const QString to = arg(2).toString();
        QStringDecoder decoder(from.toLatin1().constData());
        QStringEncoder encoder(to.toLatin1().constData());
        if (!decoder.isValid() || !encoder.isValid()) {
            *result = arg(0);
        } else {
            const QByteArray encoded = encoder(arg(0).toString());
            QStringDecoder back(to.toLatin1().constData());
            *result = VimValue(back(encoded));
        }
    } else if (name == "matchlist") {
        // The whole match followed by the nine possible groups, padded out, or
        // nothing at all when the pattern does not match.
        const QRegularExpression re = vimPatternToQtPattern(arg(1).toString());
        const QRegularExpressionMatch mm = re.match(arg(0).toString());
        QList<VimValue> items;
        if (mm.hasMatch()) {
            for (int k = 0; k <= 9; ++k)
                items.append(VimValue(mm.captured(k)));
        }
        *result = VimValue::list(items);
    } else if (name == "submatch") {
        // Only meaningful while a "\=" replacement is being worked out.
        *result = VimValue(m_subMatches.value(int(arg(0).toNumber())));
    } else if (name == "printf") {
        *result = VimValue(vimPrintf(args));
    } else if (name == "abs") {
        *result = arg(0).type() == VimValue::Float ? VimValue(qAbs(arg(0).toFloat()))
                                                    : VimValue(qAbs(arg(0).toNumber()));
    } else if (name == "empty") {
        const VimValue v = arg(0);
        bool e;
        if (v.isString())
            e = v.toString().isEmpty();
        else if (v.isList())
            e = v.listData()->isEmpty();
        else if (v.isDict())
            e = v.dictData()->isEmpty();
        else
            e = v.toNumber() == 0;
        *result = VimValue(qlonglong(e ? 1 : 0));
    } else if (name == "string") {
        *result = VimValue(arg(0).reprString());
    } else if (name == "has") {
        *result = VimValue(qlonglong(0)); // no feature is reported as present yet
    } else if (name == "exists") {
        const QString a = arg(0).toString();
        bool ex;
        if (a.startsWith('&'))
            ex = s.item(Utils::keyFromString(optionNameFromLet(a))) != nullptr;
        else if (a.startsWith('$'))
            ex = qEnvironmentVariableIsSet(a.mid(1).toLatin1().constData());
        else if (a.startsWith('*') || a.startsWith('?')) {
            // "*name" asks whether a function is there to be called, whether
            // built in, defined by a script or held by a variable as a Funcref.
            QString func = a.mid(1);
            if (func.startsWith("g:"))
                func = func.mid(2);
            VimValue held;
            ex = g.userFunctions.contains(func) || isBuiltinFunction(func)
                 || (variableValue(func, &held) && held.isFunc());
        } else {
            VimValue tmp;
            ex = variableValue(a, &tmp);
        }
        *result = VimValue(qlonglong(ex ? 1 : 0));
    } else if (name == "line") {
        *result = VimValue(qlonglong(lineColArg(arg(0).toString()).line + 1));
    } else if (name == "col") {
        const QString a = arg(0).toString();
        if (a == "$")
            *result = VimValue(qlonglong(lineContents(cursorLine() + 1).size() + 1));
        else
            *result = VimValue(qlonglong(lineColArg(a).column + 1));
    } else if (name == "getpos" || name == "getcurpos") {
        // [bufnum, lnum, col, off]; bufnum is 0 for the current buffer.
        const CursorPosition pos = lineColArg(name == "getcurpos"
                                              ? QString(".") : arg(0).toString());
        *result = VimValue::list({VimValue(qlonglong(0)),
                                  VimValue(qlonglong(pos.line + 1)),
                                  VimValue(qlonglong(pos.column + 1)),
                                  VimValue(qlonglong(0))});
    } else if (name == "setpos") {
        // setpos({expr}, [bufnum, lnum, col, off]); only the current buffer.
        const QList<VimValue> *l = arg(1).isList() ? arg(1).listData() : nullptr;
        if (!l || l->size() < 3) {
            *error = Tr::tr("setpos() expects a list of at least three numbers");
            return false;
        }
        const CursorPosition pos(int(l->at(1).toNumber()) - 1,
                                 int(l->at(2).toNumber()) - 1);
        const QString a = arg(0).toString();
        if (a == ".") {
            setCursorPosition(pos);
        } else if (a.size() == 2 && a.at(0) == '\'') {
            setMark(a.at(1), pos);
        } else {
            *error = Tr::tr("Invalid position: %1").arg(a);
            return false;
        }
        *result = VimValue(qlonglong(0));
    } else if (name == "getline") {
        const QString a = arg(0).toString();
        const int ln = a == "." ? cursorLine() + 1 : int(arg(0).toNumber());
        *result = VimValue(lineContents(ln));
    } else if (name == "setline") {
        // A list argument replaces consecutive lines starting at {lnum}.
        const int first = int(arg(0).toNumber());
        if (arg(1).isList()) {
            const QList<VimValue> *l = arg(1).listData();
            for (int i = 0; i < l->size(); ++i)
                setLineContents(first + i, l->at(i).toString());
        } else {
            setLineContents(first, arg(1).toString());
        }
        *result = VimValue(qlonglong(0));
    } else if (name == "escape") {
        QString s = arg(0).toString();
        const QString chars = arg(1).toString();
        QString out;
        for (const QChar c : std::as_const(s)) {
            if (chars.contains(c))
                out += '\\';
            out += c;
        }
        *result = VimValue(out);
    } else if (name == "indent") {
        // The indent of a line in screen columns, so a tab counts as 'tabstop'.
        const QString a = arg(0).toString();
        const int ln = a == "." ? cursorLine() + 1 : int(arg(0).toNumber());
        *result = VimValue(qlonglong(indentation(lineContents(ln)).logical));
    } else if (name == "synstack" || name == "synID") {
        // The syntax items at a position, innermost last. Only the ones the
        // document's language can be asked about are reported, so a caller
        // looking for anything besides "Comment" or "String" finds nothing.
        QStringList names;
        q->syntaxNamesRequested(int(arg(0).toNumber()), int(arg(1).toNumber()), &names);
        QList<VimValue> ids;
        for (const QString &syntaxName : std::as_const(names)) {
            if (!m_syntaxNames.contains(syntaxName))
                m_syntaxNames.append(syntaxName);
            ids.append(VimValue(qlonglong(m_syntaxNames.indexOf(syntaxName) + 1)));
        }
        if (name == "synID") // just the innermost one, 0 for nothing
            *result = ids.isEmpty() ? VimValue(qlonglong(0)) : ids.constLast();
        else
            *result = VimValue::list(ids);
    } else if (name == "synIDattr") {
        const int id = int(arg(0).toNumber());
        const QString what = arg(1).toString();
        // Only the name is known; the appearance of an item is not modelled.
        if (what == "name" && id >= 1 && id <= m_syntaxNames.size())
            *result = VimValue(m_syntaxNames.at(id - 1));
        else
            *result = VimValue(QString());
    } else if (name == "cursor") {
        // cursor({lnum}, {col}) or cursor([{lnum}, {col}, ...])
        int line = 0;
        int column = 1;
        if (arg(0).isList()) {
            const QList<VimValue> *l = arg(0).listData();
            line = l->size() > 0 ? int(l->at(0).toNumber()) : 0;
            column = l->size() > 1 ? int(l->at(1).toNumber()) : 1;
        } else {
            line = int(arg(0).toNumber());
            column = int(arg(1).toNumber());
        }
        if (line <= 0 || line > document()->blockCount()) {
            *result = VimValue(qlonglong(-1));
        } else {
            setCursorPosition(CursorPosition(line - 1, qMax(0, column - 1)));
            // Moving the cursor keeps the anchor, which is what extends a
            // selection while one is being made. Without one this is a plain
            // move and the anchor has to come along, or it would still mark
            // wherever the cursor happened to be before.
            if (!isVisualMode())
                setAnchor();
            setTargetColumn();
            *result = VimValue(qlonglong(0));
        }
    } else if (name == "search") {
        return searchFunction(args, result, error);
    } else if (name == "readfile") {
        // readfile({fname} [, {type} [, {max}]])
        const QString fileName = replaceTildeWithHome(arg(0).toString());
        const bool binary = arg(1).toString().contains('b');
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            // Vim reports this and hands back an empty list.
            showMessage(MessageError, Tr::tr("Cannot open file %1").arg(fileName));
            *result = VimValue::list();
            return true;
        }
        const QByteArray data = file.readAll();
        file.close();
        // Decode like ":source" does: prefer UTF-8, fall back to the local
        // 8-bit encoding for invalid byte sequences.
        QStringDecoder utf8(QStringDecoder::Utf8);
        QString text = utf8(data);
        if (utf8.hasError())
            text = QString::fromLocal8Bit(data);
        QStringList lines = text.split('\n');
        if (!binary) {
            if (lines.constFirst().startsWith(QChar(0xfeff))) // byte order mark
                lines[0].remove(0, 1);
            // A trailing newline does not stand for another line, and a CR in
            // front of one is dropped.
            if (lines.size() > 1 && lines.constLast().isEmpty())
                lines.removeLast();
            for (QString &line : lines) {
                if (line.endsWith('\r'))
                    line.chop(1);
            }
        }
        if (args.size() > 2) {
            const int max = int(arg(2).toNumber());
            if (max > 0)
                lines = lines.mid(0, max);
            else if (max < 0) // a negative count takes them from the end
                lines = lines.mid(qMax(0, lines.size() + max));
            else
                lines.clear();
        }
        QList<VimValue> items;
        for (const QString &line : std::as_const(lines))
            items.append(VimValue(line));
        *result = VimValue::list(items);
    } else if (name == "writefile") {
        // writefile({list}, {fname} [, {flags}])
        if (!arg(0).isList()) {
            *error = Tr::tr("writefile() expects a list");
            return false;
        }
        const QString fileName = replaceTildeWithHome(arg(1).toString());
        const QString flags = arg(2).toString();
        const bool binary = flags.contains('b');
        QFile file(fileName);
        const QIODevice::OpenMode mode = QIODevice::WriteOnly
            | (flags.contains('a') ? QIODevice::Append : QIODevice::Truncate);
        if (!file.open(mode)) {
            showMessage(MessageError, Tr::tr("Cannot open file %1").arg(fileName));
            *result = VimValue(qlonglong(-1));
            return true;
        }
        QStringList lines;
        for (const VimValue &item : *arg(0).listData())
            lines.append(item.toString());
        QString text = lines.join('\n');
        // Text mode ends the last line as well; binary mode leaves it as it is,
        // so an empty last item is what puts a newline there.
        if (!binary && !lines.isEmpty())
            text += '\n';
        file.write(text.toUtf8());
        file.close();
        *result = VimValue(qlonglong(0));
    } else if (name == "fnamemodify") {
        *result = VimValue(applyFileNameModifiers(
            replaceTildeWithHome(arg(0).toString()), arg(1).toString()));
    } else if (name == "filereadable") {
        const QFileInfo fi(replaceTildeWithHome(arg(0).toString()));
        *result = VimValue(qlonglong(fi.isFile() && fi.isReadable() ? 1 : 0));
    } else if (name == "isdirectory") {
        *result = VimValue(qlonglong(
            QFileInfo(replaceTildeWithHome(arg(0).toString())).isDir() ? 1 : 0));
    } else if (name == "getcwd") {
        *result = VimValue(QDir::currentPath());
    } else if (name == "fnameescape" || name == "shellescape") {
        const QString in = arg(0).toString();
        if (name == "shellescape") {
            // Wrapped in single quotes, with any of its own doubled out.
            QString out = in;
            out.replace("'", "'\\''");
            *result = VimValue("'" + out + "'");
        } else {
            // A backslash in front of what would otherwise be taken as syntax.
            static const QString special = " \t\n*?[{`$\\%#'\"|!<";
            QString out;
            for (const QChar c : in) {
                if (special.contains(c))
                    out += '\\';
                out += c;
            }
            *result = VimValue(out);
        }
    } else if (name == "bufnr") {
        // bufnr([{buf}]) - only the buffer this handler works on can be named,
        // since there is no list of the others to look through.
        const QString a = args.isEmpty() ? QString("%") : arg(0).toString();
        if (a == "%" || a == "" || a == "$")
            *result = VimValue(qlonglong(bufferNumber()));
        else if (!m_currentFileName.isEmpty() && m_currentFileName.contains(a))
            *result = VimValue(qlonglong(bufferNumber()));
        else
            *result = VimValue(qlonglong(-1));
    } else if (name == "getbufvar" || name == "setbufvar") {
        // getbufvar({buf}, {varname} [, {def}]) and its setter. Only this
        // buffer is reachable; anything else reads as the default and is not
        // written.
        const QString which = arg(0).toString();
        const bool isThisBuffer = which.isEmpty() || which == "%"
            || int(arg(0).toNumber()) == bufferNumber()
            || (!m_currentFileName.isEmpty() && m_currentFileName.contains(which));
        QString varName = arg(1).toString();
        const bool isOption = varName.startsWith('&');
        if (isOption)
            varName = varName.mid(1);
        // The name is taken as it stands: Vim looks for a buffer variable
        // called exactly that, so "b:mine" is not the same as "mine".

        if (name == "getbufvar") {
            VimValue found;
            bool ok = false;
            if (isThisBuffer)
                ok = isOption ? optionValue(varName, &found)
                              : variableValue("b:" + varName, &found);
            // Nothing there and nothing offered instead reads as empty.
            *result = ok ? found
                         : (args.size() > 2 ? arg(2) : VimValue(QString()));
        } else {
            if (isThisBuffer) {
                if (isOption)
                    setOption(varName, arg(2));
                else
                    setVariable("b:" + varName, arg(2));
            }
            *result = VimValue(qlonglong(0));
        }
    } else if (name == "strftime") {
        // strftime({format} [, {time}]) - the C library does the formatting, so
        // the conversions are the ones Vim documents. The parts are filled from
        // a QDateTime rather than localtime() to keep this off platform calls,
        // which leaves the time zone unset and "%Z" and "%z" with nothing to
        // report.
        const QDateTime when = args.size() > 1
            ? QDateTime::fromSecsSinceEpoch(qint64(arg(1).toNumber()))
            : QDateTime::currentDateTime();
        const QDate date = when.date();
        const QTime time = when.time();
        std::tm parts = {};
        parts.tm_sec = time.second();
        parts.tm_min = time.minute();
        parts.tm_hour = time.hour();
        parts.tm_mday = date.day();
        parts.tm_mon = date.month() - 1;
        parts.tm_year = date.year() - 1900;
        parts.tm_wday = date.dayOfWeek() % 7; // Qt counts from Monday, tm from Sunday
        parts.tm_yday = date.dayOfYear() - 1;
        parts.tm_isdst = -1;
        const QByteArray format = arg(0).toString().toLocal8Bit();
        char buffer[1024];
        const size_t used = std::strftime(buffer, sizeof(buffer),
                                          format.constData(), &parts);
        *result = VimValue(QString::fromLocal8Bit(buffer, int(used)));
    } else if (name == "did_filetype") {
        *result = VimValue(qlonglong(didFileType() ? 1 : 0));
    } else if (name == "expand") {
        *result = VimValue(expandKeyword(arg(0).toString()));
    } else if (name == "function" || name == "funcref") {
        *result = VimValue::func(arg(0).toString());
    } else if (name == "call") {
        const QList<VimValue> callArgs = arg(1).isList() ? *arg(1).listData()
                                                         : QList<VimValue>();
        return invokeCallable(arg(0), callArgs, result, error);
    } else if (g.userFunctions.contains(name)) {
        *result = callUserFunction(name, g.userFunctions.value(name), args);
    } else {
        // A variable may hold a funcref or lambda; call it.
        VimValue v;
        if (variableValue(name, &v) && v.isFunc())
            return invokeCallable(v, args, result, error);
        // "a#b()" lives in "autoload/a.vim" along the runtimepath, loaded when
        // it is first needed. Look there once and try again.
        if (name.contains('#') && loadAutoloadScript(name)
            && g.userFunctions.contains(name)) {
            *result = callUserFunction(name, g.userFunctions.value(name), args);
            return true;
        }
        *error = Tr::tr("Unknown function: %1").arg(name);
        return false;
    }
    return true;
}

bool FakeVimHandler::Private::invokeCallable(const VimValue &callable,
    const QList<VimValue> &args, VimValue *result, QString *error)
{
    if (!callable.isFunc()) {
        *error = Tr::tr("Not a function");
        return false;
    }
    const VimFunc *fn = callable.funcData();
    if (!fn->isLambda)
        return callFunction(fn->name, args, result, error);

    // A lambda: bind parameters (and the closure scope) in a fresh frame and
    // evaluate the body expression.
    QHash<QString, VimValue> frame = fn->captured;
    for (int i = 0; i < fn->params.size(); ++i)
        frame.insert(fn->params.at(i), i < args.size() ? args.at(i) : VimValue());

    const LoopSignal savedSignal = m_loopSignal;
    const bool savedReturning = m_returning;
    const VimValue savedReturnValue = m_returnValue;
    m_localScopes.append(frame);
    const bool ok = evaluateExpression(fn->body, result, error);
    m_localScopes.removeLast();
    m_loopSignal = savedSignal;
    m_returning = savedReturning;
    m_returnValue = savedReturnValue;
    return ok;
}

bool FakeVimHandler::Private::handleExUnletCommand(const ExCommand &cmd)
{
    // :unlet[!] {var} ...
    if (!cmd.matches("unl", "unlet"))
        return false;

    const QStringList names = cmd.args.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString &name : names) {
        if (!unsetVariable(name) && !cmd.hasBang) {
            showMessage(MessageError, Tr::tr("Undefined variable: %1").arg(name));
            return true;
        }
    }
    return true;
}

// The expression text of an ex-command whose argument is a Vimscript
// expression. These commands take no "bang", so a leading "!" stripped by
// parseExCommand is really the unary-not operator and must be restored.
static QString exprText(const ExCommand &cmd)
{
    return (cmd.hasBang ? QString('!') : QString()) + cmd.args;
}

bool FakeVimHandler::Private::handleExEchoCommand(const ExCommand &cmd)
{
    // :echo / :echomsg / :echon evaluate their arguments, joining with a space;
    // :echoerr shows the result as an error.
    // ":echohl {group}" only picks the colour the next message is drawn in,
    // which is not something that can be shown here, so it is accepted and
    // passed over rather than left to fail in the middle of a script.
    if (cmd.matches("echoh", "echohl"))
        return true;

    const bool isError = cmd.matches("echoe", "echoerr");
    if (!isError && !cmd.matches("ec", "echo") && !cmd.matches("echom", "echomsg")
        && !cmd.matches("echon", "echon"))
        return false;

    VimExpr e(this, exprText(cmd));
    QStringList parts;
    while (!e.atEnd()) {
        const VimValue v = e.parseExpr();
        if (!e.ok()) {
            showMessage(MessageError, e.error());
            return true;
        }
        parts.append(v.toString());
    }
    showMessage(isError ? MessageError : MessageInfo, parts.join(' '));
    return true;
}

bool FakeVimHandler::Private::handleExSilentCommand(const ExCommand &cmd)
{
    // :silent[!] {command} - run the command with messages (and, with "!",
    // errors) suppressed.
    if (!cmd.matches("sil", "silent"))
        return false;

    ++m_messageSilence;
    const bool savedSilenceErrors = m_silenceErrors;
    m_silenceErrors = m_silenceErrors || cmd.hasBang;

    QString line = m_vim9 ? vim9Statement(cmd.args) : cmd.args;
    ExCommand sub;
    while (parseExCommand(&line, &sub))
        handleExCommandHelper(sub);

    --m_messageSilence;
    m_silenceErrors = savedSilenceErrors;
    return true;
}

bool FakeVimHandler::Private::handleExModifierCommand(const ExCommand &cmd)
{
    // Command modifiers that prefix another command. Only ":noautocmd" has an
    // effect here; the rest concern state FakeVim does not keep (the jump list
    // is left alone, marks are not adjusted by these commands anyway), so they
    // just run what follows.
    const bool noAutocmd = cmd.matches("noa", "noautocmd");
    if (!noAutocmd
        && !cmd.matches("keepj", "keepjumps")
        && !cmd.matches("kee", "keepmarks")
        && !cmd.matches("keepa", "keepalt")
        && !cmd.matches("keepp", "keeppatterns")
        && !cmd.matches("loc", "lockmarks")
        && !cmd.matches("uns", "unsilent")) {
        return false;
    }
    const bool savedNoAutocmd = m_noAutocmd;
    if (noAutocmd)
        m_noAutocmd = true;

    // What follows a modifier is a statement in its own right, so in Vim9 it
    // may be a bare function call and needs the same rewriting as any line.
    QString line = m_vim9 ? vim9Statement(cmd.args) : cmd.args;
    ExCommand sub;
    while (parseExCommand(&line, &sub))
        handleExCommandHelper(sub);

    m_noAutocmd = savedNoAutocmd;
    return true;
}

static bool isAutocmdEvent(const QString &word)
{
    static const QSet<QString> events = {
        // Fired from here.
        "bufnewfile", "bufread", "bufreadpost", "bufenter", "bufleave",
        "bufwinenter", "bufwritepre", "bufwritepost", "filetype",
        "insertenter", "insertleave", "textchanged", "textchangedi",
        "cursormoved", "cursormovedi", "vimenter", "winenter", "winleave",
        "user",
        // Accepted so that a list naming one of these alongside an event that
        // does fire is still understood. Registering for one of them is not an
        // error, it simply never comes up.
        "bufadd", "bufcreate", "bufdelete", "buffilepre", "buffilepost",
        "bufhidden", "bufnew", "bufreadcmd", "bufreadpre", "bufunload",
        "bufwinleave", "bufwipeout", "bufwritecmd", "cmdlineenter",
        "cmdlineleave", "colorscheme", "completedone", "cursorhold",
        "cursorholdi", "dirchanged", "encodingchanged", "filechangedshell",
        "filereadpost", "filereadpre", "filewritepost", "filewritepre",
        "focusgained", "focuslost", "insertcharpre", "insertchange",
        "menupopup", "optionset", "quitpre", "safestate", "sessionloadpost",
        "shellcmdpost", "shellfilterpost", "sourcepost", "sourcepre",
        "stdinreadpost", "swapexists", "syntax", "tabclosed", "tabenter",
        "tableave", "tabnew", "termopen", "textyankpost", "vimleave",
        "vimleavepre", "vimresized", "winclosed", "winnew", "winresized"
    };
    return events.contains(word.toLower());
}

// "BufRead" and "BufReadPost" are two names for the same event, so reduce them
// to one before comparing a registration against a fired event.
static QString canonicalAutocmdEvent(const QString &event)
{
    const QString lower = event.toLower();
    return lower == "bufread" ? QStringLiteral("bufreadpost") : lower;
}

static bool autocmdPatternMatches(const QString &pattern, const QString &fileName)
{
    const QStringList patterns = pattern.split(',', Qt::SkipEmptyParts);
    for (const QString &p : patterns) {
        if (p == "*")
            return true;
        QString re = QRegularExpression::escape(p);
        re.replace("\\*", ".*").replace("\\?", ".");
        const QRegularExpression rx('^' + re + '$');
        if (rx.match(fileName).hasMatch())
            return true;
        if (!p.contains('/') && rx.match(fileName.section('/', -1)).hasMatch())
            return true;
    }
    return false;
}

bool FakeVimHandler::Private::handleExAutocmdCommand(const ExCommand &cmd)
{
    // :autocmd [group] {event} {pattern} {command} - register a command; a bare
    // :autocmd! clears all registered autocommands.
    if (!cmd.matches("au", "autocmd"))
        return false;

    QStringList tokens = cmd.args.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    // One command may be registered for several events at once, written with
    // commas between them.
    const auto namesEvents = [](const QString &token) {
        const QStringList parts = token.split(',', Qt::SkipEmptyParts);
        if (parts.isEmpty())
            return false;
        for (const QString &part : parts) {
            if (!isAutocmdEvent(part))
                return false;
        }
        return true;
    };
    // A group named here applies to this one command; otherwise the group
    // ":augroup" left current does, if any.
    QString group = g.currentAutoGroup;
    if (!tokens.isEmpty() && !namesEvents(tokens.first()))
        group = tokens.takeFirst();

    if (tokens.isEmpty()) {
        // ":autocmd!" removes what is registered, for the group in hand only
        // when there is one, so a plugin clearing its own leaves others alone.
        if (cmd.hasBang) {
            if (group.isEmpty()) {
                g.autoCommands.clear();
            } else {
                for (int i = g.autoCommands.size() - 1; i >= 0; --i) {
                    if (g.autoCommands.at(i).group == group)
                        g.autoCommands.removeAt(i);
                }
            }
        }
        return true;
    }
    if (tokens.size() < 3)
        return true; // nothing to register

    const QStringList events = tokens.takeFirst().split(',', Qt::SkipEmptyParts);
    const QString pattern = tokens.takeFirst();
    const QString command = tokens.join(' ');
    for (const QString &event : events) {
        AutoCommand ac;
        ac.group = group;
        ac.event = event;
        ac.pattern = pattern;
        ac.command = command;
        g.autoCommands.append(ac);
    }
    return true;
}

bool FakeVimHandler::Private::handleExAugroupCommand(const ExCommand &cmd)
{
    // :aug[roup] {name} makes a group current so the autocommands that follow
    // belong to it; ":augroup END" ends that. Plugins wrap their autocommands
    // this way to be able to clear their own without touching anyone else's.
    if (!cmd.matches("aug", "augroup"))
        return false;

    const QString name = cmd.args.trimmed();
    if (name.compare("END", Qt::CaseInsensitive) == 0)
        g.currentAutoGroup.clear();
    else if (cmd.hasBang) // ":augroup! {name}" deletes it
        g.autoCommands.removeIf([&name](const AutoCommand &ac) { return ac.group == name; });
    else
        g.currentAutoGroup = name;
    return true;
}

// Apply one line if it carries a modeline. Two forms are accepted:
//   [text]{white}{vi:|vim:|ex:}[white]{options}
//   [text]{white}{vi:|vim:|Vim:|ex:}[white]se[t] {options}:[text]
// In the first the options run to the end of the line and ":" separates them;
// in the second they end at the first unescaped ":", which is what makes a
// modeline usable inside a C comment.
void FakeVimHandler::Private::applyModeline(const QString &line)
{
    // The marker has to start the line or follow a blank, so that an ordinary
    // word ending in "vim:" is not mistaken for one. "vim" may carry a version
    // requirement, as in "vim>702:".
    static const QRegularExpression markerRe(
        "(?:^|[ \\t])(vim|Vim|vi|ex)([<=>]?)([0-9]+)?:");
    const QRegularExpressionMatch m = markerRe.match(line);
    if (!m.hasMatch())
        return;

    if (!m.captured(3).isEmpty()) {
        // Compare against the version v:version reports.
        const int wanted = m.captured(3).toInt();
        const int have = 900;
        const QString op = m.captured(2);
        const bool applies = op == "<" ? have < wanted
                           : op == ">" ? have > wanted
                           : op == "=" ? have == wanted
                                       : have >= wanted;
        if (!applies)
            return;
    }

    QString rest = line.mid(m.capturedEnd()).trimmed();
    QStringList options;
    static const QRegularExpression setRe("^se(?:t)?[ \\t]+");
    const QRegularExpressionMatch setMatch = setRe.match(rest);
    if (setMatch.hasMatch()) {
        rest = rest.mid(setMatch.capturedLength());
        // Ends at the first ":" that is not escaped; anything after it is text.
        QString collected;
        for (int i = 0; i < rest.size(); ++i) {
            const QChar c = rest.at(i);
            if (c == '\\' && i + 1 < rest.size()) {
                collected += rest.at(++i);
                continue;
            }
            if (c == ':')
                break;
            collected += c;
        }
        options = collected.split(QRegularExpression("[ \\t]+"), Qt::SkipEmptyParts);
    } else {
        options = rest.split(QRegularExpression("[ \\t:]+"), Qt::SkipEmptyParts);
    }

    for (const QString &option : std::as_const(options))
        handleExCommand("set " + option);
}

// Look for a modeline in the first and last 'modelines' lines of the buffer.
void FakeVimHandler::Private::processModelines()
{
    if (!s.modeline())
        return;
    const int count = s.modelines();
    if (count <= 0)
        return;

    const int total = document()->blockCount();
    const int head = qMin(count, total);
    for (int i = 0; i < head; ++i)
        applyModeline(document()->findBlockByNumber(i).text());
    // Do not look at a line twice when the file is shorter than 2 * 'modelines'.
    for (int i = qMax(head, total - count); i < total; ++i)
        applyModeline(document()->findBlockByNumber(i).text());
}

void FakeVimHandler::Private::setFileType(const QString &type, bool fallback)
{
    m_fileType = type;
    // A guess does not count as knowing the type, so a later ":setf" may still
    // replace it.
    if (!fallback)
        m_didFileType = true;
    triggerAutocmd("FileType");
}

// Vim's did_filetype(): true only while autocommands run and the type of this
// buffer has already been settled by something other than a guess.
bool FakeVimHandler::Private::didFileType() const
{
    return m_autocmdDepth > 0 && m_didFileType;
}

bool FakeVimHandler::Private::handleExSetFileTypeCommand(const ExCommand &cmd)
{
    // :setf[iletype] [FALLBACK] {name} - sets the type unless one is already
    // settled for this buffer, so the first rule that recognizes it wins.
    // FALLBACK marks a guess that a later ":setf" is allowed to replace.
    if (!cmd.matches("setf", "setfiletype"))
        return false;
    QString type = cmd.args.trimmed();
    const bool fallback = type.startsWith("FALLBACK ");
    if (fallback)
        type = type.mid(9).trimmed();
    if (!didFileType())
        setFileType(type, fallback);
    return true;
}

bool FakeVimHandler::Private::handleExDoAutocmdCommand(const ExCommand &cmd)
{
    // :doautocmd {event} - fire the matching autocommands now.
    if (!cmd.matches("doau", "doautocmd"))
        return false;

    const QStringList tokens = cmd.args.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString &t : tokens) {
        if (isAutocmdEvent(t)) {
            triggerAutocmd(t);
            return true;
        }
    }
    if (!tokens.isEmpty())
        triggerAutocmd(tokens.first());
    return true;
}

bool FakeVimHandler::Private::handleExCommandDefCommand(const ExCommand &cmd)
{
    // :command[!] [-attributes] {Name} {replacement}   (define a user command)
    // :comclear                                         (remove all)
    // :delcommand {Name}                                (remove one)
    if (cmd.matches("comc", "comclear")) {
        g.userCommands.clear();
        return true;
    }
    if (cmd.matches("delc", "delcommand")) {
        g.userCommands.remove(cmd.args.trimmed());
        return true;
    }
    if (!cmd.matches("com", "command"))
        return false;

    QString rest = cmd.args.trimmed();
    // Skip leading attribute tokens (-nargs=, -range, -bang, ...).
    while (rest.startsWith('-')) {
        const int sp = rest.indexOf(QRegularExpression("\\s"));
        if (sp < 0) {
            rest.clear();
            break;
        }
        rest = rest.mid(sp + 1).trimmed();
    }
    const int sp = rest.indexOf(QRegularExpression("\\s"));
    if (sp < 0)
        return true; // ":command" with no replacement: listing, treated as no-op
    g.userCommands.insert(rest.left(sp), rest.mid(sp + 1).trimmed());
    return true;
}

bool FakeVimHandler::Private::handleExUserCommand(const ExCommand &cmd)
{
    const auto it = g.userCommands.constFind(cmd.cmd);
    if (it == g.userCommands.constEnd())
        return false;

    // Expand the replacement's <...> tokens from the invocation.
    const QString args = cmd.args;
    QStringList fargs;
    const QStringList words = args.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString &w : words)
        fargs.append('"' + QString(w).replace('\\', "\\\\").replace('"', "\\\"") + '"');
    const int l1 = cmd.range.isValid() ? lineForPosition(cmd.range.beginPos) : cursorLine() + 1;
    const int l2 = cmd.range.isValid() ? lineForPosition(cmd.range.endPos) : cursorLine() + 1;

    QString line = *it;
    line.replace("<q-args>", '"' + QString(args).replace('\\', "\\\\").replace('"', "\\\"") + '"');
    line.replace("<f-args>", fargs.join(", "));
    line.replace("<args>", args);
    line.replace("<bang>", cmd.hasBang ? "!" : QString());
    line.replace("<count>", QString::number(cmd.count));
    line.replace("<line1>", QString::number(l1));
    line.replace("<line2>", QString::number(l2));
    line.replace("<lt>", "<");

    ExCommand sub;
    while (parseExCommand(&line, &sub)) {
        if (!handleExCommandHelper(sub)) {
            showMessage(MessageError, Tr::tr("Not an editor command: %1").arg(sub.cmd));
            break;
        }
    }
    return true;
}

void FakeVimHandler::Private::triggerAutocmd(const QString &event)
{
    const QString fired = canonicalAutocmdEvent(event);

    // Reading a buffer starts a fresh file type detection, so a ":setf" in one
    // of the read autocommands is free to claim it.
    if (fired == "bufreadpost" || fired == "bufnewfile")
        m_didFileType = false;

    // One level of nesting, which is what lets a ":setf" inside a read
    // autocommand still run the FileType rules; deeper chains are cut off.
    if (g.autoCommands.isEmpty() || m_noAutocmd || m_autocmdDepth >= 2)
        return;

    // FileType patterns match the filetype; other events match the file name.
    const QString target = fired == "filetype" ? m_fileType : m_currentFileName;
    // Copy: a fired command might register or clear autocommands.
    const QList<AutoCommand> commands = g.autoCommands;
    ++m_autocmdDepth;
    for (const AutoCommand &ac : commands) {
        if (canonicalAutocmdEvent(ac.event) != fired)
            continue;
        if (!autocmdPatternMatches(ac.pattern, target))
            continue;
        QString line = ac.command;
        ExCommand sub;
        while (parseExCommand(&line, &sub))
            handleExCommandHelper(sub);
    }
    --m_autocmdDepth;
}

bool FakeVimHandler::Private::handleExExecuteCommand(const ExCommand &cmd)
{
    // :execute {expr}... - evaluate the arguments, join with a space and run
    // the result as an ex command line.
    if (!cmd.matches("exe", "execute"))
        return false;

    VimExpr e(this, exprText(cmd));
    QStringList parts;
    while (!e.atEnd()) {
        const VimValue v = e.parseExpr();
        if (!e.ok()) {
            showMessage(MessageError, e.error());
            return true;
        }
        parts.append(v.toString());
    }

    QString line = parts.join(' ');
    ExCommand sub;
    while (parseExCommand(&line, &sub)) {
        if (!handleExCommandHelper(sub)) {
            showMessage(MessageError, Tr::tr("Not an editor command: %1").arg(sub.cmd));
            break;
        }
    }
    return true;
}

bool FakeVimHandler::Private::evalCondition(const QString &expr)
{
    VimValue v;
    QString error;
    if (!evaluateExpression(expr, &v, &error)) {
        showMessage(MessageError, error);
        return false;
    }
    return v.toBool();
}

// Command names that end a control-flow block; handled by the enclosing level.
static bool isFunctionStart(const QString &cmd)
{
    return cmd == "function" || cmd == "func" || cmd == "fun" || cmd == "fu"
        || cmd == "def";
}

static bool isFunctionEnd(const QString &cmd)
{
    return cmd == "endfunction" || cmd == "endfunc" || cmd == "enddef";
}

static bool isBlockTerminator(const QString &cmd)
{
    return cmd == "endif" || cmd == "else" || cmd == "elseif"
        || cmd == "endwhile" || cmd == "endfor" || isFunctionEnd(cmd)
        || cmd == "catch" || cmd == "finally" || cmd == "endtry";
}

void FakeVimHandler::Private::execSequence(const QList<ExCommand> &cmds,
    int &index, bool active)
{
    while (index < cmds.size()) {
        const ExCommand c = cmds.at(index);
        if (c.cmd == "if") {
            ++index;
            execIf(cmds, index, active, active && evalCondition(exprText(c)));
        } else if (c.cmd == "while") {
            ++index;
            execWhile(cmds, index, active, exprText(c));
        } else if (c.cmd == "for") {
            ++index;
            execFor(cmds, index, active, c.args);
        } else if (c.cmd == "try") {
            ++index;
            execTry(cmds, index, active);
        } else if (c.cmd == "throw") {
            if (active) {
                VimValue v;
                QString error;
                if (evaluateExpression(exprText(c), &v, &error)) {
                    m_throwing = true;
                    m_exception = v.toString();
                } else {
                    showMessage(MessageError, error);
                }
            }
            ++index;
        } else if (isFunctionStart(c.cmd)) {
            ++index;
            collectFunction(cmds, index, active, c);
        } else if (c.cmd == "return") {
            if (active) {
                VimValue v;
                QString error;
                if (!c.args.isEmpty() && !evaluateExpression(exprText(c), &v, &error))
                    showMessage(MessageError, error);
                m_returnValue = v;
                m_returning = true;
            }
            ++index;
        } else if (c.cmd == "break" || c.cmd == "continue") {
            if (active)
                m_loopSignal = c.cmd == "break" ? BreakSignal : ContinueSignal;
            ++index;
        } else if (c.cmd == "finish") {
            if (active)
                m_finishing = true;
            ++index;
        } else if (isBlockTerminator(c.cmd)) {
            return; // belongs to the enclosing construct
        } else {
            if (active) {
                ExCommand cmd = c;
                if (!handleExCommandHelper(cmd))
                    showMessage(MessageError, Tr::tr("Not an editor command: %1").arg(c.cmd));
            }
            ++index;
        }
        if (interpreterInterrupted())
            return; // :break/:continue/:return/:throw stops the current sequence
    }
}

void FakeVimHandler::Private::execIf(const QList<ExCommand> &cmds,
    int &index, bool active, bool condition)
{
    // "index" is positioned just after the :if. Run branches until :endif,
    // executing the first one whose guard holds (only while "active").
    bool anyTaken = active && condition;
    execSequence(cmds, index, anyTaken);
    while (index < cmds.size() && !interpreterInterrupted()) {
        const ExCommand c = cmds.at(index);
        if (c.cmd == "endif") {
            ++index;
            return;
        }
        if (c.cmd == "elseif") {
            ++index;
            const bool take = active && !anyTaken && evalCondition(exprText(c));
            anyTaken = anyTaken || take;
            execSequence(cmds, index, take);
        } else if (c.cmd == "else") {
            ++index;
            const bool take = active && !anyTaken;
            anyTaken = anyTaken || take;
            execSequence(cmds, index, take);
        } else {
            return; // missing :endif; leave the terminator to the caller
        }
    }
}

void FakeVimHandler::Private::execWhile(const QList<ExCommand> &cmds,
    int &index, bool active, const QString &condition)
{
    // "index" is positioned just after the :while. Locate the matching
    // :endwhile by skipping the body once, then run the body while the
    // condition holds.
    const int bodyStart = index;
    int scan = bodyStart;
    execSequence(cmds, scan, false);
    const int endIndex = scan;

    if (active) {
        // Guard against a runaway loop freezing the editor.
        const int loopLimit = 1000000;
        int iterations = 0;
        while (evalCondition(condition)) {
            int i = bodyStart;
            execSequence(cmds, i, true);
            if (m_returning || m_throwing)
                break; // :return/:throw unwinds past the loop
            if (m_loopSignal == BreakSignal) {
                m_loopSignal = NoSignal;
                break;
            }
            m_loopSignal = NoSignal; // a :continue just ends this iteration
            if (++iterations > loopLimit) {
                showMessage(MessageError, Tr::tr("Loop iteration limit exceeded"));
                break;
            }
        }
    }

    index = endIndex;
    if (index < cmds.size() && cmds.at(index).cmd == "endwhile")
        ++index;
}

void FakeVimHandler::Private::execFor(const QList<ExCommand> &cmds,
    int &index, bool active, const QString &spec)
{
    // spec is "{var} in {listexpr}". Bind {var} to each list element in turn.
    const int bodyStart = index;
    int scan = bodyStart;
    execSequence(cmds, scan, false);
    const int endIndex = scan;

    if (active) {
        // The loop variable is a name or, for unpacking, "[a, b, ...]".
        static const QRegularExpression re(
            "^\\s*(\\[[^\\]]*\\]|[@&$]?[A-Za-z_][A-Za-z0-9_:]*)\\s+in\\s+(.*)$");
        const QRegularExpressionMatch m = re.match(spec);
        VimValue listValue;
        QString error;
        if (!m.hasMatch()) {
            showMessage(MessageError, Tr::tr("Invalid :for: %1").arg(spec));
        } else if (!evaluateExpression(m.captured(2), &listValue, &error)) {
            showMessage(MessageError, error);
        } else if (!listValue.isList()) {
            showMessage(MessageError, Tr::tr(":for requires a list"));
        } else {
            const QString var = m.captured(1);
            QStringList unpackNames;
            if (var.startsWith('[')) {
                unpackNames = var.mid(1, var.indexOf(']') - 1)
                                  .split(QRegularExpression("\\s*,\\s*"), Qt::SkipEmptyParts);
            }
            // Iterate a copy so mutating the list in the body is well-defined.
            const QList<VimValue> items = *listValue.listData();
            for (const VimValue &item : items) {
                if (unpackNames.isEmpty()) {
                    setVariable(var, item);
                } else {
                    const QList<VimValue> parts =
                        item.isList() ? *item.listData() : QList<VimValue>();
                    for (int j = 0; j < unpackNames.size(); ++j)
                        setVariable(unpackNames.at(j),
                                    j < parts.size() ? parts.at(j) : VimValue());
                }
                int i = bodyStart;
                execSequence(cmds, i, true);
                if (m_returning || m_throwing)
                    break; // :return/:throw unwinds past the loop
                if (m_loopSignal == BreakSignal) {
                    m_loopSignal = NoSignal;
                    break;
                }
                m_loopSignal = NoSignal; // :continue just ends this iteration
            }
        }
    }

    index = endIndex;
    if (index < cmds.size() && cmds.at(index).cmd == "endfor")
        ++index;
}

void FakeVimHandler::Private::execTry(const QList<ExCommand> &cmds,
    int &index, bool active)
{
    // Snapshot of a pending interrupt (throw/return/break) so it can be carried
    // across the :catch/:finally clauses and re-raised afterwards.
    struct Pending {
        bool thrown = false;
        QString exception;
        bool returning = false;
        VimValue returnValue;
        LoopSignal signal = NoSignal;
        bool empty() const { return !thrown && !returning && signal == NoSignal; }
    };
    const auto capture = [this]() {
        Pending p{m_throwing, m_exception, m_returning, m_returnValue, m_loopSignal};
        m_throwing = false;
        m_returning = false;
        m_loopSignal = NoSignal;
        return p;
    };
    const auto catchMatches = [](const QString &args, const QString &ex) {
        QString pat = args.trimmed();
        if (pat.isEmpty())
            return true;
        if (pat.size() >= 2 && pat.startsWith('/') && pat.endsWith('/'))
            pat = pat.mid(1, pat.size() - 2);
        return vimPatternToQtPattern(pat).match(ex).hasMatch();
    };

    execSequence(cmds, index, active); // the :try body
    Pending pending = capture();
    bool handled = !pending.thrown;
    execSequence(cmds, index, false); // skip the rest of the body to the clauses

    while (index < cmds.size() && cmds.at(index).cmd == "catch") {
        const ExCommand c = cmds.at(index);
        ++index;
        const bool match = active && pending.thrown && !handled
                           && catchMatches(c.args, pending.exception);
        if (match)
            setVariable("v:exception", VimValue(pending.exception));
        execSequence(cmds, index, match);
        if (match)
            handled = true;
        if (interpreterInterrupted()) { // the catch body interrupted
            pending = capture();
            handled = !pending.thrown;
            execSequence(cmds, index, false);
        }
    }

    if (index < cmds.size() && cmds.at(index).cmd == "finally") {
        ++index;
        execSequence(cmds, index, active);
        if (interpreterInterrupted()) { // :finally replaces any pending interrupt
            pending = capture();
            handled = !pending.thrown;
            execSequence(cmds, index, false);
        }
    }

    if (index < cmds.size() && cmds.at(index).cmd == "endtry")
        ++index;

    // Re-raise whatever survived unhandled.
    if (pending.thrown && !handled) {
        m_throwing = true;
        m_exception = pending.exception;
    } else if (pending.returning) {
        m_returning = true;
        m_returnValue = pending.returnValue;
    } else if (pending.signal != NoSignal) {
        m_loopSignal = pending.signal;
    }
}

void FakeVimHandler::Private::runExCommands(const QList<ExCommand> &cmds)
{
    m_loopSignal = NoSignal;
    m_returning = false;
    m_throwing = false;
    m_finishing = false;
    int index = 0;
    while (index < cmds.size()) {
        execSequence(cmds, index, true);
        if (m_throwing)
            showMessage(MessageError, Tr::tr("Uncaught exception: %1").arg(m_exception));
        const bool finish = m_finishing;
        m_loopSignal = NoSignal; // a :break/:continue/:return/:throw outside a
        m_returning = false;     // loop, function or :try is ignored here
        m_throwing = false;
        m_finishing = false;
        if (finish)
            break; // :finish stops running the rest of the command list
        if (index < cmds.size()) {
            // A block terminator with no matching opener.
            showMessage(MessageError,
                        Tr::tr("%1 without matching :if").arg(cmds.at(index).cmd));
            ++index;
        }
    }
}

void FakeVimHandler::Private::collectFunction(const QList<ExCommand> &cmds,
    int &index, bool active, const ExCommand &header)
{
    // header.args is "Name(arg1, arg2, ...)". Gather the body up to the
    // matching :endfunction (functions do not nest in Vim).
    UserFunction fn;
    fn.vim9 = header.cmd == "def"; // :def uses bare-name args and Vim9 syntax
    static const QRegularExpression re(
        "^\\s*([A-Za-z_][A-Za-z0-9_:#]*)\\s*\\(([^)]*)\\)");
    const QRegularExpressionMatch m = re.match(header.args);
    QString name;
    if (m.hasMatch()) {
        name = m.captured(1);
        const QString params = m.captured(2).trimmed();
        const QStringList rawParams = params.isEmpty()
            ? QStringList() : params.split(QRegularExpression("\\s*,\\s*"));
        for (QString p : rawParams) {
            p = p.trimmed();
            if (p.startsWith("...")) {
                // Vim9 spells the variadic parameter "...name: type" and binds
                // the extra arguments as a list under that name.
                QString varName = p.mid(3).trimmed();
                const int typeColon = varName.indexOf(':');
                if (typeColon >= 0)
                    varName = varName.left(typeColon).trimmed();
                fn.varargName = varName;
                continue;
            }
            // A ":def" param can carry a ": type" and a "= default"; keep only
            // the name and (optional) default expression.
            QString def;
            const int eq = p.indexOf('=');
            if (eq >= 0) {
                def = p.mid(eq + 1).trimmed();
                p = p.left(eq).trimmed();
            }
            const int colon = p.indexOf(':');
            if (colon >= 0)
                p = p.left(colon).trimmed();
            fn.params.append(p);
            fn.defaults.append(def);
        }
    }

    // Vim9 ":def" nests (a nested def is a closure defined when the outer one
    // runs), so stop at the matching end rather than the first one. The inner
    // def/enddef pair stays in the body and is collected on execution.
    int depth = 0;
    while (index < cmds.size()) {
        const QString &cmd = cmds.at(index).cmd;
        if (isFunctionEnd(cmd)) {
            if (depth == 0)
                break;
            --depth;
        } else if (isFunctionStart(cmd)) {
            ++depth;
        }
        fn.body.append(cmds.at(index));
        ++index;
    }
    if (index < cmds.size())
        ++index; // consume :endfunction

    if (active && !name.isEmpty())
        g.userFunctions.insert(name, fn);
    else if (active)
        showMessage(MessageError, Tr::tr("Invalid function definition: %1").arg(header.args));
}

VimValue FakeVimHandler::Private::callUserFunction(const QString &name,
    const UserFunction &fn, const QList<VimValue> &args)
{
    // Bind arguments into a fresh local scope, run the body and return the
    // value from :return (or 0). Loop/return state is saved so nested and
    // recursive calls are independent.
    // :def binds parameters by bare name; legacy :function uses "a:name".
    const QString prefix = fn.vim9 ? QString() : QString("a:");
    QHash<QString, VimValue> frame;
    for (int i = 0; i < fn.params.size(); ++i) {
        VimValue v;
        if (i < args.size())
            v = args.at(i);
        else if (!fn.defaults.at(i).isEmpty())
            evaluateExpression(fn.defaults.at(i), &v, nullptr);
        frame.insert(prefix + fn.params.at(i), v);
    }
    // Extra (variadic) arguments: a:0 counts them, a:1.. name them and a:000
    // is the list of them.
    frame.insert("a:0", VimValue(qlonglong(qMax(0, args.size() - fn.params.size()))));
    QList<VimValue> varargs;
    for (int i = fn.params.size(); i < args.size(); ++i) {
        varargs.append(args.at(i));
        frame.insert("a:" + QString::number(i - fn.params.size() + 1), args.at(i));
    }
    frame.insert("a:000", VimValue::list(varargs));
    if (!fn.varargName.isEmpty())
        frame.insert(prefix + fn.varargName, VimValue::list(varargs));

    const LoopSignal savedSignal = m_loopSignal;
    const bool savedReturning = m_returning;
    const VimValue savedReturnValue = m_returnValue;
    const bool savedVim9 = m_vim9;
    m_loopSignal = NoSignal;
    m_returning = false;
    m_vim9 = fn.vim9; // Vim9 expression semantics inside a :def body

    m_localScopes.append(frame);
    m_callStack.append(name);
    int index = 0;
    execSequence(fn.body, index, true);
    m_callStack.removeLast();
    m_localScopes.removeLast();

    const VimValue result = m_returning ? m_returnValue : VimValue();
    m_loopSignal = savedSignal;
    m_returning = savedReturning;
    m_returnValue = savedReturnValue;
    m_vim9 = savedVim9;
    return result;
}

void FakeVimHandler::Private::handleExCommand(const QString &line0)
{
    QString line = line0; // Make sure we have a copy to prevent aliasing.

    if (line.endsWith('%')) {
        line.chop(1);
        int percent = line.toInt();
        setPosition(firstPositionInLine(percent * linesInDocument() / 100));
        clearMessage();
        return;
    }

    //qDebug() << "CMD: " << cmd;

    enterCommandMode(g.returnToMode);

    beginLargeEditBlock();
    QList<ExCommand> cmds;
    ExCommand cmd;
    while (parseExCommand(&line, &cmd))
        cmds.append(cmd);
    runExCommands(cmds);

    // if the last command closed the editor, we would crash here (:vs and then :on)
    if (!hasValidEditor())
        return;

    endEditBlock();

    if (isVisualMode())
        leaveVisualMode();
    leaveCurrentMode();
}

// Run an ex command line without touching the mode. ":" leaves insert or
// visual mode and puts the cursor where normal mode wants it, which is exactly
// what a "<Cmd>" mapping must not do.
void FakeVimHandler::Private::runExCommandLine(const QString &line0)
{
    QString line = m_vim9 ? vim9Statement(line0) : line0;
    beginLargeEditBlock();
    QList<ExCommand> cmds;
    ExCommand cmd;
    while (parseExCommand(&line, &cmd))
        cmds.append(cmd);
    runExCommands(cmds);
    if (!hasValidEditor())
        return;
    endEditBlock();
}

bool FakeVimHandler::Private::handleExCommandHelper(ExCommand &cmd)
{
    return handleExPluginCommand(cmd)
        || handleExGotoCommand(cmd)
        || handleExBangCommand(cmd)
        || handleExHistoryCommand(cmd)
        || handleExRegisterCommand(cmd)
        || handleExDelMarksCommand(cmd)
        || handleExYankDeleteCommand(cmd)
        || handleExChangeCommand(cmd)
        || handleExMoveCommand(cmd)
        || handleExJoinCommand(cmd)
        || handleExSilentCommand(cmd)
        || handleExModifierCommand(cmd)
        || handleExAutocmdCommand(cmd)
        || handleExAugroupCommand(cmd)
        || handleExDoAutocmdCommand(cmd)
        || handleExCommandDefCommand(cmd)
        || handleExSetFileTypeCommand(cmd)
        || handleExLetCommand(cmd)
        || handleExUnletCommand(cmd)
        || handleExCallCommand(cmd)
        || handleExExecuteCommand(cmd)
        || handleExMapCommand(cmd)
        || handleExMultiRepeatCommand(cmd)
        || handleExNohlsearchCommand(cmd)
        || handleExNormalCommand(cmd)
        || handleExReadCommand(cmd)
        || handleExUndoRedoCommand(cmd)
        || handleExSetCommand(cmd)
        || handleExShiftCommand(cmd)
        || handleExSortCommand(cmd)
        || handleExSourceCommand(cmd)
        || handleExImportCommand(cmd)
        || handleExSubstituteCommand(cmd)
        || handleExTabNextCommand(cmd)
        || handleExTabPreviousCommand(cmd)
        || handleExTagCommand(cmd)
        || handleExWriteCommand(cmd)
        || handleExEchoCommand(cmd)
        || handleExUserCommand(cmd);
}

bool FakeVimHandler::Private::handleExPluginCommand(const ExCommand &cmd)
{
    bool handled = false;
    int pos = m_cursor.position();
    commitCursor();
    q->handleExCommandRequested(&handled, cmd);
    //qDebug() << "HANDLER REQUEST: " << cmd.cmd << handled;
    if (handled && hasValidEditor()) {
        // The command ran while m_inFakeVim was set, so a cursor move it made
        // did not flag onCursorPositionChanged; force pullCursor to re-read it.
        // Visual mode keeps the pre-existing gated behavior: there commitCursor()
        // has extended the selection for display, which must not be pulled back.
        if (isNoVisualMode())
            m_cursorNeedsUpdate = true;
        pullCursor();
        if (m_cursor.position() != pos)
            recordJump(pos);
    }
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
        QChar c = characterAt(pos);
        if (c == other)
            ++level;
        else if (c == needle)
            --level;
        if (level == 0) {
            const int oldLine = cursorLine() - cursorLineOnScreen();
            // Making this unconditional feels better, but is not "vim like".
            if (oldLine != cursorLine() - cursorLineOnScreen())
                scrollToLine(cursorLine() - linesOnScreen() / 2);
            recordJump();
            setPosition(pos);
            setTargetColumn();
            return;
        }
    }
}

QTextCursor FakeVimHandler::Private::search(const SearchData &sd, int startPos, int count,
    bool showMessages)
{
    const QRegularExpression needleExp = vimPatternToQtPattern(sd.needle);

    if (!needleExp.isValid()) {
        if (showMessages) {
            QString error = needleExp.errorString();
            showMessage(MessageError, Tr::tr("Invalid regular expression: %1").arg(error));
        }
        if (sd.highlightMatches)
            highlightMatches(QString());
        return QTextCursor();
    }

    int repeat = count;
    const int pos = startPos + (sd.forward ? 1 : -1);

    QTextCursor tc;
    if (pos >= 0 && pos < document()->characterCount()) {
        tc = QTextCursor(document());
        tc.setPosition(pos);
        if (sd.forward && afterEndOfLine(document(), pos))
            tc.movePosition(Right);

        if (!tc.isNull()) {
            if (sd.forward)
                searchForward(&tc, needleExp, &repeat);
            else
                searchBackward(&tc, needleExp, &repeat);
        }
    }

    if (tc.isNull()) {
        if (s.wrapScan()) {
            tc = QTextCursor(document());
            tc.movePosition(sd.forward ? StartOfDocument : EndOfDocument);
            if (sd.forward)
                searchForward(&tc, needleExp, &repeat);
            else
                searchBackward(&tc, needleExp, &repeat);
            if (tc.isNull()) {
                if (showMessages) {
                    showMessage(MessageError,
                        Tr::tr("Pattern not found: %1").arg(sd.needle));
                }
            } else if (showMessages) {
                QString msg = sd.forward
                    ? Tr::tr("Search hit BOTTOM, continuing at TOP.")
                    : Tr::tr("Search hit TOP, continuing at BOTTOM.");
                showMessage(MessageWarning, msg);
            }
        } else if (showMessages) {
            QString msg = sd.forward
                ? Tr::tr("Search hit BOTTOM without match for: %1")
                : Tr::tr("Search hit TOP without match for: %1");
            showMessage(MessageError, msg.arg(sd.needle));
        }
    }

    if (sd.highlightMatches)
        highlightMatches(needleExp.pattern());

    return tc;
}

void FakeVimHandler::Private::search(const SearchData &sd, bool showMessages)
{
    const int oldLine = cursorLine() - cursorLineOnScreen();

    QTextCursor tc = search(sd, m_searchStartPosition, count(), showMessages);
    if (tc.isNull()) {
        tc = m_cursor;
        tc.setPosition(m_searchStartPosition);
    }

    if (isVisualMode()) {
        int d = tc.anchor() - tc.position();
        setPosition(tc.position() + d);
    } else {
        // Set Cursor. In contrast to the main editor we have the cursor
        // position before the anchor position.
        setAnchorAndPosition(tc.position(), tc.anchor());
    }

    // Making this unconditional feels better, but is not "vim like".
    if (oldLine != cursorLine() - cursorLineOnScreen())
        scrollToLine(cursorLine() - linesOnScreen() / 2);

    m_searchCursor = m_cursor;

    setTargetColumn();
}

bool FakeVimHandler::Private::searchNext(bool forward)
{
    SearchData sd;
    sd.needle = g.lastSearch;
    sd.forward = forward ? g.lastSearchForward : !g.lastSearchForward;
    sd.highlightMatches = true;
    m_searchStartPosition = position();
    showMessage(MessageCommand, QLatin1Char(g.lastSearchForward ? '/' : '?') + sd.needle);
    recordJump();
    search(sd);
    return finishSearch();
}

void FakeVimHandler::Private::highlightMatches(const QString &needle)
{
    g.lastNeedle = needle;
    g.highlightsCleared = false;
    updateHighlights();
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine()
{
    g.movetype = MoveLineWise;
    moveToFirstNonBlankOnLine(&m_cursor);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLine(QTextCursor *tc)
{
    tc->setPosition(tc->block().position(), KeepAnchor);
    moveToNonBlankOnLine(tc);
}

void FakeVimHandler::Private::moveToFirstNonBlankOnLineVisually()
{
    moveToStartOfLineVisually();
    moveToNonBlankOnLine(&m_cursor);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToNonBlankOnLine(QTextCursor *tc)
{
    const QTextBlock block = tc->block();
    const int maxPos = block.position() + block.length() - 1;
    int i = tc->position();
    while (characterAt(i).isSpace() && i < maxPos)
        ++i;
    tc->setPosition(i, KeepAnchor);
}

void FakeVimHandler::Private::indentSelectedText(QChar typedChar)
{
    beginEditBlock();
    setTargetColumn();
    int beginLine = qMin(lineForPosition(position()), lineForPosition(anchor()));
    int endLine = qMax(lineForPosition(position()), lineForPosition(anchor()));

    Range range(anchor(), position(), g.rangemode);
    indentText(range, typedChar);

    setPosition(firstPositionInLine(beginLine));
    handleStartOfLine();
    setTargetColumn();
    setDotCommand("%1==", endLine - beginLine + 1);
    endEditBlock();

    const int lines = endLine - beginLine + 1;
    if (lines > 2)
        showMessage(MessageInfo, Tr::tr("%n lines indented.", nullptr, lines));
}

void FakeVimHandler::Private::indentText(const Range &range, QChar typedChar)
{
    int beginBlock = blockAt(range.beginPos).blockNumber();
    int endBlock = blockAt(range.endPos).blockNumber();
    if (beginBlock > endBlock)
        std::swap(beginBlock, endBlock);

    // Don't remember current indentation in last text insertion.
    const QString lastInsertion = m_buffer->lastInsertion;
    q->indentRegion(beginBlock, endBlock, typedChar);
    m_buffer->lastInsertion = lastInsertion;
}

bool FakeVimHandler::Private::isElectricCharacter(QChar c) const
{
    bool result = false;
    q->checkForElectricCharacter(&result, c);
    return result;
}

void FakeVimHandler::Private::shiftRegionRight(int repeat)
{
    int beginLine = lineForPosition(anchor());
    int endLine = lineForPosition(position());
    int targetPos = anchor();
    if (beginLine > endLine) {
        std::swap(beginLine, endLine);
        targetPos = position();
    }
    if (s.startOfLine())
        targetPos = firstPositionInLine(beginLine);

    const int sw = shiftWidth();
    g.movetype = MoveLineWise;
    beginEditBlock();
    QTextBlock block = document()->findBlockByLineNumber(beginLine - 1);
    while (block.isValid() && lineNumber(block) <= endLine) {
        const Column col = indentation(block.text());
        QTextCursor tc = m_cursor;
        tc.setPosition(block.position());
        if (col.physical > 0)
            tc.setPosition(tc.position() + col.physical, KeepAnchor);
        tc.insertText(tabExpand(col.logical + sw * repeat));
        block = block.next();
    }
    endEditBlock();

    setPosition(targetPos);
    handleStartOfLine();

    const int lines = endLine - beginLine + 1;
    if (lines > 2) {
        showMessage(MessageInfo,
            Tr::tr("%n lines %1ed %2 time.", nullptr, lines)
            .arg(repeat > 0 ? '>' : '<').arg(qAbs(repeat)));
    }
}

void FakeVimHandler::Private::shiftRegionLeft(int repeat)
{
    shiftRegionRight(-repeat);
}

void FakeVimHandler::Private::moveToTargetColumn()
{
    const QTextBlock &bl = block();
    //Column column = cursorColumn();
    //int logical = logical
    const int pos = lastPositionInLine(bl.blockNumber() + 1, false);
    if (m_targetColumn == -1) {
        setPosition(pos);
        return;
    }
    const int physical = bl.position() + logicalToPhysicalColumn(m_targetColumn, bl.text());
    //qDebug() << "CORRECTING COLUMN FROM: " << logical << "TO" << m_targetColumn;
    setPosition(qMin(pos, physical));
}

void FakeVimHandler::Private::setTargetColumn()
{
    m_targetColumn = logicalCursorColumn();
    m_visualTargetColumn = m_targetColumn;

    QTextCursor tc = m_cursor;
    tc.movePosition(StartOfLine);
    m_targetColumnWrapped = m_cursor.position() - tc.position();
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
    if (c.isLetterOrNumber() || c == '_')
        return 2;
    return c.isSpace() ? 0 : 1;
}

void FakeVimHandler::Private::miniBufferTextEdited(const QString &text, int cursorPos,
    int anchorPos)
{
    if (!isCommandLineMode()) {
        editor()->setFocus();
    } else if (text.isEmpty()) {
        // editing cancelled
        enterFakeVim();
        handleDefaultKey(Input(Qt::Key_Escape, Qt::NoModifier, QString()));
        leaveFakeVim();
        editor()->setFocus();
    } else {
        CommandBuffer &cmdBuf = (g.mode == ExMode) ? g.commandBuffer : g.searchBuffer;
        int pos = qMax(1, cursorPos);
        int anchor = anchorPos == -1 ? pos : qMax(1, anchorPos);
        QString buffer = text;
        // prepend prompt character if missing
        if (!buffer.startsWith(cmdBuf.prompt())) {
            buffer.prepend(cmdBuf.prompt());
            ++pos;
            ++anchor;
        }
        // update command/search buffer
        cmdBuf.setContents(buffer.mid(1), pos - 1, anchor - 1);
        if (pos != cursorPos || anchor != anchorPos || buffer != text)
            q->commandBufferChanged(buffer, pos, anchor, 0);
        // update search expression
        if (g.subsubmode == SearchSubSubMode) {
            updateFind(false);
            commitCursor();
        }
    }
}

void FakeVimHandler::Private::pullOrCreateBufferData()
{
    const QVariant data = document()->property("FakeVimSharedData");
    if (data.isValid()) {
        // FakeVimHandler has been already created for this document (e.g. in other split).
        m_buffer = data.value<BufferDataPtr>();
    } else {
        // FakeVimHandler has not been created for this document yet.
        m_buffer = BufferDataPtr(new BufferData);
        document()->setProperty("FakeVimSharedData", QVariant::fromValue(m_buffer));
    }

    if (editor()->hasFocus())
        m_buffer->currentHandler = this;
}

// Helper to parse a-z,A-Z,48-57,_
static int someInt(const QString &str)
{
    if (str.toInt())
        return str.toInt();
    if (!str.isEmpty())
        return str.at(0).unicode();
    return 0;
}

void FakeVimHandler::Private::setupCharClass()
{
    for (int i = 0; i < 256; ++i) {
        const QChar c = QLatin1Char(i);
        m_charClass[i] = c.isSpace() ? 0 : 1;
    }
    const QString conf = s.isKeyword();
    for (const QString &part : conf.split(',')) {
        if (part.contains('-')) {
            const int from = someInt(part.section('-', 0, 0));
            const int to = someInt(part.section('-', 1, 1));
            for (int i = qMax(0, from); i <= qMin(255, to); ++i)
                m_charClass[i] = 2;
        } else {
            m_charClass[qMin(255, someInt(part))] = 2;
        }
    }
}

void FakeVimHandler::Private::moveToBoundary(bool simple, bool forward)
{
    QTextCursor tc(document());
    tc.setPosition(position());
    if (forward ? tc.atBlockEnd() : tc.atBlockStart())
        return;

    QChar c = characterAt(tc.position() + (forward ? -1 : 1));
    int lastClass = tc.atStart() ? -1 : charClass(c, simple);
    QTextCursor::MoveOperation op = forward ? Right : Left;
    while (true) {
        c = characterAt(tc.position());
        int thisClass = charClass(c, simple);
        if (thisClass != lastClass || (forward ? tc.atBlockEnd() : tc.atBlockStart())) {
            if (tc != m_cursor)
                tc.movePosition(forward ? Left : Right);
            break;
        }
        lastClass = thisClass;
        tc.movePosition(op);
    }
    setPosition(tc.position());
}

void FakeVimHandler::Private::moveToNextBoundary(bool end, int count, bool simple, bool forward)
{
    int repeat = count;
    while (repeat > 0 && !(forward ? atDocumentEnd() : atDocumentStart())) {
        setPosition(position() + (forward ? 1 : -1));
        moveToBoundary(simple, forward);
        if (atBoundary(end, simple))
            --repeat;
    }
}

void FakeVimHandler::Private::moveToNextBoundaryStart(int count, bool simple, bool forward)
{
    moveToNextBoundary(false, count, simple, forward);
}

void FakeVimHandler::Private::moveToNextBoundaryEnd(int count, bool simple, bool forward)
{
    moveToNextBoundary(true, count, simple, forward);
}

void FakeVimHandler::Private::moveToBoundaryStart(int count, bool simple, bool forward)
{
    moveToNextBoundaryStart(atBoundary(false, simple) ? count - 1 : count, simple, forward);
}

void FakeVimHandler::Private::moveToBoundaryEnd(int count, bool simple, bool forward)
{
    moveToNextBoundaryEnd(atBoundary(true, simple) ? count - 1 : count, simple, forward);
}

void FakeVimHandler::Private::moveToNextWord(bool end, int count, bool simple, bool forward, bool emptyLines)
{
    int repeat = count;
    while (repeat > 0 && !(forward ? atDocumentEnd() : atDocumentStart())) {
        // Move by one code point. movePosition() steps over a surrogate pair
        // (e.g. an emoji) as a unit, so the position always advances; a raw
        // "position() + 1" would land inside the pair and stall, spinning
        // this loop forever (QTCREATORBUG-25873).
        m_cursor.movePosition(forward ? Right : Left, KeepAnchor);
        moveToBoundary(simple, forward);
        if (atWordBoundary(end, simple) && (emptyLines || !atEmptyLine()) )
            --repeat;
    }
}

void FakeVimHandler::Private::moveToNextWordStart(int count, bool simple, bool forward, bool emptyLines)
{
    g.movetype = MoveExclusive;
    moveToNextWord(false, count, simple, forward, emptyLines);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToNextWordEnd(int count, bool simple, bool forward, bool emptyLines)
{
    g.movetype = MoveInclusive;
    moveToNextWord(true, count, simple, forward, emptyLines);
    setTargetColumn();
}

void FakeVimHandler::Private::moveToWordStart(int count, bool simple, bool forward, bool emptyLines)
{
    moveToNextWordStart(atWordStart(simple) ? count - 1 : count, simple, forward, emptyLines);
}

void FakeVimHandler::Private::moveToWordEnd(int count, bool simple, bool forward, bool emptyLines)
{
    moveToNextWordEnd(atWordEnd(simple) ? count - 1 : count, simple, forward, emptyLines);
}

bool FakeVimHandler::Private::handleFfTt(const QString &key, bool repeats)
{
    int key0 = key.size() == 1 ? key.at(0).unicode() : 0;
    // g.subsubmode \in { 'f', 'F', 't', 'T' }
    bool forward = g.subsubdata.is('f') || g.subsubdata.is('t');
    bool exclusive =  g.subsubdata.is('t') || g.subsubdata.is('T');
    int repeat = count();
    int n = block().position() + (forward ? block().length() : - 1);
    const int d = forward ? 1 : -1;
    // FIXME: This also depends on whether 'cpositions' Vim option contains ';'.
    const int skip = (repeats && repeat == 1 && exclusive) ? d : 0;
    int pos = position() + d + skip;

    for (; repeat > 0 && (forward ? pos < n : pos > n); pos += d) {
        if (characterAt(pos).unicode() == key0)
            --repeat;
    }

    if (repeat == 0) {
        setPosition(pos - d - (exclusive ? d : 0));
        setTargetColumn();
        return true;
    }

    return false;
}

void FakeVimHandler::Private::moveToMatchingParanthesis()
{
    bool moved = false;
    bool forward = false;

    const int anc = anchor();
    QTextCursor tc = m_cursor;

    // If no known parenthesis symbol is under cursor find one on the current line after cursor.
    static const QString parenthesesChars("([{}])");
    while (!parenthesesChars.contains(characterAt(tc.position())) && !tc.atBlockEnd())
        tc.setPosition(tc.position() + 1);

    if (tc.atBlockEnd())
        tc = m_cursor;

    if (s.matchBracketsLikeVim()) {
        // Purely textual matching, ignoring syntax (strings, comments), as in
        // Vim (QTCREATORBUG-24172 follow-up); the default uses the editor's
        // syntax-aware matcher below.
        const int match = vimMatchingParenthesis(tc.position());
        if (match >= 0) {
            setAnchorAndPosition(anc, match);
            setTargetColumn();
        }
        return;
    }

    q->moveToMatchingParenthesis(&moved, &forward, &tc);
    if (moved) {
        if (forward)
            tc.movePosition(Left, KeepAnchor, 1);
        setAnchorAndPosition(anc, tc.position());
        setTargetColumn();
    }
}

int FakeVimHandler::Private::vimMatchingParenthesis(int pos) const
{
    static const QString openers("([{");
    static const QString closers(")]}");
    const QChar ch = characterAt(pos);
    const int open = openers.indexOf(ch);
    const int close = closers.indexOf(ch);
    if (open >= 0) {
        const QChar closer = closers.at(open);
        const int last = lastPositionInDocument();
        int depth = 1;
        for (int i = pos + 1; i <= last; ++i) {
            const QChar c = characterAt(i);
            if (c == ch)
                ++depth;
            else if (c == closer && --depth == 0)
                return i;
        }
    } else if (close >= 0) {
        const QChar opener = openers.at(close);
        int depth = 1;
        for (int i = pos - 1; i >= 0; --i) {
            const QChar c = characterAt(i);
            if (c == ch)
                ++depth;
            else if (c == opener && --depth == 0)
                return i;
        }
    }
    return -1;
}

int FakeVimHandler::Private::cursorLineOnScreen() const
{
    if (!editor())
        return 0;
    const QRect rect = EDITOR(cursorRect(m_cursor));
    return rect.height() > 0 ? rect.y() / rect.height() : 0;
}

int FakeVimHandler::Private::linesOnScreen() const
{
    if (!editor())
        return 1;
    const int h = EDITOR(cursorRect(m_cursor)).height();
    return h > 0 ? EDITOR(viewport()->height()) / h : 1;
}

int FakeVimHandler::Private::cursorLine() const
{
    return lineForPosition(position()) - 1;
}

int FakeVimHandler::Private::cursorBlockNumber() const
{
    return blockAt(qMin(anchor(), position())).blockNumber();
}

int FakeVimHandler::Private::physicalCursorColumn() const
{
    return position() - block().position();
}

int FakeVimHandler::Private::physicalToLogicalColumn
    (const int physical, const QString &line) const
{
    const int ts = tabStop();
    int p = 0;
    int logical = 0;
    while (p < physical) {
        QChar c = line.at(p);
        //if (c == ' ')
        //    ++logical;
        //else
        if (c == '\t')
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
    const int ts = tabStop();
    int physical = 0;
    for (int l = 0; l < logical && physical < line.size(); ++physical) {
        QChar c = line.at(physical);
        if (c == '\t')
            l += ts - l % ts;
        else
            ++l;
    }
    return physical;
}

int FakeVimHandler::Private::windowScrollOffset() const
{
    return qMin(static_cast<int>(s.scrollOff()), linesOnScreen() / 2);
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
    if (m_cursor.isNull())
        return 0;
    return document()->blockCount();
}

void FakeVimHandler::Private::scrollToLine(int line)
{
    // Don't scroll if the line is already at the top.
    updateFirstVisibleLine();
    if (line == m_firstVisibleLine)
        return;

    // The cursor moves below rely on ensureCursorVisible() scrolling minimally
    // to bring the target line to the top of the viewport. With "Center cursor
    // on scroll" enabled it would center the line instead, throwing off zz/zt/
    // zb and 'scrolloff' (QTCREATORBUG-15407, QTCREATORBUG-9516). Disable it for
    // the duration of the scrolling.
    // "Center cursor on scroll" only exists on the plain text editors, not on
    // QTextEdit.
    const auto setCenterOnScroll = [this](bool on) {
        if (m_plaintextedit)
            m_plaintextedit->setCenterOnScroll(on);
        else if (m_qcPlainTextEdit)
            m_qcPlainTextEdit->setCenterOnScroll(on);
    };
    bool wasCentering = false;
    if (m_plaintextedit)
        wasCentering = m_plaintextedit->centerOnScroll();
    else if (m_qcPlainTextEdit)
        wasCentering = m_qcPlainTextEdit->centerOnScroll();
    if (wasCentering)
        setCenterOnScroll(false);

    const QTextCursor tc = m_cursor;

    QTextCursor tc2 = tc;
    tc2.setPosition(document()->lastBlock().position());
    EDITOR(setTextCursor(tc2));
    EDITOR(ensureCursorVisible());

    int offset = 0;
    const QTextBlock block = document()->findBlockByLineNumber(line);
    if (block.isValid()) {
        const int blockLineCount = block.layout()->lineCount();
        const int lineInBlock = line - block.firstLineNumber();
        if (0 <= lineInBlock && lineInBlock < blockLineCount) {
            QTextLine textLine = block.layout()->lineAt(lineInBlock);
            offset = textLine.textStart();
        } else {
//            QTC_CHECK(false);
        }
    }
    tc2.setPosition(block.position() + offset);
    EDITOR(setTextCursor(tc2));
    EDITOR(ensureCursorVisible());

    EDITOR(setTextCursor(tc));

    if (wasCentering)
        setCenterOnScroll(true);

    m_firstVisibleLine = line;
}

void FakeVimHandler::Private::updateFirstVisibleLine()
{
    const QTextCursor tc = EDITOR(cursorForPosition(QPoint(0,0)));
    m_firstVisibleLine = lineForPosition(tc.position()) - 1;
}

int FakeVimHandler::Private::firstVisibleLine() const
{
    return m_firstVisibleLine;
}

int FakeVimHandler::Private::lastVisibleLine() const
{
    const int line = m_firstVisibleLine + linesOnScreen();
    const QTextBlock block = document()->findBlockByLineNumber(line);
    return block.isValid() ? line : document()->lastBlock().firstLineNumber();
}

int FakeVimHandler::Private::lineOnTop(int count) const
{
    const int scrollOffset = qMax(count - 1, windowScrollOffset());
    const int line = firstVisibleLine();
    return line == 0 ? count - 1 : scrollOffset + line;
}

int FakeVimHandler::Private::lineOnBottom(int count) const
{
    const int scrollOffset = qMax(count - 1, windowScrollOffset());
    const int line = lastVisibleLine();
    return line >= document()->lastBlock().firstLineNumber() ? line - count + 1
                                                             : line - scrollOffset - 1;
}

void FakeVimHandler::Private::scrollUp(int count)
{
    scrollToLine(cursorLine() - cursorLineOnScreen() - count);
}

void FakeVimHandler::Private::updateScrollOffset()
{
    const int line = cursorLine();
    if (line < lineOnTop())
        scrollToLine(qMax(0, line - windowScrollOffset()));
    else if (line > lineOnBottom())
        scrollToLine(firstVisibleLine() + line - lineOnBottom());
}

void FakeVimHandler::Private::alignViewportToCursor(AlignmentFlag align, int line,
    bool moveToNonBlank)
{
    if (line > 0)
        setPosition(firstPositionInLine(line));
    if (moveToNonBlank)
        moveToFirstNonBlankOnLine();

    if (align == Qt::AlignTop)
        scrollUp(- cursorLineOnScreen());
    else if (align == Qt::AlignVCenter)
        scrollUp(linesOnScreen() / 2 - cursorLineOnScreen());
    else if (align == Qt::AlignBottom)
        scrollUp(linesOnScreen() - cursorLineOnScreen() - 1);
}

int FakeVimHandler::Private::lineToBlockNumber(int line) const
{
    return document()->findBlockByLineNumber(line).blockNumber();
}

void FakeVimHandler::Private::setCursorPosition(const CursorPosition &p)
{
    const int firstLine = firstVisibleLine();
    const int firstBlock = lineToBlockNumber(firstLine);
    const int lastBlock = lineToBlockNumber(firstLine + linesOnScreen() - 2);
    bool isLineVisible = firstBlock <= p.line && p.line <= lastBlock;
    setCursorPosition(&m_cursor, p);
    if (!isLineVisible)
        alignViewportToCursor(Qt::AlignVCenter);
}

void FakeVimHandler::Private::setCursorPosition(QTextCursor *tc, const CursorPosition &p)
{
    const int line = qMin(document()->blockCount() - 1, p.line);
    QTextBlock block = document()->findBlockByNumber(line);
    const int column = qMin(p.column, block.length() - 1);
    tc->setPosition(block.position() + column, KeepAnchor);
}

int FakeVimHandler::Private::lastPositionInDocument(bool ignoreMode) const
{
    return document()->characterCount()
        - (ignoreMode || isVisualMode() || isInsertMode() ? 1 : 2);
}

QString FakeVimHandler::Private::selectText(const Range &range) const
{
    QString contents;
    const QString lineEnd = range.rangemode == RangeBlockMode ? QString('\n') : QString();
    QTextCursor tc = m_cursor;
    transformText(range, tc,
        [&tc, &contents, &lineEnd]() { contents.append(tc.selection().toPlainText() + lineEnd); });
    return contents;
}

void FakeVimHandler::Private::yankText(const Range &range, int reg)
{
    const QString text = selectText(range);
    setRegister(reg, text, range.rangemode);

    // If register is not specified or " ...
    if (m_register == '"') {
        // with delete and change commands set register 1 (if text contains more lines) or
        // small delete register -
        if (g.submode == DeleteSubMode || g.submode == ChangeSubMode) {
            if (text.contains('\n'))
                setRegister('1', text, range.rangemode);
            else
                setRegister('-', text, range.rangemode);
        } else {
            // copy to yank register 0 too
            setRegister('0', text, range.rangemode);
        }
    } else if (m_register != '_') {
        // Always copy to " register too (except black hole register).
        setRegister('"', text, range.rangemode);
    }

    const int lines = blockAt(range.endPos).blockNumber()
        - blockAt(range.beginPos).blockNumber() + 1;
    if (lines > 2)
        showMessage(MessageInfo, Tr::tr("%n lines yanked.", nullptr, lines));
}

void FakeVimHandler::Private::transformText(
        const Range &range, QTextCursor &tc, const std::function<void()> &transform) const
{
    switch (range.rangemode) {
        case RangeCharMode: {
            // This can span multiple lines.
            tc.setPosition(range.beginPos, MoveAnchor);
            tc.setPosition(range.endPos, KeepAnchor);
            transform();
            tc.setPosition(range.beginPos);
            break;
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
            const int posAfter = tc.anchor();
            transform();
            tc.setPosition(posAfter);
            break;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            int beginColumn = columnAt(range.beginPos);
            int endColumn = columnAt(range.endPos);
            if (endColumn < beginColumn)
                std::swap(beginColumn, endColumn);
            if (range.rangemode == RangeBlockAndTailMode)
                endColumn = INT_MAX - 1;
            QTextBlock block = document()->findBlock(range.beginPos);
            const QTextBlock lastBlock = document()->findBlock(range.endPos);
            while (block.isValid() && block.position() <= lastBlock.position()) {
                int bCol = qMin(beginColumn, block.length() - 1);
                int eCol = qMin(endColumn + 1, block.length() - 1);
                tc.setPosition(block.position() + bCol, MoveAnchor);
                tc.setPosition(block.position() + eCol, KeepAnchor);
                transform();
                block = block.next();
            }
            tc.setPosition(range.beginPos);
            break;
        }
    }
}

void FakeVimHandler::Private::transformText(const Range &range, const Transformation &transform)
{
    beginEditBlock();
    transformText(range, m_cursor,
        [this, &transform] { m_cursor.insertText(transform(m_cursor.selection().toPlainText())); });
    endEditBlock();
    setTargetColumn();
}

void FakeVimHandler::Private::insertText(QTextCursor &tc, const QString &text)
{
  if (s.passKeys()) {
      if (tc.hasSelection() && text.isEmpty()) {
          QKeyEvent event(QEvent::KeyPress, Qt::Key_Delete, Qt::NoModifier, QString());
          passEventToEditor(event, tc);
      }

      for (QChar c : text) {
          QKeyEvent event(QEvent::KeyPress, -1, Qt::NoModifier, QString(c));
          passEventToEditor(event, tc);
      }
  } else {
      tc.insertText(text);
  }
}

void FakeVimHandler::Private::insertText(const Register &reg)
{
    if (reg.rangemode != RangeCharMode) {
        qWarning() << "WRONG INSERT MODE: " << reg.rangemode;
        return;
    }
    setAnchor();
    m_cursor.insertText(reg.contents);
    //dump("AFTER INSERT");
}

void FakeVimHandler::Private::removeText(const Range &range)
{
    transformText(range, [](const QString &) { return QString(); });
}

void FakeVimHandler::Private::downCase(const Range &range)
{
    transformText(range, [](const QString &text) { return text.toLower(); } );
}

void FakeVimHandler::Private::upCase(const Range &range)
{
    transformText(range, [](const QString &text) { return text.toUpper(); } );
}

void FakeVimHandler::Private::invertCase(const Range &range)
{
    transformText(range,
        [] (const QString &text) -> QString {
            QString result = text;
            for (int i = 0; i < result.size(); ++i) {
                const QChar c = result[i];
                result[i] = c.isUpper() ? c.toLower() : c.toUpper();
            }
            return result;
    });
}

void FakeVimHandler::Private::reflowText(const Range &range)
{
    const int textWidth = s.textWidth() > 0 ? int(s.textWidth()) : 80;

    const QTextBlock firstBlock = blockAt(qMin(range.beginPos, range.endPos));
    const QTextBlock lastBlock = blockAt(qMax(range.beginPos, range.endPos));

    QStringList lines;
    for (QTextBlock b = firstBlock; b.isValid(); b = b.next()) {
        lines.append(b.text());
        if (b == lastBlock)
            break;
    }

    // Rewrap paragraph by paragraph. Paragraphs are separated by blank lines,
    // which are kept as-is. Each paragraph is reflowed to the text width using
    // the indentation of its first line for every resulting line.
    QStringList result;
    for (int i = 0; i < lines.size(); ) {
        if (lines.at(i).trimmed().isEmpty()) {
            result.append(lines.at(i));
            ++i;
            continue;
        }

        const QString &firstLine = lines.at(i);
        int indentSize = 0;
        while (indentSize < firstLine.size() && firstLine.at(indentSize).isSpace())
            ++indentSize;
        const QString indent = firstLine.left(indentSize);

        QStringList words;
        for (; i < lines.size() && !lines.at(i).trimmed().isEmpty(); ++i)
            words += lines.at(i).simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);

        QString line = indent;
        bool hasWord = false;
        for (const QString &word : std::as_const(words)) {
            if (!hasWord) {
                line += word;
                hasWord = true;
            } else if (line.size() + 1 + word.size() <= textWidth) {
                line += QLatin1Char(' ') + word;
            } else {
                // A single word longer than the text width still gets its own
                // line rather than being split.
                result.append(line);
                line = indent + word;
            }
        }
        if (hasWord)
            result.append(line);
    }

    const int startPos = firstBlock.position();
    const int endPos = lastBlock.position() + lastBlock.length() - 1;

    QTextCursor tc = m_cursor;
    tc.setPosition(startPos);
    tc.setPosition(endPos, QTextCursor::KeepAnchor);
    tc.insertText(result.join(QLatin1Char('\n')));

    setPosition(startPos);
}

// g@: hand the region over to the function named by 'operatorfunc'. Vim sets
// the '[ and '] marks to the first and last character of the region and passes
// "char", "line" or "block" so the function knows how to read them.
void FakeVimHandler::Private::callOperatorFunc(const Range &range)
{
    const QString func = s.operatorFunc();
    if (func.isEmpty()) {
        showMessage(MessageError, Tr::tr("E774: 'operatorfunc' is empty"));
        return;
    }

    QString kind = "char";
    int first = range.beginPos;
    int last = qMax(range.beginPos, range.endPos - 1);
    if (range.rangemode == RangeBlockMode || range.rangemode == RangeBlockAndTailMode) {
        kind = "block";
    } else if (range.rangemode == RangeLineMode
               || range.rangemode == RangeLineModeExclusive) {
        kind = "line";
        // Linewise ranges keep positions inside the first and last line, while
        // the marks name the whole lines.
        first = blockAt(range.beginPos).position();
        const QTextBlock lastBlock = blockAt(range.endPos);
        last = lastBlock.position() + qMax(0, lastBlock.length() - 2);
    }

    setMark('[', CursorPosition(document(), first));
    setMark(']', CursorPosition(document(), last));

    // Vim leaves the cursor at the start of the region before the call.
    setPosition(first);
    setTargetColumn();

    VimValue result;
    QString error;
    if (!callFunction(func, {VimValue(kind)}, &result, &error))
        showMessage(MessageError, error);
}

void FakeVimHandler::Private::toggleComment(const Range &range)
{
    // Derive the comment markers from 'commentstring', so this and a script
    // reading &cms agree on how the buffer is commented.
    const QString cms = commentString();
    const int placeholder = cms.indexOf("%s");
    const QString commentString = (placeholder < 0 ? cms : cms.left(placeholder)).trimmed();
    const QString trailer = placeholder < 0 ? QString() : cms.mid(placeholder + 2).trimmed();

    transformText(range,
        [&commentString, &trailer] (const QString &text) -> QString {

        QStringList lines = text.split('\n');

        const QRegularExpression checkForComment("^\\s*"
                                                 + QRegularExpression::escape(commentString));

        const bool firstLineIsComment
                = !lines.empty() && lines.front().contains(checkForComment);

        for (auto& line : lines) {
            if (!line.isEmpty()) {
                if (firstLineIsComment) {
                    const bool hasSpaceAfterCommentString = line.contains(
                        QRegularExpression(checkForComment.pattern() + "\\s"));
                    const int sizeToReplace = hasSpaceAfterCommentString ? commentString.size() + 1
                                                                     : commentString.size();
                    line.replace(line.indexOf(commentString), sizeToReplace, "");
                    if (!trailer.isEmpty() && line.endsWith(trailer)) {
                        line.chop(trailer.size());
                        if (line.endsWith(' '))
                            line.chop(1);
                    }
                } else {
                    static const QRegularExpression regexp("[^\\s]");
                    const int indexOfFirstNonSpace = line.indexOf(regexp);
                    line = line.left(indexOfFirstNonSpace) + commentString  + " " + line.right(line.size() - indexOfFirstNonSpace);
                    if (!trailer.isEmpty())
                        line += ' ' + trailer;
                }
            }
        }

        return lines.size() == 1 ? lines.front() : lines.join("\n");
    });
}

void FakeVimHandler::Private::exchangeRange(const Range &range)
{
    if (g.exchangeRange) {
        pushUndoState(false);
        beginEditBlock();

        Range leftRange = *g.exchangeRange;
        Range rightRange = range;
        if (leftRange.beginPos > rightRange.beginPos)
            std::swap(leftRange, rightRange);

        // First replace the right range, then left one
        // If we did it the other way around, we would invalidate the positions
        // of the right range
        const QString rightText = selectText(rightRange);
        replaceText(rightRange, selectText(leftRange));
        replaceText(leftRange, rightText);

        g.exchangeRange.reset();

        endEditBlock();
    } else {
        g.exchangeRange = range;
    }
}

void FakeVimHandler::Private::replaceWithRegister(const Range &range)
{
    replaceText(range, registerContents(m_register));
}

void FakeVimHandler::Private::surroundCurrentRange(const Input &input, const QString &prefix)
{
    QString dotCommand;
    if (isVisualMode())
        dotCommand = visualDotCommand() + "S" + input.asChar();

    const bool wasVisualCharMode = isVisualCharMode();
    const bool wasVisualLineMode = isVisualLineMode();
    leaveVisualMode();

    if (dotCommand.isEmpty()) { // i.e. we came from normal mode
        dotCommand = dotCommandFromSubMode(g.submode)
                     + QLatin1Char(g.surroundUpperCaseS ? 'S' : 's')
                     + g.dotCommand + input.asChar();
    }

    if (wasVisualCharMode)
        setPosition(position() + 1);

    QString newFront, newBack;

    if (input.is('(') || input.is(')') || input.is('b')) {
        newFront = '(';
        newBack = ')';
    } else if (input.is('{') || input.is('}') || input.is('B')) {
        newFront = '{';
        newBack = '}';
    } else if (input.is('[') || input.is(']')) {
        newFront = '[';
        newBack = ']';
    } else if (input.is('<') || input.is('>') || input.is('t')) {
        newFront = '<';
        newBack = '>';
    } else if (input.is('"') || input.is('\'') || input.is('`')) {
        newFront = input.asChar();
        newBack = input.asChar();
    }

    if (g.surroundUpperCaseS || wasVisualLineMode) {
        // yS and cS add a new line before and after the surrounded text
        newFront += "\n";
        if (wasVisualLineMode)
            newBack += "\n";
        else
            newBack = "\n" + newBack;
    } else if (input.is('(') || input.is('{') || input.is('[') || input.is('[')) {
        // Opening characters add an extra space
        newFront = newFront + " ";
        newBack = " " + newBack;
    }


    if (!newFront.isEmpty()) {
        transformText(currentRange(), [&](QString text) -> QString {
            if (newFront == QChar())
                return text.mid(1, text.size() - 2);

            const QString newMiddle = (g.submode == ChangeSurroundingSubMode) ?
                                          text.mid(1, text.size() - 2) : text;

            return prefix + newFront + newMiddle + newBack;
        });
    }

    // yS, cS and VS also indent the surrounded text
    if (g.surroundUpperCaseS || wasVisualLineMode)
        replay("=a" + input.asChar());

    // Indenting has changed the dotCommand, so now set it back to the correct one
    g.dotCommand = dotCommand;
}

void FakeVimHandler::Private::replaceText(const Range &range, const QString &str)
{
    transformText(range, [&str](const QString &) { return str; } );
}

void FakeVimHandler::Private::pasteText(bool afterCursor)
{
    const QString text = registerContents(m_register);
    const RangeMode rangeMode = registerRangeMode(m_register);

    beginEditBlock();

    // In visual mode paste text only inside selection.
    bool pasteAfter = isVisualMode() ? false : afterCursor;

    if (isVisualMode())
        cutSelectedText(g.submode == ReplaceWithRegisterSubMode ? '-' : '"');

    switch (rangeMode) {
        case RangeCharMode: {
            m_targetColumn = 0;
            const int pos = position() + 1;
            if (pasteAfter && rightDist() > 0)
                moveRight();
            insertText(text.repeated(count()));
            if (text.contains('\n'))
                setPosition(pos);
            else
                moveLeft();
            break;
        }
        case RangeLineMode:
        case RangeLineModeExclusive: {
            QTextCursor tc = m_cursor;
            moveToStartOfLine();
            m_targetColumn = 0;
            bool lastLine = false;
            if (pasteAfter) {
                lastLine = document()->lastBlock() == this->block();
                if (lastLine) {
                    tc.movePosition(EndOfLine, MoveAnchor);
                    tc.insertBlock();
                }
                moveDown();
            }
            const int pos = position();
            if (lastLine)
                insertText(text.repeated(count()).left(text.size() * count() - 1));
            else
                insertText(text.repeated(count()));
            setPosition(pos);
            moveToFirstNonBlankOnLine();
            break;
        }
        case RangeBlockAndTailMode:
        case RangeBlockMode: {
            const int pos = position();
            if (pasteAfter && rightDist() > 0)
                moveRight();
            QTextCursor tc = m_cursor;
            const int col = tc.columnNumber();
            QTextBlock block = tc.block();
            const QStringList lines = text.split('\n');
            for (int i = 0; i < lines.size() - 1; ++i) {
                if (!block.isValid()) {
                    tc.movePosition(EndOfDocument);
                    tc.insertBlock();
                    block = tc.block();
                }

                // resize line
                int length = block.length();
                int begin = block.position();
                if (col >= length) {
                    tc.setPosition(begin + length - 1);
                    tc.insertText(QString(col - length + 1, ' '));
                } else {
                    tc.setPosition(begin + col);
                }

                // insert text
                const QString line = lines.at(i).repeated(count());
                tc.insertText(line);

                // next line
                block = block.next();
            }
            setPosition(pos);
            if (pasteAfter)
                moveRight();
            break;
        }
    }

    endEditBlock();
}

void FakeVimHandler::Private::cutSelectedText(int reg)
{
    pushUndoState();

    bool visualMode = isVisualMode();
    leaveVisualMode();

    Range range = currentRange();
    if (visualMode && g.rangemode == RangeCharMode)
        ++range.endPos;

    if (!reg)
        reg = m_register;

    g.submode = DeleteSubMode;
    yankText(range, reg);
    removeText(range);
    g.submode = NoSubMode;

    if (g.rangemode == RangeLineMode)
        handleStartOfLine();
    else if (g.rangemode == RangeBlockMode)
        setPosition(qMin(position(), anchor()));
}

void FakeVimHandler::Private::joinLines(int count, bool preserveSpace)
{
    int pos = position();
    const int blockNumber = m_cursor.blockNumber();

    const QString currentLine = lineContents(blockNumber + 1);
    static const QRegularExpression cppStyleRegexp("^\\s*\\/\\/");
    static const QRegularExpression cStyleRegexp("^\\s*\\/?\\*");
    static const QRegularExpression pythonStyleRegexp("^\\s*#");
    const bool startingLineIsComment
            = currentLine.contains(cppStyleRegexp)
              || currentLine.contains(cStyleRegexp)
              || currentLine.contains(pythonStyleRegexp);

    for (int i = qMax(count - 2, 0); i >= 0 && blockNumber < document()->blockCount(); --i) {
        moveBehindEndOfLine();
        pos = position();
        setAnchor();
        moveRight();
        if (preserveSpace) {
            removeText(currentRange());
        } else {
            while (characterAtCursor() == ' ' || characterAtCursor() == '\t')
                moveRight();

            // If the line we started from is a comment, remove the comment string from the next line
            if (startingLineIsComment && s.formatOptions().contains('f')) {
                if (characterAtCursor() == '/' && characterAt(position() + 1) == '/')
                    moveRight(2);
                else if (characterAtCursor() == '*' || characterAtCursor() == '#')
                    moveRight(1);

                if (characterAtCursor() == ' ')
                    moveRight();
            }

            m_cursor.insertText(QString(' '));
        }
    }
    setPosition(pos);
}

void FakeVimHandler::Private::insertNewLine()
{
    if (m_buffer->editBlockLevel <= 1 && s.passKeys()) {
        // Leaving an untouched auto-indented line clears its indentation.
        clearUntouchedAutoIndentation();
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\n");
        if (passEventToEditor(event, m_cursor)) {
            // The editor may auto-indent the freshly opened line itself. Remember
            // it so leaving it untouched removes that indentation (QTCREATORBUG-15009).
            const QString text = block().text();
            if (!text.isEmpty() && text.trimmed().isEmpty())
                m_autoIndentBlock = block().blockNumber();
            return;
        }
    }

    // Leaving an untouched auto-indented line clears its indentation.
    clearUntouchedAutoIndentation();
    insertText(QString("\n"));
    insertAutomaticIndentation(true);
}

bool FakeVimHandler::Private::handleInsertInEditor(const Input &input)
{
    if (m_buffer->editBlockLevel > 0 || !s.passKeys())
        return false;

    joinPreviousEditBlock();

    QKeyEvent event(QEvent::KeyPress, input.key(), input.modifiers(), input.text());
    setAnchor();
    if (!passEventToEditor(event, m_cursor))
        return !hasValidEditor(); // Mark event as handled if it has destroyed editor.

    endEditBlock();

    setTargetColumn();

    return true;
}

bool FakeVimHandler::Private::passEventToEditor(QEvent &event, QTextCursor &tc)
{
    removeEventFilter();
    q->requestDisableBlockSelection();

    setThinCursor();
    EDITOR(setTextCursor(tc));

    bool accepted = QApplication::sendEvent(editor(), &event);
    if (!hasValidEditor())
        return false;

    if (accepted)
        tc = editorCursor();

    return accepted;
}

QString FakeVimHandler::Private::lineContents(int line) const
{
    return document()->findBlockByLineNumber(line - 1).text();
}

QString FakeVimHandler::Private::textAt(int from, int to) const
{
    QTextCursor tc(document());
    tc.setPosition(from);
    tc.setPosition(to, KeepAnchor);
    return tc.selectedText().replace(ParagraphSeparator, '\n');
}

void FakeVimHandler::Private::setLineContents(int line, const QString &contents)
{
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    QTextCursor tc = m_cursor;
    const int begin = block.position();
    const int len = block.length();
    tc.setPosition(begin);
    tc.setPosition(begin + len - 1, KeepAnchor);
    tc.insertText(contents);
}

int FakeVimHandler::Private::blockBoundary(const QString &left,
    const QString &right, bool closing, int count) const
{
    const QString &begin = closing ? left : right;
    const QString &end = closing ? right : left;

    // shift cursor if it is already on opening/closing string
    QTextCursor tc1 = m_cursor;
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
            tc1 = m_cursor;
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

int FakeVimHandler::Private::lineNumber(const QTextBlock &block) const
{
    if (block.isVisible())
        return block.firstLineNumber() + 1;

    // Folded block has line number of the nearest previous visible line.
    QTextBlock block2 = block;
    while (block2.isValid() && !block2.isVisible())
        block2 = block2.previous();
    return block2.firstLineNumber() + 1;
}

int FakeVimHandler::Private::columnAt(int pos) const
{
    return pos - blockAt(pos).position();
}

int FakeVimHandler::Private::blockNumberAt(int pos) const
{
    return blockAt(pos).blockNumber();
}

QTextBlock FakeVimHandler::Private::blockAt(int pos) const
{
    return document()->findBlock(pos);
}

QTextBlock FakeVimHandler::Private::nextLine(const QTextBlock &block) const
{
    return blockAt(block.position() + block.length());
}

QTextBlock FakeVimHandler::Private::previousLine(const QTextBlock &block) const
{
    return blockAt(block.position() - 1);
}

int FakeVimHandler::Private::firstPositionInLine(int line, bool onlyVisibleLines) const
{
    QTextBlock block = onlyVisibleLines ? document()->findBlockByLineNumber(line - 1)
        : document()->findBlockByNumber(line - 1);
    return block.position();
}

int FakeVimHandler::Private::lastPositionInLine(int line, bool onlyVisibleLines) const
{
    QTextBlock block;
    if (onlyVisibleLines) {
        block = document()->findBlockByLineNumber(line - 1);
        // respect folds and wrapped lines
        do {
            block = nextLine(block);
        } while (block.isValid() && !block.isVisible());
        if (block.isValid()) {
            if (line > 0)
                block = block.previous();
        } else {
            block = document()->lastBlock();
        }
    } else {
        block = document()->findBlockByNumber(line - 1);
    }

    const int position = block.position() + block.length() - 1;
    if (block.length() > 1 && !isVisualMode() && !isInsertMode())
        return position - 1;
    return position;
}

int FakeVimHandler::Private::lineForPosition(int pos) const
{
    const QTextBlock block = blockAt(pos);
    if (!block.isValid())
        return 0;
    const int positionInBlock = pos - block.position();
    const int lineNumberInBlock = block.layout()->lineForTextPosition(positionInBlock).lineNumber();
    return block.firstLineNumber() + lineNumberInBlock + 1;
}

void FakeVimHandler::Private::toggleVisualMode(VisualMode visualMode)
{
    if (visualMode == g.visualMode) {
        leaveVisualMode();
    } else {
        m_positionPastEnd = false;
        m_anchorPastEnd = false;
        g.visualMode = visualMode;
        m_buffer->lastVisualMode = visualMode;
    }
}

void FakeVimHandler::Private::leaveVisualMode()
{
    if (!isVisualMode())
        return;

    if (isVisualLineMode()) {
        g.rangemode = RangeLineMode;
        g.movetype = MoveLineWise;
    } else if (isVisualCharMode()) {
        g.rangemode = RangeCharMode;
        g.movetype = MoveInclusive;
    } else if (isVisualBlockMode()) {
        g.rangemode = m_visualTargetColumn == -1 ? RangeBlockAndTailMode : RangeBlockMode;
        g.movetype = MoveInclusive;
    }

    g.visualMode = NoVisualMode;
}

void FakeVimHandler::Private::saveLastVisualMode()
{
    if (isVisualMode() && g.mode == CommandMode && g.submode == NoSubMode) {
        setMark('<', markLessPosition());
        setMark('>', markGreaterPosition());
        m_buffer->lastVisualModeInverted = anchor() > position();
        m_buffer->lastVisualMode = g.visualMode;
    }
}

QWidget *FakeVimHandler::Private::editor() const
{
    if (m_textedit)
        return static_cast<QWidget *>(m_textedit);
    if (m_plaintextedit)
        return static_cast<QWidget *>(m_plaintextedit);
    return static_cast<QWidget *>(m_qcPlainTextEdit);
}

void FakeVimHandler::Private::joinPreviousEditBlock()
{
    UNDO_DEBUG("JOIN");
    if (m_buffer->breakEditBlock) {
        beginEditBlock();
        QTextCursor tc(m_cursor);
        tc.setPosition(tc.position());
        tc.beginEditBlock();
        tc.insertText("X");
        tc.deletePreviousChar();
        tc.endEditBlock();
        m_buffer->breakEditBlock = false;
    } else {
        if (m_buffer->editBlockLevel == 0 && !m_buffer->undo.empty())
            m_buffer->undoState = m_buffer->undo.pop();
        beginEditBlock();
    }
}

void FakeVimHandler::Private::beginEditBlock(bool largeEditBlock)
{
    UNDO_DEBUG("BEGIN EDIT BLOCK" << m_buffer->editBlockLevel + 1);
    if (!largeEditBlock && !m_buffer->undoState.isValid())
        pushUndoState(false);
    if (m_buffer->editBlockLevel == 0)
        m_buffer->breakEditBlock = true;
    ++m_buffer->editBlockLevel;
}

void FakeVimHandler::Private::endEditBlock()
{
    UNDO_DEBUG("END EDIT BLOCK" << m_buffer->editBlockLevel);
    if (m_buffer->editBlockLevel <= 0) {
        qWarning("beginEditBlock() not called before endEditBlock()!");
        return;
    }
    --m_buffer->editBlockLevel;
    if (m_buffer->editBlockLevel == 0 && m_buffer->undoState.isValid()) {
        m_buffer->undo.push(m_buffer->undoState);
        m_buffer->undoState = State();
    }
    if (m_buffer->editBlockLevel == 0)
        m_buffer->breakEditBlock = false;
}

void FakeVimHandler::Private::onContentsChanged(int position, int charsRemoved, int charsAdded)
{
    // Record inserted and deleted text in insert mode.
    if (isInsertMode() && (charsAdded > 0 || charsRemoved > 0) && canModifyBufferData()) {
        BufferData::InsertState &insertState = m_buffer->insertState;
        const int oldPosition = insertState.pos2;
        if (!isInsertStateValid()) {
            insertState.pos1 = oldPosition;
            g.dotCommand = "i";
            resetCount();
        }
        int indentation = 0;
        bool changedAtEnd = false;
        // Ignore changes outside inserted text (e.g. renaming other occurrences of a variable).
        if (position + charsRemoved >= insertState.pos1 && position <= insertState.pos2) {
            if (charsRemoved > 0) {
                // Assume that in a manual edit operation a text can be removed only
                // in front of cursor (<DELETE>) or behind it (<BACKSPACE>).

                // If the recorded amount of backspace/delete keys doesn't correspond with
                // number of removed characters, assume that the document has been changed
                // externally and invalidate current insert state.

                const bool wholeDocumentChanged =
                        charsRemoved > 1
                        && charsAdded > 0
                        && charsAdded + 1 == document()->characterCount();

                if (position < insertState.pos1) {
                    // <BACKSPACE>
                    const int backspaceCount = insertState.pos1 - position;
                    const QString inserted = textAt(position, position + charsAdded);
                    const QString unified = inserted.startsWith('\n') ? inserted.mid(1) : inserted;
                    changedAtEnd = unified.startsWith(insertState.textBeforeCursor);

                    int indentNew = 0;
                    for (int i = 0, end = unified.size(); i < end; ++i) {
                        if (unified.at(i) != ' ')
                            break;
                        ++indentNew;
                    }
                    int indentOld = 0;
                    for (int i = 0, end = insertState.textBeforeCursor.size(); i < end; ++i) {
                        if (insertState.textBeforeCursor.at(i) != ' ')
                            break;
                        --indentOld;
                    }
                    indentation = indentNew + indentOld;

                    if ((backspaceCount != charsRemoved && indentation == 0 && !changedAtEnd)
                            || (oldPosition == charsRemoved && wholeDocumentChanged)) {
                        invalidateInsertState();
                    } else {
                        const QString removed = insertState.textBeforeCursor.right(
                                    qMax(backspaceCount, charsRemoved));
                        if (indentation != 0 || changedAtEnd) {
                            // automatic indent by electric chars / skipping of automatic inserted
                            insertState.pos1 = position + backspaceCount + indentation;
                            insertState.pos2 = position + charsAdded;
                        } else if ( !inserted.endsWith(removed) ) {
                            insertState.backspaces += backspaceCount;
                            insertState.pos1 = position;
                            insertState.pos2 = qMax(position, insertState.pos2 - backspaceCount);
                        } // Ignore backspaces if same text was just inserted.
                    }
                } else if (position + charsRemoved > insertState.pos2) {
                    // <DELETE>
                    const int deleteCount = position + charsRemoved - insertState.pos2;
                    if (deleteCount != charsRemoved || (oldPosition == 0 && wholeDocumentChanged))
                        invalidateInsertState();
                    else
                        insertState.deletes += deleteCount;
                }
            } else if (charsAdded > 0 && insertState.insertingSpaces) {
                for (int i = position; i < position + charsAdded; ++i) {
                    const QChar c = characterAt(i);
                    if (c.unicode() == ' ' || c.unicode() == '\t')
                        insertState.spaces.insert(i);
                }
            }

            const int newPosition = position + charsAdded;
            if (indentation == 0 && !changedAtEnd) // (un)indented has pos2 set correctly already
                insertState.pos2 = qMax(insertState.pos2 + charsAdded - charsRemoved, newPosition);
            insertState.textBeforeCursor = textAt(block().position(), newPosition);
        }
    }

    if (!m_highlighted.isEmpty())
        q->highlightMatches(m_highlighted);

    if (charsAdded > 0 || charsRemoved > 0)
        queueChangeAutocmds(true, false);
}

void FakeVimHandler::Private::onCursorPositionChanged()
{
    if (!m_inFakeVim) {
        m_cursorNeedsUpdate = true;

        // Selecting text with mouse disables the thick cursor so it's more obvious
        // that extra character under cursor is not selected when moving text around or
        // making operations on text outside FakeVim mode.
        setThinCursor(g.mode == InsertMode || editorCursor().hasSelection());
    }

    queueChangeAutocmds(false, true);
}

void FakeVimHandler::Private::onUndoCommandAdded()
{
    if (!canModifyBufferData())
        return;

    // Undo commands removed?
    UNDO_DEBUG("Undo added" << "previous: REV" << m_buffer->lastRevision);
    if (m_buffer->lastRevision >= revision()) {
        UNDO_DEBUG("UNDO REMOVED!");
        const int removed = m_buffer->lastRevision - revision();
        for (int i = m_buffer->undo.size() - 1; i >= 0; --i) {
            if ((m_buffer->undo[i].revision -= removed) < 0) {
                m_buffer->undo.remove(0, i + 1);
                break;
            }
        }
    }

    m_buffer->redo.clear();
    // External change while FakeVim disabled.
    if (m_buffer->editBlockLevel == 0 && !m_buffer->undo.isEmpty() && !isInsertMode())
        m_buffer->undo.push(State());
}

void FakeVimHandler::Private::onInputTimeout()
{
    enterFakeVim();
    EventResult result = handleKey(Input());
    leaveFakeVim(result);
}

void FakeVimHandler::Private::onFixCursorTimeout()
{
    if (editor())
        fixExternalCursorPosition(editor()->hasFocus() && !isCommandLineMode());
}

void FakeVimHandler::Private::queueChangeAutocmds(bool textChanged, bool cursorMoved)
{
    if (g.autoCommands.isEmpty())
        return; // do not run a timer per keystroke for nothing
    // Insert mode has its own pair of events.
    const bool insert = isInsertMode();
    if (textChanged) {
        if (insert)
            m_pendingTextChangedI = true;
        else
            m_pendingTextChanged = true;
    }
    if (cursorMoved) {
        if (insert)
            m_pendingCursorMovedI = true;
        else
            m_pendingCursorMoved = true;
    }
    m_autocmdTimer.start();
}

void FakeVimHandler::Private::onAutocmdTimeout()
{
    const bool textChanged = m_pendingTextChanged;
    const bool textChangedI = m_pendingTextChangedI;
    const bool cursorMoved = m_pendingCursorMoved;
    const bool cursorMovedI = m_pendingCursorMovedI;
    m_pendingTextChanged = false;
    m_pendingTextChangedI = false;
    m_pendingCursorMoved = false;
    m_pendingCursorMovedI = false;

    if (textChanged)
        triggerAutocmd("TextChanged");
    if (textChangedI)
        triggerAutocmd("TextChangedI");
    if (cursorMoved)
        triggerAutocmd("CursorMoved");
    if (cursorMovedI)
        triggerAutocmd("CursorMovedI");
}

bool FakeVimHandler::Private::isPassthroughKey(const Input &input) const
{
    const auto modeIt = g.mappings.constFind(currentModeCode());
    if (modeIt == g.mappings.constEnd())
        return false;
    const auto it = modeIt->constFind(input);
    if (it == modeIt->constEnd())
        return false;
    const Inputs &rhs = it->value();
    return rhs.size() == 1 && rhs.first() == Pass;
}

char FakeVimHandler::Private::currentModeCode() const
{
    if (g.mode == ExMode)
        return 'c';
    else if (isVisualMode())
        return 'v';
    else if (isOperatorPending())
        return 'o';
    else if (g.mode == CommandMode)
        return 'n';
    else if (g.submode != NoSubMode)
        return ' ';
    else
        return 'i';
}

void FakeVimHandler::Private::undoRedo(bool undo)
{
    UNDO_DEBUG((undo ? "UNDO" : "REDO"));

    // FIXME: That's only an approximaxtion. The real solution might
    // be to store marks and old userData with QTextBlock setUserData
    // and retrieve them afterward.
    QStack<State> &stack = undo ? m_buffer->undo : m_buffer->redo;
    QStack<State> &stack2 = undo ? m_buffer->redo : m_buffer->undo;

    State state = m_buffer->undoState.isValid() ? m_buffer->undoState
                                        : !stack.empty() ? stack.pop() : State();

    CursorPosition lastPos(m_cursor);
    if (undo ? !document()->isUndoAvailable() : !document()->isRedoAvailable()) {
        const QString msg = undo ? Tr::tr("Already at oldest change.")
            : Tr::tr("Already at newest change.");
        showMessage(MessageInfo, msg);
        UNDO_DEBUG(msg);
        return;
    }
    clearMessage();

    ++m_buffer->editBlockLevel;

    // Do undo/redo [count] times to reach previous revision.
    const int previousRevision = revision();
    if (undo) {
        do {
            EDITOR(undo());
        } while (document()->isUndoAvailable() && state.revision >= 0 && state.revision < revision());
    } else {
        do {
            EDITOR(redo());
        } while (document()->isRedoAvailable() && state.revision > revision());
    }

    --m_buffer->editBlockLevel;

    if (state.isValid()) {
        Marks marks = m_buffer->marks;
        marks.swap(state.marks);
        updateMarks(marks);
        m_buffer->lastVisualMode = state.lastVisualMode;
        m_buffer->lastVisualModeInverted = state.lastVisualModeInverted;
        setMark('.', state.position);
        setMark('\'', lastPos);
        setMark('`', lastPos);
        setCursorPosition(state.position);
        setAnchor();
        state.revision = previousRevision;
    } else {
        updateFirstVisibleLine();
        pullCursor();
    }
    stack2.push(state);

    setTargetColumn();
    if (atEndOfLine())
        moveLeft();

    UNDO_DEBUG((undo ? "UNDONE" : "REDONE"));
}

void FakeVimHandler::Private::undo()
{
    undoRedo(true);
}

void FakeVimHandler::Private::redo()
{
    undoRedo(false);
}

void FakeVimHandler::Private::updateCursorShape()
{
    setThinCursor(
        g.mode == InsertMode
        || isVisualLineMode()
        || isVisualBlockMode()
        || isCommandLineMode()
        || !editor()->hasFocus());
}

void FakeVimHandler::Private::setThinCursor(bool enable)
{
    EDITOR(setOverwriteMode(!enable));
}

bool FakeVimHandler::Private::hasThinCursor() const
{
    return !EDITOR(overwriteMode());
}

void FakeVimHandler::Private::enterReplaceMode()
{
    m_replacedChars.clear();
    enterInsertOrReplaceMode(ReplaceMode);
}

void FakeVimHandler::Private::enterInsertMode()
{
    enterInsertOrReplaceMode(InsertMode);
}

void FakeVimHandler::Private::enterInsertOrReplaceMode(Mode mode)
{
    if (mode != InsertMode && mode != ReplaceMode) {
        qWarning("Unexpected mode");
        return;
    }
    if (g.mode == mode)
        return;

    g.mode = mode;

    if (g.returnToMode == mode) {
        // Returning to insert mode after <C-O>.
        clearCurrentMode();
        moveToTargetColumn();
        invalidateInsertState();
    } else {
        // Entering insert mode from command mode.
        if (mode == InsertMode) {
            // m_targetColumn shouldn't be -1 (end of line).
            if (m_targetColumn == -1)
                setTargetColumn();
        }

        g.submode = NoSubMode;
        g.subsubmode = NoSubSubMode;
        g.returnToMode = mode;
        clearLastInsertion();
    }

    q->modeChanged(isInsertMode());
    triggerAutocmd("InsertEnter");
}

void FakeVimHandler::Private::enterVisualInsertMode(QChar command)
{
    if (isVisualBlockMode()) {
        bool append = command == 'A';
        bool change = command == 's' || command == 'c';

        leaveVisualMode();

        const CursorPosition lastAnchor = markLessPosition();
        const CursorPosition lastPosition = markGreaterPosition();
        CursorPosition pos(lastAnchor.line,
            append ? qMax(lastPosition.column, lastAnchor.column) + 1
                   : qMin(lastPosition.column, lastAnchor.column));

        if (append) {
            m_visualBlockInsert = m_visualTargetColumn == -1 ? AppendToEndOfLineBlockInsertMode
                                                             : AppendBlockInsertMode;
        } else if (change) {
            m_visualBlockInsert = ChangeBlockInsertMode;
            beginEditBlock();
            cutSelectedText();
            endEditBlock();
        } else {
            m_visualBlockInsert = InsertBlockInsertMode;
        }

        setCursorPosition(pos);
        if (m_visualBlockInsert == AppendToEndOfLineBlockInsertMode)
            moveBehindEndOfLine();
    } else {
        m_visualBlockInsert = NoneBlockInsertMode;
        leaveVisualMode();
        if (command == 'I') {
            if (lineForPosition(anchor()) <= lineForPosition(position())) {
                setPosition(qMin(anchor(), position()));
                moveToStartOfLine();
            }
        } else if (command == 'A') {
            if (lineForPosition(anchor()) <= lineForPosition(position())) {
                setPosition(position());
                moveRight(qMin(rightDist(), 1));
            } else {
                setPosition(anchor());
                moveToStartOfLine();
            }
        }
    }

    setAnchor();
    if (m_visualBlockInsert != ChangeBlockInsertMode)
        breakEditBlock();
    enterInsertMode();
}

void FakeVimHandler::Private::enterCommandMode(Mode returnToMode)
{
    if (g.isRecording && isCommandLineMode())
        record(Input(Key_Escape, NoModifier));

    if (isNoVisualMode()) {
        if (atEndOfLine()) {
            m_cursor.movePosition(Left, KeepAnchor);
            if (m_targetColumn != -1)
                setTargetColumn();
        }
        setAnchor();
    }

    g.mode = CommandMode;
    clearCurrentMode();
    g.returnToMode = returnToMode;
    m_positionPastEnd = false;
    m_anchorPastEnd = false;

    q->modeChanged(isInsertMode());
}

void FakeVimHandler::Private::enterExMode(const QString &contents)
{
    g.currentMessage.clear();
    g.commandBuffer.clear();
    if (isVisualMode())
        g.commandBuffer.setContents(QString("'<,'>") + contents, contents.size() + 5);
    else
        g.commandBuffer.setContents(contents, contents.size());
    g.mode = ExMode;
    g.submode = NoSubMode;
    g.subsubmode = NoSubSubMode;
    unfocus();

    q->modeChanged(isInsertMode());
}

void FakeVimHandler::Private::recordJump(int position)
{
    CursorPosition pos = position >= 0 ? CursorPosition(document(), position)
                                       : CursorPosition(m_cursor);
    setMark('\'', pos);
    setMark('`', pos);
    if (m_buffer->jumpListUndo.isEmpty() || m_buffer->jumpListUndo.top() != pos)
        m_buffer->jumpListUndo.push(pos);
    m_buffer->jumpListRedo.clear();
    UNDO_DEBUG("jumps: " << m_buffer->jumpListUndo);
}

void FakeVimHandler::Private::jump(int distance)
{
    QStack<CursorPosition> &from = (distance > 0) ? m_buffer->jumpListRedo : m_buffer->jumpListUndo;
    QStack<CursorPosition> &to   = (distance > 0) ? m_buffer->jumpListUndo : m_buffer->jumpListRedo;
    int len = qMin(qAbs(distance), from.size());
    CursorPosition m(m_cursor);
    setMark('\'', m);
    setMark('`', m);
    for (int i = 0; i < len; ++i) {
        to.push(m);
        setCursorPosition(from.top());
        from.pop();
    }
    setTargetColumn();

    // The buffer-local jump list ends at the position where this file was
    // entered; continue in Qt Creator's global navigation history so a jump
    // that crossed files (e.g. Follow Symbol Under Cursor) can be undone with
    // CTRL-O and redone with CTRL-I (QTCREATORBUG-12114).
    const int remaining = qAbs(distance) - len;
    if (remaining > 0)
        q->navigateHistoryRequested(distance > 0 ? remaining : -remaining);
}

Column FakeVimHandler::Private::indentation(const QString &line) const
{
    int ts = tabStop();
    int physical = 0;
    int logical = 0;
    int n = line.size();
    while (physical < n) {
        QChar c = line.at(physical);
        if (c == ' ')
            ++logical;
        else if (c == '\t')
            logical += ts - logical % ts;
        else
            break;
        ++physical;
    }
    return Column(physical, logical);
}

QString FakeVimHandler::Private::tabExpand(int n) const
{
    int ts = tabStop();
    if (expandTab() || ts < 1)
        return QString(n, ' ');
    return QString(n / ts, '\t')
         + QString(n % ts, ' ');
}

void FakeVimHandler::Private::insertAutomaticIndentation(bool goingDown, bool forceAutoIndent)
{
    if (!forceAutoIndent && !s.autoIndent() && !s.smartIndent())
        return;

    if (s.smartIndent()) {
        QTextBlock bl = block();
        Range range(bl.position(), bl.position());
        indentText(range, '\n');
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
    }

    // Remember this as a freshly auto-indented line so that leaving it without
    // typing anything removes the indentation again (QTCREATORBUG-15009).
    if (block().text().trimmed().isEmpty())
        m_autoIndentBlock = block().blockNumber();
}

void FakeVimHandler::Private::clearUntouchedAutoIndentation()
{
    const int pending = m_autoIndentBlock;
    m_autoIndentBlock = -1;
    // Only when we are still on that very line and nothing was typed on it.
    if (pending < 0 || pending != block().blockNumber())
        return;
    const QString text = block().text();
    if (text.isEmpty() || !text.trimmed().isEmpty())
        return;
    const int start = block().position();
    joinPreviousEditBlock();
    m_cursor.setPosition(start);
    m_cursor.setPosition(start + text.size(), KeepAnchor);
    m_cursor.removeSelectedText();
    endEditBlock();
}

void FakeVimHandler::Private::handleStartOfLine()
{
    if (s.startOfLine())
        moveToFirstNonBlankOnLine();
}

void FakeVimHandler::Private::replay(const QString &command, int repeat)
{
    if (repeat <= 0)
        return;

    //qDebug() << "REPLAY: " << quoteUnprintable(command);
    clearCurrentMode();
    const Inputs inputs(command);
    for (int i = 0; i < repeat; ++i) {
        for (const Input &in : inputs) {
            if (handleDefaultKey(in) != EventHandled)
                return;
        }
    }
}

QString FakeVimHandler::Private::visualDotCommand() const
{
    QTextCursor start(m_cursor);
    QTextCursor end(start);
    end.setPosition(end.anchor());

    QString command;

    if (isVisualCharMode())
        command = "v";
    else if (isVisualLineMode())
        command = "V";
    else if (isVisualBlockMode())
        command = "<c-v>";
    else
        return QString();

    const int down = qAbs(start.blockNumber() - end.blockNumber());
    if (down != 0)
        command.append(QString("%1j").arg(down));

    const int right = start.positionInBlock() - end.positionInBlock();
    if (right != 0) {
        command.append(QString::number(qAbs(right)));
        command.append(QLatin1Char(right < 0 && isVisualBlockMode() ? 'h' : 'l'));
    }

    return command;
}

void FakeVimHandler::Private::selectTextObject(bool simple, bool inner)
{
    const int position1 = this->position();
    const int anchor1 = this->anchor();
    bool setupAnchor = (position1 == anchor1);
    bool forward = anchor1 <= position1;
    const int repeat = count();

    // set anchor if not already set
    if (setupAnchor) {
        // Select nothing with 'inner' on empty line.
        if (inner && atEmptyLine() && repeat == 1) {
            g.movetype = MoveExclusive;
            return;
        }
        moveToBoundaryStart(1, simple, false);
        setAnchor();
    } else if (forward) {
        moveToNextCharacter();
    } else {
        moveToPreviousCharacter();
    }

    if (inner) {
        moveToBoundaryEnd(repeat, simple);
    } else {
        const int direction = forward ? 1 : -1;
        for (int i = 0; i < repeat; ++i) {
            // select leading spaces
            bool leadingSpace = characterAtCursor().isSpace();
            if (leadingSpace) {
                if (forward)
                    moveToNextBoundaryStart(1, simple);
                else
                    moveToNextBoundaryEnd(1, simple, false);
            }

            // select word
            if (forward)
                moveToWordEnd(1, simple);
            else
                moveToWordStart(1, simple, false);

            // select trailing spaces if no leading space
            QChar afterCursor = characterAt(position() + direction);
            if (!leadingSpace && afterCursor.isSpace() && afterCursor != ParagraphSeparator
                && !atBlockStart()) {
                if (forward)
                    moveToNextBoundaryEnd(1, simple);
                else
                    moveToNextBoundaryStart(1, simple, false);
            }

            // if there are no trailing spaces in selection select all leading spaces
            // after previous character
            if (setupAnchor && (!characterAtCursor().isSpace() || atBlockEnd())) {
                int min = block().position();
                int pos = anchor();
                while (pos >= min && characterAt(--pos).isSpace()) {}
                if (pos >= min)
                    setAnchorAndPosition(pos + 1, position());
            }

            if (i + 1 < repeat) {
                if (forward)
                    moveToNextCharacter();
                else
                    moveToPreviousCharacter();
            }
        }
    }

    if (inner) {
        g.movetype = MoveInclusive;
    } else {
        g.movetype = MoveExclusive;
        if (isNoVisualMode())
            moveToNextCharacter();
        else if (isVisualLineMode())
            g.visualMode = VisualCharMode;
    }

    setTargetColumn();
}

void FakeVimHandler::Private::selectWordTextObject(bool inner)
{
    selectTextObject(false, inner);
}

void FakeVimHandler::Private::selectWORDTextObject(bool inner)
{
    selectTextObject(true, inner);
}

void FakeVimHandler::Private::selectSentenceTextObject(bool inner)
{
    Q_UNUSED(inner)
}

void FakeVimHandler::Private::selectParagraphTextObject(bool inner)
{
    const QTextCursor oldCursor = m_cursor;
    const VisualMode oldVisualMode = g.visualMode;

    const int anchorBlock = blockNumberAt(anchor());
    const int positionBlock = blockNumberAt(position());
    const bool setupAnchor = anchorBlock == positionBlock;
    int repeat = count();

    // If anchor and position are in the same block,
    // start line selection at beginning of current paragraph.
    if (setupAnchor) {
        moveToParagraphStartOrEnd(-1);
        setAnchor();

        if (!isVisualLineMode() && isVisualMode())
            toggleVisualMode(VisualLineMode);
    }

    const bool forward = anchor() <= position();
    const int d = forward ? 1 : -1;

    bool startsAtParagraph = !atEmptyLine(position());

    moveToParagraphStartOrEnd(d);

    // If selection already changed, decreate count.
    if ((setupAnchor && g.submode != NoSubMode)
        || oldVisualMode != g.visualMode
        || m_cursor != oldCursor)
    {
        --repeat;
        if (!inner) {
            moveDown(d);
            moveToParagraphStartOrEnd(d);
            startsAtParagraph = !startsAtParagraph;
        }
    }

    if (repeat > 0) {
        bool isCountEven = repeat % 2 == 0;
        bool endsOnParagraph =
                inner ? isCountEven == startsAtParagraph : startsAtParagraph;

        if (inner) {
            repeat = repeat / 2;
            if (!isCountEven || endsOnParagraph)
                ++repeat;
        } else {
            if (endsOnParagraph)
                ++repeat;
        }

        if (!moveToNextParagraph(d * repeat)) {
            m_cursor = oldCursor;
            g.visualMode = oldVisualMode;
            return;
        }

        if (endsOnParagraph && atEmptyLine())
            moveUp(d);
        else
            moveToParagraphStartOrEnd(d);
    }

    if (!inner && setupAnchor && !atEmptyLine() && !atEmptyLine(anchor())) {
        // If position cannot select empty lines, try to select them with anchor.
        setAnchorAndPosition(position(), anchor());
        moveToNextParagraph(-d);
        moveToParagraphStartOrEnd(-d);
        setAnchorAndPosition(position(), anchor());
    }

    recordJump(oldCursor.position());
    setTargetColumn();
    g.movetype = MoveLineWise;
}

bool FakeVimHandler::Private::selectBlockTextObject(bool inner,
    QChar left, QChar right)
{
    int p1 = blockBoundary(left, right, false, count());
    if (p1 == -1)
        return false;

    int p2 = blockBoundary(left, right, true, count());
    if (p2 == -1)
        return false;

    g.movetype = MoveExclusive;

    if (inner) {
        p1 += 1;
        bool moveStart = characterAt(p1) == ParagraphSeparator;
        bool moveEnd = isFirstNonBlankOnLine(p2);
        if (moveStart)
            ++p1;
        if (moveEnd)
            p2 = blockAt(p2).position() - 1;
        if (moveStart && moveEnd)
            g.movetype = MoveLineWise;
    } else {
        p2 += 1;
    }

    if (isVisualMode())
        --p2;

    setAnchorAndPosition(p1, p2);

    return true;
}

bool FakeVimHandler::Private::changeNumberTextObject(int count)
{
    const QTextBlock block = this->block();
    const QString lineText = block.text();
    const int posMin = m_cursor.positionInBlock() + 1;

    // find first decimal, hexadecimal or octal number under or after cursor position
    static const QRegularExpression re("(0[xX])(0*[0-9a-fA-F]+)|(0)(0*[0-7]+)(?=\\D|$)|(\\d+)");
    QRegularExpressionMatch match;
    QRegularExpressionMatchIterator it = re.globalMatch(lineText);
    while (true) {
        if (!it.hasNext())
            return false;
        match = it.next();
        if (match.capturedEnd() >= posMin)
            break;
    }
    int pos = match.capturedStart();
    int len = match.capturedLength();
    QString prefix = match.captured(1) + match.captured(3);
    bool hex = prefix.size() >= 2 && (prefix[1].toLower() == 'x');
    bool octal = !hex && !prefix.isEmpty();
    const QString num = hex ? match.captured(2) : octal ? match.captured(4) : match.captured(5);

    // parse value
    bool ok;
    int base = hex ? 16 : octal ? 8 : 10;
    qlonglong value = 0;  // decimal value
    qlonglong uvalue = 0; // hexadecimal or octal value (only unsigned)
    if (hex || octal)
        uvalue = num.toULongLong(&ok, base);
    else
        value = num.toLongLong(&ok, base);
    if (!ok) {
        qWarning() << "Cannot parse number:" << num << "base:" << base;
        return false;
    }

    // negative decimal number
    if (!octal && !hex && pos > 0 && lineText[pos - 1] == '-') {
        value = -value;
        --pos;
        ++len;
    }

    // result to string
    QString repl;
    if (hex || octal)
        repl = QString::number(uvalue + count, base);
    else
        repl = QString::number(value + count, base);

    // convert hexadecimal number to upper-case if last letter was upper-case
    if (hex) {
        static const QRegularExpression regexp("[a-fA-F]");
        const int lastLetter = num.lastIndexOf(regexp);
        if (lastLetter != -1 && num[lastLetter].isUpper())
            repl = repl.toUpper();
    }

    // preserve leading zeroes
    if ((octal || hex) && repl.size() < num.size())
        prefix.append(QString("0").repeated(num.size() - repl.size()));
    repl.prepend(prefix);

    pos += block.position();
    pushUndoState();
    setAnchorAndPosition(pos, pos + len);
    replaceText(currentRange(), repl);
    setPosition(pos + repl.size() - 1);

    return true;
}

bool FakeVimHandler::Private::selectQuotedStringTextObject(bool inner,
    const QString &quote)
{
    QTextCursor tc = m_cursor;
    int sz = quote.size();

    // Vim's quote text objects (i", a", ...) operate on the current line
    // only, so quotes on other lines must not influence the pairing
    // (QTCREATORBUG-22484).
    const QTextBlock line = tc.block();
    const int lineEnd = line.position() + line.length() - 1;

    QTextCursor tc1;
    QTextCursor tc2(document());
    tc2.setPosition(line.position());
    while (tc2 <= tc) {
        tc1 = document()->find(quote, tc2);
        if (tc1.isNull() || tc1.position() > lineEnd)
            return false;
        tc2 = document()->find(quote, tc1);
        if (tc2.isNull() || tc2.position() > lineEnd)
            return false;
    }

    int p1 = tc1.position();
    int p2 = tc2.position();
    if (inner) {
        p2 = qMax(p1, p2 - sz);
        if (characterAt(p1) == ParagraphSeparator)
            ++p1;
    } else {
        p1 -= sz;
        p2 -= sz - 1;
    }

    if (isVisualMode())
        --p2;

    setAnchorAndPosition(p1, p2);
    g.movetype = MoveExclusive;

    return true;
}

bool FakeVimHandler::Private::selectArgumentTextObject(bool inner)
{
    // We are just interested whether we're currently inside angled brackets,
    // but selectBlockTextObject also moves the cursor, so set it back to
    // its original position afterwards
    QTextCursor prevCursor = m_cursor;
    const bool insideTemplateParameter = selectBlockTextObject(true, '<', '>');
    m_cursor = prevCursor;

    int openAngleBracketCount = insideTemplateParameter ? 1 : 0;

    QTextCursor tcStart(m_cursor);
    while (true) {
        if (tcStart.atStart())
            return true;

        const QChar currentChar = characterAt(tcStart.position());

        if (openAngleBracketCount == 0
                && (currentChar == '(' || currentChar == ','))
            break;

        if (currentChar == '<')
            openAngleBracketCount--;
        else if (currentChar == '>')
            openAngleBracketCount++;

        tcStart.setPosition(tcStart.position() - 1);
    }

    QTextCursor tcEnd(m_cursor);
    openAngleBracketCount = insideTemplateParameter ? 1 : 0;
    int openParanthesisCount = 0;

    while (true) {
        if (tcEnd.atEnd()) {
            return true;
        }

        const QChar currentChar = characterAt(tcEnd.position());
        if (openAngleBracketCount == 0
                && openParanthesisCount == 0
                && (currentChar == ')' || currentChar == ','))
            break;

        if (currentChar == '<')
            openAngleBracketCount++;
        else if (currentChar == '>')
            openAngleBracketCount--;
        else if (currentChar == '(')
            openParanthesisCount++;
        else if (currentChar == ')')
            openParanthesisCount--;


        tcEnd.setPosition(tcEnd.position() + 1);
    }


    if (!inner && characterAt(tcEnd.position()) == ',' && characterAt(tcStart.position()) == '(') {
        tcEnd.setPosition(tcEnd.position() + 1);
        if (characterAt(tcEnd.position()) == ' ')
            tcEnd.setPosition(tcEnd.position() + 1);
    }

    // Never include the opening paranthesis
    if (characterAt(tcStart.position()) == '(') {
        tcStart.setPosition(tcStart.position() + 1);
    } else if (inner) {
        tcStart.setPosition(tcStart.position() + 1);
        if (characterAt(tcStart.position()) == ' ')
            tcStart.setPosition(tcStart.position() + 1);
    }

    if (isVisualMode())
        tcEnd.setPosition(tcEnd.position() - 1);

    g.movetype = MoveExclusive;

    setAnchorAndPosition(tcStart.position(), tcEnd.position());
    return true;
}

bool FakeVimHandler::Private::selectTagTextObject(bool inner)
{
    // "it"/"at": select the content of, or the whole of, the innermost XML/HTML
    // tag pair around the cursor. A count selects further-out enclosing pairs.
    const QString text = document()->toPlainText();
    const int pos = position();

    // Opening, closing or self-closing tag; attribute values may contain ">".
    static const QRegularExpression tagRe(
        "<(/?)([a-zA-Z][-\\w:.]*)(?:\"[^\"]*\"|'[^']*'|[^'\">])*?(/?)>");

    struct Tag { int openStart; int openEnd; int closeStart; int closeEnd; };
    QList<Tag> enclosing;

    struct Open { QString name; int start; int end; };
    QList<Open> stack;

    QRegularExpressionMatchIterator it = tagRe.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        if (!m.captured(3).isEmpty()) // self-closing, opens no scope
            continue;
        const int start = m.capturedStart();
        const int end = m.capturedEnd();
        if (m.captured(1).isEmpty()) {
            stack.append({m.captured(2), start, end});
            continue;
        }
        // Closing tag: pair it with the nearest matching open, dropping any
        // unclosed tags opened in between.
        for (int i = stack.size() - 1; i >= 0; --i) {
            if (stack.at(i).name != m.captured(2))
                continue;
            const Open open = stack.at(i);
            while (stack.size() > i)
                stack.removeLast();
            if (open.start <= pos && pos < end)
                enclosing.append({open.start, open.end, start, end});
            break;
        }
    }

    if (enclosing.isEmpty())
        return false;

    // Innermost first.
    std::sort(enclosing.begin(), enclosing.end(),
              [](const Tag &a, const Tag &b) { return a.openStart > b.openStart; });
    const int level = count() - 1;
    if (level >= enclosing.size())
        return false;
    const Tag &tag = enclosing.at(level);

    int p1 = inner ? tag.openEnd : tag.openStart;
    int p2 = inner ? tag.closeStart : tag.closeEnd;

    g.movetype = MoveExclusive;
    if (isVisualMode())
        --p2;
    setAnchorAndPosition(p1, p2);

    return true;
}

Mark FakeVimHandler::Private::mark(QChar code) const
{
    if (isVisualMode()) {
        if (code == '<')
            return CursorPosition(document(), qMin(anchor(), position()));
        if (code == '>')
            return CursorPosition(document(), qMax(anchor(), position()));
    }

    if (code.isUpper())
        return g.marks.value(code);

    return m_buffer->marks.value(code);
}

void FakeVimHandler::Private::setMark(QChar code, CursorPosition position)
{
    if (code.isUpper())
        g.marks[code] = Mark(position, m_currentFileName);
    else
        m_buffer->marks[code] = Mark(position);
}

void FakeVimHandler::Private::removeMark(QChar code)
{
    if (code.isUpper())
        g.marks.remove(code);
    else
        m_buffer->marks.remove(code);
}

bool FakeVimHandler::Private::jumpToMark(QChar mark, bool backTickMode)
{
    Mark m = this->mark(mark);
    if (!m.isValid()) {
        showMessage(MessageError, msgMarkNotSet(mark));
        return false;
    }
    if (!m.isLocal(m_currentFileName)) {
        q->requestJumpToGlobalMark(mark, backTickMode, m.fileName());
        return false;
    }

    if ((mark == '\'' || mark == '`') && !m_buffer->jumpListUndo.isEmpty())
        m_buffer->jumpListUndo.pop();
    recordJump();
    setCursorPosition(m.position(document()));
    if (!backTickMode)
        moveToFirstNonBlankOnLine();
    if (g.submode == NoSubMode)
        setAnchor();
    setTargetColumn();

    return true;
}

void FakeVimHandler::Private::updateMarks(const Marks &newMarks)
{
    for (auto it = newMarks.cbegin(), end = newMarks.cend(); it != end; ++it)
        m_buffer->marks[it.key()] = it.value();
}

RangeMode FakeVimHandler::Private::registerRangeMode(int reg) const
{
    bool isClipboard;
    bool isSelection;
    getRegisterType(&reg, &isClipboard, &isSelection);

    if (isClipboard || isSelection) {
        QClipboard *clipboard = QApplication::clipboard();
        QClipboard::Mode mode = isClipboard ? QClipboard::Clipboard : QClipboard::Selection;

        // Use range mode from Vim's clipboard data if available.
        const QMimeData *data = clipboard->mimeData(mode);
        if (data && data->hasFormat(vimMimeText)) {
            QByteArray bytes = data->data(vimMimeText);
            if (bytes.length() > 0)
                return static_cast<RangeMode>(bytes.at(0));
        }

        // If register content is clipboard:
        //  - return RangeLineMode if text ends with new line char,
        //  - return RangeCharMode otherwise.
        QString text = clipboard->text(mode);
        return (text.endsWith('\n') || text.endsWith('\r')) ? RangeLineMode : RangeCharMode;
    }

    return g.registers[reg].rangemode;
}

void FakeVimHandler::Private::setRegister(int reg, const QString &contents, RangeMode mode)
{
    bool copyToClipboard;
    bool copyToSelection;
    bool append;
    getRegisterType(&reg, &copyToClipboard, &copyToSelection, &append);

    QString contents2 = contents;
    if ((mode == RangeLineMode || mode == RangeLineModeExclusive)
            && !contents2.endsWith('\n'))
    {
        contents2.append('\n');
    }

    if (copyToClipboard || copyToSelection) {
        if (copyToClipboard)
            setClipboardData(contents2, mode, QClipboard::Clipboard);
        if (copyToSelection)
            setClipboardData(contents2, mode, QClipboard::Selection);
    } else {
        if (append)
            g.registers[reg].contents.append(contents2);
        else
            g.registers[reg].contents = contents2;
        g.registers[reg].rangemode = mode;
    }
}

QString FakeVimHandler::Private::registerContents(int reg) const
{
    bool copyFromClipboard;
    bool copyFromSelection;
    getRegisterType(&reg, &copyFromClipboard, &copyFromSelection);

    if (copyFromClipboard || copyFromSelection) {
        QClipboard *clipboard = QApplication::clipboard();
        if (copyFromClipboard)
            return clipboard->text(QClipboard::Clipboard);
        if (copyFromSelection)
            return clipboard->text(QClipboard::Selection);
    }

    return g.registers[reg].contents;
}

void FakeVimHandler::Private::getRegisterType(int *reg, bool *isClipboard, bool *isSelection, bool *append) const
{
    bool clipboard = false;
    bool selection = false;

    // If register is uppercase, append content to lower case register on yank/delete.
    const QChar c(*reg);
    if (append != nullptr)
        *append = c.isUpper();
    if (c.isUpper())
        *reg = c.toLower().unicode();

    if (c == '"') {
        QStringList list = s.clipboard().split(',');
        clipboard = list.contains("unnamedplus");
        selection = list.contains("unnamed");
    } else if (c == '+') {
        clipboard = true;
    } else if (c == '*') {
        selection = true;
    }

    // selection (primary) is clipboard on systems without selection support
    if (selection && !QApplication::clipboard()->supportsSelection()) {
        clipboard = true;
        selection = false;
    }

    if (isClipboard != nullptr)
        *isClipboard = clipboard;
    if (isSelection != nullptr)
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
    d->m_textedit = nullptr;
    d->m_plaintextedit = nullptr;
    d->m_qcPlainTextEdit = nullptr;
}

void FakeVimHandler::updateGlobalMarksFilenames(const QString &oldFileName, const QString &newFileName)
{
    for (Mark &mark : Private::g.marks) {
        if (mark.fileName() == oldFileName)
            mark.setFileName(newFileName);
    }
}

bool FakeVimHandler::eventFilter(QObject *ob, QEvent *ev)
{
#ifndef FAKEVIM_STANDALONE
    if (!settings().useFakeVim())
        return QObject::eventFilter(ob, ev);
#endif

    if (ev->type() == QEvent::Shortcut) {
        d->passShortcuts(false);
        return false;
    }

    if (ev->type() == QEvent::KeyPress &&
        (ob == d->editor()
         || (Private::g.mode == ExMode || Private::g.subsubmode == SearchSubSubMode))) {
        auto kev = static_cast<QKeyEvent *>(ev);
        KEY_DEBUG("KEYPRESS" << kev->key() << kev->text() << QChar(kev->key()));
        EventResult res = d->handleEvent(kev);
        //if (Private::g.mode == InsertMode)
        //    completionRequested();
        // returning false core the app see it
        //KEY_DEBUG("HANDLED CODE:" << res);
        //return res != EventPassedToCore;
        //return true;
        return res == EventHandled || res == EventCancelled;
    }

    if (ev->type() == QEvent::ShortcutOverride && (ob == d->editor()
         || (Private::g.mode == ExMode || Private::g.subsubmode == SearchSubSubMode))) {
        auto kev = static_cast<QKeyEvent *>(ev);
        if (d->wantsOverride(kev)) {
            KEY_DEBUG("OVERRIDING SHORTCUT" << kev->key());
            ev->accept(); // accepting means "don't run the shortcuts"
            return true;
        }
        KEY_DEBUG("NO SHORTCUT OVERRIDE" << kev->key());
        // Make the visual selection inclusive before a Qt Creator shortcut
        // (e.g. Advanced Find) reads it synchronously (QTCREATORBUG-27442).
        d->fixExternalCursor(false);
        // Accept plain text-input keys so they arrive as key presses instead of
        // being eaten by the shortcut machinery. Needed for layouts that reach
        // characters through layout modifiers (QTCREATORBUG-24904); mirrors the
        // editor's own handling (QTCREATORBUG-22854).
        const Qt::KeyboardModifiers mods = kev->modifiers();
        kev->setAccepted((mods == Qt::NoModifier || mods == Qt::ShiftModifier
                          || mods == Qt::KeypadModifier)
                         && kev->key() < Qt::Key_Escape);
        return true;
    }

    if (ev->type() == QEvent::FocusOut && ob == d->editor()) {
        d->unfocus();
        return false;
    }

    if (ev->type() == QEvent::FocusIn && ob == d->editor())
        d->focus();

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
    d->enterFakeVim();
    d->handleCommand(cmd);
    d->leaveFakeVim();
}

void FakeVimHandler::handleReplay(const QString &keys)
{
    d->enterFakeVim();
    d->replay(keys);
    d->leaveFakeVim();
}

void FakeVimHandler::handleInput(const QString &keys)
{
    const Inputs inputs(keys);
    d->enterFakeVim();
    for (const Input &input : inputs)
        d->handleKey(input);
    d->leaveFakeVim();
}

void FakeVimHandler::enterCommandMode()
{
    d->enterCommandMode();
}

void FakeVimHandler::setCurrentFileName(const QString &fileName)
{
    d->m_currentFileName = fileName;
}

QString FakeVimHandler::currentFileName() const
{
    return d->m_currentFileName;
}

void FakeVimHandler::showMessage(MessageLevel level, const QString &msg)
{
    d->showMessage(level, msg);
}

void FakeVimHandler::triggerAutocmd(const QString &event)
{
    d->triggerAutocmd(event);
}

void FakeVimHandler::processModelines()
{
    d->processModelines();
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

void FakeVimHandler::miniBufferTextEdited(const QString &text, int cursorPos, int anchorPos)
{
    d->miniBufferTextEdited(text, cursorPos, anchorPos);
}

void FakeVimHandler::setTextCursorPosition(int position)
{
    int pos = qMax(0, qMin(position, d->lastPositionInDocument()));
    if (d->isVisualMode())
        d->setPosition(pos);
    else
        d->setAnchorAndPosition(pos, pos);
    d->setTargetColumn();

    if (!d->m_inFakeVim)
        d->commitCursor();
}

QTextCursor FakeVimHandler::textCursor() const
{
    return d->m_cursor;
}

void FakeVimHandler::setTextCursor(const QTextCursor &cursor)
{
    d->m_cursor = cursor;
}

bool FakeVimHandler::jumpToLocalMark(QChar mark, bool backTickMode)
{
    return d->jumpToMark(mark, backTickMode);
}

bool FakeVimHandler::inFakeVimMode()
{
    return d->m_inFakeVim;
}

} // namespace FakeVim::Internal

Q_DECLARE_METATYPE(FakeVim::Internal::FakeVimHandler::Private::BufferDataPtr)
