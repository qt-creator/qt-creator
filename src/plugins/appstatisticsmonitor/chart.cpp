// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "chart.h"

#include <utils/theme/theme.h>

#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QString>

namespace AppStatisticsMonitor::Internal {

static const int padding = 40;
static const int numPadding = 10;
static const QRectF dataRangeDefault = QRectF(0, 0, 5, 1);

Chart::Chart(const QString &name, QWidget *parent)
    : QWidget(parent)
    , m_name(name)
{
    setMinimumHeight(200);
    setMinimumWidth(400);
}

void Chart::addNewPoint(const QPointF &point)
{
    m_points.append(point);
    update();
}

void Chart::loadNewProcessData(QList<double> data)
{
    clear();
    for (long i = 0; i < data.size(); ++i) {
        m_points.append(QPointF(i + 1, data[i]));
    }
    update();
}

double Chart::lastPointX() const
{
    if (m_points.isEmpty())
        return 0;
    return m_points.last().x();
}

void Chart::clear()
{
    m_points.clear();
    addNewPoint({0, 0});
    update();
}

void Chart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.fillRect(rect(), Utils::creatorTheme()->color(Utils::Theme::Token_Background_Default));

    // add the name of the chart in the middle of the widget width and on the top
    painter.drawText(
        rect(),
        Qt::AlignHCenter | Qt::AlignTop,
        m_name + QString::number(m_points.last().y(), 'g', 4) + "%");

    const QRectF dataRange = calculateDataRange();
    updateScalingFactors(dataRange);

    for (double x = dataRange.left(); x <= dataRange.right(); x += m_xGridStep) {
        double xPos = padding + (x - dataRange.left()) * m_xScale;
        if (xPos < padding || xPos > width() - padding)
            continue;
        painter.setPen(Utils::creatorTheme()->color(Utils::Theme::Token_Foreground_Default));
        painter.drawLine(xPos, padding, xPos, height() - padding);

        painter.setPen(Utils::creatorTheme()->color(Utils::Theme::Token_Text_Muted));
        painter.drawText(xPos, height() - numPadding, QString::number(x));
    }

    for (double y = dataRange.top(); y <= dataRange.bottom(); y += m_yGridStep) {
        double yPos = height() - padding - (y - (int) dataRange.top()) * m_yScale;
        if (yPos < padding || yPos > height() - padding)
            continue;

        painter.setPen(Utils::creatorTheme()->color(Utils::Theme::Token_Foreground_Default));
        painter.drawLine(padding, yPos, width() - padding, yPos);

        painter.setPen(Utils::creatorTheme()->color(Utils::Theme::Token_Text_Muted));
        painter.drawText(numPadding, yPos, QString::number(y));
    }

    painter.setPen(Utils::creatorTheme()->color(Utils::Theme::Token_Foreground_Default));
    painter.drawLine(padding, height() - padding, width() - padding, height() - padding); // X axis
    painter.drawLine(padding, height() - padding, padding, padding); // Y axis

    QPen pen(Utils::creatorTheme()->color(Utils::Theme::Token_Accent_Default));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setRenderHint(QPainter::Antialiasing);
    for (int i = 1; i < m_points.size(); ++i) {
        QPointF startPoint(
            padding + (m_points[i - 1].x() - dataRange.left()) * m_xScale,
            height() - padding - (m_points[i - 1].y() - dataRange.top()) * m_yScale);
        QPointF endPoint(
            padding + (m_points[i].x() - dataRange.left()) * m_xScale,
            height() - padding - (m_points[i].y() - dataRange.top()) * m_yScale);
        painter.drawLine(startPoint, endPoint);
    }
}

void Chart::addPoint()
{
    for (int i = 0; i < 10; ++i) {
        double x = m_points.size();
        double y = sin(x) + 10 * cos(x / 10) + 10;
        m_points.append(QPointF(x, y));
    }
    update();
}

QRectF Chart::calculateDataRange() const
{
    QRectF dataRange = QRectF(0, 0, 0, 0);

    if (m_points.isEmpty())
        return dataRange;

    for (const QPointF &point : m_points) {
        dataRange.setLeft(qMin(dataRange.left(), point.x()));
        dataRange.setRight(qMax(dataRange.right(), point.x()));

        dataRange.setBottom(qMin(dataRange.bottom(), point.y()));
        dataRange.setTop(qMax(dataRange.top(), point.y()));
    }
    dataRange.setRight(round(dataRange.right()) + 1);
    dataRange.setTop(round(dataRange.top()) + 1);

    dataRange = dataRange.united(dataRangeDefault);
    return dataRange;
}

void Chart::updateScalingFactors(const QRectF &dataRange)
{
    const double xRange = dataRange.right() - dataRange.left();
    double yRange = dataRange.bottom() - dataRange.top();
    yRange = yRange == 0 ? dataRange.top() : yRange;

    m_xGridStep = qRound(xRange / 10);
    m_xGridStep = m_xGridStep == 0 ? 1 : m_xGridStep;

    m_yGridStep = yRange / 5;
    m_yGridStep = qRound(m_yGridStep * 10.0) / 10.0;
    if (yRange > 10)
        m_yGridStep = qRound(m_yGridStep);
    m_yGridStep = qMax(m_yGridStep, 0.1);

    m_xScale = (width() - 2 * padding) / xRange;
    m_yScale = (height() - 2 * padding) / yRange;
}
} // namespace AppStatisticsMonitor::Internal
