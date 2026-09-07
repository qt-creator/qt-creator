// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "headerdependencies.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <QtTaskTree/QTaskTree>

#include <functional>

namespace CMakeProjectManager::Internal {

enum class DependencyDialect {
    Gcc,
    Msvc
};

class HeaderScanUnit
{
public:
    Utils::FilePath compiler;
    Utils::FilePath source;
    QStringList arguments;

    SourceScan toScan() const;
};

using HeaderScanUnits = QList<HeaderScanUnit>;

class HeaderScanSettings
{
public:
    DependencyDialect dialect = DependencyDialect::Gcc;
    Utils::FilePath workingDirectory;
    Utils::Environment environment;
    QString showIncludesPrefix;
};

using ScanResultHandler
    = std::function<void(const SourceScan &scan, const Utils::FilePaths &dependencies)>;

DependencyDialect dependencyDialect(Utils::Id toolchainType);

quint64 commandHash(const Utils::FilePath &compiler, const QStringList &arguments);

QStringList launderScanArguments(const QStringList &arguments, DependencyDialect dialect);

Utils::CommandLine dependencyScanCommand(const HeaderScanUnit &unit, DependencyDialect dialect);

Utils::FilePaths parseMakeDependencies(const QString &output,
                                       const Utils::FilePath &workingDirectory);

HeaderScanSettings withShowIncludesPrefix(const HeaderScanSettings &settings);

Utils::FilePaths parseShowIncludes(const QString &output,
                                   const QString &prefix,
                                   const Utils::FilePath &workingDirectory);

QtTaskTree::ExecutableItem headerDependencyScanRecipe(const HeaderScanUnits &units,
                                                      const HeaderScanSettings &scanSettings,
                                                      const ScanResultHandler &handler);

} // namespace CMakeProjectManager::Internal
