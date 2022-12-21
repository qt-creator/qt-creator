// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbaseclientsettings.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>

#include <QSettings>
#include <QVariant>

using namespace Utils;

namespace VcsBase {

VcsBaseSettings::VcsBaseSettings()
{
    setAutoApply(false);

    registerAspect(&binaryPath);
    binaryPath.setSettingsKey("BinaryPath");

    registerAspect(&userName);
    userName.setSettingsKey("Username");

    registerAspect(&userEmail);
    userEmail.setSettingsKey("UserEmail");

    registerAspect(&logCount);
    logCount.setSettingsKey("LogCount");
    logCount.setRange(0, 1000 * 1000);
    logCount.setDefaultValue(100);
    logCount.setLabelText(tr("Log count:"));

    registerAspect(&path);
    path.setSettingsKey("Path");

    registerAspect(&timeout);
    timeout.setSettingsKey("Timeout");
    timeout.setRange(0, 3600 * 24 * 365);
    timeout.setDefaultValue(30);
    timeout.setLabelText(tr("Timeout:"));
    timeout.setSuffix(tr("s"));
}

VcsBaseSettings::~VcsBaseSettings() = default;

FilePaths VcsBaseSettings::searchPathList() const
{
    return Utils::transform(path.value().split(HostOsInfo::pathListSeparator(), Qt::SkipEmptyParts),
                            &FilePath::fromUserInput);
}

} // namespace VcsBase
