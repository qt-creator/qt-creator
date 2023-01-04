// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/SafeMatcher.h>

#include <QSet>

namespace CPlusPlus {

template<typename T>
class AlreadyConsideredClassContainer
{
public:
    AlreadyConsideredClassContainer() : _class(nullptr) {}
    void insert(const T *item)
    {
        if (_container.isEmpty())
            _class = item;
        _container.insert(item);
    }
    bool contains(const T *item)
    {
        if (_container.contains(item))
            return true;

        SafeMatcher matcher;
        for (const T *existingItem : std::as_const(_container)) {
            if (Matcher::match(existingItem, item, &matcher))
                return true;
        }

        return false;
    }

    void clear(const T *item)
    {
        if (_class != item || _container.size() == 1)
            _container.clear();
    }

private:
    QSet<const T *> _container;
    const T * _class;
};

} // namespace CPlusPlus
