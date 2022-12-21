// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor {
namespace Internal {

class SnippetsSettingsPagePrivate;

class SnippetsSettingsPage final : public Core::IOptionsPage
{
public:
    SnippetsSettingsPage();
    ~SnippetsSettingsPage() override;

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    SnippetsSettingsPagePrivate *d;
};

} // Internal
} // TextEditor
