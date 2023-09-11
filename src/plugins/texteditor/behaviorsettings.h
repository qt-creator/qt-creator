// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>

namespace TextEditor {

/**
 * Settings that describe how the text editor behaves. This does not include
 * the TabSettings and StorageSettings.
 */
class TEXTEDITOR_EXPORT BehaviorSettings
{
public:
    BehaviorSettings();

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    bool equals(const BehaviorSettings &bs) const;

    friend bool operator==(const BehaviorSettings &t1, const BehaviorSettings &t2) { return t1.equals(t2); }
    friend bool operator!=(const BehaviorSettings &t1, const BehaviorSettings &t2) { return !t1.equals(t2); }

    bool m_mouseHiding;
    bool m_mouseNavigation;
    bool m_scrollWheelZooming;
    bool m_constrainHoverTooltips;
    bool m_camelCaseNavigation;
    bool m_keyboardTooltips;
    bool m_smartSelectionChanging;
};

} // namespace TextEditor
