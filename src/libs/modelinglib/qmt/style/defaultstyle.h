// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "style.h"

namespace qmt {

class QMT_EXPORT DefaultStyle :  public Style
{
public:
    DefaultStyle();
    ~DefaultStyle() override;
};

} // namespace qmt
