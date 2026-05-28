// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QtGlobal>

namespace Timeline {

// Fraction of the visible range that `time` falls at.
// Returns 0 when rangeEnd == rangeStart (degenerate range).
inline double timeToProportion(qint64 time, qint64 rangeStart, qint64 rangeEnd)
{
    if (rangeEnd == rangeStart)
        return 0.0;
    return double(time - rangeStart) / double(rangeEnd - rangeStart);
}

// Pixel x-position for `time` given a widget of `width` pixels.
// No clamping: values outside [0, width] are valid (caller clips as needed).
inline double timeToPixel(qint64 time, qint64 rangeStart, qint64 rangeEnd, double width)
{
    return timeToProportion(time, rangeStart, rangeEnd) * width;
}

// Inverse: time value for pixel x.
// Returns rangeStart when width <= 0.
inline qint64 pixelToTime(double x, double width, qint64 rangeStart, qint64 rangeEnd)
{
    if (width <= 0.0)
        return rangeStart;
    return rangeStart + qint64(x / width * double(rangeEnd - rangeStart));
}

// Row index and y-offset within that row, given a flat y and a list of row heights.
// Returns {-1, 0} if y is out of range (negative or beyond total height).
struct RowHit
{
    int row;
    int yWithinRow;
};

inline RowHit rowAt(int y, const QList<int> &rowHeights)
{
    if (y < 0)
        return {-1, 0};
    int top = 0;
    for (int i = 0; i < rowHeights.size(); ++i) {
        int bottom = top + rowHeights[i];
        if (y < bottom)
            return {i, y - top};
        top = bottom;
    }
    return {-1, 0};
}

// Top pixel of row `index` given accumulated row heights.
// Returns the total height of all rows if index >= rowHeights.size().
inline int rowTop(int index, const QList<int> &rowHeights)
{
    int top = 0;
    for (int i = 0; i < index && i < rowHeights.size(); ++i)
        top += rowHeights[i];
    return top;
}

} // namespace Timeline
