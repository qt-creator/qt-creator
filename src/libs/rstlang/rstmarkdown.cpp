// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rstmarkdown.h"

#include "rstastvisitor.h"

#include <QStringList>

using namespace Qt::Literals::StringLiterals;

using namespace RstLang;

namespace {

// The directives that say how the documentation is built rather than what it
// says.  Nothing of them is shown.
bool isSkippedDirective(DirectiveAST *directive)
{
    static const QLatin1StringView types[] = {"only"_L1,
                                              "toctree"_L1,
                                              "contents"_L1,
                                              "index"_L1,
                                              "highlight"_L1,
                                              "include"_L1,
                                              "replace"_L1,
                                              "unicode"_L1,
                                              "date"_L1,
                                              "meta"_L1};
    for (QLatin1StringView type : types) {
        if (directive->isNamed(type))
            return true;
    }
    return false;
}

// The directives that hold code.
bool isCodeDirective(DirectiveAST *directive)
{
    static const QLatin1StringView types[] = {"code"_L1,
                                              "code-block"_L1,
                                              "sourcecode"_L1,
                                              "parsed-literal"_L1,
                                              "literalinclude"_L1};
    for (QLatin1StringView type : types) {
        if (directive->isNamed(type))
            return true;
    }
    return false;
}

// The directives that set what they hold apart from the text around them.
QString admonitionLabel(DirectiveAST *directive)
{
    static const struct
    {
        QLatin1StringView type;
        QLatin1StringView label;
    } admonitions[] = {{"note"_L1, "Note"_L1},
                       {"warning"_L1, "Warning"_L1},
                       {"tip"_L1, "Tip"_L1},
                       {"important"_L1, "Important"_L1},
                       {"caution"_L1, "Caution"_L1},
                       {"attention"_L1, "Attention"_L1},
                       {"danger"_L1, "Danger"_L1},
                       {"error"_L1, "Error"_L1},
                       {"hint"_L1, "Hint"_L1},
                       {"seealso"_L1, "See also"_L1},
                       {"admonition"_L1, ""_L1}};
    for (const auto &admonition : admonitions) {
        if (directive->isNamed(admonition.type))
            return admonition.label;
    }
    return {};
}

// The directives that say which version of CMake something belongs to.
QString versionLabel(DirectiveAST *directive)
{
    if (directive->isNamed("versionadded"_L1))
        return QStringLiteral("Added in version %1").arg(directive->argument());
    if (directive->isNamed("versionchanged"_L1))
        return QStringLiteral("Changed in version %1").arg(directive->argument());
    if (directive->isNamed("deprecated"_L1))
        return QStringLiteral("Deprecated since version %1").arg(directive->argument());
    return {};
}

// The directives CMake documents one of its own names with.  What they hold
// belongs to the name they carry.
bool isDefinitionDirective(DirectiveAST *directive)
{
    static const QLatin1StringView types[] = {"command"_L1,    "variable"_L1,  "envvar"_L1,
                                              "module"_L1,     "policy"_L1,    "genex"_L1,
                                              "prop_tgt"_L1,   "prop_dir"_L1,  "prop_sf"_L1,
                                              "prop_test"_L1,  "prop_gbl"_L1,  "prop_inst"_L1,
                                              "prop_cache"_L1, "manual"_L1,    "cpack_gen"_L1,
                                              "guide"_L1,      "signature"_L1, "function"_L1,
                                              "macro"_L1};
    for (QLatin1StringView type : types) {
        if (directive->isNamed(type))
            return true;
    }
    return false;
}

// The roles that name something the reader can look up.  Their text is the
// name, which reads best as code.
bool isCodeRole(QStringView role)
{
    const qsizetype colon = role.lastIndexOf(u':');
    if (colon >= 0)
        role = role.sliced(colon + 1);

    static const QLatin1StringView plain[] = {"ref"_L1,
                                              "doc"_L1,
                                              "term"_L1,
                                              "abbr"_L1,
                                              "title-reference"_L1,
                                              "emphasis"_L1};
    for (QLatin1StringView candidate : plain) {
        if (role.compare(candidate, Qt::CaseInsensitive) == 0)
            return false;
    }
    return true;
}

// Whether Markdown would read the character as markup where the text means
// it the way it stands.
bool needsEscape(QStringView text, qsizetype index)
{
    static const QLatin1StringView special("\\`*_<>[]");
    const QChar c = text.at(index);
    if (!special.contains(c))
        return false;

    // An underscore inside a word is part of the word to Markdown as well.
    if (c == u'_') {
        return !(index > 0 && text.at(index - 1).isLetterOrNumber()
                 && index + 1 < text.size() && text.at(index + 1).isLetterOrNumber());
    }
    return true;
}

QString escaped(QStringView text)
{
    QString result;
    result.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (needsEscape(text, i))
            result += u'\\';
        result += text.at(i);
    }
    return result;
}

QString codeSpan(QStringView text)
{
    const QLatin1StringView fence = text.contains(u'`') ? "``"_L1 : "`"_L1;
    const QLatin1StringView pad = text.startsWith(u'`') || text.endsWith(u'`') ? " "_L1 : ""_L1;
    return fence + pad + text.toString() + pad + fence;
}

// What a role or a reference points at stands behind its text, in angle
// brackets.
QStringView referenceText(QStringView text)
{
    const qsizetype angle = text.lastIndexOf(u'<');
    if (angle <= 0 || !text.endsWith(u'>'))
        return text;
    return text.first(angle).trimmed();
}

QString prefixed(const QString &text, const QString &first, const QString &rest)
{
    QStringList lines = text.split(u'\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const QString &prefix = i == 0 ? first : rest;
        // A prefix of spaces says nothing on a line that holds nothing.
        if (lines[i].isEmpty() && prefix.trimmed().isEmpty())
            continue;
        lines[i].prepend(prefix);
    }
    return lines.join(u'\n');
}

// A substitution may stand for text that names another one, so how deep
// they nest is bounded.
enum { SubstitutionLimit = 8 };

// Whether the markup is turned into the markup of Markdown, or taken off
// the text.  A parsed literal carries markup but stands the way it is
// written, so nothing may be added to it.
enum Markup { AsMarkdown, AsPlainText };

QString inlined(QStringView text, const MarkdownOptions &options, int depth,
                Markup markup = AsMarkdown)
{
    const auto literal = [markup](QStringView text) {
        return markup == AsMarkdown ? codeSpan(text) : text.toString();
    };
    const auto plain = [markup](QStringView text) {
        return markup == AsMarkdown ? escaped(text) : text.toString();
    };

    QString result;
    result.reserve(text.size());

    qsizetype i = 0;
    const qsizetype size = text.size();
    while (i < size) {
        const QChar c = text.at(i);

        if (c == u'\\' && i + 1 < size) {
            result += plain(text.sliced(i + 1, 1));
            i += 2;
            continue;
        }

        // Inline literal: ``the text``.
        if (c == u'`' && i + 1 < size && text.at(i + 1) == u'`') {
            const qsizetype end = text.indexOf("``"_L1, i + 2);
            if (end > 0) {
                result += literal(text.sliced(i + 2, end - i - 2));
                i = end + 2;
                continue;
            }
        }

        // Interpreted text with a role: :the role:`the text`.  The role may
        // name the domain it belongs to, as in ":cmake:command:".
        if (c == u':') {
            qsizetype end = i + 1;
            while (end < size && (text.at(end).isLetterOrNumber() || text.at(end) == u'-'
                                  || text.at(end) == u'_' || text.at(end) == u'+'
                                  || text.at(end) == u'.' || text.at(end) == u':')) {
                ++end;
            }
            if (end > i + 2 && end < size && text.at(end) == u'`'
                && text.at(end - 1) == u':') {
                const qsizetype close = text.indexOf(u'`', end + 1);
                if (close > 0) {
                    const QStringView role = text.sliced(i + 1, end - i - 2);
                    const QStringView content = referenceText(
                        text.sliced(end + 1, close - end - 1));
                    result += isCodeRole(role) ? literal(content) : plain(content);
                    i = close + 1;
                    continue;
                }
            }
        }

        // Interpreted text and a reference: `the text` and `the text`_.
        if (c == u'`') {
            const qsizetype close = text.indexOf(u'`', i + 1);
            if (close > 0) {
                const QStringView content = referenceText(text.sliced(i + 1, close - i - 1));
                result += plain(content);
                i = close + 1;
                if (i < size && text.at(i) == u'_')
                    ++i;
                continue;
            }
        }

        if (c == u'*') {
            const bool strong = i + 1 < size && text.at(i + 1) == u'*';
            const QLatin1StringView marker = strong ? "**"_L1 : "*"_L1;
            const qsizetype from = i + marker.size();
            const qsizetype close = text.indexOf(marker, from);
            if (close > from) {
                const QString inner = inlined(text.sliced(from, close - from), options, depth,
                                              markup);
                result += markup == AsMarkdown ? marker + inner + marker : inner;
                i = close + marker.size();
                continue;
            }
        }

        // A reference to a footnote or to a citation: [the label]_.  What it
        // points at stands where the document defines it.
        if (c == u'[') {
            const qsizetype close = text.indexOf(u']', i + 1);
            if (close > i + 1 && close + 1 < size && text.at(close + 1) == u'_') {
                result += plain(text.sliced(i, close - i + 1));
                i = close + 2;
                continue;
            }
        }

        // A reference to a substitution: |the name|.
        if (c == u'|') {
            const qsizetype close = text.indexOf(u'|', i + 1);
            if (close > i + 1) {
                const QString name = text.sliced(i + 1, close - i - 1).toString();
                const auto substitution = options.substitutions.constFind(name);
                if (depth < SubstitutionLimit && substitution != options.substitutions.cend()) {
                    result += inlined(*substitution, options, depth + 1, markup);
                    i = close + 1;
                    if (i < size && text.at(i) == u'_')
                        ++i;
                    continue;
                }
            }
        }

        if (markup == AsMarkdown && needsEscape(text, i))
            result += u'\\';
        result += c;
        ++i;
    }

    return result;
}

// A parsed literal stands the way it is written, but what it carries is
// markup: what a reference points at is dropped, what it says is kept.
QString literalText(LiteralBlockAST *literal, DirectiveAST *directive,
                    const MarkdownOptions &options = {})
{
    if (directive && directive->isNamed("parsed-literal"_L1))
        return inlineToPlainText(literal->block(), options);
    return literal->block();
}

class Writer
{
public:
    explicit Writer(const MarkdownOptions &options)
        : _options(options)
    {}

    QString write(ListView<BlockAST *> blocks) const
    {
        QStringList chunks;
        for (BlockAST *block : blocks) {
            const QString chunk = writeBlock(block);
            if (!chunk.isEmpty())
                chunks.append(chunk);
        }
        return chunks.join("\n\n"_L1);
    }

    QString write(const QList<BlockAST *> &blocks) const
    {
        QStringList chunks;
        for (BlockAST *block : blocks) {
            const QString chunk = writeBlock(block);
            if (!chunk.isEmpty())
                chunks.append(chunk);
        }
        return chunks.join("\n\n"_L1);
    }

private:
    QString writeBlock(BlockAST *block) const;
    QString writeDirective(DirectiveAST *directive) const;
    QString writeTable(TableAST *table) const;

    QString inlined(QStringView text) const { return inlineToMarkdown(text, _options); }

    QString code(const QString &text, QStringView language) const
    {
        const QString marker = language.isEmpty() ? _options.defaultLanguage
                                                  : language.toString();
        if (text.isEmpty())
            return {};
        return "```"_L1 + marker + u'\n' + text + "\n```"_L1;
    }

    QString heading(int level, const QString &text) const
    {
        return QString(qBound(1, level, 6), u'#') + u' ' + text;
    }

    const MarkdownOptions &_options;
};

QString Writer::writeBlock(BlockAST *block) const
{
    if (ParagraphAST *paragraph = block->asParagraph()) {
        QString text = paragraph->text();
        // The "::" that opens a literal block is not part of what the
        // paragraph says.
        if (paragraph->opensLiteralBlock()) {
            text.chop(1);
            if (text.trimmed() == ":"_L1)
                return {};
        }
        return inlined(text);
    }

    if (LiteralBlockAST *literal = block->asLiteralBlock())
        return code(literal->block(), {});

    if (LineBlockAST *lineBlock = block->asLineBlock()) {
        QStringList lines;
        for (LineAST *line : lineBlock->lines()) {
            // A line of a line block may be written over several lines of
            // the source, and still is one line.
            lines.append(inlined(line->text().toString().simplified()));
        }
        return lines.join("  \n"_L1);
    }

    if (SectionAST *section = block->asSection()) {
        QStringList chunks{heading(_options.headingLevel + section->level() - 1,
                                   inlined(section->title()))};
        const QString body = write(section->blocks());
        if (!body.isEmpty())
            chunks.append(body);
        return chunks.join("\n\n"_L1);
    }

    if (DefinitionItemAST *definition = block->asDefinitionItem()) {
        QString chunk = "- "_L1 + inlined(definition->term());
        const QString body = write(definition->blocks());
        if (!body.isEmpty())
            chunk += u'\n' + prefixed(body, "  "_L1, "  "_L1);
        return chunk;
    }

    if (BulletItemAST *bullet = block->asBulletItem())
        return prefixed(write(bullet->blocks()), "- "_L1, "  "_L1);

    if (EnumeratedItemAST *enumerated = block->asEnumeratedItem()) {
        const QString marker = enumerated->marker().toString() + u' ';
        return prefixed(write(enumerated->blocks()), marker, QString(marker.size(), u' '));
    }

    if (BlockQuoteAST *quote = block->asBlockQuote())
        return prefixed(write(quote->blocks()), "> "_L1, "> "_L1);

    if (FieldAST *field = block->asField()) {
        QString chunk = "**"_L1 + inlined(field->fieldName()) + "**"_L1;
        const QString body = write(field->blocks());
        if (!body.isEmpty())
            chunk += ": "_L1 + body.trimmed();
        return chunk;
    }

    if (FootnoteAST *footnote = block->asFootnote()) {
        QString chunk = "\\["_L1 + inlined(footnote->label()) + "\\]"_L1;
        const QString body = write(footnote->blocks());
        if (!body.isEmpty())
            chunk += u'\n' + body;
        return chunk;
    }

    if (TableAST *table = block->asTable())
        return writeTable(table);

    if (DirectiveAST *directive = block->asDirective())
        return writeDirective(directive);

    if (block->asTransition())
        return "---"_L1;

    // A substitution, a hyperlink target and a comment say nothing where the
    // documentation is read.
    return {};
}

// Markdown draws a table the way GitHub spells one, which asks for a heading:
// where reStructuredText draws none, the heading stays empty.
QString Writer::writeTable(TableAST *table) const
{
    int headerRows = 0;
    const QList<QStringList> rows = table->rows(&headerRows);
    if (rows.isEmpty())
        return {};

    qsizetype columns = 0;
    for (const QStringList &row : rows)
        columns = qMax(columns, row.size());
    if (columns == 0)
        return {};

    const auto line = [&](const QStringList &row) {
        QStringList cells;
        for (qsizetype column = 0; column < columns; ++column) {
            const QString cell = column < row.size() ? row.at(column) : QString();
            // A cell stands on one line, and a bar in it would close it.
            cells.append(inlined(cell).replace(u'|', "\\|"_L1).simplified());
        }
        return u'|' + cells.join(u'|') + u'|';
    };

    QStringList result;
    result.append(headerRows > 0 ? line(rows.constFirst()) : line({}));
    result.append(u'|' + QStringList(columns, QStringLiteral("---")).join(u'|') + u'|');
    for (qsizetype i = headerRows > 0 ? 1 : 0; i < rows.size(); ++i)
        result.append(line(rows.at(i)));
    return result.join(u'\n');
}

QString Writer::writeDirective(DirectiveAST *directive) const
{
    if (isSkippedDirective(directive))
        return {};

    if (isCodeDirective(directive)) {
        QStringList chunks;
        for (BlockAST *block : directive->content()) {
            if (LiteralBlockAST *literal = block->asLiteralBlock())
                chunks.append(code(literalText(literal, directive, _options),
                                   directive->argument()));
        }
        return chunks.join("\n\n"_L1);
    }

    const QString version = versionLabel(directive);
    if (!version.isEmpty()) {
        QString text = "**"_L1 + version + "**"_L1;
        const QString body = write(directive->content());
        if (!body.isEmpty())
            text += u'\n' + body;
        return prefixed(text, "> "_L1, "> "_L1);
    }

    const QString label = admonitionLabel(directive);
    if (!label.isEmpty() || directive->isNamed("admonition"_L1)) {
        const QString title = label.isEmpty() ? directive->argument().toString() : label;
        QString text = "**"_L1 + title + "**"_L1;
        if (!label.isEmpty() && !directive->argument().isEmpty())
            text += u' ' + inlined(directive->argument());
        const QString body = write(directive->content());
        if (!body.isEmpty())
            text += u'\n' + body;
        return prefixed(text, "> "_L1, "> "_L1);
    }

    // CMake writes the signatures of a command out one by one, each with
    // what its arguments mean.  The call stands on the directive itself or
    // on the lines below it.
    if (directive->isNamed("signature"_L1)) {
        QList<BlockAST *> body;
        for (BlockAST *block : directive->content())
            body.append(block);

        QString call = directive->argument().toString();
        if (call.isEmpty() && !body.isEmpty()) {
            if (ParagraphAST *paragraph = body.constFirst()->asParagraph()) {
                call = paragraph->block();
                body.removeFirst();
            }
        }

        QStringList chunks;
        if (!call.isEmpty())
            chunks.append(code(call, {}));
        const QString text = write(body);
        if (!text.isEmpty())
            chunks.append(text);
        return chunks.join("\n\n"_L1);
    }

    if (isDefinitionDirective(directive)) {
        QStringList chunks;
        if (!directive->argument().isEmpty()) {
            chunks.append(heading(_options.headingLevel + 1,
                                  codeSpan(directive->argument())));
        }
        const QString body = write(directive->content());
        if (!body.isEmpty())
            chunks.append(body);
        return chunks.join("\n\n"_L1);
    }

    QStringList chunks;
    if (!directive->argument().isEmpty())
        chunks.append(inlined(directive->argument()));
    const QString body = write(directive->content());
    if (!body.isEmpty())
        chunks.append(body);
    return chunks.join("\n\n"_L1);
}

// What a tooltip has room for.
class Brief: public Visitor
{
public:
    QString title;
    QString description;
    QString codeText;
    QStringView codeLanguage;

    bool done() const { return !title.isEmpty() && !description.isEmpty() && !codeText.isEmpty(); }

    bool preVisit(AST *) override { return !done(); }

    bool visit(SectionAST *ast) override
    {
        if (title.isEmpty())
            title = ast->title().toString();
        return true;
    }

    bool visit(ParagraphAST *ast) override
    {
        // The call a signature directive spells out below itself is the
        // signature of the command, not what it does.
        if (_signatureFollows) {
            _signatureFollows = false;
            codeText = ast->block();
            return false;
        }
        if (description.isEmpty() && !ast->opensLiteralBlock())
            description = ast->text();
        return false;
    }

    bool visit(LiteralBlockAST *ast) override
    {
        if (codeText.isEmpty())
            codeText = literalText(ast, _code, options);
        _code = nullptr;
        return false;
    }

    bool visit(DirectiveAST *ast) override
    {
        if (isSkippedDirective(ast))
            return false;

        if (ast->isNamed("signature"_L1)) {
            if (codeText.isEmpty()) {
                codeText = ast->argument().toString();
                _signatureFollows = codeText.isEmpty();
            }
            return true;
        }

        if (isCodeDirective(ast) && codeText.isEmpty()) {
            codeLanguage = ast->argument();
            _code = ast;
        }
        if (isDefinitionDirective(ast) && title.isEmpty())
            title = ast->argument().toString();
        return true;
    }

    // What a substitution stands for belongs where it is used, not where it
    // is written.
    bool visit(SubstitutionAST *) override { return false; }

    MarkdownOptions options;

private:
    bool _signatureFollows = false;

    // The directive the block of code that is being read belongs to.
    DirectiveAST *_code = nullptr;
};

} // namespace

QString RstLang::toMarkdown(ListView<BlockAST *> blocks, const MarkdownOptions &options)
{
    return Writer(options).write(blocks);
}

QString RstLang::toMarkdown(const QList<BlockAST *> &blocks, const MarkdownOptions &options)
{
    return Writer(options).write(blocks);
}

QString RstLang::toMarkdown(const DocumentPtr &document, const MarkdownOptions &options)
{
    if (!document)
        return {};

    MarkdownOptions merged = options;
    const QHash<QString, QString> own = document->substitutionTexts();
    for (auto it = own.cbegin(); it != own.cend(); ++it) {
        if (!merged.substitutions.contains(it.key()))
            merged.substitutions.insert(it.key(), it.value());
    }
    return toMarkdown(document->ast()->blocks(), merged);
}

QString RstLang::briefMarkdown(AST *node, const MarkdownOptions &options)
{
    if (!node)
        return {};

    Brief brief;
    brief.options = options;
    brief.accept(node);

    QStringList chunks;
    if (!brief.title.isEmpty()) {
        chunks.append(QString(qBound(1, options.headingLevel, 6), u'#') + u' '
                      + inlineToMarkdown(brief.title, options));
    }
    if (!brief.description.isEmpty())
        chunks.append(inlineToMarkdown(brief.description, options));
    if (!brief.codeText.isEmpty()) {
        const QString language = brief.codeLanguage.isEmpty()
                                     ? options.defaultLanguage
                                     : brief.codeLanguage.toString();
        chunks.append("```"_L1 + language + u'\n' + brief.codeText + "\n```"_L1);
    }
    return chunks.join("\n\n"_L1);
}

QString RstLang::briefMarkdown(const DocumentPtr &document, const MarkdownOptions &options)
{
    if (!document)
        return {};

    MarkdownOptions merged = options;
    const QHash<QString, QString> own = document->substitutionTexts();
    for (auto it = own.cbegin(); it != own.cend(); ++it) {
        if (!merged.substitutions.contains(it.key()))
            merged.substitutions.insert(it.key(), it.value());
    }
    return briefMarkdown(document->ast(), merged);
}

QString RstLang::inlineToMarkdown(QStringView text, const MarkdownOptions &options)
{
    return inlined(text, options, 0);
}

QString RstLang::inlineToPlainText(QStringView text, const MarkdownOptions &options)
{
    return inlined(text, options, 0, AsPlainText);
}
