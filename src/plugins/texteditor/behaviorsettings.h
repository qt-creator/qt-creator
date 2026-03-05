// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/store.h>

namespace TextEditor {

/**
 * Settings that describe how the text editor behaves. This does not include
 * the TabSettings and StorageSettings.
 */
class TEXTEDITOR_EXPORT BehaviorSettingsData final
{
public:
    BehaviorSettingsData() = default;

    bool m_mouseHiding = true;
    bool m_mouseNavigation = true;
    bool m_scrollWheelZooming = true;
    bool m_constrainHoverTooltips = false;
    bool m_camelCaseNavigation = true;
    bool m_keyboardTooltips = false;
    bool m_smartSelectionChanging = true;
};

class TEXTEDITOR_EXPORT BehaviorSettings final : public Utils::AspectContainer
{
public:
    explicit BehaviorSettings(const Utils::Key &keyPrefix = {});

    BehaviorSettingsData data() const;
    void setData(const BehaviorSettingsData &data);

    Utils::BoolAspect mouseHiding{this};
    Utils::BoolAspect mouseNavigation{this};
    Utils::BoolAspect scrollWheelZooming{this};
    Utils::SelectionAspect constrainHoverTooltips{this};
    Utils::BoolAspect camelCaseNavigation{this};
    Utils::BoolAspect keyboardTooltips{this};
    Utils::BoolAspect smartSelectionChanging{this};
};

void setupBehaviorSettings();

TEXTEDITOR_EXPORT BehaviorSettings &globalBehaviorSettings();

} // namespace TextEditor
