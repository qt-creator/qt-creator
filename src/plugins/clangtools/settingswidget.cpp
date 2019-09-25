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

#include "settingswidget.h"

#include "ui_settingswidget.h"

#include "clangtoolsconstants.h"
#include "clangtoolsutils.h"

#include <cpptools/clangdiagnosticconfigsmodel.h>

#include <utils/optional.h>

namespace ClangTools {
namespace Internal {

static void setupPathChooser(Utils::PathChooser *const chooser,
                             const QString &promptDiaglogTitle,
                             const QString &placeHolderText,
                             const QString &pathFromSettings,
                             const QString &historyCompleterId)
{
    chooser->setPromptDialogTitle(promptDiaglogTitle);
    chooser->lineEdit()->setPlaceholderText(placeHolderText);
    chooser->setPath(pathFromSettings);
    chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    chooser->setHistoryCompleter(historyCompleterId);
    chooser->setValidationFunction([chooser](Utils::FancyLineEdit *edit, QString *errorMessage) {
        const QString currentFilePath = chooser->fileName().toString();
        Utils::PathChooser pc;
        Utils::PathChooser *helperPathChooser;
        if (currentFilePath.isEmpty()) {
            pc.setExpectedKind(chooser->expectedKind());
            pc.setPath(edit->placeholderText());
            helperPathChooser = &pc;
        } else {
            helperPathChooser = chooser;
        }

        return chooser->defaultValidationFunction()(helperPathChooser->lineEdit(), errorMessage);
    });
}

SettingsWidget::SettingsWidget(ClangToolsSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SettingsWidget)
    , m_settings(settings)
{
    m_ui->setupUi(this);

    //
    // Group box "Executables"
    //

    QString placeHolderText = shippedClangTidyExecutable();
    QString path = settings->clangTidyExecutable();
    if (path.isEmpty() && placeHolderText.isEmpty())
        path = Constants::CLANG_TIDY_EXECUTABLE_NAME;
    setupPathChooser(m_ui->clangTidyPathChooser,
                     tr("Clang-Tidy Executable"),
                     placeHolderText,
                     path,
                     "ClangTools.ClangTidyExecutable.History");

    if (qEnvironmentVariable("QTC_USE_CLAZY_STANDALONE_PATH").isEmpty()) {
        m_ui->clazyStandalonePathChooser->setVisible(false);
        m_ui->clazyStandaloneLabel->setVisible(false);
    } else {
        placeHolderText = shippedClazyStandaloneExecutable();
        path = settings->clazyStandaloneExecutable();
        if (path.isEmpty() && placeHolderText.isEmpty())
            path = Constants::CLAZY_STANDALONE_EXECUTABLE_NAME;
        setupPathChooser(m_ui->clazyStandalonePathChooser,
                         tr("Clazy Executable"),
                         placeHolderText,
                         path,
                         "ClangTools.ClazyStandaloneExecutable.History");
    }

    //
    // Group box "Run Options"
    //

    m_ui->runSettingsWidget->fromSettings(m_settings->runSettings());
    connect(m_ui->runSettingsWidget,
            &RunSettingsWidget::diagnosticConfigsEdited,
            this,
            [this](const CppTools::ClangDiagnosticConfigs &configs) {
                const CppTools::ClangDiagnosticConfigsModel configsModel = diagnosticConfigsModel(
                    configs);
                RunSettings runSettings = m_settings->runSettings();
                if (!configsModel.hasConfigWithId(m_settings->runSettings().diagnosticConfigId())) {
                    runSettings.resetDiagnosticConfigId();
                    m_settings->setRunSettings(runSettings);
                }
                m_settings->setDiagnosticConfigs(configs);
                m_settings->writeSettings();
                m_ui->runSettingsWidget->fromSettings(runSettings);
            });
}

void SettingsWidget::apply()
{
    m_settings->setClangTidyExecutable(m_ui->clangTidyPathChooser->rawPath());
    m_settings->setClazyStandaloneExecutable(m_ui->clazyStandalonePathChooser->rawPath());
    m_settings->setRunSettings(m_ui->runSettingsWidget->toSettings());

    m_settings->writeSettings();
}

SettingsWidget::~SettingsWidget() = default;

} // namespace Internal
} // namespace ClangTools
