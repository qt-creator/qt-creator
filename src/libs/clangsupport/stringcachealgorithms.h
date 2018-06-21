/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <utils/smallstringio.h>

#include <iterator>

#include <QDebug>

namespace ClangBackEnd {

template <typename Iterator>
class Found
{
public:
    Iterator iterator;
    bool wasFound;
};

template<typename ForwardIterator,
         typename Type,
         typename Compare>
Found<ForwardIterator> findInSorted(ForwardIterator first, ForwardIterator last, const Type& value, Compare compare)
{
    ForwardIterator current;
    using DifferenceType = typename std::iterator_traits<ForwardIterator>::difference_type;
    DifferenceType count{std::distance(first, last)};
    DifferenceType step;

    while (count > 0) {
        current = first;
        step = count / 2;
        std::advance(current, step);
        auto comparison = compare(*current, value);
        if (comparison < 0) {
            first = ++current;
            count -= step + 1;
        } else if (comparison > 0) {
            count = step;
        } else {
            return {current, true};
        }
    }

    return {first, false};
}

} // namespace ClangBackEnd
