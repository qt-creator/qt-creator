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

#include "cpptools_global.h"
#include "cppcodestylesettings.h"
#include "cppcodeformatter.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QWidget>
#include <QPointer>

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class ICodeStylePreferences;
    class SnippetEditorWidget;
    class CodeStyleEditor;
}

namespace CppTools {

class CppCodeStylePreferences;

namespace Internal {

namespace Ui { class CppCodeStyleSettingsPage; }

class CppCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferencesWidget(QWidget *parent = 0);
    virtual ~CppCodeStylePreferencesWidget();

    void setCodeStyle(CppTools::CppCodeStylePreferences *codeStylePreferences);

private:
    void decorateEditors(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);
    void slotCodeStyleSettingsChanged();
    void updatePreview();
    void setTabSettings(const TextEditor::TabSettings &settings);
    void setCodeStyleSettings(const CppTools::CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *, bool preview = true);

    CppCodeStyleSettings cppCodeStyleSettings() const;

    CppCodeStylePreferences *m_preferences;
    Ui::CppCodeStyleSettingsPage *m_ui;
    QList<TextEditor::SnippetEditorWidget *> m_previews;
    bool m_blockUpdates;
};


class CppCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit CppCodeStyleSettingsPage(QWidget *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    CppCodeStylePreferences *m_pageCppCodeStylePreferences;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace CppTools
