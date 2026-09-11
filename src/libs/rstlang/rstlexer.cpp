// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rstlexer.h"

#include "rstparser.h"

#include <parsing/engine.h>

using namespace Qt::Literals::StringLiterals;

using namespace RstLang;

enum { TabStop = 8 };

// The characters reStructuredText underlines a title with and draws a
// transition from.
static bool isAdornment(QChar c)
{
    static const QLatin1StringView adornments("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
    return adornments.contains(c);
}

// Whether the token stands for a line of the document.  A line leaves the
// block it belongs to open; a marker the lexer inserts does not.
static bool isLine(int kind)
{
    switch (kind) {
    case Parser::T_TEXT:
    case Parser::T_VERBATIM:
    case Parser::T_TITLE:
    case Parser::T_TRANSITION:
    case Parser::T_BULLET:
    case Parser::T_ENUMERATOR:
    case Parser::T_FIELD_NAME:
    case Parser::T_DIRECTIVE:
    case Parser::T_SUBSTITUTION:
    case Parser::T_TARGET:
    case Parser::T_COMMENT:
    case Parser::T_LINE:
    case Parser::T_FOOTNOTE:
    case Parser::T_TABLE:
        return true;
    default:
        return false;
    }
}

// Whether the token opens a construct that owns the block indented under it,
// where the blank line in between says nothing.
static bool opensBody(int kind)
{
    switch (kind) {
    case Parser::T_BULLET:
    case Parser::T_ENUMERATOR:
    case Parser::T_FIELD_NAME:
    case Parser::T_DIRECTIVE:
    case Parser::T_SUBSTITUTION:
    case Parser::T_FOOTNOTE:
        return true;
    default:
        return false;
    }
}

// Whether the line carries on the block the one before it started.
static bool continuesRun(int lastKind, int kind)
{
    if (lastKind != kind)
        return false;
    return kind == Parser::T_TEXT || kind == Parser::T_LINE || kind == Parser::T_VERBATIM;
}

Lexer::Lexer(Engine *engine, QStringView source)
    : _engine(engine)
    , _source(source)
    , _size(int(source.size()))
{
    _indents.push_back(0);
    _line = readLine(0, 1);
}

const char *Lexer::name(int kind)
{
    switch (kind) {
    case Parser::EOF_SYMBOL:
        return "nothing";
    case Parser::T_TEXT:
        return "text";
    case Parser::T_VERBATIM:
        return "verbatim text";
    case Parser::T_TITLE:
        return "section title";
    case Parser::T_TRANSITION:
        return "transition";
    case Parser::T_BULLET:
        return "bullet";
    case Parser::T_ENUMERATOR:
        return "enumerator";
    case Parser::T_FIELD_NAME:
        return "field name";
    case Parser::T_DIRECTIVE:
        return "directive";
    case Parser::T_SUBSTITUTION:
        return "substitution definition";
    case Parser::T_TARGET:
        return "hyperlink target";
    case Parser::T_COMMENT:
        return "comment";
    case Parser::T_LINE:
        return "line block line";
    case Parser::T_FOOTNOTE:
        return "footnote";
    case Parser::T_TABLE:
        return "table";
    case Parser::T_BLANK:
        return "blank line";
    case Parser::T_INDENT:
        return "indent";
    case Parser::T_DEDENT:
        return "dedent";
    case Parser::T_SECTION_START:
        return "section start";
    case Parser::T_SECTION_END:
        return "section end";
    case Parser::T_ERROR:
        return "bad markup";
    default:
        return "unknown token";
    }
}

bool Lexer::isVerbatimDirective(QStringView type)
{
    static const QLatin1StringView types[] = {"code"_L1,
                                              "code-block"_L1,
                                              "sourcecode"_L1,
                                              "parsed-literal"_L1,
                                              "literalinclude"_L1,
                                              "math"_L1,
                                              "raw"_L1};
    for (QLatin1StringView candidate : types) {
        if (type.compare(candidate, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

int Lexer::simpleTableRule(QStringView line)
{
    int columns = 0;
    bool inRun = false;
    for (QChar c : line) {
        if (c == u'=') {
            if (!inRun)
                ++columns;
            inRun = true;
        } else if (c == u' ') {
            inRun = false;
        } else {
            return 0;
        }
    }
    return columns;
}

int Lexer::gridTableRule(QStringView line)
{
    if (line.size() < 3 || !line.startsWith(u'+') || !line.endsWith(u'+'))
        return 0;

    int columns = 0;
    for (qsizetype i = 1; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == u'+')
            ++columns;
        else if (c != u'-' && c != u'=')
            return 0;
    }
    return columns;
}

int Lexer::yylex(Token *tk)
{
    while (_queue.empty() && !_done)
        fill();

    if (_queue.empty()) {
        *tk = Token();
        tk->kind = Parser::EOF_SYMBOL;
        tk->position = _size;
        tk->line = _line.number;
        return tk->kind;
    }

    *tk = _queue.front();
    _queue.pop_front();
    return tk->kind;
}

Lexer::Line Lexer::readLine(int position, int number) const
{
    Line line;
    line.start = qMin(position, _size);
    line.number = number;

    int pos = line.start;
    int column = 0;
    while (pos < _size) {
        const QChar c = _source.at(pos);
        if (c == u' ') {
            ++column;
        } else if (c == u'\t') {
            column += TabStop - column % TabStop;
        } else {
            break;
        }
        ++pos;
    }
    line.indent = column;
    line.textStart = pos;

    int end = pos;
    while (end < _size && _source.at(end) != u'\n')
        ++end;
    line.next = end < _size ? end + 1 : _size;

    line.end = end;
    while (line.end > line.textStart && _source.at(line.end - 1).isSpace())
        --line.end;
    line.blank = line.textStart >= line.end;

    return line;
}

void Lexer::fill()
{
    std::vector<Line> blanks;
    while (_line.start < _size && _line.blank) {
        blanks.push_back(_line);
        _line = nextLine(_line);
    }

    if (_line.start >= _size) {
        finish();
        return;
    }

    if (inVerbatim()) {
        if (_line.indent >= _verbatimIndent) {
            // The blank lines a literal block encloses belong to it.
            for (const Line &blank : blanks) {
                Token &tk = push(Parser::T_VERBATIM, blank.start, blank.number, 1);
                tk.indent = _verbatimIndent;
            }
            scanVerbatim(_line);
            _line = nextLine(_line);
            return;
        }
        _verbatimIndent = -1;
        _verbatimBodyIndent = -1;
    }

    if (!blanks.empty())
        _blankPending = true;

    scanLine(_line);
}

void Lexer::finish()
{
    separate();
    closeIndents(0);
    closeSections(1);
    _done = true;
}

void Lexer::scanLine(Line line)
{
    if (line.indent < currentIndent()) {
        separate();
        closeIndents(line.indent);
        _literalFollows = false;
        _verbatimDirective = false;
        if (line.indent < _verbatimBodyIndent)
            _verbatimBodyIndent = -1;
    }

    if (_indents.size() == 1 && line.indent == 0 && scanTitle(line))
        return;

    _line = nextLine(line);

    if (line.indent > currentIndent()) {
        if (_literalFollows) {
            // What is indented under a paragraph that ends in "::" stands the
            // way it is written.
            separate(Parser::T_VERBATIM);
            _literalFollows = false;
            _verbatimIndent = line.indent;
            scanVerbatim(line);
            return;
        }

        // A construct that owns what is indented under it has the blank line
        // that ends it on the line it stands on, so the one in between says
        // nothing.
        if (_blankPending && !opensBody(_lastKind))
            separate();
        _blankPending = false;

        if (_verbatimDirective) {
            _verbatimDirective = false;
            _verbatimBodyIndent = line.indent;
        }
        openIndent(line.indent, line);
    } else {
        _literalFollows = false;
        _verbatimDirective = false;
    }

    // The body of a verbatim directive stands the way it is written, but for
    // the fields that configure the directive and that come first.
    if (line.indent == _verbatimBodyIndent && !isFieldLine(line)) {
        separate(Parser::T_VERBATIM);
        _verbatimIndent = line.indent;
        scanVerbatim(line);
        return;
    }

    scanBlockStart(line);
}

void Lexer::scanVerbatim(const Line &line)
{
    Token &tk = push(Parser::T_VERBATIM, line);
    tk.text = line.content(_source);
}

void Lexer::scanBlockStart(const Line &line)
{
    const QStringView content = line.content(_source);

    if (content.startsWith(".."_L1) && (content.size() == 2 || content.at(2).isSpace())) {
        scanExplicitMarkup(line);
        return;
    }

    if (scanField(line))
        return;

    if (scanListItem(line))
        return;

    if (content.startsWith(u'|') && (content.size() == 1 || content.at(1).isSpace())) {
        separate(Parser::T_LINE);
        const Line last = continuationEnd(line);
        Token &tk = push(Parser::T_LINE, line);
        tk.name = content.first(1);
        tk.length = last.end - line.textStart;
        tk.spelling = _source.mid(tk.position, tk.length);
        tk.text = _source.mid(tk.position + qMin<qsizetype>(2, content.size()),
                              tk.length - qMin<qsizetype>(2, content.size()));
        _blockEnd = last.end;
        _line = nextLine(last);
        return;
    }

    if (scanTable(line))
        return;

    // A run of punctuation that underlines nothing sets what stands before it
    // apart from what comes after.
    if (_blankPending && adornmentLength(line) >= 4) {
        separate();
        push(Parser::T_TRANSITION, line);
        return;
    }

    scanText(line);
}

void Lexer::scanText(const Line &line)
{
    separate(Parser::T_TEXT);
    Token &tk = push(Parser::T_TEXT, line);
    tk.text = line.content(_source);
    _literalFollows = tk.text.endsWith("::"_L1);
}

void Lexer::scanExplicitMarkup(const Line &line)
{
    const QStringView content = line.content(_source);
    qsizetype markupStart = qMin<qsizetype>(3, content.size());
    while (markupStart < content.size() && content.at(markupStart).isSpace())
        ++markupStart;
    const QStringView markup = content.sliced(markupStart).trimmed();

    const auto comment = [&] {
        separate();
        const Line last = commentEnd(line);
        Token &tk = push(Parser::T_COMMENT, line);
        tk.length = last.end - line.textStart;
        tk.spelling = _source.mid(tk.position, tk.length);
        tk.text = markup.isEmpty() ? QStringView()
                                   : _source.mid(tk.position + 3, tk.length - 3);
        _blockEnd = last.end;
        _line = nextLine(last);
    };

    if (markup.isEmpty()) {
        comment();
        return;
    }

    if (markup.startsWith(u'_')) {
        // A hyperlink target: ".. _the name: the link".
        const qsizetype colon = unescapedIndexOf(markup, u':');
        if (colon < 0) {
            comment();
            return;
        }
        separate();
        Token &tk = push(Parser::T_TARGET, line);
        tk.name = markup.sliced(1, colon - 1).trimmed();
        tk.text = markup.sliced(colon + 1).trimmed();
        return;
    }

    if (markup.startsWith(u'|')) {
        // A substitution: ".. |the name| replace:: the text".
        const qsizetype end = markup.indexOf(u'|', 1);
        if (end < 1) {
            comment();
            return;
        }
        separate();
        Token &tk = push(Parser::T_SUBSTITUTION, line);
        tk.name = markup.sliced(1, end - 1);
        tk.text = markup.sliced(end + 1).trimmed();
        _verbatimDirective = isVerbatimDirective(substitutionType(tk.text));
        return;
    }

    if (markup.startsWith(u'[')) {
        // A footnote or a citation: ".. [the label] the text".
        const qsizetype end = markup.indexOf(u']', 1);
        if (end < 2) {
            comment();
            return;
        }
        separate();
        Token &tk = push(Parser::T_FOOTNOTE, line);
        tk.name = markup.sliced(1, end - 1);
        tk.text = markup.sliced(end + 1).trimmed();
        // The body of an explicit markup block stands at the column behind
        // its marker, whatever column the label leaves off at.
        openValue(line, int(markupStart + end) + 1, line.indent + int(markupStart));
        return;
    }

    // A directive: ".. the type:: the argument".
    qsizetype colons = -1;
    for (qsizetype i = 0; i + 1 < markup.size(); ++i) {
        const QChar c = markup.at(i);
        if (c == u':' && markup.at(i + 1) == u':') {
            colons = i;
            break;
        }
        if (!c.isLetterOrNumber() && c != u'-' && c != u'_' && c != u'.' && c != u'+'
            && c != u':') {
            break;
        }
    }
    if (colons <= 0 || (colons + 2 < markup.size() && !markup.at(colons + 2).isSpace())) {
        comment();
        return;
    }

    separate();
    Token &tk = push(Parser::T_DIRECTIVE, line);
    tk.name = markup.first(colons);
    tk.text = markup.sliced(qMin(colons + 2, markup.size())).trimmed();
    _verbatimDirective = isVerbatimDirective(directiveType(tk.name));
}

bool Lexer::isFieldLine(const Line &line) const
{
    return fieldNameEnd(line) > 0;
}

// Where the colon that closes the name of a field stands, counted from the
// start of the content of the line, or zero where the line holds no field.
qsizetype Lexer::fieldNameEnd(const Line &line) const
{
    const QStringView content = line.content(_source);
    if (!content.startsWith(u':'))
        return 0;

    qsizetype i = 1;
    while (i < content.size()) {
        const QChar c = content.at(i);
        if (c == u'\\') {
            i += 2;
            continue;
        }
        // An interpreted text role reads like the name of a field but takes
        // its text right behind it.
        if (c == u'`')
            return 0;
        if (c == u':')
            break;
        ++i;
    }

    if (i <= 1 || i >= content.size())
        return 0;
    if (i + 1 < content.size() && !content.at(i + 1).isSpace())
        return 0;
    return i;
}

bool Lexer::scanField(const Line &line)
{
    const qsizetype end = fieldNameEnd(line);
    if (end <= 0)
        return false;

    const QStringView content = line.content(_source);
    separate();
    Token &tk = push(Parser::T_FIELD_NAME, line);
    tk.name = content.sliced(1, end - 1);
    tk.text = content.sliced(qMin(end + 1, content.size())).trimmed();

    openValue(line, int(end) + 1);
    return true;
}

bool Lexer::scanListItem(const Line &line)
{
    const QStringView content = line.content(_source);
    const QChar first = content.at(0);

    int markerLength = 0;
    int kind = Parser::T_BULLET;

    if (first == u'-' || first == u'*' || first == u'+') {
        markerLength = 1;
    } else if (_blankPending || !isLine(_lastKind)) {
        // An enumerator only opens a list where a block may start: a line of
        // a paragraph that reads like one carries the paragraph on.
        qsizetype i = content.at(0) == u'(' ? 1 : 0;
        const qsizetype start = i;
        if (content.at(i) == u'#') {
            ++i;
        } else if (content.at(i).isDigit()) {
            while (i < content.size() && content.at(i).isDigit())
                ++i;
        } else if (content.at(i).isLetter()) {
            ++i;
        }
        if (i > start && i < content.size()
            && (content.at(i) == u'.' || content.at(i) == u')')) {
            markerLength = int(i) + 1;
            kind = Parser::T_ENUMERATOR;
        }
    }

    if (!markerLength)
        return false;
    if (markerLength < content.size() && !content.at(markerLength).isSpace())
        return false;

    separate();
    Token &tk = push(kind, line);
    tk.name = content.first(markerLength);

    openValue(line, markerLength);
    return true;
}

// What stands behind the marker of a list item, of a field or of a footnote
// label is the first line of the body of it, read the way any line that opens
// a block is: a directive written there is a directive and not its own text.
void Lexer::openValue(const Line &line, int markerLength, int indent)
{
    const QStringView content = line.content(_source);
    qsizetype start = markerLength;
    while (start < content.size() && content.at(start).isSpace())
        ++start;
    if (start >= content.size())
        return;

    Line value = line;
    value.start = value.textStart = line.textStart + int(start);
    value.indent = indent < 0 ? line.indent + int(start) : indent;

    openIndent(value.indent, line);
    scanBlockStart(value);
}

// A table is read as one token: its cells stand in columns that the rules
// above and below them set, which is a layout rather than a nesting, and the
// grammar has nothing to say about it.
bool Lexer::scanTable(const Line &line)
{
    const QStringView content = line.content(_source);
    const bool grid = gridTableRule(content) > 0;

    // One column of "=" is a transition or the underline of a title just as
    // much as it is a table, so a simple table is only read as one where it
    // sets more than one column.
    if (!grid && simpleTableRule(content) < 2)
        return false;

    // The table ends with the rule that closes it, which is the one a blank
    // line or the end of the block stands behind.
    Line last = line;
    bool closed = false;
    for (Line it = nextLine(line); it.start < _size; it = nextLine(it)) {
        if (it.blank) {
            if (closed)
                break;
            // A blank line that no rule stands before sets two rows of a
            // simple table apart.
            continue;
        }
        if (it.indent < line.indent)
            break;
        const QStringView text = it.content(_source);
        if (grid ? gridTableRule(text) > 0 : simpleTableRule(text) > 0) {
            last = it;
            closed = true;
        }
    }

    // What no rule closes is no table: the line that read as one opens a
    // paragraph like any other.
    if (!closed)
        return false;

    separate();
    Token &tk = push(Parser::T_TABLE, line);
    tk.length = last.end - line.textStart;
    tk.spelling = _source.mid(tk.position, tk.length);
    _blockEnd = last.end;
    _line = nextLine(last);
    return true;
}

bool Lexer::scanTitle(const Line &line)
{
    const int over = adornmentLength(line);
    Line titleLine = line;
    Line lastLine;
    bool overline = false;

    // A line of punctuation that stands on its own is only read as the
    // overline of a title where it is long enough to be one.
    if (over >= 4) {
        const Line title = nextLine(line);
        if (title.start >= _size || title.blank)
            return false;
        const Line under = nextLine(title);
        if (under.start >= _size || adornmentLength(under) != over)
            return false;
        if (under.content(_source).at(0) != line.content(_source).at(0))
            return false;
        titleLine = title;
        lastLine = under;
        overline = true;
    } else {
        if (line.blank)
            return false;
        const Line under = nextLine(line);
        if (under.start >= _size || under.indent != line.indent)
            return false;
        // An underline that is shorter than the title and too short to be
        // one on its own underlines nothing.
        const int length = adornmentLength(under);
        if (length == 0 || (length < 4 && length < line.end - line.textStart))
            return false;
        lastLine = under;
    }

    const QChar adornment = lastLine.content(_source).at(0);

    separate();
    const int level = openSection(adornment, overline);

    Token &tk = push(Parser::T_TITLE, titleLine);
    tk.text = titleLine.content(_source);
    tk.adornment = adornment;
    tk.level = level;

    _blockEnd = lastLine.end;
    Token &start = push(Parser::T_SECTION_START, lastLine.end, lastLine.number, 1);
    start.level = tk.level;

    _line = nextLine(lastLine);
    return true;
}

// An explicit markup block owns what is indented under it the way a directive
// owns its body, but nothing of a comment is markup.
Lexer::Line Lexer::commentEnd(const Line &line) const
{
    Line last = line;
    for (Line it = nextLine(line); it.start < _size; it = nextLine(it)) {
        if (it.blank)
            continue;
        if (it.indent <= line.indent)
            break;
        last = it;
    }
    return last;
}

Lexer::Line Lexer::continuationEnd(const Line &line) const
{
    Line last = line;
    for (Line it = nextLine(line); it.start < _size && !it.blank; it = nextLine(it)) {
        if (it.indent <= line.indent)
            break;
        last = it;
    }
    return last;
}

int Lexer::adornmentLength(const Line &line) const
{
    if (line.blank)
        return 0;

    const QStringView content = line.content(_source);
    const QChar first = content.at(0);
    if (!isAdornment(first))
        return 0;
    for (QChar c : content) {
        if (c != first)
            return 0;
    }
    return int(content.size());
}

qsizetype Lexer::unescapedIndexOf(QStringView text, QChar character)
{
    for (qsizetype i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == u'\\') {
            ++i;
            continue;
        }
        if (c == character)
            return i;
    }
    return -1;
}

QStringView Lexer::directiveType(QStringView name)
{
    const qsizetype colon = name.lastIndexOf(u':');
    return colon < 0 ? name : name.sliced(colon + 1);
}

QStringView Lexer::substitutionType(QStringView text)
{
    const qsizetype colons = text.indexOf("::"_L1);
    return directiveType(colons < 0 ? text : text.first(colons).trimmed());
}

int Lexer::openSection(QChar adornment, bool overline)
{
    for (size_t i = 0; i < _sections.size(); ++i) {
        if (_sections[i].character == adornment && _sections[i].overline == overline) {
            closeSections(int(i) + 1);
            break;
        }
    }
    _sections.push_back({adornment, overline});
    return int(_sections.size());
}

void Lexer::closeSections(int level)
{
    while (int(_sections.size()) >= level) {
        _sections.pop_back();
        push(Parser::T_SECTION_END, _blockEnd, _line.number, 1);
    }
}

void Lexer::openIndent(int indent, const Line &line)
{
    _indents.push_back(indent);
    Token &tk = push(Parser::T_INDENT, line.start, line.number, indent + 1);
    tk.indent = indent;
}

void Lexer::closeIndents(int indent)
{
    while (_indents.size() > 1 && currentIndent() > indent) {
        _indents.pop_back();
        Token &tk = push(Parser::T_DEDENT, _blockEnd, _line.number, indent + 1);
        tk.indent = currentIndent();
    }
}

void Lexer::separate(int nextKind)
{
    // Nothing is open where the last token was one of the markers, so
    // nothing has to be closed either.
    if (!isLine(_lastKind)) {
        _blankPending = false;
        return;
    }
    if (!_blankPending && continuesRun(_lastKind, nextKind))
        return;

    _blankPending = false;
    push(Parser::T_BLANK, _blockEnd, _line.number, 1);
}

Token &Lexer::push(int kind, const Line &line)
{
    Token &tk = push(kind, line.textStart, line.number, line.indent + 1);
    tk.indent = line.indent;
    tk.length = line.end - line.textStart;
    tk.spelling = _source.mid(tk.position, tk.length);
    _blockEnd = line.end;
    return tk;
}

Token &Lexer::push(int kind, int position, int lineNumber, int column)
{
    _queue.push_back(Token());
    Token &tk = _queue.back();
    tk.kind = kind;
    tk.position = position;
    tk.line = lineNumber;
    tk.column = column;
    _lastKind = kind;
    return tk;
}
