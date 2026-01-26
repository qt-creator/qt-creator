// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "localbuild.h"

namespace Utils { class FilePath; }

namespace Axivion::Internal {

void startSingleFileAnalysis(const Utils::FilePath &file, const QString &projectName);
void cancelSingleFileAnalysis(const QString &fileName, const QString &projectName);

void removeFinishedAnalyses();
void shutdownAllAnalyses();

bool hasRunningSingleFileAnalysis(const QString &projectName);

LocalBuildInfo localBuildInfoFor(const QString &projectName, const QString &fileName);

} // namespace Axivion::Internal
