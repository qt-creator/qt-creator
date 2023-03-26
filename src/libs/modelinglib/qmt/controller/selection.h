// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
