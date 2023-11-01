// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"
#include "runsettingswidget.h"

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools::Internal {

static SettingsWidget *m_instance = nullptr;

SettingsWidget *SettingsWidget::instance()
{
    return m_instance;
}

SettingsWidget::SettingsWidget()
    : m_settings(ClangToolsSettings::instance())
{
    m_instance = this;

    auto createPathChooser = [this](ClangToolType tool)
    {
        const QString placeHolderText = toolShippedExecutable(tool).toUserOutput();
        FilePath path = m_settings->executable(tool);
        if (path.isEmpty() && placeHolderText.isEmpty()) {
            path = tool == ClangToolType::Tidy ? FilePath(Constants::CLANG_TIDY_EXECUTABLE_NAME)
                                               : FilePath(Constants::CLAZY_STANDALONE_EXECUTABLE_NAME);
        }
        PathChooser *pathChooser = new PathChooser;
        pathChooser->setExpectedKind(PathChooser::ExistingCommand);
        pathChooser->setPromptDialogTitle(tool == ClangToolType::Tidy ? Tr::tr("Clang-Tidy Executable")
                                                                      : Tr::tr("Clazy Executable"));
        pathChooser->setDefaultValue(placeHolderText);
        pathChooser->setFilePath(path);
        pathChooser->setHistoryCompleter(tool == ClangToolType::Tidy
                                        ? Key("ClangTools.ClangTidyExecutable.History")
                                        : Key("ClangTools.ClazyStandaloneExecutable.History"));
        pathChooser->setCommandVersionArguments({"--version"});
        return pathChooser;
    };
    m_clangTidyPathChooser = createPathChooser(ClangToolType::Tidy);
    m_clazyStandalonePathChooser = createPathChooser(ClangToolType::Clazy);

    m_runSettingsWidget = new RunSettingsWidget;
    m_runSettingsWidget->fromSettings(m_settings->runSettings());

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Executables")),
            Form {
                Tr::tr("Clang-Tidy:"), m_clangTidyPathChooser, br,
                Tr::tr("Clazy-Standalone:"), m_clazyStandalonePathChooser
            }
        },
        m_runSettingsWidget,
        st
    }.attachTo(this);
}

void SettingsWidget::apply()
{
    // Executables
    m_settings->setExecutable(ClangToolType::Tidy, clangTidyPath());
    m_settings->setExecutable(ClangToolType::Clazy, clazyStandalonePath());

    // Run options
    m_settings->setRunSettings(m_runSettingsWidget->toSettings());

    // Custom configs
    const ClangDiagnosticConfigs customConfigs
        = m_runSettingsWidget->diagnosticSelectionWidget()->customConfigs();
    m_settings->setDiagnosticConfigs(customConfigs);

    m_settings->writeSettings();
}

SettingsWidget::~SettingsWidget()
{
    m_instance = nullptr;
}

FilePath SettingsWidget::clangTidyPath() const
{
    return m_clangTidyPathChooser->rawFilePath();
}

FilePath SettingsWidget::clazyStandalonePath() const
{
    return m_clazyStandalonePathChooser->rawFilePath();
}

// ClangToolsOptionsPage

ClangToolsOptionsPage::ClangToolsOptionsPage()
{
    setId(Constants::SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Clang Tools"));
    setCategory("T.Analyzer");
    setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([] { return new SettingsWidget; });
}

} // ClangTools::Internal
