// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Core {
class VcsManager {
public:
    static QString findTopLevelForDirectory(const QString &directory)
    {
        return directory;
    }
};
} // namespace Core
