// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"
#include "runsettingswidget.h"

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <debugger/analyzer/analyzericons.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>

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

    resize(400, 300);

    QString placeHolderText = toolShippedExecutable(ClangToolType::Tidy).toUserOutput();
    FilePath path = m_settings->executable(ClangToolType::Tidy);
    if (path.isEmpty() && placeHolderText.isEmpty())
        path = Constants::CLANG_TIDY_EXECUTABLE_NAME;
    m_clangTidyPathChooser = new PathChooser;
    m_clangTidyPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_clangTidyPathChooser->setPromptDialogTitle(tr("Clang-Tidy Executable"));
    m_clangTidyPathChooser->setDefaultValue(placeHolderText);
    m_clangTidyPathChooser->setFilePath(path);
    m_clangTidyPathChooser->setHistoryCompleter("ClangTools.ClangTidyExecutable.History");

    placeHolderText = toolShippedExecutable(ClangToolType::Clazy).toUserOutput();
    path = m_settings->executable(ClangToolType::Clazy);
    if (path.isEmpty() && placeHolderText.isEmpty())
        path = Constants::CLAZY_STANDALONE_EXECUTABLE_NAME;
    m_clazyStandalonePathChooser = new PathChooser;
    m_clazyStandalonePathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_clazyStandalonePathChooser->setPromptDialogTitle(tr("Clazy Executable"));
    m_clazyStandalonePathChooser->setDefaultValue(placeHolderText);
    m_clazyStandalonePathChooser->setFilePath(path);
    m_clazyStandalonePathChooser->setHistoryCompleter("ClangTools.ClazyStandaloneExecutable.History");

    m_runSettingsWidget = new RunSettingsWidget;
    m_runSettingsWidget->fromSettings(m_settings->runSettings());

    using namespace Layouting;

    Column {
        Group {
            title(tr("Executables")),
            Form {
                tr("Clang-Tidy:"), m_clangTidyPathChooser, br,
                tr("Clazy-Standalone:"), m_clazyStandalonePathChooser
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
    setDisplayName(QCoreApplication::translate(
                       "ClangTools::Internal::ClangToolsOptionsPage",
                       "Clang Tools"));
    setCategory("T.Analyzer");
    setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([] { return new SettingsWidget; });
}

} // ClangTools::Internal
