// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include <QDir>

static QDir testDataRootDir(QLatin1String(TESTDATA_DIR));

inline void PrintTo(const QString &qString, ::std::ostream *os)
{
    *os << qUtf8Printable(qString);
}
