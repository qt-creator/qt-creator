// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "settingswidget.h"

#include "ui_settingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <debugger/analyzer/analyzericons.h>

#include <utils/optional.h>

using namespace Utils;

namespace ClangTools {
namespace Internal {

static SettingsWidget *m_instance = nullptr;

static void setupPathChooser(PathChooser *const chooser,
                             const QString &promptDiaglogTitle,
                             const QString &placeHolderText,
                             const FilePath &pathFromSettings,
                             const QString &historyCompleterId)
{
    chooser->setPromptDialogTitle(promptDiaglogTitle);
    chooser->setDefaultValue(placeHolderText);
    chooser->setFilePath(pathFromSettings);
    chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    chooser->setHistoryCompleter(historyCompleterId);
}

SettingsWidget *SettingsWidget::instance()
{
    return m_instance;
}

SettingsWidget::SettingsWidget()
    : m_ui(new Ui::SettingsWidget)
    , m_settings(ClangToolsSettings::instance())
{
    m_instance = this;
    m_ui->setupUi(this);

    //
    // Group box "Executables"
    //

    QString placeHolderText = shippedClangTidyExecutable().toUserOutput();
    FilePath path = m_settings->clangTidyExecutable();
    if (path.isEmpty() && placeHolderText.isEmpty())
        path = Constants::CLANG_TIDY_EXECUTABLE_NAME;
    setupPathChooser(m_ui->clangTidyPathChooser,
                     tr("Clang-Tidy Executable"),
                     placeHolderText,
                     path,
                     "ClangTools.ClangTidyExecutable.History");

    placeHolderText = shippedClazyStandaloneExecutable().toUserOutput();
    path = m_settings->clazyStandaloneExecutable();
    if (path.isEmpty() && placeHolderText.isEmpty())
        path = Constants::CLAZY_STANDALONE_EXECUTABLE_NAME;
    setupPathChooser(m_ui->clazyStandalonePathChooser,
                     tr("Clazy Executable"),
                     placeHolderText,
                     path,
                     "ClangTools.ClazyStandaloneExecutable.History");

    //
    // Group box "Run Options"
    //

    m_ui->runSettingsWidget->fromSettings(m_settings->runSettings());
}

void SettingsWidget::apply()
{
    // Executables
    m_settings->setClangTidyExecutable(clangTidyPath());
    m_settings->setClazyStandaloneExecutable(clazyStandalonePath());

    // Run options
    m_settings->setRunSettings(m_ui->runSettingsWidget->toSettings());

    // Custom configs
    const CppEditor::ClangDiagnosticConfigs customConfigs
        = m_ui->runSettingsWidget->diagnosticSelectionWidget()->customConfigs();
    m_settings->setDiagnosticConfigs(customConfigs);

    m_settings->writeSettings();
}

SettingsWidget::~SettingsWidget()
{
    m_instance = nullptr;
}

FilePath SettingsWidget::clangTidyPath() const
{
    return m_ui->clangTidyPathChooser->rawFilePath();
}

FilePath SettingsWidget::clazyStandalonePath() const
{
    return m_ui->clazyStandalonePathChooser->rawFilePath();
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

} // namespace Internal
} // namespace ClangTools
