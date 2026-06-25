// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprofilersettings.h"

#include <QDir>

namespace QtProfiler {

QtProfilerSettings::QtProfilerSettings()
{
    setSettingsGroup("QtProfiler");

    lastTraceFile.setSettingsKey("LastTraceFile");
    lastTraceFile.setExpectedKind(Utils::PathChooser::File);
    lastTraceFile.setDefaultValue(QDir::homePath());

    windowGeometry.setSettingsKey("WindowGeometry");

    // recordExecutable/recordArguments are transient (no settings key): they only
    // carry a --launch command line into the active backend at startup.
    recordExecutable.setExpectedKind(Utils::PathChooser::Command);

    exitOnError.setDefaultValue(false);

    withRpc.setDefaultValue(false);
}

QtProfilerSettings &settings()
{
    static QtProfilerSettings theSettings;
    return theSettings;
}

} // namespace QtProfiler
