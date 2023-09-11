// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettings.h"

#include <coreplugin/icore.h>

static const char mouseHidingKey[] = "MouseHiding";
static const char mouseNavigationKey[] = "MouseNavigation";
static const char scrollWheelZoomingKey[] = "ScrollWheelZooming";
static const char constrainTooltips[] = "ConstrainTooltips";
static const char camelCaseNavigationKey[] = "CamelCaseNavigation";
static const char keyboardTooltips[] = "KeyboardTooltips";
static const char smartSelectionChanging[] = "SmartSelectionChanging";

using namespace Utils;

namespace TextEditor {

BehaviorSettings::BehaviorSettings() :
    m_mouseHiding(true),
    m_mouseNavigation(true),
    m_scrollWheelZooming(true),
    m_constrainHoverTooltips(false),
    m_camelCaseNavigation(true),
    m_keyboardTooltips(false),
    m_smartSelectionChanging(true)
{
}

Store BehaviorSettings::toMap() const
{
    return {
        {mouseHidingKey, m_mouseHiding},
        {mouseNavigationKey, m_mouseNavigation},
        {scrollWheelZoomingKey, m_scrollWheelZooming},
        {constrainTooltips, m_constrainHoverTooltips},
        {camelCaseNavigationKey, m_camelCaseNavigation},
        {keyboardTooltips, m_keyboardTooltips},
        {smartSelectionChanging, m_smartSelectionChanging}
    };
}

void BehaviorSettings::fromMap(const Store &map)
{
    m_mouseHiding = map.value(mouseHidingKey, m_mouseHiding).toBool();
    m_mouseNavigation = map.value(mouseNavigationKey, m_mouseNavigation).toBool();
    m_scrollWheelZooming = map.value(scrollWheelZoomingKey, m_scrollWheelZooming).toBool();
    m_constrainHoverTooltips = map.value(constrainTooltips, m_constrainHoverTooltips).toBool();
    m_camelCaseNavigation = map.value(camelCaseNavigationKey, m_camelCaseNavigation).toBool();
    m_keyboardTooltips = map.value(keyboardTooltips, m_keyboardTooltips).toBool();
    m_smartSelectionChanging = map.value(smartSelectionChanging, m_smartSelectionChanging).toBool();
}

bool BehaviorSettings::equals(const BehaviorSettings &ds) const
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

} // namespace TextEditor

