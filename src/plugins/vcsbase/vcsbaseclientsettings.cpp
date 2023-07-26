// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbaseclientsettings.h"

#include "vcsbasetr.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>

using namespace Utils;

namespace VcsBase {

VcsBaseSettings::VcsBaseSettings()
{
    binaryPath.setSettingsKey("BinaryPath");

    userName.setSettingsKey("Username");

    userEmail.setSettingsKey("UserEmail");

    logCount.setSettingsKey("LogCount");
    logCount.setRange(0, 1000 * 1000);
    logCount.setDefaultValue(100);
    logCount.setLabelText(Tr::tr("Log count:"));

    path.setSettingsKey("Path");

    timeout.setSettingsKey("Timeout");
    timeout.setRange(0, 3600 * 24 * 365);
    timeout.setDefaultValue(30);
    timeout.setLabelText(Tr::tr("Timeout:"));
    timeout.setSuffix(Tr::tr("s"));
}

VcsBaseSettings::~VcsBaseSettings() = default;

FilePaths VcsBaseSettings::searchPathList() const
{
    // FIXME: Filepathify
    return Utils::transform(path().split(HostOsInfo::pathListSeparator(), Qt::SkipEmptyParts),
                            &FilePath::fromUserInput);
}

} // namespace VcsBase
