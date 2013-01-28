/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EASINGCONTEXTPANE_H
#define EASINGCONTEXTPANE_H

#include "easinggraph.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVariant;
namespace Ui {
    class EasingContextPane;
}
QT_END_NAMESPACE

namespace QmlJS {
    class PropertyReader;
}

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
    void changeEvent(QEvent *e);

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
    void on_overshootSpinBox_valueChanged(double );
    void on_periodSpinBox_valueChanged(double );
    void on_amplitudeSpinBox_valueChanged(double );
    void on_easingExtremesComboBox_currentIndexChanged(QString );
    void on_easingShapeComboBox_currentIndexChanged(QString );
    void on_durationSpinBox_valueChanged(int );

    void switchToGraph();
};

}
#endif // EASINGCONTEXTPANE_H
