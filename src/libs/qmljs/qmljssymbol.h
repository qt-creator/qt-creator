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

#ifndef QMLSYMBOL_H
#define QMLSYMBOL_H

#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljs_global.h>

#include <QList>
#include <QString>

namespace QmlJS {

class QMLJS_EXPORT Symbol
{
public:
    typedef QList<Symbol *> List;

public:
    virtual ~Symbol() = 0;

    virtual const QString name() const = 0;
    virtual const List members() = 0;
    virtual bool isProperty() const = 0;

    bool isBuildInSymbol();
    bool isSymbolFromFile();
    bool isIdSymbol();
    bool isPropertyDefinitionSymbol();

    virtual class PrimitiveSymbol *asPrimitiveSymbol();
    virtual class SymbolFromFile *asSymbolFromFile();
    virtual class IdSymbol *asIdSymbol();
    virtual class PropertyDefinitionSymbol *asPropertyDefinitionSymbol();
};

class QMLJS_EXPORT PrimitiveSymbol: public Symbol
{
public:
    virtual ~PrimitiveSymbol() = 0;

    virtual PrimitiveSymbol *asPrimitiveSymbol();

    virtual PrimitiveSymbol *type() const = 0;
    using Symbol::members;
    virtual List members(bool includeBaseClassMembers) = 0;
};

class QMLJS_EXPORT SymbolWithMembers: public Symbol
{
public:
    virtual ~SymbolWithMembers() = 0;

    virtual const List members();

protected:
    List _members;
};

class QMLJS_EXPORT SymbolFromFile: public SymbolWithMembers
{
public:
    SymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node);
    virtual ~SymbolFromFile();

    virtual SymbolFromFile *asSymbolFromFile();

    QString fileName() const
    { return _fileName; }

    virtual int line() const;
    virtual int column() const;

    QmlJS::AST::UiObjectMember *node() const
    { return _node; }

    virtual const QString name() const;
    virtual const List members();
    virtual bool isProperty() const;
    virtual SymbolFromFile *findMember(QmlJS::AST::Node *memberNode);

private:
    void fillTodo(QmlJS::AST::UiObjectMemberList *members);

private:
    QString _fileName;
    QmlJS::AST::UiObjectMember *_node;
    QList<QmlJS::AST::Node*> todo;
};

class QMLJS_EXPORT IdSymbol: public SymbolFromFile
{
public:
    IdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, SymbolFromFile *parentNode);
    virtual ~IdSymbol();

    IdSymbol *asIdSymbol();

    virtual int line() const;
    virtual int column() const;

    SymbolFromFile *parentNode() const
    { return _parentNode; }

    virtual const QString name() const
    { return "id"; }

    virtual const QString id() const;

private:
    QmlJS::AST::UiScriptBinding *idNode() const;

private:
    SymbolFromFile *_parentNode;
};

class QMLJS_EXPORT PropertyDefinitionSymbol: public SymbolFromFile
{
public:
    PropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode);
    virtual ~PropertyDefinitionSymbol();

    PropertyDefinitionSymbol *asPropertyDefinitionSymbol();

    virtual int line() const;
    virtual int column() const;

    virtual const QString name() const;

private:
    QmlJS::AST::UiPublicMember *propertyNode() const;
};

} // namespace Qml

#endif // QMLSYMBOL_H
