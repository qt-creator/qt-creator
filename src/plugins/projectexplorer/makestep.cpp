// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makestep.h"

#include "buildconfiguration.h"
#include "devicesupport/idevice.h"
#include "gnumakeparser.h"
#include "kitaspects.h"
#include "processparameters.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "target.h"
#include "toolchain.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QThread>

#include <optional>

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

    // FIXME: Replace with  id.name() + MAKE_COMMAND_SUFFIX  after the Key/Store transition
    m_makeCommandAspect.setSettingsKey(id.toKey() + MAKE_COMMAND_SUFFIX);
    m_makeCommandAspect.setExpectedKind(PathChooser::ExistingCommand);
    m_makeCommandAspect.setBaseFileName(PathChooser::homePath());
    m_makeCommandAspect.setHistoryCompleter("PE.MakeCommand.History");

    m_userArgumentsAspect.setSettingsKey(id.toKey() + MAKE_ARGUMENTS_SUFFIX);
    m_userArgumentsAspect.setLabelText(Tr::tr("Make arguments:"));
    m_userArgumentsAspect.setDisplayStyle(StringAspect::LineEditDisplay);

    m_jobCountAspect.setSettingsKey(id.toKey() + JOBCOUNT_SUFFIX);
    m_jobCountAspect.setLabel(Tr::tr("Parallel jobs:"));
    m_jobCountAspect.setRange(1, 999);
    m_jobCountAspect.setValue(defaultJobCount());
    m_jobCountAspect.setDefaultValue(defaultJobCount());

    const QString text = Tr::tr("Override MAKEFLAGS");
    m_overrideMakeflagsAspect.setSettingsKey(id.toKey() + OVERRIDE_MAKEFLAGS_SUFFIX);
    m_overrideMakeflagsAspect.setLabel(text, BoolAspect::LabelPlacement::AtCheckBox);

    m_nonOverrideWarning.setText("<html><body><p>" +
         Tr::tr("<code>MAKEFLAGS</code> specifies parallel jobs. Check \"%1\" to override.")
         .arg(text) + "</p></body></html>");
    m_nonOverrideWarning.setIconType(InfoLabel::Warning);

    m_disabledForSubdirsAspect.setSettingsKey(id.toKey() + ".disabledForSubdirs");
    m_disabledForSubdirsAspect.setLabel(Tr::tr("Disable in subdirectories:"));
    m_disabledForSubdirsAspect.setToolTip(Tr::tr("Runs this step only for a top-level build."));

    m_buildTargetsAspect.setSettingsKey(id.toKey() + BUILD_TARGETS_SUFFIX);
    m_buildTargetsAspect.setLabelText(Tr::tr("Targets:"));

    const auto updateMakeLabel = [this] {
        const FilePath defaultMake = defaultMakeCommand();
        const QString labelText = defaultMake.isEmpty()
                ? Tr::tr("Make:")
                : Tr::tr("Override %1:").arg(defaultMake.toUserOutput());
        m_makeCommandAspect.setLabelText(labelText);
    };

    updateMakeLabel();

    connect(&m_makeCommandAspect, &StringAspect::changed, this, updateMakeLabel);
}

void MakeStep::setSelectedBuildTarget(const QString &buildTarget)
{
    m_buildTargetsAspect.setValue({buildTarget});
}

void MakeStep::setAvailableBuildTargets(const QStringList &buildTargets)
{
    m_buildTargetsAspect.setAllValues(buildTargets);
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
    return Tr::tr("Make");
}

static const QList<ToolChain *> preferredToolChains(const Kit *kit)
{
    // prefer CXX, then C, then others
    return Utils::sorted(ToolChainKitAspect::toolChains(kit), [](ToolChain *tcA, ToolChain *tcB) {
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
}

FilePath MakeStep::defaultMakeCommand() const
{
    const Environment env = makeEnvironment();
    for (const ToolChain *tc : preferredToolChains(kit())) {
        FilePath make = tc->makeCommand(env);
        if (!make.isEmpty()) {
            IDevice::ConstPtr dev = BuildDeviceKitAspect::device(kit());
            QTC_ASSERT(dev, return {});
            return dev->filePath(make.path());
         }
    }
    return {};
}

QString MakeStep::msgNoMakeCommand()
{
    return Tr::tr("Make command missing. Specify Make command in step configuration.");
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

bool MakeStep::jobCountOverridesMakeflags() const
{
    return m_overrideMakeflagsAspect();
}

static std::optional<int> argsJobCount(const QString &str)
{
    const QStringList args = ProcessArgs::splitArgs(str, HostOsInfo::hostOs());
    const int argIndex = Utils::indexOf(args, [](const QString &arg) { return arg.startsWith("-j"); });
    if (argIndex == -1)
        return std::nullopt;
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
        return std::nullopt;
    return std::make_optional(ok && res > 0 ? res : 1000);
}

bool MakeStep::makeflagsJobCountMismatch() const
{
    const Environment env = makeEnvironment();
    if (!env.hasKey(MAKEFLAGS))
        return false;
    std::optional<int> makeFlagsJobCount = argsJobCount(env.expandedValueForKey(MAKEFLAGS));
    return makeFlagsJobCount.has_value() && *makeFlagsJobCount != m_jobCountAspect();
}

bool MakeStep::enabledForSubDirs() const
{
    return !m_disabledForSubdirsAspect();
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
    env.setupEnglishOutput();
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
    m_makeCommandAspect.setValue(command);
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
    return {"-j" + QString::number(m_jobCountAspect())};
}

QString MakeStep::userArguments() const
{
    return m_userArgumentsAspect();
}

void MakeStep::setUserArguments(const QString &args)
{
    m_userArgumentsAspect.setValue(args);
}

QStringList MakeStep::displayArguments() const
{
    return {};
}

FilePath MakeStep::makeCommand() const
{
    return m_makeCommandAspect();
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
    cmd.addArgs(m_buildTargetsAspect());

    return cmd;
}

QWidget *MakeStep::createConfigWidget()
{
    Layouting::Form builder;
    builder.addRow({m_makeCommandAspect});
    builder.addRow({m_userArgumentsAspect});
    builder.addRow({m_jobCountAspect, m_overrideMakeflagsAspect, m_nonOverrideWarning});
    if (m_disablingForSubDirsSupported)
        builder.addRow({m_disabledForSubdirsAspect});
    builder.addRow({m_buildTargetsAspect});
    builder.addItem(Layouting::noMargin);

    auto widget = builder.emerge();

    VariableChooser::addSupportForChildWidgets(widget, macroExpander());

    setSummaryUpdater([this] {
        const CommandLine make = effectiveMakeCommand(MakeStep::Display);
        if (make.executable().isEmpty())
            return Tr::tr("<b>Make:</b> %1").arg(MakeStep::msgNoMakeCommand());

        if (!buildConfiguration())
            return Tr::tr("<b>Make:</b> No build configuration.");

        ProcessParameters param;
        param.setMacroExpander(macroExpander());
        param.setWorkingDirectory(buildDirectory());
        param.setCommandLine(make);
        param.setEnvironment(buildEnvironment());

        if (param.commandMissing()) {
            return Tr::tr("<b>Make:</b> %1 not found in the environment.")
                        .arg(param.command().executable().toUserOutput()); // Override display text
        }

        return param.summaryInWorkdir(displayName());
    });

    auto updateDetails = [this] {
        const bool jobCountVisible = isJobCountSupported();
        m_jobCountAspect.setVisible(jobCountVisible);
        m_overrideMakeflagsAspect.setVisible(jobCountVisible);

        const bool jobCountEnabled = !userArgsContainsJobCount();
        m_jobCountAspect.setEnabled(jobCountEnabled);
        m_overrideMakeflagsAspect.setEnabled(jobCountEnabled);
        m_nonOverrideWarning.setVisible(makeflagsJobCountMismatch()
                                         && !jobCountOverridesMakeflags());
    };

    updateDetails();

    connect(&m_makeCommandAspect, &StringAspect::changed, widget, updateDetails);
    connect(&m_userArgumentsAspect, &StringAspect::changed, widget, updateDetails);
    connect(&m_jobCountAspect, &IntegerAspect::changed, widget, updateDetails);
    connect(&m_overrideMakeflagsAspect, &BoolAspect::changed, widget, updateDetails);
    connect(&m_buildTargetsAspect, &BaseAspect::changed, widget, updateDetails);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            widget, updateDetails);

    connect(target(), &Target::kitChanged, widget, updateDetails);

    connect(buildConfiguration(), &BuildConfiguration::environmentChanged, widget, updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::buildDirectoryChanged, widget, updateDetails);
    connect(target(), &Target::parsingFinished, widget, updateDetails);

    return widget;
}

QStringList MakeStep::availableTargets() const
{
    return m_buildTargetsAspect.allValues();
}

} // namespace ProjectExplorer
