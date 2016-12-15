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

#include "iosbuildstep.h"
#include "iosconstants.h"
#include "ui_iosbuildstep.h"
#include "iosmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

const char IOS_BUILD_STEP_ID[] = "Ios.IosBuildStep";
const char IOS_BUILD_STEP_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("Ios::Internal::IosBuildStep",
                                                             "xcodebuild");

const char BUILD_USE_DEFAULT_ARGS_KEY[] = "Ios.IosBuildStep.XcodeArgumentsUseDefault";
const char BUILD_ARGUMENTS_KEY[] = "Ios.IosBuildStep.XcodeArguments";
const char CLEAN_KEY[] = "Ios.IosBuildStep.Clean";

IosBuildStep::IosBuildStep(BuildStepList *parent) :
    AbstractProcessStep(parent, Id(IOS_BUILD_STEP_ID)),
    m_useDefaultArguments(true),
    m_clean(false)
{
    ctor();
}

IosBuildStep::IosBuildStep(BuildStepList *parent, const Id id) :
    AbstractProcessStep(parent, id),
    m_useDefaultArguments(true),
    m_clean(false)
{
    ctor();
}

IosBuildStep::IosBuildStep(BuildStepList *parent, IosBuildStep *bs) :
    AbstractProcessStep(parent, bs),
    m_baseBuildArguments(bs->m_baseBuildArguments),
    m_useDefaultArguments(bs->m_useDefaultArguments),
    m_clean(bs->m_clean)
{
    ctor();
}

void IosBuildStep::ctor()
{
    setDefaultDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::IosBuildStep",
                                                      IOS_BUILD_STEP_DISPLAY_NAME));
}

bool IosBuildStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommand(buildCommand());
    pp->setArguments(Utils::QtcProcess::joinArgs(allArguments()));
    pp->resolveAll();

    // If we are cleaning, then build can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void IosBuildStep::setClean(bool clean)
{
    m_clean = clean;
}

bool IosBuildStep::isClean() const
{
    return m_clean;
}

QVariantMap IosBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(QLatin1String(BUILD_ARGUMENTS_KEY), m_baseBuildArguments);
    map.insert(QLatin1String(BUILD_USE_DEFAULT_ARGS_KEY), m_useDefaultArguments);
    map.insert(QLatin1String(CLEAN_KEY), m_clean);
    return map;
}

bool IosBuildStep::fromMap(const QVariantMap &map)
{
    QVariant bArgs = map.value(QLatin1String(BUILD_ARGUMENTS_KEY));
    m_baseBuildArguments = bArgs.toStringList();
    m_useDefaultArguments = map.value(QLatin1String(BUILD_USE_DEFAULT_ARGS_KEY)).toBool();
    m_clean = map.value(QLatin1String(CLEAN_KEY)).toBool();

    return BuildStep::fromMap(map);
}

QStringList IosBuildStep::allArguments() const
{
    return baseArguments() + m_extraArguments;
}

QStringList IosBuildStep::defaultArguments() const
{
    QStringList res;
    Kit *kit = target()->kit();
    ToolChain *tc = ToolChainKitInformation::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    switch (target()->activeBuildConfiguration()->buildType()) {
    case BuildConfiguration::Debug :
        res << QLatin1String("-configuration") << QLatin1String("Debug");
        break;
    case BuildConfiguration::Release :
    case BuildConfiguration::Profile :
        res << QLatin1String("-configuration") << QLatin1String("Release");
        break;
    case BuildConfiguration::Unknown :
        break;
    default:
        qCWarning(iosLog) << "IosBuildStep had an unknown buildType "
                          << target()->activeBuildConfiguration()->buildType();
    }
    if (tc->typeId() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
            || tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        GccToolChain *gtc = static_cast<GccToolChain *>(tc);
        res << gtc->platformCodeGenFlags();
    }
    if (!SysRootKitInformation::sysRoot(kit).isEmpty())
        res << QLatin1String("-sdk") << SysRootKitInformation::sysRoot(kit).toString();
    res << QLatin1String("SYMROOT=") + IosManager::resDirForTarget(target());
    return res;
}

QString IosBuildStep::buildCommand() const
{
    return QLatin1String("xcodebuild"); // add path?
}

void IosBuildStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *IosBuildStep::createConfigWidget()
{
    return new IosBuildStepConfigWidget(this);
}

bool IosBuildStep::immutable() const
{
    return false;
}

void IosBuildStep::setBaseArguments(const QStringList &args)
{
    m_baseBuildArguments = args;
    m_useDefaultArguments = (args == defaultArguments());
}

void IosBuildStep::setExtraArguments(const QStringList &extraArgs)
{
    m_extraArguments = extraArgs;
}

QStringList IosBuildStep::baseArguments() const
{
    if (m_useDefaultArguments)
        return defaultArguments();
    return m_baseBuildArguments;
}

//
// IosBuildStepConfigWidget
//

IosBuildStepConfigWidget::IosBuildStepConfigWidget(IosBuildStep *buildStep)
    : m_buildStep(buildStep)
{
    m_ui = new Ui::IosBuildStep;
    m_ui->setupUi(this);

    Project *pro = m_buildStep->target()->project();

    m_ui->buildArgumentsTextEdit->setPlainText(Utils::QtcProcess::joinArgs(
                                                   m_buildStep->baseArguments()));
    m_ui->extraArgumentsLineEdit->setText(Utils::QtcProcess::joinArgs(
                                              m_buildStep->m_extraArguments));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);
    updateDetails();

    connect(m_ui->buildArgumentsTextEdit, &QPlainTextEdit::textChanged,
            this, &IosBuildStepConfigWidget::buildArgumentsChanged);
    connect(m_ui->resetDefaultsButton, &QAbstractButton::clicked,
            this, &IosBuildStepConfigWidget::resetDefaultArguments);
    connect(m_ui->extraArgumentsLineEdit, &QLineEdit::editingFinished,
            this, &IosBuildStepConfigWidget::extraArgumentsChanged);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &IosBuildStepConfigWidget::updateDetails);
    connect(m_buildStep->target(), &Target::kitChanged,
            this, &IosBuildStepConfigWidget::updateDetails);
    connect(pro, &Project::environmentChanged, this, &IosBuildStepConfigWidget::updateDetails);
}

IosBuildStepConfigWidget::~IosBuildStepConfigWidget()
{
    delete m_ui;
}

QString IosBuildStepConfigWidget::displayName() const
{
    return tr("iOS build", "iOS BuildStep display name.");
}

void IosBuildStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_buildStep->buildConfiguration();
    if (!bc)
        bc = m_buildStep->target()->activeBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_buildStep->buildCommand());
    param.setArguments(Utils::QtcProcess::joinArgs(m_buildStep->allArguments()));
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString IosBuildStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void IosBuildStepConfigWidget::buildArgumentsChanged()
{
    m_buildStep->setBaseArguments(Utils::QtcProcess::splitArgs(
                                      m_ui->buildArgumentsTextEdit->toPlainText()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);
    updateDetails();
}

void IosBuildStepConfigWidget::resetDefaultArguments()
{
    m_buildStep->setBaseArguments(m_buildStep->defaultArguments());
    m_ui->buildArgumentsTextEdit->setPlainText(Utils::QtcProcess::joinArgs(
                                                   m_buildStep->baseArguments()));
    m_ui->resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);
}

void IosBuildStepConfigWidget::extraArgumentsChanged()
{
    m_buildStep->setExtraArguments(Utils::QtcProcess::splitArgs(
                                       m_ui->extraArgumentsLineEdit->text()));
}
//
// IosBuildStepFactory
//

IosBuildStepFactory::IosBuildStepFactory(QObject *parent) :
    IBuildStepFactory(parent)
{
}

BuildStep *IosBuildStepFactory::create(BuildStepList *parent, const Id id)
{
    Q_UNUSED(id);
    IosBuildStep *step = new IosBuildStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setExtraArguments(QStringList(QLatin1String("clean")));
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        // nomal setup
    }
    return step;
}

BuildStep *IosBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    IosBuildStep *old = qobject_cast<IosBuildStep *>(source);
    Q_ASSERT(old);
    return new IosBuildStep(parent, old);
}

QList<BuildStepInfo> IosBuildStepFactory::availableSteps(BuildStepList *parent) const
{
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(parent->target()->kit());
    if (deviceType != Constants::IOS_DEVICE_TYPE
            && deviceType != Constants::IOS_SIMULATOR_TYPE)
        return {};

    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return {};

    return {{ IOS_BUILD_STEP_ID,
              QCoreApplication::translate("GenericProjectManager::Internal::IosBuildStep",
                                         IOS_BUILD_STEP_DISPLAY_NAME) }};
}

} // namespace Internal
} // namespace Ios
