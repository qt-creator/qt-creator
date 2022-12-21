// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangtoolssettings.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <memory>

namespace Utils {
class FilePath;
class PathChooser;
} // Utils

namespace ClangTools::Internal {

class RunSettingsWidget;

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

    ClangToolsSettings *m_settings;

    Utils::PathChooser *m_clangTidyPathChooser;
    Utils::PathChooser *m_clazyStandalonePathChooser;
    RunSettingsWidget *m_runSettingsWidget;
};

class ClangToolsOptionsPage final : public Core::IOptionsPage
{
public:
    ClangToolsOptionsPage();
};

} // ClangTools::Internal
