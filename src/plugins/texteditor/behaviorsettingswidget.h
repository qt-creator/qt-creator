// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

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
    explicit BehaviorSettingsWidget(QWidget *parent = nullptr);
    ~BehaviorSettingsWidget() override;

    void setActive(bool active);

    void setAssignedCodec(QTextCodec *codec);
    QByteArray assignedCodecName() const;

    void setCodeStyle(ICodeStylePreferences *preferences);

    void setAssignedTypingSettings(const TypingSettings &typingSettings);
    void assignedTypingSettings(TypingSettings *typingSettings) const;

    void setAssignedStorageSettings(const StorageSettings &storageSettings);
    void assignedStorageSettings(StorageSettings *storageSettings) const;

    void setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings);
    void assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const;

    void setAssignedExtraEncodingSettings(const ExtraEncodingSettings &encodingSettings);
    void assignedExtraEncodingSettings(ExtraEncodingSettings *encodingSettings) const;

    void setAssignedLineEnding(int lineEnding);
    int assignedLineEnding() const;

    TabSettingsWidget *tabSettingsWidget() const;

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &settings);
    void storageSettingsChanged(const TextEditor::StorageSettings &settings);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &settings);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &settings);
    void textCodecChanged(QTextCodec *codec);

private:
    void slotTypingSettingsChanged();
    void slotStorageSettingsChanged();
    void slotBehaviorSettingsChanged();
    void slotExtraEncodingChanged();
    void updateConstrainTooltipsBoxTooltip() const;

    BehaviorSettingsWidgetPrivate *d;
};

} // TextEditor
