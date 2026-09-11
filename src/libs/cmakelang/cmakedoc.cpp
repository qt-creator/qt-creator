// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakedoc.h"

#include "cmakelexer.h"
#include "cmakeparser.h"

#include <rstlang/rstastvisitor.h>
#include <rstlang/rstmarkdown.h>

#include <QRegularExpression>
#include <QStringList>

using namespace Qt::Literals::StringLiterals;

using namespace CMakeLang;

// The marker that tells a comment of a CMake file apart from one that
// carries documentation.
static const QLatin1StringView markerString(".rst:");

namespace {

// What each of the directives CMake documents a name with documents.
class DefinitionDirective
{
public:
    QLatin1StringView type;
    Documentation::Kind kind;
};

const DefinitionDirective definitionDirectives[] = {
    {"command"_L1, Documentation::Command},
    {"function"_L1, Documentation::Command},
    {"macro"_L1, Documentation::Command},
    {"variable"_L1, Documentation::Variable},
    {"envvar"_L1, Documentation::EnvironmentVariable},
    {"prop_tgt"_L1, Documentation::Property},
    {"prop_dir"_L1, Documentation::Property},
    {"prop_sf"_L1, Documentation::Property},
    {"prop_test"_L1, Documentation::Property},
    {"prop_gbl"_L1, Documentation::Property},
    {"prop_inst"_L1, Documentation::Property},
    {"prop_cache"_L1, Documentation::Property},
    {"policy"_L1, Documentation::Policy},
    {"module"_L1, Documentation::Module},
    {"genex"_L1, Documentation::GeneratorExpression},
};

Documentation::Kind definitionKind(RstLang::DirectiveAST *directive)
{
    for (const DefinitionDirective &candidate : definitionDirectives) {
        if (directive->isNamed(candidate.type))
            return candidate.kind;
    }
    return Documentation::Unknown;
}

// One signature directive may carry several calls, one to a line, which
// share what is said about them.  A call that is too long for one line
// carries on over the lines below it.
QStringList splitCalls(const QString &text, const QString &name)
{
    const QString opening = name + u'(';
    QStringList calls;
    for (const QString &line : text.split(u'\n')) {
        if (calls.isEmpty() || line.startsWith(opening, Qt::CaseInsensitive))
            calls.append(line);
        else
            calls.last() += u'\n' + line;
    }
    return calls;
}

// The lines a block is written over, the indentation it stands at removed.
QString rawBlock(QStringView source, RstLang::AST *node)
{
    QStringList lines = source.mid(node->position, node->length).toString().split(u'\n');
    const qsizetype indent = node->column - 1;
    for (qsizetype i = 1; i < lines.size(); ++i) {
        qsizetype strip = 0;
        while (strip < indent && strip < lines[i].size() && lines[i].at(strip) == u' ')
            ++strip;
        lines[i] = lines[i].sliced(strip);
    }

    while (!lines.isEmpty() && lines.constLast().trimmed().isEmpty())
        lines.removeLast();
    return lines.join(u'\n');
}

// The calls a signature directive spells out.  They stand on the directive
// itself or on the lines below it; what is left of the body says what they
// mean.
QStringList signatureCalls(QStringView source,
                           RstLang::DirectiveAST *directive,
                           const QString &name,
                           QList<RstLang::BlockAST *> *body = nullptr)
{
    QList<RstLang::BlockAST *> blocks;
    for (RstLang::BlockAST *block : directive->content())
        blocks.append(block);

    QString call = directive->argument().toString();
    if (call.isEmpty() && !blocks.isEmpty()) {
        // The call stands on the lines below the directive, the way it is
        // written: a call that is too long for one line carries on over the
        // lines below it, which reads as markup of its own.
        call = rawBlock(source, blocks.constFirst());
        blocks.removeFirst();
    }

    if (body)
        *body = blocks;
    return call.isEmpty() ? QStringList() : splitCalls(call, name);
}

// The keyword a call of a command opens with, which is what tells the
// signatures of a command like file() apart.  Empty where the first
// argument is a value rather than a keyword.
QString keywordOf(const QString &call, const QString &name)
{
    const QString opening = name + u'(';
    if (!call.startsWith(opening, Qt::CaseInsensitive))
        return {};

    qsizetype end = opening.size();
    while (end < call.size()
           && (call.at(end).isUpper() || call.at(end).isDigit() || call.at(end) == u'_')) {
        ++end;
    }
    if (end - opening.size() < 2)
        return {};
    // What follows the keyword is another argument, not more of it.
    if (end < call.size() && !call.at(end).isSpace() && call.at(end) != u')')
        return {};
    return call.mid(opening.size(), end - opening.size());
}

// Whether a heading or a paragraph announces an example.  CMake writes
// "Example:" over a block that shows a command in use, and numbers them
// where it shows several.
bool announcesExample(QStringView text)
{
    QStringView words = text.trimmed();
    while (!words.isEmpty() && !words.back().isLetterOrNumber())
        words.chop(1);

    static const QLatin1StringView example("example");
    if (words.endsWith(example, Qt::CaseInsensitive)
        || words.endsWith("examples"_L1, Qt::CaseInsensitive)) {
        return true;
    }

    if (!words.startsWith(example, Qt::CaseInsensitive))
        return false;
    words = words.sliced(example.size());
    if (words.startsWith(u's', Qt::CaseInsensitive))
        words = words.sliced(1);
    return words.isEmpty() || !words.front().isLetter();
}

// A call says how the command is called where its arguments stand for
// arguments.  One that quotes a string, expands a variable or carries a
// comment is a call the documentation makes, not the one it spells out.
bool spellsOutArguments(QStringView call)
{
    const qsizetype opening = call.indexOf(u'(');
    const qsizetype closing = call.lastIndexOf(u')');
    if (opening < 0 || closing < opening)
        return false;

    const QStringView arguments = call.sliced(opening + 1, closing - opening - 1);
    return !arguments.contains(u'"') && !arguments.contains(u'#')
           && !arguments.contains("${"_L1);
}

class Signatures: public RstLang::Visitor
{
public:
    Signatures(QStringView source, const QString &name,
               const RstLang::MarkdownOptions &options)
        : _source(source)
        , _name(name)
        , _options(options)
    {}

    // What the documentation spells out beats what it shows by example.
    QStringList result() const { return _declared.isEmpty() ? _shown : _declared; }

private:
    // CMake writes the signatures of a command out one by one, each with
    // what its arguments mean.
    bool visit(RstLang::DirectiveAST *ast) override
    {
        if (!ast->isNamed("signature"))
            return true;

        for (const QString &call : signatureCalls(_source, ast, _name))
            addDeclared(call);
        return true;
    }

    // What stands under a heading that announces an example is one.
    bool visit(RstLang::SectionAST *ast) override
    {
        if (!_examples && announcesExample(ast->title()))
            _examples = ast;
        return true;
    }

    void endVisit(RstLang::SectionAST *ast) override
    {
        if (_examples == ast)
            _examples = nullptr;
    }

    // The paragraph in front of a block of code says what the block shows.
    bool visit(RstLang::ParagraphAST *ast) override
    {
        _opening = ast->text();
        return true;
    }

    // A block that shows how the command is used may call it several
    // times, once to a line.
    bool visit(RstLang::LiteralBlockAST *ast) override
    {
        if (_examples || announcesExample(_opening))
            return false;

        for (const QString &call : splitCalls(ast->block(), _name))
            addShown(call);
        return false;
    }

    void addDeclared(const QString &call)
    {
        const QString text = plainCall(call);
        if (callsTheCommand(text))
            _declared.append(text);
    }

    // What a block of code shows is the way the command is called only
    // where it stands for any call of it.
    void addShown(const QString &call)
    {
        const QString text = plainCall(call);
        if (callsTheCommand(text) && spellsOutArguments(text))
            _shown.append(text);
    }

    // A call carries markup of its own: a substitution stands for the name
    // of the command, and a reference for what it points at.
    QString plainCall(const QString &call) const
    {
        return RstLang::inlineToPlainText(call, _options).trimmed();
    }

    // The code says how the command is called where it is a call of it
    // and nothing else.  Everything else is an example of using it.
    bool callsTheCommand(QStringView code) const
    {
        if (!code.startsWith(_name, Qt::CaseInsensitive) || !code.endsWith(u')'))
            return false;
        if (!code.sliced(_name.size()).trimmed().startsWith(u'('))
            return false;

        // The call ends where the parenthesis that opens it closes.
        int depth = 0;
        for (qsizetype i = 0; i < code.size(); ++i) {
            if (code.at(i) == u'(') {
                ++depth;
            } else if (code.at(i) == u')' && --depth == 0) {
                return i == code.size() - 1;
            }
        }
        return false;
    }

    QStringView _source;
    QString _name;
    RstLang::MarkdownOptions _options;
    RstLang::SectionAST *_examples = nullptr;
    QString _opening;
    QStringList _declared;
    QStringList _shown;
};

// What a definition list names: a term may spell out several arguments that
// mean the same, as in "``PERMISSIONS`` and ``FILE_PERMISSIONS``", and each
// of them is an argument the reader may write.  A term that spells out no
// argument of its own names one all the same.
QStringList termNames(const QString &term, const RstLang::MarkdownOptions &options)
{
    QStringList literals;
    for (qsizetype i = 0; i < term.size();) {
        const qsizetype open = term.indexOf("``"_L1, i);
        if (open < 0)
            break;
        const qsizetype close = term.indexOf("``"_L1, open + 2);
        if (close < 0)
            break;

        const QString literal = term.sliced(open + 2, close - open - 2).trimmed();
        if (!literal.isEmpty())
            literals.append(literal);
        i = close + 2;
    }

    if (!literals.isEmpty())
        return literals;

    QString name = RstLang::inlineToMarkdown(term, options);
    // What the argument is called reads as code, which is markup the
    // name itself does not carry.
    if (name.size() > 1 && name.startsWith(u'`') && name.endsWith(u'`'))
        name = name.mid(1, name.size() - 2).trimmed();
    return {name};
}

class Arguments: public RstLang::Visitor
{
public:
    Arguments(QStringView source, const QString &name,
              const RstLang::MarkdownOptions &options)
        : _source(source)
        , _name(name)
        , _options(options)
    {}

    QList<ArgumentDoc> result;

private:
    // A keyword that opens a signature of its own is an argument of the
    // command, and what the signature says is what it means.
    bool visit(RstLang::DirectiveAST *ast) override
    {
        if (!ast->isNamed("signature"))
            return true;

        QList<RstLang::BlockAST *> body;
        const QStringList calls = signatureCalls(_source, ast, _name, &body);
        const QString text = RstLang::toMarkdown(body, _options);

        for (const QString &written : calls) {
            const QString call = RstLang::inlineToPlainText(written, _options);
            const QString keyword = keywordOf(call, _name);
            if (keyword.isEmpty())
                continue;
            add(keyword, "```cmake\n"_L1 + call + "\n```"_L1
                             + (text.isEmpty() ? QString() : "\n\n"_L1 + text));
        }
        return true;
    }

    bool visit(RstLang::DefinitionItemAST *ast) override
    {
        const QString documentation = RstLang::toMarkdown(ast->blocks(), _options);
        for (const QString &name : termNames(ast->term(), _options))
            add(name, documentation);

        // An argument of an argument belongs to the argument, not to the
        // command.
        return false;
    }

    // A command may spell one of its arguments out several times, once per
    // signature it takes part in.
    void add(const QString &name, const QString &documentation)
    {
        if (name.isEmpty())
            return;

        const auto known = _index.constFind(name);
        if (known == _index.cend()) {
            _index.insert(name, result.size());
            result.append({name, documentation});
        } else if (!documentation.isEmpty()) {
            QString &existing = result[*known].documentation;
            if (existing.isEmpty())
                existing = documentation;
            else
                existing += "\n\n"_L1 + documentation;
        }
    }

    QStringView _source;
    QString _name;
    RstLang::MarkdownOptions _options;
    QHash<QString, int> _index;
};

// The blocks a ".rst:" comment is written as.
class Extractor
{
public:
    explicit Extractor(const QString &source)
        : _source(source)
    {}

    QList<DocComment> extract();

private:
    void addLineComments(const std::vector<Token> &tokens, size_t &index);
    void addBracketComment(const Token &token);

    QString _source;
    QList<DocComment> _comments;
};

// A line comment carries its text behind the "#", and a blank line as
// nothing at all.
QString lineCommentText(const Token &token)
{
    QStringView text = token.spelling;
    if (text.startsWith(u'#'))
        text = text.sliced(1);
    if (text.startsWith(u' '))
        text = text.sliced(1);
    return text.toString();
}

void Extractor::addLineComments(const std::vector<Token> &tokens, size_t &index)
{
    DocComment comment;
    // The marker stands on a line of its own; the documentation starts
    // behind it.
    comment.line = tokens[index].line + 1;

    QStringList lines;
    int line = tokens[index].line;
    size_t last = index;
    for (size_t i = index + 1; i < tokens.size(); ++i) {
        const Token &token = tokens[i];
        if (token.isNot(Parser::T_COMMENT) || token.line != line + 1)
            break;
        lines.append(lineCommentText(token));
        line = token.line;
        last = i;
    }

    comment.text = lines.join(u'\n');
    comment.end = tokens[last].end();
    index = last;
    if (!comment.text.trimmed().isEmpty())
        _comments.append(comment);
}

void Extractor::addBracketComment(const Token &token)
{
    QStringView text = token.value ? QStringView(*token.value) : token.spelling;
    text = text.sliced(markerString.size());

    // What stands behind the marker on the line it is on is not part of the
    // documentation.
    const qsizetype newline = text.indexOf(u'\n');
    text = newline < 0 ? QStringView() : text.sliced(newline + 1);

    // CMake writes the bracket that closes the comment behind a "#", and
    // that one stands inside the comment.
    if (text.endsWith(u'#') && (text.size() == 1 || text.at(text.size() - 2) == u'\n'))
        text.chop(1);

    DocComment comment;
    comment.line = token.line + 1;
    comment.end = token.end();
    comment.text = text.toString();
    if (!comment.text.trimmed().isEmpty())
        _comments.append(comment);
}

QList<DocComment> Extractor::extract()
{
    // Most CMake files carry no documentation at all, and the marker of one
    // that does is cheaper to look for than the comments are to read.
    if (!_source.contains(markerString))
        return {};

    Engine engine;
    Lexer lexer(&engine, engine.setSource(_source));
    lexer.setScanComments(true);

    std::vector<Token> tokens;
    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        if (token.is(Parser::T_COMMENT) || token.is(Parser::T_BRACKET_COMMENT))
            tokens.push_back(token);
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token &current = tokens[i];
        if (current.is(Parser::T_BRACKET_COMMENT)) {
            const QStringView text = current.value ? QStringView(*current.value)
                                                   : current.spelling;
            if (text.startsWith(markerString))
                addBracketComment(current);
        } else if (current.spelling.sliced(1) == markerString) {
            addLineComments(tokens, i);
        }
    }

    return _comments;
}

// The function or macro a documentation comment stands in front of.
CommandAST *definitionBehind(const DocumentPtr &document, int position)
{
    CommandAST *result = nullptr;
    for (CommandAST *command : document->commands()) {
        if (command->position < position)
            continue;
        if (!command->isNamed("function") && !command->isNamed("macro"))
            break;
        result = command;
        break;
    }
    return result;
}

QString definitionName(CommandAST *definition)
{
    ArgumentAST *argument = definition ? definition->arguments().first() : nullptr;
    return argument ? argument->value() : QString();
}

} // namespace

QList<DocComment> CMakeLang::documentationComments(const QString &source)
{
    return Extractor(source).extract();
}

// The line a comment declares a command on, such as
// ".. command:: find_package_handle_standard_args".  A module of CMake
// is shipped with the line endings of the platform it was unpacked on.
static QRegularExpression commandDeclaration()
{
    QStringList types;
    for (const DefinitionDirective &directive : definitionDirectives) {
        if (directive.kind == Documentation::Command)
            types.append(directive.type);
    }

    return QRegularExpression(
        QString(R"(^[ \t]*\.\.[ \t]+(?:%1)::[ \t]*(\S+)[ \t]*\r?$)").arg(types.join('|')),
        QRegularExpression::MultilineOption);
}

QStringList CMakeLang::documentedCommands(const QString &source)
{
    static const QRegularExpression declaration = commandDeclaration();

    QStringList result;
    for (const DocComment &comment : documentationComments(source)) {
        QRegularExpressionMatchIterator commands = declaration.globalMatch(comment.text);
        while (commands.hasNext())
            result.append(commands.next().captured(1));
    }
    return result;
}

QList<Documentation> CMakeLang::documentation(const RstLang::DocumentPtr &rst)
{
    QList<Documentation> result;
    if (!rst)
        return result;

    for (RstLang::DirectiveAST *directive : rst->directives()) {
        const Documentation::Kind kind = definitionKind(directive);
        if (kind == Documentation::Unknown || directive->argument().isEmpty())
            continue;

        Documentation documentation;
        documentation.name = directive->argument().toString();
        documentation.kind = kind;
        documentation.line = directive->line;
        documentation.rst = rst;
        documentation.node = directive;
        result.append(documentation);
    }
    return result;
}

Documentation CMakeLang::documentationFor(const RstLang::DocumentPtr &rst,
                                          const QString &name,
                                          Documentation::Kind kind)
{
    Documentation documentation;
    if (!rst)
        return documentation;

    // A file of the Help of CMake is named after what it documents, but one
    // that documents a module spells its commands out one by one.
    for (const Documentation &declared : CMakeLang::documentation(rst)) {
        if (declared.isNamed(name))
            return declared;
    }

    documentation.name = name;
    documentation.kind = kind;
    documentation.rst = rst;
    documentation.node = rst->ast();
    return documentation;
}

QList<Documentation> CMakeLang::documentation(const DocumentPtr &cmakeDocument,
                                              const RstLang::ParseOptions &options)
{
    QList<Documentation> result;
    if (!cmakeDocument)
        return result;

    const QString source = cmakeDocument->source().toString();
    for (const DocComment &comment : documentationComments(source)) {
        const RstLang::DocumentPtr rst = RstLang::Document::fromSource(comment.text, options);
        const QList<Documentation> declared = CMakeLang::documentation(rst);

        CommandAST *definition = definitionBehind(cmakeDocument, comment.end);
        const QString defined = definitionName(definition);

        // A comment that names nothing documents what it stands in front of.
        const QString title = rst->title().toString();
        if (declared.isEmpty() || (!title.isEmpty() && isSameCommand(title, defined))) {
            Documentation documentation;
            // The documentation spells the name the way it is meant to be
            // written, which is not always the way the definition does.
            documentation.name = title.isEmpty() ? defined : title;
            documentation.kind = isSameCommand(documentation.name, defined)
                                     ? Documentation::Command
                                     : Documentation::Module;
            documentation.line = comment.line;
            documentation.rst = rst;
            documentation.node = rst->ast();
            if (!documentation.name.isEmpty())
                result.append(documentation);
        } else if (!title.isEmpty()) {
            Documentation documentation;
            documentation.name = title;
            documentation.kind = Documentation::Module;
            documentation.line = comment.line;
            documentation.rst = rst;
            documentation.node = rst->ast();
            result.append(documentation);
        }

        for (Documentation declaration : declared) {
            declaration.line += comment.line - 1;
            result.append(declaration);
        }
    }

    return result;
}

QString CMakeLang::definitionSignature(CommandAST *definition)
{
    if (!definition || (!definition->isNamed("function") && !definition->isNamed("macro")))
        return {};

    QStringList parameters;
    for (ArgumentAST *argument : definition->arguments())
        parameters.append(argument->value());
    if (parameters.isEmpty())
        return {};

    const QString name = parameters.takeFirst();
    for (QString &parameter : parameters)
        parameter = u'<' + parameter + u'>';
    return name + u'(' + parameters.join(u' ') + u')';
}

bool CMakeLang::isSameCommand(QAnyStringView name, QAnyStringView other)
{
    return !name.isEmpty() && QAnyStringView::compare(name, other, Qt::CaseInsensitive) == 0;
}

bool Documentation::isNamed(QAnyStringView other) const
{
    return CMakeLang::isSameCommand(name, other);
}

const RstLang::MarkdownOptions &Documentation::markdownOptions() const
{
    if (!_markdownOptions) {
        RstLang::MarkdownOptions options;
        if (rst)
            options.substitutions = rst->substitutionTexts();
        options.defaultLanguage = QStringLiteral("cmake");
        _markdownOptions = options;
    }
    return *_markdownOptions;
}

QString Documentation::markdown() const
{
    if (!node)
        return {};

    const RstLang::MarkdownOptions &options = markdownOptions();
    if (RstLang::DirectiveAST *directive = node->asDirective())
        return RstLang::toMarkdown(directive->content(), options);
    if (RstLang::DocumentAST *document = node->asDocument())
        return RstLang::toMarkdown(document->blocks(), options);
    return {};
}

QString Documentation::brief() const
{
    return RstLang::briefMarkdown(node, markdownOptions());
}

QStringList Documentation::signatures() const
{
    if (!node)
        return {};

    Signatures signatures(rst->source(), name, markdownOptions());
    signatures.accept(node);
    return signatures.result();
}

QList<ArgumentDoc> Documentation::arguments() const
{
    if (!node)
        return {};

    Arguments arguments(rst->source(), name, markdownOptions());
    if (RstLang::DirectiveAST *directive = node->asDirective())
        RstLang::AST::accept(directive->blockList, &arguments);
    else
        arguments.accept(node);
    return arguments.result;
}
