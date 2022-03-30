/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "pysidebuildconfiguration.h"

#include "pipsupport.h"
#include "pythonconstants.h"
#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/commandline.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python {
namespace Internal {

constexpr char pySideBuildStep[] = "Python.PysideBuildStep";

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
    setDisplayName(tr("Run PySide6 project tool"));
    setFlags(BuildStepInfo::UniqueStep);
}

PySideBuildStep::PySideBuildStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    m_pysideProject = addAspect<StringAspect>();
    m_pysideProject->setSettingsKey("Python.PySideProjectTool");
    m_pysideProject->setLabelText(tr("PySide project tool:"));
    m_pysideProject->setToolTip(tr("Enter location of PySide project tool."));
    m_pysideProject->setDisplayStyle(StringAspect::PathChooserDisplay);
    m_pysideProject->setExpectedKind(PathChooser::Command);
    m_pysideProject->setHistoryCompleter("Python.PySideProjectTool.History");

    const FilePath pySideProjectPath = Environment::systemEnvironment().searchInPath(
        "pyside6-project");
    if (pySideProjectPath.isExecutableFile())
        m_pysideProject->setFilePath(pySideProjectPath);

    setCommandLineProvider([this] { return CommandLine(m_pysideProject->filePath(), {"build"}); });
    setWorkingDirectoryProvider([this] { return target()->project()->projectDirectory(); });
}

void PySideBuildStep::updateInterpreter(const Utils::FilePath &python)
{
    Utils::FilePath pySideProjectPath;
    const PipPackage pySide6Package("PySide6");
    const PipPackageInfo info = pySide6Package.info(python);
    for (const FilePath &file : qAsConst(info.files)) {
        if (file.fileName() == HostOsInfo::withExecutableSuffix("pyside6-project")) {
            pySideProjectPath = info.location.resolvePath(file);
            pySideProjectPath = pySideProjectPath.cleanPath();
            break;
        }
    }

    if (!pySideProjectPath.isExecutableFile())
        pySideProjectPath = Environment::systemEnvironment().searchInPath("pyside6-project");

    if (pySideProjectPath.isExecutableFile())
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
    setConfigWidgetDisplayName(tr("General"));

    setInitializer([this](const BuildInfo &) {
        buildSteps()->appendStep(pySideBuildStep);
        updateCacheAndEmitEnvironmentChanged();
    });

    updateCacheAndEmitEnvironmentChanged();
}

} // namespace Internal
} // namespace Python
