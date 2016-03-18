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

#include "texteditoroptionspage.h"
#include "tabsettingswidget.h"

namespace TextEditor {

class TabSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class CodeStylePool;

class BehaviorSettingsPageParameters
{
public:
    Core::Id id;
    QString displayName;
    QString settingsPrefix;
};

class BehaviorSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    BehaviorSettingsPage(const BehaviorSettingsPageParameters &p, QObject *parent);
    ~BehaviorSettingsPage();

    // IOptionsPage
    QWidget *widget();
    void apply();
    void finish();

    ICodeStylePreferences *codeStyle() const;
    CodeStylePool *codeStylePool() const;
    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

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
