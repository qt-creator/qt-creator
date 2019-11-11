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

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/gcctoolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>

#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios {
namespace Internal {

const char IOS_BUILD_STEP_ID[] = "Ios.IosBuildStep";
const char IOS_BUILD_STEP_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("Ios::Internal::IosBuildStep",
                                                             "xcodebuild");

const char BUILD_USE_DEFAULT_ARGS_KEY[] = "Ios.IosBuildStep.XcodeArgumentsUseDefault";
const char BUILD_ARGUMENTS_KEY[] = "Ios.IosBuildStep.XcodeArguments";
const char CLEAN_KEY[] = "Ios.IosBuildStep.Clean";

//
// IosBuildStepConfigWidget
//

class IosBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    IosBuildStepConfigWidget(IosBuildStep *buildStep)
        : BuildStepConfigWidget(buildStep), m_buildStep(buildStep)
    {
        auto buildArgumentsLabel = new QLabel(this);
        buildArgumentsLabel->setText(IosBuildStep::tr("Base arguments:"));

        m_buildArgumentsTextEdit = new QPlainTextEdit(this);
        m_buildArgumentsTextEdit->setPlainText(QtcProcess::joinArgs(m_buildStep->baseArguments()));

        m_resetDefaultsButton = new QPushButton(this);
        m_resetDefaultsButton->setLayoutDirection(Qt::RightToLeft);
        m_resetDefaultsButton->setText(IosBuildStep::tr("Reset Defaults"));
        m_resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);

        auto extraArgumentsLabel = new QLabel(this);

        m_extraArgumentsLineEdit = new QLineEdit(this);
        m_extraArgumentsLineEdit->setText(QtcProcess::joinArgs(m_buildStep->m_extraArguments));

        auto gridLayout = new QGridLayout(this);
        gridLayout->addWidget(buildArgumentsLabel, 0, 0, 1, 1);
        gridLayout->addWidget(m_buildArgumentsTextEdit, 0, 1, 2, 1);
        gridLayout->addWidget(m_resetDefaultsButton, 1, 2, 1, 1);
        gridLayout->addWidget(extraArgumentsLabel, 2, 0, 1, 1);
        gridLayout->addWidget(m_extraArgumentsLineEdit, 2, 1, 1, 1);

        extraArgumentsLabel->setText(IosBuildStep::tr("Extra arguments:"));

        setDisplayName(IosBuildStep::tr("iOS build", "iOS BuildStep display name."));

        updateDetails();

        connect(m_buildArgumentsTextEdit, &QPlainTextEdit::textChanged,
                this, &IosBuildStepConfigWidget::buildArgumentsChanged);
        connect(m_resetDefaultsButton, &QAbstractButton::clicked,
                this, &IosBuildStepConfigWidget::resetDefaultArguments);
        connect(m_extraArgumentsLineEdit, &QLineEdit::editingFinished,
                this, &IosBuildStepConfigWidget::extraArgumentsChanged);

        connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
                this, &IosBuildStepConfigWidget::updateDetails);
        connect(m_buildStep->target(), &Target::kitChanged,
                this, &IosBuildStepConfigWidget::updateDetails);

        connect(m_buildStep->buildConfiguration(), &BuildConfiguration::environmentChanged,
                this, &IosBuildStepConfigWidget::updateDetails);
    }

private:
    void buildArgumentsChanged()
    {
        m_buildStep->setBaseArguments(QtcProcess::splitArgs(m_buildArgumentsTextEdit->toPlainText()));
        m_resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);
        updateDetails();
    }

    void resetDefaultArguments()
    {
        m_buildStep->setBaseArguments(m_buildStep->defaultArguments());
        m_buildArgumentsTextEdit->setPlainText(QtcProcess::joinArgs(m_buildStep->baseArguments()));
        m_resetDefaultsButton->setEnabled(!m_buildStep->m_useDefaultArguments);
    }

    void extraArgumentsChanged()
    {
        m_buildStep->setExtraArguments(QtcProcess::splitArgs(m_extraArgumentsLineEdit->text()));
    }

    void updateDetails()
    {
        BuildConfiguration *bc = m_buildStep->buildConfiguration();

        ProcessParameters param;
        param.setMacroExpander(bc->macroExpander());
        param.setWorkingDirectory(bc->buildDirectory());
        param.setEnvironment(bc->environment());
        param.setCommandLine({m_buildStep->buildCommand(), m_buildStep->allArguments()});

        setSummaryText(param.summary(displayName()));
    }

    IosBuildStep *m_buildStep;

    QPlainTextEdit *m_buildArgumentsTextEdit;
    QPushButton *m_resetDefaultsButton;
    QLineEdit *m_extraArgumentsLineEdit;
};

IosBuildStep::IosBuildStep(BuildStepList *parent) :
    AbstractProcessStep(parent, IOS_BUILD_STEP_ID)
{
    setDefaultDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::IosBuildStep",
                                                      IOS_BUILD_STEP_DISPLAY_NAME));
    if (parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        m_clean = true;
        setExtraArguments(QStringList("clean"));
    }
}

bool IosBuildStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitAspect::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommandLine({buildCommand(), allArguments()});

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

QVariantMap IosBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(BUILD_ARGUMENTS_KEY, m_baseBuildArguments);
    map.insert(BUILD_USE_DEFAULT_ARGS_KEY, m_useDefaultArguments);
    map.insert(CLEAN_KEY, m_clean);
    return map;
}

bool IosBuildStep::fromMap(const QVariantMap &map)
{
    QVariant bArgs = map.value(BUILD_ARGUMENTS_KEY);
    m_baseBuildArguments = bArgs.toStringList();
    m_useDefaultArguments = map.value(BUILD_USE_DEFAULT_ARGS_KEY).toBool();
    m_clean = map.value(CLEAN_KEY).toBool();

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
    ToolChain *tc = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    switch (buildConfiguration()->buildType()) {
    case BuildConfiguration::Debug :
        res << "-configuration" << "Debug";
        break;
    case BuildConfiguration::Release :
    case BuildConfiguration::Profile :
        res << "-configuration" << "Release";
        break;
    case BuildConfiguration::Unknown :
        break;
    default:
        qCWarning(iosLog) << "IosBuildStep had an unknown buildType "
                          << buildConfiguration()->buildType();
    }
    if (tc->typeId() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
            || tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        auto gtc = static_cast<GccToolChain *>(tc);
        res << gtc->platformCodeGenFlags();
    }
    if (!SysRootKitAspect::sysRoot(kit).isEmpty())
        res << "-sdk" << SysRootKitAspect::sysRoot(kit).toString();
    res << "SYMROOT=" + buildConfiguration()->buildDirectory().toString();
    return res;
}

FilePath IosBuildStep::buildCommand() const
{
    return FilePath::fromString("xcodebuild"); // add path?
}

void IosBuildStep::doRun()
{
    AbstractProcessStep::doRun();
}

BuildStepConfigWidget *IosBuildStep::createConfigWidget()
{
    return new IosBuildStepConfigWidget(this);
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
// IosBuildStepFactory
//

IosBuildStepFactory::IosBuildStepFactory()
{
    registerStep<IosBuildStep>(IOS_BUILD_STEP_ID);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE,
                             Constants::IOS_SIMULATOR_TYPE});
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_CLEAN,
                           ProjectExplorer::Constants::BUILDSTEPS_BUILD});
    setDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::IosBuildStep",
                                               IOS_BUILD_STEP_DISPLAY_NAME));
}

} // namespace Internal
} // namespace Ios
