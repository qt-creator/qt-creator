// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "../schema/fieldlocation.h"
#include "../schema/schema.h"

#include <utils/result.h>

#include <QJsonObject>
#include <QString>

class QIODevice;

namespace CommonTraceFormat {

// Parses a CTF2 metadata stream (RFC 7464 JSON text sequences) into a Schema.
class CMNTRCEFMT_EXPORT MetadataReader
{
public:
    explicit MetadataReader(QIODevice *dev);

    Utils::Result<Schema> read();

private:
    // Split device content into RS-delimited fragments.
    QList<QByteArray> readFragments(qint64 &errorOffset);

    Utils::Result<void> parsePreamble(const QJsonObject &obj, Schema &schema, int idx);
    Utils::Result<void> parseFieldClassAlias(const QJsonObject &obj, Schema &schema, int idx);
    Utils::Result<void> parseTraceClass(const QJsonObject &obj, Schema &schema, int idx);
    Utils::Result<void> parseClockClass(const QJsonObject &obj, Schema &schema, int idx);
    Utils::Result<void> parseDataStreamClass(const QJsonObject &obj, Schema &schema, int idx);
    Utils::Result<void> parseEventRecordClass(const QJsonObject &obj, Schema &schema, int idx);

    Utils::Result<FieldClassPtr> parseFieldClass(
        const QJsonValue &val, const Schema &schema, int idx, int depth = 0) const;
    Utils::Result<FieldLocation> parseFieldLocation(const QJsonObject &obj, int idx) const;
    Utils::Result<void> parseRootStructureFieldClass(
        const QJsonObject &obj,
        const QString &key,
        const Schema &schema,
        int idx,
        FieldClassPtr &out) const;

    QIODevice *m_dev;
};

} // namespace CommonTraceFormat
