/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlrewriter.h"

#include <qmljs/parser/qmljsast_p.h>

#include <QDebug>
#include <QTextBlock>

#include <typeinfo>

using namespace QmlDesigner::Internal;

QMLRewriter::QMLRewriter(QmlDesigner::TextModifier &textModifier):
        m_textModifier(&textModifier),
        m_didRewriting(false)
{
}

bool QMLRewriter::operator()(QmlJS::AST::UiProgram *ast)
{
    setDidRewriting(false);

    QmlJS::AST::Node::accept(ast, this);

    return didRewriting();
}

void QMLRewriter::replace(int offset, int length, const QString &text)
{
    m_textModifier->replace(offset, length, text);
}

void QMLRewriter::move(const QmlDesigner::TextModifier::MoveInfo &moveInfo)
{
    m_textModifier->move(moveInfo);
}

QString QMLRewriter::textBetween(int startPosition, int endPosition) const
{
    return m_textModifier->text().mid(startPosition, endPosition - startPosition);
}

QString QMLRewriter::textAt(const QmlJS::AST::SourceLocation &location) const
{
    return m_textModifier->text().mid(location.offset, location.length);
}

unsigned QMLRewriter::calculateIndentDepth(const QmlJS::AST::SourceLocation &position) const
{
    QTextDocument *doc = m_textModifier->textDocument();
    QTextCursor tc(doc);
    tc.setPosition(position.offset);
    const int lineOffset = tc.block().position();
    unsigned indentDepth = 0;

    forever {
        const QChar ch = doc->characterAt(lineOffset + indentDepth);

        if (ch.isNull() || !ch.isSpace())
            break;
        else
            ++indentDepth;
    }

    return indentDepth;
}

QString QMLRewriter::addIndentation(const QString &text, unsigned depth)
{
    if (depth == 0)
        return text;

    const QString indentation(depth, QLatin1Char(' '));

    if (text.isEmpty())
        return indentation;

    const QLatin1Char lineSep('\n');
    const QStringList lines = text.split(lineSep);
    QString result;

    for (int i = 0; i < lines.size(); ++i) {
        if (i > 0)
            result += lineSep;
        const QString &line = lines.at(i);
        if (!line.isEmpty()) {
            result += indentation;
            result += line;
        }
    }

    return result;
}

QString QMLRewriter::removeIndentationFromLine(const QString &text, int depth)
{
    int charsToRemove = 0;
    for (int i = 0; i < depth && i < text.length(); ++i) {
        if (text.at(i).isSpace())
            charsToRemove++;
        else
            break;
    }

    if (charsToRemove == 0)
        return text;
    else
        return text.mid(charsToRemove);
}

QString QMLRewriter::removeIndentation(const QString &text, unsigned depth)
{
    const QLatin1Char lineSep('\n');
    const QStringList lines = text.split(lineSep);
    QString result;

    for (int i = 0; i < lines.size(); ++i) {
        if (i > 0)
            result += lineSep;
        result += removeIndentationFromLine(lines.at(i), depth);
    }

    return result;
}

QmlJS::AST::SourceLocation QMLRewriter::calculateLocation(QmlJS::AST::UiQualifiedId *id)
{
    Q_ASSERT(id != 0);

    const QmlJS::AST::SourceLocation startLocation = id->identifierToken;

    QmlJS::AST::UiQualifiedId *nextId = id;
    while (nextId->next) {
        nextId = nextId->next;
    }

    const QmlJS::AST::SourceLocation endLocation = nextId->identifierToken;

    return QmlJS::AST::SourceLocation(startLocation.offset, endLocation.end() - startLocation.offset);
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::UiObjectMember *member)
{
    QmlJS::AST::UiScriptBinding *binding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding *>(member);
    if (binding)
        return isMissingSemicolon(binding->statement);
    else
        return false;
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::Statement *stmt)
{
    if (QmlJS::AST::ExpressionStatement *eStmt = QmlJS::AST::cast<QmlJS::AST::ExpressionStatement *>(stmt)) {
        return !eStmt->semicolonToken.isValid();
    } else if (QmlJS::AST::IfStatement *iStmt = QmlJS::AST::cast<QmlJS::AST::IfStatement *>(stmt)) {
        if (iStmt->elseToken.isValid())
            return isMissingSemicolon(iStmt->ko);
        else
            return isMissingSemicolon(iStmt->ok);
    } else if (QmlJS::AST::DebuggerStatement *dStmt = QmlJS::AST::cast<QmlJS::AST::DebuggerStatement *>(stmt)) {
        return !dStmt->semicolonToken.isValid();
    } else {
        return false;
    }
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool QMLRewriter::includeSurroundingWhitespace(int &start, int &end) const
{
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

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
QmlJS::AST::UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members, const QmlDesigner::PropertyNameList &propertyOrder)
{
    const int objectDefinitionInsertionPoint = propertyOrder.indexOf(PropertyName()); // XXX ????

    QmlJS::AST::UiObjectMemberList *lastObjectDef = 0;
    QmlJS::AST::UiObjectMemberList *lastNonObjectDef = 0;

    for (QmlJS::AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        QmlJS::AST::UiObjectMember *member = iter->member;
        int idx = -1;

        if (QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(member))
            lastObjectDef = iter;
        else if (QmlJS::AST::UiArrayBinding *arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(member))
            idx = propertyOrder.indexOf(toString(arrayBinding->qualifiedId).toUtf8());
        else if (QmlJS::AST::UiObjectBinding *objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(member))
            idx = propertyOrder.indexOf(toString(objectBinding->qualifiedId).toUtf8());
        else if (QmlJS::AST::UiScriptBinding *scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding*>(member))
            idx = propertyOrder.indexOf(toString(scriptBinding->qualifiedId).toUtf8());
        else if (QmlJS::AST::cast<QmlJS::AST::UiPublicMember*>(member))
            idx = propertyOrder.indexOf("property");

        if (idx < objectDefinitionInsertionPoint)
            lastNonObjectDef = iter;
    }

    if (lastObjectDef)
        return lastObjectDef;
    else
        return lastNonObjectDef;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
QmlJS::AST::UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(QmlJS::AST::UiObjectMemberList *members,
                                                           const QmlDesigner::PropertyName &propertyName,
                                                           const QmlDesigner::PropertyNameList &propertyOrder)
{
    if (!members)
        return 0; // empty members

    QHash<QString, QmlJS::AST::UiObjectMemberList *> orderedMembers;

    for (QmlJS::AST::UiObjectMemberList *iter = members; iter; iter = iter->next) {
        QmlJS::AST::UiObjectMember *member = iter->member;

        if (QmlJS::AST::UiArrayBinding *arrayBinding = QmlJS::AST::cast<QmlJS::AST::UiArrayBinding*>(member))
            orderedMembers[toString(arrayBinding->qualifiedId)] = iter;
        else if (QmlJS::AST::UiObjectBinding *objectBinding = QmlJS::AST::cast<QmlJS::AST::UiObjectBinding*>(member))
            orderedMembers[toString(objectBinding->qualifiedId)] = iter;
        else if (QmlJS::AST::cast<QmlJS::AST::UiObjectDefinition*>(member))
            orderedMembers[QString::null] = iter;
        else if (QmlJS::AST::UiScriptBinding *scriptBinding = QmlJS::AST::cast<QmlJS::AST::UiScriptBinding*>(member))
            orderedMembers[toString(scriptBinding->qualifiedId)] = iter;
        else if (QmlJS::AST::cast<QmlJS::AST::UiPublicMember*>(member))
            orderedMembers[QStringLiteral("property")] = iter;
    }

    int idx = propertyOrder.indexOf(propertyName);
    if (idx == -1)
        idx = propertyOrder.indexOf(PropertyName());
    if (idx == -1)
        idx = propertyOrder.size() - 1;

    for (; idx > 0; --idx) {
        const QString prop = QString::fromLatin1(propertyOrder.at(idx - 1));
        QmlJS::AST::UiObjectMemberList *candidate = orderedMembers.value(prop, 0);
        if (candidate != 0)
            return candidate;
    }

    return 0;
}

void QMLRewriter::dump(const ASTPath &path)
{
    qDebug() << "AST path with" << path.size() << "node(s):";
    for (int i = 0; i < path.size(); ++i) {
        auto node = path.at(i);
        qDebug().noquote() << QString(i + 1, QLatin1Char('-')) << typeid(*node).name();
    }
}
