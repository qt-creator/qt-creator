// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdocrenderer.h"

#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

using namespace Qt::Literals::StringLiterals;

namespace QDoc::Internal {

QString canonicalName(const QString &text)
{
    if (text.isEmpty())
        return {};
    QString name = text.simplified();

    const int paren = name.indexOf('(');
    if (paren != -1)
        name = name.left(paren).trimmed();

    name = name.split(' ').last();

    while (!name.isEmpty() && (name.at(0) == '*' || name.at(0) == '&'))
        name.remove(0, 1);
    return name.trimmed();
}

QStringList nameKeys(const QString &canonical)
{
    if (canonical.isEmpty())
        return {};
    QStringList keys = {canonical};

    QString rest = canonical;
    int cut = rest.indexOf("::");
    while (cut != -1) {
        rest = rest.mid(cut + 2);
        if (!rest.isEmpty())
            keys.append(rest);
        cut = rest.indexOf("::");
    }

    const QString last = keys.last();
    if (last.contains('.')) {
        const QString leaf = last.mid(last.lastIndexOf('.') + 1);
        if (!leaf.isEmpty())
            keys.append(leaf);
    }
    return keys;
}

QStringList keysForTopic(const QString &command, const QString &arg)
{
    if (arg.isEmpty())
        return {};

    QString text = arg;
    if (command == "qmlproperty" || command == "qmlattachedproperty") {
        static const QRegularExpression pattern(R"(^\S+\s+(.+)$)");
        const QRegularExpressionMatch match = pattern.match(arg.trimmed());
        if (match.hasMatch())
            text = match.captured(1);
    }
    return nameKeys(canonicalName(text));
}

void AnchorIndex::add(const QString &key, const QString &id)
{
    if (key.isEmpty() || id.isEmpty())
        return;
    if (!m_byKey.contains(key))
        m_byKey.insert(key, id);
}

void AnchorIndex::addAll(const QStringList &keys, const QString &id)
{
    for (const QString &key : keys)
        add(key, id);
}

QString AnchorIndex::lookup(const QString &target) const
{
    const QString raw = target.trimmed();
    if (raw.isEmpty())
        return {};
    static const QRegularExpression external("^(https?|ftp|mailto|file):",
                                             QRegularExpression::CaseInsensitiveOption);
    if (external.match(raw).hasMatch() || raw.startsWith('#'))
        return {};
    static const QRegularExpression page(R"(\.html?$)", QRegularExpression::CaseInsensitiveOption);
    if (raw.contains('/') || page.match(raw).hasMatch())
        return {};

    if (m_byKey.contains(raw))
        return m_byKey.value(raw);

    const QString canonical = canonicalName(raw);
    const bool qualified = canonical.contains("::");

    for (const QString &key : nameKeys(canonical)) {
        if (qualified && !key.contains("::"))
            break;
        if (m_byKey.contains(key))
            return m_byKey.value(key);
    }
    return {};
}

static QString unescapeAttr(const QString &text)
{
    QString out = text;
    out.replace("&#39;", "'");
    out.replace("&quot;", "\"");
    out.replace("&gt;", ">");
    out.replace("&lt;", "<");
    out.replace("&amp;", "&");
    return out;
}

QString resolveLinks(const QString &html, const AnchorIndex &anchors)
{
    if (anchors.isEmpty())
        return html;

    static const QRegularExpression link(
        R"rx(<a class="qdoc-link" title="([^"]*)" data-target="([^"]*)">)rx");
    QString out;
    int last = 0;
    for (const QRegularExpressionMatch &match : link.globalMatch(html)) {
        const QString id = anchors.lookup(unescapeAttr(match.captured(2)));
        if (id.isEmpty())
            continue;
        out += html.mid(last, match.capturedStart() - last);
        out += QString(R"(<a class="qdoc-link qdoc-link-resolved" href="#%1" title="%2" )"
                       R"(data-target="%3">)")
                   .arg(id, match.captured(1), match.captured(2));
        last = match.capturedEnd();
    }
    out += html.mid(last);
    return out;
}

// One pass, and none at all for text that needs no escaping, which most of a page
// is. Five QString::replace() calls over every text node and every code block was
// the largest single cost of rendering a big page.
static QString escape(const QString &text, bool forAttribute)
{
    const auto replacement = [forAttribute](QChar c) -> QLatin1StringView {
        switch (c.unicode()) {
        case '&':
            return "&amp;"_L1;
        case '<':
            return "&lt;"_L1;
        case '>':
            return "&gt;"_L1;
        case '"':
            return "&quot;"_L1;
        case '\'':
            return forAttribute ? "&#39;"_L1 : QLatin1StringView();
        default:
            return {};
        }
    };

    int plain = 0;
    while (plain < text.size() && replacement(text.at(plain)).isNull())
        ++plain;
    if (plain == text.size())
        return text;

    QString out = text.left(plain);
    out.reserve(text.size() + 32);
    for (int i = plain; i < text.size(); ++i) {
        const QLatin1StringView escaped = replacement(text.at(i));
        if (escaped.isNull())
            out += text.at(i);
        else
            out += escaped;
    }
    return out;
}

QString escapeHtml(const QString &text)
{
    return escape(text, false);
}

QString escapeAttr(const QString &text)
{
    return escape(text, true);
}

static QString plainText(const Nodes &nodes)
{
    QString out;
    for (const NodePtr &node : nodes) {
        if (node->kind == NodeKind::Text || node->kind == NodeKind::InlineCode)
            out += node->text;
        else if (!node->children.isEmpty())
            out += plainText(node->children);
    }
    return out.trimmed();
}

static QString signatureFor(const ParsedDoc &doc)
{
    for (const Topic &topic : doc.topics) {
        if (topic.command == "fn" || topic.command == "macro" || topic.command == "qmlmethod"
            || topic.command == "qmlattachedmethod" || topic.command == "qmlsignal"
            || topic.command == "qmlattachedsignal") {
            return topic.arg;
        }
    }
    if (doc.topics.isEmpty())
        return doc.followingDeclaration;
    return {};
}

static QSet<QString> parameterNames(const QString &signature)
{
    QSet<QString> names;
    const int open = signature.indexOf('(');
    if (open == -1)
        return names;

    int depth = 0;
    int end = -1;
    for (int i = open; i < signature.size(); ++i) {
        if (signature.at(i) == '(') {
            ++depth;
        } else if (signature.at(i) == ')') {
            --depth;
            if (depth == 0) {
                end = i;
                break;
            }
        }
    }
    if (end == -1)
        return names;

    QStringList parameters;
    QString current;
    int nesting = 0;
    for (const QChar &c : signature.mid(open + 1, end - open - 1)) {
        if (c == '<' || c == '(' || c == '[' || c == '{')
            ++nesting;
        else if (c == '>' || c == ')' || c == ']' || c == '}')
            --nesting;
        if (c == ',' && nesting == 0) {
            parameters.append(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parameters.append(current);

    static const QRegularExpression identifier("[A-Za-z_][A-Za-z0-9_]*");
    static const QRegularExpression builtinType(
        "^(int|bool|char|short|long|float|double|void|unsigned|signed|const|auto)$");
    for (const QString &parameter : parameters) {
        const QString withoutDefault = parameter.split('=').first();
        QStringList identifiers;
        for (const QRegularExpressionMatch &match : identifier.globalMatch(withoutDefault))
            identifiers.append(match.captured());
        if (identifiers.isEmpty())
            continue;
        const QString name = identifiers.last();
        if (builtinType.match(name).hasMatch())
            continue;
        if (withoutDefault.trimmed() == name && !name.isEmpty() && name.at(0).isUpper())
            continue;
        names.insert(name);
    }
    return names;
}

static QString collapseSingleParagraph(const QString &html)
{
    const QString trimmed = html.trimmed();
    static const QRegularExpression pattern(R"(^<p>([\s\S]*)</p>$)");
    const QRegularExpressionMatch match = pattern.match(trimmed);
    if (match.hasMatch() && !match.captured(1).contains("<p>"))
        return match.captured(1);
    return trimmed;
}

static QString listSeparator(int index, int count)
{
    if (index == count - 1)
        return ".";
    if (count == 2)
        return " and ";
    if (index == 0 || index < count - 2)
        return ", ";
    return ", and ";
}

static QString shortenPath(const Utils::FilePath &path)
{
    const QStringList parts = path.path().split('/');
    if (parts.size() <= 4)
        return path.path();
    return QString("%1/%2").arg(QChar(0x2026), parts.mid(parts.size() - 3).join('/'));
}

static QString sourceLabel(const Utils::FilePaths &paths)
{
    if (paths.isEmpty())
        return {};
    QStringList shortened;
    for (const Utils::FilePath &path : paths)
        shortened.append(shortenPath(path));
    return "<div class=\"qdoc-code-source\">" + escapeHtml(shortened.join(", ")) + "</div>";
}

static QString writtenQuote(const NodePtr &node)
{
    if (node->mode == "snippet")
        return QString("\\snippet %1 %2").arg(node->name, node->identifier);
    return QString("\\%1 %2").arg(node->mode == "open" ? QString("quotefromfile") : node->mode,
                                  node->name);
}

static const QSet<QString> &cppKeywords()
{
    static const QSet<QString> keywords = {
        "alignas", "alignof", "auto", "bool", "break", "case", "catch", "char", "class",
        "concept", "const", "consteval", "constexpr", "constinit", "const_cast",
        "continue", "co_await", "co_return", "co_yield", "decltype", "default",
        "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
        "export", "extern", "false", "float", "for", "friend", "goto", "if",
        "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "nullptr", "operator", "private", "protected", "public", "register",
        "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
        "static", "static_assert", "static_cast", "struct", "switch", "template",
        "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
        "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
        "while", "emit", "signals", "slots", "Q_OBJECT", "Q_PROPERTY", "override",
        "final",
    };
    return keywords;
}

static const QSet<QString> &qmlKeywords()
{
    static const QSet<QString> keywords = {
        "import", "as", "property", "signal", "function", "readonly", "default",
        "alias", "var", "let", "const", "int", "real", "string", "bool", "color",
        "true", "false", "null", "undefined", "if", "else", "for", "while",
        "return", "on", "component", "required", "pragma", "enum",
    };
    return keywords;
}

QString highlight(const QString &text, const QString &lang)
{
    const QString escaped = escapeHtml(text);
    if (lang.isEmpty())
        return escaped;

    const QSet<QString> *keywords = nullptr;
    if (lang == "cpp")
        keywords = &cppKeywords();
    else if (lang == "qml" || lang == "javascript")
        keywords = &qmlKeywords();

    static const QRegularExpression pattern(
        R"((&quot;(?:[^&\\]|\\.|&(?!quot;))*&quot;|&#39;(?:[^&\\]|\\.)*&#39;|)"
        R"(//[^\n]*|/\*[\s\S]*?\*/|#[^\n]*))");
    static const QRegularExpression word(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)");
    static const QRegularExpression number(R"(\b(0x[0-9a-fA-F]+|\d+(?:\.\d+)?)\b)");

    const auto highlightCode = [&](const QString &code) {
        QString out = code;
        if (keywords) {
            QString replaced;
            int last = 0;
            for (const QRegularExpressionMatch &match : word.globalMatch(out)) {
                if (!keywords->contains(match.captured()))
                    continue;
                replaced += out.mid(last, match.capturedStart() - last);
                replaced += "<span class=\"hl-keyword\">" + match.captured() + "</span>";
                last = match.capturedEnd();
            }
            replaced += out.mid(last);
            out = replaced;
        }
        out.replace(number, R"(<span class="hl-number">\1</span>)");
        return out;
    };

    QString result;
    int last = 0;
    for (const QRegularExpressionMatch &match : pattern.globalMatch(escaped)) {
        result += highlightCode(escaped.mid(last, match.capturedStart() - last));
        const QString segment = match.captured();
        const bool isComment = segment.startsWith("//") || segment.startsWith("/*")
                               || segment.startsWith('#');
        result += QString("<span class=\"%1\">%2</span>")
                      .arg(isComment ? QString("hl-comment") : QString("hl-string"), segment);
        last = match.capturedEnd();
    }
    result += highlightCode(escaped.mid(last));
    return result;
}

Renderer::Renderer(const RenderContext &context)
    : m_context(context)
{}

void Renderer::report(const QString &message, int offset, const QString &severity)
{
    Problem problem;
    problem.message = message;
    problem.offset = offset;
    problem.severity = severity;
    problem.rule = ruleForMessage(message);
    m_problems.append(problem);
}

QString Renderer::renderBlock(const ParsedDoc &doc, int index)
{
    QStringList parts;
    // The empty anchor carries a zero-width space, since an anchor with no text at
    // all leaves nothing in the document to scroll to.
    parts.append(QString("<section class=\"qdoc-block\" id=\"qdoc-block-%1\" data-line=\"%2\">"
                         "<a name=\"qdoc-block-%1\">&#8203;</a>")
                     .arg(index)
                     .arg(doc.line));
    m_documentedParameters.clear();
    m_parameterOffsets.clear();
    m_quoteSessions = doc.quoteSessions;
    m_currentBlockLine = doc.line;

    if (m_context.showTopicHeaders)
        parts.append(renderTopicHeader(doc));
    parts.append(renderMetaBadges(doc));
    if (!doc.brief.isEmpty()) {
        parts.append("<p class=\"qdoc-brief\">" + inlineNodes(doc.brief) + "</p>");
        checkBrief(doc);
    }
    parts.append(blocks(doc.body));
    checkSeeAlso(doc);
    checkDocumentedParameters(doc);
    parts.append("</section>");
    return parts.join('\n');
}

QString Renderer::resolveSamePageLinks(const QString &html) const
{
    return resolveLinks(html, m_anchors);
}

void Renderer::checkBrief(const ParsedDoc &doc)
{
    const QString text = plainText(doc.brief);
    if (text.isEmpty() || text.endsWith('.'))
        return;
    report("'\\brief' statement does not end with a full stop.", doc.briefOffset);
}

void Renderer::checkSeeAlso(const ParsedDoc &doc)
{
    QSet<QString> own;
    for (const Topic &topic : doc.topics) {
        static const QRegularExpression typePrefix(R"(^\S+\s+)");
        const bool isQmlProperty = topic.command == "qmlproperty"
                                   || topic.command == "qmlattachedproperty";
        QString arg = topic.arg;
        if (isQmlProperty)
            arg.remove(typePrefix);
        const QString canonical = canonicalName(arg);
        if (!canonical.isEmpty()) {
            const QStringList keys = nameKeys(canonical);
            own.unite(QSet<QString>(keys.begin(), keys.end()));
        }
    }
    if (own.isEmpty())
        return;

    const std::function<void(const Nodes &)> walk = [&](const Nodes &nodes) {
        for (const NodePtr &node : nodes) {
            if (node->kind == NodeKind::SeeAlso) {
                for (const SeeAlsoLink &seeAlso : node->links) {
                    if (own.contains(canonicalName(seeAlso.target))) {
                        report(QString("Redundant link to self in \\sa command for %1")
                                   .arg(seeAlso.target),
                               node->offset);
                    }
                }
            }
            walk(node->children);
        }
    };
    walk(doc.body);
}

void Renderer::checkDocumentedParameters(const ParsedDoc &doc)
{
    if (m_documentedParameters.isEmpty())
        return;

    const QString signature = signatureFor(doc);
    if (signature.isEmpty())
        return;
    static const QRegularExpression tooComplex(R"([<>]|\.\.\.|\(\s*\*)");
    if (tooComplex.match(signature).hasMatch())
        return;
    const QSet<QString> allowed = parameterNames(signature);
    if (allowed.isEmpty())
        return;

    const QString owner = canonicalName(signature);
    for (const QString &name : m_documentedParameters) {
        if (!name.isEmpty() && !allowed.contains(name)) {
            report(QString("No such parameter '%1' in %2()")
                       .arg(name, owner.isEmpty() ? QString("this function") : owner),
                   m_parameterOffsets.value(name));
        }
    }
}

QString Renderer::renderTopicHeader(const ParsedDoc &doc)
{
    if (doc.topics.isEmpty()) {
        if (!doc.title.isEmpty())
            return "<h1 class=\"qdoc-title\">" + escapeHtml(doc.title) + "</h1>";
        if (!doc.followingDeclaration.isEmpty()) {
            const QStringList keys = nameKeys(canonicalName(doc.followingDeclaration));
            const QString id = keys.isEmpty() ? QString() : uniqueId("m-" + slugify(keys.first()));
            if (!id.isEmpty())
                m_anchors.addAll(keys, id);
            const QString idAttr = id.isEmpty() ? QString()
                                                : QString(" id=\"%1\"").arg(escapeAttr(id));

            return QString("<h2 class=\"qdoc-signature qdoc-signature-inferred\"%1>"
                           "<code>%2</code></h2>")
                       .arg(idAttr, escapeHtml(doc.followingDeclaration));
        }
        return {};
    }

    QStringList out;

    const QStringList group = doc.metas.value("qmlpropertygroup");
    if (!group.isEmpty())
        out.append("<h1 class=\"qdoc-title\">" + escapeHtml(group.first()) + "</h1>");

    for (const Topic &topic : doc.topics) {
        const QString command = topic.command;
        const QString arg = topic.arg;
        const QString suffix = topicTitle(command);

        const QStringList keys = keysForTopic(command, arg);
        const QString anchorId = keys.isEmpty() ? QString()
                                                : uniqueId("m-" + slugify(keys.first()));
        if (!anchorId.isEmpty())
            m_anchors.addAll(keys, anchorId);
        const QString idAttr = anchorId.isEmpty()
                                   ? QString()
                                   : QString(" id=\"%1\"").arg(escapeAttr(anchorId));

        if (command == "page" || command == "module" || command == "group"
            || command == "example" || command == "qmlmodule" || command == "externalpage"
            || command == "attribution") {
            const QString title = doc.title.isEmpty()
                                      ? arg.split(QRegularExpression(R"(\s+)")).first()
                                      : doc.title;
            out.append(QString("<h1 class=\"qdoc-title\"%1>%2</h1>")
                           .arg(idAttr, escapeHtml(title)));
            if (!doc.subtitle.isEmpty())
                out.append("<p class=\"qdoc-subtitle\">" + escapeHtml(doc.subtitle) + "</p>");
            if (command != "page" || doc.title.isEmpty()) {
                out.append(QString("<p class=\"qdoc-topicline\"><span class=\"qdoc-cmd\">"
                                   "\\%1</span> %2</p>")
                               .arg(command, escapeHtml(arg)));
            }
            continue;
        }

        if (command == "fn" || command == "macro" || command == "qmlmethod"
            || command == "qmlattachedmethod" || command == "qmlsignal"
            || command == "qmlattachedsignal") {
            out.append(QString("<h2 class=\"qdoc-signature\"%1><code>%2</code></h2>")
                           .arg(idAttr, escapeHtml(arg))
                       + (suffix.isEmpty()
                              ? QString()
                              : "<p class=\"qdoc-kind\">" + escapeHtml(suffix) + "</p>"));
            continue;
        }

        if (command == "qmlproperty" || command == "qmlattachedproperty") {
            static const QRegularExpression typeAndName(R"(^(\S+)\s+(.+)$)");
            const QRegularExpressionMatch match = typeAndName.match(arg);
            const QString body
                = match.hasMatch()
                      ? QString("<code>%1</code> <span class=\"qdoc-type-sep\">:</span> "
                                "<code class=\"qdoc-type\">%2</code>")
                            .arg(escapeHtml(match.captured(2)), escapeHtml(match.captured(1)))
                      : "<code>" + escapeHtml(arg) + "</code>";
            out.append(QString("<h2 class=\"qdoc-member\"%1>%2%3</h2>")
                           .arg(idAttr,
                                body,
                                suffix.isEmpty()
                                    ? QString()
                                    : " <span class=\"qdoc-kind-inline\">" + escapeHtml(suffix)
                                          + "</span>"));
            continue;
        }

        if (command == "enum" || command == "typedef" || command == "typealias"
            || command == "property" || command == "variable" || command == "qmlenum") {
            out.append(QString("<h2 class=\"qdoc-member\"%1><code>%2</code>%3</h2>")
                           .arg(idAttr,
                                escapeHtml(arg),
                                suffix.isEmpty()
                                    ? QString()
                                    : " <span class=\"qdoc-kind-inline\">" + escapeHtml(suffix)
                                          + "</span>"));
            continue;
        }

        const QString title = suffix.isEmpty() ? arg : arg + ' ' + suffix;
        out.append(QString("<h1 class=\"qdoc-title\"%1>%2</h1>").arg(idAttr, escapeHtml(title)));
    }
    return out.join('\n');
}

QString Renderer::renderMetaBadges(const ParsedDoc &doc)
{
    using Badge = QPair<QString, QString>;
    QList<Badge> badges;
    const QHash<QString, QStringList> &meta = doc.metas;
    const auto first = [&meta](const QString &key) { return meta.value(key).value(0); };
    const auto has = [&meta](const QString &key) { return meta.contains(key); };

    if (has("internal"))
        badges.append(Badge{"internal", "Internal"});
    if (has("deprecated")) {
        const QString since = first("deprecated");
        badges.append(Badge{"deprecated",
                            since.isEmpty() ? QString("Deprecated")
                                            : "Deprecated since " + since});
    }
    if (has("preliminary"))
        badges.append(Badge{"preliminary", "Preliminary"});
    if (has("since"))
        badges.append(Badge{"since", "Since " + first("since")});
    if (has("inmodule"))
        badges.append(Badge{"module", first("inmodule")});
    if (has("inqmlmodule"))
        badges.append(Badge{"module", "import " + first("inqmlmodule")});
    if (has("nativetype"))
        badges.append(Badge{"inherits", "Native type " + first("nativetype")});
    if (has("qtvariable"))
        badges.append(Badge{"module", "QT += " + first("qtvariable")});
    if (has("inheaderfile"))
        badges.append(Badge{"header", "#include <" + first("inheaderfile") + ">"});
    if (has("inherits"))
        badges.append(Badge{"inherits", "Inherits " + first("inherits")});
    if (has("relates"))
        badges.append(Badge{"relates", "Relates to " + first("relates")});
    if (has("threadsafe"))
        badges.append(Badge{"thread", "Thread safe"});
    if (has("reentrant"))
        badges.append(Badge{"thread", "Reentrant"});
    if (has("nonreentrant"))
        badges.append(Badge{"thread", "Non-reentrant"});
    if (has("readonly"))
        badges.append(Badge{"flag", "read-only"});
    if (has("required"))
        badges.append(Badge{"flag", "required"});
    if (has("qmldefault"))
        badges.append(Badge{"flag", "default property"});
    if (has("abstract") || has("qmlabstract"))
        badges.append(Badge{"flag", "abstract"});
    if (has("overload")) {
        const QString target = first("overload");
        if (target == "primary")
            badges.append(Badge{"flag", "primary overload"});
        else if (!target.isEmpty())
            badges.append(Badge{"flag", "overloads " + target});
        else
            badges.append(Badge{"flag", "overload"});
    }
    if (has("default"))
        badges.append(Badge{"flag", "default: " + first("default")});
    for (const QString &group : meta.value("ingroup"))
        badges.append(Badge{"group", group});

    if (badges.isEmpty())
        return {};
    QStringList rendered;
    for (const auto &[kind, text] : badges) {
        rendered.append(QString("<span class=\"qdoc-badge qdoc-badge-%1\">%2</span>")
                            .arg(kind, escapeHtml(text)));
    }
    return "<p class=\"qdoc-badges\">" + rendered.join(' ') + "</p>";
}

QString Renderer::blocks(const Nodes &nodes)
{
    QStringList out;
    for (const NodePtr &node : nodes)
        out.append(block(node));
    return out.join('\n');
}

QString Renderer::block(const NodePtr &node)
{
    switch (node->kind) {
    case NodeKind::Para:
        return "<p>" + inlineNodes(node->children) + "</p>";

    case NodeKind::Heading: {
        const QString id = uniqueId(node->id.isEmpty() ? slugify(node->text) : node->id);
        m_headings.append(Heading{node->level, node->text, id});
        m_anchors.add(node->text, id);
        m_anchors.add(slugify(node->text), id);
        const QString tag = QString("h%1").arg(qMin(node->level + 1, 6));
        const QString body = node->children.isEmpty() ? escapeHtml(node->text)
                                                      : inlineNodes(node->children);
        return QString("<%1 id=\"%2\" class=\"qdoc-section\">%3</%1>")
            .arg(tag, escapeAttr(id), body);
    }

    case NodeKind::Code:
        return codeBlock(node->text, node->lang, node->bad);

    case NodeKind::Quote:
        return quoteBlock(node);

    case NodeKind::CodeGroup:
        return codeGroup(node);

    case NodeKind::List:
        return list(node);

    case NodeKind::Table:
        return table(node);

    case NodeKind::ValueList:
        return valueList(node);

    case NodeKind::Admonition: {
        static const QHash<QString, QString> labels = {
            {"note", "Note"}, {"warning", "Warning"}, {"important", "Important"},
        };
        const QString label = labels.value(node->style);
        const QString body = blocks(node->children);
        if (!label.isEmpty()) {
            return QString("<div class=\"qdoc-admonition qdoc-%1\">"
                           "<span class=\"qdoc-admonition-label\">%2:</span> %3</div>")
                .arg(node->style, label, collapseSingleParagraph(body));
        }
        return QString("<div class=\"qdoc-admonition qdoc-%1\">%2</div>").arg(node->style, body);
    }

    case NodeKind::Details:
        return QString("<details class=\"qdoc-details\"><summary>%1</summary>%2</details>")
            .arg(inlineNodes(node->summary), blocks(node->children));

    case NodeKind::Div:
        return QString("<div class=\"qdoc-div\" data-attrs=\"%1\">%2</div>")
            .arg(escapeAttr(node->attrs), blocks(node->children));

    case NodeKind::Raw:
        return rawHtml(node->text);

    case NodeKind::HorizontalRule:
        return "<hr>";

    case NodeKind::Image:
        return image(node);

    case NodeKind::Caption:
        return "<p class=\"qdoc-caption\">" + escapeHtml(node->text) + "</p>";

    case NodeKind::SeeAlso: {
        const int count = node->links.size();
        QString links;
        for (int i = 0; i < count; ++i) {
            const SeeAlsoLink &seeAlso = node->links.at(i);
            links += link(seeAlso.target,
                          escapeHtml(seeAlso.text.isEmpty() ? seeAlso.target : seeAlso.text))
                     + listSeparator(i, count);
        }
        return "<p class=\"qdoc-seealso\"><span class=\"qdoc-seealso-label\">See also</span> "
               + links + "</p>";
    }

    case NodeKind::Toc:
        blocks(node->children);
        return {};

    case NodeKind::Compares:
        return QString("<div class=\"qdoc-compares\"><p class=\"qdoc-compares-kind\">"
                       "Compares %1</p><pre>%2</pre></div>")
            .arg(escapeHtml(node->style), escapeHtml(node->text));

    case NodeKind::BadMarkup:
        return badMarkup(node, false);

    case NodeKind::SectionMarker:
        return QString("<div class=\"qdoc-section-marker\" title=\"Included with \\include "
                       "&lt;file&gt; %1\"><span class=\"qdoc-section-marker-name\">%2</span></div>")
            .arg(escapeAttr(node->name), escapeHtml(node->name));

    case NodeKind::Placeholder:
        if (!node->problem.isEmpty()) {
            report(node->problem, node->offset);
            return placeholder(QString("\\%1 %2").arg(node->name, node->args), node->problem);
        }
        return placeholder(node->name, node->args);

    case NodeKind::Anchor:
        m_anchors.add(node->name, node->id);
        m_anchors.add(node->id, node->id);
        return QString("<span class=\"qdoc-anchor\" id=\"%1\"></span>").arg(escapeAttr(node->id));

    default:
        return {};
    }
}

QString Renderer::list(const NodePtr &node)
{
    QStringList items;
    for (const NodePtr &item : node->children)
        items.append("<li>" + collapseSingleParagraph(blocks(item->children)) + "</li>");
    const QString body = items.join('\n');

    if (node->listStyle == ListStyle::Bullet)
        return "<ul>" + body + "</ul>";
    static const QHash<int, QString> types = {
        {int(ListStyle::Numeric), "1"},
        {int(ListStyle::LowerAlpha), "a"},
        {int(ListStyle::UpperAlpha), "A"},
        {int(ListStyle::LowerRoman), "i"},
        {int(ListStyle::UpperRoman), "I"},
    };
    const QString start = node->listStart != 1 ? QString(" start=\"%1\"").arg(node->listStart)
                                               : QString();
    return QString("<ol type=\"%1\"%2>%3</ol>")
        .arg(types.value(int(node->listStyle), "1"), start, body);
}

QString Renderer::table(const NodePtr &node)
{
    QStringList rows;
    for (const NodePtr &row : node->children) {
        const QString tag = row->header ? QString("th") : QString("td");
        QString cells;
        for (const NodePtr &cell : row->children) {
            QStringList attrs;
            if (cell->colspan > 1)
                attrs.append(QString("colspan=\"%1\"").arg(cell->colspan));
            if (cell->rowspan > 1)
                attrs.append(QString("rowspan=\"%1\"").arg(cell->rowspan));
            const QString open = attrs.isEmpty() ? QString("<%1>").arg(tag)
                                                 : QString("<%1 %2>").arg(tag, attrs.join(' '));
            cells += open + collapseSingleParagraph(blocks(cell->children)) + "</" + tag + ">";
        }
        rows.append("<tr>" + cells + "</tr>");
    }
    const QString caption = node->caption.isEmpty()
                                ? QString()
                                : "<caption>" + escapeHtml(node->caption) + "</caption>";
    return "<table class=\"qdoc-table\">" + caption + rows.join('\n') + "</table>";
}

QString Renderer::valueList(const NodePtr &node)
{
    QStringList rows;
    for (const NodePtr &item : node->children) {
        const QString since
            = item->since.isEmpty()
                  ? QString()
                  : QString(" <span class=\"qdoc-badge qdoc-badge-since\">since %1</span>")
                        .arg(escapeHtml(item->since));
        rows.append(QString("<tr><td class=\"qdoc-value-name\"><code>%1</code>%2</td>"
                            "<td>%3</td></tr>")
                        .arg(escapeHtml(item->name),
                             since,
                             collapseSingleParagraph(blocks(item->children))));
    }
    return "<table class=\"qdoc-table qdoc-valuelist\"><tr><th>Constant</th>"
           "<th>Description</th></tr>"
           + rows.join('\n') + "</table>";
}

QString Renderer::codeBlock(const QString &text, const QString &lang, bool bad)
{
    const QString cls = bad ? QString("qdoc-code qdoc-badcode") : QString("qdoc-code");
    const QString label = bad ? QString("<span class=\"qdoc-code-label\">avoid</span>")
                              : QString();
    return QString("<div class=\"%1\">%2<pre><code>%3</code></pre></div>")
        .arg(cls, label, highlight(text, lang));
}

QString Renderer::codeGroup(const NodePtr &node)
{
    QStringList chunks;
    QStringList failures;
    Utils::FilePaths paths;
    QString lang;

    for (const NodePtr &part : node->children) {
        if (part->kind == NodeKind::Dots || part->kind == NodeKind::CodeLine) {
            if (!chunks.isEmpty() && chunks.last().endsWith('\n'))
                chunks.last().chop(1);
            chunks.append(part->kind == NodeKind::Dots ? QString(part->indent, ' ') + "..."
                                                       : QString());
            continue;
        }
        if (part->kind == NodeKind::Code) {
            chunks.append(part->text);
            if (lang.isEmpty())
                lang = part->lang;
            continue;
        }
        if (m_context.doc.confFile.isEmpty()) {
            failures.append(placeholder(writtenQuote(part), "quoting needs a .qdocconf"));
            continue;
        }
        const QuoteResult result = resolveQuotePart(part);
        if (!result.ok) {
            failures.append(placeholder(writtenQuote(part), result.reason));
            continue;
        }
        // A group that only skips prints nothing, and an empty chunk would defeat
        // the trailing-newline chop above.
        if (result.text.trimmed().isEmpty())
            continue;
        chunks.append(result.text);
        if (!paths.contains(result.path))
            paths.append(result.path);
        // The language belongs to the atom, and only its creator sets one, so the
        // first part decides.
        if (lang.isEmpty())
            lang = part->lang.isEmpty() ? result.lang : part->lang;
    }

    if (chunks.isEmpty())
        return failures.join('\n');

    return sourceLabel(paths) + codeBlock(chunks.join('\n'), lang, false)
           + failures.join('\n');
}

// A \quotefromfile split across several listings resolves once per listing, so a
// failure would otherwise be reported once per listing as well.
QuoteResult Renderer::resolveQuotePart(const NodePtr &node)
{
    QuoteCache *cache = m_context.quoteCache ? m_context.quoteCache : &m_ownQuoteCache;
    const QuoteResult result = resolveQuote(*node,
                                            m_quoteSessions.value(node->sessionId),
                                            m_context.doc.exampleDirs,
                                            cache);
    if (result.ok)
        return result;
    // A session is one command however many listings its steps are split over, so it
    // is reported once; a \snippet stands for itself.
    const QString key = QString("%1\n%2\n%3")
                            .arg(node->sessionId)
                            .arg(node->name, result.reason);
    if (node->sessionId == 0 || !m_reportedQuoteFailures.contains(key)) {
        report(result.reason, node->offset);
        m_reportedQuoteFailures.insert(key);
    }
    return result;
}

QString Renderer::quoteBlock(const NodePtr &node)
{
    if (m_context.doc.confFile.isEmpty())
        return placeholder(writtenQuote(node), "quoting needs a .qdocconf");
    const QuoteResult result = resolveQuotePart(node);
    if (!result.ok)
        return placeholder(writtenQuote(node), result.reason);
    if (result.text.trimmed().isEmpty())
        return {};
    return sourceLabel({result.path})
           + codeBlock(result.text, node->lang.isEmpty() ? result.lang : node->lang, false);
}

QString Renderer::image(const NodePtr &node)
{
    const QString alt = node->alt;
    const QString command = node->isInline ? QString("\\inlineimage") : QString("\\image");

    if (node->missingAltText && m_context.doc.reportMissingAltText) {
        report(QString("%1 %2 is without a textual description, QDoc will not generate an "
                       "alt text for the image.")
                   .arg(command, node->name),
               node->offset);
    }

    if (m_context.doc.confFile.isEmpty()) {
        // Nowhere to look yet, which is not the author's doing, so nothing is
        // reported either.
        return placeholder(QString("%1 %2").arg(command, node->name),
                           "images need a .qdocconf");
    }
    const Utils::FilePath file = findImageFile(node->name, m_context.doc.imageSearchDirs);
    if (file.isEmpty()) {
        report(QString("Missing image: %1").arg(node->name), node->offset);
        return placeholder(QString("%1 %2").arg(command, node->name),
                           "image not found in imagedirs");
    }
    const QString uri = QUrl::fromLocalFile(file.toFSPathString()).toString();

    const QString mode = m_context.altTextDisplay;
    const QString cls = node->isInline ? QString("qdoc-inlineimage") : QString("qdoc-image");
    const QString title = mode == "hover" && !alt.isEmpty()
                              ? QString(" title=\"%1\"").arg(escapeAttr(alt))
                              : QString();
    const QString img = QString("<img class=\"%1\" src=\"%2\" alt=\"%3\"%4>")
                            .arg(cls, escapeAttr(uri), escapeAttr(alt), title);

    if (node->isInline) {
        if (mode == "hidden" || alt.isEmpty() || mode == "hover")
            return img;
        return QString("<span class=\"qdoc-inlineimage-wrap\" title=\"%1\">%2</span>")
            .arg(escapeAttr(alt), img);
    }

    QString caption;
    if (mode == "caption") {
        if (!alt.isEmpty())
            caption = "<figcaption class=\"qdoc-alt\">" + escapeHtml(alt) + "</figcaption>";
        else if (node->missingAltText)
            caption = "<figcaption class=\"qdoc-alt qdoc-alt-missing\">no alt text</figcaption>";
    }
    return "<figure class=\"qdoc-figure\">" + img + caption + "</figure>";
}

QString Renderer::rawHtml(const QString &html)
{
    if (m_context.doc.confFile.isEmpty() || html.isEmpty() || !html.contains("src"))
        return html;

    static const QRegularExpression srcAttr(
        R"rx((\ssrc\s*=\s*)("([^"]*)"|'([^']*)'))rx",
        QRegularExpression::CaseInsensitiveOption);
    QString out;
    int last = 0;
    for (const QRegularExpressionMatch &match : srcAttr.globalMatch(html)) {
        const QString src = match.hasCaptured(3) ? match.captured(3) : match.captured(4);
        const Utils::FilePath file = findRawImageFile(src,
                                                      m_context.doc.imageSearchDirs,
                                                      m_context.doc.extraImageFiles);
        if (file.isEmpty())
            continue;
        const QString uri = QUrl::fromLocalFile(file.toFSPathString()).toString();
        out += html.mid(last, match.capturedStart() - last);
        out += match.captured(1) + '"' + escapeAttr(uri) + '"';
        last = match.capturedEnd();
    }
    out += html.mid(last);
    return out;
}

QString Renderer::badMarkup(const NodePtr &node, bool isInline)
{
    const QString severity = node->severity == "error" ? QString("error") : QString("warning");
    const QString label = escapeHtml(node->text);
    const QString title = escapeAttr(node->message.isEmpty() ? QString("Problem in this markup")
                                                             : node->message);

    if (isInline) {
        return QString("<span class=\"qdoc-bad qdoc-bad-%1\" title=\"%2\">%3</span>")
            .arg(severity, title, label.isEmpty() ? QString("&#9888;") : label);
    }
    return QString("<div class=\"qdoc-bad-block qdoc-bad-%1\" role=\"note\" title=\"%2\">%3"
                   "<span class=\"qdoc-bad-message\">%4</span></div>")
        .arg(severity,
             title,
             label.isEmpty() ? QString() : "<code>" + label + "</code> ",
             escapeHtml(node->message));
}

QString Renderer::placeholder(const QString &command, const QString &detail)
{
    if (!m_context.showUnsupported)
        return {};
    const bool written = command.startsWith('\\');
    const QString label = written ? command
                                  : '\\' + command
                                        + (detail.isEmpty() ? QString() : ' ' + detail);
    const QString dash = QString(" %1 ").arg(QChar(0x2014));
    const QString note = dash + (written && !detail.isEmpty()
                                     ? detail
                                     : QString("not available in preview"));
    return QString("<div class=\"qdoc-placeholder\" role=\"note\"><code>%1</code>"
                   "<span class=\"qdoc-placeholder-note\">%2</span></div>")
        .arg(escapeHtml(label), escapeHtml(note));
}

QString Renderer::inlineNodes(const Nodes &nodes)
{
    QString out;
    for (const NodePtr &node : nodes)
        out += inlineNode(node);
    return out;
}

QString Renderer::inlineNode(const NodePtr &node)
{
    switch (node->kind) {
    case NodeKind::Text:
        return escapeHtml(node->text);

    case NodeKind::InlineCode:
        return "<code class=\"qdoc-inline-code\">" + escapeHtml(node->text) + "</code>";

    case NodeKind::Format: {
        const QString inner = inlineNodes(node->children);
        const QString &style = node->style;
        if (style == "bold")
            return "<b>" + inner + "</b>";
        if (style == "italic")
            return "<i>" + inner + "</i>";
        if (style == "parameter") {
            const QString name = plainText(node->children);
            if (!m_documentedParameters.contains(name))
                m_documentedParameters.append(name);
            m_parameterOffsets.insert(name, node->offset);
            return "<i class=\"qdoc-parameter\">" + inner + "</i>";
        }
        if (style == "teletype")
            return "<code>" + inner + "</code>";
        if (style == "subscript")
            return "<sub>" + inner + "</sub>";
        if (style == "superscript")
            return "<sup>" + inner + "</sup>";
        if (style == "underline")
            return "<u>" + inner + "</u>";
        if (style == "uicontrol")
            return "<span class=\"qdoc-uicontrol\">" + inner + "</span>";
        if (style == "trademark")
            return inner + "<sup class=\"qdoc-tm\">&trade;</sup>";
        if (style == "span")
            return QString("<span class=\"%1\">%2</span>").arg(escapeAttr(node->cssClass), inner);
        if (style == "notranslate") {
            return "<span class=\"qdoc-notranslate\">" + inner + "</span>";
        }
        return inner;
    }

    case NodeKind::Link:
        return link(node->target, inlineNodes(node->children), node->linkParams);

    case NodeKind::Image:
        return image(node);

    case NodeKind::Anchor:
        noteTarget(node);
        m_anchors.add(node->name, node->id);
        m_anchors.add(node->id, node->id);
        return QString("<span class=\"qdoc-anchor\" id=\"%1\"></span>").arg(escapeAttr(node->id));

    case NodeKind::BadMarkup:
        return badMarkup(node, true);

    case NodeKind::IncludeParameter:
        return QString("<span class=\"qdoc-include-argument\" title=\"Argument %1 of "
                       "\\include, supplied by the including file\">\\%1</span>")
            .arg(node->index);

    case NodeKind::UnresolvedMacro: {
        QStringList missing;
        for (const QString &variable : node->variables)
            missing.append('$' + variable);
        const QString hint
            = missing.isEmpty()
                  ? QString("\\%1 is defined with an empty value.").arg(node->name)
                  : QString("\\%1 expands to %2, which the documentation build supplies.")
                        .arg(node->name, missing.join(", "));
        return QString("<span class=\"qdoc-unresolved-macro\" title=\"%1\">\\%2</span>")
            .arg(escapeAttr(hint), escapeHtml(node->name));
    }

    case NodeKind::LineBreak:
        return "<br>";

    case NodeKind::Raw:
        return rawHtml(node->text);

    default:
        return {};
    }
}

QString Renderer::link(const QString &target, const QString &innerHtml, const QString &params)
{
    const QString hint = params.isEmpty() ? target : params + ": " + target;
    return QString("<a class=\"qdoc-link\" title=\"%1\" data-target=\"%2\">%3</a>")
        .arg(escapeAttr(hint), escapeAttr(target), innerHtml);
}

void Renderer::noteTarget(const NodePtr &node)
{
    if (node->name.isEmpty())
        return;
    if (m_seenTargets.contains(node->name)) {
        report(QString("Duplicate target name '%1'. The previous occurrence is here: line %2")
                   .arg(node->name)
                   .arg(m_seenTargets.value(node->name) + 1),
               node->offset);
        return;
    }
    m_seenTargets.insert(node->name, m_currentBlockLine);
}

QString Renderer::uniqueId(const QString &base)
{
    QString id = base.isEmpty() ? QString("section") : base;
    int n = 2;
    while (m_usedIds.contains(id))
        id = QString("%1-%2").arg(base).arg(n++);
    m_usedIds.insert(id);
    return id;
}

// Where an offset into a comment body lands in the file. extractDocBlocks() records
// where each body starts and demargin() preserves length, so a body offset maps onto a
// file offset by addition. The exception is a //! block, whose prefixes are stripped
// when its lines are joined; there the block's first line is used.
static void resolvePositions(const QString &text,
                             const DocBlock &block,
                             const QList<int> &lineStarts,
                             QList<Problem> *problems)
{
    for (Problem &problem : *problems) {
        if (block.fromLineComment) {
            problem.line = block.line + 1;
            problem.column = 0;
            continue;
        }
        const int absolute = qBound(0, block.offset + problem.offset, text.size());
        const auto after = std::upper_bound(lineStarts.begin(), lineStarts.end(), absolute);
        const int index = int(after - lineStarts.begin()) - 1;
        problem.line = index + 1;
        problem.column = absolute - lineStarts.at(index);
    }
}

// Only PureDocParser warns about this, and it handles .qdoc files: in a C++ source the
// comment attaches to the next declaration, and a .qdocinc is a fragment of somebody
// else's topic.
static bool documentsNothing(const DocumentKind &kind, const ParsedDoc &doc)
{
    return kind.wholeFileIsDoc && !kind.fragment && doc.topics.isEmpty()
           && (!doc.body.isEmpty() || !doc.brief.isEmpty());
}

PreviewPage renderPreviewPage(const QString &text,
                              const Utils::FilePath &file,
                              const RenderContext &context)
{
    const DocumentKind kind = documentKind(file.path(), text);
    const QList<DocBlock> docBlocks = extractDocBlocks(text, kind);

    ParserContext parserContext;
    parserContext.fragment = kind.fragment;
    parserContext.quoted = kind.quoted;
    parserContext.macros = context.doc.macros;
    parserContext.defines = context.doc.defines;
    parserContext.codeLanguages = context.doc.codeLanguages;
    parserContext.tabSize = context.doc.tabSize;
    const Utils::FilePaths includeDirs = Utils::FilePaths{file.parentDir()}
                                         + context.doc.sourceDirs;
    if (context.quoteCache)
        context.quoteCache->forgetContents();

    // The same cache as the quoting commands use: without it every \include pays a
    // directory walk again on every keystroke.
    QHash<QString, Utils::FilePath> *fileSearch = context.quoteCache
                                                     ? &context.quoteCache->fileSearch
                                                     : nullptr;
    parserContext.resolveInclude = [includeDirs, fileSearch](const QString &name) {
        IncludeResult result;
        const Utils::FilePath resolved = findQuoteFile(name, includeDirs, fileSearch);
        if (resolved.isEmpty())
            return result;
        const Utils::Result<QByteArray> contents = resolved.fileContents();
        if (!contents)
            return result;
        result.found = true;
        result.text = QString::fromUtf8(*contents);
        result.path = resolved.toFSPathString();
        return result;
    };

    QList<int> lineStarts = {0};
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == '\n')
            lineStarts.append(i + 1);
    }

    Renderer renderer(context);
    PreviewPage page;
    QStringList rendered;
    int index = 0;
    for (const DocBlock &docBlock : docBlocks) {
        ParsedDoc doc = parseDoc(docBlock.text, parserContext);
        doc.line = docBlock.line;
        doc.followingDeclaration = docBlock.followingDeclaration;
        if (documentsNothing(kind, doc)) {
            Problem problem;
            problem.message = "This qdoc comment contains no topic command (for example "
                              "'\\class', '\\page').";
            problem.rule = "missing-topic-command";
            problem.severity = "info";
            doc.problems.append(problem);
        }
        const int rendererProblems = renderer.problems().size();
        rendered.append(renderer.renderBlock(doc, index++));
        QList<Problem> blockProblems = doc.problems;
        blockProblems.append(renderer.problems().mid(rendererProblems));
        resolvePositions(text, docBlock, lineStarts, &blockProblems);
        page.problems.append(blockProblems);
    }
    page.headings = renderer.headings();
    for (const DocBlock &docBlock : docBlocks)
        page.blocks.append({docBlock.line, docBlock.endLine});
    page.body = renderer.resolveSamePageLinks(rendered.join('\n'));
    return page;
}

} // namespace QDoc::Internal
