/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "gitsettings.h"

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
#ifdef Q_OS_WIN
    declareKey(timeoutKey, 60);
#else
    declareKey(timeoutKey, 30);
#endif
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
