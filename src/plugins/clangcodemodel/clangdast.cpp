// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdast.h"

#include <languageclient/client.h>
#include <languageserverprotocol/jsonkeys.h>
#include <languageserverprotocol/lsptypes.h>
#include <utils/hostosinfo.h>
#include <utils/filepath.h>

#include <QStringView>

using namespace Core;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace Utils;

namespace ClangCodeModel::Internal {

static constexpr char roleKey[] = "role";
static constexpr char arcanaKey[] = "arcana";

QString ClangdAstNode::role() const { return typedValue<QString>(roleKey); }
QString ClangdAstNode::kind() const { return typedValue<QString>(kindKey); }
std::optional<QString> ClangdAstNode::detail() const
{
    return optionalValue<QString>(detailKey);
}
std::optional<QString> ClangdAstNode::arcana() const
{
    return optionalValue<QString>(arcanaKey);
}
Range ClangdAstNode::range() const { return typedValue<Range>(rangeKey); }
bool ClangdAstNode::hasRange() const { return contains(rangeKey); }
bool ClangdAstNode::isValid() const { return contains(roleKey) && contains(kindKey); }

std::optional<QList<ClangdAstNode>> ClangdAstNode::children() const
{
    return optionalArray<ClangdAstNode>(childrenKey);
}

bool ClangdAstNode::arcanaContains(const QString &s) const
{
    const std::optional<QString> arcanaString = arcana();
    return arcanaString && arcanaString->contains(s);
}

bool ClangdAstNode::isFunction() const
{
    return role() == "declaration"
           && (kind() == "Function" || kind() == "FunctionProto" || kind() == "CXXMethod");
}

bool ClangdAstNode::isMemberFunctionCall() const
{
    return role() == "expression" && (kind() == "CXXMemberCall"
                                      || (kind() == "Member" && arcanaContains("member function")));
}

bool ClangdAstNode::isPureVirtualDeclaration() const
{
    return role() == "declaration" && kind() == "CXXMethod" && arcanaContains("virtual pure");
}

bool ClangdAstNode::isPureVirtualDefinition() const
{
    return role() == "declaration" && kind() == "CXXMethod" && arcanaContains("' pure");
}

bool ClangdAstNode::mightBeAmbiguousVirtualCall() const
{
    if (!isMemberFunctionCall())
        return false;
    bool hasBaseCast = false;
    bool hasRecordType = false;
    const QList<ClangdAstNode> childList = children().value_or(QList<ClangdAstNode>());
    for (const ClangdAstNode &c : childList) {
        if (!hasBaseCast && c.detailIs("UncheckedDerivedToBase"))
            hasBaseCast = true;
        if (!hasRecordType && c.role() == "specifier" && c.kind() == "TypeSpec")
            hasRecordType = true;
        if (hasBaseCast && hasRecordType)
            return false;
    }
    return true;
}

bool ClangdAstNode::isTemplateParameterDeclaration() const
{
    return role() == "declaration" && (kind() == "TemplateTypeParm"
                                       || kind() == "NonTypeTemplateParm");
}

QString ClangCodeModel::Internal::ClangdAstNode::type() const
{
    const std::optional<QString> arcanaString = arcana();
    if (!arcanaString)
        return {};
    return typeFromPos(*arcanaString, 0);
}

QString ClangdAstNode::typeFromPos(const QString &s, int pos) const
{
    const int quote1Offset = s.indexOf('\'', pos);
    if (quote1Offset == -1)
        return {};
    const int quote2Offset = s.indexOf('\'', quote1Offset + 1);
    if (quote2Offset == -1)
        return {};
    if (s.mid(quote2Offset + 1, 2) == ":'")
        return typeFromPos(s, quote2Offset + 2);
    return s.mid(quote1Offset + 1, quote2Offset - quote1Offset - 1);
}

HelpItem::Category ClangdAstNode::qdocCategoryForDeclaration(HelpItem::Category fallback)
{
    const auto childList = children();
    if (!childList || childList->size() < 2)
        return fallback;
    const ClangdAstNode c1 = childList->first();
    if (c1.role() != "type" || c1.kind() != "Auto")
        return fallback;
    QList<ClangdAstNode> typeCandidates = {childList->at(1)};
    while (!typeCandidates.isEmpty()) {
        const ClangdAstNode n = typeCandidates.takeFirst();
        if (n.role() == "type") {
            if (n.kind() == "Enum")
                return HelpItem::Enum;
            if (n.kind() == "Record")
                return HelpItem::ClassOrNamespace;
            return fallback;
        }
        typeCandidates << n.children().value_or(QList<ClangdAstNode>());
    }
    return fallback;
}

bool ClangdAstNode::hasConstType() const
{
    QString theType = type();
    if (theType.endsWith("const"))
        theType.chop(5);

    // We don't care about the "inner" type of templates.
    const int openAngleBracketPos = theType.indexOf('<');
    if (openAngleBracketPos != -1) {
        const int closingAngleBracketPos = theType.lastIndexOf('>');
        if (closingAngleBracketPos > openAngleBracketPos) {
            theType = theType.left(openAngleBracketPos)
                    + theType.mid(closingAngleBracketPos + 1);
        }
    }
    const int xrefCount = theType.count("&&");
    const int refCount = theType.count('&') - 2 * xrefCount;
    const int ptrRefCount = theType.count('*') + refCount;
    const int constCount = theType.count("const");
    if (ptrRefCount == 0)
        return constCount > 0 || detailIs("LValueToRValue") || arcanaContains("xvalue");
    return ptrRefCount <= constCount;
}

bool ClangdAstNode::childContainsRange(int index, const LanguageServerProtocol::Range &range) const
{
    const std::optional<QList<ClangdAstNode>> childList = children();
    return childList && childList->size() > index && childList->at(index).range().contains(range);
}

bool ClangdAstNode::hasChildWithRole(const QString &role) const
{
    return Utils::contains(children().value_or(QList<ClangdAstNode>()),
                           [&role](const ClangdAstNode &c) { return c.role() == role; });
}

bool ClangdAstNode::hasChild(const std::function<bool(const ClangdAstNode &)> &predicate,
                             bool recursive) const
{
    std::function<bool(const ClangdAstNode &)> fullPredicate = predicate;
    if (recursive) {
        fullPredicate = [&predicate](const ClangdAstNode &n) {
            if (predicate(n))
                return true;
            return n.hasChild(predicate, true);
        };
    }
    return Utils::contains(children().value_or(QList<ClangdAstNode>()), fullPredicate);
}

QString ClangdAstNode::operatorString() const
{
    if (kind() == "BinaryOperator")
        return detail().value_or(QString());
    QTC_ASSERT(kind() == "CXXOperatorCall", return {});
    const std::optional<QString> arcanaString = arcana();
    if (!arcanaString)
        return {};
    const int closingQuoteOffset = arcanaString->lastIndexOf('\'');
    if (closingQuoteOffset <= 0)
        return {};
    const int openingQuoteOffset = arcanaString->lastIndexOf('\'', closingQuoteOffset - 1);
    if (openingQuoteOffset == -1)
        return {};
    return arcanaString->mid(openingQuoteOffset + 1, closingQuoteOffset
                             - openingQuoteOffset - 1);
}

ClangdAstNode::FileStatus ClangdAstNode::fileStatus(const FilePath &thisFile) const
{
    const std::optional<QString> arcanaString = arcana();
    if (!arcanaString)
        return FileStatus::Unknown;

    // Example arcanas:
    // "FunctionDecl 0x7fffb5d0dbd0 </tmp/test.cpp:1:1, line:5:1> line:1:6 func 'void ()'"
    // "VarDecl 0x7fffb5d0dcf0 </tmp/test.cpp:2:5, /tmp/test.h:1:1> /tmp/test.cpp:2:10 b 'bool' cinit"
    // The second one is for a particularly silly construction where the RHS of an
    // initialization comes from an included header.
    const int openPos = arcanaString->indexOf('<');
    if (openPos == -1)
        return FileStatus::Unknown;
    const int closePos = arcanaString->indexOf('>', openPos + 1);
    if (closePos == -1)
        return FileStatus::Unknown;
    bool hasOurs = false;
    bool hasOther = false;
    for (int startPos = openPos + 1; startPos < closePos;) {
        int colon1Pos = arcanaString->indexOf(':', startPos);
        if (colon1Pos == -1 || colon1Pos > closePos)
            break;
        if (HostOsInfo::isWindowsHost())
            colon1Pos = arcanaString->indexOf(':', colon1Pos + 1);
        if (colon1Pos == -1 || colon1Pos > closePos)
            break;
        const int colon2Pos = arcanaString->indexOf(':', colon1Pos + 2);
        if (colon2Pos == -1 || colon2Pos > closePos)
            break;
        const int line = subViewEnd(*arcanaString, colon1Pos + 1, colon2Pos).toInt();
        if (line == 0)
            break;
        const QStringView fileOrLineString = subViewEnd(*arcanaString, startPos, colon1Pos);
        if (fileOrLineString != QLatin1String("line")) {
            if (FilePath::fromUserInput(fileOrLineString.toString()) == thisFile)
                hasOurs = true;
            else
                hasOther = true;
        }
        const int commaPos = arcanaString->indexOf(',', colon2Pos + 2);
        if (commaPos != -1)
            startPos = commaPos + 2;
        else
            break;
    }
    if (hasOurs)
        return hasOther ? FileStatus::Mixed : FileStatus::Ours;
    return hasOther ? FileStatus::Foreign : FileStatus::Unknown;
}

void ClangdAstNode::print(int indent) const
{
    (qDebug().noquote() << QByteArray(indent, ' ')).quote() << role() << kind()
            << detail().value_or(QString()) << arcana().value_or(QString()) << range();
    for (const ClangdAstNode &c : children().value_or(QList<ClangdAstNode>()))
        c.print(indent + 2);
}

QStringView subViewLen(const QString &s, qsizetype start, qsizetype length)
{
    if (start < 0 || length < 0 || start + length > s.length())
        return {};
    return QStringView(s).mid(start, length);
}

QStringView subViewEnd(const QString &s, qsizetype start, qsizetype end)
{
    return subViewLen(s, start, end - start);
}

class AstPathCollector
{
public:
    AstPathCollector(const ClangdAstNode &root, const Range &range)
        : m_root(root), m_range(range) {}

    ClangdAstPath collectPath()
    {
        if (!m_root.isValid())
            return {};
        visitNode(m_root, true);
        return m_done ? m_path : m_longestSubPath;
    }

private:
    void visitNode(const ClangdAstNode &node, bool isRoot = false)
    {
        if (!isRoot && (!node.hasRange() || !node.range().contains(m_range)))
            return;
        m_path << node;

        class PathDropper {
        public:
            PathDropper(AstPathCollector &collector) : m_collector(collector) {};
            ~PathDropper() {
                if (m_collector.m_done)
                    return;
                if (m_collector.m_path.size() > m_collector.m_longestSubPath.size())
                    m_collector.m_longestSubPath = m_collector.m_path;
                m_collector.m_path.removeLast();
            }
        private:
            AstPathCollector &m_collector;
        } pathDropper(*this);

        // Still traverse the children, because they could have the same range.
        if (node.range() == m_range)
            m_done = true;

        const auto children = node.children();
        if (!children)
            return;

        QList<ClangdAstNode> childrenToCheck;
        if (node.kind() == "Function" || node.role() == "expression") {
            // Functions and expressions can contain implicit nodes that make the list unsorted.
            // They cannot be ignored, as we need to consider them in certain contexts.
            // Therefore, the binary search cannot be used here.
            childrenToCheck = *children;
        } else {
            for (auto it = std::lower_bound(children->cbegin(), children->cend(), m_range,
                                            leftOfRange);
                 it != children->cend() && !m_range.isLeftOf(it->range()); ++it) {
                childrenToCheck << *it;
            }
        }

        const bool wasDone = m_done;
        for (const ClangdAstNode &child : std::as_const(childrenToCheck)) {
            visitNode(child);
            if (m_done && !wasDone)
                break;
        }
    }

    static bool leftOfRange(const ClangdAstNode &node, const Range &range)
    {
        // Class and struct nodes can contain implicit constructors, destructors and
        // operators, which appear at the end of the list, but whose range is the same
        // as the class name. Therefore, we must force them not to compare less to
        // anything else.
        return node.range().isLeftOf(range) && !node.arcanaContains(" implicit ");
    };

    const ClangdAstNode &m_root;
    const Range &m_range;
    ClangdAstPath m_path;
    ClangdAstPath m_longestSubPath;
    bool m_done = false;
};

ClangdAstPath getAstPath(const ClangdAstNode &root, const Range &range)
{
    return AstPathCollector(root, range).collectPath();
}

ClangdAstPath getAstPath(const ClangdAstNode &root, const Position &pos)
{
    return getAstPath(root, Range(pos, pos));
}

MessageId requestAst(Client *client, const FilePath &filePath, const Range range,
                     const AstHandler &handler)
{
    class AstParams : public JsonObject
    {
    public:
        AstParams(const TextDocumentIdentifier &document, const Range &range = {})
        {
            setTextDocument(document);
            if (range.isValid())
                setRange(range);
        }

        using JsonObject::JsonObject;

        // The open file to inspect.
        TextDocumentIdentifier textDocument() const
        { return typedValue<TextDocumentIdentifier>(textDocumentKey); }
        void setTextDocument(const TextDocumentIdentifier &id) { insert(textDocumentKey, id); }

        // The region of the source code whose AST is fetched. The highest-level node that entirely
        // contains the range is returned.
        std::optional<Range> range() const { return optionalValue<Range>(rangeKey); }
        void setRange(const Range &range) { insert(rangeKey, range); }

        bool isValid() const override { return contains(textDocumentKey); }
    };

    class AstRequest : public Request<ClangdAstNode, std::nullptr_t, AstParams>
    {
    public:
        using Request::Request;
        explicit AstRequest(const AstParams &params) : Request("textDocument/ast", params) {}
    };

    AstRequest request(AstParams(TextDocumentIdentifier(client->hostPathToServerUri(filePath)),
                                 range));
    request.setResponseCallback([handler, reqId = request.id()](AstRequest::Response response) {
        const auto result = response.result();
        handler(result ? *result : ClangdAstNode(), reqId);
    });
    client->sendMessage(request, Client::SendDocUpdates::Ignore);
    return request.id();
}

} // namespace ClangCodeModel::Internal
