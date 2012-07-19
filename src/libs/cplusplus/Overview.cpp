/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "Overview.h"
#include "NamePrettyPrinter.h"
#include "TypePrettyPrinter.h"

#include <Control.h>
#include <CoreTypes.h>
#include <FullySpecifiedType.h>

using namespace CPlusPlus;

Overview::Overview()
    : _markedArgument(0),
      _markedArgumentBegin(0),
      _markedArgumentEnd(0),
      _showArgumentNames(false),
      _showReturnTypes(false),
      _showFunctionSignatures(true),
      _showDefaultArguments(true),
      _showTemplateParameters(false)
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

bool Overview::showDefaultArguments() const
{
    return _showDefaultArguments;
}

void Overview::setShowDefaultArguments(bool showDefaultArguments)
{
    _showDefaultArguments = showDefaultArguments;
}

bool Overview::showTemplateParameters() const
{
    return _showTemplateParameters;
}

void Overview::setShowTemplateParameters(bool showTemplateParameters)
{
    _showTemplateParameters = showTemplateParameters;
}

QString Overview::prettyName(const Name *name) const
{
    NamePrettyPrinter pp(this);
    return pp(name);
}

QString Overview::prettyName(const QList<const Name *> &fullyQualifiedName) const
{
    QString result;
    const int size = fullyQualifiedName.size();
    for (int i = 0; i < size; ++i) {
        result.append(prettyName(fullyQualifiedName.at(i)));
        if (i < size - 1)
            result.append(QLatin1String("::"));
    }
    return result;
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

