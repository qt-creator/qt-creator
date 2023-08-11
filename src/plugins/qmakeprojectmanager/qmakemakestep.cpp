// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakemakestep.h"

#include "qmakebuildconfiguration.h"
#include "qmakenodes.h"
#include "qmakeparser.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeprojectmanagertr.h"
#include "qmakesettings.h"
#include "qmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/xcodebuildparser.h>

#include <utils/process.h>
#include <utils/variablechooser.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class QmakeMakeStep : public MakeStep
{
public:
    QmakeMakeStep(BuildStepList *bsl, Id id);

private:
    bool init() override;
    void setupOutputFormatter(OutputFormatter *formatter) override;
    Tasking::GroupItem runRecipe() final;
    QStringList displayArguments() const override;

    bool m_scriptTarget = false;
    FilePath m_makeFileToCheck;
    bool m_unalignedBuildDir;
    bool m_ignoredNonTopLevelBuild = false;
};

QmakeMakeStep::QmakeMakeStep(BuildStepList *bsl, Id id)
    : MakeStep(bsl, id)
{
    if (bsl->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        setIgnoreReturnValue(true);
        setUserArguments("clean");
    }
    supportDisablingForSubdirs();
}

bool QmakeMakeStep::init()
{
    // Note: This skips the Makestep::init() level.
    if (!AbstractProcessStep::init())
        return false;

    const auto bc = static_cast<QmakeBuildConfiguration *>(buildConfiguration());

    const CommandLine unmodifiedMake = effectiveMakeCommand(Execution);
    const FilePath makeExecutable = unmodifiedMake.executable();
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

    FilePath workingDirectory;
    if (bc->subNodeBuild())
        workingDirectory = bc->qmakeBuildSystem()->buildDir(bc->subNodeBuild()->filePath());
    else
        workingDirectory = bc->buildDirectory();
    pp->setWorkingDirectory(workingDirectory);

    CommandLine makeCmd(makeExecutable);

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

        m_makeFileToCheck = workingDirectory / makefile;
    } else {
        FilePath makefile = bc->makefile();
        if (!makefile.isEmpty()) {
            makeCmd.addArgs({"-f", makefile.path()});
            m_makeFileToCheck = workingDirectory / makefile.path();
        } else {
            m_makeFileToCheck = workingDirectory / "Makefile";
        }
    }

    makeCmd.addArgs(unmodifiedMake.arguments(), CommandLine::Raw);

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
            const FilePath sourceFileDir = bc->fileNodeBuild()->filePath().parentDir();
            const FilePath proFileDir = subProFile->proFile()->sourceDir().canonicalPath();
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
        QString objectFile = relObjectsDir + bc->fileNodeBuild()->filePath().baseName()
                             + subProFile->objectExtension();
        makeCmd.addArg(objectFile);
    }

    pp->setEnvironment(makeEnvironment());
    pp->setCommandLine(makeCmd);

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

    return true;
}

void QmakeMakeStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser());
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit());
    OutputTaskParser *xcodeBuildParser = nullptr;
    if (tc && tc->targetAbi().os() == Abi::DarwinOS) {
        xcodeBuildParser = new XcodebuildParser;
        formatter->addLineParser(xcodeBuildParser);
    }
    QList<OutputLineParser *> additionalParsers = kit()->createOutputParsers();

    // make may cause qmake to be run, add last to make sure it has a low priority.
    additionalParsers << new QMakeParser;

    if (xcodeBuildParser) {
        for (OutputLineParser * const p : std::as_const(additionalParsers))
            p->setRedirectionDetector(xcodeBuildParser);
    }
    formatter->addLineParsers(additionalParsers);
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());

    AbstractProcessStep::setupOutputFormatter(formatter);
}

Tasking::GroupItem QmakeMakeStep::runRecipe()
{
    using namespace Tasking;

    const auto onSetup = [this] {
        if (m_scriptTarget || m_ignoredNonTopLevelBuild)
            return SetupResult::StopWithDone;

        if (!m_makeFileToCheck.exists()) {
            const bool success = ignoreReturnValue();
            if (!success) {
                emit addOutput(Tr::tr("Cannot find Makefile. Check your build settings."),
                               OutputFormat::NormalMessage);
            }
            return success ? SetupResult::StopWithDone : SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };
    const auto onError = [this] {
        if (m_unalignedBuildDir && settings().warnAgainstUnalignedBuildDir()) {
            const QString msg = Tr::tr("The build directory is not at the same level as the source "
                                       "directory, which could be the reason for the build failure.");
            emit addTask(BuildSystemTask(Task::Warning, msg));
        }
    };

    return Group {
        ignoreReturnValue() ? finishAllAndDone : stopOnError,
        onGroupSetup(onSetup),
        onGroupError(onError),
        defaultProcessTask()
    };
}

QStringList QmakeMakeStep::displayArguments() const
{
    const auto bc = static_cast<QmakeBuildConfiguration *>(buildConfiguration());
    if (bc && !bc->makefile().isEmpty())
        return {"-f", bc->makefile().path()};
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

} // Internal
} // QmakeProjectManager
