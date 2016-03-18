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

#include "qmt/infrastructure/uid.h"

#include <QList>

namespace qmt {

class QMT_EXPORT Selection
{
public:
    class Index
    {
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

    bool isEmpty() const { return m_indices.isEmpty(); }
    QList<Index> indices() const { return m_indices; }
    void setIndices(const QList<Index> &indices);

    void clear();
    void append(const Index &index);
    void append(const Uid &elementKey, const Uid &ownerKey);

private:
    QList<Index> m_indices;
};

} // namespace qmt
