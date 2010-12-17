/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef EASINGCONTEXTPANE_H
#define EASINGCONTEXTPANE_H

#include <QWidget>
#include <QVariant>

#include "easinggraph.h"


QT_BEGIN_NAMESPACE
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
