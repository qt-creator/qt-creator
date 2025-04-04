// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nanotrace/nanotracehr.h>
#include <sqlite/sourcelocation.h>

namespace QmlDesigner {

namespace ModelTracing {

#ifdef ENABLE_MODEL_TRACING
class SourceLocation : public Sqlite::source_location
{
public:
    consteval SourceLocation(const char *fileName = __builtin_FILE(),
                             const char *functionName = __builtin_FUNCTION(),
                             const uint_least32_t line = __builtin_LINE())
        : Sqlite::source_location(Sqlite::source_location::current(fileName, functionName, line))
    {}

    template<typename String>
    friend void convertToString(String &string, SourceLocation sourceLocation)
    {
        using NanotraceHR::dictonary;
        using NanotraceHR::keyValue;
        auto dict = dictonary(keyValue("file", sourceLocation.file_name()),
                              keyValue("function", sourceLocation.function_name()),
                              keyValue("line", sourceLocation.line()));
        convertToString(string, dict);

        string.append(',');
        convertToString(string, "id");
        string.append(':');
        string.append('\"');
        string.append(sourceLocation.file_name());
        string.append(':');
        string.append(sourceLocation.line());
        string.append('\"');
    }
};

#else
class SourceLocation
{
public:
    template<typename String>
    friend void convertToString(String &, SourceLocation)
    {}
};
#endif

} // namespace ModelTracing

} // namespace QmlDesigner
