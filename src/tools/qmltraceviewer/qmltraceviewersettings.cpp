// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltraceviewersettings.h"

#include <QDir>

namespace QmlTraceViewer {

QmlTraceViewerSettings::QmlTraceViewerSettings()
{
    setSettingsGroup("QmlTraceViewer");

    lastTraceFile.setSettingsKey("LastTraceFile");
    lastTraceFile.setExpectedKind(Utils::PathChooser::File);
    lastTraceFile.setDefaultValue(QDir::homePath());

    windowGeometry.setSettingsKey("WindowGeometry");

    recordProcessName.setSettingsKey("RecordProcessName");
    recordProcessName.setDefaultValue("sampler-testapp");

    recordIntervalUs.setSettingsKey("RecordIntervalUs");
    recordIntervalUs.setDefaultValue(200); // ~5 kHz; 0 = as fast as possible

    recordExecutable.setSettingsKey("RecordExecutable");
    recordExecutable.setExpectedKind(Utils::PathChooser::Command);

    recordArguments.setSettingsKey("RecordArguments");

    recordWorkingDirectory.setSettingsKey("RecordWorkingDirectory");
    recordWorkingDirectory.setExpectedKind(Utils::PathChooser::ExistingDirectory);

    exitOnError.setDefaultValue(false);

    withRpc.setDefaultValue(false);
}

QmlTraceViewerSettings &settings()
{
    static QmlTraceViewerSettings theSettings;
    return theSettings;
}

} // namespace QmlTraceViewer
