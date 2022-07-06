/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "makeinstallstep.h"

#include "remotelinux_constants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QFormLayout>
#include <QSet>
#include <QTemporaryDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const char MakeAspectId[] = "RemoteLinux.MakeInstall.Make";
const char InstallRootAspectId[] = "RemoteLinux.MakeInstall.InstallRoot";
const char CleanInstallRootAspectId[] = "RemoteLinux.MakeInstall.CleanInstallRoot";
const char FullCommandLineAspectId[] = "RemoteLinux.MakeInstall.FullCommandLine";
const char CustomCommandLineAspectId[] = "RemoteLinux.MakeInstall.CustomCommandLine";

MakeInstallStep::MakeInstallStep(BuildStepList *parent, Id id) : MakeStep(parent, id)
{
    makeCommandAspect()->setVisible(false);
    buildTargetsAspect()->setVisible(false);
    userArgumentsAspect()->setVisible(false);
    overrideMakeflagsAspect()->setVisible(false);
    nonOverrideWarning()->setVisible(false);
    jobCountAspect()->setVisible(false);
    disabledForSubdirsAspect()->setVisible(false);

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

    const auto makeAspect = addAspect<ExecutableAspect>(parent->target(),
                                                        ExecutableAspect::BuildDevice);
    makeAspect->setId(MakeAspectId);
    makeAspect->setSettingsKey(MakeAspectId);
    makeAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    makeAspect->setLabelText(tr("Command:"));
    connect(makeAspect, &ExecutableAspect::changed,
            this, &MakeInstallStep::updateCommandFromAspect);

    const auto installRootAspect = addAspect<StringAspect>();
    installRootAspect->setId(InstallRootAspectId);
    installRootAspect->setSettingsKey(InstallRootAspectId);
    installRootAspect->setDisplayStyle(StringAspect::PathChooserDisplay);
    installRootAspect->setExpectedKind(PathChooser::Directory);
    installRootAspect->setLabelText(tr("Install root:"));
    installRootAspect->setFilePath(rootPath);
    connect(installRootAspect, &StringAspect::changed,
            this, &MakeInstallStep::updateArgsFromAspect);

    const auto cleanInstallRootAspect = addAspect<BoolAspect>();
    cleanInstallRootAspect->setId(CleanInstallRootAspectId);
    cleanInstallRootAspect->setSettingsKey(CleanInstallRootAspectId);
    cleanInstallRootAspect->setLabel(tr("Clean install root first:"),
                                     BoolAspect::LabelPlacement::InExtraLabel);
    cleanInstallRootAspect->setValue(true);

    const auto commandLineAspect = addAspect<StringAspect>();
    commandLineAspect->setId(FullCommandLineAspectId);
    commandLineAspect->setDisplayStyle(StringAspect::LabelDisplay);
    commandLineAspect->setLabelText(tr("Full command line:"));

    const auto customCommandLineAspect = addAspect<StringAspect>();
    customCommandLineAspect->setId(CustomCommandLineAspectId);
    customCommandLineAspect->setSettingsKey(CustomCommandLineAspectId);
    customCommandLineAspect->setDisplayStyle(StringAspect::LineEditDisplay);
    customCommandLineAspect->setLabelText(tr("Custom command line:"));
    customCommandLineAspect->makeCheckable(StringAspect::CheckBoxPlacement::Top,
                                           tr("Use custom command line instead:"),
                                           "RemoteLinux.MakeInstall.EnableCustomCommandLine");
    const auto updateCommand = [this] {
        updateCommandFromAspect();
        updateArgsFromAspect();
        updateFromCustomCommandLineAspect();
    };

    connect(customCommandLineAspect, &StringAspect::checkedChanged, this, updateCommand);
    connect(customCommandLineAspect, &StringAspect::changed,
            this, &MakeInstallStep::updateFromCustomCommandLineAspect);

    connect(target(), &Target::buildSystemUpdated, this, updateCommand);

    const MakeInstallCommand cmd = buildSystem()->makeInstallCommand(rootPath);
    QTC_ASSERT(!cmd.command.isEmpty(), return);
    makeAspect->setExecutable(cmd.command.executable());
}

Utils::Id MakeInstallStep::stepId()
{
    return Constants::MakeInstallStepId;
}

QString MakeInstallStep::displayName()
{
    return tr("Install into temporary host directory");
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

    const FilePath rootDir = installRoot().onDevice(makeCommand());
    if (rootDir.isEmpty()) {
        emit addTask(BuildSystemTask(Task::Error, tr("You must provide an install root.")));
        return false;
    }
    if (cleanInstallRoot() && !rootDir.removeRecursively()) {
        emit addTask(BuildSystemTask(Task::Error,
                                        tr("The install root \"%1\" could not be cleaned.")
                                            .arg(rootDir.displayName())));
        return false;
    }
    if (!rootDir.exists() && !rootDir.createDir()) {
        emit addTask(BuildSystemTask(Task::Error,
                                        tr("The install root \"%1\" could not be created.")
                                            .arg(rootDir.displayName())));
        return false;
    }
    if (this == deployConfiguration()->stepList()->steps().last()) {
        emit addTask(BuildSystemTask(Task::Warning,
                                        tr("The \"make install\" step should probably not be "
                                            "last in the list of deploy steps. "
                                            "Consider moving it up.")));
    }

    const MakeInstallCommand cmd = buildSystem()->makeInstallCommand(rootDir);
    if (cmd.environment.isValid()) {
        Environment env = processParameters()->environment();
        for (auto it = cmd.environment.constBegin(); it != cmd.environment.constEnd(); ++it) {
            if (cmd.environment.isEnabled(it)) {
                const QString key = cmd.environment.key(it);
                env.set(key, cmd.environment.expandedValueForKey(key));
            }
        }
        processParameters()->setEnvironment(env);
    }
    m_noInstallTarget = false;

    const auto buildStep = buildConfiguration()->buildSteps()->firstOfType<AbstractProcessStep>();
    m_isCmakeProject = buildStep
            && buildStep->processParameters()->command().executable().toString()
            .contains("cmake");

    return true;
}

void MakeInstallStep::finish(bool success)
{
    if (success) {
        const bool hack = makeCommand().needsDevice();
        const FilePath rootDir = installRoot().onDevice(makeCommand());

        m_deploymentData = DeploymentData();
        m_deploymentData.setLocalInstallRoot(rootDir);

        const int startPos = rootDir.toString().length();

        const auto appFileNames = transform<QSet<QString>>(buildSystem()->applicationTargets(),
            [](const BuildTargetInfo &appTarget) { return appTarget.targetFilePath.fileName(); });

        auto handleFile = [this, &appFileNames, startPos, hack](const FilePath &filePath) {
            const DeployableFile::Type type = appFileNames.contains(filePath.fileName())
                ? DeployableFile::TypeExecutable
                : DeployableFile::TypeNormal;
            QString targetDir = filePath.parentDir().toString().mid(startPos);
            // FIXME: This is conceptually the wrong place, but currently "downstream" like
            // the rsync step doesn't handle full remote paths here.
            targetDir = FilePath::fromString(targetDir).path();

            // FIXME: Hack, Part#2: If the build was indeed not local, drop the remoteness.
            // As we rely on shared build directory, this "maps" to the host.
            if (hack)
                m_deploymentData.addFile(FilePath::fromString(filePath.path()), targetDir, type);
            else
                m_deploymentData.addFile(filePath, targetDir, type);
            return true;
        };
        rootDir.iterateDirectory(handleFile,
                                 {{}, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories});

        buildSystem()->setDeploymentData(m_deploymentData);
    } else if (m_noInstallTarget && m_isCmakeProject) {
        emit addTask(DeploymentTask(Task::Warning, tr("You need to add an install statement "
                   "to your CMakeLists.txt file for deployment to work.")));
    }
    MakeStep::finish(success);
}

void MakeInstallStep::stdError(const QString &line)
{
    // When using Makefiles: "No rule to make target 'install'"
    // When using ninja: "ninja: error: unknown target 'install'"
    if (line.contains("target 'install'"))
        m_noInstallTarget = true;
    MakeStep::stdError(line);
}

FilePath MakeInstallStep::installRoot() const
{
    return static_cast<StringAspect *>(aspect(InstallRootAspectId))->filePath();
}

bool MakeInstallStep::cleanInstallRoot() const
{
    return static_cast<BoolAspect *>(aspect(CleanInstallRootAspectId))->value();
}

void MakeInstallStep::updateCommandFromAspect()
{
    if (customCommandLineAspect()->isChecked())
        return;
    setMakeCommand(aspect<ExecutableAspect>()->executable());
    updateFullCommandLine();
}

void MakeInstallStep::updateArgsFromAspect()
{
    if (customCommandLineAspect()->isChecked())
        return;
    const CommandLine cmd = buildSystem()->makeInstallCommand(installRoot()).command;
    setUserArguments(cmd.arguments());
    updateFullCommandLine();
}

void MakeInstallStep::updateFullCommandLine()
{
    // FIXME: Only executable?
    static_cast<StringAspect *>(aspect(FullCommandLineAspectId))->setValue(
                QDir::toNativeSeparators(
                    ProcessArgs::quoteArg(makeExecutable().toString()))
                + ' '  + userArguments());
}

void MakeInstallStep::updateFromCustomCommandLineAspect()
{
    const StringAspect * const aspect = customCommandLineAspect();
    if (!aspect->isChecked())
        return;
    const QStringList tokens = ProcessArgs::splitArgs(aspect->value());
    setMakeCommand(tokens.isEmpty() ? FilePath() : FilePath::fromString(tokens.first()));
    setUserArguments(ProcessArgs::joinArgs(tokens.mid(1)));
}

StringAspect *MakeInstallStep::customCommandLineAspect() const
{
    return static_cast<StringAspect *>(aspect(CustomCommandLineAspectId));
}

bool MakeInstallStep::fromMap(const QVariantMap &map)
{
    if (!MakeStep::fromMap(map))
        return false;
    updateCommandFromAspect();
    updateArgsFromAspect();
    updateFromCustomCommandLineAspect();
    return true;
}

} // namespace RemoteLinux
