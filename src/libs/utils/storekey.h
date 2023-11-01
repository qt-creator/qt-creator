// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QByteArrayView>
#include <QString>
#include <QHashFunctions>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Key
{
public:
    Key() = default;
    Key(const Key &) = default;
    Key(Key &&) = default;

    Key(const QByteArray &key) : data(key) {}

    template <int N>
    Key(const char (&key)[N]) : data(key) {}

    // FIXME:
    // The following is wanted, but not used yet due to unclear ASAN report.
    // template <int N>
    // Key(const char (&key)[N]) : Key(key, strlen(key)) {}

    Key(const char *key, size_t n);

    Key(const Key &base, int number);
    ~Key();

    Key &operator=(const Key &) = default;
    Key &operator=(Key &&) = default;

    const QByteArrayView view() const;
    const QByteArray &toByteArray() const;
    QByteArrayView operator()() const { return data; }

    bool isEmpty() const { return data.isEmpty(); }
    void clear() { data.clear(); }

    friend bool operator<(const Key &a, const Key &b) { return a.data < b.data; }
    friend bool operator==(const Key &a, const Key &b) { return a.data == b.data; }

    friend Key operator+(const Key &a, const Key &b)
    {
        return Key(a.data + b.data);
    }
    friend Key operator+(const Key &a, char b)
    {
        return Key(a.data + b);
    }
    friend size_t qHash(const Key &key, size_t seed = 0)
    {
        return qHash(key.data, seed);
    }

private:
    QByteArray data;
};

QTCREATOR_UTILS_EXPORT Key keyFromString(const QString &str);
QTCREATOR_UTILS_EXPORT QString stringFromKey(const Key &key);

} // Utils
