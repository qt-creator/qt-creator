// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectstorage/projectstoragetypes.h>
#include <projectstorageids.h>

#include <utils/smallstringview.h>

#include <QVarLengthArray>

namespace QmlDesigner {

class ModulesStorageInterface
{

public:
    ModulesStorageInterface(const ModulesStorageInterface &) = delete;
    ModulesStorageInterface &operator=(const ModulesStorageInterface &) = delete;
    ModulesStorageInterface(ModulesStorageInterface &&) = default;
    ModulesStorageInterface &operator=(ModulesStorageInterface &&) = default;

    virtual ModuleId moduleId(::Utils::SmallStringView name, Storage::ModuleKind kind) const = 0;
    virtual SmallModuleIds<128>
    moduleIdsStartsWith(Utils::SmallStringView startsWith, Storage::ModuleKind kind) const = 0;
    virtual QmlDesigner::Storage::Module module(ModuleId moduleId) const = 0;

protected:
    ModulesStorageInterface() = default;
    ~ModulesStorageInterface() = default;
};

} // namespace QmlDesigner
