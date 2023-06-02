// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QList>
#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProcessInfo
{
public:
    qint64 processId = 0;
    QString executable;
    QString commandLine;

    bool operator<(const ProcessInfo &other) const;

    static QList<ProcessInfo> processInfoList(const Utils::FilePath &deviceRoot = Utils::FilePath());
};

} // namespace Utils
