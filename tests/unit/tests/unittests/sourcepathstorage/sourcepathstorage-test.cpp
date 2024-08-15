// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <sourcepathstorage/sourcepathstorage.h>

namespace {

using QmlDesigner::Cache::SourceContext;
using QmlDesigner::Cache::SourceName;
using QmlDesigner::SourceContextId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::SourceNameId;

MATCHER_P2(IsSourceContext,
           id,
           value,
           std::string(negation ? "isn't " : "is ") + PrintToString(SourceContext{value, id}))
{
    const SourceContext &sourceContext = arg;

    return sourceContext.id == id && sourceContext.value == value;
}

class SourcePathStorage : public testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::SourcePathStorage storage{database, database.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    SourcePathStorage() = default;

    ~SourcePathStorage() { storage.resetForTestsOnly(); }

    void addSomeDummyData()
    {
        storage.fetchSourceContextId("/path/dummy");
        storage.fetchSourceContextId("/path/dummy2");
        storage.fetchSourceContextId("/path/");

        storage.fetchSourceNameId("foo");
        storage.fetchSourceNameId("dummy");
        storage.fetchSourceNameId("bar");
        storage.fetchSourceNameId("bar");
    }

    template<typename Range>
    static auto toValues(Range &&range)
    {
        using Type = typename std::decay_t<Range>;

        return std::vector<typename Type::value_type>{range.begin(), range.end()};
    }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    QmlDesigner::SourcePathStorage &storage = staticData->storage;
    Sqlite::Database &database = staticData->database;
};

TEST_F(SourcePathStorage, fetch_source_context_id_returns_always_the_same_id_for_the_same_path)

{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to");

    ASSERT_THAT(newSourceContextId, Eq(sourceContextId));
}

TEST_F(SourcePathStorage, fetch_source_context_id_returns_not_the_same_id_for_different_path)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto newSourceContextId = storage.fetchSourceContextId("/path/to2");

    ASSERT_THAT(newSourceContextId, Ne(sourceContextId));
}

TEST_F(SourcePathStorage, fetch_source_context_path)
{
    auto sourceContextId = storage.fetchSourceContextId("/path/to");

    auto path = storage.fetchSourceContextPath(sourceContextId);

    ASSERT_THAT(path, Eq("/path/to"));
}

TEST_F(SourcePathStorage, fetch_unknown_source_context_path_throws)
{
    ASSERT_THROW(storage.fetchSourceContextPath(SourceContextId::create(323)),
                 QmlDesigner::SourceContextIdDoesNotExists);
}

TEST_F(SourcePathStorage, fetch_all_source_contexts_are_empty_if_no_source_contexts_exists)
{
    storage.clearSources();

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts), IsEmpty());
}

TEST_F(SourcePathStorage, fetch_all_source_contexts)
{
    storage.clearSources();
    auto sourceContextId = storage.fetchSourceContextId("/path/to");
    auto sourceContextId2 = storage.fetchSourceContextId("/path/to2");

    auto sourceContexts = storage.fetchAllSourceContexts();

    ASSERT_THAT(toValues(sourceContexts),
                UnorderedElementsAre(IsSourceContext(sourceContextId, "/path/to"),
                                     IsSourceContext(sourceContextId2, "/path/to2")));
}

TEST_F(SourcePathStorage, fetch_source_id_first_time)
{
    addSomeDummyData();

    auto sourceNameId = storage.fetchSourceNameId("foo");

    ASSERT_TRUE(sourceNameId.isValid());
}

TEST_F(SourcePathStorage, fetch_existing_source_id)
{
    addSomeDummyData();
    auto createdSourceNameId = storage.fetchSourceNameId("foo");

    auto sourceNameId = storage.fetchSourceNameId("foo");

    ASSERT_THAT(sourceNameId, createdSourceNameId);
}

TEST_F(SourcePathStorage, fetch_source_id_with_different_name_are_not_equal)
{
    addSomeDummyData();
    auto sourceNameId2 = storage.fetchSourceNameId("foo");

    auto sourceNameId = storage.fetchSourceNameId("foo2");

    ASSERT_THAT(sourceNameId, Ne(sourceNameId2));
}

TEST_F(SourcePathStorage, fetch_source_name_and_source_context_id_for_non_existing_source_id)
{
    ASSERT_THROW(storage.fetchSourceName(SourceNameId::create(212)),
                 QmlDesigner::SourceNameIdDoesNotExists);
}

TEST_F(SourcePathStorage, fetch_source_name_for_non_existing_entry)
{
    addSomeDummyData();
    auto sourceNameId = storage.fetchSourceNameId("foo");

    auto sourceName = storage.fetchSourceName(sourceNameId);

    ASSERT_THAT(sourceName, Eq("foo"));
}

TEST_F(SourcePathStorage, fetch_all_sources)
{
    storage.clearSources();

    auto sources = storage.fetchAllSourceNames();

    ASSERT_THAT(toValues(sources), IsEmpty());
}

TEST_F(SourcePathStorage, fetch_source_id_unguarded_first_time)
{
    addSomeDummyData();
    std::lock_guard lock{database};

    auto sourceId = storage.fetchSourceNameIdUnguarded("foo");

    ASSERT_TRUE(sourceId.isValid());
}

TEST_F(SourcePathStorage, fetch_existing_source_id_unguarded)
{
    addSomeDummyData();
    std::lock_guard lock{database};
    auto createdSourceId = storage.fetchSourceNameIdUnguarded("foo");

    auto sourceId = storage.fetchSourceNameIdUnguarded("foo");

    ASSERT_THAT(sourceId, createdSourceId);
}

TEST_F(SourcePathStorage, fetch_source_id_unguarded_with_different_name_are_not_equal)
{
    addSomeDummyData();
    std::lock_guard lock{database};
    auto sourceId2 = storage.fetchSourceNameIdUnguarded("foo");

    auto sourceId = storage.fetchSourceNameIdUnguarded("foo2");

    ASSERT_THAT(sourceId, Ne(sourceId2));
}
} // namespace
