// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sqliteglobal.h"

#include <utils/span.h>

#include <QByteArray>

#include <algorithm>
#include <cstring>
#include <vector>

namespace Sqlite {

class BlobView
{
public:
    BlobView() = default;

    BlobView(const std::byte *data, std::size_t size)
        : m_data(data)
        , m_size(size)
    {}

    BlobView(const QByteArray &byteArray)
        : m_data(reinterpret_cast<const std::byte *>(byteArray.constData()))
        , m_size(static_cast<std::size_t>(byteArray.size()))
    {}

    BlobView(const std::vector<std::byte> &bytes)
        : m_data(bytes.data())
        , m_size(static_cast<std::size_t>(bytes.size()))
    {}

    BlobView(Utils::span<const std::byte> bytes)
        : m_data(bytes.data())
        , m_size(static_cast<std::size_t>(bytes.size()))
    {}

    const std::byte *data() const { return m_data; }
    const char *cdata() const { return reinterpret_cast<const char *>(m_data); }
    std::size_t size() const { return m_size; }
    int sisize() const { return static_cast<int>(m_size); }
    long long ssize() const { return static_cast<long long>(m_size); }
    bool empty() const { return !m_size; }

    friend bool operator==(Sqlite::BlobView first, Sqlite::BlobView second)
    {
        return first.size() == second.size()
               && std::memcmp(first.data(), second.data(), first.size()) == 0;
    }

private:
    const std::byte *m_data{};
    std::size_t m_size{};
};

class Blob
{
public:
    Blob(BlobView blobView)
    {
        bytes.reserve(blobView.size());
        std::copy_n(blobView.data(), blobView.size(), std::back_inserter(bytes));
    }

    std::vector<std::byte> bytes;

    friend bool operator==(const Sqlite::Blob &first, const Sqlite::Blob &second)
    {
        return BlobView{first.bytes} == BlobView{second.bytes};
    }

    friend bool operator==(const Sqlite::Blob &first, Sqlite::BlobView second)
    {
        return BlobView{first.bytes} == second;
    }

    friend bool operator==(Sqlite::BlobView first, const Sqlite::Blob &second)
    {
        return second == first;
    }
};

class ByteArrayBlob
{
public:
    ByteArrayBlob(BlobView blobView)
        : byteArray{blobView.cdata(), blobView.sisize()}
    {}

    QByteArray byteArray;
};

} // namespace Sqlite
