// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <projectexplorer/abi.h>
#include <utils/filepath.h>

#include <QLoggingCategory>

namespace QtSupport {
class QtVersion;

namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(abiDetect)

ProjectExplorer::Abis qtAbisFromJson(
    const QtVersion &qtVersion, const Utils::FilePaths &possibleLocations);

} // namespace QtSupport
} // namespace Internal
