// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace SilverSearcher {
namespace Internal {

class OutputParserTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();
};

} // namespace Internal
} // namespace SilverSearcher
