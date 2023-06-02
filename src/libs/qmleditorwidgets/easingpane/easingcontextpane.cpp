// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "easingcontextpane.h"

#include "easinggraph.h"
#include "../qmleditorwidgetstr.h"

#include <qmljs/qmljspropertyreader.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSequentialAnimationGroup>

namespace QmlEditorWidgets {

class PixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
public:
    PixmapItem(const QPixmap &pix) : QGraphicsPixmapItem(pix) { }
};

class EasingSimulation : public QObject
{
    Q_OBJECT
public:
    QGraphicsView *m_g;
    EasingSimulation(QObject *parent, QGraphicsView *v):QObject(parent) {
        m_qtLogo = new PixmapItem(QPixmap(":/qmleditorwidgets/qt_logo.png"));
        m_scene.addItem(m_qtLogo);
        m_scene.setSceneRect(0,0,v->viewport()->width(),m_qtLogo->boundingRect().height());
        m_qtLogo->hide();
        m_sequential = nullptr;
        m_g = v;
        m_g->setScene(&m_scene);
    }

    ~EasingSimulation() { delete m_qtLogo; }

    QGraphicsScene *scene() { return &m_scene; }
    void show() { m_qtLogo->show(); }
    void hide() { m_qtLogo->hide(); }
    void reset() {
        m_qtLogo->setPos(0,0);
        m_scene.setSceneRect(0,0,m_g->viewport()->width(),m_qtLogo->boundingRect().height());
    }
    void stop() {
        if (m_sequential) {
            m_sequential->stop();
            reset();
            emit finished();
        }
    }
    bool running() {
        if (m_sequential)
            return m_sequential->state()==QAbstractAnimation::Running;
        else
            return false;
    }
    void updateCurve(QEasingCurve newCurve, int newDuration) {
        if (running()) {
            m_sequential->pause();
            static_cast<QPropertyAnimation *>(m_sequential->animationAt(1))->setEasingCurve(newCurve);
            static_cast<QPropertyAnimation *>(m_sequential->animationAt(1))->setDuration(newDuration);
            m_sequential->resume();
        }
    }
    void animate(int duration, const QEasingCurve &curve) {
        reset();
        m_sequential = new QSequentialAnimationGroup;
        connect(m_sequential,&QAbstractAnimation::finished,this,&EasingSimulation::finished);
        m_sequential->addPause(150);
        QPropertyAnimation *m_anim = new QPropertyAnimation (m_qtLogo, "pos");
        m_anim->setStartValue(QPointF(0, 0));
        m_anim->setEndValue(QPointF(m_scene.sceneRect().width() - m_qtLogo->boundingRect().width(), 0));
        m_anim->setDuration(duration);
        m_anim->setEasingCurve(curve);
        m_sequential->addAnimation(m_anim);
        m_sequential->addPause(300);
        m_sequential->start();
    }

signals:
    void finished();

private:
    PixmapItem *m_qtLogo;
    QGraphicsScene m_scene;
    QSequentialAnimationGroup *m_sequential;
};


EasingContextPane::EasingContextPane(QWidget *parent)
    : QWidget(parent)
{
    m_graphicsView = new QGraphicsView;
    m_graphicsView->setFixedSize({290, 90});
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_graphicsView->setInteractive(false);

    m_playButton = new QPushButton;
    m_playButton->setIcon(Utils::Icons::RUN_SMALL.icon());
    m_playButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_playButton->setToolTip(Tr::tr("Play simulation."));

    m_easingShapeComboBox = new QComboBox;
    m_easingShapeComboBox->addItems(
        {"Linear", "Quad", "Cubic", "Quart", "Quint", "Sine", "Expo", "Circ", "Elastic", "Back", "Bounce"});
    m_easingShapeComboBox->setToolTip(Tr::tr("Type of easing curve."));
    m_easingExtremesComboBox = new QComboBox;
    m_easingExtremesComboBox->addItems(
        {"In", "Out", "InOut", "OutIn"});
    m_easingExtremesComboBox->setToolTip(Tr::tr("Acceleration or deceleration of easing curve."));

    m_durationSpinBox = new QSpinBox;
    m_durationSpinBox->setKeyboardTracking(false);
    m_durationSpinBox->setMaximum(9999999);
    m_durationSpinBox->setMinimum(-1);
    m_durationSpinBox->setSingleStep(10);
    m_durationSpinBox->setSuffix(Tr::tr(" ms"));
    m_durationSpinBox->setToolTip(Tr::tr("Duration of animation."));

    m_amplitudeSpinBox = new QDoubleSpinBox;
    m_amplitudeSpinBox->setSingleStep(0.05);
    m_amplitudeSpinBox->setToolTip(Tr::tr("Amplitude of elastic and bounce easing curves."));
    m_periodSpinBox = new QDoubleSpinBox;
    m_periodSpinBox->setSingleStep(0.01);
    m_periodSpinBox->setToolTip(Tr::tr("Easing period of an elastic curve."));
    m_overshootSpinBox = new QDoubleSpinBox;
    m_overshootSpinBox->setSingleStep(0.05);
    m_overshootSpinBox->setToolTip(Tr::tr("Easing overshoot for a back curve."));
    for (auto spinBox : {m_amplitudeSpinBox, m_periodSpinBox, m_overshootSpinBox}) {
        spinBox->setDecimals(3);
        spinBox->setKeyboardTracking(false);
        spinBox->setMaximum(999999.9);
    }

    using namespace Layouting;
    Column {
        Row { m_graphicsView, m_playButton, },
        Row {
            Form {
                Tr::tr("Easing"), m_easingShapeComboBox, br,
                Tr::tr("Duration"), m_durationSpinBox, br,
                Tr::tr("Period"), m_periodSpinBox, br,
            },
            Form {
                Tr::tr("Subtype"), m_easingExtremesComboBox, br,
                Tr::tr("Amplitude"), m_amplitudeSpinBox, br,
                Tr::tr("Overshoot"), m_overshootSpinBox, br,
            },
        },
    }.attachTo(this);

    m_simulation = new EasingSimulation(this, m_graphicsView);
    m_easingGraph = new EasingGraph(this);
    m_easingGraph->raise();
    setLinear();

    setGraphDisplayMode(GraphMode);

    connect(m_simulation, &EasingSimulation::finished, this, &EasingContextPane::switchToGraph);
    connect(m_playButton, &QPushButton::clicked, this, &EasingContextPane::playClicked);
    connect(m_overshootSpinBox, &QDoubleSpinBox::valueChanged,
            this, &EasingContextPane::overshootChanged);
    connect(m_periodSpinBox, &QDoubleSpinBox::valueChanged,
            this, &EasingContextPane::periodChanged);
    connect(m_amplitudeSpinBox, &QDoubleSpinBox::valueChanged,
            this, &EasingContextPane::amplitudeChanged);
    connect(m_easingExtremesComboBox, &QComboBox::currentIndexChanged,
            this, &EasingContextPane::easingExtremesChanged);
    connect(m_easingShapeComboBox, &QComboBox::currentIndexChanged,
            this, &EasingContextPane::easingShapeChanged);
    connect(m_durationSpinBox, &QSpinBox::valueChanged,
            this, &EasingContextPane::durationChanged);
}

bool EasingContextPane::acceptsType(const QStringList &types)
{
    return types.contains(QLatin1String("NumberAnimation")) ||
            types.contains(QLatin1String("PropertyAnimation")) ||
            types.contains(QLatin1String("ColorAnimation")) ||
            types.contains(QLatin1String("RotationAnimation"));
}

void EasingContextPane::setProperties(QmlJS::PropertyReader *propertyReader)
{
    m_easingGraph->setGeometry(m_graphicsView->geometry().adjusted(2,2,-2,-2));
    QString newEasingType = QLatin1String("Linear");
    if (propertyReader->hasProperty(QLatin1String("easing.type"))) {
        newEasingType = propertyReader->readProperty(QLatin1String("easing.type")).toString();
        if (newEasingType.contains(QLatin1Char('.')))
            newEasingType = newEasingType.right(newEasingType.length() - newEasingType.indexOf(QLatin1Char('.')) - 1);
    }

    m_easingGraph->setEasingName(newEasingType);
    m_easingShapeComboBox->setCurrentIndex(m_easingShapeComboBox->findText(m_easingGraph->easingShape()));
    m_easingExtremesComboBox->setCurrentIndex(m_easingExtremesComboBox->findText(m_easingGraph->easingExtremes()));


    if (propertyReader->hasProperty(QLatin1String("easing.period"))) {
        qreal period = propertyReader->readProperty(QLatin1String("easing.period")).toDouble();
        if (period < m_periodSpinBox->minimum() || period > m_periodSpinBox->maximum())
            m_periodSpinBox->setValue(m_periodSpinBox->minimum());
        else
            m_periodSpinBox->setValue(period);
    }
    else
        m_periodSpinBox->setValue(0.3);

    if (propertyReader->hasProperty(QLatin1String("easing.amplitude"))) {
        qreal amplitude = propertyReader->readProperty(QLatin1String("easing.amplitude")).toDouble();
        if (amplitude < m_amplitudeSpinBox->minimum() || amplitude > m_amplitudeSpinBox->maximum())
            m_amplitudeSpinBox->setValue(m_amplitudeSpinBox->minimum());
        else
            m_amplitudeSpinBox->setValue(amplitude);
    }
    else
        m_amplitudeSpinBox->setValue(1.0);

    if (propertyReader->hasProperty(QLatin1String("easing.overshoot"))) {
        qreal overshoot = propertyReader->readProperty(QLatin1String("easing.overshoot")).toDouble();
        if (overshoot < m_overshootSpinBox->minimum() || overshoot > m_overshootSpinBox->maximum())
            m_overshootSpinBox->setValue(m_overshootSpinBox->minimum());
        else
            m_overshootSpinBox->setValue(overshoot);
    }
    else
        m_overshootSpinBox->setValue(1.70158);

    if (propertyReader->hasProperty(QLatin1String("duration"))) {
        qreal duration = propertyReader->readProperty(QLatin1String("duration")).toInt();
        if (duration < m_durationSpinBox->minimum() || duration > m_durationSpinBox->maximum())
            m_durationSpinBox->setValue(m_durationSpinBox->minimum());
        else
            m_durationSpinBox->setValue(duration);
    }
    else
        m_durationSpinBox->setValue(250);
}

void EasingContextPane::setGraphDisplayMode(GraphDisplayMode newMode)
{
    m_displayMode = newMode;
    switch (newMode) {
    case GraphMode: {
            m_simulation->hide();
            m_easingGraph->show();
            break;
        }
    case SimulationMode: {
            m_simulation->show();
            m_easingGraph->hide();
            break;
        }
    default: break;
    }

}

void EasingContextPane::startAnimation()
{
    if (m_simulation->running()) {
        m_simulation->stop();
    } else {
        m_simulation->animate(m_durationSpinBox->value(), m_easingGraph->easingCurve());
        m_playButton->setIcon(Utils::Icons::STOP_SMALL.icon());
    }

}

void EasingContextPane::switchToGraph()
{
    m_playButton->setIcon(Utils::Icons::RUN_SMALL.icon());
    setGraphDisplayMode(GraphMode);
}

void EasingContextPane::setOthers()
{
    m_easingExtremesComboBox->setEnabled(true);
    m_amplitudeSpinBox->setEnabled(false);
    m_overshootSpinBox->setEnabled(false);
    m_overshootSpinBox->setEnabled(false);
    m_periodSpinBox->setEnabled(false);
}

void EasingContextPane::setLinear()
{
    m_easingExtremesComboBox->setEnabled(false);
    m_amplitudeSpinBox->setEnabled(false);
    m_overshootSpinBox->setEnabled(false);
    m_periodSpinBox->setEnabled(false);
}

void EasingContextPane::setBack()
{
    m_easingExtremesComboBox->setEnabled(true);
    m_amplitudeSpinBox->setEnabled(false);
    m_overshootSpinBox->setEnabled(true);
    m_periodSpinBox->setEnabled(false);
}

void EasingContextPane::setElastic()
{
    m_easingExtremesComboBox->setEnabled(true);
    m_amplitudeSpinBox->setEnabled(true);
    m_overshootSpinBox->setEnabled(false);
    m_periodSpinBox->setEnabled(true);
}

void EasingContextPane::setBounce()
{
    m_easingExtremesComboBox->setEnabled(true);
    m_amplitudeSpinBox->setEnabled(true);
    m_overshootSpinBox->setEnabled(false);
    m_periodSpinBox->setEnabled(false);
}

} //QmlDesigner

void QmlEditorWidgets::EasingContextPane::durationChanged(int newValue)
{
    m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
    emit propertyChanged(QLatin1String("duration"), newValue);
}

void QmlEditorWidgets::EasingContextPane::easingShapeChanged(int newIndex)
{
    QTC_ASSERT(newIndex >= 0, return);
    const QString newShape = m_easingShapeComboBox->itemText(newIndex);
    if (newShape==QLatin1String("Linear"))
        setLinear();
    else if (newShape==QLatin1String("Bounce"))
        setBounce();
    else if (newShape==QLatin1String("Elastic"))
        setElastic();
    else if (newShape==QLatin1String("Back"))
        setBack();
    else
        setOthers();

    if (m_easingGraph->easingShape() != newShape) {
        m_easingGraph->setEasingShape(newShape);
        // reload easing parameters
        m_easingGraph->setAmplitude(m_amplitudeSpinBox->value());
        m_easingGraph->setPeriod(m_periodSpinBox->value());
        m_easingGraph->setOvershoot(m_overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlEditorWidgets::EasingContextPane::easingExtremesChanged(int newIndex)
{
    QTC_ASSERT(newIndex >= 0, return);
    const QString newExtremes = m_easingExtremesComboBox->itemText(newIndex);
    if (m_easingGraph->easingExtremes() != newExtremes) {
        m_easingGraph->setEasingExtremes(newExtremes);
        m_easingGraph->setAmplitude(m_amplitudeSpinBox->value());
        m_easingGraph->setPeriod(m_periodSpinBox->value());
        m_easingGraph->setOvershoot(m_overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlEditorWidgets::EasingContextPane::amplitudeChanged(double newAmplitude)
{
    if ((newAmplitude != m_easingGraph->amplitude()) &&
        (m_easingGraph->easingShape()==QLatin1String("Bounce")
         || m_easingGraph->easingShape()==QLatin1String("Elastic"))) {
        m_easingGraph->setAmplitude(newAmplitude);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.amplitude"), newAmplitude);
    }
}

void QmlEditorWidgets::EasingContextPane::periodChanged(double newPeriod)
{
    if ((newPeriod != m_easingGraph->period()) && (m_easingGraph->easingShape()==QLatin1String("Elastic"))) {
        m_easingGraph->setPeriod(newPeriod);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.period"), newPeriod);
    }

}

void QmlEditorWidgets::EasingContextPane::overshootChanged(double newOvershoot)
{
    if ((newOvershoot != m_easingGraph->overshoot()) && (m_easingGraph->easingShape()==QLatin1String("Back"))) {
        m_easingGraph->setOvershoot(newOvershoot);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),m_durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.overshoot"), newOvershoot);
    }
}

void QmlEditorWidgets::EasingContextPane::playClicked()
{
    setGraphDisplayMode(SimulationMode);
    startAnimation();
}

#include "easingcontextpane.moc"
