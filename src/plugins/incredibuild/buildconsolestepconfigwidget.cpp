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

#include "buildconsolestepconfigwidget.h"

#include "ui_buildconsolebuildstep.h"

#include <coreplugin/variablechooser.h>

namespace IncrediBuild {
namespace Internal {

BuildConsoleStepConfigWidget::BuildConsoleStepConfigWidget(BuildConsoleBuildStep *buildConsoleStep)
    : ProjectExplorer::BuildStepConfigWidget(buildConsoleStep)
    , m_buildStepUI(nullptr)
    , m_buildStep(buildConsoleStep)
{
    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!buildConsoleStep->loadedFromMap())
        buildConsoleStep->tryToMigrate();

    m_buildStepUI = new Ui_BuildConsoleBuildStep();
    m_buildStepUI->setupUi(this);
    Core::VariableChooser::addSupportForChildWidgets(this, buildConsoleStep->macroExpander());

    m_buildStepUI->commandBuilder->addItems(m_buildStep->supportedCommandBuilders());
    m_buildStepUI->commandBuilder->setCurrentText(m_buildStep->commandBuilder()->displayName());
    connect(m_buildStepUI->commandBuilder, &QComboBox::currentTextChanged, this, &BuildConsoleStepConfigWidget::commandBuilderChanged);

    QString command, defaultCommand;
    defaultCommand = m_buildStep->commandBuilder()->defaultCommand();
    m_buildStepUI->makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    command = m_buildStep->commandBuilder()->command();
    if (command != defaultCommand)
        m_buildStepUI->makePathChooser->setPath(command);

    m_buildStepUI->makePathChooser->setExpectedKind(Utils::PathChooser::Kind::ExistingCommand);
    m_buildStepUI->makePathChooser->setBaseDirectory(Utils::FilePath::fromString(Utils::PathChooser::homePath()));
    m_buildStepUI->makePathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.MakeCommand.History"));
    connect(m_buildStepUI->makePathChooser, &Utils::PathChooser::rawPathChanged, this, &BuildConsoleStepConfigWidget::makePathEdited);

    QString defaultArgs;
    for (const QString &a : m_buildStep->commandBuilder()->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_buildStep->commandBuilder()->arguments())
        args += "\"" + a + "\" ";

    m_buildStepUI->makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
    if (args != defaultArgs)
        m_buildStepUI->makeArgumentsLineEdit->setText(args);

    connect(m_buildStepUI->makeArgumentsLineEdit, &QLineEdit::textEdited, this, &BuildConsoleStepConfigWidget::commandArgsChanged);

    m_buildStepUI->avoidLocal->setChecked(m_buildStep->avoidLocal());
    connect(m_buildStepUI->avoidLocal, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::avoidLocalChanged);

    m_buildStepUI->profileXmlPathChooser->setExpectedKind(Utils::PathChooser::Kind::File);
    m_buildStepUI->profileXmlPathChooser->setBaseDirectory(Utils::FilePath::fromString(Utils::PathChooser::homePath()));
    m_buildStepUI->profileXmlPathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.ProfileXml.History"));
    m_buildStepUI->profileXmlPathChooser->setPath(m_buildStep->profileXml());
    connect(m_buildStepUI->profileXmlPathChooser, &Utils::PathChooser::rawPathChanged, this, &BuildConsoleStepConfigWidget::profileXmlEdited);

    m_buildStepUI->maxCpuSpin->setValue(m_buildStep->maxCpu());
    connect(m_buildStepUI->maxCpuSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &BuildConsoleStepConfigWidget::maxCpuChanged);

    m_buildStepUI->newestWindowsOs->addItems(m_buildStep->supportedWindowsVersions());
    m_buildStepUI->newestWindowsOs->setCurrentText(m_buildStep->maxWinVer());
    connect(m_buildStepUI->newestWindowsOs, &QComboBox::currentTextChanged, this, &BuildConsoleStepConfigWidget::maxWinVerChanged);

    m_buildStepUI->oldestWindowsOs->addItems(m_buildStep->supportedWindowsVersions());
    m_buildStepUI->oldestWindowsOs->setCurrentText(m_buildStep->minWinVer());
    connect(m_buildStepUI->oldestWindowsOs, &QComboBox::currentTextChanged, this, &BuildConsoleStepConfigWidget::minWinVerChanged);

    m_buildStepUI->titleEdit->setText(m_buildStep->title());
    connect(m_buildStepUI->titleEdit, &QLineEdit::textEdited, this, &BuildConsoleStepConfigWidget::titleEdited);

    m_buildStepUI->monFilePathChooser->setExpectedKind(Utils::PathChooser::Kind::Any);
    m_buildStepUI->monFilePathChooser->setBaseDirectory(Utils::FilePath::fromString(Utils::PathChooser::homePath()));
    m_buildStepUI->monFilePathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.MonFile.History"));
    m_buildStepUI->monFilePathChooser->setPath(m_buildStep->monFile());
    connect(m_buildStepUI->monFilePathChooser, &Utils::PathChooser::rawPathChanged, this, &BuildConsoleStepConfigWidget::monFileEdited);

    m_buildStepUI->suppressStdOut->setChecked(m_buildStep->suppressStdOut());
    connect(m_buildStepUI->suppressStdOut, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::suppressStdOutChanged);

    m_buildStepUI->logFilePathChooser->setExpectedKind(Utils::PathChooser::Kind::SaveFile);
    m_buildStepUI->logFilePathChooser->setBaseDirectory(Utils::FilePath::fromString(Utils::PathChooser::homePath()));
    m_buildStepUI->logFilePathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.BuildConsole.LogFile.History"));
    m_buildStepUI->logFilePathChooser->setPath(m_buildStep->logFile());
    connect(m_buildStepUI->logFilePathChooser, &Utils::PathChooser::rawPathChanged, this, &BuildConsoleStepConfigWidget::logFileEdited);

    m_buildStepUI->showCmd->setChecked(m_buildStep->showCmd());
    connect(m_buildStepUI->showCmd, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::showCmdChanged);

    m_buildStepUI->showAgents->setChecked(m_buildStep->showAgents());
    connect(m_buildStepUI->showAgents, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::showAgentsChanged);

    m_buildStepUI->showTime->setChecked(m_buildStep->showTime());
    connect(m_buildStepUI->showTime, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::showTimeChanged);

    m_buildStepUI->hideHeader->setChecked(m_buildStep->hideHeader());
    connect(m_buildStepUI->hideHeader, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::hideHeaderChanged);

    m_buildStepUI->logLevel->addItems(m_buildStep->supportedLogLevels());
    m_buildStepUI->logLevel->setCurrentText(m_buildStep->logLevel());
    connect(m_buildStepUI->logLevel, &QComboBox::currentTextChanged, this, &BuildConsoleStepConfigWidget::logLevelChanged);

    m_buildStepUI->setEnvEdit->setText(m_buildStep->setEnv());
    connect(m_buildStepUI->setEnvEdit, &QLineEdit::textEdited, this, &BuildConsoleStepConfigWidget::setEnvChanged);

    m_buildStepUI->stopOnError->setChecked(m_buildStep->stopOnError());
    connect(m_buildStepUI->stopOnError, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::stopOnErrorChanged);

    m_buildStepUI->additionalArgsEdit->setText(m_buildStep->additionalArguments());
    connect(m_buildStepUI->additionalArgsEdit, &QLineEdit::textEdited, this, &BuildConsoleStepConfigWidget::additionalArgsChanged);

    m_buildStepUI->openMonitor->setChecked(m_buildStep->openMonitor());
    connect(m_buildStepUI->openMonitor, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::openMonitorChanged);

    m_buildStepUI->keepJobsNum->setChecked(m_buildStep->keepJobNum());
    connect(m_buildStepUI->keepJobsNum, &QCheckBox::stateChanged, this, &BuildConsoleStepConfigWidget::keepJobNumChanged);
}

BuildConsoleStepConfigWidget::~BuildConsoleStepConfigWidget()
{
    delete m_buildStepUI;
    m_buildStepUI = nullptr;
}

QString BuildConsoleStepConfigWidget::displayName() const
{
    return tr("IncrediBuild for Windows");
}

QString BuildConsoleStepConfigWidget::summaryText() const
{
    return "<b>" + displayName() + "</b>";
}

void BuildConsoleStepConfigWidget::avoidLocalChanged()
{
    m_buildStep->avoidLocal(m_buildStepUI->avoidLocal->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::profileXmlEdited()
{
    m_buildStep->profileXml(m_buildStepUI->profileXmlPathChooser->rawPath());
}

void BuildConsoleStepConfigWidget::maxCpuChanged(int)
{
    m_buildStep->maxCpu(m_buildStepUI->maxCpuSpin->value());
}

void BuildConsoleStepConfigWidget::maxWinVerChanged(const QString &)
{
    m_buildStep->maxWinVer(m_buildStepUI->newestWindowsOs->currentText());
}

void BuildConsoleStepConfigWidget::minWinVerChanged(const QString &)
{
    m_buildStep->minWinVer(m_buildStepUI->oldestWindowsOs->currentText());
}

void BuildConsoleStepConfigWidget::titleEdited(const QString &)
{
    m_buildStep->title(m_buildStepUI->titleEdit->text());
}

void BuildConsoleStepConfigWidget::monFileEdited()
{
    m_buildStep->monFile(m_buildStepUI->monFilePathChooser->rawPath());
}

void BuildConsoleStepConfigWidget::suppressStdOutChanged()
{
    m_buildStep->suppressStdOut(m_buildStepUI->suppressStdOut->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::logFileEdited()
{
    m_buildStep->logFile(m_buildStepUI->logFilePathChooser->rawPath());
}

void BuildConsoleStepConfigWidget::showCmdChanged()
{
    m_buildStep->showCmd(m_buildStepUI->showCmd->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::showAgentsChanged()
{
    m_buildStep->showAgents(m_buildStepUI->showAgents->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::showTimeChanged()
{
    m_buildStep->showTime(m_buildStepUI->showTime->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::hideHeaderChanged()
{
    m_buildStep->hideHeader(m_buildStepUI->hideHeader->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::logLevelChanged(const QString&)
{
    m_buildStep->logLevel(m_buildStepUI->logLevel->currentText());
}

void BuildConsoleStepConfigWidget::setEnvChanged(const QString&)
{
    m_buildStep->setEnv(m_buildStepUI->setEnvEdit->text());
}

void BuildConsoleStepConfigWidget::stopOnErrorChanged()
{
    m_buildStep->stopOnError(m_buildStepUI->stopOnError->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::additionalArgsChanged(const QString&)
{
    m_buildStep->additionalArguments(m_buildStepUI->additionalArgsEdit->text());
}

void BuildConsoleStepConfigWidget::openMonitorChanged()
{
    m_buildStep->openMonitor(m_buildStepUI->openMonitor->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::keepJobNumChanged()
{
    m_buildStep->keepJobNum(m_buildStepUI->keepJobsNum->checkState() == Qt::CheckState::Checked);
}

void BuildConsoleStepConfigWidget::commandBuilderChanged(const QString &)
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
        m_buildStepUI->makeArgumentsLineEdit->setText(QString());

    QString command, defaultCommand;
    defaultCommand = m_buildStep->commandBuilder()->defaultCommand();
    m_buildStepUI->makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    command = m_buildStep->commandBuilder()->command();
    if (command != defaultCommand)
        m_buildStepUI->makePathChooser->setPath(command);
    else
        m_buildStepUI->makePathChooser->setPath("");
}

void BuildConsoleStepConfigWidget::commandArgsChanged(const QString &)
{
    m_buildStep->commandBuilder()->arguments(m_buildStepUI->makeArgumentsLineEdit->text());
}

void BuildConsoleStepConfigWidget::makePathEdited()
{
    m_buildStep->commandBuilder()->command(m_buildStepUI->makePathChooser->rawPath());
}

} // namespace Internal
} // namespace IncrediBuild
