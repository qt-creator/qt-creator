// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Sqlite {
class Database;
}

namespace QmlDesigner {
class ProjectStorageInterface;
class SourcePathCacheInterface;

template<typename Database>
class ProjectStorage;

template<typename Type>
using NotNullPointer = Type *;
}
