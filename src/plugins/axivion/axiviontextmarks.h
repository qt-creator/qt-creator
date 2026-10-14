// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <functional>

namespace Axivion::Internal {

namespace Dto { class FileViewDto; }

enum class LineMarkerType { DashboardGlobal, DashboardLocal, SFA };

class LineIssuesOrigin
{
public:
    QString projectName;
    QString version;
    Utils::FilePath analysisPath;
};

void handleIssuesForFile(const Dto::FileViewDto &fileView, const Utils::FilePath &filePath,
                         const std::optional<Utils::FilePath> &bauhausSuite,
                         LineMarkerType type,
                         const LineIssuesOrigin &origin,
                         const QByteArray &origSource);
void clearAllMarks(LineMarkerType type);
void clearMarks(const Utils::FilePath &filePath, LineMarkerType type);
void clearMarksIf(LineMarkerType type,
                  const std::function<bool(const Utils::FilePath &filePath,
                                           const LineIssuesOrigin &origin)> &condition);
void clearMarksWithoutPathMapping(LineMarkerType type);
bool hasLineIssues(const Utils::FilePath &filePath, LineMarkerType type);
std::optional<LineIssuesOrigin> lineIssuesOrigin(const Utils::FilePath &filePath,
                                                 LineMarkerType type);
QHash<Utils::FilePath, QList<qint64>> takeAllSfaIssues();

void updateExistingMarks();

} // namespace Axivion::Internal

