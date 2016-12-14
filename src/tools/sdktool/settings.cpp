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

#include "settings.h"
#include "operation.h"

#include <app/app_version.h>

#include <iostream>

#include <QCoreApplication>
#include <QDir>

Settings *Settings::m_instance = 0;

Settings *Settings::instance()
{
    return m_instance;
}

Settings::Settings() :
    operation(0)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    // autodetect sdk dir:
    sdkPath = Utils::FileName::fromString(QCoreApplication::applicationDirPath());
    sdkPath.appendPath(QLatin1String(DATA_PATH));
    sdkPath = Utils::FileName::fromString(QDir::cleanPath(sdkPath.toString()));
    sdkPath.appendPath(QLatin1String(Core::Constants::IDE_SETTINGSVARIANT_STR)
                       + QLatin1String("/qtcreator"));
}

Utils::FileName Settings::getPath(const QString &file)
{
    Utils::FileName result = sdkPath;
    const QString lowerFile = file.toLower();
    const QStringList identical
            = QStringList({ "android", "cmaketools", "debuggers", "devices",
                            "profiles", "qtversions", "toolchains" });
    if (lowerFile == "cmake")
        result.appendPath("cmaketools");
    else if (lowerFile == "kits")
        result.appendPath("profiles");
    else if (lowerFile == "qtversions")
        result.appendPath("qtversion");
    else if (identical.contains(lowerFile))
        result.appendPath(lowerFile);
    else
        result.appendPath(file); // handle arbitrary file names not known yet
    result.appendString(".xml");
    return result;
}
