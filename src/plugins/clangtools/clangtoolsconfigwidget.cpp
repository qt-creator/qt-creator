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

#include "clangtoolsconfigwidget.h"
#include "ui_clangtoolsbasicsettings.h"
#include "ui_clangtoolsconfigwidget.h"

#include "clangtoolsutils.h"

#include <cpptools/clangdiagnosticconfigswidget.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>

#include <QDir>
#include <QThread>

namespace ClangTools {
namespace Internal {

ClangToolsConfigWidget::ClangToolsConfigWidget(
        ClangToolsSettings *settings,
        QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ClangToolsConfigWidget)
    , m_settings(settings)
{
    m_ui->setupUi(this);

    m_ui->simultaneousProccessesSpinBox->setValue(settings->savedSimultaneousProcesses());
    m_ui->simultaneousProccessesSpinBox->setMinimum(1);
    m_ui->simultaneousProccessesSpinBox->setMaximum(QThread::idealThreadCount());
    connect(m_ui->simultaneousProccessesSpinBox,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [settings](int count) { settings->setSimultaneousProcesses(count); });

    QCheckBox *buildBeforeAnalysis = m_ui->clangToolsBasicSettings->ui()->buildBeforeAnalysis;
    buildBeforeAnalysis->setCheckState(settings->savedBuildBeforeAnalysis()
                                              ? Qt::Checked : Qt::Unchecked);
    connect(buildBeforeAnalysis, &QCheckBox::toggled, [settings](bool checked) {
        settings->setBuildBeforeAnalysis(checked);
    });

    CppTools::ClangDiagnosticConfigsSelectionWidget *clangDiagnosticConfigsSelectionWidget
            = m_ui->clangToolsBasicSettings->ui()->clangDiagnosticConfigsSelectionWidget;
    clangDiagnosticConfigsSelectionWidget->refresh(settings->savedDiagnosticConfigId());

    connect(clangDiagnosticConfigsSelectionWidget,
            &CppTools::ClangDiagnosticConfigsSelectionWidget::currentConfigChanged,
            this, [this](const Core::Id &currentConfigId) {
        m_settings->setDiagnosticConfigId(currentConfigId);
    });

    connect(CppTools::codeModelSettings().data(), &CppTools::CppCodeModelSettings::changed,
            this, [=]() {
        // Settings were applied so apply also the current selection if possible.
        clangDiagnosticConfigsSelectionWidget->refresh(m_settings->diagnosticConfigId());
        m_settings->writeSettings();
    });
}

ClangToolsConfigWidget::~ClangToolsConfigWidget() = default;

} // namespace Internal
} // namespace ClangTools
