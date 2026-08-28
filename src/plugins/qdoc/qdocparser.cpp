// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdocparser.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>

#include <optional>

namespace QDoc::Internal {

static QString inlineFormatStyle(const QString &command)
{
    static const QHash<QString, QString> formats = {
        {"b", "bold"},
        {"bold", "bold"},
        {"i", "italic"},
        {"e", "italic"},
        {"a", "parameter"},
        {"c", "teletype"},
        {"tt", "teletype"},
        {"sub", "subscript"},
        {"sup", "superscript"},
        {"underline", "underline"},
        {"uicontrol", "uicontrol"},
        {"tm", "trademark"},
        {"index", "index"},
    };
    return formats.value(command);
}

static QString admonitionKind(const QString &command)
{
    static const QHash<QString, QString> admonitions = {
        {"note", "note"},
        {"warning", "warning"},
        {"important", "important"},
        {"legalese", "legalese"},
        {"quotation", "quotation"},
        {"sidebar", "sidebar"},
        {"footnote", "footnote"},
    };
    return admonitions.value(command);
}

static bool isMarkupCommand(const QString &command)
{
    static const QSet<QString> commands = {
        "a", "annotatedlist", "b", "badcode", "bold", "br", "brief", "c", "caption",
        "code", "codeline", "compareswith", "details", "div", "dots", "e", "else",
        "endcode", "endcompareswith", "enddetails", "enddiv", "endfootnote", "endif",
        "endlegalese", "endlink", "endlist", "endmapref", "endomit", "endquotation",
        "endraw", "endsection1", "endsection2", "endsection3", "endsection4",
        "endsidebar", "endtable", "footnote", "generatelist", "header", "hr", "i",
        "if", "image", "important", "include", "inlineimage", "index", "input",
        "keyword", "l", "legalese", "li", "link", "list", "meta", "note",
        "notranslate", "o", "omit", "omitvalue", "overload", "printline", "printto",
        "printuntil", "quotation", "quotefile", "quotefromfile", "raw", "row", "sa",
        "section1", "section2", "section3", "section4", "sidebar", "sincelist",
        "skipline", "skipto", "skipuntil", "snippet", "span", "sub", "sup", "table",
        "tableofcontents", "target", "title", "tm", "toc", "tocentry", "endtoc",
        "tt", "uicontrol", "underline", "unicode", "value", "warning", "qml",
        "endqml", "cpp", "endcpp", "cpptext", "endcpptext",
    };
    return commands.contains(command);
}

static bool isTopicCommand(const QString &command)
{
    static const QSet<QString> commands = {
        "class", "concept", "dontdocument", "enum", "example", "externalpage", "fn",
        "group", "headerfile", "macro", "module", "namespace", "page", "property",
        "qmlattachedmethod", "qmlattachedproperty", "qmlattachedsignal", "qmlbasictype",
        "qmlenum", "qmlmethod", "qmlmodule", "qmlproperty", "qmlpropertygroup",
        "qmlsignal", "qmlsingletontype", "qmltype", "qmluncreatabletype", "qmlvaluetype",
        "struct", "typealias", "typedef", "union", "variable",
    };
    return commands.contains(command);
}

static bool isMetaCommand(const QString &command)
{
    static const QSet<QString> commands = {
        "abstract", "attribution", "cmakecomponent", "cmakepackage", "cmaketargetitem",
        "compares", "compareswith", "default", "deprecated", "inheaderfile", "inherits",
        "ingroup", "inmodule", "inpublicgroup", "inqmlmodule", "instantiates",
        "internal", "modulestate", "nativetype", "nextpage", "noautolist",
        "nonreentrant", "obsolete", "overload", "preliminary", "previouspage",
        "qmlabstract", "qmldefault", "qmlenumeratorsfrom", "qtcmakepackage",
        "qtcmaketargetitem", "qtvariable", "readonly", "reentrant", "reimp", "relates",
        "required", "since", "startpage", "subtitle", "threadsafe", "wrapper",
        "brief", "title", "meta",
    };
    return commands.contains(command);
}

static bool isUnsupportedCommand(const QString &command)
{
    static const QSet<QString> commands = {"generatelist", "annotatedlist", "sincelist"};
    return commands.contains(command);
}

static bool isComparisonCategory(const QString &category)
{
    static const QSet<QString> categories = {"strong", "weak", "partial", "equality"};
    return categories.contains(category);
}

static bool isAsciiAlnum(QChar c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isKnownCommand(const QString &command)
{
    return isMarkupCommand(command) || isTopicCommand(command) || isMetaCommand(command);
}

bool isEntityCommand(const QString &command)
{
    return isTopicCommand(command) || isMetaCommand(command);
}


// Blocks whose body is verbatim text. Only their terminator matters here, so the
// names are spelled out rather than taken from the dispatch table.
static const QStringList &verbatimBlocks()
{
    static const QStringList blocks = {"code", "badcode", "qml", "cpp", "cpptext", "raw", "omit"};
    return blocks;
}

static QString terminatorOf(const QString &block)
{
    return "\\end" + (block == "badcode" ? QString("code") : block);
}

QList<MarkupToken> scanMarkupLine(const QString &text, int *state)
{
    QList<MarkupToken> tokens;
    if (*state > 0) {
        const QString terminator = terminatorOf(verbatimBlocks().at(*state - 1));
        const int at = text.indexOf(terminator);
        if (at == -1) {
            tokens.append({0, int(text.size()), MarkupSpan::Code});
            return tokens;
        }
        if (at > 0)
            tokens.append({0, at, MarkupSpan::Code});
        tokens.append({at, int(terminator.size()), MarkupSpan::Command});
        *state = 0;
        return tokens;
    }

    if (text.trimmed().startsWith("//")) {
        tokens.append({0, int(text.size()), MarkupSpan::Comment});
        return tokens;
    }

    tokens.append({0, int(text.size()), MarkupSpan::Body});
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == '/' && text.mid(i, 3) == "/*!") {
            tokens.append({i, 3, MarkupSpan::Comment});
            i += 2;
            continue;
        }
        if (c == '*' && text.mid(i, 2) == "*/") {
            tokens.append({i, 2, MarkupSpan::Comment});
            ++i;
            continue;
        }
        if (c == '{') {
            const int end = text.indexOf('}', i);
            const int last = end == -1 ? text.size() : end + 1;
            tokens.append({i, int(last - i), MarkupSpan::Argument});
            i = last - 1;
            continue;
        }
        if (c != '\\')
            continue;

        int end = i + 1;
        while (end < text.size() && isAsciiAlnum(text.at(end)))
            ++end;
        const QString command = text.mid(i + 1, end - i - 1);
        // An escape such as \\ or \{, or a name only the linter has an opinion about.
        if (command.isEmpty() || !isKnownCommand(command)) {
            i = end;
            continue;
        }
        tokens.append({i, end - i, MarkupSpan::Command});
        i = end - 1;

        if (isEntityCommand(command)) {
            if (end < text.size())
                tokens.append({end, int(text.size() - end), MarkupSpan::Entity});
            return tokens;
        }
        if (verbatimBlocks().contains(command))
            *state = verbatimBlocks().indexOf(command) + 1;
    }
    return tokens;
}

static QString deprecationMessage(const QString &command)
{
    static const QHash<QString, QString> messages = {
        {"bold", "'\\bold' is deprecated. Use '\\b'"},
        {"i", "'\\i' is deprecated. Use '\\e' for italic or '\\li' for list item"},
        {"o", "'\\o' is deprecated. Use '\\li'"},
        {"instantiates",
         "\\instantiates is deprecated and will be removed in a future version. "
         "Use \\nativetype instead."},
        {"tableofcontents",
         "\\tableofcontents is deprecated and will be removed in a future version."},
    };
    return messages.value(command);
}

QString topicTitle(const QString &command)
{
    static const QHash<QString, QString> titles = {
        {"class", "Class"},
        {"struct", "Struct"},
        {"union", "Union"},
        {"namespace", "Namespace"},
        {"concept", "Concept"},
        {"headerfile", "Header File"},
        {"module", "Module"},
        {"group", "Group"},
        {"example", "Example"},
        {"qmltype", "QML Type"},
        {"qmlclass", "QML Type"},
        {"qmlsingletontype", "QML Type"},
        {"qmluncreatabletype", "QML Type"},
        {"qmlvaluetype", "QML Value Type"},
        {"qmlbasictype", "QML Value Type"},
        {"qmlmodule", "QML Module"},
        {"enum", "Enum"},
        {"typedef", "Typedef"},
        {"typealias", "Type Alias"},
        {"property", "Property"},
        {"variable", "Variable"},
        {"macro", "Macro"},
        {"qmlproperty", "QML Property"},
        {"qmlattachedproperty", "Attached Property"},
        {"qmlsignal", "QML Signal"},
        {"qmlattachedsignal", "Attached Signal"},
        {"qmlmethod", "QML Method"},
        {"qmlattachedmethod", "Attached Method"},
        {"qmlenum", "QML Enum"},
    };
    return titles.value(command);
}

QString ruleForMessage(const QString &message)
{
    static const QList<QPair<QRegularExpression, QString>> rules = {
        {QRegularExpression(R"(^Missing '[}\]]'$|^Missing '\\end)"), "unterminated-block"},
        {QRegularExpression("is deprecated"), "deprecated-command"},
        {QRegularExpression("^Unknown macro |^Unknown command "), "unknown-command"},
        {QRegularExpression("^Unrecognized list style"), "list-style"},
        {QRegularExpression("^Unrecognized markup language"), "code-language"},
        {QRegularExpression("^Missing comma in"), "sa-missing-comma"},
        {QRegularExpression(R"(is not needed in '\\sa')"), "sa-trailing-punctuation"},
        {QRegularExpression(R"(^'\\sa' has no link targets)"), "sa-empty"},
        {QRegularExpression(R"(^Duplicate \w+ name)"), "duplicate-target"},
        {QRegularExpression("^Macro .*(too few arguments|does not have a default definition)"),
         "macro-arguments"},
        {QRegularExpression("does not end with a full stop"), "brief-punctuation"},
        {QRegularExpression("^No such (template )?parameter"), "no-such-parameter"},
        {QRegularExpression("^Redundant link to self"), "redundant-sa-self"},
        {QRegularExpression("no topic command"), "missing-topic-command"},
        {QRegularExpression("without a textual description"), "missing-alt-text"},
        {QRegularExpression("^Missing image:"), "missing-image"},
        {QRegularExpression("snippet marker|in exampledirs|^Empty qdoc snippet"),
         "missing-snippet"},
        {QRegularExpression(R"(qdoc include file|^Cannot find '.*' in ')"), "missing-include"},
        {QRegularExpression(R"(didn't match here|^Missing pattern after|)"
                            R"(^Invalid regular expression|failed at end of file|)"
                            R"(without a preceding '\\quotefromfile')"),
         "quote-command"},
        {QRegularExpression("^Cannot find file or directory"), "config-missing-path"},
        {QRegularExpression("variable '.*' undefined"), "config-undefined-variable"},
        {QRegularExpression(R"(^Cannot open |^Expected '=' or|^Unexpected character|)"
                            R"(^Unterminated string|^Unexpected '='|^Bad include syntax|)"
                            R"(^Too many nested includes|^Cannot resolve \$)"),
         "config-syntax"},
        {QRegularExpression(R"(outside of '|within '|^Unexpected '\\|^Command '.*' outside|)"
                            R"(^Cannot nest|outside table item)"),
         "misplaced-command"},
        {QRegularExpression(R"(^Expected an argument|^Expected '\{'|^Invalid Unicode|)"
                            R"(^Invalid argument to|^Missing argument to|^Missing format name|)"
                            R"(^Unbalanced parentheses)"),
         "invalid-argument"},
    };

    for (const auto &[pattern, rule] : rules) {
        if (pattern.match(message).hasMatch())
            return rule;
    }
    return "invalid-argument";
}

static QString applyMatch(const QString &text, const QString &matchExpr)
{
    const QRegularExpression re(matchExpr);
    if (!re.isValid())
        return text;

    QString result;
    bool found = false;
    for (const QRegularExpressionMatch &m : re.globalMatch(text)) {
        found = true;
        if (m.lastCapturedIndex() > 0) {
            for (int c = 1; c <= m.lastCapturedIndex(); ++c)
                result += m.captured(c);
        } else {
            result += m.captured(0);
        }
    }
    return found ? result : QString();
}

static QString substitute(const QString &def, const QStringList &args)
{
    QString out;
    for (const QChar &c : def) {
        const char16_t code = c.unicode();
        if (code >= 1 && code <= 7)
            out += int(code) <= args.size() ? args.at(int(code) - 1) : QString();
        else
            out += c;
    }
    return out;
}

static bool isTrailingPunct(QChar c)
{
    return c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '?';
}

static bool isBuiltinCodeLanguage(const QString &lang)
{
    return lang == "c" || lang == "cpp" || lang == "qml" || lang == "text";
}

static const int MaxMacroExpansions = 1000;
static const int MaxIncludeSplices = 100;
static const int MaxDotsIndent = 64;

static const QChar EmDash(0x2014);

static QString simplifiedArgument(const QString &text)
{
    return text.trimmed().simplified();
}

static QString expandArgumentsInString(const QString &text, const QStringList &args)
{
    if (args.isEmpty())
        return text;
    QString out;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == '\\' && i + 1 < text.size()) {
            const QChar next = text.at(i + 1);
            if (next >= '1' && next <= '9') {
                const int digit = next.digitValue();
                if (digit <= args.size()) {
                    out += args.at(digit - 1);
                    ++i;
                    continue;
                }
            }
        }
        out += text.at(i);
    }
    return out;
}

static QString untabify(const QString &text, int tabSize)
{
    if (!text.contains('\t') || tabSize <= 0)
        return text;
    QStringList lines;
    for (const QString &line : text.split('\n')) {
        QString out;
        for (const QChar &c : line) {
            if (c == '\t')
                out += QString(tabSize - (out.size() % tabSize), ' ');
            else
                out += c;
        }
        lines.append(out);
    }
    return lines.join('\n');
}

static bool isIncludeMarker(const QString &line, const QString &identifier)
{
    const QString trimmed = line.trimmed();
    if (!trimmed.startsWith("//!"))
        return false;
    const int open = trimmed.indexOf('[');
    const int close = trimmed.indexOf(']');
    if (open < 0 || close <= open)
        return false;
    return trimmed.mid(open + 1, close - open - 1).trimmed() == identifier;
}

static std::optional<QString> extractIncludeSection(const QString &text,
                                                    const QString &identifier)
{
    const QStringList lines = text.split('\n');
    int i = 0;
    for (; i < lines.size(); ++i) {
        if (isIncludeMarker(lines.at(i), identifier))
            break;
    }
    if (i >= lines.size() - 1)
        return {};
    ++i;

    QStringList out;
    for (; i < lines.size(); ++i) {
        if (isIncludeMarker(lines.at(i), identifier))
            break;
        out.append(lines.at(i));
    }
    return out.isEmpty() ? std::optional<QString>() : out.join('\n');
}

static void parseSpan(const QString &text, int *colspan, int *rowspan)
{
    *colspan = 1;
    *rowspan = 1;
    static const QRegularExpression pattern(R"(^(\d+)\s*,\s*(\d+)$)");
    const QRegularExpressionMatch match = pattern.match(text.trimmed());
    if (!match.hasMatch())
        return;
    *colspan = qMax(1, match.captured(1).toInt());
    *rowspan = qMax(1, match.captured(2).toInt());
}

static Nodes trimInline(const Nodes &nodes)
{
    Nodes out = nodes;
    while (!out.isEmpty() && out.first()->kind == NodeKind::Text
           && out.first()->text.trimmed().isEmpty()) {
        out.removeFirst();
    }
    while (!out.isEmpty() && out.last()->kind == NodeKind::Text
           && out.last()->text.trimmed().isEmpty()) {
        out.removeLast();
    }
    if (!out.isEmpty()) {
        if (out.first()->kind == NodeKind::Text) {
            NodePtr node = makeText(out.first()->text);
            while (!node->text.isEmpty() && node->text.at(0).isSpace())
                node->text.remove(0, 1);
            out[0] = node;
        }
        if (out.last()->kind == NodeKind::Text) {
            NodePtr node = makeText(out.last()->text);
            while (!node->text.isEmpty() && node->text.at(node->text.size() - 1).isSpace())
                node->text.chop(1);
            out[out.size() - 1] = node;
        }
    }
    return out;
}

static QString defaultLanguage(const QString &command)
{
    if (command == "qml")
        return "qml";
    if (command == "cpp" || command == "cpptext")
        return "cpp";
    return {};
}

static bool isJoinableCode(const NodePtr &node)
{
    if (!node)
        return false;
    if (node->kind == NodeKind::Quote)
        return true;
    return node->kind == NodeKind::Code && !node->bad;
}

int fromAlpha(const QString &text)
{
    int n = 0;
    for (const QChar &c : text.toLower()) {
        const char16_t code = c.unicode();
        if (code < 'a' || code > 'z')
            return 0;
        n = n * 26 + (code - 'a' + 1);
    }
    return n;
}

static QString toRoman(int n)
{
    static const QList<QPair<int, QString>> table = {
        {1000, "m"}, {900, "cm"}, {500, "d"}, {400, "cd"}, {100, "c"}, {90, "xc"},
        {50, "l"},   {40, "xl"},  {10, "x"},  {9, "ix"},   {5, "v"},   {4, "iv"},
        {1, "i"},
    };
    QString out;
    int rest = n;
    for (const auto &[value, glyph] : table) {
        while (rest >= value) {
            out += glyph;
            rest -= value;
        }
    }
    return out;
}

int fromRoman(const QString &text)
{
    static const QHash<QChar, int> values = {
        {'i', 1}, {'v', 5}, {'x', 10}, {'l', 50}, {'c', 100}, {'d', 500}, {'m', 1000},
    };
    int n = 0;
    int previous = 0;
    const QString lower = text.toLower();
    for (const QChar &c : lower) {
        const int value = values.value(c, 0);
        if (!value)
            return 0;
        if (previous != 0 && value > previous)
            n -= 2 * previous;
        n += value;
        previous = value;
    }
    return toRoman(n) == lower ? n : 0;
}

ListHint parseListHint(const QString &hint)
{
    ListHint result;
    if (hint.isEmpty())
        return result;

    static const QRegularExpression pattern(R"(^(\W*)([0-9]+|[A-Z]+|[a-z]+)(\W*)$)");
    const QRegularExpressionMatch match = pattern.match(hint);
    if (!match.hasMatch()) {
        result.unrecognized = true;
        return result;
    }

    result.prefix = match.captured(1);
    result.suffix = match.captured(3);
    const QString token = match.captured(2);
    const bool lower = token == token.toLower();

    static const QRegularExpression numericPattern(R"(^\s*[0-9]+\s*$)");
    if (numericPattern.match(hint).hasMatch()) {
        result.style = ListStyle::Numeric;
        result.start = hint.trimmed().toInt();
        return result;
    }

    const int roman = fromRoman(token);
    if (roman > 0 && roman != 100 && roman != 500) {
        result.style = lower ? ListStyle::LowerRoman : ListStyle::UpperRoman;
        result.start = roman;
        return result;
    }

    const int alpha = fromAlpha(token);
    result.style = lower ? ListStyle::LowerAlpha : ListStyle::UpperAlpha;
    result.start = alpha ? alpha : 1;
    return result;
}

QString slugify(const QString &text)
{
    static const QRegularExpression command(R"(\\[a-z]+\s*)");
    static const QRegularExpression braces("[{}]");
    static const QRegularExpression nonWord("[^a-z0-9]+");
    static const QRegularExpression edges("^-+|-+$");
    QString out = text.toLower();
    out.replace(command, QString());
    out.replace(braces, QString());
    out.replace(nonWord, "-");
    out.replace(edges, QString());
    return out;
}

QString cleanLink(const QString &link)
{
    QString cleaned = link;
    cleaned.replace("\\#", "#");
    const int colon = cleaned.indexOf(':');
    if (colon == -1 || (!cleaned.startsWith("file:") && !cleaned.startsWith("mailto:")))
        return cleaned;
    return cleaned.mid(colon + 1).trimmed();
}

QString dedent(const QString &text)
{
    QString normalized = text;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');
    if (normalized.startsWith('\n'))
        normalized.remove(0, 1);
    while (!normalized.isEmpty() && normalized.at(normalized.size() - 1).isSpace())
        normalized.chop(1);

    QStringList lines = normalized.split('\n');
    int min = -1;
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty())
            continue;
        int indent = 0;
        while (indent < line.size() && line.at(indent).isSpace())
            ++indent;
        if (min == -1 || indent < min)
            min = indent;
    }
    if (min <= 0)
        return lines.join('\n');
    for (QString &line : lines)
        line = line.mid(min);
    return lines.join('\n');
}

QString flattenText(const Nodes &nodes)
{
    QString out;
    for (const NodePtr &node : nodes) {
        if (node->kind == NodeKind::Text || node->kind == NodeKind::InlineCode)
            out += node->text;
        else if (!node->children.isEmpty())
            out += flattenText(node->children);
    }
    return out.simplified();
}

namespace {
class Edit
{
public:
    int at = 0;
    int removed = 0;
    int inserted = 0;
};

class OpenList
{
public:
    NodePtr node;
    bool itemOpen = false;
};

class OpenTable
{
public:
    NodePtr node;
    bool cellOpen = false;
};

class QuoteSession
{
public:
    int id = 0;
    QString file;
    QString lang;
    int groups = 0;
};

class ConditionEvaluator
{
public:
    ConditionEvaluator(const QString &text, const QSet<QString> &defines)
        : m_text(text)
        , m_defines(defines)
    {}

    bool evaluate()
    {
        const bool value = readOr();
        skipSpaces();
        if (m_failed || m_pos != m_text.size())
            return true;
        return value;
    }

private:
    void skipSpaces()
    {
        while (m_pos < m_text.size() && m_text.at(m_pos).isSpace())
            ++m_pos;
    }

    bool readOr()
    {
        bool value = readAnd();
        for (;;) {
            skipSpaces();
            if (!m_text.mid(m_pos).startsWith("||"))
                return value;
            m_pos += 2;
            value = readAnd() || value;
        }
    }

    bool readAnd()
    {
        bool value = readUnary();
        for (;;) {
            skipSpaces();
            if (!m_text.mid(m_pos).startsWith("&&"))
                return value;
            m_pos += 2;
            value = readUnary() && value;
        }
    }

    bool readUnary()
    {
        skipSpaces();
        if (m_pos < m_text.size() && m_text.at(m_pos) == '!') {
            ++m_pos;
            return !readUnary();
        }
        return readPrimary();
    }

    bool readPrimary()
    {
        skipSpaces();
        if (m_pos < m_text.size() && m_text.at(m_pos) == '(') {
            ++m_pos;
            const bool value = readOr();
            skipSpaces();
            if (m_pos < m_text.size() && m_text.at(m_pos) == ')')
                ++m_pos;
            else
                m_failed = true;
            return value;
        }
        const QString name = readName();
        if (name.isEmpty()) {
            m_failed = true;
            return true;
        }
        if (name == "defined") {
            skipSpaces();
            if (m_pos < m_text.size() && m_text.at(m_pos) == '(') {
                ++m_pos;
                const QString inner = readName();
                skipSpaces();
                if (m_pos < m_text.size() && m_text.at(m_pos) == ')')
                    ++m_pos;
                else
                    m_failed = true;
                return m_defines.contains(inner);
            }
        }
        if (name == "true")
            return true;
        if (name == "false")
            return false;
        return m_defines.contains(name);
    }

    QString readName()
    {
        skipSpaces();
        QString name;
        while (m_pos < m_text.size()
               && (isAsciiAlnum(m_text.at(m_pos)) || m_text.at(m_pos) == '_')) {
            name += m_text.at(m_pos++);
        }
        return name;
    }

    QString m_text;
    QSet<QString> m_defines;
    int m_pos = 0;
    bool m_failed = false;
};

class Parser
{
public:
    explicit Parser(const ParserContext &context)
        : m_context(context)
        , m_singleLineParagraph(context.singleLineParagraph)
    {}

    ParsedDoc parse(const QString &source)
    {
        m_input = source;
        m_originalLength = source.size();
        m_pos = 0;
        m_containers = {&m_blocks};

        run();
        closePara();
        // QDoc reports the innermost construct that was never closed. An unclosed
        // \legalese is exempt, which DocParser::parse() notes is for compatibility.
        if (!m_openStack.isEmpty() && m_openStack.last() == "legalese")
            m_openStack.removeLast();
        // A fragment may leave a construct open for the file that includes it to
        // close: qtquick's layout.qdocinc is nothing but \li lines.
        if (m_context.fragment)
            m_openStack.clear();
        if (!m_openStack.isEmpty())
            warn(QString("Missing '\\end%1'").arg(m_openStack.last()), "error", m_input.size());
        else if (!m_conditionStack.isEmpty())
            warn("Missing '\\endif'", "error", m_input.size());
        while (m_containers.size() > 1)
            m_containers.removeLast();

        ParsedDoc doc;
        doc.topics = m_topics;
        doc.metas = m_metas;
        doc.brief = m_brief;
        doc.briefOffset = m_briefOffset;
        doc.title = m_title;
        doc.subtitle = m_subtitle;
        doc.body = m_blocks;
        doc.problems = m_problems;
        for (auto it = m_sessionNodes.cbegin(); it != m_sessionNodes.cend(); ++it) {
            QList<QList<QuoteStep>> groups;
            for (const NodePtr &node : it.value())
                groups.append(node->steps);
            doc.quoteSessions.insert(it.key(), groups);
        }
        doc.isInternal = m_metas.contains("internal");
        return doc;
    }

    bool atEnd() const { return m_pos >= m_input.size(); }

    QChar peek(int offset = 0) const
    {
        const int at = m_pos + offset;
        return at >= 0 && at < m_input.size() ? m_input.at(at) : QChar();
    }

    void warn(const QString &message,
              const QString &severity = QString("warning"),
              int at = -1,
              const QString &rule = {})
    {
        Problem problem;
        problem.message = message;
        problem.severity = severity;
        problem.rule = rule.isEmpty() ? ruleForMessage(message) : rule;
        problem.offset = toOriginalOffset(at < 0 ? m_pos : at);
        problem.synthetic = isSynthetic(at < 0 ? m_pos : at);
        m_problems.append(problem);
    }

    void flag(const QString &message,
              const QString &severity = QString("warning"),
              int at = -1,
              const QString &text = {},
              const QString &rule = {})
    {
        const QString id = rule.isEmpty() ? ruleForMessage(message) : rule;
        warn(message, severity, at, id);
        NodePtr node = makeNode(NodeKind::BadMarkup);
        node->message = message;
        node->severity = severity;
        node->rule = id;
        node->text = text;
        if (m_hasPara)
            appendInline(node);
        else
            currentContainer()->append(node);
    }

    void recordEdit(int at, int removed, int inserted)
    {
        m_edits.append({at, removed, inserted});
    }

    int toOriginalOffset(int offset) const
    {
        int result = offset;
        for (int i = m_edits.size() - 1; i >= 0; --i) {
            const Edit &edit = m_edits.at(i);
            if (result >= edit.at + edit.inserted)
                result += edit.removed - edit.inserted;
            else if (result > edit.at)
                result = edit.at;
        }
        return qBound(0, result, m_originalLength);
    }

    bool isSynthetic(int offset) const
    {
        int result = offset;
        for (int i = m_edits.size() - 1; i >= 0; --i) {
            const Edit &edit = m_edits.at(i);
            if (result >= edit.at + edit.inserted)
                result += edit.removed - edit.inserted;
            else if (result > edit.at)
                return true;
        }
        return false;
    }

    void skipSpacesOnLine()
    {
        while (!atEnd() && (peek() == ' ' || peek() == '\t'))
            ++m_pos;
    }

    void skipSpacesOrOneEndl()
    {
        skipSpacesOnLine();
        if (peek() == '\r')
            ++m_pos;
        if (peek() == '\n') {
            ++m_pos;
            skipSpacesOnLine();
        }
    }

    void skipAllSpaces()
    {
        while (!atEnd() && peek().isSpace())
            ++m_pos;
    }

    bool isBlankLine() const
    {
        int i = m_pos;
        while (i < m_input.size() && (m_input.at(i) == ' ' || m_input.at(i) == '\t'))
            ++i;
        return i >= m_input.size() || m_input.at(i) == '\n' || m_input.at(i) == '\r';
    }

    bool isLeftBraceAhead() const
    {
        int i = m_pos;
        int newlines = 0;
        while (i < m_input.size() && m_input.at(i).isSpace()) {
            if (m_input.at(i) == '\n' && ++newlines > 1)
                return false;
            ++i;
        }
        return i < m_input.size() && m_input.at(i) == '{';
    }

    bool isLeftBracketAhead() const
    {
        int i = m_pos;
        while (i < m_input.size() && (m_input.at(i) == ' ' || m_input.at(i) == '\t'))
            ++i;
        return i < m_input.size() && m_input.at(i) == '[';
    }

    bool getBracedArgument(QString *result, bool verbatim = false)
    {
        if (peek() != '{')
            return false;
        ++m_pos;
        int depth = 0;
        QString out;
        while (!atEnd()) {
            const QChar c = peek();
            if (c == '{') {
                ++depth;
                out += c;
                ++m_pos;
            } else if (c == '}') {
                --depth;
                if (depth < 0) {
                    ++m_pos;
                    *result = out;
                    return true;
                }
                out += c;
                ++m_pos;
            } else if (c == '\\' && !verbatim && expandMacroInPlace()) {
                // Rewritten in place; continue from the same position.
            } else {
                out += !verbatim && c.isSpace() ? QChar(' ') : c;
                ++m_pos;
            }
        }
        flag("Missing '}'", "error");
        *result = out;
        return true;
    }

    QString getArgument(bool verbatim = false)
    {
        skipSpacesOrOneEndl();
        QString braced;
        if (getBracedArgument(&braced, verbatim))
            return simplifiedArgument(braced);

        const int start = m_pos;
        int depth = 0;
        QString out;
        while (!atEnd() && (depth > 0 || !peek().isSpace())) {
            const QChar c = peek();
            if (c == '(' || c == '[' || c == '{') {
                ++depth;
                out += c;
                ++m_pos;
            } else if (c == ')' || c == ']' || c == '}') {
                --depth;
                if (m_pos == start || depth >= 0) {
                    out += c;
                    ++m_pos;
                } else {
                    break;
                }
            } else if (c == '\\' && !verbatim && expandMacroInPlace()) {
                // Rewritten in place; continue from the same position.
            } else {
                out += c;
                ++m_pos;
            }
        }
        if (out.size() > 1 && m_pos > 0 && isTrailingPunct(m_input.at(m_pos - 1))
            && !out.endsWith("...")) {
            out.chop(1);
            --m_pos;
        }
        if (out.size() > 2 && m_input.mid(m_pos - 2, 2) == "'s") {
            out.chop(2);
            m_pos -= 2;
        }
        return simplifiedArgument(out);
    }

    bool getOptionalArgument(QString *result)
    {
        skipSpacesOnLine();
        if (atEnd())
            return false;
        if (peek() == '\n' || peek() == '\r')
            return false;
        if (peek() == '\\' && isAsciiAlnum(peek(1)))
            return false;
        if (m_input.mid(m_pos, 3) == "//!")
            return false;
        *result = getArgument();
        return true;
    }

    bool getBracketedArgument(QString *result)
    {
        skipSpacesOnLine();
        if (peek() != '[')
            return false;
        ++m_pos;
        QString out;
        int depth = 0;
        while (!atEnd()) {
            const QChar c = peek();
            if (c == '[') {
                ++depth;
            } else if (c == ']') {
                if (depth == 0) {
                    ++m_pos;
                    *result = out;
                    return true;
                }
                --depth;
            }
            out += c;
            ++m_pos;
        }
        flag("Missing ']'", "error");
        *result = out;
        return true;
    }

    QList<SeeAlsoLink> parseAlso(int backslashAt)
    {
        QList<SeeAlsoLink> links;
        QSet<QString> seen;

        const auto atLineComment = [this] {
            skipSpacesOnLine();
            return m_input.mid(m_pos, 3) == "//!";
        };
        const auto skipToNewline = [this] {
            while (!atEnd() && peek() != '\n')
                ++m_pos;
        };

        skipSpacesOnLine();
        while (!atLineEnd()) {
            const int entryStart = m_pos;
            QString target;
            QString text;
            bool skipMe = false;

            if (peek() == '{') {
                target = getArgument();
                if (isLeftBraceAhead()) {
                    text = getArgument();
                    if (target.endsWith("::"))
                        target += text;
                } else {
                    text = target;
                }
            } else {
                target = getArgument();
                text = cleanLink(target);
                if (target == "and" || target == ".")
                    skipMe = true;
            }

            if (!skipMe && !target.isEmpty() && !seen.contains(target)) {
                seen.insert(target);
                links.append({target, text});
            }

            skipSpacesOnLine();
            if (atLineComment())
                skipToNewline();

            if (peek() == ',') {
                ++m_pos;
                if (atLineComment())
                    skipToNewline();
                skipSpacesOrOneEndl();
            } else if (closesTheSentence()) {
                warn(QString("A trailing '%1' is not needed in '\\sa'").arg(peek()),
                     "hint",
                     backslashAt,
                     "sa-trailing-punctuation");
                skipToLineEnd();
            } else if (!atLineEnd()) {
                flag("Missing comma in '\\sa'", "warning", backslashAt, "\\sa");
            }

            if (m_pos == entryStart) {
                ++m_pos;
                break;
            }
        }
        return links;
    }

    QString readLanguageArgument(const QString &cmdName, int backslashAt)
    {
        if (!isLeftBracketAhead())
            return {};
        QString bracketed;
        getBracketedArgument(&bracketed);
        const QString lang = bracketed.toLower();
        if (!lang.isEmpty() && !isBuiltinCodeLanguage(lang) && !m_context.codeLanguages.isEmpty()
            && !m_context.codeLanguages.contains(lang)) {
            flag(QString("Unrecognized markup language '%1'").arg(lang),
                 "warning",
                 backslashAt,
                 QString("\\%1 [%2]").arg(cmdName, lang),
                 "code-language");
        }
        return lang;
    }

    bool atLineEnd() const { return atEnd() || peek() == '\n' || peek() == '\r'; }

    void skipToLineEnd()
    {
        while (!atLineEnd())
            ++m_pos;
    }

    bool closesTheSentence() const
    {
        if (!isTrailingPunct(peek()))
            return false;
        int i = m_pos + 1;
        while (i < m_input.size() && (m_input.at(i) == ' ' || m_input.at(i) == '\t'))
            ++i;
        return i >= m_input.size() || m_input.at(i) == '\n' || m_input.at(i) == '\r';
    }

    QString getRestOfLine()
    {
        skipSpacesOnLine();
        QString out;
        bool simplify = false;
        for (;;) {
            QString line;
            bool continued = false;
            while (!atLineEnd()) {
                if (peek() == '\\' && isLineContinuation()) {
                    ++m_pos;
                    skipSpacesOnLine();
                    continued = true;
                    break;
                }
                line += m_input.at(m_pos++);
            }
            out += (out.isEmpty() ? QString() : QString(" ")) + line;
            if (peek() == '\r')
                ++m_pos;
            if (peek() == '\n')
                ++m_pos;
            if (!continued)
                break;
            simplify = true;
            while (peek() == '\r' || peek() == '\n')
                ++m_pos;
            skipSpacesOnLine();
        }
        return simplify ? simplifiedArgument(out) : out.trimmed();
    }

    bool isLineContinuation() const
    {
        int i = m_pos + 1;
        while (i < m_input.size() && (m_input.at(i) == ' ' || m_input.at(i) == '\t'))
            ++i;
        return i >= m_input.size() || m_input.at(i) == '\n' || m_input.at(i) == '\r';
    }

    QString getMetaCommandArgument(const QString &cmdStr)
    {
        skipSpacesOnLine();
        const int begin = m_pos;
        int parenDepth = 0;

        while (m_pos < m_input.size() && (peek() != '\n' || parenDepth > 0)) {
            const QChar c = peek();
            if (c == '(') {
                ++parenDepth;
            } else if (c == ')') {
                --parenDepth;
            } else if (c == '\\' && expandMacroInPlace()) {
                continue;
            }
            ++m_pos;
        }
        if (m_pos == m_input.size() && parenDepth > 0) {
            m_pos = begin;
            warn(QString("Unbalanced parentheses in '%1'").arg(cmdStr));
        }
        const QString text = simplifiedArgument(m_input.mid(begin, m_pos - begin));
        skipSpacesOnLine();
        return text;
    }

    QString getUntilEnd(const QString &name)
    {
        const QString terminator = "\\end" + name;
        const int at = m_input.indexOf(terminator, m_pos);
        QString text;
        if (at == -1) {
            text = m_input.mid(m_pos);
            m_pos = m_input.size();
            flag(QString("Missing '%1'").arg(terminator),
                 "error",
                 m_pos,
                 terminator,
                 "unterminated-block");
        } else {
            text = m_input.mid(m_pos, at - m_pos);
            m_pos = at + terminator.size();
        }
        return text;
    }

    bool expandMacroInPlace()
    {
        const int start = m_pos;
        int i = m_pos + 1;
        QString name;
        while (i < m_input.size() && isAsciiAlnum(m_input.at(i)))
            name += m_input.at(i++);
        if (name.isEmpty()) {
            if (peek(1) == '\\') {
                recordEdit(m_pos + 1, 1, 0);
                m_input.remove(m_pos + 1, 1);
            }
            return false;
        }
        if (!m_context.macros.contains(name))
            return false;
        const Macro macro = m_context.macros.value(name);
        if (macro.def.isEmpty())
            return false;

        if (m_macroExpansions >= MaxMacroExpansions) {
            if (m_macroExpansions == MaxMacroExpansions) {
                ++m_macroExpansions;
                flag(QString("Macro '\\%1' expanded %2 times %3 recursive definition? "
                             "Expansion stopped.")
                         .arg(name)
                         .arg(MaxMacroExpansions)
                         .arg(EmDash),
                     "error",
                     start,
                     "\\" + name);
            }
            return false;
        }
        ++m_macroExpansions;

        m_pos = i;
        const QStringList args = readMacroArguments(macro);
        QString expanded = substitute(macro.def, args);
        if (!macro.match.isEmpty())
            expanded = applyMatch(expanded, macro.match);
        recordEdit(start, m_pos - start, expanded.size());
        m_input.replace(start, m_pos - start, expanded);
        m_pos = start;
        return true;
    }

    QStringList readMacroArguments(const Macro &macro)
    {
        QStringList args;
        const int start = m_pos;
        for (int n = 0; n < macro.params; ++n) {
            const int before = m_pos;
            const QString arg = getArgument();
            if (arg.isEmpty() && m_pos == before)
                break;
            args.append(arg);
        }
        if (args.size() < macro.params) {
            flag(QString("Macro '\\%1' invoked with too few arguments (expected %2, got %3)")
                     .arg(macro.name)
                     .arg(macro.params)
                     .arg(args.size()),
                 "warning",
                 start,
                 "\\" + macro.name);
        }
        return args;
    }

    Nodes *currentContainer()
    {
        if (m_containers.isEmpty())
            m_containers.append(&m_blocks);
        return m_containers.last();
    }

    void popContainer()
    {
        if (m_containers.size() > 1)
            m_containers.removeLast();
    }

    void pushBlock(const NodePtr &node)
    {
        closePara();
        currentContainer()->append(node);
    }

    void appendCodePart(const NodePtr &part)
    {
        Nodes *container = currentContainer();
        const NodePtr last = container->isEmpty() ? NodePtr() : container->last();

        if (last && last->kind == NodeKind::CodeGroup) {
            last->children.append(part);
            return;
        }
        if (isJoinableCode(last)) {
            NodePtr group = makeNode(NodeKind::CodeGroup);
            group->children = {last, part};
            (*container)[container->size() - 1] = group;
            return;
        }
        if (part->kind == NodeKind::Dots || part->kind == NodeKind::CodeLine) {
            NodePtr group = makeNode(NodeKind::CodeGroup);
            group->children = {part};
            container->append(group);
            return;
        }
        container->append(part);
    }

    void appendGapToCode(const QuoteStep &step, const NodePtr &part)
    {
        if (m_quoteGroupNode && quoteGroupIsCurrent()) {
            m_quoteGroupNode->steps.append(step);
            return;
        }
        appendCodePart(part);
    }

    void startQuoteSession(const NodePtr &node)
    {
        QuoteSession session;
        session.id = ++m_lastSessionId;
        session.file = node->name;
        session.lang = node->lang;
        session.groups = 1;
        m_quoteSession = session;
        node->sessionId = session.id;
        node->groupIndex = 0;
        m_quoteGroupNode = node;
        m_sessionNodes[session.id].append(node);
    }

    bool quoteGroupIsCurrent()
    {
        Nodes *container = currentContainer();
        if (container->isEmpty() || !m_quoteGroupNode)
            return false;
        const NodePtr last = container->last();
        if (last == m_quoteGroupNode)
            return true;
        if (last->kind == NodeKind::CodeGroup && !last->children.isEmpty())
            return last->children.last() == m_quoteGroupNode;
        return false;
    }

    NodePtr quoteGroupForStep(int backslashAt)
    {
        if (quoteGroupIsCurrent())
            return m_quoteGroupNode;

        NodePtr node = makeNode(NodeKind::Quote);
        node->mode = "open";
        node->name = m_quoteSession->file;
        node->lang = m_quoteSession->lang;
        node->sessionId = m_quoteSession->id;
        node->groupIndex = m_quoteSession->groups++;
        node->offset = toOriginalOffset(backslashAt);
        appendCodePart(node);
        m_quoteGroupNode = node;
        m_sessionNodes[node->sessionId].append(node);
        return node;
    }

    Nodes *openPara()
    {
        if (!m_hasPara) {
            if (m_valueList && m_paraWrap.isEmpty() && !currentContainer()->isEmpty())
                closeValueList();
            m_hasPara = true;
            m_para.clear();
            m_formatStack.clear();
        }
        return &m_para;
    }

    Nodes *inlineTarget()
    {
        openPara();
        if (!m_formatStack.isEmpty())
            return &m_formatStack.last()->children;
        return &m_para;
    }

    void appendInline(const NodePtr &node) { inlineTarget()->append(node); }

    void appendText(const QString &text)
    {
        if (text.isEmpty())
            return;
        Nodes *target = inlineTarget();
        if (!target->isEmpty() && target->last()->kind == NodeKind::Text)
            target->last()->text += text;
        else
            target->append(makeText(text));
    }

    void appendSpace()
    {
        Nodes *target = inlineTarget();
        if (!target->isEmpty() && target->last()->kind == NodeKind::Text) {
            if (!target->last()->text.endsWith(' '))
                target->last()->text += ' ';
            return;
        }
        target->append(makeText(" "));
    }

    void closePara()
    {
        if (!m_hasPara)
            return;
        const Nodes children = trimInline(m_para);
        m_hasPara = false;
        m_para.clear();
        m_formatStack.clear();

        if (!m_captureInto.isEmpty()) {
            const QString slot = m_captureInto;
            m_captureInto.clear();
            m_paraWrap.clear();
            m_singleLineParagraph = m_context.singleLineParagraph;
            if (!children.isEmpty()) {
                if (slot == "brief") {
                    m_brief = children;
                } else {
                    m_title = flattenText(children);
                }
            }
            return;
        }

        if (children.isEmpty()) {
            m_paraWrap.clear();
            return;
        }
        NodePtr node = makeNode(NodeKind::Para);
        node->children = children;
        if (!m_paraWrap.isEmpty()) {
            NodePtr admonition = makeNode(NodeKind::Admonition);
            admonition->style = m_paraWrap;
            admonition->children = {node};
            currentContainer()->append(admonition);
            m_paraWrap.clear();
        } else {
            currentContainer()->append(node);
        }
    }

    void closeValueList()
    {
        if (m_valueList) {
            closePara();
            popContainer();
            m_valueList.reset();
        }
    }

    void run()
    {
        while (!atEnd()) {
            const QChar ch = peek();
            if (ch == '\\') {
                handleBackslash();
                continue;
            }
            if (ch == '\n') {
                ++m_pos;
                if (isBlankLine()) {
                    closePara();
                    while (!atEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\r'))
                        ++m_pos;
                } else if (m_hasPara) {
                    appendSpace();
                }
                continue;
            }
            if (ch == '/' && peek(1) == '/' && peek(2) == '!') {
                const int commentAt = m_pos;
                m_pos += 2;
                const QString rest = getRestOfLine();
                if (m_pos > 0 && m_input.at(m_pos - 1) == '\n')
                    --m_pos;
                static const QRegularExpression markerPattern(R"(^!\s*\[(.+)\]\s*$)");
                const QRegularExpressionMatch match = markerPattern.match(rest);
                if (m_context.fragment && match.hasMatch()) {
                    closePara();
                    NodePtr node = makeNode(NodeKind::SectionMarker);
                    node->name = match.captured(1).trimmed();
                    node->offset = toOriginalOffset(commentAt);
                    currentContainer()->append(node);
                }
                continue;
            }
            if (ch == '\r') {
                ++m_pos;
                continue;
            }
            if (ch == ' ' || ch == '\t') {
                ++m_pos;
                if (m_hasPara)
                    appendSpace();
                continue;
            }
            appendText(ch);
            ++m_pos;
        }
    }

    void handleBackslash()
    {
        const int backslashAt = m_pos;
        ++m_pos;
        QString name;
        while (!atEnd() && isAsciiAlnum(peek()))
            name += m_input.at(m_pos++);

        if (name.isEmpty()) {
            const QChar next = peek();
            if (next.isNull())
                return;
            if (next == '\\') {
                appendText("\\");
                ++m_pos;
            } else if (next.isSpace()) {
                skipAllSpaces();
                appendSpace();
            } else {
                appendText(next);
                ++m_pos;
            }
            return;
        }

        const QString deprecation = deprecationMessage(name);
        if (!deprecation.isEmpty()) {
            const bool announced = name == "instantiates" || name == "tableofcontents";
            warn(deprecation, announced ? QString("info") : QString("warning"), backslashAt);
        }

        if (dispatch(name, backslashAt))
            return;

        if (m_context.macros.contains(name)) {
            const Macro macro = m_context.macros.value(name);
            if (macro.unresolved) {
                m_pos = backslashAt + 1 + name.size();
                NodePtr node = makeNode(NodeKind::UnresolvedMacro);
                node->name = name;
                node->variables = macro.variables;
                appendInline(node);
                return;
            }
            if (macro.def.isEmpty() && !macro.hasRaw) {
                flag(QString("Macro '%1' does not have a default definition").arg(name),
                     "warning",
                     backslashAt,
                     "\\" + name);
            }
            m_pos = backslashAt;
            if (macro.hasRaw && macro.def.isEmpty()) {
                m_pos = backslashAt + 1 + name.size();
                const QStringList args = readMacroArguments(macro);
                NodePtr node = makeNode(NodeKind::Raw);
                node->text = substitute(macro.raw, args);
                node->isInline = true;
                appendInline(node);
                return;
            }
            if (expandMacroInPlace())
                return;
            m_pos = backslashAt + 1 + name.size();
        }

        if (isTopicCommand(name)) {
            readTopic(name);
            return;
        }
        if (isMetaCommand(name)) {
            readMeta(name);
            return;
        }
        if (m_context.fragment && name.size() == 1 && name.at(0) >= '1' && name.at(0) <= '9') {
            NodePtr node = makeNode(NodeKind::IncludeParameter);
            node->index = name.toInt();
            appendInline(node);
            return;
        }
        if (m_context.quoted) {
            appendText("\\" + name);
            return;
        }
        const bool looksLikeMacro = name != name.toLower() && !m_context.macros.isEmpty();
        flag(looksLikeMacro ? QString("Unknown macro '%1'").arg(name)
                            : QString("Unknown command '\\%1'").arg(name),
             "warning",
             backslashAt,
             "\\" + name);
    }

    bool dispatch(const QString &name, int backslashAt)
    {
        if (!isMarkupCommand(name)) {
            return false;
        }

        const QString format = inlineFormatStyle(name);
        if (!format.isEmpty()) {
            openPara();
            if (name == "c" || name == "tt") {
                NodePtr node = makeNode(NodeKind::InlineCode);
                node->text = getArgument(true);
                node->isInline = true;
                appendInline(node);
            } else if (name == "tm" && m_singleLineParagraph) {
                appendText(getArgument());
            } else {
                NodePtr node = makeNode(NodeKind::Format);
                node->style = format;
                node->children = {makeText(getArgument())};
                node->offset = toOriginalOffset(backslashAt);
                appendInline(node);
            }
            return true;
        }

        if (name.startsWith("section") && name.size() == 8) {
            closeValueList();
            NodePtr node = makeNode(NodeKind::Heading);
            node->level = name.right(1).toInt();
            node->children = parseInlineFragment(getRestOfLine(), true);
            node->text = flattenText(node->children);
            node->id = slugify(node->text);
            pushBlock(node);
            return true;
        }
        if (name.startsWith("endsection") && name.size() == 11) {
            closePara();
            return true;
        }

        if (name == "note" || name == "warning" || name == "important") {
            closePara();
            m_paraWrap = admonitionKind(name);
            skipSpacesOrOneEndl();
            return true;
        }

        if (name == "legalese" || name == "quotation" || name == "sidebar"
            || name == "footnote") {
            closePara();
            NodePtr node = makeNode(NodeKind::Admonition);
            node->style = admonitionKind(name);
            currentContainer()->append(node);
            m_containers.append(&node->children);
            m_openStack.append(name);
            return true;
        }
        if (name == "endlegalese" || name == "endquotation" || name == "endsidebar"
            || name == "endfootnote") {
            closeContainer(name.mid(3), backslashAt);
            return true;
        }

        if (name == "details") {
            closePara();
            NodePtr node = makeNode(NodeKind::Details);
            if (isLeftBraceAhead()) {
                node->summary = parseInlineFragment(getArgument(true));
            } else if (!isBlankLine()) {
                flag("Expected '{' when parsing \\details argument",
                     "warning",
                     backslashAt,
                     "\\details");
                getRestOfLine();
            }
            if (node->summary.isEmpty())
                node->summary = {makeText("...")};
            currentContainer()->append(node);
            m_containers.append(&node->children);
            m_openStack.append("details");
            return true;
        }
        if (name == "enddetails") {
            closeContainer("details", backslashAt);
            return true;
        }

        if (name == "div") {
            closePara();
            NodePtr node = makeNode(NodeKind::Div);
            if (isLeftBraceAhead())
                node->attrs = getArgument();
            currentContainer()->append(node);
            m_containers.append(&node->children);
            m_openStack.append("div");
            return true;
        }
        if (name == "enddiv") {
            closeContainer("div", backslashAt);
            return true;
        }

        if (name == "notranslate") {
            openPara();
            NodePtr node = makeNode(NodeKind::Format);
            node->style = "notranslate";
            node->children = parseInlineFragment(getArgument(true));
            appendInline(node);
            return true;
        }

        if (name == "code" || name == "badcode" || name == "qml" || name == "cpp"
            || name == "cpptext") {
            closePara();
            const QString lang = name == "code" ? readLanguageArgument(name, backslashAt)
                                                : QString();
            const QStringList parameters = getMetaCommandArgument("\\" + name)
                                               .split(' ', Qt::SkipEmptyParts);
            const QString endName = name == "badcode" ? QString("code") : name;
            const QString raw = expandArgumentsInString(
                untabify(getUntilEnd(endName), m_context.tabSize), parameters);
            NodePtr node = makeNode(NodeKind::Code);
            node->lang = lang.isEmpty() ? defaultLanguage(name) : lang;
            node->text = dedent(raw);
            node->bad = name == "badcode";
            currentContainer()->append(node);
            return true;
        }
        if (name == "endcode" || name == "endqml" || name == "endcpp" || name == "endcpptext")
            return true;

        if (name == "raw") {
            closePara();
            QString outputFormat;
            if (!getOptionalArgument(&outputFormat) || outputFormat.isEmpty()) {
                flag("Missing format name after '\\raw'", "warning", backslashAt, "\\raw");
            }
            const QString html = getUntilEnd("raw");
            if (outputFormat.isEmpty() || outputFormat.contains("html", Qt::CaseInsensitive)) {
                NodePtr node = makeNode(NodeKind::Raw);
                node->text = html;
                currentContainer()->append(node);
            }
            return true;
        }
        if (name == "endraw")
            return true;

        if (name == "omit") {
            closePara();
            getUntilEnd("omit");
            return true;
        }
        if (name == "endomit")
            return true;

        if (name == "list") {
            closePara();
            closeValueList();
            QString hint;
            getOptionalArgument(&hint);
            const ListHint parsed = parseListHint(hint);
            if (parsed.unrecognized) {
                flag(QString("Unrecognized list style '%1'").arg(hint),
                     "warning",
                     backslashAt,
                     "\\list " + hint,
                     "list-style");
            }
            NodePtr node = makeNode(NodeKind::List);
            node->listStyle = parsed.style;
            node->listStart = parsed.start;
            node->listPrefix = parsed.prefix;
            node->listSuffix = parsed.suffix;
            currentContainer()->append(node);
            m_openStack.append("list");
            m_listStack.append({node, false});
            return true;
        }
        if (name == "endlist") {
            closePara();
            if (!m_listStack.isEmpty()) {
                if (m_listStack.last().itemOpen)
                    popContainer();
                m_listStack.removeLast();
                popOpen("list");
            } else {
                flag("Unexpected '\\endlist'", "warning", backslashAt, "\\endlist");
            }
            return true;
        }

        if (name == "li" || name == "o") {
            handleListItem(backslashAt);
            return true;
        }

        if (name == "table") {
            closePara();
            closeValueList();
            NodePtr node = makeNode(NodeKind::Table);
            QString width;
            if (getOptionalArgument(&width))
                node->width = width;
            currentContainer()->append(node);
            m_openStack.append("table");
            m_tableStack.append({node, false});
            return true;
        }
        if (name == "endtable") {
            closePara();
            if (!m_tableStack.isEmpty()) {
                if (m_tableStack.last().cellOpen)
                    popContainer();
                m_tableStack.removeLast();
                popOpen("table");
            } else {
                flag("Unexpected '\\endtable'", "warning", backslashAt, "\\endtable");
            }
            return true;
        }
        if (name == "header" || name == "row") {
            handleTableRow(name == "header", backslashAt);
            return true;
        }

        if (name == "caption") {
            const QString text = getRestOfLine();
            if (!m_tableStack.isEmpty()) {
                m_tableStack.last().node->caption = text;
            } else {
                NodePtr node = makeNode(NodeKind::Caption);
                node->text = text;
                pushBlock(node);
            }
            return true;
        }

        if (name == "value") {
            handleValue();
            return true;
        }
        if (name == "omitvalue") {
            closePara();
            getArgument();
            return true;
        }

        if (name == "l") {
            openPara();
            NodePtr node = makeNode(NodeKind::Link);
            if (isLeftBracketAhead()) {
                QString params;
                getBracketedArgument(&params);
                node->linkParams = params;
                node->hasLinkParams = true;
            }
            node->target = getArgument();
            if (isLeftBraceAhead()) {
                skipSpacesOrOneEndl();
                QString text;
                getBracedArgument(&text, true);
                node->children = parseInlineFragment(text);
            } else {
                node->children = {makeText(cleanLink(node->target))};
            }
            appendInline(node);
            return true;
        }
        if (name == "link") {
            openPara();
            NodePtr node = makeNode(NodeKind::Link);
            node->target = getArgument();
            appendInline(node);
            m_formatStack.append(node);
            skipSpacesOrOneEndl();
            return true;
        }
        if (name == "endlink") {
            for (int i = m_formatStack.size() - 1; i >= 0; --i) {
                if (m_formatStack.at(i)->kind == NodeKind::Link) {
                    while (m_formatStack.size() > i)
                        m_formatStack.removeLast();
                    return true;
                }
            }
            return true;
        }
        if (name == "sa") {
            closePara();
            closeValueList();
            const QList<SeeAlsoLink> links = parseAlso(backslashAt);
            if (links.isEmpty()) {
                warn("'\\sa' has no link targets", "hint", backslashAt, "sa-empty");
                return true;
            }
            NodePtr node = makeNode(NodeKind::SeeAlso);
            node->links = links;
            node->offset = toOriginalOffset(backslashAt);
            currentContainer()->append(node);
            return true;
        }

        if (name == "target" || name == "keyword") {
            const QString target = getRestOfLine();
            if (target.isEmpty()) {
                flag(QString("Expected an argument for \\%1").arg(name),
                     "warning",
                     backslashAt,
                     "\\" + name);
                return true;
            }
            if (name == "target" && !m_tableStack.isEmpty() && !m_openStack.isEmpty()
                && m_openStack.last() == "table" && !m_tableStack.last().cellOpen) {
                flag("Found a \\target command outside table item in a table. Move the "
                     "\\target inside the \\li to resolve this warning.",
                     "warning",
                     backslashAt,
                     QString("\\%1 %2").arg(name, target));
            }
            NodePtr node = makeNode(NodeKind::Anchor);
            node->name = target;
            node->id = slugify(target);
            appendInline(node);
            return true;
        }

        if (name == "image" || name == "inlineimage") {
            const bool inlineImage = name == "inlineimage";
            if (inlineImage)
                openPara();
            else
                closePara();

            NodePtr node = makeNode(NodeKind::Image);
            node->name = getArgument();
            QString alt;
            bool braced = false;
            if (isLeftBraceAhead()) {
                braced = true;
                alt = getArgument();
            } else if (!inlineImage) {
                alt = getRestOfLine();
            }
            if (alt.size() > 1 && alt.startsWith('"') && alt.endsWith('"'))
                alt = alt.mid(1, alt.size() - 2);

            node->alt = alt;
            node->hasAlt = !alt.isEmpty();
            node->missingAltText = !braced && alt.isEmpty();
            node->isInline = inlineImage;
            node->offset = toOriginalOffset(backslashAt);
            if (inlineImage)
                appendInline(node);
            else
                currentContainer()->append(node);
            return true;
        }

        if (name == "snippet") {
            closePara();
            NodePtr node = makeNode(NodeKind::Quote);
            node->mode = "snippet";
            node->lang = readLanguageArgument(name, backslashAt);
            node->name = getArgument();
            node->identifier = getRestOfLine();
            node->offset = toOriginalOffset(backslashAt);
            appendCodePart(node);
            return true;
        }
        if (name == "quotefile") {
            closePara();
            NodePtr node = makeNode(NodeKind::Quote);
            node->mode = "quotefile";
            node->lang = readLanguageArgument(name, backslashAt);
            node->name = getRestOfLine();
            node->offset = toOriginalOffset(backslashAt);
            currentContainer()->append(node);
            return true;
        }
        if (name == "quotefromfile") {
            closePara();
            NodePtr node = makeNode(NodeKind::Quote);
            node->mode = "open";
            node->lang = readLanguageArgument(name, backslashAt);
            node->name = getRestOfLine();
            node->offset = toOriginalOffset(backslashAt);
            currentContainer()->append(node);
            startQuoteSession(node);
            return true;
        }
        if (name == "printline" || name == "printto" || name == "printuntil"
            || name == "skipline" || name == "skipto" || name == "skipuntil") {
            closePara();
            const QString pattern = getRestOfLine();
            if (m_quoteSession) {
                quoteGroupForStep(backslashAt)->steps.append({name, pattern});
            } else {
                flag(QString("'\\%1' without a preceding '\\quotefromfile'").arg(name),
                     "warning",
                     backslashAt,
                     "\\" + name);
            }
            return true;
        }
        if (name == "dots") {
            QString arg;
            const bool given = getOptionalArgument(&arg);
            bool ok = false;
            const int parsed = given ? arg.toInt(&ok) : 0;
            NodePtr node = makeNode(NodeKind::Dots);
            node->indent = ok ? qBound(0, parsed, MaxDotsIndent) : 4;
            if (ok && node->indent != parsed) {
                warn(QString("Invalid argument to '\\dots': the indent must be between 0 and %1")
                         .arg(MaxDotsIndent),
                     "warning",
                     backslashAt);
            }
            appendGapToCode({"dots", QString::number(node->indent)}, node);
            return true;
        }
        if (name == "codeline") {
            appendGapToCode({"codeline", {}}, makeNode(NodeKind::CodeLine));
            return true;
        }

        if (name == "include" || name == "input") {
            closePara();
            const QString target = getArgument();
            QString identifier;
            QStringList parameters;
            if (isLeftBraceAhead()) {
                identifier = getArgument();
                while (isLeftBraceAhead() && parameters.size() < 9)
                    parameters.append(getArgument());
            } else {
                identifier = getRestOfLine();
            }
            handleInclude(target, identifier, parameters, backslashAt);
            return true;
        }

        if (name == "if") {
            const QString condition = getRestOfLine();
            const bool active = ConditionEvaluator(condition, m_context.defines).evaluate();
            m_conditionStack.append(active);
            if (!active)
                skipToConditionBranch();
            return true;
        }
        if (name == "else") {
            if (!m_conditionStack.isEmpty())
                skipToEndif();
            return true;
        }
        if (name == "endif") {
            if (!m_conditionStack.isEmpty())
                m_conditionStack.removeLast();
            return true;
        }

        if (name == "br") {
            appendInline(makeNode(NodeKind::LineBreak));
            return true;
        }
        if (name == "hr") {
            pushBlock(makeNode(NodeKind::HorizontalRule));
            return true;
        }
        if (name == "unicode") {
            const QString arg = getArgument();
            bool ok = false;
            const int code = arg.startsWith("0x") ? arg.mid(2).toInt(&ok, 16) : arg.toInt(&ok);
            if (ok && code > 0 && code <= 0x10ffff) {
                const char32_t ucs4 = char32_t(code);
                appendText(QString::fromUcs4(&ucs4, 1));
            } else {
                flag(QString("Invalid Unicode character '%1' specified with '\\unicode'").arg(arg),
                     "warning",
                     backslashAt,
                     "\\unicode " + arg);
            }
            return true;
        }
        if (name == "span") {
            openPara();
            NodePtr node = makeNode(NodeKind::Format);
            node->style = "span";
            node->cssClass = getArgument(true);
            node->children = parseInlineFragment(getArgument(true));
            appendInline(node);
            return true;
        }
        if (name == "meta") {
            const QString key = getArgument();
            const QString value = getArgument();
            addMeta("meta", key + ' ' + value);
            return true;
        }
        if (name == "overload") {
            const QString arg = isBlankLine() ? getMetaCommandArgument("\\overload")
                                              : getRestOfLine();
            addMeta("overload", arg.trimmed());
            return true;
        }
        if (name == "brief" || name == "title") {
            closePara();
            m_captureInto = name;
            if (name == "title")
                m_singleLineParagraph = true;
            if (name == "brief")
                m_briefOffset = toOriginalOffset(backslashAt);
            return true;
        }

        if (name == "toc") {
            closePara();
            if (m_openStack.contains("toc"))
                flag("Cannot nest '\\toc' commands", "warning", backslashAt, "\\toc");
            NodePtr node = makeNode(NodeKind::Toc);
            currentContainer()->append(node);
            m_containers.append(&node->children);
            m_openStack.append("toc");
            return true;
        }
        if (name == "endtoc") {
            closeContainer("toc", backslashAt);
            return true;
        }
        if (name == "tocentry") {
            if (!m_openStack.contains("toc")) {
                flag("Command '\\tocentry' outside of '\\toc'",
                     "warning",
                     backslashAt,
                     "\\tocentry",
                     "misplaced-command");
            }
            return dispatch("l", backslashAt);
        }
        if (name == "tableofcontents") {
            closePara();
            if (isLeftBraceAhead())
                getArgument();
            return true;
        }
        if (name == "compareswith") {
            closePara();
            if (m_openStack.contains("compareswith")) {
                flag("Cannot nest '\\compareswith' commands",
                     "warning",
                     backslashAt,
                     "\\compareswith");
            }
            const QString kind = getRestOfLine();
            const QStringList parts = kind.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
            const QString category = parts.isEmpty() ? QString() : parts.first();
            if (category.isEmpty()) {
                flag("Missing argument to \\compareswith command. Provide at least one type "
                     "name, or a list of types separated by spaces.",
                     "warning",
                     backslashAt,
                     "\\compareswith");
            } else if (!isComparisonCategory(category.toLower())) {
                flag(QString("Invalid argument to \\compareswith command: `%1` Valid arguments "
                             "are `strong`, `weak`, `partial`, or `equality`.")
                         .arg(category),
                     "warning",
                     backslashAt,
                     category);
            } else if (parts.size() < 2) {
                flag("Missing argument to \\compareswith command. Provide at least one type "
                     "name, or a list of types separated by spaces.",
                     "warning",
                     backslashAt,
                     "\\compareswith " + category);
            }
            NodePtr node = makeNode(NodeKind::Compares);
            node->style = kind;
            node->text = getUntilEnd("compareswith").trimmed();
            currentContainer()->append(node);
            return true;
        }
        if (name == "endcompareswith" || name == "endmapref" || name == "index")
            return true;

        if (isUnsupportedCommand(name)) {
            closePara();
            NodePtr node = makeNode(NodeKind::Placeholder);
            node->name = name;
            node->args = getRestOfLine();
            node->offset = toOriginalOffset(backslashAt);
            currentContainer()->append(node);
            return true;
        }

        return false;
    }

    void closeContainer(const QString &kind, int at)
    {
        closePara();
        if (!m_openStack.isEmpty() && m_openStack.last() == kind) {
            m_openStack.removeLast();
            popContainer();
        } else {
            flag(QString("Unexpected '\\end%1'").arg(kind), "warning", at, "\\end" + kind);
        }
    }

    void popOpen(const QString &kind)
    {
        const int at = m_openStack.lastIndexOf(kind);
        if (at != -1)
            m_openStack.removeAt(at);
    }

    void openImplicitList()
    {
        NodePtr list = makeNode(NodeKind::List);
        list->implicit = true;
        currentContainer()->append(list);
        m_openStack.append("list");
        m_listStack.append({list, false});
    }

    void handleListItem(int at)
    {
        closePara();
        const bool inTable = !m_tableStack.isEmpty() && !m_openStack.isEmpty()
                             && m_openStack.last() == "table";

        if (inTable) {
            OpenTable &table = m_tableStack.last();
            QString spans = "1,1";
            if (isLeftBraceAhead()) {
                spans = getArgument();
                if (isLeftBraceAhead())
                    getArgument();
            }
            if (table.node->children.isEmpty())
                table.node->children.append(makeNode(NodeKind::TableRow));
            NodePtr row = table.node->children.last();
            if (table.cellOpen)
                popContainer();
            NodePtr cell = makeNode(NodeKind::TableCell);
            parseSpan(spans, &cell->colspan, &cell->rowspan);
            row->children.append(cell);
            m_containers.append(&cell->children);
            table.cellOpen = true;
            return;
        }

        if (!m_listStack.isEmpty()) {
            OpenList &list = m_listStack.last();
            if (list.itemOpen)
                popContainer();
            NodePtr item = makeNode(NodeKind::ListItem);
            list.node->children.append(item);
            m_containers.append(&item->children);
            list.itemOpen = true;
            skipSpacesOrOneEndl();
            return;
        }

        if (m_context.fragment) {
            openImplicitList();
            handleListItem(at);
            return;
        }
        flag("Command '\\li' outside of '\\list' and '\\table'", "warning", at, "\\li");
    }

    void handleTableRow(bool isHeader, int at)
    {
        closePara();
        if (m_tableStack.isEmpty()) {
            const QString command = isHeader ? QString("header") : QString("row");
            const QString outer = m_openStack.isEmpty() ? QString() : m_openStack.last();
            const QString message = outer.isEmpty()
                                        ? QString("Cannot use '\\%1' outside of '\\table'")
                                              .arg(command)
                                        : QString("Cannot use '\\%1' within '\\%2'")
                                              .arg(command, outer);
            flag(message, "warning", at, "\\" + command);
            return;
        }
        OpenTable &table = m_tableStack.last();
        if (table.cellOpen) {
            popContainer();
            table.cellOpen = false;
        }
        NodePtr row = makeNode(NodeKind::TableRow);
        row->header = isHeader;
        table.node->children.append(row);
    }

    void handleValue()
    {
        closePara();
        if (!m_valueList) {
            NodePtr node = makeNode(NodeKind::ValueList);
            currentContainer()->append(node);
            m_valueList = node;
            m_containers.append(&m_valueListPlaceholder);
        }
        QString name = getArgument();
        QString since;
        if (name.startsWith("[since ") && name.endsWith(']')) {
            since = name.mid(7, name.size() - 8);
            name = getArgument();
        }
        popContainer();
        NodePtr item = makeNode(NodeKind::ValueItem);
        item->name = name;
        item->since = since;
        m_valueList->children.append(item);
        m_containers.append(&item->children);
        skipSpacesOrOneEndl();
    }

    void handleInclude(const QString &target,
                       const QString &identifier,
                       const QStringList &parameters,
                       int backslashAt)
    {
        const auto pushPlaceholder = [this, &target, backslashAt](const QString &args,
                                                                  const QString &problem) {
            NodePtr node = makeNode(NodeKind::Placeholder);
            node->name = "include";
            node->args = args;
            node->offset = toOriginalOffset(backslashAt);
            node->problem = problem;
            currentContainer()->append(node);
            Q_UNUSED(target)
        };

        if (m_includeSplices >= MaxIncludeSplices) {
            pushPlaceholder(QString("%1 %2 too many nested includes").arg(target).arg(EmDash),
                            QString("Too many nested includes %1 does '%2' include itself?")
                                .arg(EmDash)
                                .arg(target));
            return;
        }
        const IncludeResult resolved = m_context.resolveInclude ? m_context.resolveInclude(target)
                                                                : IncludeResult();
        if (!resolved.found) {
            pushPlaceholder(target
                                + (identifier.isEmpty() ? QString()
                                                        : QString(" {%1}").arg(identifier))
                                + QString(" %1 file not found").arg(EmDash),
                            QString("Cannot find qdoc include file '%1'").arg(target));
            return;
        }
        QString text = resolved.text;
        if (!identifier.isEmpty()) {
            const std::optional<QString> extracted = extractIncludeSection(text, identifier);
            if (!extracted) {
                pushPlaceholder(QString("%1 {%2} %3 section not found")
                                    .arg(target, identifier)
                                    .arg(EmDash),
                                QString("Cannot find '%1' in '%2'").arg(identifier, target));
                return;
            }
            text = *extracted;
        }
        text = expandArgumentsInString(text, parameters);
        text.replace("\r\n", "\n");
        text.replace('\r', '\n');
        ++m_includeSplices;
        const QString spliced = '\n' + text + '\n';
        recordEdit(m_pos, 0, spliced.size());
        m_input.insert(m_pos, spliced);
    }

    void skipToConditionBranch() { skipCondition(true); }

    void skipToEndif() { skipCondition(false); }

    void skipCondition(bool stopAtElse)
    {
        int depth = 0;
        while (!atEnd()) {
            const int next = m_input.indexOf('\\', m_pos);
            if (next == -1) {
                m_pos = m_input.size();
                return;
            }
            m_pos = next + 1;
            QString name;
            while (m_pos < m_input.size() && isAsciiAlnum(peek()))
                name += m_input.at(m_pos++);
            if (name == "if") {
                ++depth;
            } else if (name == "endif") {
                if (depth == 0) {
                    if (!m_conditionStack.isEmpty())
                        m_conditionStack.removeLast();
                    return;
                }
                --depth;
            } else if (stopAtElse && name == "else" && depth == 0) {
                return;
            }
        }
    }

    void readTopic(const QString &name)
    {
        QString bracketed;
        if (isLeftBracketAhead())
            getBracketedArgument(&bracketed);
        const QString arg = getMetaCommandArgument(name);

        addMeta(name, arg.isEmpty() ? bracketed : arg);

        if (!name.endsWith("propertygroup"))
            m_topics.append({name, arg, bracketed});
    }

    void readMeta(const QString &name)
    {
        QString bracketed;
        if (isLeftBracketAhead())
            getBracketedArgument(&bracketed);

        if (name == "deprecated" || name == "obsolete") {
            addMeta("deprecated", bracketed);
            return;
        }

        const QString arg = getMetaCommandArgument(name);
        if (name == "subtitle") {
            m_subtitle = arg;
            return;
        }
        addMeta(name, arg.isEmpty() ? bracketed : arg);
    }

    void addMeta(const QString &name, const QString &value) { m_metas[name].append(value); }

    Nodes parseInlineFragment(const QString &text, bool singleLineParagraph = false)
    {
        if (text.isEmpty())
            return {};
        ParserContext context = m_context;
        context.singleLineParagraph = singleLineParagraph;
        Parser sub(context);
        const ParsedDoc result = sub.parse(text);
        m_problems.append(result.problems);
        Nodes inlineNodes;
        for (const NodePtr &block : result.body) {
            if (block->kind == NodeKind::Para)
                inlineNodes.append(block->children);
            else
                inlineNodes.append(block);
        }
        return inlineNodes;
    }

private:
    ParserContext m_context;
    QString m_input;
    int m_originalLength = 0;
    int m_pos = 0;
    QList<Edit> m_edits;
    int m_macroExpansions = 0;
    int m_includeSplices = 0;

    Nodes m_blocks;
    QList<Nodes *> m_containers;
    Nodes m_para;
    bool m_hasPara = false;
    QString m_paraWrap;
    Nodes m_formatStack;
    QStringList m_openStack;
    QList<OpenList> m_listStack;
    QList<OpenTable> m_tableStack;
    QList<bool> m_conditionStack;
    NodePtr m_valueList;
    Nodes m_valueListPlaceholder;
    QString m_captureInto;
    bool m_singleLineParagraph = false;

    std::optional<QuoteSession> m_quoteSession;
    NodePtr m_quoteGroupNode;
    QHash<int, Nodes> m_sessionNodes;
    int m_lastSessionId = 0;

    QList<Topic> m_topics;
    QHash<QString, QStringList> m_metas;
    Nodes m_brief;
    int m_briefOffset = 0;
    QString m_title;
    QString m_subtitle;
    QList<Problem> m_problems;
};

} // namespace

ParsedDoc parseDoc(const QString &source, const ParserContext &context)
{
    return Parser(context).parse(source);
}

static const QRegularExpression &topicLinePattern()
{
    static const QRegularExpression pattern(
        R"(^[ \t\r*]*\\(page|class|struct|union|namespace|fn|typedef|enum|property|variable|)"
        R"(macro|module|qmlmodule|group|example|externalpage|headerfile|qmltype|qmlvaluetype|)"
        R"(qmlbasictype|qmlsignal|qmlmethod|qmlattachedproperty|qmlproperty)\b)",
        QRegularExpression::MultilineOption);
    return pattern;
}

DocumentKind documentKind(const QString &fileName, const QString &text)
{
    const QString lower = fileName.toLower();
    if (lower.endsWith(".qdocinc"))
        return {true, true, false};
    if (lower.endsWith(".qdoc")) {
        static const QRegularExpression inSnippets(R"((^|[\\/])snippets[\\/])");
        if (inSnippets.match(fileName).hasMatch() && !topicLinePattern().match(text).hasMatch())
            return {true, true, true};
        return {true, false, false};
    }
    return {false, false, false};
}

static QString demargin(const QString &body, int startColumn)
{
    const int marginColumn = startColumn + 1;
    QStringList lines = body.split('\n');
    if (lines.size() < 2)
        return body;

    for (int i = 1; i < lines.size(); ++i) {
        const QString &line = lines.at(i);
        if (i == lines.size() - 1 && line.trimmed().isEmpty())
            continue;
        if (line.size() <= marginColumn)
            return body;
        if (line.at(marginColumn) != '*')
            return body;
        for (int c = 0; c < marginColumn; ++c) {
            if (line.at(c) != ' ' && line.at(c) != '\t')
                return body;
        }
    }

    for (int i = 1; i < lines.size(); ++i) {
        QString &line = lines[i];
        if (line.size() > marginColumn)
            line[marginColumn] = ' ';
    }
    return lines.join('\n');
}

static const int MaxDeclarationSkip = 6;

static QString findFollowingDeclaration(const QString &text, int from)
{
    if (from >= text.size())
        return {};
    const QStringList lines = text.mid(from).split('\n');
    QStringList collected;
    int skipped = 0;

    for (const QString &raw : lines) {
        const QString line = raw.trimmed();

        if (line.isEmpty()) {
            if (!collected.isEmpty())
                break;
            if (++skipped > MaxDeclarationSkip)
                break;
            continue;
        }

        if (line.startsWith('#') || line.startsWith("//")) {
            if (!collected.isEmpty())
                break;
            if (++skipped > MaxDeclarationSkip)
                break;
            continue;
        }
        if (line.startsWith("/*"))
            break;

        collected.append(line);
        const QString joined = collected.join(' ');
        static const QRegularExpression complete(R"([;{)]$|\bQ_[A-Z_]+\b)");
        if (joined.count('(') <= joined.count(')') && complete.match(joined).hasMatch())
            break;
        if (collected.size() >= 4)
            break;
    }

    if (collected.isEmpty())
        return {};
    QString out = collected.join(' ');
    static const QRegularExpression openBrace(R"(\s*\{\s*$)");
    out.remove(openBrace);
    if (out.endsWith(';'))
        out.chop(1);
    return out.simplified();
}

static void afterFileHeader(const QString &text, int *offset, int *line)
{
    *offset = 0;
    *line = 0;
    int at = 0;
    int lineNumber = 0;
    while (at < text.size()) {
        int eol = text.indexOf('\n', at);
        if (eol == -1)
            eol = text.size();
        const QString trimmed = text.mid(at, eol - at).trimmed();
        const bool isComment = trimmed.startsWith("//") && !trimmed.startsWith("//!");
        if (!trimmed.isEmpty() && !isComment)
            break;
        at = eol + 1;
        ++lineNumber;
    }
    if (at >= text.size())
        return;
    *offset = at;
    *line = lineNumber;
}

QList<DocBlock> extractDocBlocks(const QString &text, const DocumentKind &kind)
{
    QList<DocBlock> blocks;
    const int len = text.size();
    int i = 0;
    int line = 0;
    int lineStart = 0;

    const auto at = [&text, len](int index) {
        return index >= 0 && index < len ? text.at(index) : QChar();
    };
    const auto advanceTo = [&](int target) {
        while (i < target && i < len) {
            if (text.at(i) == '\n') {
                ++line;
                lineStart = i + 1;
            }
            ++i;
        }
    };

    while (i < len) {
        const QChar ch = text.at(i);

        if (ch == '"' || ch == '\'') {
            const QChar quote = ch;
            ++i;
            while (i < len) {
                if (text.at(i) == '\\') {
                    i += 2;
                    continue;
                }
                if (text.at(i) == quote) {
                    ++i;
                    break;
                }
                if (text.at(i) == '\n')
                    break;
                ++i;
            }
            continue;
        }

        if (ch == 'R' && at(i + 1) == '"') {
            const int open = text.indexOf('(', i + 2);
            if (open != -1 && open - (i + 2) < 17) {
                const QString closer = ')' + text.mid(i + 2, open - i - 2) + '"';
                const int end = text.indexOf(closer, open);
                if (end != -1) {
                    advanceTo(end + closer.size());
                    continue;
                }
            }
        }

        if (ch == '/' && at(i + 1) == '*') {
            const bool isDoc = at(i + 2) == '!';
            const int startLine = line;
            const int column = i - lineStart;
            const int bodyStart = i + 3;
            const int end = text.indexOf("*/", i + 2);
            const int stop = end == -1 ? len : end;
            if (isDoc) {
                const QString rawBody = text.mid(bodyStart, stop - bodyStart);
                advanceTo(stop);
                DocBlock block;
                block.text = demargin(rawBody, column);
                block.line = startLine;
                block.endLine = line;
                block.column = column;
                block.offset = bodyStart;
                block.followingDeclaration = findFollowingDeclaration(text, stop + 2);
                blocks.append(block);
            } else {
                advanceTo(stop);
            }
            advanceTo(stop + 2);
            continue;
        }

        if (ch == '/' && at(i + 1) == '/') {
            const bool isDoc = at(i + 2) == '!' && !kind.wholeFileIsDoc;
            const int startLine = line;
            const int column = i - lineStart;
            int eol = text.indexOf('\n', i);
            if (eol == -1)
                eol = len;
            if (isDoc) {
                QStringList parts = {text.mid(i + 3, eol - i - 3)};
                int cursor = eol;
                int lastEnd = eol;
                for (;;) {
                    const int nextLineStart = cursor + 1;
                    if (nextLineStart >= len)
                        break;
                    int probe = nextLineStart;
                    while (probe < len && (text.at(probe) == ' ' || text.at(probe) == '\t'))
                        ++probe;
                    if (text.mid(probe, 3) != "//!")
                        break;
                    int nextEol = text.indexOf('\n', probe);
                    if (nextEol == -1)
                        nextEol = len;
                    parts.append(text.mid(probe + 3, nextEol - probe - 3));
                    cursor = nextEol;
                    lastEnd = nextEol;
                }
                advanceTo(lastEnd);
                DocBlock block;
                block.text = parts.join('\n');
                block.line = startLine;
                block.endLine = line;
                block.column = column;
                block.offset = i + 3;
                block.fromLineComment = true;
                blocks.append(block);
                continue;
            }
            advanceTo(eol);
            continue;
        }

        advanceTo(i + 1);
    }

    if (blocks.isEmpty() && kind.wholeFileIsDoc) {
        int offset = 0;
        int startLine = 0;
        afterFileHeader(text, &offset, &startLine);
        DocBlock block;
        block.text = text.mid(offset);
        block.line = startLine;
        block.endLine = text.count('\n');
        block.offset = offset;
        blocks.append(block);
    }
    return blocks;
}

bool looksLikeQDoc(const QString &text)
{
    static const QRegularExpression pattern(
        R"(/\*!|^[^\S\n]*\\(class|page|fn|qmltype|module|group|property|enum|example|)"
        R"(headerfile|qmlproperty|qmlmodule|namespace|struct)\b)",
        QRegularExpression::MultilineOption);
    return pattern.match(text).hasMatch();
}

} // namespace QDoc::Internal
