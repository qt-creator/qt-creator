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

#include "qmakemakestep.h"

#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakenodes.h"
#include "qmakebuildconfiguration.h"
#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/xcodebuildparser.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>

using ExtensionSystem::PluginManager;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

const char MAKESTEP_BS_ID[] = "Qt4ProjectManager.MakeStep";

QmakeMakeStep::QmakeMakeStep(BuildStepList *bsl)
    : MakeStep(bsl, MAKESTEP_BS_ID)
{
    if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        setClean(true);
        setUserArguments("clean");
    }
}

QmakeBuildConfiguration *QmakeMakeStep::qmakeBuildConfiguration() const
{
    return static_cast<QmakeBuildConfiguration *>(buildConfiguration());
}

bool QmakeMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    QmakeBuildConfiguration *bc = qmakeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    const QString make = effectiveMakeCommand();
    if (make.isEmpty())
        emit addTask(makeCommandMissingTask());

    if (!bc || make.isEmpty()) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    QString workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->subNodeBuild()->buildDir();
    else
        workingDirectory = bc->buildDirectory().toString();
    pp->setWorkingDirectory(workingDirectory);

    pp->setCommand(make);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(isClean());

    QString args;

    QmakeProjectManager::QmakeProFileNode *subNode = bc->subNodeBuild();
    QmakeProjectManager::QmakeProFile *subProFile = subNode ? subNode->proFile() : nullptr;
    if (subProFile) {
        QString makefile = subProFile->makefile();
        if (makefile.isEmpty())
            makefile = "Makefile";
        // Use Makefile.Debug and Makefile.Release
        // for file builds, since the rules for that are
        // only in those files.
        if (subProFile->isDebugAndRelease() && bc->fileNodeBuild()) {
            if (bc->buildType() == QmakeBuildConfiguration::Debug)
                makefile += ".Debug";
            else
                makefile += ".Release";
        }
        if (makefile != "Makefile") {
            Utils::QtcProcess::addArg(&args, "-f");
            Utils::QtcProcess::addArg(&args, makefile);
        }
        m_makeFileToCheck = QDir(workingDirectory).filePath(makefile);
    } else {
        if (!bc->makefile().isEmpty()) {
            Utils::QtcProcess::addArg(&args, "-f");
            Utils::QtcProcess::addArg(&args, bc->makefile());
            m_makeFileToCheck = QDir(workingDirectory).filePath(bc->makefile());
        } else {
            m_makeFileToCheck = QDir(workingDirectory).filePath("Makefile");
        }
    }

    Utils::QtcProcess::addArgs(&args, allArguments());
    if (bc->fileNodeBuild() && subProFile) {
        QString objectsDir = subProFile->objectsDirectory();
        if (objectsDir.isEmpty()) {
            objectsDir = subProFile->buildDir(bc).toString();
            if (subProFile->isDebugAndRelease()) {
                if (bc->buildType() == QmakeBuildConfiguration::Debug)
                    objectsDir += "/debug";
                else
                    objectsDir += "/release";
            }
        }
        QString relObjectsDir = QDir(pp->workingDirectory()).relativeFilePath(objectsDir);
        if (relObjectsDir == ".")
            relObjectsDir.clear();
        if (!relObjectsDir.isEmpty())
            relObjectsDir += '/';
        QString objectFile = relObjectsDir +
                bc->fileNodeBuild()->filePath().toFileInfo().baseName() +
                subProFile->objectExtension();
        Utils::QtcProcess::addArg(&args, objectFile);
    }
    pp->setEnvironment(environment(bc));
    pp->setArguments(args);
    pp->resolveAll();

    setOutputParser(new ProjectExplorer::GnuMakeParser());
    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(),
                                                       ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (tc && tc->targetAbi().os() == Abi::DarwinOS)
        appendOutputParser(new XcodebuildParser);
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());
    appendOutputParser(new QMakeParser); // make may cause qmake to be run, add last to make sure
                                         // it has a low priority.

    m_scriptTarget = (static_cast<QmakeProject *>(bc->target()->project())->rootProjectNode()->projectType() == ProjectType::ScriptTemplate);

    return AbstractProcessStep::init(earlierSteps);
}

void QmakeMakeStep::run(QFutureInterface<bool> & fi)
{
    if (m_scriptTarget) {
        reportRunResult(fi, true);
        return;
    }

    if (!QFileInfo::exists(m_makeFileToCheck)) {
        if (!ignoreReturnValue())
            emit addOutput(tr("Cannot find Makefile. Check your build settings."), BuildStep::OutputFormat::NormalMessage);
        const bool success = ignoreReturnValue();
        reportRunResult(fi, success);
        return;
    }

    AbstractProcessStep::run(fi);
}

///
// QmakeMakeStepFactory
///

QmakeMakeStepFactory::QmakeMakeStepFactory()
{
    registerStep<QmakeMakeStep>(MAKESTEP_BS_ID);
    setSupportedProjectType(Constants::QMAKEPROJECT_ID);
    setDisplayName(MakeStep::defaultDisplayName());
}
