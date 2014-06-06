/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <algorithm>

#include <qcompilerdetection.h> // for Q_REQUIRE_RESULT

namespace Utils
{

template<typename T, typename F>
bool anyOf(const T &container, F predicate)
{
    return std::any_of(container.begin(), container.end(), predicate);
}

template<typename T, typename F>
bool allOf(const T &container, F predicate)
{
    return std::all_of(container.begin(), container.end(), predicate);
}

template<typename T, typename F>
void erase(QList<T> &container, F predicate)
{
    container.erase(std::remove_if(container.begin(), container.end(), predicate),
                    container.end());
}

template<typename T, typename F>
bool contains(const T &container, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::find_if(begin, end, function);
    return it != end;
}

template<typename T, typename F>
typename T::value_type findOr(const T &container, typename T::value_type other, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::find_if(begin, end, function);
    if (it == end)
        return other;
    return *it;
}

template<typename T, typename F>
typename T::value_type maxElementOr(const T &container, typename T::value_type other, F function)
{
    typename T::const_iterator end = container.end();
    typename T::const_iterator begin = container.begin();

    typename T::const_iterator it = std::max_element(begin, end, function);
    if (it == end)
        return other;
    return *it;
}

// Note: add overloads for other container types as needed
template<typename T, typename F>
Q_REQUIRED_RESULT
auto transform(const QList<T> &container, F function) -> QList<decltype(function(T()))>
{
    QList<decltype(function(T()))> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result),
                   function);
    return result;
}

}

#endif // ALGORITHM_H
