/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_DUMPERS_H
#define CPLUSPLUS_DUMPERS_H

#include <CPlusPlusForwardDeclarations.h>
#include <FullySpecifiedType.h>
#include <Symbol.h>
#include <LookupItem.h>

#include <QString>

namespace CPlusPlus {

QString CPLUSPLUS_EXPORT toString(const Name *name, QString id = QLatin1String("Name"));
QString CPLUSPLUS_EXPORT toString(FullySpecifiedType ty, QString id = QLatin1String("Type"));
QString CPLUSPLUS_EXPORT toString(const Symbol *s, QString id = QLatin1String("Symbol"));
QString CPLUSPLUS_EXPORT toString(LookupItem it, QString id = QLatin1String("LookupItem"));
QString CPLUSPLUS_EXPORT toString(const ClassOrNamespace *binding, QString id = QLatin1String("ClassOrNamespace"));

void CPLUSPLUS_EXPORT dump(const Name *name);
void CPLUSPLUS_EXPORT dump(FullySpecifiedType ty);
void CPLUSPLUS_EXPORT dump(const Symbol *s);
void CPLUSPLUS_EXPORT dump(LookupItem it);
void CPLUSPLUS_EXPORT dump(const ClassOrNamespace *binding);

} // namespace CPlusPlus

#endif
