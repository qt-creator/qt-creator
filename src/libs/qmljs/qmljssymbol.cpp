/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljssymbol.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlJS;
using namespace QmlJS;
using namespace QmlJS::AST;

Symbol::~Symbol()
{
}

bool Symbol::isBuildInSymbol()
{ return asPrimitiveSymbol() != 0; }

bool Symbol::isSymbolFromFile()
{ return asSymbolFromFile() != 0; }

bool Symbol::isIdSymbol()
{ return asIdSymbol() != 0; }

bool Symbol::isPropertyDefinitionSymbol()
{ return asPropertyDefinitionSymbol() != 0; }

PrimitiveSymbol *Symbol::asPrimitiveSymbol()
{ return 0; }

SymbolFromFile *Symbol::asSymbolFromFile()
{ return 0; }

IdSymbol *Symbol::asIdSymbol()
{ return 0; }

PropertyDefinitionSymbol *Symbol::asPropertyDefinitionSymbol()
{ return 0; }

PrimitiveSymbol::~PrimitiveSymbol()
{}

PrimitiveSymbol *PrimitiveSymbol::asPrimitiveSymbol()
{ return this; }

SymbolWithMembers::~SymbolWithMembers()
{ qDeleteAll(_members); }

const Symbol::List SymbolWithMembers::members()
{ return _members; }

SymbolFromFile::SymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node):
        _fileName(fileName),
        _node(node)
{
    if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(_node)) {
        if (objectBinding->initializer)
            for (UiObjectMemberList *iter = objectBinding->initializer->members; iter; iter = iter->next)
                if (iter->member)
                    todo.append(iter->member);
    } else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(_node)) {
        if (objectDefinition->initializer)
            for (UiObjectMemberList *iter = objectDefinition->initializer->members; iter; iter = iter->next)
                if (iter->member)
                    todo.append(iter->member);
    } else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(_node)) {
        for (UiArrayMemberList *iter = arrayBinding->members; iter; iter = iter->next)
            if (iter->member)
                todo.append(iter->member);
    }
}

SymbolFromFile::~SymbolFromFile()
{}

SymbolFromFile *SymbolFromFile::asSymbolFromFile()
{ return this; }

int SymbolFromFile::line() const
{ return _node->firstSourceLocation().startLine; }

int SymbolFromFile::column() const
{ return _node->firstSourceLocation().startColumn; }

static inline QString toString(UiQualifiedId *qId)
{
    QString result;

    for (UiQualifiedId *iter = qId; iter; iter = iter->next) {
        if (!iter->name)
            continue;

        result += iter->name->asString();

        if (iter->next)
            result += '.';
    }

    return result;
}

const QString SymbolFromFile::name() const
{
    if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(_node))
        return toString(objectBinding->qualifiedId);
    else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(_node))
        return toString(scriptBinding->qualifiedId);
    else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(_node))
        return toString(arrayBinding->qualifiedId);
    else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(_node))
        return toString(objectDefinition->qualifiedTypeNameId);
    else
        return QString::null;
}

const Symbol::List SymbolFromFile::members()
{
    if (!todo.isEmpty()) {
        foreach (Node *todoNode, todo) {
            if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(todoNode))
                _members.append(new SymbolFromFile(fileName(), objectBinding));
            else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(todoNode))
                _members.append(new SymbolFromFile(fileName(), objectDefinition));
            else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(todoNode))
                _members.append(new SymbolFromFile(fileName(), arrayBinding));
            else if (UiPublicMember *publicMember = cast<UiPublicMember*>(todoNode))
                _members.append(new PropertyDefinitionSymbol(fileName(), publicMember));
            else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(todoNode)) {
                if (scriptBinding->qualifiedId && scriptBinding->qualifiedId->name && scriptBinding->qualifiedId->name->asString() == QLatin1String("id") && !scriptBinding->qualifiedId->next)
                    _members.append(new IdSymbol(fileName(), scriptBinding, this));
                else
                    _members.append(new SymbolFromFile(fileName(), scriptBinding));
            }
        }

        todo.clear();
    }

    return _members;
}

bool SymbolFromFile::isProperty() const
{ return cast<UiObjectDefinition*>(_node) == 0; }

SymbolFromFile *SymbolFromFile::findMember(QmlJS::AST::Node *memberNode)
{
    List symbols = members();

    foreach (Symbol *symbol, symbols)
        if (symbol->isSymbolFromFile())
            if (memberNode == symbol->asSymbolFromFile()->node())
                return symbol->asSymbolFromFile();

    return 0;
}

IdSymbol::IdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, SymbolFromFile *parentNode):
        SymbolFromFile(fileName, idNode),
        _parentNode(parentNode)
{}

IdSymbol::~IdSymbol()
{}

IdSymbol *IdSymbol::asIdSymbol()
{ return this; }

int IdSymbol::line() const
{ return idNode()->statement->firstSourceLocation().startLine; }

int IdSymbol::column() const
{ return idNode()->statement->firstSourceLocation().startColumn; }

const QString IdSymbol::id() const
{
    if (ExpressionStatement *e = cast<ExpressionStatement*>(idNode()->statement))
        if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
            if (i->name)
                return i->name->asString();

    return QString();
}

QmlJS::AST::UiScriptBinding *IdSymbol::idNode() const
{ return cast<UiScriptBinding*>(node()); }

PropertyDefinitionSymbol::PropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode):
        SymbolFromFile(fileName, propertyNode)
{}

PropertyDefinitionSymbol::~PropertyDefinitionSymbol()
{}

PropertyDefinitionSymbol *PropertyDefinitionSymbol::asPropertyDefinitionSymbol()
{ return this; }

int PropertyDefinitionSymbol::line() const
{ return propertyNode()->identifierToken.startLine; }

int PropertyDefinitionSymbol::column() const
{ return propertyNode()->identifierToken.startColumn; }

QmlJS::AST::UiPublicMember *PropertyDefinitionSymbol::propertyNode() const
{ return cast<UiPublicMember*>(node()); }

const QString PropertyDefinitionSymbol::name() const
{ return propertyNode()->name->asString(); }
