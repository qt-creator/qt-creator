// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
