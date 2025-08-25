// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QTimer>
#include <QUrl>

namespace Tasking { class Group; }

namespace Python::Internal {

class PipPackageInfo
{
public:
    QString name;
    QString version;
    QString summary;
    QUrl homePage;
    QString author;
    QString authorEmail;
    QString license;
    Utils::FilePath location;
    QStringList requiresPackage;
    QStringList requiredByPackage;
    Utils::FilePaths files;

    void parseField(const QString &field, const QStringList &value);
};

class PipPackage
{
public:
    explicit PipPackage(const QString &packageName = {},
                        const QString &displayName = {},
                        const QString &version = {})
        : packageName(packageName)
        , displayName(displayName.isEmpty() ? packageName : displayName)
        , version(version)
    {}
    QString packageName;
    QString displayName;
    QString version;
};

class PipInstallerData
{
public:
    QString packagesDisplayName() const;

    Utils::FilePath python;
    Utils::FilePath workingDirectory;
    Utils::FilePath requirementsFile;
    Utils::FilePath targetPath;
    QList<PipPackage> packages;
    bool upgrade = false;
    bool silent = false;
};

Tasking::Group pipInstallerTask(const PipInstallerData &data);

} // Python::Internal
