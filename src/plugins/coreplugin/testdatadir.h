// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QString>

#define QTC_DECLARE_MYTESTDATADIR(PATH)                                          \
    class MyTestDataDir : public Core::Tests::TestDataDir                        \
    {                                                                            \
    public:                                                                      \
        MyTestDataDir(const QString &testDataDirectory = QString())              \
            : TestDataDir(QLatin1String(SRCDIR "/" PATH) + testDataDirectory) {} \
    };

namespace Utils { class FilePath; }

namespace Core {
namespace Tests {

class CORE_EXPORT TestDataDir
{
public:
    TestDataDir(const QString &directory);

    QString file(const QString &fileName) const;
    Utils::FilePath filePath(const QString &fileName) const;
    QString directory(const QString &subdir = QString(), bool clean = true) const;

    QString path() const;

private:
    QString m_directory;
};

} // namespace Tests
} // namespace Core
