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

namespace TextEditor {

class TabSettings;
class TabSettingsWidget;
class ICodeStylePreferences;

namespace Ui { class TabPreferencesWidget; }

class TEXTEDITOR_EXPORT SimpleCodeStylePreferencesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimpleCodeStylePreferencesWidget(QWidget *parent = 0);

    void setPreferences(ICodeStylePreferences *tabPreferences);

    void setFlat(bool on);
    TabSettingsWidget *tabSettingsWidget() const;

private:
    void slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences);
    void slotTabSettingsChanged(const TextEditor::TabSettings &settings);

    TabSettingsWidget *m_tabSettingsWidget;
    ICodeStylePreferences *m_preferences;
};

} // namespace TextEditor
