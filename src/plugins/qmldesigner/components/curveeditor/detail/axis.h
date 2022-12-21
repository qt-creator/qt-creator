// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QToolBar>
#include <QWidget>

namespace QmlDesigner {

struct Axis
{
    static Axis compute(double dmin, double dmax, double height, double pt);

    double lmin;
    double lmax;
    double lstep;
};

} // End namespace QmlDesigner.
