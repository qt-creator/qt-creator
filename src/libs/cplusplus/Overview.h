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

#ifndef CPLUSPLUS_OVERVIEW_H
#define CPLUSPLUS_OVERVIEW_H

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <QList>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Overview
{
public:
    Overview();

    QString operator()(const Name *name) const
    { return prettyName(name); }

    QString operator()(const QList<const Name *> &fullyQualifiedName) const
    { return prettyName(fullyQualifiedName); }

    QString operator()(const FullySpecifiedType &type, const Name *name = 0) const
    { return prettyType(type, name); }

    QString prettyName(const Name *name) const;
    QString prettyName(const QList<const Name *> &fullyQualifiedName) const;
    QString prettyType(const FullySpecifiedType &type, const Name *name = 0) const;
    QString prettyType(const FullySpecifiedType &type, const QString &name) const;

public:

    enum StarBindFlag {
        BindToIdentifier = 0x1,
        BindToTypeName = 0x2,
        BindToLeftSpecifier = 0x4,
        BindToRightSpecifier = 0x8
    };
    Q_DECLARE_FLAGS(StarBindFlags, StarBindFlag)

    StarBindFlags starBindFlags;
    bool showArgumentNames: 1;
    bool showReturnTypes: 1;
    bool showFunctionSignatures: 1;
    bool showDefaultArguments: 1;
    bool showTemplateParameters: 1;
    bool includeWhiteSpaceInOperatorName: 1; /// "operator =()" vs "operator=()"

    unsigned markedArgument;
    int markedArgumentBegin;
    int markedArgumentEnd;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_OVERVIEW_H
