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

#include <coreplugin/dialogs/ioptionspage.h>
#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
    class TabSettings;
    class CodeStyleEditor;
    class ICodeStylePreferences;
}

namespace QmlJSTools {
namespace Internal {

namespace Ui { class QmlJSCodeStyleSettingsPage; }

class QmlJSCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(QWidget *parent = 0);
    ~QmlJSCodeStylePreferencesWidget();

    void setPreferences(TextEditor::ICodeStylePreferences *preferences);

private:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged();
    void updatePreview();

    TextEditor::ICodeStylePreferences *m_preferences;
    Ui::QmlJSCodeStyleSettingsPage *m_ui;
};


class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    explicit QmlJSCodeStyleSettingsPage(QWidget *parent = 0);

    QWidget *widget();
    void apply();
    void finish();

private:
    TextEditor::ICodeStylePreferences *m_pageTabPreferences;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace CppTools
