// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "runsettingswidget.h"
#include "ui_runsettingswidget.h"

#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"
#include "executableinfo.h"
#include "settingswidget.h"

#include <cppeditor/clangdiagnosticconfigswidget.h>

#include <QThread>

using namespace Utils;

namespace ClangTools {
namespace Internal {

RunSettingsWidget::RunSettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::RunSettingsWidget)
{
    m_ui->setupUi(this);
}

RunSettingsWidget::~RunSettingsWidget()
{
    delete m_ui;
}

CppEditor::ClangDiagnosticConfigsSelectionWidget *RunSettingsWidget::diagnosticSelectionWidget()
{
    return m_ui->diagnosticWidget;
}

static CppEditor::ClangDiagnosticConfigsWidget *createEditWidget(
    const CppEditor::ClangDiagnosticConfigs &configs, const Utils::Id &configToSelect)
{
    // Determine executable paths
    FilePath clangTidyPath;
    FilePath clazyStandalonePath;
    if (auto settingsWidget = SettingsWidget::instance()) {
        // Global settings case; executables might not yet applied to settings
        clangTidyPath = settingsWidget->clangTidyPath();
        clangTidyPath = clangTidyPath.isEmpty() ? clangTidyFallbackExecutable()
                                                : fullPath(clangTidyPath);

        clazyStandalonePath = settingsWidget->clazyStandalonePath();
        clazyStandalonePath = clazyStandalonePath.isEmpty() ? clazyStandaloneFallbackExecutable()
                                                            : fullPath(clazyStandalonePath);
    } else {
        // "Projects Mode > Clang Tools" case, check settings
        clangTidyPath = clangTidyExecutable();
        clazyStandalonePath = clazyStandaloneExecutable();
    }

    return new DiagnosticConfigsWidget(configs,
                                       configToSelect,
                                       ClangTidyInfo(clangTidyPath),
                                       ClazyStandaloneInfo::getInfo(clazyStandalonePath));
}

void RunSettingsWidget::fromSettings(const RunSettings &s)
{
    disconnect(m_ui->diagnosticWidget, 0, 0, 0);
    m_ui->diagnosticWidget->refresh(diagnosticConfigsModel(),
                                    s.diagnosticConfigId(),
                                    createEditWidget);
    connect(m_ui->diagnosticWidget,
            &CppEditor::ClangDiagnosticConfigsSelectionWidget::changed,
            this,
            &RunSettingsWidget::changed);

    disconnect(m_ui->buildBeforeAnalysis, 0, 0, 0);
    m_ui->buildBeforeAnalysis->setToolTip(hintAboutBuildBeforeAnalysis());
    m_ui->buildBeforeAnalysis->setCheckState(s.buildBeforeAnalysis() ? Qt::Checked : Qt::Unchecked);
    connect(m_ui->buildBeforeAnalysis, &QCheckBox::toggled, [this](bool checked) {
        if (!checked)
            showHintAboutBuildBeforeAnalysis();
        emit changed();
    });

    disconnect(m_ui->parallelJobsSpinBox, 0, 0, 0);
    m_ui->parallelJobsSpinBox->setValue(s.parallelJobs());
    m_ui->parallelJobsSpinBox->setMinimum(1);
    m_ui->parallelJobsSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->parallelJobsSpinBox, &QSpinBox::valueChanged, this, &RunSettingsWidget::changed);
    m_ui->analyzeOpenFiles->setChecked(s.analyzeOpenFiles());
    connect(m_ui->analyzeOpenFiles, &QCheckBox::toggled, this, &RunSettingsWidget::changed);

}

RunSettings RunSettingsWidget::toSettings() const
{
    RunSettings s;
    s.setDiagnosticConfigId(m_ui->diagnosticWidget->currentConfigId());
    s.setBuildBeforeAnalysis(m_ui->buildBeforeAnalysis->checkState() == Qt::CheckState::Checked);
    s.setParallelJobs(m_ui->parallelJobsSpinBox->value());
    s.setAnalyzeOpenFiles(m_ui->analyzeOpenFiles->checkState() == Qt::CheckState::Checked);

    return s;
}

} // namespace Internal
} // namespace ClangTools
