// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "localbuild.h"

namespace Utils { class FilePath; }

namespace Axivion::Internal {

void startSingleFileAnalysis(const Utils::FilePath &file);
void cancelSingleFileAnalysis(const Utils::FilePath &filePath);

void removeFinishedAnalyses();
void shutdownAllAnalyses();

LocalBuildInfo localBuildInfoFor(const Utils::FilePath &filePath);

} // namespace Axivion::Internal
