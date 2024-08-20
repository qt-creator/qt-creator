// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"

namespace QmlDesigner {

class ProjectStorageObserver
{
public:
    virtual void removedTypeIds(const TypeIds &removedTypeIds) = 0;
    virtual void exportedTypesChanged() = 0;
};
} // namespace QmlDesigner
