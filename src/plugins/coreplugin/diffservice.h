// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QObject>

namespace Utils {
class FilePath;
class FilePaths;
}

namespace Core {

class IDocument;

class CORE_EXPORT DiffService
{
public:
    static DiffService *instance();

    DiffService();
    virtual ~DiffService();

    virtual void diffFiles(const Utils::FilePath &leftFilePath,
                           const Utils::FilePath &rightFilePath) = 0;
    virtual void diffModifiedFiles(const Utils::FilePaths &filePaths) = 0;
    // Diffs the in-memory contents of two documents. Unlike diffFiles(), this
    // works for documents that have no file on disk (e.g. scratch editors).
    virtual void diffDocuments(IDocument *leftDocument, IDocument *rightDocument) = 0;
};

} // namespace Core

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(Core::DiffService, "Core::DiffService")
QT_END_NAMESPACE
