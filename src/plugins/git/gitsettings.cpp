/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gitsettings.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <QCoreApplication>

namespace Git {
namespace Internal {

const QLatin1String GitSettings::pullRebaseKey("PullRebase");
const QLatin1String GitSettings::showTagsKey("ShowTags");
const QLatin1String GitSettings::omitAnnotationDateKey("OmitAnnotationDate");
const QLatin1String GitSettings::ignoreSpaceChangesInDiffKey("SpaceIgnorantDiff");
const QLatin1String GitSettings::ignoreSpaceChangesInBlameKey("SpaceIgnorantBlame");
const QLatin1String GitSettings::blameMoveDetection("BlameDetectMove");
const QLatin1String GitSettings::diffPatienceKey("DiffPatience");
const QLatin1String GitSettings::winSetHomeEnvironmentKey("WinSetHomeEnvironment");
const QLatin1String GitSettings::gitkOptionsKey("GitKOptions");
const QLatin1String GitSettings::logDiffKey("LogDiff");
const QLatin1String GitSettings::repositoryBrowserCmd("RepositoryBrowserCmd");
const QLatin1String GitSettings::graphLogKey("GraphLog");
const QLatin1String GitSettings::colorLogKey("ColorLog");
const QLatin1String GitSettings::firstParentKey("FirstParent");
const QLatin1String GitSettings::followRenamesKey("FollowRenames");
const QLatin1String GitSettings::lastResetIndexKey("LastResetIndex");
const QLatin1String GitSettings::refLogShowDateKey("RefLogShowDate");

GitSettings::GitSettings()
{
    setSettingsGroup("Git");

    declareKey(binaryPathKey, "git");
    declareKey(timeoutKey, Utils::HostOsInfo::isWindowsHost() ? 60 : 30);
    declareKey(pullRebaseKey, false);
    declareKey(showTagsKey, false);
    declareKey(omitAnnotationDateKey, false);
    declareKey(ignoreSpaceChangesInDiffKey, true);
    declareKey(blameMoveDetection, 0);
    declareKey(ignoreSpaceChangesInBlameKey, true);
    declareKey(diffPatienceKey, true);
    declareKey(winSetHomeEnvironmentKey, true);
    declareKey(gitkOptionsKey, QString());
    declareKey(logDiffKey, false);
    declareKey(repositoryBrowserCmd, QString());
    declareKey(graphLogKey, false);
    declareKey(colorLogKey, true);
    declareKey(firstParentKey, false);
    declareKey(followRenamesKey, true);
    declareKey(lastResetIndexKey, 0);
    declareKey(refLogShowDateKey, false);
}

Utils::FilePath GitSettings::gitExecutable(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default
    // to pathless binary
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();

    Utils::FilePath binPath = binaryPath();
    if (binPath.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("Git::Internal::GitSettings",
                                                        "The binary \"%1\" could not be located in the path \"%2\"")
                .arg(stringValue(binaryPathKey), stringValue(pathKey));
    }
    return binPath;
}

} // namespace Internal
} // namespace Git
