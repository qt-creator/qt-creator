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

#ifndef QMT_SELECTION_H
#define QMT_SELECTION_H

#include "qmt/infrastructure/uid.h"

#include <QList>


namespace qmt {

class QMT_EXPORT Selection
{
public:
    class Index {
    public:

        Index(const Uid &elementKey, const Uid &ownerKey)
            : m_elementKey(elementKey),
              m_ownerKey(ownerKey)
        {
        }

        Uid elementKey() const { return m_elementKey; }

        Uid ownerKey() const { return m_ownerKey; }

    private:

        Uid m_elementKey;

        Uid m_ownerKey;
    };

protected:

    Selection();

public:

    ~Selection();

public:

    bool isEmpty() const { return m_indices.isEmpty(); }

    QList<Index> indices() const { return m_indices; }

    void setIndices(const QList<Index> &indices);

public:

    void clear();

    void append(const Index &index);

    void append(const Uid &elementKey, const Uid &ownerKey);

private:

    QList<Index> m_indices;
};

}


#endif // QMT_SELECTION_H
