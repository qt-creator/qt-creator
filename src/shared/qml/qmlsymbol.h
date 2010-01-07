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

#include <qml/parser/qmljsastfwd_p.h>
#include <qml/qml_global.h>

#include <QList>
#include <QString>

namespace Qml {

class QML_EXPORT QmlSymbol
{
public:
    typedef QList<QmlSymbol *> List;

public:
    virtual ~QmlSymbol() = 0;

    virtual const QString name() const = 0;
    virtual const List members() = 0;
    virtual bool isProperty() const = 0;

    bool isBuildInSymbol();
    bool isSymbolFromFile();
    bool isIdSymbol();
    bool isPropertyDefinitionSymbol();

    virtual class QmlBuildInSymbol *asBuildInSymbol();
    virtual class QmlSymbolFromFile *asSymbolFromFile();
    virtual class QmlIdSymbol *asIdSymbol();
    virtual class QmlPropertyDefinitionSymbol *asPropertyDefinitionSymbol();
};

class QML_EXPORT QmlBuildInSymbol: public QmlSymbol
{
public:
    virtual ~QmlBuildInSymbol() = 0;

    virtual QmlBuildInSymbol *asBuildInSymbol();

    virtual QmlBuildInSymbol *type() const = 0;
    virtual List members(bool includeBaseClassMembers) = 0;
};

class QML_EXPORT QmlSymbolWithMembers: public QmlSymbol
{
public:
    virtual ~QmlSymbolWithMembers() = 0;

    virtual const List members();

protected:
    List _members;
};

class QML_EXPORT QmlSymbolFromFile: public QmlSymbolWithMembers
{
public:
    QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node);
    virtual ~QmlSymbolFromFile();

    virtual QmlSymbolFromFile *asSymbolFromFile();

    QString fileName() const
    { return _fileName; }

    virtual int line() const;
    virtual int column() const;

    QmlJS::AST::UiObjectMember *node() const
    { return _node; }

    virtual const QString name() const;
    virtual const List members();
    virtual bool isProperty() const;
    virtual QmlSymbolFromFile *findMember(QmlJS::AST::Node *memberNode);

private:
    void fillTodo(QmlJS::AST::UiObjectMemberList *members);

private:
    QString _fileName;
    QmlJS::AST::UiObjectMember *_node;
    QList<QmlJS::AST::Node*> todo;
};

class QML_EXPORT QmlIdSymbol: public QmlSymbolFromFile
{
public:
    QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, QmlSymbolFromFile *parentNode);
    virtual ~QmlIdSymbol();

    QmlIdSymbol *asIdSymbol();

    virtual int line() const;
    virtual int column() const;

    QmlSymbolFromFile *parentNode() const
    { return _parentNode; }

    virtual const QString name() const
    { return "id"; }

    virtual const QString id() const;

private:
    QmlJS::AST::UiScriptBinding *idNode() const;

private:
    QmlSymbolFromFile *_parentNode;
};

class QML_EXPORT QmlPropertyDefinitionSymbol: public QmlSymbolFromFile
{
public:
    QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode);
    virtual ~QmlPropertyDefinitionSymbol();

    QmlPropertyDefinitionSymbol *asPropertyDefinitionSymbol();

    virtual int line() const;
    virtual int column() const;

    virtual const QString name() const;

private:
    QmlJS::AST::UiPublicMember *propertyNode() const;
};

} // namespace Qml

#endif // QMLSYMBOL_H
