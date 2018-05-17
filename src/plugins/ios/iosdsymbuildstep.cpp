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

#include "iosdsymbuildstep.h"

#include "iosconstants.h"
#include "ui_iospresetbuildstep.h"
#include "iosconfigurations.h"
#include "iosrunconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

static const char USE_DEFAULT_ARGS_PARTIAL_KEY[] = ".ArgumentsUseDefault";
static const char COMMAND_PARTIAL_KEY[] = ".Command";
static const char ARGUMENTS_PARTIAL_KEY[] = ".Arguments";
static const char CLEAN_PARTIAL_KEY[] = ".Clean";

IosDsymBuildStep::IosDsymBuildStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Constants::IOS_DSYM_BUILD_STEP_ID),
    m_clean(parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
{
}

bool IosDsymBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommand(command());
    pp->setArguments(Utils::QtcProcess::joinArgs(arguments()));
    pp->resolveAll();

    // If we are cleaning, then build can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);

    setOutputParser(target()->kit()->createOutputParser());
    if (outputParser())
        outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

QVariantMap IosDsymBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(id().withSuffix(ARGUMENTS_PARTIAL_KEY).toString(),
               arguments());
    map.insert(id().withSuffix(USE_DEFAULT_ARGS_PARTIAL_KEY).toString(),
               isDefault());
    map.insert(id().withSuffix(CLEAN_PARTIAL_KEY).toString(), m_clean);
    map.insert(id().withSuffix(COMMAND_PARTIAL_KEY).toString(), command());
    return map;
}

bool IosDsymBuildStep::fromMap(const QVariantMap &map)
{
    QVariant bArgs = map.value(id().withSuffix(ARGUMENTS_PARTIAL_KEY).toString());
    m_arguments = bArgs.toStringList();
    bool useDefaultArguments = map.value(
                id().withSuffix(USE_DEFAULT_ARGS_PARTIAL_KEY).toString()).toBool();
    m_clean = map.value(id().withSuffix(CLEAN_PARTIAL_KEY).toString(), m_clean).toBool();
    m_command = map.value(id().withSuffix(COMMAND_PARTIAL_KEY).toString(), m_command)
            .toString();
    if (useDefaultArguments) {
        m_command = defaultCommand();
        m_arguments = defaultArguments();
    }

    return BuildStep::fromMap(map);
}

QStringList IosDsymBuildStep::defaultArguments() const
{
    if (m_clean)
        return defaultCleanCmdList().mid(1);
    return defaultCmdList().mid(1);
}

QString IosDsymBuildStep::defaultCommand() const
{
    if (m_clean)
        return defaultCleanCmdList().at(0);
    else
        return defaultCmdList().at(0);
}

QStringList IosDsymBuildStep::defaultCleanCmdList() const
{
    auto runConf = qobject_cast<IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return QStringList("echo"));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return QStringList({"rm", "-rf", dsymPath});
}

QStringList IosDsymBuildStep::defaultCmdList() const
{
    QString dsymutilCmd = "dsymutil";
    Utils::FileName dsymUtilPath = IosConfigurations::developerPath()
            .appendPath("Toolchains/XcodeDefault.xctoolchain/usr/bin/dsymutil");
    if (dsymUtilPath.exists())
        dsymutilCmd = dsymUtilPath.toUserOutput();
    IosRunConfiguration *runConf =
            qobject_cast<IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return QStringList("echo"));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return QStringList({dsymutilCmd, "-o", dsymPath, runConf->localExecutable().toUserOutput()});
}

QString IosDsymBuildStep::command() const
{
    if (m_command.isEmpty())
        return defaultCommand();
    return m_command;
}

void IosDsymBuildStep::setCommand(const QString &command)
{
    if (command == m_command)
        return;
    if (command.isEmpty() || command == defaultCommand()) {
        if (arguments() == defaultArguments())
            m_command.clear();
        else
            m_command = defaultCommand();
    } else if (m_command.isEmpty()) {
        m_arguments = defaultArguments();
        m_command = command;
    } else {
        m_command = command;
    }
}

bool IosDsymBuildStep::isDefault() const
{
    return arguments() == defaultArguments() && command() == defaultCommand();
}

void IosDsymBuildStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *IosDsymBuildStep::createConfigWidget()
{
    return new IosDsymBuildStepConfigWidget(this);
}

bool IosDsymBuildStep::immutable() const
{
    return false;
}

void IosDsymBuildStep::setArguments(const QStringList &args)
{
    if (arguments() == args)
        return;
    if (args == defaultArguments() && command() == defaultCommand())
        m_command.clear();
    else {
        if (m_command.isEmpty())
            m_command = defaultCommand();
        m_arguments = args;
    }
}

QStringList IosDsymBuildStep::arguments() const
{
    if (m_command.isEmpty())
        return defaultArguments();
    return m_arguments;
}

//
// IosDsymBuildStepConfigWidget
//

IosDsymBuildStepConfigWidget::IosDsymBuildStepConfigWidget(IosDsymBuildStep *buildStep)
    : m_buildStep(buildStep)
{
    m_ui = new Ui::IosPresetBuildStep;
    m_ui->setupUi(this);

    Project *pro = m_buildStep->target()->project();

    m_ui->commandLineEdit->setText(m_buildStep->command());
    m_ui->argumentsTextEdit->setPlainText(Utils::QtcProcess::joinArgs(
                                                   m_buildStep->arguments()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();

    connect(m_ui->argumentsTextEdit, &QPlainTextEdit::textChanged,
            this, &IosDsymBuildStepConfigWidget::argumentsChanged);
    connect(m_ui->commandLineEdit, &QLineEdit::editingFinished,
            this, &IosDsymBuildStepConfigWidget::commandChanged);
    connect(m_ui->resetDefaultsButton, &QAbstractButton::clicked,
            this, &IosDsymBuildStepConfigWidget::resetDefaults);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &IosDsymBuildStepConfigWidget::updateDetails);
    connect(m_buildStep->target(), &Target::kitChanged,
            this, &IosDsymBuildStepConfigWidget::updateDetails);
    pro->subscribeSignal(&BuildConfiguration::environmentChanged, this, [this]() {
        if (static_cast<BuildConfiguration *>(sender())->isActive())
            updateDetails();
    });
    connect(pro, &Project::activeProjectConfigurationChanged,
            this, [this](ProjectConfiguration *pc) {
        if (pc && pc->isActive())
            updateDetails();
    });
}

IosDsymBuildStepConfigWidget::~IosDsymBuildStepConfigWidget()
{
    delete m_ui;
}

QString IosDsymBuildStepConfigWidget::displayName() const
{
    return m_buildStep->displayName();
}

void IosDsymBuildStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_buildStep->buildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_buildStep->command());
    param.setArguments(Utils::QtcProcess::joinArgs(m_buildStep->arguments()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString IosDsymBuildStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void IosDsymBuildStepConfigWidget::commandChanged()
{
    m_buildStep->setCommand(m_ui->commandLineEdit->text());
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();
}

void IosDsymBuildStepConfigWidget::argumentsChanged()
{
    m_buildStep->setArguments(Utils::QtcProcess::splitArgs(
                                      m_ui->argumentsTextEdit->toPlainText()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();
}

void IosDsymBuildStepConfigWidget::resetDefaults()
{
    m_buildStep->setCommand(m_buildStep->defaultCommand());
    m_buildStep->setArguments(m_buildStep->defaultArguments());
    m_ui->commandLineEdit->setText(m_buildStep->command());
    m_ui->argumentsTextEdit->setPlainText(Utils::QtcProcess::joinArgs(
                                              m_buildStep->arguments()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();
}

//
// IosDsymBuildStepFactory
//

IosDsymBuildStepFactory::IosDsymBuildStepFactory()
{
    registerStep<IosDsymBuildStep>(Constants::IOS_DSYM_BUILD_STEP_ID);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE,
                             Constants::IOS_SIMULATOR_TYPE});
    setDisplayName("dsymutil");
}

} // namespace Internal
} // namespace Ios
