// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selection.h"

namespace qmt {

Selection::Selection()
{
}

Selection::~Selection()
{
}

void Selection::setIndices(const QList<qmt::Selection::Index> &indices)
{
    m_indices = indices;
}

void Selection::clear()
{
    m_indices.clear();
}

void Selection::append(const Index &index)
{
    m_indices.append(index);
}

void Selection::append(const Uid &elementKey, const Uid &ownerKey)
{
    append(Index(elementKey, ownerKey));
}

} // namespace qmt
