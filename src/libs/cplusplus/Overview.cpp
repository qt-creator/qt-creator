/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/

#include "Overview.h"
#include "NamePrettyPrinter.h"
#include "TypePrettyPrinter.h"
#include <FullySpecifiedType.h>

using namespace CPlusPlus;

Overview::Overview()
    : _markArgument(0),
      _showArgumentNames(false),
      _showReturnTypes(false),
      _showFunctionSignatures(true)
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

void Overview::setMarkArgument(unsigned position)
{
    _markArgument = position;
}

bool Overview::showFunctionSignatures() const
{
    return _showFunctionSignatures;
}

void Overview::setShowFunctionSignatures(bool showFunctionSignatures)
{
    _showFunctionSignatures = showFunctionSignatures;
}

QString Overview::prettyName(Name *name) const
{
    NamePrettyPrinter pp(this);
    return pp(name);
}

QString Overview::prettyType(const FullySpecifiedType &ty, Name *name) const
{
    return prettyType(ty, prettyName(name));
}

QString Overview::prettyType(const FullySpecifiedType &ty,
                             const QString &name) const
{
    TypePrettyPrinter pp(this);
    pp.setMarkArgument(_markArgument);
    pp.setShowArgumentNames(_showArgumentNames);
    pp.setShowReturnTypes(_showReturnTypes);
    pp.setShowFunctionSignatures(_showFunctionSignatures);
    return pp(ty, name);
}
