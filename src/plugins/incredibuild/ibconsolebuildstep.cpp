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

#include "ibconsolebuildstep.h"

#include "commandbuilder.h"
#include "commandbuilderaspect.h"
#include "incredibuildconstants.h"

#include <coreplugin/variablechooser.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QLabel>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild {
namespace Internal {

namespace Constants {
const QLatin1String IBCONSOLE_NICE("IncrediBuild.IBConsole.Nice");
const QLatin1String IBCONSOLE_COMMANDBUILDER("IncrediBuild.IBConsole.CommandBuilder");
const QLatin1String IBCONSOLE_KEEPJOBNUM("IncrediBuild.IBConsole.KeepJobNum");
const QLatin1String IBCONSOLE_FORCEREMOTE("IncrediBuild.IBConsole.ForceRemote");
const QLatin1String IBCONSOLE_ALTERNATE("IncrediBuild.IBConsole.Alternate");
}

class IBConsoleBuildStep;

class IBConsoleStepConfigWidget : public BuildStepConfigWidget
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::IBConsoleBuildStep)

public:
    explicit IBConsoleStepConfigWidget(IBConsoleBuildStep *ibConsoleStep);
};

class IBConsoleBuildStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::IBConsoleBuildStep)

public:
    IBConsoleBuildStep(BuildStepList *buildStepList, Id id);

    bool init() final;
    BuildStepConfigWidget *createConfigWidget() final;
    void setupOutputFormatter(OutputFormatter *formatter) final;

public:
    CommandBuilderAspect *m_commandBuilder;
    BaseIntegerAspect *m_nice{nullptr};
    BaseBoolAspect *m_keepJobNum{nullptr};
    BaseBoolAspect *m_forceRemote{nullptr};
    BaseBoolAspect *m_alternate{nullptr};

    BuildStepList *m_earlierSteps{};
};

IBConsoleStepConfigWidget::IBConsoleStepConfigWidget(IBConsoleBuildStep *ibConsoleStep)
    : BuildStepConfigWidget(ibConsoleStep)
{
    setDisplayName(tr("IncrediBuild for Linux"));
    setSummaryText("<b>" + displayName() + "</b>");

    QFont font;
    font.setBold(true);
    font.setWeight(75);

    auto section1 = new QLabel("Target and configuration", this);
    section1->setFont(font);

    auto section2 = new QLabel(this);
    section2->setText(tr("IncrediBuild Distribution control"));
    section2->setFont(font);

    const auto emphasize = [](const QString &msg) { return QString("<i>" + msg); };
    auto infoLabel1 = new QLabel(emphasize(tr("Enter the appropriate arguments to your build "
                                              "command.")), this);
    auto infoLabel2 = new QLabel(emphasize(tr("Make sure the build command's "
                                              "multi-job parameter value is large enough (such as "
                                               "-j200 for the JOM or Make build tools)")), this);

    LayoutBuilder builder(this);
    builder.addRow(section1);
    builder.addRow(ibConsoleStep->m_commandBuilder);
    builder.addRow(infoLabel1);
    builder.addRow(infoLabel2);
    builder.addRow(ibConsoleStep->m_keepJobNum);

    builder.addRow(section2);
    builder.addRow(ibConsoleStep->m_nice);
    builder.addRow(ibConsoleStep->m_forceRemote);
    builder.addRow(ibConsoleStep->m_alternate);

    Core::VariableChooser::addSupportForChildWidgets(this, ibConsoleStep->macroExpander());
}

// IBConsoleBuildStep

IBConsoleBuildStep::IBConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
    , m_earlierSteps(buildStepList)
{
    setDisplayName("IncrediBuild for Linux");

    m_commandBuilder = addAspect<CommandBuilderAspect>(this);
    m_commandBuilder->setSettingsKey(Constants::IBCONSOLE_COMMANDBUILDER);

    m_nice = addAspect<BaseIntegerAspect>();
    m_nice->setSettingsKey(Constants::IBCONSOLE_NICE);
    m_nice->setToolTip(tr("Specify nice value. Nice Value should be numeric and between -20 and 19"));
    m_nice->setLabel(tr("Nice value:"));
    m_nice->setRange(-20, 19);

    m_keepJobNum = addAspect<BaseBoolAspect>();
    m_keepJobNum->setSettingsKey(Constants::IBCONSOLE_KEEPJOBNUM);
    m_keepJobNum->setLabel(tr("Keep Original Jobs Num:"));
    m_keepJobNum->setToolTip(tr("Setting this option to true, forces IncrediBuild to not override "
                                "the -j command line switch. The default IncrediBuild behavior is "
                                "to set a high value to the -j command line switch which controls "
                                "the number of processes that the build tools executed by Qt will "
                                "execute in parallel (the default IncrediBuild behavior will set "
                                "this value to 200)."));

    m_forceRemote = addAspect<BaseBoolAspect>();
    m_forceRemote->setSettingsKey(Constants::IBCONSOLE_ALTERNATE);
    m_forceRemote->setLabel(tr("Force remote:"));

    m_alternate = addAspect<BaseBoolAspect>();
    m_alternate->setSettingsKey(Constants::IBCONSOLE_FORCEREMOTE);
    m_alternate->setLabel(tr("Alternate tasks preference:"));
}

void IBConsoleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

bool IBConsoleBuildStep::init()
{
    QStringList args;

    if (m_nice->value() != 0)
        args.append(QString("--nice %0 ").arg(m_nice->value()));

    if (m_alternate->value())
        args.append("--alternate");

    if (m_forceRemote->value())
        args.append("--force-remote");

    CommandBuilder *commandBuilder = m_commandBuilder->commandBuilder();
    commandBuilder->keepJobNum(m_keepJobNum->value());
    args.append(commandBuilder->fullCommandFlag());

    CommandLine cmdLine("ib_console", args);
    ProcessParameters *procParams = processParameters();
    procParams->setCommandLine(cmdLine);
    procParams->setEnvironment(Environment::systemEnvironment());

    BuildConfiguration *buildConfig = buildConfiguration();
    if (buildConfig) {
        procParams->setWorkingDirectory(buildConfig->buildDirectory());
        procParams->setEnvironment(buildConfig->environment());
        procParams->setMacroExpander(buildConfig->macroExpander());
    }

    return AbstractProcessStep::init();
}

BuildStepConfigWidget *IBConsoleBuildStep::createConfigWidget()
{
    return new IBConsoleStepConfigWidget(this);
}


// IBConsoleStepFactory

IBConsoleStepFactory::IBConsoleStepFactory()
{
    registerStep<IBConsoleBuildStep>(IncrediBuild::Constants::IBCONSOLE_BUILDSTEP_ID);
    setDisplayName(QObject::tr("IncrediBuild for Linux"));
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_CLEAN});
}

} // namespace Internal
} // namespace IncrediBuild
