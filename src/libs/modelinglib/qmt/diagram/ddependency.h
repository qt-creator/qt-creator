// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "drelation.h"

#include "qmt/model/mdependency.h"

namespace qmt {

class DObject;

class QMT_EXPORT DDependency : public DRelation
{
public:
    DDependency();
    ~DDependency() override;

    Uid source() const;
    void setSource(const Uid &source);
    Uid target() const;
    void setTarget(const Uid &target);
    MDependency::Direction direction() const { return m_direction; }
    void setDirection(MDependency::Direction direction);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;

private:
    MDependency::Direction m_direction = MDependency::AToB;
};

} // namespace qmt
