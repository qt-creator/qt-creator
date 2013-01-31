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

#include "Overview.h"
#include "NamePrettyPrinter.h"
#include "TypePrettyPrinter.h"

#include <Control.h>
#include <CoreTypes.h>
#include <FullySpecifiedType.h>

using namespace CPlusPlus;

Overview::Overview()
    : starBindFlags(BindToIdentifier), // default to "Qt Style"
      showArgumentNames(false),
      showReturnTypes(false),
      showFunctionSignatures(true),
      showDefaultArguments(true),
      showTemplateParameters(false),
      markedArgument(0),
      markedArgumentBegin(0),
      markedArgumentEnd(0)
{ }

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

