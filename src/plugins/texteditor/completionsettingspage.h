// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commentssettings.h"
#include "completionsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace TextEditor {
namespace Internal {

class CompletionSettingsPage : public Core::IOptionsPage
{
public:
    CompletionSettingsPage();

    const CompletionSettings &completionSettings() const;
    const CommentsSettings &commentsSettings() const;

private:
    friend class CompletionSettingsPageWidget;

    CommentsSettings m_commentsSettings;
    CompletionSettings m_completionSettings;
};

} // namespace Internal
} // namespace TextEditor
