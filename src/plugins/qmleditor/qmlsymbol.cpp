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
#include "qmlsymbol.h"

using namespace QmlEditor;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlSymbol::~QmlSymbol()
{
}

bool QmlSymbol::isBuildInSymbol() const
{ return asBuildInSymbol() != 0; }

bool QmlSymbol::isSymbolFromFile() const
{ return asSymbolFromFile() != 0; }

bool QmlSymbol::isIdSymbol() const
{ return asIdSymbol() != 0; }

QmlBuildInSymbol const *QmlSymbol::asBuildInSymbol() const
{ return 0; }

QmlSymbolFromFile const *QmlSymbol::asSymbolFromFile() const
{ return 0; }

QmlIdSymbol const *QmlSymbol::asIdSymbol() const
{ return 0; }

QmlBuildInSymbol::~QmlBuildInSymbol()
{}

QmlBuildInSymbol const* QmlBuildInSymbol::asBuildInSymbol() const
{ return this; }

QmlSymbolFromFile::QmlSymbolFromFile(const QString &fileName, QmlJS::AST::UiObjectMember *node):
        _fileName(fileName),
        _node(node)
{}

QmlSymbolFromFile::~QmlSymbolFromFile()
{}

const QmlSymbolFromFile *QmlSymbolFromFile::asSymbolFromFile() const
{ return this; }

int QmlSymbolFromFile::line() const
{ return _node->firstSourceLocation().startLine; }

int QmlSymbolFromFile::column() const
{ return _node->firstSourceLocation().startColumn; }

QmlIdSymbol::QmlIdSymbol(const QString &fileName, QmlJS::AST::UiScriptBinding *idNode, const QmlSymbolFromFile &parentNode):
        QmlSymbolFromFile(fileName, idNode),
        _parentNode(parentNode)
{}

QmlIdSymbol::~QmlIdSymbol()
{}

QmlIdSymbol const *QmlIdSymbol::asIdSymbol() const
{ return this; }

int QmlIdSymbol::line() const
{ return idNode()->statement->firstSourceLocation().startLine; }

int QmlIdSymbol::column() const
{ return idNode()->statement->firstSourceLocation().startColumn; }

QmlJS::AST::UiScriptBinding *QmlIdSymbol::idNode() const
{ return cast<UiScriptBinding*>(node()); }

QmlPropertyDefinitionSymbol::QmlPropertyDefinitionSymbol(const QString &fileName, QmlJS::AST::UiPublicMember *propertyNode):
        QmlSymbolFromFile(fileName, propertyNode)
{}

QmlPropertyDefinitionSymbol::~QmlPropertyDefinitionSymbol()
{}

int QmlPropertyDefinitionSymbol::line() const
{ return propertyNode()->identifierToken.startLine; }

int QmlPropertyDefinitionSymbol::column() const
{ return propertyNode()->identifierToken.startColumn; }

QmlJS::AST::UiPublicMember *QmlPropertyDefinitionSymbol::propertyNode() const
{ return cast<UiPublicMember*>(node()); }
