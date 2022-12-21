// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor {

class DisplaySettings;
class MarginSettings;
class DisplaySettingsPagePrivate;

class DisplaySettingsPage : public Core::IOptionsPage
{
public:
    DisplaySettingsPage();
    ~DisplaySettingsPage() override;

    const DisplaySettings &displaySettings() const;
    const MarginSettings &marginSettings() const;

private:
    DisplaySettingsPagePrivate *d;
};

} // namespace TextEditor
