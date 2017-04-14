/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include "easinggraph.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVariant;
namespace Ui { class EasingContextPane; }
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {
class EasingSimulation;

class EasingContextPane : public QWidget
{
    Q_OBJECT

    enum GraphDisplayMode { GraphMode, SimulationMode };
public:
    explicit EasingContextPane(QWidget *parent = 0);
    ~EasingContextPane();

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
    Ui::EasingContextPane *ui;
    GraphDisplayMode m_displayMode;
    EasingGraph *m_easingGraph;
    EasingSimulation *m_simulation;

private slots:
    void on_playButton_clicked();
    void on_overshootSpinBox_valueChanged(double);
    void on_periodSpinBox_valueChanged(double);
    void on_amplitudeSpinBox_valueChanged(double);
    void on_easingExtremesComboBox_currentIndexChanged(const QString &);
    void on_easingShapeComboBox_currentIndexChanged(const QString &);
    void on_durationSpinBox_valueChanged(int);

    void switchToGraph();
};

}
