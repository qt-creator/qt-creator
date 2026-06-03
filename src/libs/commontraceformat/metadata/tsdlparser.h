// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "../schema/schema.h"

#include <utils/result.h>

#include <QByteArray>

namespace CommonTraceFormat {

// Parses CTF 1.8 TSDL (Trace Stream Description Language) text into a Schema.
class CMNTRCEFMT_EXPORT TsdlParser
{
public:
    static Utils::Result<Schema> parse(const QByteArray &tsdl);
};

} // namespace CommonTraceFormat
