// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

#include <utils/textcodec.h>

namespace TextEditor {

class ICodeStylePreferences;
class TabSettingsWidget;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;

struct BehaviorSettingsWidgetPrivate;

class TEXTEDITOR_EXPORT BehaviorSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    BehaviorSettingsWidget(TypingSettings *typingSettings,
                           StorageSettings *storageSettings,
                           BehaviorSettings *behaviorSettings,
                           ExtraEncodingSettings *encodingSettings,
                           QWidget *parent);

    ~BehaviorSettingsWidget() override;

    bool isDirty() const;
    void apply();

    void setActive(bool active);

    void setCodeStyle(ICodeStylePreferences *preferences);

    TabSettingsWidget *tabSettingsWidget() const;

private:
    BehaviorSettingsWidgetPrivate *d;
};

} // TextEditor
