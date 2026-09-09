// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>
#include <utils/store.h>

#include <QMetaType>
#include <QObject>

namespace CMakeProjectManager::Internal {

// What the indenter and the built-in formatter of the CMake editor do beyond
// what the tab settings of the style say.
class CMakeCodeStyleSettings
{
public:
    // A space between the name of a command and the parenthesis that opens
    // its arguments: if (WIN32) rather than if(WIN32).
    bool spaceBeforeControlParen = false;
    bool spaceBeforeCommandParen = false;

    // Whether the values below a line that holds nothing but a keyword of the
    // command take a level of their own.
    bool indentKeywordValues = true;

    // Whether the whitespace before a comment at the end of a line is left as
    // it is, so that comments lined up in a column stay lined up.
    bool keepCommentColumn = true;

    void toMap(Utils::Store &map) const;
    void fromMap(const Utils::Store &map);

    friend bool operator==(const CMakeCodeStyleSettings &, const CMakeCodeStyleSettings &)
        = default;

    static Utils::Id settingsId();
};

void setupCMakeCodeStyle();

#ifdef WITH_TESTS
QObject *createCMakeCodeStyleTest();
#endif

} // CMakeProjectManager::Internal

Q_DECLARE_METATYPE(CMakeProjectManager::Internal::CMakeCodeStyleSettings)
