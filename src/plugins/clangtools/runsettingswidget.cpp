// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runsettingswidget.h"

#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "diagnosticconfigswidget.h"
#include "executableinfo.h"
#include "settingswidget.h"

#include <cppeditor/clangdiagnosticconfigswidget.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QSpinBox>
#include <QThread>

using namespace CppEditor;
using namespace Utils;

namespace ClangTools::Internal {

RunSettingsWidget::RunSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    resize(383, 125);

    m_diagnosticWidget = new ClangDiagnosticConfigsSelectionWidget;

    m_buildBeforeAnalysis = new QCheckBox(tr("Build the project before analysis"));

    m_analyzeOpenFiles = new QCheckBox(tr("Analyze open files"));

    m_parallelJobsSpinBox = new QSpinBox;
    m_parallelJobsSpinBox->setRange(1, 32);

    using namespace Layouting;

    // FIXME: Let RunSettingsWidget inherit from QGroupBox?
    Column {
        Group {
            title(tr("Run Options")),
            Column {
                m_diagnosticWidget,
                m_buildBeforeAnalysis,
                m_analyzeOpenFiles,
                Row { tr("Parallel jobs:"), m_parallelJobsSpinBox, st },
            }
        }
    }.attachTo(this, WithoutMargins);
}

RunSettingsWidget::~RunSettingsWidget() = default;

ClangDiagnosticConfigsSelectionWidget *RunSettingsWidget::diagnosticSelectionWidget()
{
    return m_diagnosticWidget;
}

static ClangDiagnosticConfigsWidget *createEditWidget(const ClangDiagnosticConfigs &configs,
                                                      const Id &configToSelect)
{
    // Determine executable paths
    FilePath clangTidyPath;
    FilePath clazyStandalonePath;
    if (auto settingsWidget = SettingsWidget::instance()) {
        // Global settings case; executables might not yet applied to settings
        clangTidyPath = settingsWidget->clangTidyPath();
        clangTidyPath = clangTidyPath.isEmpty() ? toolFallbackExecutable(ClangToolType::Tidy)
                                                : fullPath(clangTidyPath);

        clazyStandalonePath = settingsWidget->clazyStandalonePath();
        clazyStandalonePath = clazyStandalonePath.isEmpty()
                            ? toolFallbackExecutable(ClangToolType::Clazy)
                            : fullPath(clazyStandalonePath);
    } else {
        // "Projects Mode > Clang Tools" case, check settings
        clangTidyPath = toolExecutable(ClangToolType::Tidy);
        clazyStandalonePath = toolExecutable(ClangToolType::Clazy);
    }

    return new DiagnosticConfigsWidget(configs,
                                       configToSelect,
                                       ClangTidyInfo(clangTidyPath),
                                       ClazyStandaloneInfo::getInfo(clazyStandalonePath));
}

void RunSettingsWidget::fromSettings(const RunSettings &s)
{
    disconnect(m_diagnosticWidget, 0, 0, 0);
    m_diagnosticWidget->refresh(diagnosticConfigsModel(),
                                s.diagnosticConfigId(),
                                createEditWidget);
    connect(m_diagnosticWidget, &ClangDiagnosticConfigsSelectionWidget::changed,
            this, &RunSettingsWidget::changed);

    disconnect(m_buildBeforeAnalysis, 0, 0, 0);
    m_buildBeforeAnalysis->setToolTip(hintAboutBuildBeforeAnalysis());
    m_buildBeforeAnalysis->setCheckState(s.buildBeforeAnalysis() ? Qt::Checked : Qt::Unchecked);
    connect(m_buildBeforeAnalysis, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked)
            showHintAboutBuildBeforeAnalysis();
        emit changed();
    });

    disconnect(m_parallelJobsSpinBox, 0, 0, 0);
    m_parallelJobsSpinBox->setValue(s.parallelJobs());
    m_parallelJobsSpinBox->setMinimum(1);
    m_parallelJobsSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_parallelJobsSpinBox, &QSpinBox::valueChanged, this, &RunSettingsWidget::changed);
    m_analyzeOpenFiles->setChecked(s.analyzeOpenFiles());
    connect(m_analyzeOpenFiles, &QCheckBox::toggled, this, &RunSettingsWidget::changed);

}

RunSettings RunSettingsWidget::toSettings() const
{
    RunSettings s;
    s.setDiagnosticConfigId(m_diagnosticWidget->currentConfigId());
    s.setBuildBeforeAnalysis(m_buildBeforeAnalysis->checkState() == Qt::CheckState::Checked);
    s.setParallelJobs(m_parallelJobsSpinBox->value());
    s.setAnalyzeOpenFiles(m_analyzeOpenFiles->checkState() == Qt::CheckState::Checked);

    return s;
}

} // ClangTools::Internal
