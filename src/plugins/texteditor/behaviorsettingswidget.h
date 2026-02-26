// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

#include <utils/textcodec.h>

namespace TextEditor {

class ICodeStylePreferences;
class TabSettingsWidget;
class TypingSettingsData;
class StorageSettingsData;
class BehaviorSettingsData;
class ExtraEncodingSettingsData;

struct BehaviorSettingsWidgetPrivate;

class TEXTEDITOR_EXPORT BehaviorSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BehaviorSettingsWidget(QWidget *parent = nullptr);
    ~BehaviorSettingsWidget() override;

    void setActive(bool active);

    void setAssignedEncoding(const Utils::TextEncoding &encoding);
    Utils::TextEncoding currentEncoding() const;

    void setCodeStyle(ICodeStylePreferences *preferences);

    void setAssignedTypingSettings(const TypingSettingsData &typingSettings);
    void assignedTypingSettings(TypingSettingsData *typingSettings) const;

    void setAssignedStorageSettings(const StorageSettingsData &storageSettings);
    void assignedStorageSettings(StorageSettingsData *storageSettings) const;

    void setAssignedBehaviorSettings(const BehaviorSettingsData &behaviorSettings);
    void assignedBehaviorSettings(BehaviorSettingsData *behaviorSettings) const;

    void setAssignedExtraEncodingSettings(const ExtraEncodingSettingsData &encodingSettings);
    void assignedExtraEncodingSettings(ExtraEncodingSettingsData *encodingSettings) const;

    void setAssignedLineEnding(int lineEnding);
    int assignedLineEnding() const;

    TabSettingsWidget *tabSettingsWidget() const;

signals:
    void typingSettingsChanged(const TextEditor::TypingSettingsData &settings);
    void storageSettingsChanged(const TextEditor::StorageSettingsData &settings);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettingsData &settings);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettingsData &settings);
    void textEncodingChanged(const Utils::TextEncoding &encoding);

private:
    void slotTypingSettingsChanged();
    void slotStorageSettingsChanged();
    void slotBehaviorSettingsChanged();
    void slotExtraEncodingChanged();
    void updateConstrainTooltipsBoxTooltip() const;

    BehaviorSettingsWidgetPrivate *d;
};

} // TextEditor
