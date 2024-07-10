// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DRelation;
class DObject;
class CustomRelation;

class QMT_EXPORT StyledRelation
{
public:
    StyledRelation(const DRelation *relation, const DObject *endA, const DObject *endB,
                   const CustomRelation *customRelation);
    ~StyledRelation();

    const DRelation *relation() const { return m_relation; }
    const DObject *endA() const { return m_endA; }
    const DObject *endB() const { return m_endB; }
    const CustomRelation *customRelation() const { return m_customRelation; }

private:
    const DRelation *m_relation = nullptr;
    const DObject *m_endA = nullptr;
    const DObject *m_endB = nullptr;
    const CustomRelation *m_customRelation = nullptr;
};

} // namespace qmt
