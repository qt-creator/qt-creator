// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modulesstorage.h"

#include "projectstoragetracing.h"

#include <projectstorageexceptions.h>

#include <sqlitedatabase.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using ProjectStorageTracing::category;

struct ModulesStorage::Statements
{
    Statements(Sqlite::Database &database)
        : database{database}
    {
        NanotraceHR::Tracer tracer{"module storage statements constructor", category()};
    }

    Sqlite::Database &database;
    mutable Sqlite::ReadStatement<1, 2> selectModuleNameStatement{
        "SELECT moduleId FROM modules WHERE name=?1 AND kind=?2", database};
    mutable Sqlite::ReadWriteStatement<1, 2> upsertModuleNameStatement{
        "INSERT INTO modules(name, kind) VALUES(?1, ?2) "
        "ON CONFLICT DO UPDATE SET name=?1, kind=?2 "
        "RETURNING moduleId",
        database};
    mutable Sqlite::ReadStatement<2, 1> selectModuleStatement{
        "SELECT name, kind FROM modules WHERE moduleId =?1", database};
    mutable Sqlite::ReadStatement<3> selectAllModulesStatement{
        "SELECT name, kind, moduleId FROM modules", database};
};

class ModulesStorage::Initializer
{
public:
    Initializer(Database &database, bool isInitialized)
    {
        NanotraceHR::Tracer tracer{"module storage initializer constructor", category()};

        if (!isInitialized)
            createModulesTable(database);

        database.setIsInitialized(true);
    }

    void createModulesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("modules");

        table.addColumn("moduleId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        auto &nameColumn = table.addColumn("name", Sqlite::StrictColumnType::Text);
        auto &kindColumn = table.addColumn("kind", Sqlite::StrictColumnType::Integer);

        table.addUniqueIndex({nameColumn, kindColumn});
        table.addIndex({kindColumn});

        table.initialize(database);
    }
};

ModulesStorage::ModulesStorage(Database &database, bool isInitialized)
    : database{database}
    , exclusiveTransaction{database}
    , initializer{std::make_unique<ModulesStorage::Initializer>(database, isInitialized)}
    , moduleCache{ModulesStorageAdapter{*this}}
    , s{std::make_unique<ModulesStorage::Statements>(database)}
{
    NanotraceHR::Tracer tracer{"module storage constructor", category()};

    exclusiveTransaction.commit();

    database.walCheckpointFull();

    moduleCache.populate();
}

ModulesStorage::~ModulesStorage() = default;

ModuleId ModulesStorage::moduleId(Utils::SmallStringView moduleName, Storage::ModuleKind kind) const
{
    NanotraceHR::Tracer tracer{"get module id",
                               category(),
                               keyValue("module name", moduleName),
                               keyValue("module kind", kind)};

    if (moduleName.empty())
        return ModuleId{};

    auto moduleId = moduleCache.id({moduleName, kind});

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

SmallModuleIds<128> ModulesStorage::moduleIdsStartsWith(Utils::SmallStringView startsWith,
                                                        Storage::ModuleKind kind) const
{
    NanotraceHR::Tracer tracer{"get module ids that starts with",
                               category(),
                               keyValue("module name starts with", startsWith),
                               keyValue("module kind", kind)};

    if (startsWith.isEmpty())
        return {};

    auto projection = [&](ModuleView view) -> ModuleView {
        return {view.name.substr(0, startsWith.size()), view.kind};
    };

    auto moduleIds = moduleCache.ids<128>({startsWith, kind}, projection);

    return moduleIds;
}

Storage::Module ModulesStorage::module(ModuleId moduleId) const
{
    NanotraceHR::Tracer tracer{"get module name", category(), keyValue("module id", moduleId)};

    if (!moduleId)
        throw ModuleDoesNotExists{};

    auto module = moduleCache.value(moduleId);

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module kind", module.kind));

    return {module.name, module.kind};
}

void ModulesStorage::resetForTestsOnly()
{
    moduleCache.clearForTestOnly();
}

ModuleId ModulesStorage::fetchModuleIdUnguarded(Utils::SmallStringView name,
                                                Storage::ModuleKind kind) const
{
    NanotraceHR::Tracer tracer{"fetch module id ungarded",
                               category(),
                               keyValue("module name", name),
                               keyValue("module kind", kind)};

    auto moduleId = s->selectModuleNameStatement.value<ModuleId>(name, kind);

    if (not moduleId)
        moduleId = s->upsertModuleNameStatement.value<ModuleId>(name, kind);

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

Storage::Module ModulesStorage::fetchModuleUnguarded(ModuleId id) const
{
    NanotraceHR::Tracer tracer{"fetch module ungarded", category(), keyValue("module id", id)};

    auto module = s->selectModuleStatement.value<Storage::Module>(id);

    if (!module)
        throw ModuleDoesNotExists{};

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module name", module.kind));

    return module;
}

ModuleId ModulesStorage::fetchModuleId(Utils::SmallStringView moduleName,
                                       Storage::ModuleKind moduleKind)
{
    NanotraceHR::Tracer tracer{"fetch module id",
                               category(),
                               keyValue("module name", moduleName),
                               keyValue("module kind", moduleKind)};

    auto moduleId = Sqlite::withImplicitTransaction(database, [&] {
        return fetchModuleIdUnguarded(moduleName, moduleKind);
    });

    tracer.end(keyValue("module id", moduleId));

    return moduleId;
}

Storage::Module ModulesStorage::fetchModule(ModuleId id)
{
    NanotraceHR::Tracer tracer{"fetch module name", category(), keyValue("module id", id)};

    auto module = Sqlite::withDeferredTransaction(database, [&] { return fetchModuleUnguarded(id); });

    tracer.end(keyValue("module name", module.name));
    tracer.end(keyValue("module kind", module.kind));

    return module;
}

ModulesStorage::ModuleCacheEntries ModulesStorage::fetchAllModules() const
{
    NanotraceHR::Tracer tracer{"fetch all modules", category()};

    return s->selectAllModulesStatement.valuesWithTransaction<ModuleCacheEntry, 128>();
}

auto ModulesStorage::ModulesStorageAdapter::fetchId(ModuleView module)
{
    NanotraceHR::Tracer tracer{"module stoeage adapter fetch id",
                               category(),
                               NanotraceHR::keyValue("module name", module.name),
                               NanotraceHR::keyValue("module kind", module.kind)};
    return storage.fetchModuleId(module.name, module.kind);
}

auto ModulesStorage::ModulesStorageAdapter::fetchValue(ModuleId id)
{
    NanotraceHR::Tracer tracer{"module stoeage adapter fetch value",
                               category(),
                               NanotraceHR::keyValue("module id", id)};

    return storage.fetchModule(id);
}

auto ModulesStorage::ModulesStorageAdapter::fetchAll()
{
    NanotraceHR::Tracer tracer{"module stoeage adapter fetch all", category()};

    return storage.fetchAllModules();
}

} // namespace QmlDesigner
