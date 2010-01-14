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

#include <QString>

#include "qmljsastfwd_p.h"

namespace QmlDesigner {

class QmlSymbol
{
public:
    virtual ~QmlSymbol() = 0;

    bool isBuildInSymbol() const;
    bool isSymbolFromFile() const;
    bool isIdSymbol() const;

    virtual class QmlBuildInSymbol const *asBuildInSymbol() const;
    virtual class QmlSymbolFromFile const *asSymbolFromFile() const;
    virtual class QmlIdSymbol const *asIdSymbol() const;
};

class QmlBuildInSymbol: public QmlSymbol
{
public:
    virtual ~QmlBuildInSymbol();

    virtual QmlBuildInSymbol const *asBuildInSymbol() const;

private:
};

class QmlSymbolFromFile: public QmlSymbol
{
public:
    QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node);
    virtual ~QmlSymbolFromFile();

    virtual QmlSymbolFromFile const *asSymbolFromFile() const;

    QString fileName() const
    { return _fileName; }

    virtual int line() const;
    virtual int column() const;

    QmlJS::AST::UiObjectMember *node() const
    { return _node; }

private:
    QString _fileName;
    QmlJS::AST::UiObjectMember *_node;
};

class QmlIdSymbol: public QmlSymbolFromFile
{
public:
    QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, const QmlSymbolFromFile &parentNode);
    virtual ~QmlIdSymbol();

    QmlIdSymbol const *asIdSymbol() const;

    virtual int line() const;
    virtual int column() const;

    QmlSymbolFromFile const *parentNode() const
    { return &_parentNode; }

private:
    QmlJS::AST::UiScriptBinding *idNode() const;

private:
    QmlSymbolFromFile _parentNode;
};

class QmlPropertyDefinitionSymbol: public QmlSymbolFromFile
{
public:
    QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode);
    virtual ~QmlPropertyDefinitionSymbol();

    virtual int line() const;
    virtual int column() const;

private:
    QmlJS::AST::UiPublicMember *propertyNode() const;
};

} // namespace QmlDesigner

#endif // QMLSYMBOL_H
