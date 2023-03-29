// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pysidebuildconfiguration.h"

#include "pythonconstants.h"
#include "pythonproject.h"
#include "pythontr.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/commandline.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

const char pySideBuildStep[] = "Python.PysideBuildStep";

PySideBuildConfigurationFactory::PySideBuildConfigurationFactory()
{
    registerBuildConfiguration<PySideBuildConfiguration>("Python.PySideBuildConfiguration");
    setSupportedProjectType(PythonProjectId);
    setSupportedProjectMimeTypeName(Constants::C_PY_MIMETYPE);
    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool) {
        BuildInfo info;
        info.displayName = "build";
        info.typeName = "build";
        info.buildDirectory = projectPath.parentDir();
        return QList<BuildInfo>{info};
    });
}

PySideBuildStepFactory::PySideBuildStepFactory()
{
    registerStep<PySideBuildStep>(pySideBuildStep);
    setSupportedProjectType(PythonProjectId);
    setDisplayName(Tr::tr("Run PySide6 project tool"));
    setFlags(BuildStep::UniqueStep);
}

PySideBuildStep::PySideBuildStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    m_pysideProject = addAspect<StringAspect>();
    m_pysideProject->setSettingsKey("Python.PySideProjectTool");
    m_pysideProject->setLabelText(Tr::tr("PySide project tool:"));
    m_pysideProject->setToolTip(Tr::tr("Enter location of PySide project tool."));
    m_pysideProject->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_pysideProject->setExpectedKind(PathChooser::Command);
    m_pysideProject->setHistoryCompleter("Python.PySideProjectTool.History");

    const FilePath pySideProjectPath = Environment::systemEnvironment().searchInPath(
        "pyside6-project");
    if (pySideProjectPath.isExecutableFile())
        m_pysideProject->setFilePath(pySideProjectPath);

    setCommandLineProvider([this] { return CommandLine(m_pysideProject->filePath(), {"build"}); });
    setWorkingDirectoryProvider([this] {
        return m_pysideProject->filePath().withNewMappedPath(target()->project()->projectDirectory()); // FIXME: new path needed?
    });
    setEnvironmentModifier([this](Environment &env) {
        env.prependOrSetPath(m_pysideProject->filePath().parentDir());
    });
}

void PySideBuildStep::updatePySideProjectPath(const Utils::FilePath &pySideProjectPath)
{
    m_pysideProject->setFilePath(pySideProjectPath);
}

void PySideBuildStep::doRun()
{
    if (processParameters()->effectiveCommand().isExecutableFile())
        AbstractProcessStep::doRun();
    else
        emit finished(true);
}

PySideBuildConfiguration::PySideBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("General"));

    setInitializer([this](const BuildInfo &) {
        buildSteps()->appendStep(pySideBuildStep);
        updateCacheAndEmitEnvironmentChanged();
    });

    updateCacheAndEmitEnvironmentChanged();
}

} // Python::Internal
