// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

namespace QDoc::Internal {

enum class NodeKind {
    Text,
    InlineCode,
    Format,
    Link,
    Image,
    Anchor,
    BadMarkup,
    IncludeParameter,
    UnresolvedMacro,
    LineBreak,
    Raw,
    Para,
    Heading,
    Code,
    Quote,
    CodeGroup,
    Dots,
    CodeLine,
    List,
    ListItem,
    Table,
    TableRow,
    TableCell,
    ValueList,
    ValueItem,
    Admonition,
    Details,
    Div,
    SeeAlso,
    Toc,
    Compares,
    SectionMarker,
    Placeholder,
    HorizontalRule,
    Caption,
};

enum class ListStyle { Bullet, Numeric, LowerAlpha, UpperAlpha, LowerRoman, UpperRoman };

class QuoteStep
{
public:
    QString op;
    QString pattern;
};

class SeeAlsoLink
{
public:
    QString target;
    QString text;
};

class Node;
using NodePtr = std::shared_ptr<Node>;
using Nodes = QList<NodePtr>;

class Node
{
public:
    explicit Node(NodeKind k = NodeKind::Text) : kind(k) {}

    NodeKind kind;

    QString text;
    QString name;
    QString id;
    QString style;
    QString lang;
    QString target;
    QString linkParams;
    QString alt;
    QString since;
    QString identifier;
    QString attrs;
    QString width;
    QString caption;
    QString cssClass;
    QString message;
    QString severity;
    QString rule;
    QString args;
    QString problem;
    QString mode;
    QString listPrefix;
    QString listSuffix;

    ListStyle listStyle = ListStyle::Bullet;
    int level = 0;
    int listStart = 1;
    int index = 0;
    int indent = 0;
    int colspan = 1;
    int rowspan = 1;
    int offset = 0;
    int sessionId = 0;
    int groupIndex = 0;

    bool header = false;
    bool bad = false;
    bool isInline = false;
    bool missingAltText = false;
    bool implicit = false;
    bool hasAlt = false;
    bool hasLinkParams = false;

    QStringList variables;
    QList<SeeAlsoLink> links;
    QList<QuoteStep> steps;

    Nodes summary;
    Nodes children;
};

inline NodePtr makeNode(NodeKind kind)
{
    return std::make_shared<Node>(kind);
}

inline NodePtr makeText(const QString &text)
{
    NodePtr node = makeNode(NodeKind::Text);
    node->text = text;
    return node;
}

class Problem
{
public:
    QString message;
    QString severity;
    QString rule;
    int offset = 0;   // Into the comment body
    int line = 0;     // 1-based line in the file, 0 when it is not known
    int column = 0;
    bool synthetic = false;
};

class Topic
{
public:
    QString command;
    QString arg;
    QString bracketed;
};

class ParsedDoc
{
public:
    QList<Topic> topics;
    QHash<QString, QStringList> metas;
    Nodes brief;
    Nodes body;
    QList<Problem> problems;
    QHash<int, QList<QList<QuoteStep>>> quoteSessions;
    QString title;
    QString subtitle;
    QString followingDeclaration;
    int briefOffset = 0;
    int line = 0;
    bool isInternal = false;
};

class Macro
{
public:
    QString name;
    QString def;
    QString raw;
    QString match;
    int params = 0;
    bool hasRaw = false;
    bool unresolved = false;
    QStringList variables;
};

class IncludeResult
{
public:
    bool found = false;
    QString text;
    QString path;
};

using IncludeResolver = std::function<IncludeResult(const QString &)>;

class ParserContext
{
public:
    QHash<QString, Macro> macros;
    QSet<QString> defines;
    QSet<QString> codeLanguages;
    IncludeResolver resolveInclude;
    int tabSize = 8;
    bool fragment = false;
    bool quoted = false;
    bool singleLineParagraph = false;
};

ParsedDoc parseDoc(const QString &source, const ParserContext &context = {});

class ListHint
{
public:
    ListStyle style = ListStyle::Bullet;
    int start = 1;
    QString prefix;
    QString suffix;
    bool unrecognized = false;
};

ListHint parseListHint(const QString &hint);

int fromAlpha(const QString &text);

int fromRoman(const QString &text);

QString slugify(const QString &text);

QString cleanLink(const QString &link);

QString dedent(const QString &text);

QString flattenText(const Nodes &nodes);

class DocBlock
{
public:
    QString text;
    QString followingDeclaration;
    int line = 0;
    int endLine = 0;
    int column = 0;
    int offset = 0;
    bool fromLineComment = false;
};

class DocumentKind
{
public:
    bool wholeFileIsDoc = false;
    bool fragment = false;
    bool quoted = false;
};

DocumentKind documentKind(const QString &fileName, const QString &text);

QList<DocBlock> extractDocBlocks(const QString &text, const DocumentKind &kind);

bool looksLikeQDoc(const QString &text);

QString ruleForMessage(const QString &message);

QString topicTitle(const QString &command);

// Whether a backslash word is a command QDoc knows, and whether it names or
// annotates the documented entity rather than marking up text.
bool isKnownCommand(const QString &command);
bool isEntityCommand(const QString &command);

enum class MarkupSpan {
    Body,     // Prose
    Command,  // A backslash command
    Entity,   // What a topic or meta command names
    Argument, // A braced argument
    Comment,  // A // line and the /*! and */ delimiters
    Code,     // The body of a verbatim block
};

class MarkupToken
{
public:
    int start = 0;
    int length = 0;
    MarkupSpan span = MarkupSpan::Body;
};

// Splits one line of markup into spans, for highlighting it. state carries what the
// previous line left open and is updated; 0 is ordinary markup.
QList<MarkupToken> scanMarkupLine(const QString &text, int *state);

} // namespace QDoc::Internal
