// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runsettingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolssettings.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"
#include "executableinfo.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>
#include <cppeditor/clangdiagnosticconfigswidget.h>

#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools::Internal {

class SettingsWidget : public Core::IOptionsPageWidget
{
public:
    SettingsWidget();
    ~SettingsWidget() override;

private:
    void apply() final;

    ClangToolsSettings *m_settings;
    Utils::PathChooser *m_clangTidyPathChooser;
    Utils::PathChooser *m_clazyStandalonePathChooser;
};

SettingsWidget::SettingsWidget()
    : m_settings(ClangToolsSettings::instance())
{
    auto createPathChooser = [this](ClangToolType tool)
    {
        const FilePath defaultValue = toolShippedExecutable(tool);
        FilePath path = m_settings->executable(tool);
        if (path.isEmpty() && defaultValue.isEmpty()) {
            path = tool == ClangToolType::Tidy ? FilePath(Constants::CLANG_TIDY_EXECUTABLE_NAME)
                                               : FilePath(Constants::CLAZY_STANDALONE_EXECUTABLE_NAME);
        }
        PathChooser *pathChooser = new PathChooser;
        pathChooser->setExpectedKind(PathChooser::ExistingCommand);
        pathChooser->setPromptDialogTitle(tool == ClangToolType::Tidy
                                              ? Tr::tr("Clang-Tidy Executable")
                                              : Tr::tr("Clazy Executable"));
        pathChooser->setDefaultValue(defaultValue);
        pathChooser->setFilePath(path);
        pathChooser->setHistoryCompleter(tool == ClangToolType::Tidy
                                             ? Key("ClangTools.ClangTidyExecutable.History")
                                             : Key("ClangTools.ClazyStandaloneExecutable.History"));
        pathChooser->setCommandVersionArguments({"--version"});
        return pathChooser;
    };
    m_clangTidyPathChooser = createPathChooser(ClangToolType::Tidy);
    m_clazyStandalonePathChooser = createPathChooser(ClangToolType::Clazy);

    m_settings->diagnosticConfigId.setEditWidgetFactory(
        [this](const ClangDiagnosticConfigs &configs, const Id &id)
            -> ClangDiagnosticConfigsWidget * {
            FilePath tidy = m_clangTidyPathChooser->unexpandedFilePath();
            tidy = tidy.isEmpty() ? toolFallbackExecutable(ClangToolType::Tidy)
                                  : fullPath(tidy);
            FilePath clazy = m_clazyStandalonePathChooser->unexpandedFilePath();
            clazy = clazy.isEmpty() ? toolFallbackExecutable(ClangToolType::Clazy)
                                    : fullPath(clazy);
            return new DiagnosticConfigsWidget(configs, id,
                                               ClangTidyInfo(tidy),
                                               ClazyStandaloneInfo(clazy));
        });

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Executables")),
            Form {
                Tr::tr("Clang-Tidy:"), m_clangTidyPathChooser, br,
                Tr::tr("Clazy-Standalone:"), m_clazyStandalonePathChooser
            }
        },
        *m_settings,
        st
    }.attachTo(this);

    installMarkSettingsDirtyTriggerRecursively(this);
}

SettingsWidget::~SettingsWidget()
{
    m_settings->diagnosticConfigId.setEditWidgetFactory({});
}

void SettingsWidget::apply()
{
    m_settings->setExecutable(ClangToolType::Tidy,
                              m_clangTidyPathChooser->unexpandedFilePath());
    m_settings->setExecutable(ClangToolType::Clazy,
                              m_clazyStandalonePathChooser->unexpandedFilePath());
    m_settings->setDiagnosticConfigs(m_settings->diagnosticConfigId.customConfigs());
    m_settings->apply();
    m_settings->writeSettings();
}

// ClangToolsOptionsPage

class ClangToolsOptionsPage final : public Core::IOptionsPage
{
public:
    ClangToolsOptionsPage()
    {
        setId(Constants::SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Clang Tools"));
        setCategory("T.Analyzer");
        setWidgetCreator([] { return new SettingsWidget; });
    }
};

void setupClangToolsOptionsPage()
{
    static ClangToolsOptionsPage theClangToolsOptionsPage;
}

} // namespace ClangTools::Internal
