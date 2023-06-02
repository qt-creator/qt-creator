// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

enum class FileStreamHandle : int {};

class QTCREATOR_UTILS_EXPORT FileStreamerManager
{
public:
    static FileStreamHandle copy(const FilePath &source, const FilePath &destination,
                                 const CopyContinuation &cont);
    static FileStreamHandle copy(const FilePath &source, const FilePath &destination,
                                 QObject *context, const CopyContinuation &cont);

    static FileStreamHandle read(const FilePath &source, const ReadContinuation &cont = {});
    static FileStreamHandle read(const FilePath &source, QObject *context,
                                 const ReadContinuation &cont = {});

    static FileStreamHandle write(const FilePath &destination, const QByteArray &data,
                                  const WriteContinuation &cont = {});
    static FileStreamHandle write(const FilePath &destination, const QByteArray &data,
                                  QObject *context, const WriteContinuation &cont = {});

    // If called from the same thread that started the task, no continuation is going to be called.
    static void stop(FileStreamHandle handle);
    static void stopAll();
};

} // namespace Utils
