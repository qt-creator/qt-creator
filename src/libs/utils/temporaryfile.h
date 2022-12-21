// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QTemporaryFile>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TemporaryFile : public QTemporaryFile
{
public:
    explicit TemporaryFile(const QString &pattern);
    ~TemporaryFile();
};

} // namespace Utils
