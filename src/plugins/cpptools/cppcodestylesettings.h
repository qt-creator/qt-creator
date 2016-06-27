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

#include "cpptools_global.h"

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CPlusPlus { class Overview; }

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

    bool preferGetterNameWithoutGetPrefix;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const CppCodeStyleSettings &rhs) const;
    bool operator==(const CppCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const CppCodeStyleSettings &s) const { return !equals(s); }

    static CppCodeStyleSettings currentProjectCodeStyle();
    static CppCodeStyleSettings currentGlobalCodeStyle();

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
