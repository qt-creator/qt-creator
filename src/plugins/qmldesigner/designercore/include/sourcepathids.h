// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sqlite/sqliteids.h>

#include <utils/span.h>

#include <QVarLengthArray>

namespace QmlDesigner {

enum class SourcePathIdType {
    SourceName,
    SourceContext,

};

using SourceContextId = Sqlite::BasicId<SourcePathIdType::SourceContext, int>;
using SourceContextIds = std::vector<SourceContextId>;
template<std::size_t size>
using SmallSourceContextIds = QVarLengthArray<SourceContextId, size>;

using SourceNameId = Sqlite::BasicId<SourcePathIdType::SourceName, int>;
using SourceNameIds = std::vector<SourceNameId>;
template<std::size_t size>
using SmallSourceNameIds = QVarLengthArray<SourceNameId, size>;

using SourceId = Sqlite::CompoundBasicId<SourcePathIdType::SourceName, SourcePathIdType::SourceContext>;
using SourceIds = std::vector<SourceId>;
template<std::size_t size>
using SmallSourceIds = QVarLengthArray<SourceId, size>;

} // namespace QmlDesigner
