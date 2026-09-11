// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rstdocument.h"

#include "rstastvisitor.h"
#include "rstparser.h"

#include <QStringList>

using namespace Qt::Literals::StringLiterals;

using namespace RstLang;

namespace {

class Collector: public Visitor
{
public:
    QList<SectionAST *> sections;
    QList<DirectiveAST *> directives;
    QHash<QString, SubstitutionAST *> substitutions;

    bool visit(SectionAST *ast) override
    {
        sections.append(ast);
        return true;
    }

    bool visit(DirectiveAST *ast) override
    {
        directives.append(ast);
        return true;
    }

    bool visit(SubstitutionAST *ast) override
    {
        substitutions.insert(ast->substitutionName().toString(), ast);
        return true;
    }

    // A literal block holds no markup, so nothing of it is collected.
    bool visit(LiteralBlockAST *) override { return false; }
};

} // namespace

// The lines of an included file stand where the directive that names it
// stands, so every one of them but the first carries its indentation.
static QString indented(const QString &text, int indent)
{
    QString trimmed = text;
    while (trimmed.endsWith(u'\n') || trimmed.endsWith(u'\r'))
        trimmed.chop(1);

    const QString prefix(indent, u' ');
    QStringList lines = trimmed.split(u'\n');
    for (qsizetype i = 1; i < lines.size(); ++i) {
        if (!lines[i].trimmed().isEmpty())
            lines[i].prepend(prefix);
    }
    return lines.join(u'\n');
}

namespace {

// Puts the text of every file an include directive names in the place of the
// directive.  A file that is included may include another, down to the depth
// the options allow; a file that is already being read is left standing, so
// that a cycle ends the expansion instead of feeding it.
class IncludeExpander
{
public:
    IncludeExpander(const ParseOptions &options, Engine *engine)
        : _options(options)
        , _engine(engine)
    {}

    QString expand(const QString &source);

private:
    // What stands in the way of expanding an include, at the place in the
    // expanded document where it is written.
    class Problem
    {
    public:
        int position = 0;
        QString message;
    };

    // The text of one file, every include it names spliced in.  Its first
    // character ends up at "base" of the document that is built.
    QString expanded(const QString &source, int base, int depth);

    void report(const QString &text) const;

    const ParseOptions &_options;
    Engine *_engine;

    // The files on the way to the one that is being read.
    QStringList _path;
    QList<Problem> _problems;
};

QString IncludeExpander::expand(const QString &source)
{
    if (!_options.resolveInclude || _options.includeDirectives.isEmpty())
        return source;

    const QString text = expanded(source, 0, 0);
    report(text);
    return text;
}

QString IncludeExpander::expanded(const QString &source, int base, int depth)
{
    const QStringView view(source);

    QString result;
    int last = 0;

    Engine engine;
    Lexer lexer(&engine, engine.setSource(source));
    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        if (token.isNot(Parser::T_DIRECTIVE))
            continue;
        const QString type = token.name.toString();
        if (!_options.includeDirectives.contains(type, Qt::CaseInsensitive))
            continue;

        const QString name = token.text.toString();
        result += view.sliced(last, token.position - last);
        const int position = base + int(result.size());

        // What is not expanded is left the way it is written, so that the
        // document still says which file belongs here.
        const auto keep = [&] { result += view.sliced(token.position, token.length); };
        last = token.end();

        const qsizetype seen = _path.indexOf(name);
        if (seen >= 0) {
            QStringList cycle = _path.mid(seen);
            cycle.append(name);
            _problems.append({position,
                              QStringLiteral("Include cycle: %1.").arg(cycle.join(" -> "_L1))});
            keep();
            continue;
        }

        if (depth >= _options.includeLimit) {
            _problems.append(
                {position,
                 QStringLiteral("Includes nest deeper than %1 files.").arg(_options.includeLimit)});
            keep();
            continue;
        }

        const std::optional<QString> included = _options.resolveInclude(type, name);
        if (!included) {
            keep();
            continue;
        }

        // The lines of the included file are indented before they are read, so
        // that what an include of its own names lands where it belongs.
        _path.append(name);
        result += expanded(indented(*included, token.indent), position, depth + 1);
        _path.removeLast();
    }

    result += view.sliced(last);
    return result;
}

// Where a problem stands is known as a position in the expanded document; the
// line and the column it reads as are what counting up to it says.
void IncludeExpander::report(const QString &text) const
{
    int line = 1;
    int column = 1;
    int at = 0;
    for (const Problem &problem : _problems) {
        for (; at < problem.position && at < text.size(); ++at) {
            if (text.at(at) == u'\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        _engine->warning(line, column, problem.position, 0, problem.message);
    }
}

} // namespace

DocumentPtr Document::fromSource(const QString &source, const ParseOptions &options)
{
    std::shared_ptr<Document> document(new Document);
    const QString text = IncludeExpander(options, &document->_engine).expand(source);
    Parser parser(&document->_engine, text);
    document->_ast = parser.parse();
    document->collect();
    return document;
}

QString Document::errorString() const
{
    QStringList messages;
    for (const Diagnostic &diagnostic : diagnostics()) {
        if (diagnostic.isError())
            messages.append(diagnostic.message);
    }
    return messages.join(u'\n');
}

DirectiveAST *Document::directive(QAnyStringView type, QAnyStringView argument) const
{
    for (DirectiveAST *candidate : _directives) {
        if (!candidate->isNamed(type))
            continue;
        if (argument.isEmpty()
            || QAnyStringView::compare(argument, candidate->argument()) == 0) {
            return candidate;
        }
    }
    return nullptr;
}

QHash<QString, QString> Document::substitutionTexts() const
{
    QHash<QString, QString> result;
    for (auto it = _substitutions.cbegin(); it != _substitutions.cend(); ++it) {
        SubstitutionAST *substitution = it.value();
        QStringList parts;
        if (!substitution->argument().isEmpty())
            parts.append(substitution->argument().toString());
        for (BlockAST *block : substitution->blocks()) {
            if (ParagraphAST *paragraph = block->asParagraph())
                parts.append(paragraph->text());
        }
        result.insert(it.key(), parts.join(u' '));
    }
    return result;
}

QStringView Document::title() const
{
    return _sections.isEmpty() ? QStringView() : _sections.first()->title();
}

void Document::collect()
{
    Collector collector;
    collector.accept(_ast);
    _sections = collector.sections;
    _directives = collector.directives;
    _substitutions = collector.substitutions;
}
