/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
