// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprofilersettings.h"

#include <utils/algorithm.h>
#include <utils/pathchooser.h>

namespace QtProfiler {

constexpr int maxRecentFiles = 10;

QtProfilerSettings::QtProfilerSettings()
{
    setSettingsGroup("QtProfiler");

    lastTraceFile.setSettingsKey("LastTraceFile");
    lastTraceFile.setExpectedKind(Utils::PathChooserKind::File);

    recentFiles.setSettingsKey("RecentFiles");

    windowGeometry.setSettingsKey("WindowGeometry");

    // recordExecutable/recordArguments are transient (no settings key): they only
    // carry a --launch command line into the active backend at startup.
    recordExecutable.setExpectedKind(Utils::PathChooserKind::Command);

    exitOnError.setDefaultValue(false);

    withRpc.setDefaultValue(false);
}

void QtProfilerSettings::addRecentFile(const Utils::FilePath &filePath)
{
    const Utils::FilePath absolutePath = filePath.absoluteFilePath();
    Utils::FilePaths files = Utils::filtered(recentFiles(),
                                             [&absolutePath](const Utils::FilePath &f) {
        return f != absolutePath;
    });
    if (absolutePath.exists())
        files.prepend(absolutePath);
    recentFiles.setValue(Utils::transform(files.mid(0, maxRecentFiles),
                                          &Utils::FilePath::toUserOutput));
}

Utils::FilePaths QtProfilerSettings::sanitizedRecentFiles()
{
    const Utils::FilePaths storedFiles = recentFiles();
    const Utils::FilePaths files = Utils::filtered(storedFiles, &Utils::FilePath::exists);
    if (files.size() != storedFiles.size())
        recentFiles.setValue(Utils::transform(files, &Utils::FilePath::toUserOutput));
    return files;
}

QtProfilerSettings &settings()
{
    static QtProfilerSettings theSettings;
    return theSettings;
}

} // namespace QtProfiler
