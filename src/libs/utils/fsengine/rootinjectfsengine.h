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
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) override
    {
        std::unique_ptr<QAbstractFileEngineIterator> baseIterator(
            QFSFileEngine::beginEntryList(filters, filterNames));
        return new FileIteratorWrapper(std::move(baseIterator), filters, filterNames);
    }
};

} // namespace Internal
} // namespace Utils
