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

#ifndef CPPCODESTYLESETTINGS_H
#define CPPCODESTYLESETTINGS_H

#include "cpptools_global.h"

#include <cplusplus/Overview.h>

#include <QMetaType>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStyleSettings
{
public:
    CppCodeStyleSettings();

    bool indentBlockBraces;
    bool indentBlockBody;
    bool indentClassBraces;
    bool indentEnumBraces;
    bool indentNamespaceBraces;
    bool indentNamespaceBody;
    bool indentAccessSpecifiers;
    bool indentDeclarationsRelativeToAccessSpecifiers;
    bool indentFunctionBody;
    bool indentFunctionBraces;
    bool indentSwitchLabels;
    bool indentStatementsRelativeToSwitchLabels;
    bool indentBlocksRelativeToSwitchLabels;
    bool indentControlFlowRelativeToSwitchLabels;

    // Formatting of pointer and reference declarations, see Overview::StarBindFlag.
    bool bindStarToIdentifier;
    bool bindStarToTypeName;
    bool bindStarToLeftSpecifier;
    bool bindStarToRightSpecifier;

    // false: if (a &&
    //            b)
    //            c;
    // true:  if (a &&
    //                b)
    //            c;
    // but always: while (a &&
    //                    b)
    //                 foo;
    bool extraPaddingForConditionsIfConfusingAlign;

    // false: a = a +
    //                b;
    // true:  a = a +
    //            b
    bool alignAssignments;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const CppCodeStyleSettings &rhs) const;
    bool operator==(const CppCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const CppCodeStyleSettings &s) const { return !equals(s); }

    /*! Returns an Overview configured by the current project's code style.

        If no current project is available or an error occurs when getting the
        current project's code style, the current global code style settings
        are applied.
        */
    static CPlusPlus::Overview currentProjectCodeStyleOverview();

    /*! Returns an Overview configured by the current global code style.

        If there occurred an error getting the current global code style, a
        default constructed Overview is returned.
        */
    static CPlusPlus::Overview currentGlobalCodeStyleOverview();
};

} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::CppCodeStyleSettings)

#endif // CPPCODESTYLESETTINGS_H
