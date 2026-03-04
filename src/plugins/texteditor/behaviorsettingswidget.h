// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

namespace TextEditor {

class ICodeStylePreferences;
class SimpleCodeStylePreferencesWidget;
class TabSettingsWidget;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;

class TEXTEDITOR_EXPORT BehaviorSettingsWidget : public QWidget
{
public:
    BehaviorSettingsWidget(TypingSettings *typingSettings,
                           StorageSettings *storageSettings,
                           BehaviorSettings *behaviorSettings,
                           ExtraEncodingSettings *encodingSettings,
                           QWidget *parent);

    void setCodeStyle(ICodeStylePreferences *preferences);

    TabSettingsWidget *tabSettingsWidget() const;

private:
    SimpleCodeStylePreferencesWidget *tabPreferencesWidget;

    TypingSettings *typingSettings;
    StorageSettings *storageSettings;
    BehaviorSettings *behaviorSettings;
    ExtraEncodingSettings *encodingSettings;
};

} // TextEditor
