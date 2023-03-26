// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <cmath>

QT_BEGIN_NAMESPACE
class QColor;
class QPalette;
class QPointF;
class QRectF;
class QTransform;
QT_END_NAMESPACE

#include <vector>

namespace QmlDesigner {

double scaleX(const QTransform &transform);

double scaleY(const QTransform &transform);

void grow(QRectF &rect, const QPointF &point);

QRectF bbox(const QRectF &rect, const QTransform &transform);

QPalette singleColorPalette(const QColor &color);

template<typename T>
inline void freeClear(T &vec)
{
    for (auto *&el : vec)
        delete el;
    vec.clear();
}

template<typename TV, typename TC>
inline TV clamp(const TV &val, const TC &lo, const TC &hi)
{
    return val < lo ? lo : (val > hi ? hi : val);
}

template<typename T>
inline T lerp(double blend, const T &a, const T &b)
{
    return (1.0 - blend) * a + blend * b;
}

template<typename T>
inline T reverseLerp(double blend, const T &a, const T &b)
{
    return (blend - b) / (a - b);
}

template<typename T>
inline int roundToInt(const T &val)
{
    return static_cast<int>(std::round(val));
}

} // End namespace QmlDesigner.
