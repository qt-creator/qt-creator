// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>

namespace Nim {

class NimParser : public ProjectExplorer::OutputTaskParser
{
    Result handleLine(const QString &line, Utils::OutputFormat) override;
};

#ifdef WITH_TESTS
class NimParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void testNimParser_data();
    void testNimParser();
};
#endif

} // Nim
