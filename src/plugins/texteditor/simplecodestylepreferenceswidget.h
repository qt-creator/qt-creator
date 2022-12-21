// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

namespace TextEditor {

class TabSettings;
class TabSettingsWidget;
class ICodeStylePreferences;

namespace Ui { class TabPreferencesWidget; }

class TEXTEDITOR_EXPORT SimpleCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleCodeStylePreferencesWidget(QWidget *parent = nullptr);

    void setPreferences(ICodeStylePreferences *tabPreferences);

    TabSettingsWidget *tabSettingsWidget() const;

private:
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);

    TabSettingsWidget *m_tabSettingsWidget;
    ICodeStylePreferences *m_preferences = nullptr;
};

} // namespace TextEditor
