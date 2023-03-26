// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>

#include <QHash>
#include <QMap>

namespace Core { class IDocument; }

namespace ClangTools {
namespace Internal {

class VirtualFileSystemOverlay
{
public:
    VirtualFileSystemOverlay(const QString &rootPattern);

    void update();

    Utils::FilePath overlayFilePath() const;
    Utils::FilePath autoSavedFilePath(Core::IDocument *doc) const;
    Utils::FilePath originalFilePath(const Utils::FilePath &file) const;

private:
    Utils::TemporaryDirectory m_root;
    Utils::FilePath m_overlayFilePath;
    struct AutoSavedPath
    {
        int revision = -1;
        Utils::FilePath path;
    };

    QHash<Core::IDocument *, AutoSavedPath> m_saved;
    QMap<Utils::FilePath, Utils::FilePath> m_mapping;
};

} // namespace Internal
} // namespace ClangTools
