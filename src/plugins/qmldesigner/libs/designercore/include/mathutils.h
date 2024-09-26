// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlDesigner {

inline double round(double value, int digits)
{
    double factor = digits * 10.;
    return double(qRound64(value * factor)) / factor;
}


}
