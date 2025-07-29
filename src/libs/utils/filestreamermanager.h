// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filepath.h"

namespace Utils::FileStreamerManager {

QTCREATOR_UTILS_EXPORT FileStreamHandle copy(const Continuation<> &cont,
                                             const FilePath &source,
                                             const FilePath &destination);

QTCREATOR_UTILS_EXPORT FileStreamHandle read(const Continuation<QByteArray> &cont,
                                             const FilePath &source);

QTCREATOR_UTILS_EXPORT FileStreamHandle write(const Continuation<qint64> &cont,
                                              const FilePath &destination,
                                              const QByteArray &data);

// If called from the same thread that started the task, no continuation is going to be called.
QTCREATOR_UTILS_EXPORT void stop(FileStreamHandle handle);
QTCREATOR_UTILS_EXPORT void stopAll();

} // namespace Utils::FileStreamerManager
