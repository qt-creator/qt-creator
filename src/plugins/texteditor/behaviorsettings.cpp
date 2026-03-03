// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettings.h"

#include "texteditorsettings.h"
#include "texteditortr.h"

#include <utils/hostosinfo.h>
#include <utils/qtcsettings.h>

using namespace Utils;

namespace TextEditor {

BehaviorSettings::BehaviorSettings(const Key &keyPrefix)
{
    setSettingsGroup("textBehaviorSettings");

    mouseHiding.setSettingsKey(keyPrefix + "MouseHiding");
    mouseHiding.setDefaultValue(true);
    mouseHiding.setVisible(!HostOsInfo::isMacHost());
    mouseHiding.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseHiding.setLabelText(Tr::tr("Hide mouse cursor while typing"));

    mouseNavigation.setSettingsKey(keyPrefix + "MouseNavigation");
    mouseNavigation.setDefaultValue(true);
    mouseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseNavigation.setLabelText(Tr::tr("Enable &mouse navigation"));

    scrollWheelZooming.setSettingsKey(keyPrefix + "ScrollWheelZooming");
    scrollWheelZooming.setDefaultValue(true);
    scrollWheelZooming.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    scrollWheelZooming.setLabelText(Tr::tr("Enable scroll &wheel zooming"));

    constrainHoverTooltips.setSettingsKey(keyPrefix + "ConstrainTooltips");
    constrainHoverTooltips.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    constrainHoverTooltips.setUseDataAsSavedValue();
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));
    constrainHoverTooltips.addOption({
        Tr::tr("On Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on mouseover."),
        QVariant(false)
    });
    constrainHoverTooltips.addOption({
        Tr::tr("On Shift+Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on Shift+Mouseover."),
        QVariant(true)
    });
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));

    camelCaseNavigation.setSettingsKey(keyPrefix + "CamelCaseNavigation");
    camelCaseNavigation.setDefaultValue(true);
    camelCaseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    camelCaseNavigation.setLabelText(Tr::tr("Enable built-in camel case &navigation"));

    keyboardTooltips.setSettingsKey(keyPrefix + "KeyboardTooltips");
    keyboardTooltips.setDefaultValue(false);
    keyboardTooltips.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    keyboardTooltips.setLabelText(Tr::tr("Show help tooltips using keyboard shortcut (Alt)"));
    keyboardTooltips.setToolTip(Tr::tr("Pressing Alt displays context-sensitive help or type information as tooltips."));

    smartSelectionChanging.setSettingsKey(keyPrefix + "SmartSelectionChanging");
    smartSelectionChanging.setDefaultValue(true);
    smartSelectionChanging.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    smartSelectionChanging.setLabelText(Tr::tr("Enable smart selection changing"));
    smartSelectionChanging.setToolTip(Tr::tr("Using Select Block Up / Down actions will now provide smarter selections."));
}

void BehaviorSettings::setData(const BehaviorSettingsData &data)
{
    mouseHiding.setValue(data.m_mouseHiding);
    mouseNavigation.setValue(data.m_mouseNavigation);
    scrollWheelZooming.setValue(data.m_scrollWheelZooming);
    constrainHoverTooltips.setValue(data.m_constrainHoverTooltips);
    camelCaseNavigation.setValue(data.m_camelCaseNavigation);
    keyboardTooltips.setValue(data.m_keyboardTooltips);
    smartSelectionChanging.setValue(data.m_smartSelectionChanging);
}

BehaviorSettingsData BehaviorSettings::data() const
{
    BehaviorSettingsData d;
    d.m_mouseHiding = mouseHiding();
    d.m_mouseNavigation = mouseNavigation();
    d.m_scrollWheelZooming = scrollWheelZooming();
    d.m_constrainHoverTooltips = constrainHoverTooltips();
    d.m_camelCaseNavigation = camelCaseNavigation();
    d.m_keyboardTooltips = keyboardTooltips();
    d.m_smartSelectionChanging = smartSelectionChanging();
    return d;
}

bool BehaviorSettingsData::equals(const BehaviorSettingsData &ds) const
{
    return m_mouseHiding == ds.m_mouseHiding
        && m_mouseNavigation == ds.m_mouseNavigation
        && m_scrollWheelZooming == ds.m_scrollWheelZooming
        && m_constrainHoverTooltips == ds.m_constrainHoverTooltips
        && m_camelCaseNavigation == ds.m_camelCaseNavigation
        && m_keyboardTooltips == ds.m_keyboardTooltips
        && m_smartSelectionChanging == ds.m_smartSelectionChanging
        ;
}

BehaviorSettings &globalBehaviorSettings()
{
    static BehaviorSettings theGlobalBehaviorSettings;
    return theGlobalBehaviorSettings;
}

void updateGlobalBehaviorSettings(const BehaviorSettingsData &newBehaviorSettings)
{
    if (newBehaviorSettings.equals(globalBehaviorSettings().data()))
        return;

    globalBehaviorSettings().setData(newBehaviorSettings);
    globalBehaviorSettings().writeSettings();

    emit TextEditorSettings::instance()->behaviorSettingsChanged(newBehaviorSettings);
}

void setupBehaviorSettings()
{
    globalBehaviorSettings().readSettings();
}

} // namespace TextEditor

