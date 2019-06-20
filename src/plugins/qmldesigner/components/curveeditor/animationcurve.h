/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "keyframe.h"

#include <vector>

namespace DesignTools {

class AnimationCurve
{
public:
    AnimationCurve();

    AnimationCurve(const std::vector<Keyframe> &frames);

    bool isValid() const;

    double minimumTime() const;

    double maximumTime() const;

    double minimumValue() const;

    double maximumValue() const;

    std::vector<Keyframe> keyframes() const;

    std::vector<QPointF> extrema() const;

    std::vector<double> yForX(double x) const;

    std::vector<double> xForY(double y, uint segment) const;

    bool intersects(const QPointF &coord, double radius);

private:
    std::vector<Keyframe> m_frames;

    double m_minY;

    double m_maxY;
};

} // End namespace DesignTools.
