// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QVariantMap>

namespace Squish {
namespace Internal {

class SquishUtils
{
public:
    static QStringList validTestCases(const QString &baseDirectory);
    static QVariantMap readSuiteConf(const QString &suiteConfPath);
    static QString objectsMapPath(const QString &suitePath);
    static QString extensionForLanguage(const QString &language);

private:
    SquishUtils() {}
};

} // namespace Internal
} // namespace Squish
