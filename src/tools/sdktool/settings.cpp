// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settings.h"
#include "operation.h"

#include <app/app_version.h>

#include <QCoreApplication>

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
    sdkPath = Utils::FilePath::fromUserInput(QCoreApplication::applicationDirPath())
            .pathAppended(DATA_PATH).cleanPath()
            .pathAppended(Core::Constants::IDE_SETTINGSVARIANT_STR)
            .pathAppended(Core::Constants::IDE_ID);
}

Utils::FilePath Settings::getPath(const QString &file)
{
    Utils::FilePath result = sdkPath;
    const QString lowerFile = file.toLower();
    const QStringList identical = {
        "android", "cmaketools", "debuggers", "devices", "profiles", "qtversions", "toolchains", "abi"
    };
    if (lowerFile == "cmake")
        result = result.pathAppended("cmaketools");
    else if (lowerFile == "kits")
        result = result.pathAppended("profiles");
    else if (lowerFile == "qtversions")
        result = result.pathAppended("qtversion");
    else if (identical.contains(lowerFile))
        result = result.pathAppended(lowerFile);
    else
        result = result.pathAppended(file); // handle arbitrary file names not known yet
    return result.stringAppended(".xml");
}
