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

#ifndef GITSETTINGS_H
#define GITSETTINGS_H

#include <vcsbase/vcsbaseclientsettings.h>

#include <QStringList>

namespace Git {
namespace Internal {

// Todo: Add user name and password?
class GitSettings : public VcsBase::VcsBaseClientSettings
{
public:
    GitSettings();

    static const QLatin1String pathKey;
    static const QLatin1String pullRebaseKey;
    static const QLatin1String omitAnnotationDateKey;
    static const QLatin1String ignoreSpaceChangesInDiffKey;
    static const QLatin1String ignoreSpaceChangesInBlameKey;
    static const QLatin1String diffPatienceKey;
    static const QLatin1String winSetHomeEnvironmentKey;
    static const QLatin1String showPrettyFormatKey;
    static const QLatin1String gitkOptionsKey;
    static const QLatin1String logDiffKey;
    static const QLatin1String repositoryBrowserCmd;

    QString gitBinaryPath(bool *ok = 0, QString *errorMessage = 0) const;

    GitSettings &operator = (const GitSettings &s);

private:
    mutable QString m_binaryPath;
};

} // namespace Internal
} // namespace Git

#endif // GITSETTINGS_H
