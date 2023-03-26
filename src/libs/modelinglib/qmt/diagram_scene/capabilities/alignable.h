// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace qmt {

class IAlignable
{
public:
    enum AlignType {
        AlignLeft,
        AlignRight,
        AlignTop,
        AlignBottom,
        AlignHcenter,
        AlignVcenter,
        AlignWidth,
        AlignHeight,
        AlignSize,
        AlignHCenterDistance,
        AlignVCenterDistance,
        AlignHBorderDistance,
        AlignVBorderDistance
    };

    virtual ~IAlignable() {}

    virtual void align(AlignType alignType, const QString &identifier) = 0;
};

} // namespace qmt
