/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "makestep.h"

#include "buildconfiguration.h"
#include "gnumakeparser.h"
#include "kitinformation.h"
#include "project.h"
#include "processparameters.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "toolchain.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/optional.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QThread>

using namespace Core;
using namespace Utils;

const char BUILD_TARGETS_SUFFIX[] = ".BuildTargets";
const char MAKE_ARGUMENTS_SUFFIX[] = ".MakeArguments";
const char MAKE_COMMAND_SUFFIX[] = ".MakeCommand";
const char OVERRIDE_MAKEFLAGS_SUFFIX[] = ".OverrideMakeflags";
const char JOBCOUNT_SUFFIX[] = ".JobCount";

const char MAKEFLAGS[] = "MAKEFLAGS";

namespace ProjectExplorer {

MakeStep::MakeStep(BuildStepList *parent, Id id)
    : AbstractProcessStep(parent, id)
{
    setLowPriority();

    setCommandLineProvider([this] { return effectiveMakeCommand(Execution); });

    m_makeCommandAspect = addAspect<StringAspect>();
    m_makeCommandAspect->setSettingsKey(id.withSuffix(MAKE_COMMAND_SUFFIX).toString());
    m_makeCommandAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_makeCommandAspect->setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommandAspect->setBaseFileName(FilePath::fromString(PathChooser::homePath()));
    m_makeCommandAspect->setHistoryCompleter("PE.MakeCommand.History");

    m_userArgumentsAspect = addAspect<StringAspect>();
    m_userArgumentsAspect->setSettingsKey(id.withSuffix(MAKE_ARGUMENTS_SUFFIX).toString());
    m_userArgumentsAspect->setLabelText(tr("Make arguments:"));
    m_userArgumentsAspect->setDisplayStyle(StringAspect::LineEditDisplay);

    m_jobCountContainer = addAspect<AspectContainer>();

    m_userJobCountAspect = m_jobCountContainer->addAspect<IntegerAspect>();
    m_userJobCountAspect->setSettingsKey(id.withSuffix(JOBCOUNT_SUFFIX).toString());
    m_userJobCountAspect->setLabel(tr("Parallel jobs:"));
    m_userJobCountAspect->setRange(1, 999);
    m_userJobCountAspect->setValue(defaultJobCount());
    m_userJobCountAspect->setDefaultValue(defaultJobCount());

    const QString text = tr("Override MAKEFLAGS");
    m_overrideMakeflagsAspect = m_jobCountContainer->addAspect<BoolAspect>();
    m_overrideMakeflagsAspect->setSettingsKey(id.withSuffix(OVERRIDE_MAKEFLAGS_SUFFIX).toString());
    m_overrideMakeflagsAspect->setLabel(text, BoolAspect::LabelPlacement::AtCheckBox);

    m_nonOverrideWarning = m_jobCountContainer->addAspect<TextDisplay>();
    m_nonOverrideWarning->setToolTip("<html><body><p>" +
         tr("<code>MAKEFLAGS</code> specifies parallel jobs. Check \"%1\" to override.")
         .arg(text) + "</p></body></html>");
    m_nonOverrideWarning->setIconType(InfoLabel::Warning);

    m_disabledForSubdirsAspect = addAspect<BoolAspect>();
    m_disabledForSubdirsAspect->setSettingsKey(id.withSuffix(".disabledForSubdirs").toString());
    m_disabledForSubdirsAspect->setLabel(tr("Disable in subdirectories:"));
    m_disabledForSubdirsAspect->setToolTip(tr("Runs this step only for a top-level build."));

    m_buildTargetsAspect = addAspect<MultiSelectionAspect>();
    m_buildTargetsAspect->setSettingsKey(id.withSuffix(BUILD_TARGETS_SUFFIX).toString());
    m_buildTargetsAspect->setLabelText(tr("Targets:"));

    const auto updateMakeLabel = [this] {
        const QString defaultMake = defaultMakeCommand().toString();
        const QString labelText = defaultMake.isEmpty()
                ? tr("Make:")
                : tr("Override %1:").arg(QDir::toNativeSeparators(defaultMake));
        m_makeCommandAspect->setLabelText(labelText);
    };

    updateMakeLabel();

    connect(m_makeCommandAspect, &StringAspect::changed, this, updateMakeLabel);
}

void MakeStep::setSelectedBuildTarget(const QString &buildTarget)
{
    m_buildTargetsAspect->setValue({buildTarget});
}

void MakeStep::setAvailableBuildTargets(const QStringList &buildTargets)
{
    m_buildTargetsAspect->setAllValues(buildTargets);
}

bool MakeStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    const CommandLine make = effectiveMakeCommand(Execution);
    if (make.executable().isEmpty())
        emit addTask(makeCommandMissingTask());

    if (make.executable().isEmpty()) {
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

void MakeStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

QString MakeStep::defaultDisplayName()
{
    return tr("Make");
}

static const QList<ToolChain *> preferredToolChains(const Kit *kit)
{
    QList<ToolChain *> tcs = ToolChainKitAspect::toolChains(kit);
    // prefer CXX, then C, then others
    Utils::sort(tcs, [](ToolChain *tcA, ToolChain *tcB) {
        if (tcA->language() == tcB->language())
            return false;
        if (tcA->language() == Constants::CXX_LANGUAGE_ID)
            return true;
        if (tcB->language() == Constants::CXX_LANGUAGE_ID)
            return false;
        if (tcA->language() == Constants::C_LANGUAGE_ID)
            return true;
        return false;
    });
    return tcs;
}

FilePath MakeStep::defaultMakeCommand() const
{
    const Utils::Environment env = makeEnvironment();
    for (const ToolChain *tc : preferredToolChains(kit())) {
        FilePath make = tc->makeCommand(env);
        if (!make.isEmpty())
            return make;
    }
    return {};
}

QString MakeStep::msgNoMakeCommand()
{
    return tr("Make command missing. Specify Make command in step configuration.");
}

Task MakeStep::makeCommandMissingTask()
{
    return BuildSystemTask(Task::Error, msgNoMakeCommand());
}

bool MakeStep::isJobCountSupported() const
{
    const QList<ToolChain *> tcs = preferredToolChains(kit());
    const ToolChain *tc = tcs.isEmpty() ? nullptr : tcs.constFirst();
    return tc && tc->isJobCountSupported();
}

int MakeStep::jobCount() const
{
    return m_userJobCountAspect->value();
}

bool MakeStep::jobCountOverridesMakeflags() const
{
    return m_overrideMakeflagsAspect->value();
}

static Utils::optional<int> argsJobCount(const QString &str)
{
    const QStringList args = Utils::QtcProcess::splitArgs(str, Utils::HostOsInfo::hostOs());
    const int argIndex = Utils::indexOf(args, [](const QString &arg) { return arg.startsWith("-j"); });
    if (argIndex == -1)
        return Utils::nullopt;
    QString arg = args.at(argIndex);
    bool requireNumber = false;
    // -j [4] as separate arguments (or no value)
    if (arg == "-j") {
        if (args.size() <= argIndex + 1)
            return 1000; // unlimited
        arg = args.at(argIndex + 1);
    } else { // -j4
        arg = arg.mid(2).trimmed();
        requireNumber = true;
    }
    bool ok = false;
    const int res = arg.toInt(&ok);
    if (!ok && requireNumber)
        return Utils::nullopt;
    return Utils::make_optional(ok && res > 0 ? res : 1000);
}

bool MakeStep::makeflagsJobCountMismatch() const
{
    const Environment env = makeEnvironment();
    if (!env.hasKey(MAKEFLAGS))
        return false;
    Utils::optional<int> makeFlagsJobCount = argsJobCount(env.expandedValueForKey(MAKEFLAGS));
    return makeFlagsJobCount.has_value() && *makeFlagsJobCount != m_userJobCountAspect->value();
}

bool MakeStep::enabledForSubDirs() const
{
    return !m_disabledForSubdirsAspect->value();
}

bool MakeStep::makeflagsContainsJobCount() const
{
    const Environment env = makeEnvironment();
    if (!env.hasKey(MAKEFLAGS))
        return false;
    return argsJobCount(env.expandedValueForKey(MAKEFLAGS)).has_value();
}

bool MakeStep::userArgsContainsJobCount() const
{
    return argsJobCount(userArguments()).has_value();
}

Environment MakeStep::makeEnvironment() const
{
    Environment env = buildEnvironment();
    Utils::Environment::setupEnglishOutput(&env);
    if (makeCommand().isEmpty()) {
        // We also prepend "L" to the MAKEFLAGS, so that nmake / jom are less verbose
        const QList<ToolChain *> tcs = preferredToolChains(target()->kit());
        const ToolChain *tc = tcs.isEmpty() ? nullptr : tcs.constFirst();
        if (tc && tc->targetAbi().os() == Abi::WindowsOS
                && tc->targetAbi().osFlavor() != Abi::WindowsMSysFlavor) {
            env.set(MAKEFLAGS, 'L' + env.expandedValueForKey(MAKEFLAGS));
        }
    }
    return env;
}

void MakeStep::setMakeCommand(const FilePath &command)
{
    m_makeCommandAspect->setFilePath(command);
}

int MakeStep::defaultJobCount()
{
    return QThread::idealThreadCount();
}

QStringList MakeStep::jobArguments() const
{
    if (!isJobCountSupported() || userArgsContainsJobCount()
            || (makeflagsContainsJobCount() && !jobCountOverridesMakeflags())) {
        return {};
    }
    return {"-j" + QString::number(m_userJobCountAspect->value())};
}

QString MakeStep::userArguments() const
{
    return m_userArgumentsAspect->value();
}

void MakeStep::setUserArguments(const QString &args)
{
    m_userArgumentsAspect->setValue(args);
}

QStringList MakeStep::displayArguments() const
{
    return {};
}

FilePath MakeStep::makeCommand() const
{
    return m_makeCommandAspect->filePath();
}

FilePath MakeStep::makeExecutable() const
{
    const FilePath cmd = makeCommand();
    return cmd.isEmpty() ? defaultMakeCommand() : cmd;
}

CommandLine MakeStep::effectiveMakeCommand(MakeCommandType type) const
{
    CommandLine cmd(makeExecutable());

    if (type == Display)
        cmd.addArgs(displayArguments());
    cmd.addArgs(userArguments(), CommandLine::Raw);
    cmd.addArgs(jobArguments());
    cmd.addArgs(m_buildTargetsAspect->value());

    return cmd;
}

QWidget *MakeStep::createConfigWidget()
{
    auto widget = new QWidget;

    LayoutBuilder builder(widget);
    builder.addRow(m_makeCommandAspect);
    builder.addRow(m_userArgumentsAspect);
    builder.addRow(m_jobCountContainer);
    if (m_disablingForSubDirsSupported)
        builder.addRow(m_disabledForSubdirsAspect);
    builder.addRow(m_buildTargetsAspect);

    VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    setSummaryUpdater([this] {
        const CommandLine make = effectiveMakeCommand(MakeStep::Display);
        if (make.executable().isEmpty())
            return tr("<b>Make:</b> %1").arg(MakeStep::msgNoMakeCommand());

        if (!buildConfiguration())
            return tr("<b>Make:</b> No build configuration.");

        ProcessParameters param;
        param.setMacroExpander(macroExpander());
        param.setWorkingDirectory(buildDirectory());
        param.setCommandLine(make);
        param.setEnvironment(buildEnvironment());

        if (param.commandMissing()) {
            return tr("<b>Make:</b> %1 not found in the environment.")
                        .arg(param.command().executable().toUserOutput()); // Override display text
        }

        return param.summaryInWorkdir(displayName());
    });

    auto updateDetails = [this] {
        const bool jobCountVisible = isJobCountSupported();
        m_userJobCountAspect->setVisible(jobCountVisible);
        m_overrideMakeflagsAspect->setVisible(jobCountVisible);

        const bool jobCountEnabled = !userArgsContainsJobCount();
        m_userJobCountAspect->setEnabled(jobCountEnabled);
        m_overrideMakeflagsAspect->setEnabled(jobCountEnabled);
        m_nonOverrideWarning->setVisible(makeflagsJobCountMismatch()
                                         && !jobCountOverridesMakeflags());
    };

    updateDetails();

    connect(m_makeCommandAspect, &StringAspect::changed, widget, updateDetails);
    connect(m_userArgumentsAspect, &StringAspect::changed, widget, updateDetails);
    connect(m_userJobCountAspect, &IntegerAspect::changed, widget, updateDetails);
    connect(m_overrideMakeflagsAspect, &BoolAspect::changed, widget, updateDetails);
    connect(m_buildTargetsAspect, &BaseAspect::changed, widget, updateDetails);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            widget, updateDetails);

    connect(target(), &Target::kitChanged, widget, updateDetails);

    connect(buildConfiguration(), &BuildConfiguration::environmentChanged, widget, updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, widget, updateDetails);
    connect(target(), &Target::parsingFinished, widget, updateDetails);

    return widget;
}

bool MakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargetsAspect->value().contains(target);
}

void MakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargetsAspect->value();
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargetsAspect->setValue(old);
}

QStringList MakeStep::availableTargets() const
{
    return m_buildTargetsAspect->allValues();
}

} // namespace ProjectExplorer
