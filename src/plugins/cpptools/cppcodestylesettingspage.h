/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPCODESTYLESETTINGSPAGE_H
#define CPPCODESTYLESETTINGSPAGE_H

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

private slots:
    void decorateEditors(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);
    void slotCodeStyleSettingsChanged();
    void updatePreview();
    void setTabSettings(const TextEditor::TabSettings &settings);
    void setCodeStyleSettings(const CppTools::CppCodeStyleSettings &settings, bool preview = true);
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *, bool preview = true);

private:
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

#endif // CPPCODESTYLESETTINGSPAGE_H
