/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPLUSPLUS_DUMPERS_H
#define CPLUSPLUS_DUMPERS_H

#include "LookupItem.h"

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Symbol.h>

#include <QString>

namespace CPlusPlus {

QString CPLUSPLUS_EXPORT toString(const Name *name, QString id = QLatin1String("Name"));
QString CPLUSPLUS_EXPORT toString(FullySpecifiedType ty, QString id = QLatin1String("Type"));
QString CPLUSPLUS_EXPORT toString(const Symbol *s, QString id = QLatin1String("Symbol"));
QString CPLUSPLUS_EXPORT toString(const LookupItem &it, const QString &id = QLatin1String("LookupItem"));
QString CPLUSPLUS_EXPORT toString(const LookupScope *binding, QString id = QLatin1String("LookupScope"));

void CPLUSPLUS_EXPORT dump(const Name *name);
void CPLUSPLUS_EXPORT dump(const FullySpecifiedType &ty);
void CPLUSPLUS_EXPORT dump(const Symbol *s);
void CPLUSPLUS_EXPORT dump(const LookupItem &it);
void CPLUSPLUS_EXPORT dump(const LookupScope *binding);

} // namespace CPlusPlus

#endif
