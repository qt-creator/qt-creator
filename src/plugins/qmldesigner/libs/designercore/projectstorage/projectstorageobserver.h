// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstorageids.h"
#include "projectstorageinfotypes.h"

namespace QmlDesigner {

class ProjectStorageObserver
{
public:
    using ExportedTypeNames = Storage::Info::ExportedTypeNames;

    virtual void removedTypeIds(const TypeIds &removedTypeIds) = 0;
    virtual void exportedTypesChanged() = 0;
    virtual void exportedTypeNamesChanged(const ExportedTypeNames &added, const ExportedTypeNames &removed) = 0;
};
} // namespace QmlDesigner
