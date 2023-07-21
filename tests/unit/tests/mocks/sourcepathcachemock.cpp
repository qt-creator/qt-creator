// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcepathcachemock.h"

namespace {

template<typename BasicId>
void incrementBasicId(BasicId &id)
{
    id = BasicId::create(id.internalId() + 1);
}
} // namespace

QmlDesigner::SourceId SourcePathCacheMock::createSourceId(QmlDesigner::SourcePathView path)
{
    static QmlDesigner::SourceId id;

    incrementBasicId(id);

    ON_CALL(*this, sourceId(Eq(path))).WillByDefault(Return(id));
    ON_CALL(*this, sourcePath(Eq(id))).WillByDefault(Return(path));

    return id;
}

SourcePathCacheMockWithPaths::SourcePathCacheMockWithPaths(QmlDesigner::SourcePathView path)
    : path{path}
{
    sourceId = createSourceId(path);
}
