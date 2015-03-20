/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

IosPresetBuildStep::~IosPresetBuildStep()
{
}

bool IosPresetBuildStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
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

    return AbstractProcessStep::init();
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

    connect(m_ui->argumentsTextEdit, SIGNAL(textChanged()),
            SLOT(argumentsChanged()));
    connect(m_ui->commandLineEdit, SIGNAL(editingFinished()),
            SLOT(commandChanged()));
    connect(m_ui->resetDefaultsButton, SIGNAL(clicked()),
            SLOT(resetDefaults()));

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            SLOT(updateDetails()));
    connect(m_buildStep->target(), SIGNAL(kitChanged()),
            SLOT(updateDetails()));
    connect(pro, SIGNAL(environmentChanged()),
            SLOT(updateDetails()));
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
    if (!canCreate(parent, id))
        return 0;
    IosPresetBuildStep *step = createPresetStep(parent, id);
    if (step->completeSetup())
        return step;
    delete step;
    return 0;
}

bool IosPresetBuildStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *IosPresetBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    IosPresetBuildStep *old = qobject_cast<IosPresetBuildStep *>(source);
    Q_ASSERT(old);
    IosPresetBuildStep *res = createPresetStep(parent, old->id());
    if (res->completeSetupWithStep(old))
        return res;
    delete res;
    return 0;
}

bool IosPresetBuildStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *IosPresetBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    IosPresetBuildStep *bs = createPresetStep(parent, idFromMap(map));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QString IosDsymBuildStepFactory::displayNameForId(const Id id) const
{
    if (id == Constants::IOS_DSYM_BUILD_STEP_ID)
        return QLatin1String("dsymutil");
    return QString();
}

QList<Id> IosDsymBuildStepFactory::availableCreationIds(BuildStepList *parent) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return QList<Id>();
    Kit *kit = parent->target()->kit();
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    if (deviceType == Constants::IOS_DEVICE_TYPE
            || deviceType == Constants::IOS_SIMULATOR_TYPE)
        return QList<Id>() << Id(Constants::IOS_DSYM_BUILD_STEP_ID);
    return QList<Id>();
}

bool IosDsymBuildStepFactory::canCreate(BuildStepList *parent, const Id id) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return false;
    Kit *kit = parent->target()->kit();
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    return ((deviceType == Constants::IOS_DEVICE_TYPE
            || deviceType == Constants::IOS_SIMULATOR_TYPE)
            && id == Constants::IOS_DSYM_BUILD_STEP_ID);
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
    QTC_ASSERT(runConf, return QStringList(QLatin1String("echo")));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(QLatin1String(".dSYM"));
    return QStringList()
            << QLatin1String("rm")
            << QLatin1String("-rf")
            << dsymPath;
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
    return QStringList()
            << dsymutilCmd
            << QLatin1String("-o")
            << dsymPath
            << runConf->localExecutable().toUserOutput();
}


} // namespace Internal
} // namespace Ios
