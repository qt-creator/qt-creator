// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlrewriter.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QTextBlock>

#include <typeinfo>

static Q_LOGGING_CATEGORY(qmlRewriter, "qtc.rewriter.qmlrewriter", QtWarningMsg)

namespace QmlDesigner::Internal {

using NanotraceHR::keyValue;
using RewriterTracing::category;

QMLRewriter::QMLRewriter(QmlDesigner::TextModifier &textModifier):
        m_textModifier(&textModifier),
        m_didRewriting(false)
{
    NanotraceHR::Tracer tracer{"QML rewriter constructor", category()};
}

bool QMLRewriter::operator()(QmlJS::AST::UiProgram *ast)
{
    NanotraceHR::Tracer tracer{"QML rewriter operator()", category()};

    setDidRewriting(false);

    QmlJS::AST::Node::accept(ast, this);

    return didRewriting();
}

void QMLRewriter::replace(int offset, int length, const QString &text)
{
    NanotraceHR::Tracer tracer{"QML rewriter replace",
                               category(),
                               keyValue("offset", offset),
                               keyValue("length", length),
                               keyValue("text", text)};

    m_textModifier->replace(offset, length, text);
}

void QMLRewriter::move(const QmlDesigner::TextModifier::MoveInfo &moveInfo)
{
    NanotraceHR::Tracer tracer{"QML rewriter move", category(), keyValue("move info", moveInfo)};

    m_textModifier->move(moveInfo);
}

QString QMLRewriter::textBetween(int startPosition, int endPosition) const
{
    NanotraceHR::Tracer tracer{"QML rewriter text between",
                               category(),
                               keyValue("start position", startPosition),
                               keyValue("end position", endPosition)};

    return m_textModifier->text().mid(startPosition, endPosition - startPosition);
}

QString QMLRewriter::textAt(const QmlJS::SourceLocation &location) const
{
    NanotraceHR::Tracer tracer{"QML rewriter text at", category()};

    return m_textModifier->text().mid(location.offset, location.length);
}

int QMLRewriter::indentDepth() const
{
    NanotraceHR::Tracer tracer{"QML rewriter indent depth", category()};

    return textModifier()->tabSettings().m_indentSize;
}

unsigned QMLRewriter::calculateIndentDepth(const QmlJS::SourceLocation &position) const
{
    NanotraceHR::Tracer tracer{"QML rewriter calculate indent depth", category()};

    QTextDocument *doc = m_textModifier->textDocument();
    QTextCursor tc(doc);
    tc.setPosition(position.offset);

    return textModifier()->tabSettings().indentationColumn(tc.block().text());
}

QString QMLRewriter::addIndentation(const QString &text, unsigned depth)
{
    NanotraceHR::Tracer tracer{"QML rewriter add indentation",
                               category(),
                               keyValue("text", text),
                               keyValue("depth", depth)};

    if (depth == 0)
        return text;

    TextEditor::TabSettings tabSettings = textModifier()->tabSettings();
    QString result;
    bool addLineSep = false;
    constexpr char lineSep('\n');
    const QStringList lines = text.split(lineSep);
    for (const QString &line : lines) {
        if (addLineSep)
            result += lineSep;

        addLineSep = true;
        if (line.isEmpty())
            continue;

        const int firstNoneSpace = TextEditor::TabSettings::firstNonSpace(line);
        const int lineIndentColumn = tabSettings.indentationColumn(line) + int(depth);
        result.append(tabSettings.indentationString(0, lineIndentColumn, 0));
        result.append(line.mid(firstNoneSpace));
    }
    return result;
}

QmlJS::SourceLocation QMLRewriter::calculateLocation(QmlJS::AST::UiQualifiedId *id)
{
    NanotraceHR::Tracer tracer{"QML rewriter calculate location", category()};

    Q_ASSERT(id != nullptr);

    const QmlJS::SourceLocation startLocation = id->identifierToken;

    QmlJS::AST::UiQualifiedId *nextId = id;
    while (nextId->next) {
        nextId = nextId->next;
    }

    const QmlJS::SourceLocation endLocation = nextId->identifierToken;

    return QmlJS::SourceLocation(startLocation.offset, endLocation.end() - startLocation.offset);
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::UiObjectMember *member)
{
    NanotraceHR::Tracer tracer{"QML rewriter is missing semicolon", category()};

    auto binding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(member);
    if (binding)
        return isMissingSemicolon(binding->statement);
    else
        return false;
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::Statement *stmt)
{
    NanotraceHR::Tracer tracer{"QML rewriter is missing semicolon", category()};

    if (auto eStmt = QmlJS::AST::cast<QmlJS::AST::ExpressionStatement *>(stmt)) {
        return !eStmt->semicolonToken.isValid();
    } else if (auto iStmt = QmlJS::AST::cast<QmlJS::AST::IfStatement *>(stmt)) {
        if (iStmt->elseToken.isValid())
            return isMissingSemicolon(iStmt->ko);
        else
            return isMissingSemicolon(iStmt->ok);
    } else if (auto dStmt = QmlJS::AST::cast<QmlJS::AST::DebuggerStatement *>(stmt)) {
        return !dStmt->semicolonToken.isValid();
    } else {
        return false;
    }
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool QMLRewriter::includeSurroundingWhitespace(int &start, int &end) const
{
    NanotraceHR::Tracer tracer{"QML rewriter include surrounding whitespace",
                               category(),
                               keyValue("start", start),
                               keyValue("end", end)};

    QTextDocument *doc = m_textModifier->textDocument();
    bool includeStartingWhitespace = true;
    bool paragraphFound = false;

    if (end >= 0) {
        QChar c = doc->characterAt(end);
        while (c.isSpace()) {
            ++end;

            if (c == QChar::ParagraphSeparator) {
                paragraphFound = true;
                break;
            } else if (end == doc->characterCount()) {
                break;
            }

            c = doc->characterAt(end);
        }

        includeStartingWhitespace = paragraphFound;
    }

    if (includeStartingWhitespace) {
        while (start > 0) {
            const QChar c = doc->characterAt(start - 1);

            if (!c.isSpace())
                break;
            else if (c == QChar::ParagraphSeparator)
                break;

            --start;
        }
    }

    return paragraphFound;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void QMLRewriter::includeLeadingEmptyLine(int &start) const
{
    NanotraceHR::Tracer tracer{"QML rewriter include leading empty line",
                               category(),
                               keyValue("start", start)};

    QTextDocument *doc = textModifier()->textDocument();

    if (start == 0)
        return;

    if (doc->characterAt(start - 1) != QChar::ParagraphSeparator)
        return;

    QTextCursor tc(doc);
    tc.setPosition(start);
    const int blockNr = tc.blockNumber();
    if (blockNr == 0)
        return;

    const QTextBlock prevBlock = tc.block().previous();
    const QString trimmedPrevBlockText = prevBlock.text().trimmed();
    if (!trimmedPrevBlockText.isEmpty())
        return;

    start = prevBlock.position();
}

namespace {
int indexOf(Utils::span<const QmlDesigner::PropertyNameView> properties,
            QmlDesigner::PropertyNameView name)
{
    auto found = std::ranges::find(properties, name);
    if (found == properties.end())
        return -1;

    return static_cast<int>(std::distance(properties.begin(), found));
}
} // namespace

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
QmlJS::AST::UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(
    QmlJS::AST::UiObjectMemberList *members, Utils::span<const PropertyNameView> propertyOrder)
{
    NanotraceHR::Tracer tracer{"QML rewriter search member to insert after", category()};

    const int objectDefinitionInsertionPoint = indexOf(propertyOrder, ""); // XXX ????

    QmlJS::AST::UiObjectMemberList *lastObjectDef = nullptr;
    QmlJS::AST::UiObjectMemberList *lastNonObjectDef = nullptr;

    for (QmlJS::AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        QmlJS::AST::UiObjectMember *member = iter->member;
        int idx = -1;

        if (QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(member))
            lastObjectDef = iter;
        else if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(member))
            idx = indexOf(propertyOrder, toString(arrayBinding->qualifiedId).toUtf8());
        else if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(member))
            idx = indexOf(propertyOrder, toString(objectBinding->qualifiedId).toUtf8());
        else if (auto scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding*>(member))
            idx = indexOf(propertyOrder, toString(scriptBinding->qualifiedId).toUtf8());
        else if (QmlJS::AST::cast<QmlJS::AST::UiPublicMember*>(member))
            idx = indexOf(propertyOrder, "property");

        if (idx < objectDefinitionInsertionPoint)
            lastNonObjectDef = iter;
    }

    if (lastObjectDef)
        return lastObjectDef;
    else
        return lastNonObjectDef;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
QmlJS::AST::UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(
    QmlJS::AST::UiObjectMemberList *members,
    PropertyNameView propertyName,
    Utils::span<const PropertyNameView> propertyOrder)
{
    NanotraceHR::Tracer tracer{"QML rewriter search member to insert after",
                               category(),
                               keyValue("property name", propertyName)};

    if (!members)
        return nullptr; // empty members

    QHash<QString, QmlJS::AST::UiObjectMemberList *> orderedMembers;

    for (QmlJS::AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        QmlJS::AST::UiObjectMember *member = iter->member;

        if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(member))
            orderedMembers[toString(arrayBinding->qualifiedId)] = iter;
        else if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(member))
            orderedMembers[toString(objectBinding->qualifiedId)] = iter;
        else if (QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(member))
            orderedMembers[QString()] = iter;
        else if (auto scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding*>(member))
            orderedMembers[toString(scriptBinding->qualifiedId)] = iter;
        else if (QmlJS::AST::cast<QmlJS::AST::UiPublicMember*>(member))
            orderedMembers[QStringLiteral("property")] = iter;
    }

    int idx = indexOf(propertyOrder, propertyName);
    if (idx == -1)
        idx = indexOf(propertyOrder, "");
    if (idx == -1)
        idx = static_cast<int>(propertyOrder.size()) - 1;

    for (; idx > 0; --idx) {
        const QString prop = QString::fromLatin1(propertyOrder[static_cast<std::size_t>(idx - 1)]);
        QmlJS::AST::UiObjectMemberList *candidate = orderedMembers.value(prop, nullptr);
        if (candidate != nullptr)
            return candidate;
    }

    return nullptr;
}

QmlJS::AST::UiObjectMemberList *QMLRewriter::searchChildrenToInsertAfter(
    QmlJS::AST::UiObjectMemberList *members, Utils::span<const PropertyNameView> propertyOrder, int pos)
{
    NanotraceHR::Tracer tracer{"QML rewriter search children to insert after",
                               category(),
                               keyValue("position", pos)};

    // An empty property name should be available in the propertyOrder List, which is the right place
    // to define the objects there.
    const int objectDefinitionInsertionPoint = indexOf(propertyOrder, "");

    QmlJS::AST::UiObjectMemberList *lastObjectDef = nullptr;
    QmlJS::AST::UiObjectMemberList *lastNonObjectDef = nullptr;
    int objectPos = 0;

    for (QmlJS::AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        QmlJS::AST::UiObjectMember *member = iter->member;
        int idx = -1;

        if (QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition *>(member)) {
            if (pos < 0)
                break;
            lastObjectDef = iter;
            if (objectPos++ == pos)
                break;
        } else if (auto arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding *>(member))
            idx = indexOf(propertyOrder, toString(arrayBinding->qualifiedId).toUtf8());
        else if (auto objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding *>(member))
            idx = indexOf(propertyOrder, toString(objectBinding->qualifiedId).toUtf8());
        else if (auto scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(member))
            idx = indexOf(propertyOrder, toString(scriptBinding->qualifiedId).toUtf8());
        else if (QmlJS::AST::cast<QmlJS::AST::UiPublicMember *>(member))
            idx = indexOf(propertyOrder, "property");

        if (idx < objectDefinitionInsertionPoint)
            lastNonObjectDef = iter;
    }

    if (lastObjectDef)
        return lastObjectDef;
    else
        return lastNonObjectDef;
}

void QMLRewriter::dump(const ASTPath &path)
{
    NanotraceHR::Tracer tracer{"QML rewriter dump", category()};

    qCDebug(qmlRewriter) << "AST path with" << path.size() << "node(s):";
    for (int i = 0; i < path.size(); ++i) {
        auto node = path.at(i);
        qCDebug(qmlRewriter).noquote() << QString(i + 1, QLatin1Char('-')) << typeid(*node).name();
    }
}

void QMLRewriter::throwRecursionDepthError()
{
    NanotraceHR::Tracer tracer{"QML rewriter throw recursion depth error", category()};

    qCWarning(qmlRewriter)
        << "Warning: Hit maximum recursion level while visiting AST in QMLRewriter";
}

} // namespace QmlDesigner::Internal
