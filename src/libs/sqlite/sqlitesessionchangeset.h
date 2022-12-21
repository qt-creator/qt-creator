// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqlite3_fwd.h"
#include "sqliteblob.h"
#include "sqliteglobal.h"
#include "sqlitevalue.h"

#include <utils/smallstring.h>

#include <memory>
#include <vector>
#include <iosfwd>

namespace Sqlite {

class Sessions;

enum class Operation : char { Invalid, Insert, Update, Delete };

namespace SessionChangeSetInternal {

class SentinelIterator
{};

class ValueViews
{
public:
    ValueView newValue;
    ValueView oldValue;
};

class SQLITE_EXPORT ConstTupleIterator
{
public:
    using difference_type = int;
    using value_type = ValueViews;
    using pointer = const ValueViews *;
    using reference = const ValueViews &;
    using iterator_category = std::forward_iterator_tag;

    ConstTupleIterator(sqlite3_changeset_iter *sessionIterator,
                       int index,
                       Sqlite::Operation operation)
        : m_sessionIterator{sessionIterator}
        , m_column{index}
        , m_operation{operation}
    {}

    ConstTupleIterator operator++()
    {
        ++m_column;

        return *this;
    }

    friend bool operator==(const ConstTupleIterator &first, const ConstTupleIterator &second)
    {
        return first.m_column == second.m_column;
    }

    friend bool operator!=(const ConstTupleIterator &first, const ConstTupleIterator &second)
    {
        return !(first == second);
    }

    ValueViews operator*() const;

private:
    sqlite3_changeset_iter *m_sessionIterator = {};
    int m_column = 0;
    Sqlite::Operation m_operation = Sqlite::Operation::Invalid;
};

class SQLITE_EXPORT Tuple
{
public:
    using difference_type = int;
    using value_type = ValueView;
    using reference = ValueView &;
    using const_reference = const ValueView &;
    using iterator = ConstTupleIterator;
    using const_iterator = ConstTupleIterator;
    using size_type = int;

    Utils::SmallStringView table;
    sqlite3_changeset_iter *sessionIterator = {};
    int columnCount = 0;
    Sqlite::Operation operation = Sqlite::Operation::Invalid;

    ValueViews operator[](int column) const;
    ConstTupleIterator begin() const { return {sessionIterator, 0, operation}; }
    ConstTupleIterator end() const { return {sessionIterator, columnCount, operation}; }
};

enum class State : char { Invalid, Row, Done };

class SQLITE_EXPORT ConstIterator
{
public:
    using difference_type = long;
    using value_type = Tuple;
    using pointer = const Tuple *;
    using reference = const Tuple &;
    using iterator_category = std::input_iterator_tag;

    ConstIterator(sqlite3_changeset_iter *sessionIterator)
        : m_sessionIterator(sessionIterator)
    {}

    ConstIterator(const ConstIterator &) = delete;
    void operator=(const ConstIterator &) = delete;

    ConstIterator(ConstIterator &&other)
        : m_sessionIterator(other.m_sessionIterator)
        , m_state(other.m_state)
    {
        other.m_sessionIterator = {};
        other.m_state = State::Done;
    }

    ConstIterator &operator=(ConstIterator &&other)
    {
        auto tmp = std::move(other);
        swap(tmp, *this);

        return *this;
    }

    ~ConstIterator();

    friend void swap(ConstIterator &first, ConstIterator &second) noexcept
    {
        std::swap(first.m_sessionIterator, second.m_sessionIterator);
        std::swap(first.m_state, second.m_state);
    }

    ConstIterator &operator++();

    friend bool operator==(const ConstIterator &first, const ConstIterator &second)
    {
        return first.m_sessionIterator == second.m_sessionIterator;
    }

    friend bool operator!=(const ConstIterator &first, const ConstIterator &second)
    {
        return !(first == second);
    }

    friend bool operator==(const ConstIterator &first, SentinelIterator)
    {
        return first.m_state == State::Done;
    }

    friend bool operator!=(const ConstIterator &first, SentinelIterator)
    {
        return first.m_state == State::Row;
    }

    friend bool operator==(SentinelIterator first, const ConstIterator &second)
    {
        return second == first;
    }

    friend bool operator!=(SentinelIterator first, const ConstIterator &second)
    {
        return second != first;
    }

    Tuple operator*() const;

    State state() const { return m_state; }

private:
    sqlite3_changeset_iter *m_sessionIterator = {};
    State m_state = State::Invalid;
};
} // namespace SessionChangeSetInternal

class SQLITE_EXPORT SessionChangeSet
{
public:
    SessionChangeSet(BlobView blob);
    SessionChangeSet(Sessions &session);
    ~SessionChangeSet();
    SessionChangeSet(const SessionChangeSet &) = delete;
    void operator=(const SessionChangeSet &) = delete;
    SessionChangeSet(SessionChangeSet &&other) noexcept
    {
        SessionChangeSet temp;
        swap(temp, other);
        swap(temp, *this);
    }
    void operator=(SessionChangeSet &);

    BlobView asBlobView() const;

    friend void swap(SessionChangeSet &first, SessionChangeSet &second) noexcept
    {
        SessionChangeSet temp;
        std::swap(temp.m_data, first.m_data);
        std::swap(temp.m_size, first.m_size);
        std::swap(first.m_data, second.m_data);
        std::swap(first.m_size, second.m_size);
        std::swap(temp.m_data, second.m_data);
        std::swap(temp.m_size, second.m_size);
    }

    SessionChangeSetInternal::ConstIterator begin() const;
    SessionChangeSetInternal::SentinelIterator end() const { return {}; }

    void *data() const { return m_data; }

    int size() const { return m_size; }

private:
    SessionChangeSet() = default;

private:
    void *m_data = nullptr;
    int m_size = {};
};

using SessionChangeSets = std::vector<SessionChangeSet>;

} // namespace Sqlite
