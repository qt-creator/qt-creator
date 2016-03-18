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

namespace TextEditor {

class DisplaySettings;
class MarginSettings;

class DisplaySettingsPageParameters
{
public:
    Core::Id id;
    QString displayName;
    QString settingsPrefix;
};

class DisplaySettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    DisplaySettingsPage(const DisplaySettingsPageParameters &p, QObject *parent);
    ~DisplaySettingsPage();

    // IOptionsPage
    QWidget *widget();
    void apply();
    void finish();

    const DisplaySettings &displaySettings() const;
    const MarginSettings &marginSettings() const;

signals:
    void displaySettingsChanged(const TextEditor::DisplaySettings &);
    void marginSettingsChanged(const TextEditor::MarginSettings &);

private:
    void settingsFromUI(DisplaySettings &displaySettings, MarginSettings &marginSettings) const;
    void settingsToUI();
    void setDisplaySettings(const DisplaySettings &, const MarginSettings &newMarginSettings);

    struct DisplaySettingsPagePrivate;
    DisplaySettingsPagePrivate *d;
};

} // namespace TextEditor
