// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Axivion::Internal {

namespace Dto { class FileViewDto; }

enum class LineMarkerType { Dashboard, SFA };

// bauhausSuite == std::nullopt used for LineMarkerType::Dashboard, otherwise LineMarkerType::SFA
void handleIssuesForFile(const Dto::FileViewDto &fileView, const Utils::FilePath &filePath,
                         const std::optional<Utils::FilePath> &bauhausSuite,
                         const QByteArray &origSource);
void clearAllMarks(LineMarkerType type);
void clearMarks(const Utils::FilePath &filePath, LineMarkerType type);
bool hasLineIssues(const Utils::FilePath &filePath, LineMarkerType type);

void updateExistingMarks();

} // namespace Axivion::Internal

