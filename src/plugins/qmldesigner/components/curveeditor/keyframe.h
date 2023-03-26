// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPointF>
#include <QVariant>

#include <string>

namespace QmlDesigner {

class Keyframe
{
public:
    enum class ValueType { Undefined, Bool, Integer, Double };
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

} // End namespace QmlDesigner.
