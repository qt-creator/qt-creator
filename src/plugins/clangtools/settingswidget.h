// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolssettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <memory>

namespace Utils { class FilePath; }

namespace ClangTools {
namespace Internal {

namespace Ui { class SettingsWidget; }

class SettingsWidget : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    static SettingsWidget *instance();

    SettingsWidget();
    ~SettingsWidget() override;

    Utils::FilePath clangTidyPath() const;
    Utils::FilePath clazyStandalonePath() const;

private:
    void apply() final;

    std::unique_ptr<Ui::SettingsWidget> m_ui;
    ClangToolsSettings *m_settings;
};

class ClangToolsOptionsPage final : public Core::IOptionsPage
{
public:
    ClangToolsOptionsPage();
};

} // namespace Internal
} // namespace ClangTools
