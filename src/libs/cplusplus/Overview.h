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

#ifndef CPLUSPLUS_OVERVIEW_H
#define CPLUSPLUS_OVERVIEW_H

#include <CPlusPlusForwardDeclarations.h>

#include <QList>
#include <QString>

namespace CPlusPlus {

/*!
   \class Overview

   \brief Converts a FullySpecifiedType and/or any qualified name,
          to its string representation.

   The public data members (except the ones starting with "marked")
   determine what exactly and how to print.
 */

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
    /*!
        \enum Overview::StarBindFlag

        The StarBindFlags describe how the '*' and '&' in pointers/references
        should be bound in the string representation.

        This also applies to rvalue references ('&&'), but not to
        pointers to functions or arrays like in

          void (*p)()
          void (*p)[]

        since it seems to be quite uncommon to use spaces there.

        See the examples below. These assume that exactly one
        flag is set. That is, it may look different with
        flag combinations.

          \value BindToIdentifier
                 e.g. "char *foo", but not "char * foo"
          \value BindToTypeName
                 e.g. "char*", but not "char *"
          \value BindToLeftSpecifier
                 e.g. "char * const* const", but not "char * const * const"
          \value BindToRightSpecifier
                 e.g. "char *const", but not "char * const"
     */
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

    /*!
        You can get the start and end position of a function argument
        in the resulting string. Set "markedArgument" to the desired
        argument. After processing, "markedArgumentBegin" and
        "markedArgumentEnd" will contain the positions.
     */
    unsigned markedArgument;
    int markedArgumentBegin;
    int markedArgumentEnd;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_OVERVIEW_H
