// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QChart>
#include <QList>
#include <QPaintEvent>
#include <QPointF>
#include <QRectF>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QChartView;
class QLineSeries;
class QValueAxis;
QT_END_NAMESPACE

namespace AppStatisticsMonitor::Internal {

class AppStatisticsMonitorChart : public QChart
{
public:
    AppStatisticsMonitorChart(
        const QString &name, QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});

    void addNewPoint(const QPointF &point);
    void loadNewProcessData(const QList<double> &data);
    double lastPointX() const;
    void clear();
    QChartView *chartView();

private:
    QLineSeries *m_series;
    QStringList m_titles;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QPointF m_point;

    QChartView *m_chartView;
    QString m_name;
};

class Chart : public QWidget
{
public:
    Chart(const QString &name, QWidget *parent = nullptr);

    void addNewPoint(const QPointF &point);
    void loadNewProcessData(const QList<double> &data);
    double lastPointX() const;

    void clear();

private:
    void paintEvent(QPaintEvent *event) override;
    void addPoint();
    QRectF calculateDataRange() const;
    void updateScalingFactors(const QRectF &dataRange);

private:
    QList<QPointF> m_points;
    QString m_name;

    double m_xScale = 1;
    double m_yScale = 1;
    double m_xGridStep = 1;
    double m_yGridStep = 1;
};

} // namespace AppStatisticsMonitor::Internal
