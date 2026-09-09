// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "boxdrawing.h"

#include <QPainter>
#include <QPainterPath>

#include <array>
#include <cmath>
#include <optional>

namespace TerminalSolution {
namespace {

// How thick an arm is drawn. Two letter names keep the table rows short.
enum Weight : quint8 {
    NN = 0, // No line, the arm is not there at all
    LL = 1, // Light, one thin line
    HH = 2, // Heavy, one thick line, twice as thick as a light one
    DD = 3, // Double, two light rails with a light gap between them
};

// Arm directions. The order matters: opposite is (dir + 2) % 4, perpendicular (dir + 1)
// and (dir + 3) % 4.
enum Direction { Up = 0, Right = 1, Down = 2, Left = 3 };
constexpr int directionCount = 4;

enum Axis { Horizontal, Vertical };

// Up and Down are vertical, Right and Left horizontal.
constexpr Axis axisOf(int direction)
{
    return direction % 2 == 0 ? Vertical : Horizontal;
}

// The first and last character of the two Unicode blocks, used as range bounds.
constexpr char16_t lightHorizontal = 0x2500;
constexpr char16_t heavyUpAndLightDown = 0x257F;
constexpr char16_t quadrantUpperRightAndLowerLeftAndLowerRight = 0x259F;

// Box drawing characters that are not plain line junctions: zero in the table below,
// painted by dashSpec(), paintArc() and paintDiagonal() instead.
constexpr char16_t lightTripleDashHorizontal = 0x2504;
constexpr char16_t heavyTripleDashHorizontal = 0x2505;
constexpr char16_t lightTripleDashVertical = 0x2506;
constexpr char16_t heavyTripleDashVertical = 0x2507;
constexpr char16_t lightQuadrupleDashHorizontal = 0x2508;
constexpr char16_t heavyQuadrupleDashHorizontal = 0x2509;
constexpr char16_t lightQuadrupleDashVertical = 0x250A;
constexpr char16_t heavyQuadrupleDashVertical = 0x250B;
constexpr char16_t lightDoubleDashHorizontal = 0x254C;
constexpr char16_t heavyDoubleDashHorizontal = 0x254D;
constexpr char16_t lightDoubleDashVertical = 0x254E;
constexpr char16_t heavyDoubleDashVertical = 0x254F;
constexpr char16_t lightArcDownAndRight = 0x256D;
constexpr char16_t lightArcDownAndLeft = 0x256E;
constexpr char16_t lightArcUpAndLeft = 0x256F;
constexpr char16_t lightArcUpAndRight = 0x2570;
constexpr char16_t lightDiagonalUpperRightToLowerLeft = 0x2571;
constexpr char16_t lightDiagonalUpperLeftToLowerRight = 0x2572;
constexpr char16_t lightDiagonalCross = 0x2573;

// The block elements, not in the table at all. paintBlock() derives all of them.
constexpr char16_t upperHalfBlock = 0x2580;
constexpr char16_t lowerOneEighthBlock = 0x2581;
constexpr char16_t lowerSevenEighthsBlock = 0x2587;
constexpr char16_t fullBlock = 0x2588;
constexpr char16_t leftSevenEighthsBlock = 0x2589;
constexpr char16_t leftOneEighthBlock = 0x258F;
constexpr char16_t rightHalfBlock = 0x2590;
constexpr char16_t lightShade = 0x2591;
constexpr char16_t darkShade = 0x2593;
constexpr char16_t upperOneEighthBlock = 0x2594;
constexpr char16_t rightOneEighthBlock = 0x2595;
constexpr char16_t quadrantLowerLeft = 0x2596;

// The powerline separators, private use area, so unnamed by Unicode. These are the
// powerline project's names: hard is the filled triangle, soft the line.
constexpr char16_t rightHardDivider = 0xE0B0;
constexpr char16_t rightSoftDivider = 0xE0B1;
constexpr char16_t leftHardDivider = 0xE0B2;
constexpr char16_t leftSoftDivider = 0xE0B3;

// A spec packs the four arm weights, one nibble each, in Direction order.
constexpr int armBits = 4;
constexpr quint16 armMask = 0xf;

constexpr int armShift(int direction)
{
    return (directionCount - 1 - direction) * armBits;
}

constexpr quint16 arms(Weight up, Weight right, Weight down, Weight left)
{
    return quint16(
        (quint16(up) << armShift(Up)) | (quint16(right) << armShift(Right))
        | (quint16(down) << armShift(Down)) | (quint16(left) << armShift(Left)));
}

constexpr std::array<Weight, directionCount> armWeightsOf(quint16 spec)
{
    std::array<Weight, directionCount> weights = {};
    for (int dir = Up; dir < directionCount; ++dir)
        weights[dir] = Weight((spec >> armShift(dir)) & armMask);
    return weights;
}

// Indexed by (code point - lightHorizontal). Zero means not a plain line junction, see
// dashSpec(), paintArc() and paintDiagonal(). The comments are the Unicode names, which
// the constants above carry without the "BOX DRAWINGS" they all share.
constexpr std::array<quint16, heavyUpAndLightDown - lightHorizontal + 1> lineTable = {{
    arms(NN, LL, NN, LL), // U+2500 BOX DRAWINGS LIGHT HORIZONTAL
    arms(NN, HH, NN, HH), // U+2501 BOX DRAWINGS HEAVY HORIZONTAL
    arms(LL, NN, LL, NN), // U+2502 BOX DRAWINGS LIGHT VERTICAL
    arms(HH, NN, HH, NN), // U+2503 BOX DRAWINGS HEAVY VERTICAL
    0,                    // U+2504 BOX DRAWINGS LIGHT TRIPLE DASH HORIZONTAL
    0,                    // U+2505 BOX DRAWINGS HEAVY TRIPLE DASH HORIZONTAL
    0,                    // U+2506 BOX DRAWINGS LIGHT TRIPLE DASH VERTICAL
    0,                    // U+2507 BOX DRAWINGS HEAVY TRIPLE DASH VERTICAL
    0,                    // U+2508 BOX DRAWINGS LIGHT QUADRUPLE DASH HORIZONTAL
    0,                    // U+2509 BOX DRAWINGS HEAVY QUADRUPLE DASH HORIZONTAL
    0,                    // U+250A BOX DRAWINGS LIGHT QUADRUPLE DASH VERTICAL
    0,                    // U+250B BOX DRAWINGS HEAVY QUADRUPLE DASH VERTICAL
    arms(NN, LL, LL, NN), // U+250C BOX DRAWINGS LIGHT DOWN AND RIGHT
    arms(NN, HH, LL, NN), // U+250D BOX DRAWINGS DOWN LIGHT AND RIGHT HEAVY
    arms(NN, LL, HH, NN), // U+250E BOX DRAWINGS DOWN HEAVY AND RIGHT LIGHT
    arms(NN, HH, HH, NN), // U+250F BOX DRAWINGS HEAVY DOWN AND RIGHT
    arms(NN, NN, LL, LL), // U+2510 BOX DRAWINGS LIGHT DOWN AND LEFT
    arms(NN, NN, LL, HH), // U+2511 BOX DRAWINGS DOWN LIGHT AND LEFT HEAVY
    arms(NN, NN, HH, LL), // U+2512 BOX DRAWINGS DOWN HEAVY AND LEFT LIGHT
    arms(NN, NN, HH, HH), // U+2513 BOX DRAWINGS HEAVY DOWN AND LEFT
    arms(LL, LL, NN, NN), // U+2514 BOX DRAWINGS LIGHT UP AND RIGHT
    arms(LL, HH, NN, NN), // U+2515 BOX DRAWINGS UP LIGHT AND RIGHT HEAVY
    arms(HH, LL, NN, NN), // U+2516 BOX DRAWINGS UP HEAVY AND RIGHT LIGHT
    arms(HH, HH, NN, NN), // U+2517 BOX DRAWINGS HEAVY UP AND RIGHT
    arms(LL, NN, NN, LL), // U+2518 BOX DRAWINGS LIGHT UP AND LEFT
    arms(LL, NN, NN, HH), // U+2519 BOX DRAWINGS UP LIGHT AND LEFT HEAVY
    arms(HH, NN, NN, LL), // U+251A BOX DRAWINGS UP HEAVY AND LEFT LIGHT
    arms(HH, NN, NN, HH), // U+251B BOX DRAWINGS HEAVY UP AND LEFT
    arms(LL, LL, LL, NN), // U+251C BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    arms(LL, HH, LL, NN), // U+251D BOX DRAWINGS VERTICAL LIGHT AND RIGHT HEAVY
    arms(HH, LL, LL, NN), // U+251E BOX DRAWINGS UP HEAVY AND RIGHT DOWN LIGHT
    arms(LL, LL, HH, NN), // U+251F BOX DRAWINGS DOWN HEAVY AND RIGHT UP LIGHT
    arms(HH, LL, HH, NN), // U+2520 BOX DRAWINGS VERTICAL HEAVY AND RIGHT LIGHT
    arms(HH, HH, LL, NN), // U+2521 BOX DRAWINGS DOWN LIGHT AND RIGHT UP HEAVY
    arms(LL, HH, HH, NN), // U+2522 BOX DRAWINGS UP LIGHT AND RIGHT DOWN HEAVY
    arms(HH, HH, HH, NN), // U+2523 BOX DRAWINGS HEAVY VERTICAL AND RIGHT
    arms(LL, NN, LL, LL), // U+2524 BOX DRAWINGS LIGHT VERTICAL AND LEFT
    arms(LL, NN, LL, HH), // U+2525 BOX DRAWINGS VERTICAL LIGHT AND LEFT HEAVY
    arms(HH, NN, LL, LL), // U+2526 BOX DRAWINGS UP HEAVY AND LEFT DOWN LIGHT
    arms(LL, NN, HH, LL), // U+2527 BOX DRAWINGS DOWN HEAVY AND LEFT UP LIGHT
    arms(HH, NN, HH, LL), // U+2528 BOX DRAWINGS VERTICAL HEAVY AND LEFT LIGHT
    arms(HH, NN, LL, HH), // U+2529 BOX DRAWINGS DOWN LIGHT AND LEFT UP HEAVY
    arms(LL, NN, HH, HH), // U+252A BOX DRAWINGS UP LIGHT AND LEFT DOWN HEAVY
    arms(HH, NN, HH, HH), // U+252B BOX DRAWINGS HEAVY VERTICAL AND LEFT
    arms(NN, LL, LL, LL), // U+252C BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    arms(NN, LL, LL, HH), // U+252D BOX DRAWINGS LEFT HEAVY AND RIGHT DOWN LIGHT
    arms(NN, HH, LL, LL), // U+252E BOX DRAWINGS RIGHT HEAVY AND LEFT DOWN LIGHT
    arms(NN, HH, LL, HH), // U+252F BOX DRAWINGS DOWN LIGHT AND HORIZONTAL HEAVY
    arms(NN, LL, HH, LL), // U+2530 BOX DRAWINGS DOWN HEAVY AND HORIZONTAL LIGHT
    arms(NN, LL, HH, HH), // U+2531 BOX DRAWINGS RIGHT LIGHT AND LEFT DOWN HEAVY
    arms(NN, HH, HH, LL), // U+2532 BOX DRAWINGS LEFT LIGHT AND RIGHT DOWN HEAVY
    arms(NN, HH, HH, HH), // U+2533 BOX DRAWINGS HEAVY DOWN AND HORIZONTAL
    arms(LL, LL, NN, LL), // U+2534 BOX DRAWINGS LIGHT UP AND HORIZONTAL
    arms(LL, LL, NN, HH), // U+2535 BOX DRAWINGS LEFT HEAVY AND RIGHT UP LIGHT
    arms(LL, HH, NN, LL), // U+2536 BOX DRAWINGS RIGHT HEAVY AND LEFT UP LIGHT
    arms(LL, HH, NN, HH), // U+2537 BOX DRAWINGS UP LIGHT AND HORIZONTAL HEAVY
    arms(HH, LL, NN, LL), // U+2538 BOX DRAWINGS UP HEAVY AND HORIZONTAL LIGHT
    arms(HH, LL, NN, HH), // U+2539 BOX DRAWINGS RIGHT LIGHT AND LEFT UP HEAVY
    arms(HH, HH, NN, LL), // U+253A BOX DRAWINGS LEFT LIGHT AND RIGHT UP HEAVY
    arms(HH, HH, NN, HH), // U+253B BOX DRAWINGS HEAVY UP AND HORIZONTAL
    arms(LL, LL, LL, LL), // U+253C BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    arms(LL, LL, LL, HH), // U+253D BOX DRAWINGS LEFT HEAVY AND RIGHT VERTICAL LIGHT
    arms(LL, HH, LL, LL), // U+253E BOX DRAWINGS RIGHT HEAVY AND LEFT VERTICAL LIGHT
    arms(LL, HH, LL, HH), // U+253F BOX DRAWINGS VERTICAL LIGHT AND HORIZONTAL HEAVY
    arms(HH, LL, LL, LL), // U+2540 BOX DRAWINGS UP HEAVY AND DOWN HORIZONTAL LIGHT
    arms(LL, LL, HH, LL), // U+2541 BOX DRAWINGS DOWN HEAVY AND UP HORIZONTAL LIGHT
    arms(HH, LL, HH, LL), // U+2542 BOX DRAWINGS VERTICAL HEAVY AND HORIZONTAL LIGHT
    arms(HH, LL, LL, HH), // U+2543 BOX DRAWINGS LEFT UP HEAVY AND RIGHT DOWN LIGHT
    arms(HH, HH, LL, LL), // U+2544 BOX DRAWINGS RIGHT UP HEAVY AND LEFT DOWN LIGHT
    arms(LL, LL, HH, HH), // U+2545 BOX DRAWINGS LEFT DOWN HEAVY AND RIGHT UP LIGHT
    arms(LL, HH, HH, LL), // U+2546 BOX DRAWINGS RIGHT DOWN HEAVY AND LEFT UP LIGHT
    arms(HH, HH, LL, HH), // U+2547 BOX DRAWINGS DOWN LIGHT AND UP HORIZONTAL HEAVY
    arms(LL, HH, HH, HH), // U+2548 BOX DRAWINGS UP LIGHT AND DOWN HORIZONTAL HEAVY
    arms(HH, LL, HH, HH), // U+2549 BOX DRAWINGS RIGHT LIGHT AND LEFT VERTICAL HEAVY
    arms(HH, HH, HH, LL), // U+254A BOX DRAWINGS LEFT LIGHT AND RIGHT VERTICAL HEAVY
    arms(HH, HH, HH, HH), // U+254B BOX DRAWINGS HEAVY VERTICAL AND HORIZONTAL
    0,                    // U+254C BOX DRAWINGS LIGHT DOUBLE DASH HORIZONTAL
    0,                    // U+254D BOX DRAWINGS HEAVY DOUBLE DASH HORIZONTAL
    0,                    // U+254E BOX DRAWINGS LIGHT DOUBLE DASH VERTICAL
    0,                    // U+254F BOX DRAWINGS HEAVY DOUBLE DASH VERTICAL
    arms(NN, DD, NN, DD), // U+2550 BOX DRAWINGS DOUBLE HORIZONTAL
    arms(DD, NN, DD, NN), // U+2551 BOX DRAWINGS DOUBLE VERTICAL
    arms(NN, DD, LL, NN), // U+2552 BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    arms(NN, LL, DD, NN), // U+2553 BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    arms(NN, DD, DD, NN), // U+2554 BOX DRAWINGS DOUBLE DOWN AND RIGHT
    arms(NN, NN, LL, DD), // U+2555 BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    arms(NN, NN, DD, LL), // U+2556 BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    arms(NN, NN, DD, DD), // U+2557 BOX DRAWINGS DOUBLE DOWN AND LEFT
    arms(LL, DD, NN, NN), // U+2558 BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    arms(DD, LL, NN, NN), // U+2559 BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    arms(DD, DD, NN, NN), // U+255A BOX DRAWINGS DOUBLE UP AND RIGHT
    arms(LL, NN, NN, DD), // U+255B BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    arms(DD, NN, NN, LL), // U+255C BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    arms(DD, NN, NN, DD), // U+255D BOX DRAWINGS DOUBLE UP AND LEFT
    arms(LL, DD, LL, NN), // U+255E BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    arms(DD, LL, DD, NN), // U+255F BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    arms(DD, DD, DD, NN), // U+2560 BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    arms(LL, NN, LL, DD), // U+2561 BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    arms(DD, NN, DD, LL), // U+2562 BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    arms(DD, NN, DD, DD), // U+2563 BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    arms(NN, DD, LL, DD), // U+2564 BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    arms(NN, LL, DD, LL), // U+2565 BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    arms(NN, DD, DD, DD), // U+2566 BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    arms(LL, DD, NN, DD), // U+2567 BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    arms(DD, LL, NN, LL), // U+2568 BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    arms(DD, DD, NN, DD), // U+2569 BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    arms(LL, DD, LL, DD), // U+256A BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    arms(DD, LL, DD, LL), // U+256B BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    arms(DD, DD, DD, DD), // U+256C BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    0,                    // U+256D BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
    0,                    // U+256E BOX DRAWINGS LIGHT ARC DOWN AND LEFT
    0,                    // U+256F BOX DRAWINGS LIGHT ARC UP AND LEFT
    0,                    // U+2570 BOX DRAWINGS LIGHT ARC UP AND RIGHT
    0,                    // U+2571 BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT
    0,                    // U+2572 BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
    0,                    // U+2573 BOX DRAWINGS LIGHT DIAGONAL CROSS
    arms(NN, NN, NN, LL), // U+2574 BOX DRAWINGS LIGHT LEFT
    arms(LL, NN, NN, NN), // U+2575 BOX DRAWINGS LIGHT UP
    arms(NN, LL, NN, NN), // U+2576 BOX DRAWINGS LIGHT RIGHT
    arms(NN, NN, LL, NN), // U+2577 BOX DRAWINGS LIGHT DOWN
    arms(NN, NN, NN, HH), // U+2578 BOX DRAWINGS HEAVY LEFT
    arms(HH, NN, NN, NN), // U+2579 BOX DRAWINGS HEAVY UP
    arms(NN, HH, NN, NN), // U+257A BOX DRAWINGS HEAVY RIGHT
    arms(NN, NN, HH, NN), // U+257B BOX DRAWINGS HEAVY DOWN
    arms(NN, HH, NN, LL), // U+257C BOX DRAWINGS LIGHT LEFT AND HEAVY RIGHT
    arms(LL, NN, HH, NN), // U+257D BOX DRAWINGS LIGHT UP AND HEAVY DOWN
    arms(NN, LL, NN, HH), // U+257E BOX DRAWINGS HEAVY LEFT AND LIGHT RIGHT
    arms(HH, NN, LL, NN), // U+257F BOX DRAWINGS HEAVY UP AND LIGHT DOWN
}};

// The quadrants a block element can fill.
constexpr quint8 upperLeftQuadrant = 0b0001;
constexpr quint8 upperRightQuadrant = 0b0010;
constexpr quint8 lowerLeftQuadrant = 0b0100;
constexpr quint8 lowerRightQuadrant = 0b1000;

// Which quadrants each of the last block elements fills. The comments are the Unicode
// names without the "QUADRANT" they all share.
constexpr std::array<quint8, quadrantUpperRightAndLowerLeftAndLowerRight - quadrantLowerLeft + 1>
    quadrantTable = {{
        // U+2596 LOWER LEFT
        lowerLeftQuadrant,
        // U+2597 LOWER RIGHT
        lowerRightQuadrant,
        // U+2598 UPPER LEFT
        upperLeftQuadrant,
        // U+2599 UPPER LEFT AND LOWER LEFT AND LOWER RIGHT
        upperLeftQuadrant | lowerLeftQuadrant | lowerRightQuadrant,
        // U+259A UPPER LEFT AND LOWER RIGHT
        upperLeftQuadrant | lowerRightQuadrant,
        // U+259B UPPER LEFT AND UPPER RIGHT AND LOWER LEFT
        upperLeftQuadrant | upperRightQuadrant | lowerLeftQuadrant,
        // U+259C UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT
        upperLeftQuadrant | upperRightQuadrant | lowerRightQuadrant,
        // U+259D UPPER RIGHT
        upperRightQuadrant,
        // U+259E UPPER RIGHT AND LOWER LEFT
        upperRightQuadrant | lowerLeftQuadrant,
        // U+259F UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT
        upperRightQuadrant | lowerLeftQuadrant | lowerRightQuadrant,
    }};

struct DashSpec
{
    Axis axis;
    Weight weight;
    int count;
};

std::optional<DashSpec> dashSpec(char16_t c)
{
    switch (c) {
    case lightTripleDashHorizontal:
        return DashSpec{Horizontal, LL, 3};
    case heavyTripleDashHorizontal:
        return DashSpec{Horizontal, HH, 3};
    case lightTripleDashVertical:
        return DashSpec{Vertical, LL, 3};
    case heavyTripleDashVertical:
        return DashSpec{Vertical, HH, 3};
    case lightQuadrupleDashHorizontal:
        return DashSpec{Horizontal, LL, 4};
    case heavyQuadrupleDashHorizontal:
        return DashSpec{Horizontal, HH, 4};
    case lightQuadrupleDashVertical:
        return DashSpec{Vertical, LL, 4};
    case heavyQuadrupleDashVertical:
        return DashSpec{Vertical, HH, 4};
    case lightDoubleDashHorizontal:
        return DashSpec{Horizontal, LL, 2};
    case heavyDoubleDashHorizontal:
        return DashSpec{Horizontal, HH, 2};
    case lightDoubleDashVertical:
        return DashSpec{Vertical, LL, 2};
    case heavyDoubleDashVertical:
        return DashSpec{Vertical, HH, 2};
    default:
        return {};
    }
}

// All coordinates are snapped to whole device pixels. Neighbouring cells snap their
// shared edge from the same value, so the shapes tile without seams.
struct Geometry
{
    QRectF cell;
    qreal dpr = 1.0;
    qreal light = 1.0;
    qreal heavy = 2.0;

    qreal thickness(Weight w) const { return w == HH ? heavy : light; }

    qreal snap(qreal value) const { return snapToDevicePixel(value, dpr); }

    // A line of the given thickness from "from" to "to" along the axis, centered on
    // perpCenter across it.
    QRectF band(Axis axis, qreal from, qreal to, qreal perpCenter, qreal thickness) const
    {
        const qreal a0 = snap(qMin(from, to));
        const qreal a1 = snap(qMax(from, to));
        const qreal p0 = snap(perpCenter - thickness / 2.0);

        if (axis == Vertical)
            return QRectF(p0, a0, thickness, a1 - a0);
        return QRectF(a0, p0, a1 - a0, thickness);
    }
};

Geometry geometryFor(const QRectF &cellRect, qreal dpr)
{
    Geometry g;
    g.dpr = dpr;
    g.cell = QRectF(
        QPointF(g.snap(cellRect.left()), g.snap(cellRect.top())),
        QPointF(g.snap(cellRect.right()), g.snap(cellRect.bottom())));

    // A light line is a twelfth of the cell height, a heavy one twice that, and neither
    // thinner than a device pixel.
    constexpr qreal cellHeightsPerLightLine = 12.0;
    constexpr qreal heavyPerLight = 2.0;

    const qreal onePixel = 1.0 / dpr;
    g.light = qMax(onePixel, g.snap(cellRect.height() / cellHeightsPerLightLine));
    g.heavy = qMax(heavyPerLight * onePixel, g.snap(g.light * heavyPerLight));
    return g;
}

void paintLines(QPainter &p, quint16 spec, const Geometry &g, const QColor &color)
{
    const std::array<Weight, directionCount> weights = armWeightsOf(spec);

    const qreal centerX = g.cell.center().x();
    const qreal centerY = g.cell.center().y();

    // A double line is two light rails with a light gap, so a rail sits one light line
    // off center. Distances from the center, in rails: to the near rail's near edge, and
    // to the far rail's far edge.
    constexpr qreal nearRailEdge = 0.5;
    constexpr qreal farRailEdge = 1.5;

    for (int dir = Up; dir < directionCount; ++dir) {
        const Weight weight = weights[dir];
        if (weight == NN)
            continue;

        const Axis axis = axisOf(dir);
        const qreal center = axis == Vertical ? centerY : centerX;
        const qreal perpCenter = axis == Vertical ? centerX : centerY;

        // Points from the center of the cell towards the edge this arm ends at.
        const qreal sign = (dir == Down || dir == Right) ? 1.0 : -1.0;
        const qreal edge = dir == Up      ? g.cell.top()
                           : dir == Right ? g.cell.right()
                           : dir == Down  ? g.cell.bottom()
                                          : g.cell.left();

        // The perpendicular arms, in rail offset order: left and right for a vertical
        // arm, up and down for a horizontal one.
        const Weight negWeight = weights[axis == Vertical ? Left : Up];
        const Weight posWeight = weights[axis == Vertical ? Right : Down];
        const qreal perpThickness = qMax(
            negWeight == NN ? 0.0 : g.thickness(negWeight),
            posWeight == NN ? 0.0 : g.thickness(posWeight));

        const qreal thickness = g.thickness(weight);
        const int railCount = weight == DD ? 2 : 1;

        for (int rail = 0; rail < railCount; ++rail) {
            const qreal offset = weight == DD ? (rail == 0 ? -g.light : g.light) : 0.0;

            // Where the arm ends inside. By default the cell center, where it meets
            // the opposite arm.
            qreal inner = center;

            if (negWeight == DD || posWeight == DD) {
                // A perpendicular double line is a wall: a rail running into it stops,
                // one with no wall on its side wraps around it into the corner. This is
                // what makes the hole in U+256C.
                const bool blocked = offset == 0.0
                                         ? (negWeight == DD && posWeight == DD)
                                         : (offset < 0.0 ? negWeight == DD : posWeight == DD);
                inner = center + sign * (blocked ? nearRailEdge * g.light : -farRailEdge * g.light);
            } else if (weights[(dir + 2) % directionCount] == NN && perpThickness > 0.0) {
                // Corner or T junction: overlap the perpendicular line, or the joint
                // gets a notch.
                inner = center - sign * perpThickness / 2.0;
            }

            p.fillRect(g.band(axis, edge, inner, perpCenter + offset, thickness), color);
        }
    }
}

void paintDashes(QPainter &p, const DashSpec &spec, const Geometry &g, const QColor &color)
{
    const qreal length = spec.axis == Vertical ? g.cell.height() : g.cell.width();
    const qreal segment = length / spec.count;
    // How much of a segment is dash, the rest is the gap to the next one.
    constexpr qreal dashPerSegment = 0.6;

    const qreal dash = qMax(1.0 / g.dpr, g.snap(segment * dashPerSegment));
    const qreal start = spec.axis == Vertical ? g.cell.top() : g.cell.left();
    const qreal perpCenter = spec.axis == Vertical ? g.cell.center().x() : g.cell.center().y();
    const qreal thickness = g.thickness(spec.weight);

    for (int i = 0; i < spec.count; ++i) {
        const qreal from = start + i * segment + (segment - dash) / 2.0;
        p.fillRect(g.band(spec.axis, from, from + dash, perpCenter, thickness), color);
    }
}

void paintArc(QPainter &p, char16_t c, const Geometry &g, const QColor &color)
{
    // The two edges the arc connects, as its name says.
    bool toRight = false;
    bool toBottom = false;
    switch (c) {
    case lightArcDownAndRight:
        toRight = true, toBottom = true;
        break;
    case lightArcDownAndLeft:
        toRight = false, toBottom = true;
        break;
    case lightArcUpAndLeft:
        toRight = false, toBottom = false;
        break;
    case lightArcUpAndRight:
        toRight = true, toBottom = false;
        break;
    }

    // The pen is centered on the path, so the arc meets the lines of the neighboring cells
    // crisply only where its rails sit exactly where paintLines puts theirs.
    const auto railCenter = [&](qreal center) {
        return g.snap(center - g.light / 2.0) + g.light / 2.0;
    };
    const qreal centerX = railCenter(g.cell.center().x());
    const qreal centerY = railCenter(g.cell.center().y());

    // A quarter circle, as large as the narrower half of the cell allows, with a straight
    // arm from each of its ends to the edge. A quarter of the cell's own ellipse would be
    // stretched along the taller axis, which leaves the arc almost straight where the
    // neighbor's line continues it and bends it all in one corner.
    const qreal toEdgeX = (toRight ? g.cell.right() : g.cell.left()) - centerX;
    const qreal toEdgeY = (toBottom ? g.cell.bottom() : g.cell.top()) - centerY;
    const qreal radius = qMin(qAbs(toEdgeX), qAbs(toEdgeY));
    const qreal cornerX = toRight ? radius : -radius;
    const qreal cornerY = toBottom ? radius : -radius;
    // Where the control points of a cubic sit that approximates a quarter circle.
    constexpr qreal handleOfQuarterCircle = 0.5522847498307933;
    constexpr qreal towardsCenter = 1.0 - handleOfQuarterCircle;

    QPainterPath path(QPointF(centerX + toEdgeX, centerY));
    path.lineTo(QPointF(centerX + cornerX, centerY));
    path.cubicTo(
        QPointF(centerX + cornerX * towardsCenter, centerY),
        QPointF(centerX, centerY + cornerY * towardsCenter),
        QPointF(centerX, centerY + cornerY));
    path.lineTo(QPointF(centerX, centerY + toEdgeY));

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, g.light, Qt::SolidLine, Qt::FlatCap));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}

void paintDiagonal(QPainter &p, char16_t c, const Geometry &g, const QColor &color)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, g.light, Qt::SolidLine, Qt::FlatCap));

    if (c == lightDiagonalUpperLeftToLowerRight || c == lightDiagonalCross)
        p.drawLine(g.cell.topLeft(), g.cell.bottomRight());
    if (c == lightDiagonalUpperRightToLowerLeft || c == lightDiagonalCross)
        p.drawLine(g.cell.bottomLeft(), g.cell.topRight());
}

void paintBlock(QPainter &p, char16_t c, const Geometry &g, const QColor &color)
{
    const QRectF &r = g.cell;

    // Fractions of the cell, snapped to device pixels.
    const auto fill = [&](qreal x0, qreal y0, qreal x1, qreal y1, const QColor &col) {
        const QPointF topLeft(g.snap(r.left() + r.width() * x0), g.snap(r.top() + r.height() * y0));
        const QPointF
            bottomRight(g.snap(r.left() + r.width() * x1), g.snap(r.top() + r.height() * y1));
        p.fillRect(QRectF(topLeft, bottomRight), col);
    };

    // The eighth blocks are eighths of the cell, the shades quarters of the color.
    constexpr qreal eighthOfCell = 1.0 / 8.0;
    constexpr qreal shadeSteps = 4.0;

    if (c == upperHalfBlock) {
        fill(0.0, 0.0, 1.0, 0.5, color);
    } else if (c <= lowerSevenEighthsBlock) {
        const qreal eighths = c - lowerOneEighthBlock + 1;
        fill(0.0, 1.0 - eighths * eighthOfCell, 1.0, 1.0, color);
    } else if (c == fullBlock) {
        fill(0.0, 0.0, 1.0, 1.0, color);
    } else if (c <= leftOneEighthBlock) {
        // The left blocks count down, from seven eighths to one.
        const qreal eighths = 7 - (c - leftSevenEighthsBlock);
        fill(0.0, 0.0, eighths * eighthOfCell, 1.0, color);
    } else if (c == rightHalfBlock) {
        fill(0.5, 0.0, 1.0, 1.0, color);
    } else if (c <= darkShade) {
        // Light, medium and dark shade, a quarter darker each.
        QColor shade = color;
        shade.setAlphaF(color.alphaF() * (c - lightShade + 1) / shadeSteps);
        fill(0.0, 0.0, 1.0, 1.0, shade);
    } else if (c == upperOneEighthBlock) {
        fill(0.0, 0.0, 1.0, eighthOfCell, color);
    } else if (c == rightOneEighthBlock) {
        fill(1.0 - eighthOfCell, 0.0, 1.0, 1.0, color);
    } else {
        const quint8 mask = quadrantTable[c - quadrantLowerLeft];
        if (mask & upperLeftQuadrant)
            fill(0.0, 0.0, 0.5, 0.5, color);
        if (mask & upperRightQuadrant)
            fill(0.5, 0.0, 1.0, 0.5, color);
        if (mask & lowerLeftQuadrant)
            fill(0.0, 0.5, 0.5, 1.0, color);
        if (mask & lowerRightQuadrant)
            fill(0.5, 0.5, 1.0, 1.0, color);
    }
}

void paintPowerline(QPainter &p, char16_t c, const Geometry &g, const QColor &color)
{
    const QRectF &r = g.cell;
    const bool pointsRight = c == rightHardDivider || c == rightSoftDivider;
    const bool filled = c == rightHardDivider || c == leftHardDivider;

    const QPointF tip(pointsRight ? r.right() : r.left(), r.center().y());
    const QPointF top(pointsRight ? r.left() : r.right(), r.top());
    const QPointF bottom(pointsRight ? r.left() : r.right(), r.bottom());

    p.setRenderHint(QPainter::Antialiasing, true);

    if (filled) {
        QPainterPath path(top);
        path.lineTo(tip);
        path.lineTo(bottom);
        path.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(path);
    } else {
        p.setPen(QPen(color, g.light, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(QPolygonF({top, tip, bottom}));
    }
}

bool isBoxDrawingCharacter(char16_t c)
{
    return (c >= lightHorizontal && c <= quadrantUpperRightAndLowerLeftAndLowerRight)
           || (c >= rightHardDivider && c <= leftSoftDivider);
}

} // namespace

bool paintBoxDrawingCharacter(
    QPainter &painter, const QRectF &cellRect, char16_t c, qreal devicePixelRatio)
{
    if (!isBoxDrawingCharacter(c))
        return false;

    const Geometry g = geometryFor(cellRect, devicePixelRatio);
    const QColor color = painter.pen().color();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    if (c >= rightHardDivider) {
        paintPowerline(painter, c, g, color);
    } else if (c > heavyUpAndLightDown) {
        paintBlock(painter, c, g, color);
    } else if (const quint16 spec = lineTable[c - lightHorizontal]) {
        paintLines(painter, spec, g, color);
    } else if (const std::optional<DashSpec> dashes = dashSpec(c)) {
        paintDashes(painter, *dashes, g, color);
    } else if (c >= lightArcDownAndRight && c <= lightArcUpAndRight) {
        paintArc(painter, c, g, color);
    } else {
        paintDiagonal(painter, c, g, color);
    }

    painter.restore();
    return true;
}

} // namespace TerminalSolution
