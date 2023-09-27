// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicontroller.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>

using namespace Utils;

namespace ModelEditor {
namespace Internal {

// Settings entries
const char SETTINGS_RIGHT_SPLITTER[] = "ModelEditorPlugin/RightSplitter";
const char SETTINGS_RIGHT_HORIZ_SPLITTER[] = "ModelEditorPlugin/RightHorizSplitter";

bool UiController::hasRightSplitterState() const
{
    return !m_rightSplitterState.isEmpty();
}

QByteArray UiController::rightSplitterState() const
{
    return m_rightSplitterState;
}

bool UiController::hasRightHorizSplitterState() const
{
    return  !m_rightHorizSplitterState.isEmpty();
}

QByteArray UiController::rightHorizSplitterState() const
{
    return m_rightHorizSplitterState;
}

void UiController::onRightSplitterChanged(const QByteArray &state)
{
    m_rightSplitterState = state;
    emit rightSplitterChanged(state);
}

void UiController::onRightHorizSplitterChanged(const QByteArray &state)
{
    m_rightHorizSplitterState = state;
    emit rightHorizSplitterChanged(state);
}

void UiController::saveSettings()
{
    QtcSettings *settings = Core::ICore::settings();
    if (hasRightSplitterState())
        settings->setValue(SETTINGS_RIGHT_SPLITTER, m_rightSplitterState);
    if (hasRightHorizSplitterState())
        settings->setValue(SETTINGS_RIGHT_HORIZ_SPLITTER, m_rightHorizSplitterState);
}

void UiController::loadSettings()
{
    QtcSettings *settings = Core::ICore::settings();
    if (settings->contains(SETTINGS_RIGHT_SPLITTER))
        m_rightSplitterState = settings->value(SETTINGS_RIGHT_SPLITTER).toByteArray();
    if (settings->contains(SETTINGS_RIGHT_HORIZ_SPLITTER))
        m_rightHorizSplitterState = settings->value(SETTINGS_RIGHT_HORIZ_SPLITTER).toByteArray();
}

} // namespace Internal
} // namespace ModelEditor
