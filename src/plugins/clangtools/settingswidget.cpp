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

#include <coreplugin/icore.h>

#include <cpptools/clangdiagnosticconfigswidget.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

#include <QDir>
#include <QThread>

#include <memory>

namespace ClangTools {
namespace Internal {

static void setupPathChooser(Utils::PathChooser *const chooser,
                             const QString &promptDiaglogTitle,
                             const QString &placeHolderText,
                             const QString &pathFromSettings,
                             const QString &historyCompleterId,
                             std::function<void(const QString &path)> savePath)
{
    chooser->setPromptDialogTitle(promptDiaglogTitle);
    chooser->lineEdit()->setPlaceholderText(placeHolderText);
    chooser->setPath(pathFromSettings);
    chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    chooser->setHistoryCompleter(historyCompleterId);
    QObject::connect(chooser, &Utils::PathChooser::rawPathChanged, savePath),
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
                     "ClangTools.ClangTidyExecutable.History",
                     [settings](const QString &path) { settings->setClangTidyExecutable(path); });

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
                         "ClangTools.ClazyStandaloneExecutable.History",
                         [settings](const QString &path) {
                             settings->setClazyStandaloneExecutable(path);
                         });
    }

    //
    // Group box "Run Options"
    //
    m_ui->simultaneousProccessesSpinBox->setValue(settings->savedSimultaneousProcesses());
    m_ui->simultaneousProccessesSpinBox->setMinimum(1);
    m_ui->simultaneousProccessesSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->simultaneousProccessesSpinBox,
            QOverload<int>::of(&QSpinBox::valueChanged),
            [settings](int count) { settings->setSimultaneousProcesses(count); });

    QCheckBox *buildBeforeAnalysis = m_ui->buildBeforeAnalysis;
    buildBeforeAnalysis->setToolTip(hintAboutBuildBeforeAnalysis());
    buildBeforeAnalysis->setCheckState(settings->savedBuildBeforeAnalysis()
                                              ? Qt::Checked : Qt::Unchecked);
    connect(buildBeforeAnalysis, &QCheckBox::toggled, [settings](bool checked) {
        if (!checked)
            showHintAboutBuildBeforeAnalysis();
        settings->setBuildBeforeAnalysis(checked);
    });

    CppTools::ClangDiagnosticConfigsSelectionWidget *diagnosticWidget = m_ui->diagnosticWidget;
    diagnosticWidget->refresh(settings->savedDiagnosticConfigId());

    connect(diagnosticWidget,
            &CppTools::ClangDiagnosticConfigsSelectionWidget::currentConfigChanged,
            this, [this](const Core::Id &currentConfigId) {
        m_settings->setDiagnosticConfigId(currentConfigId);
    });

    connect(CppTools::codeModelSettings().data(), &CppTools::CppCodeModelSettings::changed,
            this, [=]() {
        // Settings were applied so apply also the current selection if possible.
        diagnosticWidget->refresh(m_settings->diagnosticConfigId());
        m_settings->writeSettings();
    });
}

SettingsWidget::~SettingsWidget() = default;

} // namespace Internal
} // namespace ClangTools
