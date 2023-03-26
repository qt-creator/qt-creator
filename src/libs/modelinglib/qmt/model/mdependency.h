// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mrelation.h"

namespace qmt {

class MObject;

class QMT_EXPORT MDependency : public MRelation
{
public:
    enum Direction {
        AToB,
        BToA,
        Bidirectional
    };

    MDependency();
    MDependency(const MDependency &rhs);
    ~MDependency() override;

    MDependency &operator=(const MDependency &rhs);

    Direction direction() const { return m_direction; }
    void setDirection(Direction direction);
    Uid source() const;
    void setSource(const Uid &source);
    Uid target() const;
    void setTarget(const Uid &target);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;

private:
    Direction m_direction = AToB;
};

} // namespace qmt
