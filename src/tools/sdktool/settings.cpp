/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    const QStringList identical = QStringList()
            << QLatin1String("profiles")
            << QLatin1String("qtversion")
            << QLatin1String("toolchains")
            << QLatin1String("devices")
            << QLatin1String("android")
            << QLatin1String("debuggers");
    if (lowerFile == QLatin1String("kits"))
        result.appendPath(QLatin1String("profiles"));
    else if (lowerFile == QLatin1String("qtversions"))
        result.appendPath(QLatin1String("qtversion"));
    else if (identical.contains(lowerFile))
        result.appendPath(lowerFile);
    else
        return Utils::FileName();
    result.appendString(QLatin1String(".xml"));
    return result;
}
