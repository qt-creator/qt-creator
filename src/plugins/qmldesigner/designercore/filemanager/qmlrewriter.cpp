/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlrewriter.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>
#include <QTextBlock>

#include <typeinfo>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlDesigner::Internal;

QMLRewriter::QMLRewriter(QmlDesigner::TextModifier &textModifier):
        m_textModifier(&textModifier),
        m_didRewriting(false)
{
}

bool QMLRewriter::operator()(QmlJS::AST::UiProgram *ast)
{
    setDidRewriting(false);

    Node::accept(ast, this);

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

unsigned QMLRewriter::calculateIndentDepth(const SourceLocation &position) const
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

    const SourceLocation startLocation = id->identifierToken;

    UiQualifiedId *nextId = id;
    while (nextId->next) {
        nextId = nextId->next;
    }

    const SourceLocation endLocation = nextId->identifierToken;

    return SourceLocation(startLocation.offset, endLocation.end() - startLocation.offset);
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::UiObjectMember *member)
{
    UiScriptBinding *binding = AST::cast<UiScriptBinding *>(member);
    if (binding)
        return isMissingSemicolon(binding->statement);
    else
        return false;
}

bool QMLRewriter::isMissingSemicolon(QmlJS::AST::Statement *stmt)
{
    if (ExpressionStatement *eStmt = AST::cast<ExpressionStatement *>(stmt)) {
        return !eStmt->semicolonToken.isValid();
    } else if (IfStatement *iStmt = AST::cast<IfStatement *>(stmt)) {
        if (iStmt->elseToken.isValid())
            return isMissingSemicolon(iStmt->ko);
        else
            return isMissingSemicolon(iStmt->ok);
    } else if (DebuggerStatement *dStmt = AST::cast<DebuggerStatement *>(stmt)) {
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
UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(UiObjectMemberList *members, const QStringList &propertyOrder)
{
    const int objectDefinitionInsertionPoint = propertyOrder.indexOf(QString());

    UiObjectMemberList *lastObjectDef = 0;
    UiObjectMemberList *lastNonObjectDef = 0;

    for (UiObjectMemberList *iter = members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;
        int idx = -1;

        if (cast<UiObjectDefinition*>(member))
            lastObjectDef = iter;
        else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member))
            idx = propertyOrder.indexOf(toString(arrayBinding->qualifiedId));
        else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member))
            idx = propertyOrder.indexOf(toString(objectBinding->qualifiedId));
        else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member))
            idx = propertyOrder.indexOf(toString(scriptBinding->qualifiedId));
        else if (cast<UiPublicMember*>(member))
            idx = propertyOrder.indexOf(QLatin1String("property"));

        if (idx < objectDefinitionInsertionPoint)
            lastNonObjectDef = iter;
    }

    if (lastObjectDef)
        return lastObjectDef;
    else
        return lastNonObjectDef;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(UiObjectMemberList *members, const QString &propertyName, const QStringList &propertyOrder)
{
    if (!members)
        return 0; // empty members

    QHash<QString, UiObjectMemberList *> orderedMembers;

    for (UiObjectMemberList *iter = members; iter; iter = iter->next) {
        UiObjectMember *member = iter->member;

        if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member))
            orderedMembers[toString(arrayBinding->qualifiedId)] = iter;
        else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member))
            orderedMembers[toString(objectBinding->qualifiedId)] = iter;
        else if (cast<UiObjectDefinition*>(member))
            orderedMembers[QString::null] = iter;
        else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member))
            orderedMembers[toString(scriptBinding->qualifiedId)] = iter;
        else if (cast<UiPublicMember*>(member))
            orderedMembers[QLatin1String("property")] = iter;
    }

    int idx = propertyOrder.indexOf(propertyName);
    if (idx == -1)
        idx = propertyOrder.indexOf(QString());
    if (idx == -1)
        idx = propertyOrder.size() - 1;

    for (; idx > 0; --idx) {
        const QString prop = propertyOrder.at(idx - 1);
        UiObjectMemberList *candidate = orderedMembers.value(prop, 0);
        if (candidate != 0)
            return candidate;
    }

    return 0;
}

void QMLRewriter::dump(const ASTPath &path)
{
    qDebug() << "AST path with" << path.size() << "node(s):";
    for (int i = 0; i < path.size(); ++i) {
        qDebug() << qPrintable(QString(i + 1, QLatin1Char('-'))) << typeid(*path.at(i)).name();
    }
}
