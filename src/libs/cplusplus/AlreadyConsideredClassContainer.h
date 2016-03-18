/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <cplusplus/SafeMatcher.h>

#include <QSet>

namespace CPlusPlus {

template<typename T>
class AlreadyConsideredClassContainer
{
public:
    AlreadyConsideredClassContainer() : _class(0) {}
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
        foreach (const T *existingItem, _container) {
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
