// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "drelation.h"

namespace qmt {

class DClass;

class QMT_EXPORT DInheritance : public DRelation
{
public:
    DInheritance();
    ~DInheritance() override;

    Uid derived() const;
    void setDerived(const Uid &derived);
    Uid base() const;
    void setBase(const Uid &base);

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;
};

} // namespace qmt
