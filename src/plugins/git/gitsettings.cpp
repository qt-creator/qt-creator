/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gitsettings.h"

#include <utils/hostosinfo.h>
#include <QCoreApplication>

namespace Git {
namespace Internal {

const QLatin1String GitSettings::pullRebaseKey("PullRebase");
const QLatin1String GitSettings::omitAnnotationDateKey("OmitAnnotationDate");
const QLatin1String GitSettings::ignoreSpaceChangesInDiffKey("SpaceIgnorantDiff");
const QLatin1String GitSettings::ignoreSpaceChangesInBlameKey("SpaceIgnorantBlame");
const QLatin1String GitSettings::diffPatienceKey("DiffPatience");
const QLatin1String GitSettings::winSetHomeEnvironmentKey("WinSetHomeEnvironment");
const QLatin1String GitSettings::showPrettyFormatKey("DiffPrettyFormat");
const QLatin1String GitSettings::gitkOptionsKey("GitKOptions");
const QLatin1String GitSettings::logDiffKey("LogDiff");
const QLatin1String GitSettings::repositoryBrowserCmd("RepositoryBrowserCmd");

GitSettings::GitSettings()
{
    setSettingsGroup(QLatin1String("Git"));

    declareKey(binaryPathKey, QLatin1String("git"));
    declareKey(timeoutKey, Utils::HostOsInfo::isWindowsHost() ? 60 : 30);
    declareKey(pullRebaseKey, false);
    declareKey(omitAnnotationDateKey, false);
    declareKey(ignoreSpaceChangesInDiffKey, true);
    declareKey(ignoreSpaceChangesInBlameKey, true);
    declareKey(diffPatienceKey, true);
    declareKey(winSetHomeEnvironmentKey, false);
    declareKey(gitkOptionsKey, QString());
    declareKey(showPrettyFormatKey, 2);
    declareKey(logDiffKey, false);
    declareKey(repositoryBrowserCmd, QString());
}

QString GitSettings::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default
    // to pathless binary
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();

    QString binPath = binaryPath();
    if (binPath.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("Git::Internal::GitSettings",
                                                        "The binary '%1' could not be located in the path '%2'")
                .arg(stringValue(binaryPathKey), stringValue(pathKey));
    }
    return binPath;
}

GitSettings &GitSettings::operator = (const GitSettings &s)
{
    VcsBaseClientSettings::operator =(s);
    return *this;
}

} // namespace Internal
} // namespace Git
