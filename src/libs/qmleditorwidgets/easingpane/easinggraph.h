// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>
#include <QEasingCurve>
#include <QHash>

QT_BEGIN_NAMESPACE


class EasingGraph: public QWidget
{
    Q_OBJECT

    Q_PROPERTY (QString easingShape READ easingShape WRITE setEasingShape NOTIFY easingShapeChanged)
    Q_PROPERTY (QString easingExtremes READ easingExtremes WRITE setEasingExtremes NOTIFY easingExtremesChanged)
    Q_PROPERTY (QString easingName READ easingName WRITE setEasingName NOTIFY easingNameChanged)
    Q_PROPERTY (qreal overshoot READ overshoot WRITE setOvershoot NOTIFY overshootChanged)
    Q_PROPERTY (qreal amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
    Q_PROPERTY (qreal period READ period WRITE setPeriod NOTIFY periodChanged)
    Q_PROPERTY (qreal duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY (QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY (QColor zeroColor READ zeroColor WRITE setZeroColor NOTIFY zeroColorChanged)

public:
    EasingGraph(QWidget *parent=nullptr);
    ~EasingGraph() override;

    QEasingCurve::Type easingType() const;
    QEasingCurve easingCurve() const;

    QString easingShape() const;
    void setEasingShape(const QString &newShape);
    QString easingExtremes() const;
    void setEasingExtremes(const QString &newExtremes);
    QString easingName() const;
    void setEasingName(const QString &newName);
    qreal overshoot() const;
    void setOvershoot(qreal newOvershoot);
    qreal amplitude() const;
    void setAmplitude(qreal newAmplitude);
    qreal period() const;
    void setPeriod(qreal newPeriod);
    qreal duration() const;
    void setDuration(qreal newDuration);

    QColor color() const;
    void setColor(const QColor &newColor);
    QColor zeroColor() const;
    void setZeroColor(const QColor &newColor);

    QRectF boundingRect() const;
    //void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    void paintEvent(QPaintEvent *) override;

signals:
    void easingShapeChanged();
    void easingExtremesChanged();
    void easingNameChanged();
    void overshootChanged();
    void amplitudeChanged();
    void durationChanged();
    void periodChanged();
    void colorChanged();
    void zeroColorChanged();

private:
    QColor m_color;
    QColor m_zeroColor; // the color for the "zero" and "one" lines
    qreal m_duration;

    QString m_easingExtremes;

    QEasingCurve m_curveFunction;
    QHash <QString,QEasingCurve::Type> m_availableNames;
};

QT_END_NAMESPACE

//QML_DECLARE_TYPE(EasingGraph)
