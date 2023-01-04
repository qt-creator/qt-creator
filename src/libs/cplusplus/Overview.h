// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>

#include <QList>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Overview
{
public:
    Overview();

    QString prettyName(const Name *name) const;
    QString prettyName(const QList<const Name *> &fullyQualifiedName) const;
    QString prettyType(const FullySpecifiedType &type, const Name *name = nullptr) const;
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
    bool trailingReturnType: 1;

    int markedArgument;
    int markedArgumentBegin;
    int markedArgumentEnd;
};

} // namespace CPlusPlus
