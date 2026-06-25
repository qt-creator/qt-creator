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

    // recordExecutable/recordArguments are transient (no settings key): they only
    // carry a --launch command line into the active backend at startup.
    recordExecutable.setExpectedKind(Utils::PathChooser::Command);

    exitOnError.setDefaultValue(false);

    withRpc.setDefaultValue(false);
}

QmlTraceViewerSettings &settings()
{
    static QmlTraceViewerSettings theSettings;
    return theSettings;
}

} // namespace QmlTraceViewer
