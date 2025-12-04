// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modulesstorageinterface.h"

#include <sourcepathstorage/storagecache.h>

#include <qmldesignercorelib_exports.h>

#include <sqlitetransaction.h>

#include <shared_mutex>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

class QMLDESIGNERCORE_EXPORT ModulesStorage final : public ModulesStorageInterface
{
    using Database = Sqlite::Database;

    enum class Relink { No, Yes };
    enum class RaiseError { No, Yes };

public:
    ModulesStorage(Database &database, bool isInitialized);
    ~ModulesStorage();

    ModuleId moduleId(Utils::SmallStringView moduleName, Storage::ModuleKind kind) const override;

    SmallModuleIds<128> moduleIdsStartsWith(Utils::SmallStringView startsWith,
                                            Storage::ModuleKind kind) const override;

    Storage::Module module(ModuleId moduleId) const override;

    void resetForTestsOnly();

private:
    struct ModuleView
    {
        ModuleView() = default;

        ModuleView(Utils::SmallStringView name, Storage::ModuleKind kind)
            : name{name}
            , kind{kind}
        {}

        ModuleView(const Storage::Module &module)
            : name{module.name}
            , kind{module.kind}
        {}

        Utils::SmallStringView name;
        Storage::ModuleKind kind;

        friend std::strong_ordering operator<=>(ModuleView first, ModuleView second)
        {
            return std::tie(first.kind, first.name) <=> std::tie(second.kind, second.name);
        }

        friend bool operator==(const Storage::Module &first, ModuleView second)
        {
            return first.name == second.name && first.kind == second.kind;
        }

        friend bool operator==(ModuleView first, const Storage::Module &second)
        {
            return second == first;
        }
    };

    class ModulesStorageAdapter
    {
    public:
        auto fetchId(ModuleView module);

        auto fetchValue(ModuleId id);

        auto fetchAll();

        ModulesStorage &storage;
    };

    friend ModulesStorageAdapter;

    struct ModuleNameLess
    {
        bool operator()(ModuleView first, ModuleView second) const noexcept
        {
            return first < second;
        }
    };

    class ModuleCacheEntry : public StorageCacheEntry<Storage::Module, ModuleView, ModuleId>
    {
        using Base = StorageCacheEntry<Storage::Module, ModuleView, ModuleId>;

    public:
        using Base::Base;

        ModuleCacheEntry(Utils::SmallStringView name, Storage::ModuleKind kind, ModuleId moduleId)
            : Base{{name, kind}, moduleId}
        {}

        friend bool operator==(const ModuleCacheEntry &first, const ModuleCacheEntry &second)
        {
            return &first == &second && first.value == second.value;
        }

        friend bool operator==(const ModuleCacheEntry &first, ModuleView second)
        {
            return first.value.name == second.name && first.value.kind == second.kind;
        }
    };

    using ModuleCacheEntries = std::vector<ModuleCacheEntry>;

    using ModuleCache = StorageCache<Storage::Module,
                                     ModuleView,
                                     ModuleId,
                                     ModulesStorageAdapter,
                                     std::shared_mutex,
                                     ModuleNameLess,
                                     ModuleCacheEntry>;

    ModuleId fetchModuleId(Utils::SmallStringView moduleName, Storage::ModuleKind moduleKind);

    Storage::Module fetchModule(ModuleId id);

    ModuleCacheEntries fetchAllModules() const;

    ModuleId fetchModuleIdUnguarded(Utils::SmallStringView name, Storage::ModuleKind moduleKind) const;

    Storage::Module fetchModuleUnguarded(ModuleId id) const;

    class Initializer;

    struct Statements;

public:
    Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Database> exclusiveTransaction;
    std::unique_ptr<Initializer> initializer;
    mutable ModuleCache moduleCache{ModulesStorageAdapter{*this}};
    std::unique_ptr<Statements> s;
};


} // namespace QmlDesigner
