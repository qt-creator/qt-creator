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
#include <projectexplorer/processparameters.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QFormLayout>
#include <QTemporaryDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const char MakeAspectId[] = "RemoteLinux.MakeInstall.Make";
const char InstallRootAspectId[] = "RemoteLinux.MakeInstall.InstallRoot";
const char CleanInstallRootAspectId[] = "RemoteLinux.MakeInstall.CleanInstallRoot";
const char FullCommandLineAspectId[] = "RemoteLinux.MakeInstall.FullCommandLine";
const char CustomCommandLineAspectId[] = "RemoteLinux.MakeInstall.CustomCommandLine";

MakeInstallStep::MakeInstallStep(BuildStepList *parent, Utils::Id id) : MakeStep(parent, id)
{
    setDefaultDisplayName(displayName());

    const auto makeAspect = addAspect<ExecutableAspect>();
    makeAspect->setId(MakeAspectId);
    makeAspect->setSettingsKey(MakeAspectId);
    makeAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    makeAspect->setLabelText(tr("Command:"));
    connect(makeAspect, &ExecutableAspect::changed,
            this, &MakeInstallStep::updateCommandFromAspect);

    const auto installRootAspect = addAspect<BaseStringAspect>();
    installRootAspect->setId(InstallRootAspectId);
    installRootAspect->setSettingsKey(InstallRootAspectId);
    installRootAspect->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    installRootAspect->setExpectedKind(PathChooser::Directory);
    installRootAspect->setLabelText(tr("Install root:"));
    connect(installRootAspect, &BaseStringAspect::changed,
            this, &MakeInstallStep::updateArgsFromAspect);

    const auto cleanInstallRootAspect = addAspect<BaseBoolAspect>();
    cleanInstallRootAspect->setId(CleanInstallRootAspectId);
    cleanInstallRootAspect->setSettingsKey(CleanInstallRootAspectId);
    cleanInstallRootAspect->setLabel(tr("Clean install root first:"),
                                     BaseBoolAspect::LabelPlacement::InExtraLabel);
    cleanInstallRootAspect->setValue(false);

    const auto commandLineAspect = addAspect<BaseStringAspect>();
    commandLineAspect->setId(FullCommandLineAspectId);
    commandLineAspect->setDisplayStyle(BaseStringAspect::LabelDisplay);
    commandLineAspect->setLabelText(tr("Full command line:"));

    const auto customCommandLineAspect = addAspect<BaseStringAspect>();
    customCommandLineAspect->setId(CustomCommandLineAspectId);
    customCommandLineAspect->setSettingsKey(CustomCommandLineAspectId);
    customCommandLineAspect->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    customCommandLineAspect->setLabelText(tr("Custom command line:"));
    customCommandLineAspect->makeCheckable(BaseStringAspect::CheckBoxPlacement::Top,
                                           tr("Use custom command line instead:"),
                                           "RemoteLinux.MakeInstall.EnableCustomCommandLine");
    connect(customCommandLineAspect, &BaseStringAspect::checkedChanged,
            this, &MakeInstallStep::updateCommandFromAspect);
    connect(customCommandLineAspect, &BaseStringAspect::checkedChanged,
            this, &MakeInstallStep::updateArgsFromAspect);
    connect(customCommandLineAspect, &BaseStringAspect::checkedChanged,
            this, &MakeInstallStep::updateFromCustomCommandLineAspect);
    connect(customCommandLineAspect, &BaseStringAspect::changed,
            this, &MakeInstallStep::updateFromCustomCommandLineAspect);

    QTemporaryDir tmpDir;
    installRootAspect->setFilePath(FilePath::fromString(tmpDir.path()));
    const MakeInstallCommand cmd = target()->makeInstallCommand(tmpDir.path());
    QTC_ASSERT(!cmd.command.isEmpty(), return);
    makeAspect->setExecutable(cmd.command);
}

Utils::Id MakeInstallStep::stepId()
{
    return Constants::MakeInstallStepId;
}

QString MakeInstallStep::displayName()
{
    return tr("Install into temporary host directory");
}

BuildStepConfigWidget *MakeInstallStep::createConfigWidget()
{
    return BuildStep::createConfigWidget();
}

bool MakeInstallStep::init()
{
    if (!MakeStep::init())
        return false;
    const QString rootDirPath = installRoot().toString();
    if (rootDirPath.isEmpty()) {
        emit addTask(BuildSystemTask(Task::Error, tr("You must provide an install root.")));
        return false;
    }
    QDir rootDir(rootDirPath);
    if (cleanInstallRoot() && !rootDir.removeRecursively()) {
        emit addTask(BuildSystemTask(Task::Error,
                                        tr("The install root \"%1\" could not be cleaned.")
                                            .arg(installRoot().toUserOutput())));
        return false;
    }
    if (!rootDir.exists() && !QDir::root().mkpath(rootDirPath)) {
        emit addTask(BuildSystemTask(Task::Error,
                                        tr("The install root \"%1\" could not be created.")
                                            .arg(installRoot().toUserOutput())));
        return false;
    }
    if (this == deployConfiguration()->stepList()->steps().last()) {
        emit addTask(BuildSystemTask(Task::Warning,
                                        tr("The \"make install\" step should probably not be "
                                            "last in the list of deploy steps. "
                                            "Consider moving it up.")));
    }
    const MakeInstallCommand cmd = target()->makeInstallCommand(installRoot().toString());
    if (cmd.environment.size() > 0) {
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
        m_deploymentData = DeploymentData();
        m_deploymentData.setLocalInstallRoot(installRoot());
        QDirIterator dit(installRoot().toString(), QDir::Files | QDir::Hidden,
                         QDirIterator::Subdirectories);
        while (dit.hasNext()) {
            dit.next();
            const QFileInfo fi = dit.fileInfo();
            m_deploymentData.addFile(fi.filePath(),
                                     fi.dir().path().mid(installRoot().toString().length()));
        }
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
    return static_cast<BaseStringAspect *>(aspect(InstallRootAspectId))->filePath();
}

bool MakeInstallStep::cleanInstallRoot() const
{
    return static_cast<BaseBoolAspect *>(aspect(CleanInstallRootAspectId))->value();
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
    setUserArguments(QtcProcess::joinArgs(target()->makeInstallCommand(
        static_cast<BaseStringAspect *>(aspect(InstallRootAspectId))->filePath().toString())
                                          .arguments));
    updateFullCommandLine();
}

void MakeInstallStep::updateFullCommandLine()
{
    // FIXME: Only executable?
    static_cast<BaseStringAspect *>(aspect(FullCommandLineAspectId))->setValue(
                QDir::toNativeSeparators(
                    QtcProcess::quoteArg(makeExecutable().toString()))
                + ' '  + userArguments());
}

void MakeInstallStep::updateFromCustomCommandLineAspect()
{
    const BaseStringAspect * const aspect = customCommandLineAspect();
    if (!aspect->isChecked())
        return;
    const QStringList tokens = QtcProcess::splitArgs(aspect->value());
    setMakeCommand(tokens.isEmpty() ? FilePath() : FilePath::fromString(tokens.first()));
    setUserArguments(QtcProcess::joinArgs(tokens.mid(1)));
}

BaseStringAspect *MakeInstallStep::customCommandLineAspect() const
{
    return static_cast<BaseStringAspect *>(aspect(CustomCommandLineAspectId));
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
