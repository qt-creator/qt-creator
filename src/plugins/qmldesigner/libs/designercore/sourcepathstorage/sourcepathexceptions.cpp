// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "sourcepathexceptions.h"

#include <tracing/qmldesignertracing.h>

    namespace QmlDesigner {

using namespace NanotraceHR::Literals;
using NanotraceHR::keyValue;
using SourcePathStorageTracing::category;

const char *SourcePathError::what() const noexcept
{
    return "Source path error!";
}

NoSourcePathForInvalidSourceId::NoSourcePathForInvalidSourceId()
{
    category().threadEvent("NoSourcePathForInvalidSourceId");
}

const char *NoSourcePathForInvalidSourceId::what() const noexcept
{
    return "You cannot get a file path for an invalid file path id!";
}

NoSourceContextPathForInvalidSourceContextId::NoSourceContextPathForInvalidSourceContextId()
{
    category().threadEvent("NoSourceContextPathForInvalidSourceContextId");
}

const char *NoSourceContextPathForInvalidSourceContextId::what() const noexcept
{
    return "You cannot get a directory path for an invalid directory path id!";
}

SourceContextIdDoesNotExists::SourceContextIdDoesNotExists()
{
    category().threadEvent("SourceContextIdDoesNotExists");
}

const char *SourceContextIdDoesNotExists::what() const noexcept
{
    return "The source context id does not exist in the database!";
}

SourceNameIdDoesNotExists::SourceNameIdDoesNotExists()
{
    category().threadEvent("SourceNameIdDoesNotExists");
}

const char *SourceNameIdDoesNotExists::what() const noexcept
{
    return "The source id does not exist in the database!";
}

} // namespace QmlDesigner
