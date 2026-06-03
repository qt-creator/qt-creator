// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fieldclass.h"
#include "fieldlocation.h"

#include <QString>

namespace CommonTraceFormat {

enum class StringEncoding { Utf8, Utf16Be, Utf16Le, Utf32Be, Utf32Le };

struct CMNTRCEFMT_EXPORT NullTerminatedStringFC : FieldClass
{
    StringEncoding encoding = StringEncoding::Utf8;
    explicit NullTerminatedStringFC()
        : FieldClass(FieldClassKind::NullTerminatedString)
    {}
};

struct CMNTRCEFMT_EXPORT StaticLengthStringFC : FieldClass
{
    qint64 length = 0; // bytes (spec 5.3.13: JSON integer >= 0)
    StringEncoding encoding = StringEncoding::Utf8;
    explicit StaticLengthStringFC()
        : FieldClass(FieldClassKind::StaticLengthString)
    {}
};

struct CMNTRCEFMT_EXPORT DynamicLengthStringFC : FieldClass
{
    FieldLocation lengthFieldLocation;
    StringEncoding encoding = StringEncoding::Utf8;
    explicit DynamicLengthStringFC()
        : FieldClass(FieldClassKind::DynamicLengthString)
    {}
};

} // namespace CommonTraceFormat
