// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnutils.h"

namespace GNProjectManager::Internal {

QString resolveGNPath(QString gnPath, const Utils::FilePath &rootPath, bool stripTailingSlash)
{
    if (gnPath.startsWith("//")) {
        QString relative = gnPath.mid(2); // strip "//"
        gnPath = rootPath.pathAppended(relative).toUrlishString();
    }
    return stripTailingSlash && gnPath.endsWith('/') ? gnPath.left(gnPath.size() - 1) : gnPath;
}

} // namespace GNProjectManager::Internal
