/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "ibconsolestepconfigwidget.h"

#include "ui_ibconsolebuildstep.h"

#include <coreplugin/variablechooser.h>

namespace IncrediBuild {
namespace Internal {

IBConsoleStepConfigWidget::IBConsoleStepConfigWidget(IBConsoleBuildStep *ibConsoleStep)
    : ProjectExplorer::BuildStepConfigWidget(ibConsoleStep)
    , m_buildStepUI(nullptr)
    , m_buildStep(ibConsoleStep)
{
    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!ibConsoleStep->loadedFromMap())
        ibConsoleStep->tryToMigrate();

    m_buildStepUI = new Ui_IBConsoleBuildStep();
    m_buildStepUI->setupUi(this);
    Core::VariableChooser::addSupportForChildWidgets(this, ibConsoleStep->macroExpander());

    m_buildStepUI->commandBuilder->addItems(m_buildStep->supportedCommandBuilders());
    m_buildStepUI->commandBuilder->setCurrentText(m_buildStep->commandBuilder()->displayName());
    connect(m_buildStepUI->commandBuilder, &QComboBox::currentTextChanged, this, &IBConsoleStepConfigWidget::commandBuilderChanged);

    QString command, defaultCommand;
    defaultCommand = m_buildStep->commandBuilder()->defaultCommand();
    m_buildStepUI->makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    command = m_buildStep->commandBuilder()->command();
    if (command != defaultCommand)
        m_buildStepUI->makePathChooser->setPath(command);

    m_buildStepUI->makePathChooser->setExpectedKind(Utils::PathChooser::Kind::ExistingCommand);
    m_buildStepUI->makePathChooser->setBaseDirectory(Utils::FilePath::fromString(Utils::PathChooser::homePath()));
    m_buildStepUI->makePathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.IBConsole.MakeCommand.History"));
    connect(m_buildStepUI->makePathChooser, &Utils::PathChooser::rawPathChanged, this, &IBConsoleStepConfigWidget::makePathEdited);

    QString defaultArgs;
    for (const QString &a : m_buildStep->commandBuilder()->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_buildStep->commandBuilder()->arguments())
        args += "\"" + a + "\" ";

    m_buildStepUI->makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
    if (args != defaultArgs)
        m_buildStepUI->makeArgumentsLineEdit->setText(args);
    connect(m_buildStepUI->makeArgumentsLineEdit, &QLineEdit::textEdited, this, &IBConsoleStepConfigWidget::commandArgsChanged);

    m_buildStepUI->niceSpin->setValue(m_buildStep->nice());
    connect(m_buildStepUI->niceSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &IBConsoleStepConfigWidget::niceChanged);

    m_buildStepUI->keepJobsNum->setChecked(m_buildStep->keepJobNum());
    connect(m_buildStepUI->keepJobsNum, &QCheckBox::stateChanged, this, &IBConsoleStepConfigWidget::keepJobNumChanged);

    m_buildStepUI->alternate->setChecked(m_buildStep->alternate());
    connect(m_buildStepUI->alternate, &QCheckBox::stateChanged, this, &IBConsoleStepConfigWidget::alternateChanged);

    m_buildStepUI->forceRemote->setChecked(m_buildStep->forceRemote());
    connect(m_buildStepUI->forceRemote, &QCheckBox::stateChanged, this, &IBConsoleStepConfigWidget::forceRemoteChanged);
}

IBConsoleStepConfigWidget::~IBConsoleStepConfigWidget()
{
    delete m_buildStepUI;
    m_buildStepUI = nullptr;
}

QString IBConsoleStepConfigWidget::displayName() const
{
    return tr("IncrediBuild for Linux");
}

QString IBConsoleStepConfigWidget::summaryText() const
{
    return "<b>" + displayName() + "</b>";
}

void IBConsoleStepConfigWidget::niceChanged(int)
{
    m_buildStep->nice(m_buildStepUI->niceSpin->value());
}

void IBConsoleStepConfigWidget::commandBuilderChanged(const QString&)
{
    m_buildStep->commandBuilder(m_buildStepUI->commandBuilder->currentText());

    QString defaultArgs;
    for (const QString &a : m_buildStep->commandBuilder()->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_buildStep->commandBuilder()->arguments())
        args += "\"" + a + "\" ";
    if (args != defaultArgs)
        m_buildStepUI->makeArgumentsLineEdit->setText(args);
    else
        m_buildStepUI->makeArgumentsLineEdit->setText("");

    QString command, defaultCommand;
    defaultCommand = m_buildStep->commandBuilder()->defaultCommand();
    m_buildStepUI->makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    command = m_buildStep->commandBuilder()->command();
    if (command != defaultCommand)
        m_buildStepUI->makePathChooser->setPath(command);
    else
        m_buildStepUI->makePathChooser->setPath("");
}

void IBConsoleStepConfigWidget::commandArgsChanged(const QString&)
{
    m_buildStep->commandBuilder()->arguments(m_buildStepUI->makeArgumentsLineEdit->text());
}

void IBConsoleStepConfigWidget::makePathEdited()
{
    m_buildStep->commandBuilder()->command(m_buildStepUI->makePathChooser->rawPath());
}

void IBConsoleStepConfigWidget::keepJobNumChanged()
{
    m_buildStep->keepJobNum(m_buildStepUI->keepJobsNum->checkState() == Qt::CheckState::Checked);
}

void IBConsoleStepConfigWidget::alternateChanged()
{
    m_buildStep->alternate(m_buildStepUI->alternate->checkState() == Qt::CheckState::Checked);
}

void IBConsoleStepConfigWidget::forceRemoteChanged()
{
    m_buildStep->forceRemote(m_buildStepUI->forceRemote->checkState() == Qt::CheckState::Checked);
}

} // namespace Internal
} // namespace IncrediBuild
