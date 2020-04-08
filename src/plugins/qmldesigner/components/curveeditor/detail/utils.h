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

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QColor;
class QPalette;
class QPointF;
class QRectF;
class QTransform;
QT_END_NAMESPACE

#include <vector>

namespace DesignTools {

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
inline double clamp(const TV &val, const TC &lo, const TC &hi)
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

} // End namespace DesignTools.
