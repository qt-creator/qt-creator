// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Utils {

enum OutputFormat
{
    NormalMessageFormat,
    ErrorMessageFormat,
    LogMessageFormat,
    DebugFormat,
    StdOutFormat,
    StdErrFormat,
    GeneralMessageFormat,
    NumberOfFormats // Keep this entry last.
};

} // namespace Utils
