/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    explicit BehaviorSettingsWidget(QWidget *parent = 0);
    virtual ~BehaviorSettingsWidget();

    void setActive(bool active);

    void setAssignedCodec(QTextCodec *codec);
    QTextCodec *assignedCodec() const;

    void setCodeStyle(ICodeStylePreferences *preferences);

    void setAssignedTypingSettings(const TypingSettings &typingSettings);
    void assignedTypingSettings(TypingSettings *typingSettings) const;

    void setAssignedStorageSettings(const StorageSettings &storageSettings);
    void assignedStorageSettings(StorageSettings *storageSettings) const;

    void setAssignedBehaviorSettings(const BehaviorSettings &behaviorSettings);
    void assignedBehaviorSettings(BehaviorSettings *behaviorSettings) const;

    void setAssignedExtraEncodingSettings(const ExtraEncodingSettings &encodingSettings);
    void assignedExtraEncodingSettings(ExtraEncodingSettings *encodingSettings) const;

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
    void slotEncodingBoxChanged(int index);
    void updateConstrainTooltipsBoxTooltip() const;

    BehaviorSettingsWidgetPrivate *d;
};

} // TextEditor
