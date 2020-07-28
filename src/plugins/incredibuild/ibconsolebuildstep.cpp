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

#include "cmakecommandbuilder.h"
#include "incredibuildconstants.h"
#include "makecommandbuilder.h"

#include <coreplugin/variablechooser.h>

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

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

private:
    void commandBuilderChanged();
    void commandArgsChanged();
    void makePathEdited();

    IBConsoleBuildStep *m_buildStep;

    QLineEdit *makeArgumentsLineEdit;
    QComboBox *commandBuilder;
    PathChooser *makePathChooser;
};

class IBConsoleBuildStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::IBConsoleBuildStep)

public:
    IBConsoleBuildStep(BuildStepList *buildStepList, Id id);

    bool init() final;

    BuildStepConfigWidget *createConfigWidget() final;

    bool fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

    void setCommandBuilder(const QString &commandBuilder);

    void tryToMigrate();

    void setupOutputFormatter(OutputFormatter *formatter) final;

public:
    BaseIntegerAspect *m_nice{nullptr};
    BaseBoolAspect *m_keepJobNum{nullptr};
    BaseBoolAspect *m_forceRemote{nullptr};
    BaseBoolAspect *m_alternate{nullptr};

    BuildStepList *m_earlierSteps{};
    bool m_loadedFromMap{false};

    CommandBuilder m_customCommandBuilder{this}; // "Custom Command"- needs to be first in the list.
    MakeCommandBuilder m_makeCommandBuilder{this};
    CMakeCommandBuilder m_cmakeCommandBuilder{this};

    CommandBuilder *m_commandBuilders[3] {
        &m_customCommandBuilder,
        &m_makeCommandBuilder,
        &m_cmakeCommandBuilder
    };
    CommandBuilder *m_activeCommandBuilder{m_commandBuilders[0]};
};

IBConsoleStepConfigWidget::IBConsoleStepConfigWidget(IBConsoleBuildStep *ibConsoleStep)
    : BuildStepConfigWidget(ibConsoleStep)
    , m_buildStep(ibConsoleStep)
{
    setDisplayName(tr("IncrediBuild for Linux"));
    setSummaryText("<b>" + displayName() + "</b>");

    QFont font;
    font.setBold(true);
    font.setWeight(75);

    auto section1 = new QLabel("Target and configuration", this);
    section1->setFont(font);

    auto commandHelperLabel = new QLabel(tr("Command Helper:"), this);
    commandHelperLabel->setToolTip(tr("Select an helper to establish the build command."));

    makePathChooser = new PathChooser(this);
    makeArgumentsLineEdit = new QLineEdit(this);
    const QString defaultCommand = m_buildStep->m_activeCommandBuilder->defaultCommand();
    makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    const QString command = m_buildStep->m_activeCommandBuilder->command();
    if (command != defaultCommand)
        makePathChooser->setPath(command);

    makePathChooser->setExpectedKind(PathChooser::Kind::ExistingCommand);
    makePathChooser->setBaseDirectory(FilePath::fromString(PathChooser::homePath()));
    makePathChooser->setHistoryCompleter(QLatin1String("IncrediBuild.IBConsole.MakeCommand.History"));
    connect(makePathChooser, &PathChooser::rawPathChanged, this, &IBConsoleStepConfigWidget::makePathEdited);

    QString defaultArgs;
    for (const QString &a : m_buildStep->m_activeCommandBuilder->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_buildStep->m_activeCommandBuilder->arguments())
        args += "\"" + a + "\" ";

    makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
    if (args != defaultArgs)
        makeArgumentsLineEdit->setText(args);
    connect(makeArgumentsLineEdit, &QLineEdit::textEdited, this, &IBConsoleStepConfigWidget::commandArgsChanged);

    auto section2 = new QLabel(this);
    section2->setText(tr("IncrediBuild Distribution control"));
    section2->setFont(font);

    commandBuilder = new QComboBox(this);
    for (CommandBuilder *p : m_buildStep->m_commandBuilders)
        commandBuilder->addItem(p->displayName());
    commandBuilder->setCurrentText(m_buildStep->m_activeCommandBuilder->displayName());
    connect(commandBuilder, &QComboBox::currentTextChanged, this, &IBConsoleStepConfigWidget::commandBuilderChanged);

    const auto emphasize = [](const QString &msg) { return QString("<i>" + msg); };
    auto infoLabel1 = new QLabel(emphasize(tr("Enter the appropriate arguments to your build "
                                              "command.")), this);
    auto infoLabel2 = new QLabel(emphasize(tr("Make sure the build command's "
                                              "multi-job parameter value is large enough (such as "
                                               "-j200 for the JOM or Make build tools)")), this);

    LayoutBuilder builder(this);
    builder.addRow(section1);
    builder.startNewRow().addItems(commandHelperLabel, commandBuilder);
    builder.startNewRow().addItems(tr("Make command:"), makePathChooser);
    builder.startNewRow().addItems(tr("Make arguments:"), makeArgumentsLineEdit);
    builder.addRow(infoLabel1);
    builder.addRow(infoLabel2);
    builder.addRow(ibConsoleStep->m_keepJobNum);

    builder.addRow(section2);
    builder.addRow(ibConsoleStep->m_nice);
    builder.addRow(ibConsoleStep->m_forceRemote);
    builder.addRow(ibConsoleStep->m_alternate);

    Core::VariableChooser::addSupportForChildWidgets(this, ibConsoleStep->macroExpander());
}

void IBConsoleStepConfigWidget::commandBuilderChanged()
{
    m_buildStep->setCommandBuilder(commandBuilder->currentText());

    QString defaultArgs;
    for (const QString &a : m_buildStep->m_activeCommandBuilder->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_buildStep->m_activeCommandBuilder->arguments())
        args += "\"" + a + "\" ";
    if (args == defaultArgs)
        args.clear();
    makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
    makeArgumentsLineEdit->setText(args);

    const QString defaultCommand = m_buildStep->m_activeCommandBuilder->defaultCommand();
    QString command = m_buildStep->m_activeCommandBuilder->command();
    if (command != defaultCommand)
        command.clear();
    makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    makePathChooser->setPath(command);
}

void IBConsoleStepConfigWidget::commandArgsChanged()
{
    m_buildStep->m_activeCommandBuilder->arguments(makeArgumentsLineEdit->text());
}

void IBConsoleStepConfigWidget::makePathEdited()
{
    m_buildStep->m_activeCommandBuilder->command(makePathChooser->rawPath());
}


// IBConsoleBuildStep

IBConsoleBuildStep::IBConsoleBuildStep(BuildStepList *buildStepList, Id id)
    : AbstractProcessStep(buildStepList, id)
    , m_earlierSteps(buildStepList)
{
    setDisplayName("IncrediBuild for Linux");

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

void IBConsoleBuildStep::tryToMigrate()
{
    // This is called when creating a fresh build step.
    // Attempt to detect build system from pre-existing steps.
    for (CommandBuilder *p : m_commandBuilders) {
        if (p->canMigrate(m_earlierSteps)) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void IBConsoleBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

bool IBConsoleBuildStep::fromMap(const QVariantMap &map)
{
    m_loadedFromMap = true;

    // Command builder. Default to the first in list, which should be the "Custom Command"
    setCommandBuilder(map.value(Constants::IBCONSOLE_COMMANDBUILDER, QVariant(m_commandBuilders[0]->displayName())).toString());
    bool result = m_activeCommandBuilder->fromMap(map);

    return result && AbstractProcessStep::fromMap(map);
}

QVariantMap IBConsoleBuildStep::toMap() const
{
    QVariantMap map = AbstractProcessStep::toMap();

    map[IncrediBuild::Constants::INCREDIBUILD_BUILDSTEP_TYPE] = QVariant(IncrediBuild::Constants::IBCONSOLE_BUILDSTEP_ID);
    map[Constants::IBCONSOLE_COMMANDBUILDER] = QVariant(m_activeCommandBuilder->displayName());
    m_activeCommandBuilder->toMap(&map);

    return map;
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

    m_activeCommandBuilder->keepJobNum(m_keepJobNum->value());
    args.append(m_activeCommandBuilder->fullCommandFlag());

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
    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!m_loadedFromMap)
        tryToMigrate();

    return new IBConsoleStepConfigWidget(this);
}



void IBConsoleBuildStep::setCommandBuilder(const QString &commandBuilder)
{
    for (CommandBuilder *p : m_commandBuilders) {
        if (p->displayName().compare(commandBuilder) == 0) {
            m_activeCommandBuilder = p;
            break;
        }
    }
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
