// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Dto { class FileViewDto; }

namespace Axivion::Internal {

enum class LineMarkerType { Dashboard, SFA };

// bauhausSuite == std::nullopt used for LineMarkerType::Dashboard, otherwise LineMarkerType::SFA
void handleIssuesForFile(const Dto::FileViewDto &fileView, const Utils::FilePath &filePath,
                         const std::optional<Utils::FilePath> &bauhausSuite);
void clearAllMarks(LineMarkerType type);
void clearMarks(const Utils::FilePath &filePath, LineMarkerType type);
bool hasLineIssues(const Utils::FilePath &filePath, LineMarkerType type);

void updateExistingMarks();

} // namespace Axivion::Internal

