/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QList>

namespace qmt {

template<class T>
class Container
{
protected:
    Container() = default;
    Container(const Container<T> &rhs)
        : m_elements(rhs.m_elements)
    {
        rhs.m_elements.clear();
    }

public:
    ~Container()
    {
        qDeleteAll(m_elements);
    }

    Container &operator=(const Container<T> &rhs)
    {
        if (this != &rhs) {
            qDeleteAll(m_elements);
            m_elements = rhs.m_elements;
            rhs.m_elements.clear();
        }
        return *this;
    }

    bool isEmpty() const { return m_elements.empty(); }
    int size() const { return m_elements.size(); }
    QList<T *> elements() const { return m_elements; }

    QList<T *> takeElements() const
    {
        QList<T *> elements = m_elements;
        m_elements.clear();
        return elements;
    }

    void submitElements(const QList<T *> &elements)
    {
        qDeleteAll(m_elements);
        m_elements = elements;
    }

    void reset()
    {
        qDeleteAll(m_elements);
        m_elements.clear();
    }

    void submit(T *element)
    {
        m_elements.append(element);
    }

private:
    mutable QList<T *> m_elements;
};

} // namespace qmt
