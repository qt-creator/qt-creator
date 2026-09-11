// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "rstlang.h"

#include <QString>

#include <deque>
#include <vector>

namespace RstLang {

// One line of the document, or one of the markers the lexer inserts to spell
// out the structure that the indentation of the source only implies.
class RSTLANG_EXPORT Token
{
public:
    int kind = 0;
    int position = 0;
    int length = 0;
    int line = 1;
    int column = 1;

    // The column the content of the line starts at, tabs expanded.  Set for
    // the tokens a line produces, and on INDENT for the level it opens.
    int indent = 0;

    // How deep the section a title opens sits, the outermost being one.
    int level = 0;

    // The whole line, markup included.
    QStringView spelling;

    // What the line names: the type of a directive, the name of a field, of a
    // hyperlink target or of a substitution, the marker of a list item.
    QStringView name;

    // What the line says: the text of a text line, the argument of a
    // directive, the value of a field, the link of a target, the title of a
    // section.
    QStringView text;

    // The character a section title is underlined with.
    QChar adornment;

    bool is(int k) const { return k == kind; }
    bool isNot(int k) const { return k != kind; }

    int begin() const { return position; }
    int end() const { return position + length; }
};

// Turns reStructuredText into a token stream the LALR grammar can read: one
// token per line, plus INDENT, DEDENT, SECTION_START and SECTION_END for the
// nesting that RST writes as indentation and as underlined titles, and BLANK
// where one block ends and the next begins.
//
// The stream the lexer hands out is well formed: every INDENT has its DEDENT,
// every SECTION_START its SECTION_END, and a BLANK stands wherever a block
// ends, whether the source spells the blank line out or not.  The grammar
// relies on all of that.
class RSTLANG_EXPORT Lexer
{
public:
    Lexer(Engine *engine, QStringView source);

    Engine *engine() const { return _engine; }

    int yylex(Token *tk);

    static const char *name(int kind);

    // Whether the body of a directive of this type is taken the way it
    // stands instead of being read as markup.
    static bool isVerbatimDirective(QStringView type);

    // How many columns the line draws the rule of a table over, zero where it
    // draws none: "=====  =====" for a simple table, "+-----+-----+" for a
    // grid one.  A table is read as one token, so what its cells are is read
    // off the same rules again where the table is rendered.
    static int simpleTableRule(QStringView line);
    static int gridTableRule(QStringView line);

private:
    class Line
    {
    public:
        int start = 0;      // The first character of the line.
        int textStart = 0;  // The first character that is not indentation.
        int end = 0;        // Past the last character, trailing space excluded.
        int next = 0;       // The first character of the line that follows.
        int number = 1;
        int indent = 0;
        bool blank = true;

        QStringView content(QStringView source) const
        {
            return source.mid(textStart, end - textStart);
        }
    };

    void fill();
    void finish();

    Line readLine(int position, int number) const;
    Line nextLine(const Line &line) const { return readLine(line.next, line.number + 1); }

    // By value: scanning a line moves the lexer on to the next one.
    void scanLine(Line line);
    void scanBlockStart(const Line &line);
    void scanText(const Line &line);
    void scanVerbatim(const Line &line);
    void scanExplicitMarkup(const Line &line);
    bool scanTitle(const Line &line);
    bool scanListItem(const Line &line);
    bool scanField(const Line &line);
    bool scanTable(const Line &line);

    // What stands behind the marker of a list item, of a field or of a
    // footnote label opens the body of it, as the first line of that body.
    // The body sits at the column the line is written at, unless the markup
    // sets one of its own.
    void openValue(const Line &line, int markerLength, int indent = -1);

    bool isFieldLine(const Line &line) const;
    qsizetype fieldNameEnd(const Line &line) const;

    // The line an explicit markup block starts on owns the lines indented
    // under it.  Returns the last of them.
    Line commentEnd(const Line &line) const;

    // A line of a line block carries on over the lines indented under it.
    // Returns the last of them.
    Line continuationEnd(const Line &line) const;

    int adornmentLength(const Line &line) const;

    static qsizetype unescapedIndexOf(QStringView text, QChar character);
    static QStringView directiveType(QStringView name);
    static QStringView substitutionType(QStringView text);

    int openSection(QChar adornment, bool overline);
    void closeSections(int level);

    void openIndent(int indent, const Line &line);
    void closeIndents(int indent);

    // Ends the block that is open, if one is.  The grammar reads a BLANK as
    // the end of a paragraph and as the end of the body of a construct, so
    // one stands wherever a block ends, whether the source spells it out or
    // not.
    void separate(int nextKind = -1);

    Token &push(int kind, const Line &line);
    Token &push(int kind, int position, int lineNumber, int column);

    bool inVerbatim() const { return _verbatimIndent >= 0; }
    int currentIndent() const { return _indents.back(); }

    Engine *_engine = nullptr;
    QStringView _source;
    int _size = 0;

    Line _line;
    std::deque<Token> _queue;
    std::vector<int> _indents;

    class Adornment
    {
    public:
        QChar character;
        bool overline = false;
    };
    std::vector<Adornment> _sections;

    int _lastKind = -1;

    // Where the block ends that the markers the lexer inserts stand behind.
    int _blockEnd = 0;

    // The column of the literal block that is being read, negative outside
    // of one.
    int _verbatimIndent = -1;

    // The column of the body of the verbatim directive that is being read.
    // Its lines are literal, but for the fields it opens with.
    int _verbatimBodyIndent = -1;

    // Set by a paragraph that ends in "::": what is indented under it is a
    // literal block.
    bool _literalFollows = false;

    // Set by a directive whose body is taken verbatim, until the body opens.
    bool _verbatimDirective = false;

    bool _blankPending = false;
    bool _done = false;
};

} // namespace RstLang
