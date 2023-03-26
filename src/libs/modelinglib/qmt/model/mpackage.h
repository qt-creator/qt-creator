// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mobject.h"
#include "qmt/infrastructure/handles.h"

namespace qmt {

class QMT_EXPORT MPackage : public MObject
{
public:
    MPackage();
    ~MPackage() override;

    void accept(MVisitor *visitor) override;
    void accept(MConstVisitor *visitor) const override;
};

} // namespace qmt
