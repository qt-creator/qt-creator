// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mrelation.h"
#include "qmt/infrastructure/handle.h"

namespace qmt {

class QMT_EXPORT MInheritance : public MRelation
{
public:
    MInheritance();
    MInheritance(const MInheritance &rhs);
    ~MInheritance() override;

    MInheritance operator=(const MInheritance &rhs);

    Uid derived() const;
    void setDerived(const Uid &derived);
    Uid base() const;
    void setBase(const Uid &base);

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;
};

} // namespace qmt
