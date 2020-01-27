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

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
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

MakeInstallStep::MakeInstallStep(BuildStepList *parent) : MakeStep(parent, stepId())
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
    cleanInstallRootAspect->setLabel(tr("Clean install root first"));
    cleanInstallRootAspect->setValue(false);

    const auto commandLineAspect = addAspect<BaseStringAspect>();
    commandLineAspect->setId(FullCommandLineAspectId);
    commandLineAspect->setDisplayStyle(BaseStringAspect::LabelDisplay);
    commandLineAspect->setLabelText(tr("Full command line:"));

    QTemporaryDir tmpDir;
    installRootAspect->setFilePath(FilePath::fromString(tmpDir.path()));
    const MakeInstallCommand cmd = target()->makeInstallCommand(tmpDir.path());
    QTC_ASSERT(!cmd.command.isEmpty(), return);
    makeAspect->setExecutable(cmd.command);
}

Core::Id MakeInstallStep::stepId()
{
    return "RemoteLinux.MakeInstall";
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
        emit addTask(Task(Task::Error, tr("You must provide an install root."), FilePath(), -1,
                     Constants::TASK_CATEGORY_BUILDSYSTEM));
        return false;
    }
    QDir rootDir(rootDirPath);
    if (cleanInstallRoot() && !rootDir.removeRecursively()) {
        emit addTask(Task(Task::Error, tr("The install root \"%1\" could not be cleaned.")
                          .arg(installRoot().toUserOutput()),
                          FilePath(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM));
        return false;
    }
    if (!rootDir.exists() && !QDir::root().mkpath(rootDirPath)) {
        emit addTask(Task(Task::Error, tr("The install root \"%1\" could not be created.")
                          .arg(installRoot().toUserOutput()),
                          FilePath(), -1, Constants::TASK_CATEGORY_BUILDSYSTEM));
        return false;
    }
    if (this == deployConfiguration()->stepList()->steps().last()) {
        emit addTask(Task(Task::Warning, tr("The \"make install\" step should probably not be "
                                            "last in the list of deploy steps. "
                                            "Consider moving it up."), FilePath(), -1,
                          Constants::TASK_CATEGORY_BUILDSYSTEM));
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
    const auto buildStep = buildConfiguration()
            ->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD)
            ->firstOfType<AbstractProcessStep>();
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
        target()->setDeploymentData(m_deploymentData);
    } else if (m_noInstallTarget && m_isCmakeProject) {
        emit addTask(Task(Task::Warning, tr("You need to add an install statement to your "
                                            "CMakeLists.txt file for deployment to work."),
                          FilePath(), -1, ProjectExplorer::Constants::TASK_CATEGORY_DEPLOYMENT));
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
    setMakeCommand(aspect<ExecutableAspect>()->executable());
    updateFullCommandLine();
}

void MakeInstallStep::updateArgsFromAspect()
{
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

bool MakeInstallStep::fromMap(const QVariantMap &map)
{
    if (!MakeStep::fromMap(map))
        return false;
    updateCommandFromAspect();
    updateArgsFromAspect();
    return true;
}

} // namespace RemoteLinux
