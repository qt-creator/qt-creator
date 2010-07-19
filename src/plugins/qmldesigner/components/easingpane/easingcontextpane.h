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

namespace QmlDesigner {
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

    bool acceptsType(const QString &);

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
