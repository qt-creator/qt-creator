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

#ifndef CONTAINER_COMMON_H
#define CONTAINER_COMMON_H


namespace ClangBackEnd {

template <typename Container>
bool compareContainer(const Container &first, const Container &second)
{
    if (first.size() != second.size())
        return first.size() < second.size();

    Container firstCopy = first;
    Container secondCopy = second;

    std::sort(firstCopy.begin(), firstCopy.end());
    std::sort(secondCopy.begin(), secondCopy.end());

    auto isProjectPartContainerSmaller = [] (decltype(*first.cbegin()) &firstElement,
                                             decltype(*second.cbegin()) &secondElement) {
        return firstElement < secondElement;
    };

    return std::equal(firstCopy.cbegin(),
                      firstCopy.cend(),
                      secondCopy.cbegin(),
                      isProjectPartContainerSmaller);
}
}

#endif // CONTAINER_COMMON_H

