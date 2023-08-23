// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "makeinstallstep.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/makestep.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QSet>
#include <QTemporaryDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

class MakeInstallStep : public MakeStep
{
public:
    MakeInstallStep(BuildStepList *parent, Id id);

private:
    void fromMap(const Store &map) override;
    QWidget *createConfigWidget() override;
    bool init() override;
    Tasking::GroupItem runRecipe() final;
    bool isJobCountSupported() const override { return false; }

    void updateCommandFromAspect();
    void updateArgsFromAspect();
    void updateFullCommandLine();
    void updateFromCustomCommandLineAspect();

    ExecutableAspect m_makeBinary{this};
    FilePathAspect m_installRoot{this};
    BoolAspect m_cleanInstallRoot{this};
    StringAspect m_fullCommand{this};
    StringAspect m_customCommand{this};

    DeploymentData m_deploymentData;
    bool m_noInstallTarget = false;
    bool m_isCmakeProject = false;
};

MakeInstallStep::MakeInstallStep(BuildStepList *parent, Id id) : MakeStep(parent, id)
{
    m_makeBinary.setVisible(false);
    m_buildTargetsAspect.setVisible(false);
    m_userArgumentsAspect.setVisible(false);
    m_overrideMakeflagsAspect.setVisible(false);
    m_nonOverrideWarning.setVisible(false);
    m_jobCountAspect.setVisible(false);
    m_disabledForSubdirsAspect.setVisible(false);

    // FIXME: Hack, Part#1: If the build device is not local, start with a temp dir
    // inside the build dir. On Docker that's typically shared with the host.
    const IDevice::ConstPtr device = BuildDeviceKitAspect::device(target()->kit());
    const bool hack = device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    FilePath rootPath;
    if (hack) {
        rootPath = buildDirectory().pathAppended(".tmp-root");
    } else {
        QTemporaryDir tmpDir;
        rootPath = FilePath::fromString(tmpDir.path());
    }

    m_makeBinary.setDeviceSelector(parent->target(), ExecutableAspect::BuildDevice);
    m_makeBinary.setSettingsKey("RemoteLinux.MakeInstall.Make");
    m_makeBinary.setReadOnly(false);
    m_makeBinary.setLabelText(Tr::tr("Command:"));
    connect(&m_makeBinary, &BaseAspect::changed,
            this, &MakeInstallStep::updateCommandFromAspect);

    m_installRoot.setSettingsKey("RemoteLinux.MakeInstall.InstallRoot");
    m_installRoot.setExpectedKind(PathChooser::Directory);
    m_installRoot.setLabelText(Tr::tr("Install root:"));
    m_installRoot.setValue(rootPath);
    connect(&m_installRoot, &BaseAspect::changed,
            this, &MakeInstallStep::updateArgsFromAspect);

    m_cleanInstallRoot.setSettingsKey("RemoteLinux.MakeInstall.CleanInstallRoot");
    m_cleanInstallRoot.setLabelText(Tr::tr("Clean install root first:"));
    m_cleanInstallRoot.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    m_cleanInstallRoot.setDefaultValue(true);

    m_fullCommand.setDisplayStyle(StringAspect::LabelDisplay);
    m_fullCommand.setLabelText(Tr::tr("Full command line:"));

    m_customCommand.setSettingsKey("RemoteLinux.MakeInstall.CustomCommandLine");
    m_customCommand.setDisplayStyle(StringAspect::LineEditDisplay);
    m_customCommand.setLabelText(Tr::tr("Custom command line:"));
    m_customCommand.makeCheckable(CheckBoxPlacement::Top,
                                  Tr::tr("Use custom command line instead:"),
                                  "RemoteLinux.MakeInstall.EnableCustomCommandLine");
    const auto updateCommand = [this] {
        updateCommandFromAspect();
        updateArgsFromAspect();
        updateFromCustomCommandLineAspect();
    };

    connect(&m_customCommand, &StringAspect::checkedChanged, this, updateCommand);
    connect(&m_customCommand, &StringAspect::changed,
            this, &MakeInstallStep::updateFromCustomCommandLineAspect);

    connect(target(), &Target::buildSystemUpdated, this, updateCommand);

    const MakeInstallCommand cmd = buildSystem()->makeInstallCommand(rootPath);
    QTC_ASSERT(!cmd.command.isEmpty(), return);
    m_makeBinary.setExecutable(cmd.command.executable());

    connect(this, &BuildStep::addOutput, this, [this](const QString &string, OutputFormat format) {
        // When using Makefiles: "No rule to make target 'install'"
        // When using ninja: "ninja: error: unknown target 'install'"
        if (format == OutputFormat::Stderr && string.contains("target 'install'"))
            m_noInstallTarget = true;
    });
}

QWidget *MakeInstallStep::createConfigWidget()
{
    // Note: this intentionally skips the MakeStep::createConfigWidget() level.
    return BuildStep::createConfigWidget();
}

bool MakeInstallStep::init()
{
    if (!MakeStep::init())
        return false;

    const FilePath rootDir = makeCommand().withNewPath(m_installRoot().path()); // FIXME: Needed?
    if (rootDir.isEmpty()) {
        emit addTask(BuildSystemTask(Task::Error, Tr::tr("You must provide an install root.")));
        return false;
    }
    if (m_cleanInstallRoot() && !rootDir.removeRecursively()) {
        emit addTask(BuildSystemTask(Task::Error,
                                        Tr::tr("The install root \"%1\" could not be cleaned.")
                                            .arg(rootDir.displayName())));
        return false;
    }
    if (!rootDir.exists() && !rootDir.createDir()) {
        emit addTask(BuildSystemTask(Task::Error,
                                        Tr::tr("The install root \"%1\" could not be created.")
                                            .arg(rootDir.displayName())));
        return false;
    }
    if (this == deployConfiguration()->stepList()->steps().last()) {
        emit addTask(BuildSystemTask(Task::Warning,
                                        Tr::tr("The \"make install\" step should probably not be "
                                            "last in the list of deploy steps. "
                                            "Consider moving it up.")));
    }

    const MakeInstallCommand cmd = buildSystem()->makeInstallCommand(rootDir);
    if (cmd.environment.hasChanges()) {
        Environment env = processParameters()->environment();
        cmd.environment.forEachEntry([&](const QString &key, const QString &value, bool enabled) {
            if (enabled)
                env.set(key, cmd.environment.expandVariables(value));
        });
        processParameters()->setEnvironment(env);
    }
    m_noInstallTarget = false;

    const auto buildStep = buildConfiguration()->buildSteps()->firstOfType<AbstractProcessStep>();
    m_isCmakeProject = buildStep
            && buildStep->processParameters()->command().executable().toString()
            .contains("cmake");

    return true;
}

Tasking::GroupItem MakeInstallStep::runRecipe()
{
    using namespace Tasking;

    const auto onDone = [this] {
        const FilePath rootDir = makeCommand().withNewPath(m_installRoot().path()); // FIXME: Needed?

        m_deploymentData = DeploymentData();
        m_deploymentData.setLocalInstallRoot(rootDir);

        const int startPos = rootDir.path().length();

        const auto appFileNames = transform<QSet<QString>>(buildSystem()->applicationTargets(),
            [](const BuildTargetInfo &appTarget) { return appTarget.targetFilePath.fileName(); });

        auto handleFile = [this, &appFileNames, startPos](const FilePath &filePath) {
            const DeployableFile::Type type = appFileNames.contains(filePath.fileName())
                ? DeployableFile::TypeExecutable : DeployableFile::TypeNormal;
            const QString targetDir = filePath.parentDir().path().mid(startPos);
            m_deploymentData.addFile(filePath, targetDir, type);
            return IterationPolicy::Continue;
        };
        rootDir.iterateDirectory(
            handleFile, {{}, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories});

        buildSystem()->setDeploymentData(m_deploymentData);
    };
    const auto onError = [this] {
        if (m_noInstallTarget && m_isCmakeProject) {
            emit addTask(DeploymentTask(Task::Warning, Tr::tr("You need to add an install "
                "statement to your CMakeLists.txt file for deployment to work.")));
        }
    };

    return Group { onGroupDone(onDone), onGroupError(onError), defaultProcessTask() };
}

void MakeInstallStep::updateCommandFromAspect()
{
    if (m_customCommand.isChecked())
        return;
    setMakeCommand(m_makeBinary());
    updateFullCommandLine();
}

void MakeInstallStep::updateArgsFromAspect()
{
    if (m_customCommand.isChecked())
        return;
    const CommandLine cmd = buildSystem()->makeInstallCommand(m_installRoot()).command;
    setUserArguments(cmd.arguments());
    updateFullCommandLine();
}

void MakeInstallStep::updateFullCommandLine()
{
    CommandLine cmd{makeExecutable(), userArguments(), CommandLine::Raw};
    m_fullCommand.setValue(cmd.toUserOutput());
}

void MakeInstallStep::updateFromCustomCommandLineAspect()
{
    if (m_customCommand.isChecked())
        return;
    const QStringList tokens = ProcessArgs::splitArgs(m_customCommand(), HostOsInfo::hostOs());
    setMakeCommand(tokens.isEmpty() ? FilePath() : FilePath::fromString(tokens.first()));
    setUserArguments(ProcessArgs::joinArgs(tokens.mid(1)));
}

void MakeInstallStep::fromMap(const Store &map)
{
    MakeStep::fromMap(map);
    if (hasError())
        return;
    updateCommandFromAspect();
    updateArgsFromAspect();
    updateFromCustomCommandLineAspect();
}

// Factory

MakeInstallStepFactory::MakeInstallStepFactory()
{
    registerStep<MakeInstallStep>(Constants::MakeInstallStepId);
    setDisplayName(Tr::tr("Install into temporary host directory"));
}

} // RemoteLinux::Internal
