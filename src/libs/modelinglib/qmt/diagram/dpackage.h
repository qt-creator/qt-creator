// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dobject.h"

namespace qmt {

class QMT_EXPORT DPackage : public DObject
{
public:
    DPackage();
    ~DPackage() override;

    void accept(DVisitor *visitor) override;
    void accept(DConstVisitor *visitor) const override;
};

} // namespace qmt
