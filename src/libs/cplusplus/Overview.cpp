/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "Overview.h"
#include "NamePrettyPrinter.h"
#include "TypePrettyPrinter.h"
#include <FullySpecifiedType.h>

using namespace CPlusPlus;

Overview::Overview()
    : _markedArgument(0),
      _markedArgumentBegin(0),
      _markedArgumentEnd(0),
      _showArgumentNames(false),
      _showReturnTypes(false),
      _showFunctionSignatures(true),
      _showFullyQualifiedNames(false),
      _showDefaultArguments(true)
{ }

Overview::~Overview()
{ }

bool Overview::showArgumentNames() const
{
    return _showArgumentNames;
}

void Overview::setShowArgumentNames(bool showArgumentNames)
{
    _showArgumentNames = showArgumentNames;
}

void Overview::setShowReturnTypes(bool showReturnTypes)
{
    _showReturnTypes = showReturnTypes;
}

bool Overview::showReturnTypes() const
{
    return _showReturnTypes;
}

unsigned Overview::markedArgument() const
{
    return _markedArgument;
}

void Overview::setMarkedArgument(unsigned position)
{
    _markedArgument = position;
}

int Overview::markedArgumentBegin() const
{
    return _markedArgumentBegin;
}

void Overview::setMarkedArgumentBegin(int begin)
{
    _markedArgumentBegin = begin;
}

int Overview::markedArgumentEnd() const
{
    return _markedArgumentEnd;
}

void Overview::setMarkedArgumentEnd(int end)
{
    _markedArgumentEnd = end;
}

bool Overview::showFunctionSignatures() const
{
    return _showFunctionSignatures;
}

void Overview::setShowFunctionSignatures(bool showFunctionSignatures)
{
    _showFunctionSignatures = showFunctionSignatures;
}

bool Overview::showFullyQualifiedNames() const
{
    return _showFullyQualifiedNames;
}

void Overview::setShowFullyQualifiedNamed(bool showFullyQualifiedNames)
{
    _showFullyQualifiedNames = showFullyQualifiedNames;
}

bool Overview::showDefaultArguments() const
{
    return _showDefaultArguments;
}

void Overview::setShowDefaultArguments(bool showDefaultArguments)
{
    _showDefaultArguments = showDefaultArguments;
}

QString Overview::prettyName(const Name *name) const
{
    NamePrettyPrinter pp(this);
    return pp(name);
}

QString Overview::prettyType(const FullySpecifiedType &ty, const Name *name) const
{
    return prettyType(ty, prettyName(name));
}

QString Overview::prettyType(const FullySpecifiedType &ty,
                             const QString &name) const
{
    TypePrettyPrinter pp(this);
    return pp(ty, name);
}
