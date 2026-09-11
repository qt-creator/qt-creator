// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rstast.h"

#include "rstastvisitor.h"

#include <QStringList>

using namespace Qt::Literals::StringLiterals;

using namespace RstLang;

void AST::accept(Visitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void AST::accept(AST *ast, Visitor *visitor)
{
    if (ast)
        ast->accept(visitor);
}

QString LinesAST::text() const
{
    QStringList result;
    for (LineAST *line : lines())
        result.append(line->text().toString());
    return result.join(u' ');
}

bool ParagraphAST::opensLiteralBlock() const
{
    const LineAST *last = lines().last();
    return last && last->text().endsWith("::"_L1);
}

int LinesAST::indent() const
{
    int result = -1;
    for (LineAST *line : lines()) {
        if (line->text().isEmpty())
            continue;
        if (result < 0 || line->token.indent < result)
            result = line->token.indent;
    }
    return qMax(result, 0);
}

QString LinesAST::block() const
{
    const int base = indent();
    QStringList result;
    for (LineAST *line : lines()) {
        const QStringView text = line->text();
        // A line of the block that is indented deeper keeps what it has over
        // the block it stands in.
        const int extra = text.isEmpty() ? 0 : line->token.indent - base;
        result.append(QString(extra, u' ') + text.toString());
    }

    while (!result.isEmpty() && result.constLast().isEmpty())
        result.removeLast();
    return result.join(u'\n');
}

QString DefinitionItemAST::term() const
{
    QStringList result;
    for (LineAST *line : termLines())
        result.append(line->text().toString());
    return result.join(u' ');
}

QStringView DirectiveAST::domain() const
{
    const qsizetype colon = token.name.lastIndexOf(u':');
    return colon < 0 ? QStringView() : token.name.first(colon);
}

QStringView DirectiveAST::type() const
{
    const qsizetype colon = token.name.lastIndexOf(u':');
    return colon < 0 ? token.name : token.name.sliced(colon + 1);
}

bool DirectiveAST::isNamed(QAnyStringView spelling) const
{
    return QAnyStringView::compare(spelling, type(), Qt::CaseInsensitive) == 0;
}

// The fields a directive opens with are what configures it.  Everything from
// the first block that is not a field on is what it holds.
static List<BlockAST *> *skipOptions(List<BlockAST *> *blocks)
{
    List<BlockAST *> *it = blocks;
    while (it && it->value->asField())
        it = it->next;
    return it;
}

QList<FieldAST *> DirectiveAST::options() const
{
    QList<FieldAST *> result;
    for (List<BlockAST *> *it = blockList; it; it = it->next) {
        FieldAST *field = it->value->asField();
        if (!field)
            break;
        result.append(field);
    }
    return result;
}

ListView<BlockAST *> DirectiveAST::content() const
{
    return ListView<BlockAST *>(skipOptions(blockList));
}

QStringView DirectiveAST::option(QAnyStringView name) const
{
    for (FieldAST *field : options()) {
        if (QAnyStringView::compare(name, field->fieldName(), Qt::CaseInsensitive) == 0)
            return field->value();
    }
    return {};
}

QStringView SubstitutionAST::directiveType() const
{
    const qsizetype colons = token.text.indexOf("::"_L1);
    return colons < 0 ? token.text : token.text.first(colons).trimmed();
}

QStringView SubstitutionAST::argument() const
{
    const qsizetype colons = token.text.indexOf("::"_L1);
    return colons < 0 ? QStringView() : token.text.sliced(colons + 2).trimmed();
}

// The table the way it is written, the column it stands at kept, so that the
// rules and the cells below them line up.
static QStringList tableLines(const Token &token)
{
    const QString text = QString(token.indent, u' ') + token.spelling.toString();
    QStringList lines = text.split(u'\n');
    for (QString &line : lines) {
        while (line.endsWith(u'\r'))
            line.chop(1);
    }
    return lines;
}

// What a cell holds reads on as one text, so a line that carries a cell on
// stands behind what the line above it left.
static void carryOn(QString &cell, const QString &more)
{
    if (more.isEmpty())
        return;
    cell += cell.isEmpty() ? more : u' ' + more;
}

// A simple table sets its columns with the runs of "=" of its first rule, and
// every column but the last one ends where the next one starts.
static QList<QStringList> simpleTableRows(const QStringList &lines, int *headerRows)
{
    const QString &rule = lines.constFirst();
    QList<qsizetype> starts;
    for (qsizetype i = 0; i < rule.size(); ++i) {
        if (rule.at(i) == u'=' && (i == 0 || rule.at(i - 1) != u'='))
            starts.append(i);
    }

    const auto cells = [&starts](const QString &line) {
        QStringList result;
        for (qsizetype column = 0; column < starts.size(); ++column) {
            const qsizetype from = starts.at(column);
            const qsizetype to = column + 1 < starts.size() ? starts.at(column + 1)
                                                            : line.size();
            result.append(from < line.size() ? line.mid(from, to - from).trimmed() : QString());
        }
        return result;
    };

    QList<QStringList> rows;
    int rules = 0;
    int header = 0;
    bool opensRow = true;
    for (const QString &line : lines) {
        if (Lexer::simpleTableRule(line) > 0) {
            ++rules;
            // The rule below the heading is the second one the table draws,
            // and the one that closes it the third.
            if (rules == 2)
                header = int(rows.size());
            opensRow = true;
            continue;
        }
        if (line.trimmed().isEmpty()) {
            opensRow = true;
            continue;
        }

        const QStringList row = cells(line);
        // A line whose first column is empty says more about the row above it.
        if (opensRow || rows.isEmpty() || !row.constFirst().isEmpty()) {
            rows.append(row);
            opensRow = false;
            continue;
        }
        QStringList &last = rows.last();
        for (qsizetype column = 0; column < row.size() && column < last.size(); ++column)
            carryOn(last[column], row.at(column));
    }

    if (headerRows)
        *headerRows = rules >= 3 ? header : 0;
    return rows;
}

// A grid table sets its columns with the "+" of its first rule, and the rule
// that holds "=" instead of "-" is the one below its heading.
static QList<QStringList> gridTableRows(const QStringList &lines, int *headerRows)
{
    const QString &rule = lines.constFirst();
    QList<qsizetype> bounds;
    for (qsizetype i = 0; i < rule.size(); ++i) {
        if (rule.at(i) == u'+')
            bounds.append(i);
    }

    QList<QStringList> rows;
    QStringList row;
    int header = 0;
    for (const QString &line : lines) {
        if (Lexer::gridTableRule(line) > 0) {
            if (!row.isEmpty()) {
                rows.append(row);
                row.clear();
            }
            if (line.contains(u'='))
                header = int(rows.size());
            continue;
        }

        for (qsizetype column = 0; column + 1 < bounds.size(); ++column) {
            const qsizetype from = qMin(bounds.at(column) + 1, line.size());
            const qsizetype to = qMin(bounds.at(column + 1), line.size());
            const QString cell = from < to ? line.mid(from, to - from).trimmed() : QString();
            if (column < row.size())
                carryOn(row[column], cell);
            else
                row.append(cell);
        }
    }

    if (!row.isEmpty())
        rows.append(row);
    if (headerRows)
        *headerRows = header;
    return rows;
}

QList<QStringList> TableAST::rows(int *headerRows) const
{
    if (headerRows)
        *headerRows = 0;

    const QStringList lines = tableLines(token);
    if (lines.isEmpty())
        return {};
    if (Lexer::gridTableRule(lines.constFirst()) > 0)
        return gridTableRows(lines, headerRows);
    return simpleTableRows(lines, headerRows);
}

void DocumentAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this))
        accept(blockList, visitor);
    visitor->endVisit(this);
}

void LineAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

#define RSTLANG_LINES_ACCEPT(Name) \
    void Name##AST::accept0(Visitor *visitor) \
    { \
        if (visitor->visit(this)) \
            accept(lineList, visitor); \
        visitor->endVisit(this); \
    }

RSTLANG_LINES_ACCEPT(Paragraph)
RSTLANG_LINES_ACCEPT(LiteralBlock)
RSTLANG_LINES_ACCEPT(LineBlock)

#undef RSTLANG_LINES_ACCEPT

#define RSTLANG_BODY_ACCEPT(Name) \
    void Name##AST::accept0(Visitor *visitor) \
    { \
        if (visitor->visit(this)) \
            accept(blockList, visitor); \
        visitor->endVisit(this); \
    }

RSTLANG_BODY_ACCEPT(BlockQuote)
RSTLANG_BODY_ACCEPT(Section)
RSTLANG_BODY_ACCEPT(BulletItem)
RSTLANG_BODY_ACCEPT(EnumeratedItem)
RSTLANG_BODY_ACCEPT(Field)
RSTLANG_BODY_ACCEPT(Directive)
RSTLANG_BODY_ACCEPT(Substitution)
RSTLANG_BODY_ACCEPT(Footnote)

#undef RSTLANG_BODY_ACCEPT

void DefinitionItemAST::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(termList, visitor);
        accept(blockList, visitor);
    }
    visitor->endVisit(this);
}

void TargetAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void CommentAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void TransitionAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}

void TableAST::accept0(Visitor *visitor)
{
    visitor->visit(this);
    visitor->endVisit(this);
}
