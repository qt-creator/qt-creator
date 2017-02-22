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
#include "iosmanager.h"
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

IosPresetBuildStep::IosPresetBuildStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id),
    m_clean(parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
{
}

bool IosPresetBuildStep::completeSetup()
{
    m_command = defaultCommand();
    m_arguments = defaultArguments();
    return true;
}

bool IosPresetBuildStep::completeSetupWithStep(BuildStep *bs)
{
    IosPresetBuildStep *o = qobject_cast<IosPresetBuildStep *>(bs);
    if (!o)
        return false;
    m_arguments = o->m_arguments;
    m_clean = o->m_clean;
    m_command = o->m_command;
    return true;
}

bool IosPresetBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

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

QVariantMap IosPresetBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(id().withSuffix(QLatin1String(ARGUMENTS_PARTIAL_KEY)).toString(),
               arguments());
    map.insert(id().withSuffix(QLatin1String(USE_DEFAULT_ARGS_PARTIAL_KEY)).toString(),
               isDefault());
    map.insert(id().withSuffix(QLatin1String(CLEAN_PARTIAL_KEY)).toString(), m_clean);
    map.insert(id().withSuffix(QLatin1String(COMMAND_PARTIAL_KEY)).toString(), command());
    return map;
}

bool IosPresetBuildStep::fromMap(const QVariantMap &map)
{
    QVariant bArgs = map.value(id().withSuffix(QLatin1String(ARGUMENTS_PARTIAL_KEY)).toString());
    m_arguments = bArgs.toStringList();
    bool useDefaultArguments = map.value(
                id().withSuffix(QLatin1String(USE_DEFAULT_ARGS_PARTIAL_KEY)).toString()).toBool();
    m_clean = map.value(id().withSuffix(QLatin1String(CLEAN_PARTIAL_KEY)).toString(), m_clean).toBool();
    m_command = map.value(id().withSuffix(QLatin1String(COMMAND_PARTIAL_KEY)).toString(), m_command)
            .toString();
    if (useDefaultArguments) {
        m_command = defaultCommand();
        m_arguments = defaultArguments();
    }

    return BuildStep::fromMap(map);
}

QStringList IosPresetBuildStep::defaultArguments() const
{
    if (m_clean)
        return defaultCleanCmdList().mid(1);
    return defaultCmdList().mid(1);
}

QString IosPresetBuildStep::defaultCommand() const
{
    if (m_clean)
        return defaultCleanCmdList().at(0);
    else
        return defaultCmdList().at(0);
}

QString IosPresetBuildStep::command() const
{
    if (m_command.isEmpty())
        return defaultCommand();
    return m_command;
}

void IosPresetBuildStep::setCommand(const QString &command)
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

bool IosPresetBuildStep::clean() const
{
    return m_clean;
}

void IosPresetBuildStep::setClean(bool clean)
{
    if (m_clean != clean) {
        m_clean = clean;
        m_arguments = defaultArguments();
        m_command = defaultCommand();
    }
}

bool IosPresetBuildStep::isDefault() const
{
    return arguments() == defaultArguments() && command() == defaultCommand();
}

void IosPresetBuildStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *IosPresetBuildStep::createConfigWidget()
{
    return new IosPresetBuildStepConfigWidget(this);
}

bool IosPresetBuildStep::immutable() const
{
    return false;
}

void IosPresetBuildStep::setArguments(const QStringList &args)
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

QStringList IosPresetBuildStep::arguments() const
{
    if (m_command.isEmpty())
        return defaultArguments();
    return m_arguments;
}

//
// IosPresetBuildStepConfigWidget
//

IosPresetBuildStepConfigWidget::IosPresetBuildStepConfigWidget(IosPresetBuildStep *buildStep)
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
            this, &IosPresetBuildStepConfigWidget::argumentsChanged);
    connect(m_ui->commandLineEdit, &QLineEdit::editingFinished,
            this, &IosPresetBuildStepConfigWidget::commandChanged);
    connect(m_ui->resetDefaultsButton, &QAbstractButton::clicked,
            this, &IosPresetBuildStepConfigWidget::resetDefaults);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &IosPresetBuildStepConfigWidget::updateDetails);
    connect(m_buildStep->target(), &Target::kitChanged,
            this, &IosPresetBuildStepConfigWidget::updateDetails);
    connect(pro, &Project::environmentChanged, this, &IosPresetBuildStepConfigWidget::updateDetails);
}

IosPresetBuildStepConfigWidget::~IosPresetBuildStepConfigWidget()
{
    delete m_ui;
}

QString IosPresetBuildStepConfigWidget::displayName() const
{
    return m_buildStep->displayName();
}

void IosPresetBuildStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_buildStep->buildConfiguration();
    if (!bc)
        bc = m_buildStep->target()->activeBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_buildStep->command());
    param.setArguments(Utils::QtcProcess::joinArgs(m_buildStep->arguments()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString IosPresetBuildStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void IosPresetBuildStepConfigWidget::commandChanged()
{
    m_buildStep->setCommand(m_ui->commandLineEdit->text());
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();
}

void IosPresetBuildStepConfigWidget::argumentsChanged()
{
    m_buildStep->setArguments(Utils::QtcProcess::splitArgs(
                                      m_ui->argumentsTextEdit->toPlainText()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->isDefault());
    updateDetails();
}

void IosPresetBuildStepConfigWidget::resetDefaults()
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
// IosPresetBuildStepFactory
//

IosPresetBuildStepFactory::IosPresetBuildStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

BuildStep *IosPresetBuildStepFactory::create(BuildStepList *parent, const Id id)
{
    IosPresetBuildStep *step = createPresetStep(parent, id);
    if (step->completeSetup())
        return step;
    delete step;
    return 0;
}

BuildStep *IosPresetBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    IosPresetBuildStep *old = qobject_cast<IosPresetBuildStep *>(source);
    Q_ASSERT(old);
    IosPresetBuildStep *res = createPresetStep(parent, old->id());
    if (res->completeSetupWithStep(old))
        return res;
    delete res;
    return 0;
}

BuildStep *IosPresetBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    IosPresetBuildStep *bs = createPresetStep(parent, idFromMap(map));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<BuildStepInfo> IosDsymBuildStepFactory::availableSteps(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return {};

    Id deviceType = DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());
    if (deviceType != Constants::IOS_DEVICE_TYPE && deviceType != Constants::IOS_SIMULATOR_TYPE)
        return {};

    return {{Constants::IOS_DSYM_BUILD_STEP_ID, "dsymutil"}};
}

IosPresetBuildStep *IosDsymBuildStepFactory::createPresetStep(BuildStepList *parent, const Id id) const
{
    return new IosDsymBuildStep(parent, id);
}

IosDsymBuildStep::IosDsymBuildStep(BuildStepList *parent, const Id id)
    : IosPresetBuildStep(parent, id)
{
    setDefaultDisplayName(QLatin1String("dsymutil"));
}

QStringList IosDsymBuildStep::defaultCleanCmdList() const
{
    IosRunConfiguration *runConf =
            qobject_cast<IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return QStringList("echo"));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(QLatin1String(".dSYM"));
    return QStringList({"rm", "-rf", dsymPath});
}

QStringList IosDsymBuildStep::defaultCmdList() const
{
    QString dsymutilCmd = QLatin1String("dsymutil");
    Utils::FileName dsymUtilPath = IosConfigurations::developerPath()
            .appendPath(QLatin1String("Toolchains/XcodeDefault.xctoolchain/usr/bin/dsymutil"));
    if (dsymUtilPath.exists())
        dsymutilCmd = dsymUtilPath.toUserOutput();
    IosRunConfiguration *runConf =
            qobject_cast<IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return QStringList(QLatin1String("echo")));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(QLatin1String(".dSYM"));
    return QStringList({dsymutilCmd, "-o", dsymPath, runConf->localExecutable().toUserOutput()});
}


} // namespace Internal
} // namespace Ios
