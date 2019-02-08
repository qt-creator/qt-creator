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

#include <utils/optional.h>

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace CPlusPlus { class Overview; }
namespace TextEditor { class TabSettings; }

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStyleSettings
{
public:
    CppCodeStyleSettings();

    bool indentBlockBraces = false;
    bool indentBlockBody = true;
    bool indentClassBraces = false;
    bool indentEnumBraces = false;
    bool indentNamespaceBraces = false;
    bool indentNamespaceBody = false;
    bool indentAccessSpecifiers = false;
    bool indentDeclarationsRelativeToAccessSpecifiers = true;
    bool indentFunctionBody = true;
    bool indentFunctionBraces = false;
    bool indentSwitchLabels = false;
    bool indentStatementsRelativeToSwitchLabels = true;
    bool indentBlocksRelativeToSwitchLabels = false;
    bool indentControlFlowRelativeToSwitchLabels = true;

    // Formatting of pointer and reference declarations, see Overview::StarBindFlag.
    bool bindStarToIdentifier = true;
    bool bindStarToTypeName = false;
    bool bindStarToLeftSpecifier = false;
    bool bindStarToRightSpecifier = false;

    // false: if (a &&
    //            b)
    //            c;
    // true:  if (a &&
    //                b)
    //            c;
    // but always: while (a &&
    //                    b)
    //                 foo;
    bool extraPaddingForConditionsIfConfusingAlign = true;

    // false: a = a +
    //                b;
    // true:  a = a +
    //            b
    bool alignAssignments = false;

    bool preferGetterNameWithoutGetPrefix = true;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const CppCodeStyleSettings &rhs) const;
    bool operator==(const CppCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const CppCodeStyleSettings &s) const { return !equals(s); }

    static Utils::optional<CppCodeStyleSettings> currentProjectCodeStyle();
    static CppCodeStyleSettings currentGlobalCodeStyle();
    static TextEditor::TabSettings currentProjectTabSettings();
    static TextEditor::TabSettings currentGlobalTabSettings();

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
