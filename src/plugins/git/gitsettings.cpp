/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gitsettings.h"
#include "gitconstants.h"

#include <utils/synchronousprocess.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

static const char *groupC = "Git";
static const char *sysEnvKeyC = "SysEnv";
static const char *pathKeyC = "Path";
static const char *logCountKeyC = "LogCount";
static const char *timeoutKeyC = "TimeOut";

enum { defaultLogCount =  10 , defaultTimeOut = 30};

namespace Git {
namespace Internal {

GitSettings::GitSettings() :
    adoptPath(false),
    logCount(defaultLogCount),
    timeout(defaultTimeOut)
{
}

void GitSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    adoptPath = settings->value(QLatin1String(sysEnvKeyC), false).toBool();
    path = settings->value(QLatin1String(pathKeyC), QString()).toString();
    logCount = settings->value(QLatin1String(logCountKeyC), defaultLogCount).toInt();
    timeout = settings->value(QLatin1String(timeoutKeyC), defaultTimeOut).toInt();
    settings->endGroup();
}

void GitSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(sysEnvKeyC), adoptPath);
    settings->setValue(QLatin1String(pathKeyC), path);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->setValue(QLatin1String(timeoutKeyC), timeout);
    settings->endGroup();
}

bool GitSettings::equals(const GitSettings &s) const
{
    return adoptPath == s.adoptPath && path == s.path && logCount == s.logCount && timeout == s.timeout;
}

QString GitSettings::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    // Locate binary in path if one is specified, otherwise default
    // to pathless binary
    if (ok)
        *ok = true;
    if (errorMessage)
        errorMessage->clear();
    const QString binary = QLatin1String(Constants::GIT_BINARY);
    // Easy, git is assumed to be elsewhere accessible
    if (!adoptPath)
        return binary;
    // Search in path?
    const QString pathBinary = Core::Utils::SynchronousProcess::locateBinary(path, binary);
    if (pathBinary.isEmpty()) {
        if (ok)
            *ok = false;
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("GitSettings",
                                                        "The binary '%1' could not be located in the path '%2'").arg(binary, path);
        return binary;
    }
    return pathBinary;
}

}
}
