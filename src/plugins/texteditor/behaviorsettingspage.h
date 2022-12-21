// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "tabsettingswidget.h"

#include <coreplugin/dialogs/ioptionspage.h>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace TextEditor {

class TabSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class CodeStylePool;

class BehaviorSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    BehaviorSettingsPage();
    ~BehaviorSettingsPage() override;

    // IOptionsPage
    QWidget *widget() override;
    void apply() override;
    void finish() override;

    ICodeStylePreferences *codeStyle() const;
    CodeStylePool *codeStylePool() const;
    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

private:
    void openCodingStylePreferences(TextEditor::TabSettingsWidget::CodingStyleLink link);

    void settingsFromUI(TypingSettings *typingSettings,
                        StorageSettings *storageSettings,
                        BehaviorSettings *behaviorSettings,
                        ExtraEncodingSettings *extraEncodingSettings) const;
    void settingsToUI();

    QList<QTextCodec *> m_codecs;
    struct BehaviorSettingsPagePrivate;
    BehaviorSettingsPagePrivate *d;
};

} // namespace TextEditor
