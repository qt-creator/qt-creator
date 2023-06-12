// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QGraphicsView;
class QVariant;
class EasingGraph;

QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class EasingSimulation;

class EasingContextPane : public QWidget
{
    Q_OBJECT

    enum GraphDisplayMode { GraphMode, SimulationMode };
public:
    explicit EasingContextPane(QWidget *parent = nullptr);

    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setGraphDisplayMode(GraphDisplayMode newMode);
    void startAnimation();

    bool acceptsType(const QStringList &);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool);

protected:
    void setOthers();
    void setLinear();
    void setBack();
    void setElastic();
    void setBounce();

private:
    QGraphicsView *m_graphicsView;
    QPushButton *m_playButton;
    QComboBox *m_easingShapeComboBox;
    QSpinBox *m_durationSpinBox;
    QDoubleSpinBox *m_periodSpinBox;
    QComboBox *m_easingExtremesComboBox;
    QDoubleSpinBox *m_amplitudeSpinBox;
    QDoubleSpinBox *m_overshootSpinBox;

    GraphDisplayMode m_displayMode;
    EasingGraph *m_easingGraph;
    EasingSimulation *m_simulation;

private slots:
    void playClicked();
    void overshootChanged(double);
    void periodChanged(double);
    void amplitudeChanged(double);
    void easingExtremesChanged(int);
    void easingShapeChanged(int);
    void durationChanged(int);

    void switchToGraph();
};

}
