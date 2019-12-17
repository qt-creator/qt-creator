/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimcompilerbuildstepconfigwidget.h"
#include "nimbuildconfiguration.h"
#include "nimbuildsystem.h"
#include "nimcompilerbuildstep.h"

#include "ui_nimcompilerbuildstepconfigwidget.h"

#include "../nimconstants.h"

#include <projectexplorer/processparameters.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimCompilerBuildStepConfigWidget::NimCompilerBuildStepConfigWidget(NimCompilerBuildStep *buildStep)
    : BuildStepConfigWidget(buildStep)
    , m_buildStep(buildStep)
    , m_ui(new Ui::NimCompilerBuildStepConfigWidget())
{
    m_ui->setupUi(this);

    setDisplayName(tr(Constants::C_NIMCOMPILERBUILDSTEPWIDGET_DISPLAY));
    setSummaryText(tr(Constants::C_NIMCOMPILERBUILDSTEPWIDGET_SUMMARY));

    // Connect the project signals
    connect(m_buildStep->project(),
            &Project::fileListChanged,
            this,
            &NimCompilerBuildStepConfigWidget::updateUi);

    // Connect build step signals
    connect(m_buildStep, &NimCompilerBuildStep::processParametersChanged,
            this, &NimCompilerBuildStepConfigWidget::updateUi);

    // Connect UI signals
    connect(m_ui->targetComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &NimCompilerBuildStepConfigWidget::onTargetChanged);
    connect(m_ui->additionalArgumentsLineEdit, &QLineEdit::textEdited,
            this, &NimCompilerBuildStepConfigWidget::onAdditionalArgumentsTextEdited);
    connect(m_ui->defaultArgumentsComboBox, QOverload<int>::of(&QComboBox::activated),
            this, &NimCompilerBuildStepConfigWidget::onDefaultArgumentsComboBoxIndexChanged);

    updateUi();
}

NimCompilerBuildStepConfigWidget::~NimCompilerBuildStepConfigWidget() = default;

void NimCompilerBuildStepConfigWidget::onTargetChanged(int index)
{
    Q_UNUSED(index)
    auto data = m_ui->targetComboBox->currentData();
    FilePath path = FilePath::fromString(data.toString());
    m_buildStep->setTargetNimFile(path);
}

void NimCompilerBuildStepConfigWidget::onDefaultArgumentsComboBoxIndexChanged(int index)
{
    auto options = static_cast<NimCompilerBuildStep::DefaultBuildOptions>(index);
    m_buildStep->setDefaultCompilerOptions(options);
}

void NimCompilerBuildStepConfigWidget::updateUi()
{
    updateCommandLineText();
    updateTargetComboBox();
    updateAdditionalArgumentsLineEdit();
    updateDefaultArgumentsComboBox();
}

void NimCompilerBuildStepConfigWidget::onAdditionalArgumentsTextEdited(const QString &text)
{
    m_buildStep->setUserCompilerOptions(text.split(QChar::Space));
}

void NimCompilerBuildStepConfigWidget::updateCommandLineText()
{
    ProcessParameters *parameters = m_buildStep->processParameters();

    const CommandLine cmd = parameters->command();
    const QStringList parts = QtcProcess::splitArgs(cmd.toUserOutput());

    m_ui->commandTextEdit->setText(parts.join(QChar::LineFeed));
}

void NimCompilerBuildStepConfigWidget::updateTargetComboBox()
{
    QTC_ASSERT(m_buildStep, return );

    // Re enter the files
    m_ui->targetComboBox->clear();

    const FilePaths nimFiles = m_buildStep->project()->files([](const Node *n) {
        return Project::AllFiles(n) && n->path().endsWith(".nim");
    });

    for (const FilePath &file : nimFiles)
        m_ui->targetComboBox->addItem(file.fileName(), file.toString());

    const int index = m_ui->targetComboBox->findData(m_buildStep->targetNimFile().toString());
    m_ui->targetComboBox->setCurrentIndex(index);
}

void NimCompilerBuildStepConfigWidget::updateAdditionalArgumentsLineEdit()
{
    const QString text = m_buildStep->userCompilerOptions().join(QChar::Space);
    m_ui->additionalArgumentsLineEdit->setText(text);
}

void NimCompilerBuildStepConfigWidget::updateDefaultArgumentsComboBox()
{
    const int index = m_buildStep->defaultCompilerOptions();
    m_ui->defaultArgumentsComboBox->setCurrentIndex(index);
}

}

