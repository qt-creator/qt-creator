// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "styledrelation.h"

namespace qmt {

StyledRelation::StyledRelation(const DRelation *relation, const DObject *endA, const DObject *endB)
    : m_relation(relation),
      m_endA(endA),
      m_endB(endB)
{
}

StyledRelation::~StyledRelation()
{
}

} // namespace qmt
