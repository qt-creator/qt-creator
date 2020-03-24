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

#include <QPointF>
#include <QVariant>

namespace DesignTools {

class Keyframe
{
public:
    enum class Interpolation { Undefined, Step, Linear, Bezier, Easing };

    Keyframe();

    Keyframe(const QPointF &position);

    Keyframe(const QPointF &position, const QVariant &data);

    Keyframe(const QPointF &position, const QPointF &leftHandle, const QPointF &rightHandle);

    bool isValid() const;

    bool hasData() const;

    bool isUnified() const;

    bool hasLeftHandle() const;

    bool hasRightHandle() const;

    QPointF position() const;

    QPointF leftHandle() const;

    QPointF rightHandle() const;

    QVariant data() const;

    std::string string() const;

    Interpolation interpolation() const;

    void setUnified(bool unified);

    void setPosition(const QPointF &pos);

    void setLeftHandle(const QPointF &pos);

    void setRightHandle(const QPointF &pos);

    void setData(const QVariant &data);

    void setInterpolation(Interpolation interpol);

private:
    Interpolation m_interpolation = Interpolation::Undefined;

    bool m_unified;

    QPointF m_position;

    QPointF m_leftHandle;

    QPointF m_rightHandle;

    QVariant m_data;
};

std::string toString(Keyframe::Interpolation interpol);

} // End namespace DesignTools.
