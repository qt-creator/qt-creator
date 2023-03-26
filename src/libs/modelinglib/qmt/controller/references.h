// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

namespace qmt {

template<class T>
class References
{
protected:
    References() = default;

public:
    ~References() = default;

    bool isEmpty() const { return m_elements.empty(); }
    int size() const { return m_elements.size(); }
    QList<T *> elements() const { return m_elements; }
    void setElements(const QList<T *> &elements) { m_elements = elements; }
    void clear() { m_elements.clear(); }
    void append(T *element) { m_elements.append(element); }

private:
    QList<T *> m_elements;
};

} // namespace qmt
