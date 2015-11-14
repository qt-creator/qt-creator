/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_CONTAINER_H
#define QMT_CONTAINER_H

#include <QList>

namespace qmt {

template<class T>
class Container
{
protected:
    Container() { }
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

#endif // QMT_CONTAINER_H
