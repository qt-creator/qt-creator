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

#include "qmljsast_p.h"
#include "qmljsengine_p.h"
#include "qmlsymbol.h"

using namespace Qml;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlSymbol::~QmlSymbol()
{
}

bool QmlSymbol::isBuildInSymbol()
{ return asBuildInSymbol() != 0; }

bool QmlSymbol::isSymbolFromFile()
{ return asSymbolFromFile() != 0; }

bool QmlSymbol::isIdSymbol()
{ return asIdSymbol() != 0; }

bool QmlSymbol::isPropertyDefinitionSymbol()
{ return asPropertyDefinitionSymbol() != 0; }

QmlBuildInSymbol *QmlSymbol::asBuildInSymbol()
{ return 0; }

QmlSymbolFromFile *QmlSymbol::asSymbolFromFile()
{ return 0; }

QmlIdSymbol *QmlSymbol::asIdSymbol()
{ return 0; }

QmlPropertyDefinitionSymbol *QmlSymbol::asPropertyDefinitionSymbol()
{ return 0; }

QmlBuildInSymbol::~QmlBuildInSymbol()
{}

QmlBuildInSymbol *QmlBuildInSymbol::asBuildInSymbol()
{ return this; }

QmlSymbolWithMembers::~QmlSymbolWithMembers()
{ qDeleteAll(_members); }

const QmlSymbol::List QmlSymbolWithMembers::members()
{ return _members; }

QmlSymbolFromFile::QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node):
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

QmlSymbolFromFile::~QmlSymbolFromFile()
{}

QmlSymbolFromFile *QmlSymbolFromFile::asSymbolFromFile()
{ return this; }

int QmlSymbolFromFile::line() const
{ return _node->firstSourceLocation().startLine; }

int QmlSymbolFromFile::column() const
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

const QString QmlSymbolFromFile::name() const
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

const QmlSymbol::List QmlSymbolFromFile::members()
{
    if (!todo.isEmpty()) {
        foreach (Node *todoNode, todo) {
            if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(todoNode))
                _members.append(new QmlSymbolFromFile(fileName(), objectBinding));
            else if (UiObjectDefinition *objectDefinition = cast<UiObjectDefinition*>(todoNode))
                _members.append(new QmlSymbolFromFile(fileName(), objectDefinition));
            else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(todoNode))
                _members.append(new QmlSymbolFromFile(fileName(), arrayBinding));
            else if (UiPublicMember *publicMember = cast<UiPublicMember*>(todoNode))
                _members.append(new QmlPropertyDefinitionSymbol(fileName(), publicMember));
            else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(todoNode)) {
                if (scriptBinding->qualifiedId && scriptBinding->qualifiedId->name && scriptBinding->qualifiedId->name->asString() == QLatin1String("id") && !scriptBinding->qualifiedId->next)
                    _members.append(new QmlIdSymbol(fileName(), scriptBinding, this));
                else
                    _members.append(new QmlSymbolFromFile(fileName(), scriptBinding));
            }
        }

        todo.clear();
    }

    return _members;
}

bool QmlSymbolFromFile::isProperty() const
{ return cast<UiObjectDefinition*>(_node) == 0; }

QmlSymbolFromFile *QmlSymbolFromFile::findMember(QmlJS::AST::Node *memberNode)
{
    List symbols = members();

    foreach (QmlSymbol *symbol, symbols)
        if (symbol->isSymbolFromFile())
            if (memberNode == symbol->asSymbolFromFile()->node())
                return symbol->asSymbolFromFile();

    return 0;
}

QmlIdSymbol::QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, QmlSymbolFromFile *parentNode):
        QmlSymbolFromFile(fileName, idNode),
        _parentNode(parentNode)
{}

QmlIdSymbol::~QmlIdSymbol()
{}

QmlIdSymbol *QmlIdSymbol::asIdSymbol()
{ return this; }

int QmlIdSymbol::line() const
{ return idNode()->statement->firstSourceLocation().startLine; }

int QmlIdSymbol::column() const
{ return idNode()->statement->firstSourceLocation().startColumn; }

const QString QmlIdSymbol::id() const
{
    if (ExpressionStatement *e = cast<ExpressionStatement*>(idNode()->statement))
        if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
            if (i->name)
                return i->name->asString();

    return QString();
}

QmlJS::AST::UiScriptBinding *QmlIdSymbol::idNode() const
{ return cast<UiScriptBinding*>(node()); }

QmlPropertyDefinitionSymbol::QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode):
        QmlSymbolFromFile(fileName, propertyNode)
{}

QmlPropertyDefinitionSymbol::~QmlPropertyDefinitionSymbol()
{}

QmlPropertyDefinitionSymbol *QmlPropertyDefinitionSymbol::asPropertyDefinitionSymbol()
{ return this; }

int QmlPropertyDefinitionSymbol::line() const
{ return propertyNode()->identifierToken.startLine; }

int QmlPropertyDefinitionSymbol::column() const
{ return propertyNode()->identifierToken.startColumn; }

QmlJS::AST::UiPublicMember *QmlPropertyDefinitionSymbol::propertyNode() const
{ return cast<UiPublicMember*>(node()); }

const QString QmlPropertyDefinitionSymbol::name() const
{ return propertyNode()->name->asString(); }
