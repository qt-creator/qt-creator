// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "chart.h"

#include <utils/theme/theme.h>

#include <QAbstractAxis>
#include <QChartView>
#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRandomGenerator>
#include <QSplineSeries>
#include <QString>
#include <QValueAxis>

using namespace Utils;

namespace AppStatisticsMonitor::Internal {

static const int padding = 40;
static const int numPadding = 10;
static const QRectF dataRangeDefault = QRectF(0, 0, 5, 1);

AppStatisticsMonitorChart::AppStatisticsMonitorChart(
    const QString &name, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(QChart::ChartTypeCartesian, parent, wFlags)
    , m_series(new QLineSeries(this))
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_point(0, 0)
    , m_chartView(new QChartView(this))
    , m_name(name)
{
    m_chartView->setMinimumHeight(200);
    m_chartView->setMinimumWidth(400);
    const QBrush brushTitle(creatorColor(Theme::Token_Text_Muted));
    // const QBrush brush(creatorColor(Theme::Token_Background_Default)); left for the future
    const QBrush brush(creatorColor(Theme::BackgroundColorNormal));
    const QPen penBack(creatorColor(Theme::Token_Text_Muted));
    const QPen penAxis(creatorColor(Theme::Token_Text_Muted));

    setTitleBrush(brushTitle);
    setBackgroundBrush(brush);
    setBackgroundPen(penBack);
    m_axisX->setLinePen(penAxis);
    m_axisY->setLinePen(penAxis);
    m_axisX->setLabelsColor(creatorColor(Theme::Token_Text_Muted));
    m_axisY->setLabelsColor(creatorColor(Theme::Token_Text_Muted));
    QPen pen(creatorColor(Theme::Token_Accent_Default));
    pen.setWidth(2);
    m_series->setPen(pen);

    setTitle(m_name + " " + QString::number(m_point.y(), 'g', 4) + "%");
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addSeries(m_series);

    addAxis(m_axisX, Qt::AlignBottom);
    addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_axisX->applyNiceNumbers();
    m_axisY->applyNiceNumbers();
    legend()->hide();

    clear();
}

QChartView *AppStatisticsMonitorChart::chartView()
{
    return m_chartView;
}

void AppStatisticsMonitorChart::addNewPoint(const QPointF &point)
{
    m_point = point;
    if (m_axisY->max() < m_point.y())
        m_axisY->setRange(0, qRound(m_point.y()) + 1);
    m_axisX->setRange(0, qRound(m_point.x()) + 1);

    setTitle(m_name + " " + QString::number(m_point.y(), 'g', 4) + "%");
    m_series->append(m_point);
}

void AppStatisticsMonitorChart::loadNewProcessData(const QList<double> &data)
{
    clear();
    QList<QPointF> points{{0, 0}};
    int i = 0;
    double max_y = 0;

    for (double e : qAsConst(data)) {
        points.push_back({double(++i), e});
        max_y = qMax(max_y, e);
    }

    m_axisY->setRange(0, qRound(max_y) + 1);
    m_axisX->setRange(0, data.size() + 1);

    m_series->clear();
    m_series->append(points);
}

void AppStatisticsMonitorChart::clear()
{
    m_axisX->setRange(0, 5);
    m_axisY->setRange(0, 1);
    m_series->clear();
    m_series->append(0, 0);
}

double AppStatisticsMonitorChart::lastPointX() const
{
    return m_point.x();
}

//---------------------- Chart -----------------------

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

void Chart::loadNewProcessData(const QList<double> &data)
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
    // painter.fillRect(rect(), creatorColor(Theme::Token_Background_Default)); left for the future
    painter.fillRect(rect(), creatorColor(Theme::BackgroundColorNormal));

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
        painter.setPen(creatorColor(Theme::Token_Foreground_Default));
        painter.drawLine(xPos, padding, xPos, height() - padding);

        painter.setPen(creatorColor(Theme::Token_Text_Muted));
        painter.drawText(xPos, height() - numPadding, QString::number(x));
    }

    for (double y = dataRange.top(); y <= dataRange.bottom(); y += m_yGridStep) {
        double yPos = height() - padding - (y - (int) dataRange.top()) * m_yScale;
        if (yPos < padding || yPos > height() - padding)
            continue;

        painter.setPen(creatorColor(Theme::Token_Foreground_Default));
        painter.drawLine(padding, yPos, width() - padding, yPos);

        painter.setPen(creatorColor(Theme::Token_Text_Muted));
        painter.drawText(numPadding, yPos, QString::number(y));
    }

    painter.setPen(creatorColor(Theme::Token_Foreground_Default));
    painter.drawLine(padding, height() - padding, width() - padding, height() - padding); // X axis
    painter.drawLine(padding, height() - padding, padding, padding); // Y axis

    QPen pen(creatorColor(Theme::Token_Accent_Default));
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
