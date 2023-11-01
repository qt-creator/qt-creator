// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/store.h>

namespace CPlusPlus { class Overview; }
namespace TextEditor { class TabSettings; }
namespace ProjectExplorer { class Project; }

namespace CppEditor {

class CPPEDITOR_EXPORT CppCodeStyleSettings
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

    // TODO only kept to allow conversion to the new setting getterNameTemplate in
    // CppEditor/QuickFixSetting. Remove in 4.16
    bool preferGetterNameWithoutGetPrefix = true;

#ifdef WITH_TESTS
    bool forceFormatting = false;
#endif

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const CppCodeStyleSettings &rhs) const;
    bool operator==(const CppCodeStyleSettings &s) const { return equals(s); }
    bool operator!=(const CppCodeStyleSettings &s) const { return !equals(s); }

    static CppCodeStyleSettings getProjectCodeStyle(ProjectExplorer::Project *project);
    static CppCodeStyleSettings currentProjectCodeStyle();
    static CppCodeStyleSettings currentGlobalCodeStyle();
    static TextEditor::TabSettings getProjectTabSettings(ProjectExplorer::Project *project);
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

} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::CppCodeStyleSettings)
