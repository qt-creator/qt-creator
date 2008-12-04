/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gitsettings.h"

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

static const char *groupC = "Git";
static const char *sysEnvKeyC = "SysEnv";
static const char *pathKeyC = "Path";
static const char *logCountKeyC = "LogCount";

enum { defaultLogCount =  10 };

namespace Git {
namespace Internal {

GitSettings::GitSettings() :
    adoptPath(false),
    logCount(defaultLogCount)
{
}

void GitSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(groupC));
    adoptPath = settings->value(QLatin1String(sysEnvKeyC), false).toBool();
    path = settings->value(QLatin1String(pathKeyC), QString()).toString();
    logCount = settings->value(QLatin1String(logCountKeyC), defaultLogCount).toInt();
    settings->endGroup();
}

void GitSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(groupC));
    settings->setValue(QLatin1String(sysEnvKeyC), adoptPath);
    settings->setValue(QLatin1String(pathKeyC), path);
    settings->setValue(QLatin1String(logCountKeyC), logCount);
    settings->endGroup();
}

bool GitSettings::equals(const GitSettings &s) const
{
    return adoptPath == s.adoptPath  && path == s.path && logCount == s.logCount;
}

}
}
