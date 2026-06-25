// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sampler.h"

#include "profilertr.h"

#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>

using namespace Profiler;
using namespace Utils;

namespace QmlProfiler::Internal {

SamplerSettings::SamplerSettings()
{
    setAutoApply(true);

    executable.setSettingsKey("Executable");
    executable.setExpectedKind(PathChooser::Command);
    executable.setLabelText(Tr::tr("Executable:"));

    arguments.setSettingsKey("Arguments");
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    arguments.setLabelText(Tr::tr("Arguments:"));

    workingDirectory.setSettingsKey("WorkingDirectory");
    workingDirectory.setExpectedKind(PathChooser::ExistingDirectory);
    workingDirectory.setLabelText(Tr::tr("Working directory:"));
}

Result<> SamplerSettings::fillLaunch(RecordingSession &session) const
{
    const FilePath exe = executable();
    if (exe.isEmpty())
        return ResultError(Tr::tr("Set an executable to launch, or choose a different start mode."));
    session.launchCommand = CommandLine(exe,
                                        ProcessArgs::splitArgs(arguments(), HostOsInfo::hostOs()));
    session.launchWorkingDir = workingDirectory();
    return ResultOk;
}

} // namespace QmlProfiler::Internal
