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
#include "qmakesettings.h"
#include "qmakestep.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/xcodebuildparser.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

QmakeMakeStep::QmakeMakeStep(BuildStepList *bsl, Utils::Id id)
    : MakeStep(bsl, id)
{
    if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        setClean(true);
        setUserArguments("clean");
    }
    supportDisablingForSubdirs();
}

bool QmakeMakeStep::init()
{
    const auto bc = static_cast<QmakeBuildConfiguration *>(buildConfiguration());

    const Utils::CommandLine unmodifiedMake = effectiveMakeCommand(Execution);
    const Utils::FilePath makeExecutable = unmodifiedMake.executable();
    if (makeExecutable.isEmpty())
        emit addTask(makeCommandMissingTask());

    if (!bc || makeExecutable.isEmpty()) {
        emitFaultyConfigurationMessage();
        return false;
    }

    // Ignore all but the first make step for a non-top-level build. See QTCREATORBUG-15794.
    m_ignoredNonTopLevelBuild = (bc->fileNodeBuild() || bc->subNodeBuild()) && !enabledForSubDirs();

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());

    Utils::FilePath workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->qmakeBuildSystem()->buildDir(bc->subNodeBuild()->filePath());
    else
        workingDirectory = bc->buildDirectory();
    pp->setWorkingDirectory(workingDirectory);

    // If we are cleaning, then make can fail with a error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on a already clean project
    setIgnoreReturnValue(isClean());

    Utils::CommandLine makeCmd(makeExecutable);

    QmakeProjectManager::QmakeProFileNode *subProFile = bc->subNodeBuild();
    if (subProFile) {
        QString makefile = subProFile->makefile();
        if (makefile.isEmpty())
            makefile = "Makefile";
        // Use Makefile.Debug and Makefile.Release
        // for file builds, since the rules for that are
        // only in those files.
        if (subProFile->isDebugAndRelease() && bc->fileNodeBuild()) {
            if (buildType() == QmakeBuildConfiguration::Debug)
                makefile += ".Debug";
            else
                makefile += ".Release";
        }

        if (makefile != "Makefile")
            makeCmd.addArgs({"-f", makefile});

        m_makeFileToCheck = QDir(workingDirectory.toString()).filePath(makefile);
    } else {
        QString makefile = bc->makefile();
        if (!makefile.isEmpty()) {
            makeCmd.addArgs({"-f", makefile});
            m_makeFileToCheck = QDir(workingDirectory.toString()).filePath(makefile);
        } else {
            m_makeFileToCheck = QDir(workingDirectory.toString()).filePath("Makefile");
        }
    }

    makeCmd.addArgs(unmodifiedMake.arguments(), Utils::CommandLine::Raw);

    if (bc->fileNodeBuild() && subProFile) {
        QString objectsDir = subProFile->objectsDirectory();
        if (objectsDir.isEmpty()) {
            objectsDir = bc->qmakeBuildSystem()->buildDir(subProFile->filePath()).toString();
            if (subProFile->isDebugAndRelease()) {
                if (bc->buildType() == QmakeBuildConfiguration::Debug)
                    objectsDir += "/debug";
                else
                    objectsDir += "/release";
            }
        }

        if (subProFile->isObjectParallelToSource()) {
            const Utils::FilePath sourceFileDir = bc->fileNodeBuild()->filePath().parentDir();
            const Utils::FilePath proFileDir = subProFile->proFile()->sourceDir().canonicalPath();
            if (!objectsDir.endsWith('/'))
                objectsDir += QLatin1Char('/');
            objectsDir += sourceFileDir.relativeChildPath(proFileDir).toString();
            objectsDir = QDir::cleanPath(objectsDir);
        }

        QString relObjectsDir = QDir(pp->workingDirectory().toString())
                .relativeFilePath(objectsDir);
        if (relObjectsDir == ".")
            relObjectsDir.clear();
        if (!relObjectsDir.isEmpty())
            relObjectsDir += '/';
        QString objectFile = relObjectsDir +
                bc->fileNodeBuild()->filePath().toFileInfo().baseName() +
                subProFile->objectExtension();
        makeCmd.addArg(objectFile);
    }

    pp->setEnvironment(makeEnvironment());
    pp->setCommandLine(makeCmd);
    pp->resolveAll();

    auto rootNode = dynamic_cast<QmakeProFileNode *>(project()->rootProjectNode());
    QTC_ASSERT(rootNode, return false);
    m_scriptTarget = rootNode->projectType() == ProjectType::ScriptTemplate;
    m_unalignedBuildDir = !bc->isBuildDirAtSafeLocation();

    // A user doing "make clean" indicates they want a proper rebuild, so make sure to really
    // execute qmake on the next build.
    if (stepList()->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        const auto qmakeStep = bc->qmakeStep();
        if (qmakeStep)
            qmakeStep->setForced(true);
    }

    return AbstractProcessStep::init();
}

void QmakeMakeStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    formatter->addLineParser(new ProjectExplorer::GnuMakeParser());
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(target()->kit());
    OutputTaskParser *xcodeBuildParser = nullptr;
    if (tc && tc->targetAbi().os() == Abi::DarwinOS) {
        xcodeBuildParser = new XcodebuildParser;
        formatter->addLineParser(xcodeBuildParser);
    }
    QList<Utils::OutputLineParser *> additionalParsers = target()->kit()->createOutputParsers();

    // make may cause qmake to be run, add last to make sure it has a low priority.
    additionalParsers << new QMakeParser;

    if (xcodeBuildParser) {
        for (Utils::OutputLineParser * const p : qAsConst(additionalParsers))
            p->setRedirectionDetector(xcodeBuildParser);
    }
    formatter->addLineParsers(additionalParsers);
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());

    AbstractProcessStep::setupOutputFormatter(formatter);
}

void QmakeMakeStep::doRun()
{
    if (m_scriptTarget || m_ignoredNonTopLevelBuild) {
        emit finished(true);
        return;
    }

    if (!QFileInfo::exists(m_makeFileToCheck)) {
        if (!ignoreReturnValue())
            emit addOutput(tr("Cannot find Makefile. Check your build settings."), BuildStep::OutputFormat::NormalMessage);
        const bool success = ignoreReturnValue();
        emit finished(success);
        return;
    }

    AbstractProcessStep::doRun();
}

void QmakeMakeStep::finish(bool success)
{
    if (!success && !isCanceled() && m_unalignedBuildDir
            && QmakeSettings::warnAgainstUnalignedBuildDir()) {
        const QString msg = tr("The build directory is not at the same level as the source "
                               "directory, which could be the reason for the build failure.");
        emit addTask(BuildSystemTask(Task::Warning, msg));
    }
    MakeStep::finish(success);
}

QStringList QmakeMakeStep::displayArguments() const
{
    const auto bc = static_cast<QmakeBuildConfiguration *>(buildConfiguration());
    if (bc && !bc->makefile().isEmpty())
        return {"-f", bc->makefile()};
    return {};
}

///
// QmakeMakeStepFactory
///

QmakeMakeStepFactory::QmakeMakeStepFactory()
{
    registerStep<QmakeMakeStep>(Constants::MAKESTEP_BS_ID);
    setSupportedProjectType(Constants::QMAKEPROJECT_ID);
    setDisplayName(MakeStep::defaultDisplayName());
}
