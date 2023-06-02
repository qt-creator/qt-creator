// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"

#include <app/app_version.h>

#include <QCoreApplication>
#include <QDir>

static Settings *m_instance = nullptr;

Settings *Settings::instance()
{
    return m_instance;
}

Settings::Settings()
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    // autodetect sdk dir:
    sdkPath = QDir::cleanPath(QCoreApplication::applicationDirPath()
        + '/' + DATA_PATH
        + '/' + Core::Constants::IDE_SETTINGSVARIANT_STR
        + '/' + Core::Constants::IDE_ID);
}

QString Settings::getPath(const QString &file)
{
    QString result = sdkPath;
    const QString lowerFile = file.toLower();
    const QStringList identical = {
        "android", "cmaketools", "debuggers", "devices", "profiles", "qtversions", "toolchains", "abi"
    };
    if (lowerFile == "cmake")
        result += "/cmaketools";
    else if (lowerFile == "kits")
        result += "/profiles";
    else if (lowerFile == "qtversions")
        result += "/qtversion";
    else if (identical.contains(lowerFile))
        result += '/' + lowerFile;
    else
        result += '/' + file; // handle arbitrary file names not known yet
    return result += ".xml";
}
