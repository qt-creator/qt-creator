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
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanager.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

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

IosBuildStep::~IosBuildStep()
{
}

bool IosBuildStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit());
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
    // Force output to english for the parsers. Do this here and not in the toolchain's
    // addToEnvironment() to not screw up the users run environment.
    env.set(QLatin1String("LC_ALL"), QLatin1String("C"));
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

    return AbstractProcessStep::init();
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
    ToolChain *tc = ToolChainKitInformation::toolChain(kit);
    switch (target()->activeBuildConfiguration()->buildType()) {
    case BuildConfiguration::Debug :
        res << QLatin1String("-configuration") << QLatin1String("Debug");
        break;
    case BuildConfiguration::Release :
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

    connect(m_ui->buildArgumentsTextEdit, SIGNAL(textChanged()),
            this, SLOT(buildArgumentsChanged()));
    connect(m_ui->resetDefaultsButton, SIGNAL(clicked()),
            this, SLOT(resetDefaultArguments()));
    connect(m_ui->extraArgumentsLineEdit, SIGNAL(editingFinished()),
            this, SLOT(extraArgumentsChanged()));

    connect(ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
            this, SLOT(updateDetails()));
    connect(m_buildStep->target(), SIGNAL(kitChanged()),
            this, SLOT(updateDetails()));
    connect(pro, SIGNAL(environmentChanged()),
            this, SLOT(updateDetails()));
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

bool IosBuildStepFactory::canCreate(BuildStepList *parent, const Id id) const
{
    if (parent->id() != ProjectExplorer::Constants::BUILDSTEPS_CLEAN
            && parent->id() != ProjectExplorer::Constants::BUILDSTEPS_BUILD)
        return false;
    Kit *kit = parent->target()->kit();
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    return ((deviceType == Constants::IOS_DEVICE_TYPE
            || deviceType == Constants::IOS_SIMULATOR_TYPE)
            && id == IOS_BUILD_STEP_ID);
}

BuildStep *IosBuildStepFactory::create(BuildStepList *parent, const Id id)
{
    if (!canCreate(parent, id))
        return 0;
    IosBuildStep *step = new IosBuildStep(parent);
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        step->setClean(true);
        step->setExtraArguments(QStringList(QLatin1String("clean")));
    } else if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_BUILD) {
        // nomal setup
    }
    return step;
}

bool IosBuildStepFactory::canClone(BuildStepList *parent, BuildStep *source) const
{
    return canCreate(parent, source->id());
}

BuildStep *IosBuildStepFactory::clone(BuildStepList *parent, BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    IosBuildStep *old(qobject_cast<IosBuildStep *>(source));
    Q_ASSERT(old);
    return new IosBuildStep(parent, old);
}

bool IosBuildStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *IosBuildStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    IosBuildStep *bs(new IosBuildStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Id> IosBuildStepFactory::availableCreationIds(BuildStepList *parent) const
{
    Kit *kit = parent->target()->kit();
    Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    if (deviceType == Constants::IOS_DEVICE_TYPE
            || deviceType == Constants::IOS_SIMULATOR_TYPE)
        return QList<Id>() << Id(IOS_BUILD_STEP_ID);
    return QList<Id>();
}

QString IosBuildStepFactory::displayNameForId(const Id id) const
{
    if (id == IOS_BUILD_STEP_ID)
        return QCoreApplication::translate("GenericProjectManager::Internal::IosBuildStep",
                                           IOS_BUILD_STEP_DISPLAY_NAME);
    return QString();
}

} // namespace Internal
} // namespace Ios
