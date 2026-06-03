// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fieldclass.h"
#include "fieldlocation.h"

#include <QList>
#include <QString>

namespace CommonTraceFormat {

// Roles a static-length BLOB field class may carry (spec 5.3.3).
enum class BlobRole { None, MetadataStreamUuid };

struct CMNTRCEFMT_EXPORT StaticLengthBlobFC : FieldClass
{
    qint64 length = 0; // bytes (spec 5.3.16: JSON integer >= 0)
    QString mediaType = QStringLiteral("application/octet-stream");
    QList<BlobRole> roles;
    explicit StaticLengthBlobFC()
        : FieldClass(FieldClassKind::StaticLengthBlob)
    {}
};

struct CMNTRCEFMT_EXPORT DynamicLengthBlobFC : FieldClass
{
    FieldLocation lengthFieldLocation;
    QString mediaType = QStringLiteral("application/octet-stream");
    explicit DynamicLengthBlobFC()
        : FieldClass(FieldClassKind::DynamicLengthBlob)
    {}
};

} // namespace CommonTraceFormat
