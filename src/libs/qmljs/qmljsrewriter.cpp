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

#include "qmljsrewriter.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsutils.h>
#include <utils/changeset.h>

// ### FIXME: remove these includes:
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace Utils;

Rewriter::Rewriter(const QString &originalText,
                   ChangeSet *changeSet,
                   const QStringList &propertyOrder)
    : m_originalText(originalText)
    , m_changeSet(changeSet)
    , m_propertyOrder(propertyOrder)
{
    Q_ASSERT(changeSet);
}

Rewriter::Range Rewriter::addBinding(AST::UiObjectInitializer *ast,
                                     const QString &propertyName,
                                     const QString &propertyValue,
                                     BindingType bindingType)
{
    UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members,
                                                                propertyName,
                                                                m_propertyOrder);
    return addBinding(ast, propertyName, propertyValue, bindingType, insertAfter);
}

Rewriter::Range Rewriter::addBinding(AST::UiObjectInitializer *ast,
                                     const QString &propertyName,
                                     const QString &propertyValue,
                                     BindingType bindingType,
                                     UiObjectMemberList *insertAfter)
{
    SourceLocation endOfPreviousMember;
    SourceLocation startOfNextMember;

    if (insertAfter == 0 || insertAfter->member == 0) {
        // insert as first member
        endOfPreviousMember = ast->lbraceToken;

        if (ast->members && ast->members->member)
            startOfNextMember = ast->members->member->firstSourceLocation();
        else
            startOfNextMember = ast->rbraceToken;
    } else {
        endOfPreviousMember = insertAfter->member->lastSourceLocation();

        if (insertAfter->next && insertAfter->next->member)
            startOfNextMember = insertAfter->next->member->firstSourceLocation();
        else
            startOfNextMember = ast->rbraceToken;
    }
    const bool isOneLiner = endOfPreviousMember.startLine == startOfNextMember.startLine;
    bool needsPreceedingSemicolon = false;
    bool needsTrailingSemicolon = false;

    if (isOneLiner) {
        if (insertAfter == 0) { // we're inserting after an lbrace
            if (ast->members) { // we're inserting before a member (and not the rbrace)
                needsTrailingSemicolon = bindingType == ScriptBinding;
            }
        } else { // we're inserting after a member, not after the lbrace
            if (endOfPreviousMember.isValid()) { // there already is a semicolon after the previous member
                if (insertAfter->next && insertAfter->next->member) { // and the after us there is a member, not an rbrace, so:
                    needsTrailingSemicolon = bindingType == ScriptBinding;
                }
            } else { // there is no semicolon after the previous member (probably because there is an rbrace after us/it, so:
                needsPreceedingSemicolon = true;
            }
        }
    }

    QString newPropertyTemplate;
    switch (bindingType) {
    case ArrayBinding:
        newPropertyTemplate = QLatin1String("%1: [\n%2\n]");
        break;

    case ObjectBinding:
        newPropertyTemplate = QLatin1String("%1: %2");
        break;

    case ScriptBinding:
        newPropertyTemplate = QLatin1String("%1: %2");
        break;

    default:
        Q_ASSERT(!"unknown property type");
    }

    if (isOneLiner) {
        if (needsPreceedingSemicolon)
            newPropertyTemplate.prepend(QLatin1Char(';'));
        newPropertyTemplate.prepend(QLatin1Char(' '));
        if (needsTrailingSemicolon)
            newPropertyTemplate.append(QLatin1Char(';'));
    } else {
        newPropertyTemplate.prepend(QLatin1Char('\n'));
    }

    m_changeSet->insert(endOfPreviousMember.end(),
                         newPropertyTemplate.arg(propertyName, propertyValue));

    return Range(endOfPreviousMember.end(), endOfPreviousMember.end());
}

UiObjectMemberList *Rewriter::searchMemberToInsertAfter(UiObjectMemberList *members,
                                                        const QStringList &propertyOrder)
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

UiArrayMemberList *Rewriter::searchMemberToInsertAfter(UiArrayMemberList *members,
                                                        const QStringList &propertyOrder)
{
    const int objectDefinitionInsertionPoint = propertyOrder.indexOf(QString());

    UiArrayMemberList *lastObjectDef = 0;
    UiArrayMemberList *lastNonObjectDef = 0;

    for (UiArrayMemberList *iter = members; iter; iter = iter->next) {
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

UiObjectMemberList *Rewriter::searchMemberToInsertAfter(UiObjectMemberList *members,
                                                        const QString &propertyName,
                                                        const QStringList &propertyOrder)
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

void Rewriter::changeBinding(UiObjectInitializer *ast,
                             const QString &propertyName,
                             const QString &newValue,
                             BindingType binding)
{
    QString prefix, suffix;
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx != -1) {
        prefix = propertyName.left(dotIdx);
        suffix = propertyName.mid(dotIdx + 1);
    }

    for (UiObjectMemberList *members = ast->members; members; members = members->next) {
        UiObjectMember *member = members->member;

        // for non-grouped properties:
        if (isMatchingPropertyMember(propertyName, member)) {
            switch (binding) {
            case ArrayBinding:
                insertIntoArray(cast<UiArrayBinding*>(member), newValue);
                break;

            case ObjectBinding:
                replaceMemberValue(member, newValue, false);
                break;

            case ScriptBinding:
                replaceMemberValue(member, newValue, nextMemberOnSameLine(members));
                break;

            default:
                Q_ASSERT(!"Unhandled QmlRefactoring::PropertyType");
            }

            break;
        }
        // for grouped properties:
        else if (!prefix.isEmpty()) {
            if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member)) {
                if (toString(def->qualifiedTypeNameId) == prefix)
                    changeBinding(def->initializer, suffix, newValue, binding);
            }
        }
    }
}

void Rewriter::replaceMemberValue(UiObjectMember *propertyMember,
                                  const QString &newValue,
                                  bool needsSemicolon)
{
    QString replacement = newValue;
    int startOffset = -1;
    int endOffset = -1;
    if (UiObjectBinding *objectBinding = AST::cast<UiObjectBinding *>(propertyMember)) {
        startOffset = objectBinding->qualifiedTypeNameId->identifierToken.offset;
        endOffset = objectBinding->initializer->rbraceToken.end();
    } else if (UiScriptBinding *scriptBinding = AST::cast<UiScriptBinding *>(propertyMember)) {
        startOffset = scriptBinding->statement->firstSourceLocation().offset;
        endOffset = scriptBinding->statement->lastSourceLocation().end();
    } else if (UiArrayBinding *arrayBinding = AST::cast<UiArrayBinding *>(propertyMember)) {
        startOffset = arrayBinding->lbracketToken.offset;
        endOffset = arrayBinding->rbracketToken.end();
    } else if (UiPublicMember *publicMember = AST::cast<UiPublicMember*>(propertyMember)) {
        if (publicMember->statement) {
            startOffset = publicMember->statement->firstSourceLocation().offset;
            if (publicMember->semicolonToken.isValid())
                endOffset = publicMember->semicolonToken.end();
            else
                endOffset = publicMember->statement->lastSourceLocation().offset;
        } else {
            startOffset = publicMember->lastSourceLocation().end();
            endOffset = startOffset;
            if (publicMember->semicolonToken.isValid())
                startOffset = publicMember->semicolonToken.offset;
            replacement.prepend(QLatin1String(": "));
        }
    } else {
        return;
    }

    if (needsSemicolon)
        replacement += QLatin1Char(';');

    m_changeSet->replace(startOffset, endOffset, replacement);
}

bool Rewriter::isMatchingPropertyMember(const QString &propertyName,
                                        UiObjectMember *member)
{
    if (UiPublicMember *publicMember = cast<UiPublicMember*>(member))
        return publicMember->name == propertyName;
    else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member))
        return toString(objectBinding->qualifiedId) == propertyName;
    else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member))
        return toString(scriptBinding->qualifiedId) == propertyName;
    else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member))
        return toString(arrayBinding->qualifiedId) == propertyName;
    else
        return false;
}

bool Rewriter::nextMemberOnSameLine(UiObjectMemberList *members)
{
    if (members && members->next && members->next->member)
        return members->next->member->firstSourceLocation().startLine == members->member->lastSourceLocation().startLine;
    else
        return false;
}

void Rewriter::insertIntoArray(UiArrayBinding *ast, const QString &newValue)
{
    if (!ast)
        return;

    UiObjectMember *lastMember = 0;
    for (UiArrayMemberList *iter = ast->members; iter; iter = iter->next) {
        lastMember = iter->member;
    }

    if (!lastMember)
        return;

    const int insertionPoint = lastMember->lastSourceLocation().end();
    m_changeSet->insert(insertionPoint, QLatin1String(",\n") + newValue);
}

void Rewriter::removeBindingByName(UiObjectInitializer *ast, const QString &propertyName)
{
    QString prefix;
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx != -1)
        prefix = propertyName.left(dotIdx);

    for (UiObjectMemberList *it = ast->members; it; it = it->next) {
        UiObjectMember *member = it->member;

        // run full name match (for ungrouped properties):
        if (isMatchingPropertyMember(propertyName, member))
            removeMember(member);
        // check for grouped properties:
        else if (!prefix.isEmpty()) {
            if (UiObjectDefinition *def = cast<UiObjectDefinition *>(member)) {
                if (toString(def->qualifiedTypeNameId) == prefix)
                    removeGroupedProperty(def, propertyName);
            }
        }
    }
}

void Rewriter::removeGroupedProperty(UiObjectDefinition *ast,
                                     const QString &propertyName)
{
    int dotIdx = propertyName.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;

    const QString propName = propertyName.mid(dotIdx + 1);

    UiObjectMember *wanted = 0;
    unsigned memberCount = 0;
    for (UiObjectMemberList *it = ast->initializer->members; it; it = it->next) {
        ++memberCount;
        UiObjectMember *member = it->member;

        if (!wanted && isMatchingPropertyMember(propName, member))
            wanted = member;
    }

    if (!wanted)
        return;
    if (memberCount == 1)
        removeMember(ast);
    else
        removeMember(wanted);
}

void Rewriter::removeMember(UiObjectMember *member)
{
    int start = member->firstSourceLocation().offset;
    int end = member->lastSourceLocation().end();

    includeSurroundingWhitespace(m_originalText, start, end);

    m_changeSet->remove(start, end);
}

bool Rewriter::includeSurroundingWhitespace(const QString &source, int &start, int &end)
{
    bool includeStartingWhitespace = true;
    bool paragraphFound = false;
    bool paragraphSkipped = false;

    if (end >= 0) {
        QChar c = source.at(end);

        while (c.isSpace()) {
            ++end;
            if (c.unicode() == 10) {
                paragraphFound = true;
                paragraphSkipped = true;
                break;
            } else if (end == source.length()) {
                break;
            }

            c = source.at(end);
        }

        includeStartingWhitespace = paragraphFound;
    }

    paragraphFound = false;
    if (includeStartingWhitespace) {
        while (start > 0) {
            const QChar c = source.at(start - 1);

            if (c.unicode() == 10) {
                paragraphFound = true;
                break;
            }
            if (!c.isSpace())
                break;

            --start;
        }
    }
    if (!paragraphFound && paragraphSkipped) //keep the line ending
        --end;

    return paragraphFound;
}

void Rewriter::includeLeadingEmptyLine(const QString &source, int &start)
{
    QTextDocument doc(source);

    if (start == 0)
        return;

    if (doc.characterAt(start - 1) != QChar::ParagraphSeparator)
        return;

    QTextCursor tc(&doc);
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

void Rewriter::includeEmptyGroupedProperty(UiObjectDefinition *groupedProperty, UiObjectMember *memberToBeRemoved, int &start, int &end)
{
    if (groupedProperty->qualifiedTypeNameId && !groupedProperty->qualifiedTypeNameId->name.isEmpty()
            && groupedProperty->qualifiedTypeNameId->name.at(0).isLower()) {
        // grouped property
        UiObjectMemberList *memberIter = groupedProperty->initializer->members;
        while (memberIter) {
            if (memberIter->member != memberToBeRemoved)
                return;
            memberIter = memberIter->next;
        }
        start = groupedProperty->firstSourceLocation().begin();
        end = groupedProperty->lastSourceLocation().end();
    }
}

#if 0
UiObjectMemberList *QMLRewriter::searchMemberToInsertAfter(UiObjectMemberList *members, const QStringList &propertyOrder)
{
    const int objectDefinitionInsertionPoint = propertyOrder.indexOf(QString::null);

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

#endif

void Rewriter::appendToArrayBinding(UiArrayBinding *arrayBinding,
                                    const QString &content)
{
    UiObjectMember *lastMember = 0;
    for (UiArrayMemberList *iter = arrayBinding->members; iter; iter = iter->next)
        if (iter->member)
            lastMember = iter->member;

    if (!lastMember)
        return; // an array binding cannot be empty, so there will (or should) always be a last member.

    const int insertionPoint = lastMember->lastSourceLocation().end();

    m_changeSet->insert(insertionPoint, QLatin1String(",\n") + content);
}

Rewriter::Range Rewriter::addObject(UiObjectInitializer *ast, const QString &content)
{
    UiObjectMemberList *insertAfter = searchMemberToInsertAfter(ast->members, m_propertyOrder);
    return addObject(ast, content, insertAfter);
}

Rewriter::Range Rewriter::addObject(UiObjectInitializer *ast, const QString &content, UiObjectMemberList *insertAfter)
{
    int insertionPoint;
    QString textToInsert;
    if (insertAfter && insertAfter->member) {
        insertionPoint = insertAfter->member->lastSourceLocation().end();
        textToInsert += QLatin1String("\n");
    } else {
        insertionPoint = ast->lbraceToken.end();
    }

    textToInsert += content;
    m_changeSet->insert(insertionPoint, QLatin1String("\n") + textToInsert);

    return Range(insertionPoint, insertionPoint);
}

Rewriter::Range Rewriter::addObject(UiArrayBinding *ast, const QString &content)
{
    UiArrayMemberList *insertAfter = searchMemberToInsertAfter(ast->members, m_propertyOrder);
    return addObject(ast, content, insertAfter);
}

Rewriter::Range Rewriter::addObject(UiArrayBinding *ast, const QString &content, UiArrayMemberList *insertAfter)
{
    int insertionPoint;
    QString textToInsert;
    if (insertAfter && insertAfter->member) {
        insertionPoint = insertAfter->member->lastSourceLocation().end();
        textToInsert = QLatin1String(",\n") + content;
    } else {
        insertionPoint = ast->lbracketToken.end();
        textToInsert += QLatin1String("\n") + content + QLatin1Char(',');
    }

    m_changeSet->insert(insertionPoint, textToInsert);

    return Range(insertionPoint, insertionPoint);
}

void Rewriter::removeObjectMember(UiObjectMember *member, UiObjectMember *parent)
{
    int start = member->firstSourceLocation().offset;
    int end = member->lastSourceLocation().end();

    if (UiArrayBinding *parentArray = cast<UiArrayBinding *>(parent)) {
        extendToLeadingOrTrailingComma(parentArray, member, start, end);
    } else {
        if (UiObjectDefinition *parentObjectDefinition = cast<UiObjectDefinition *>(parent))
            includeEmptyGroupedProperty(parentObjectDefinition, member, start, end);
        includeSurroundingWhitespace(m_originalText, start, end);
    }

    includeLeadingEmptyLine(m_originalText, start);
    m_changeSet->remove(start, end);
}

void Rewriter::extendToLeadingOrTrailingComma(UiArrayBinding *parentArray,
                                              UiObjectMember *member,
                                              int &start,
                                              int &end) const
{
    UiArrayMemberList *currentMember = 0;
    for (UiArrayMemberList *it = parentArray->members; it; it = it->next) {
        if (it->member == member) {
            currentMember = it;
            break;
        }
    }

    if (!currentMember)
        return;

    if (currentMember->commaToken.isValid()) {
        // leading comma
        start = currentMember->commaToken.offset;
        if (includeSurroundingWhitespace(m_originalText, start, end))
            --end;
    } else if (currentMember->next && currentMember->next->commaToken.isValid()) {
        // trailing comma
        end = currentMember->next->commaToken.end();
        includeSurroundingWhitespace(m_originalText, start, end);
    } else {
        // array with 1 element, so remove the complete binding
        start = parentArray->firstSourceLocation().offset;
        end = parentArray->lastSourceLocation().end();
        includeSurroundingWhitespace(m_originalText, start, end);
    }
}
