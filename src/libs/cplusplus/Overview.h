/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
    bool showEnclosingTemplate: 1;
    bool includeWhiteSpaceInOperatorName: 1; /// "operator =()" vs "operator=()"

    unsigned markedArgument;
    int markedArgumentBegin;
    int markedArgumentEnd;
};

} // namespace CPlusPlus
