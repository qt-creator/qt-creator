// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fileiteratordevicesappender.h"

#include <QtCore/private/qfsfileengine_p.h>

namespace Utils {
namespace Internal {

class RootInjectFSEngine : public QFSFileEngine
{
public:
    using QFSFileEngine::QFSFileEngine;

public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    IteratorUniquePtr beginEntryList(const QString &path,
                                     QDir::Filters filters,
                                     const QStringList &filterNames) override
    {
        return std::make_unique<FileIteratorWrapper>(
            QFSFileEngine::beginEntryList(path, filters, filterNames));
    }
#else
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override
    {
        std::unique_ptr<QAbstractFileEngineIterator> baseIterator(
            QFSFileEngine::beginEntryList(filters, filterNames));
        return new FileIteratorWrapper(std::move(baseIterator));
    }
#endif
};

} // namespace Internal
} // namespace Utils
