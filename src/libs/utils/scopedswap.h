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

#if !(__cplusplus > 199711L || __GXX_EXPERIMENTAL_CXX0X__ || _MSC_VER >= 1600 || defined( _LIBCPP_VERSION )) || \
    (defined(__GNUC_LIBSTD__) && ((__GNUC_LIBSTD__-0) * 100 + __GNUC_LIBSTD_MINOR__-0 <= 402))
#  include <algorithm>
#else // C++11
#  include <utility>
#endif


namespace Utils {
/// RAII object to save a value, and restore it when the scope is left.
template<typename T>
class ScopedSwap
{
    T oldValue;
    T &ref;

public:
    ScopedSwap(T &var, T newValue)
        : oldValue(newValue)
        , ref(var)
    {
        std::swap(ref, oldValue);
    }

    ~ScopedSwap()
    {
        std::swap(ref, oldValue);
    }
};

typedef ScopedSwap<bool> ScopedBoolSwap;

} // Utils namespace
