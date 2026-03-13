// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Utils {
class FilePath;
} // namespace Utils

namespace QmlTraceViewer::RPC {

void notifyTraceFileLoadingStarted(const Utils::FilePath &file);
void notifyTraceFileLoadingFinished(const Utils::FilePath &file, const QString &errorMessage);
void notifyTraceEventSelected(const QString &sourceFilePath, int line, int column);
void notifyTraceDiscarded();

} // namespace QmlTraceViewer::RPC
