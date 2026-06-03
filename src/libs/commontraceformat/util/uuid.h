// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QUuid>

#include <array>
#include <span>

namespace CommonTraceFormat {

// Convert QUuid to 16 raw bytes in RFC 4122 big-endian order.
inline std::array<uint8_t, 16> uuidToBytes(const QUuid &uuid)
{
    const auto rfc = uuid.toRfc4122();
    std::array<uint8_t, 16> out{};
    for (int i = 0; i < 16; ++i)
        out[i] = static_cast<uint8_t>(rfc[i]);
    return out;
}

// Parse 16 raw bytes (RFC 4122 big-endian) into a QUuid.
inline QUuid uuidFromBytes(std::span<const uint8_t, 16> bytes)
{
    QByteArray ba(reinterpret_cast<const char *>(bytes.data()), 16);
    return QUuid::fromRfc4122(ba);
}

} // namespace CommonTraceFormat
