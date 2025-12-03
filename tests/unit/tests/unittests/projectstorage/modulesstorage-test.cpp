// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <modulesstorage/modulesstorage.h>
#include <projectstorage/projectstorageexceptions.h>

#include <sqlitedatabase.h>

namespace {

using QmlDesigner::ModuleId;
using QmlDesigner::Storage::ModuleKind;

namespace Storage = QmlDesigner::Storage;

auto IsModule(Utils::SmallStringView name, ModuleKind kind)
{
    return AllOf(Field("QmlDesigner::Storage::Module::name", &QmlDesigner::Storage::Module::name, name),
                 Field("QmlDesigner::Storage::Module::kind", &QmlDesigner::Storage::Module::kind, kind));
}

class ModuleStorage : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage storage{database, database.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    ModuleStorage() {}

    ~ModuleStorage() { storage.resetForTestsOnly(); }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    QmlDesigner::ModulesStorage &storage = staticData->storage;
};

TEST_F(ModuleStorage, get_module_id)
{
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    ASSERT_TRUE(id);
}

TEST_F(ModuleStorage, get_invalid_module_id_for_empty_name)
{
    auto id = storage.moduleId("", ModuleKind::QmlLibrary);

    ASSERT_FALSE(id);
}

TEST_F(ModuleStorage, get_same_module_id_again)
{
    auto initialId = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    ASSERT_THAT(id, Eq(initialId));
}

TEST_F(ModuleStorage, different_module_kind_returns_different_id)
{
    auto qmlId = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto cppId = storage.moduleId("Qml", ModuleKind::CppLibrary);

    ASSERT_THAT(cppId, Ne(qmlId));
}

TEST_F(ModuleStorage, module_throws_if_id_is_invalid)
{
    ASSERT_THROW(storage.module(ModuleId{}), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ModuleStorage, module_throws_if_id_does_not_exists)
{
    ASSERT_THROW(storage.module(ModuleId::create(222)), QmlDesigner::ModuleDoesNotExists);
}

TEST_F(ModuleStorage, get_module)
{
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    auto module = storage.module(id);

    ASSERT_THAT(module, IsModule("Qml", ModuleKind::QmlLibrary));
}

TEST_F(ModuleStorage, populate_module_cache)
{
    auto id = storage.moduleId("Qml", ModuleKind::QmlLibrary);

    QmlDesigner::ModulesStorage newStorage{database, database.isInitialized()};

    ASSERT_THAT(newStorage.module(id), IsModule("Qml", ModuleKind::QmlLibrary));
}

TEST_F(ModuleStorage, get_no_module_ids_if_they_starts_with_nothing)
{
    storage.moduleId("QtQml", ModuleKind::QmlLibrary);

    auto ids = storage.moduleIdsStartsWith("", ModuleKind::QmlLibrary);

    ASSERT_THAT(ids, IsEmpty());
}

TEST_F(ModuleStorage, get_module_ids_if_they_starts_with_New)
{
    auto quickId = storage.moduleId("NewQuick", ModuleKind::QmlLibrary);
    storage.moduleId("NewQuick", ModuleKind::CppLibrary);
    auto quick3dId = storage.moduleId("NewQuick3D", ModuleKind::QmlLibrary);
    storage.moduleId("NewQml", ModuleKind::CppLibrary);
    auto qmlId = storage.moduleId("NewQml", ModuleKind::QmlLibrary);
    storage.moduleId("Foo", ModuleKind::QmlLibrary);
    storage.moduleId("Zoo", ModuleKind::QmlLibrary);

    auto ids = storage.moduleIdsStartsWith("New", ModuleKind::QmlLibrary);

    ASSERT_THAT(ids, UnorderedElementsAre(qmlId, quickId, quick3dId));
}

} // namespace
