#include "easingcontextpane.h"
#include "ui_easingcontextpane.h"
#include <qmljs/qmljspropertyreader.h>

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

#include <qdebug.h>


namespace QmlDesigner {

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
    EasingSimulation(QObject *parent=0, int length=210):QObject(parent) {
        m_qtLogo = new PixmapItem(QPixmap(":/qt_logo.png"));
        m_scene.addItem(m_qtLogo);
        m_scene.setSceneRect(0,0,length,m_qtLogo->boundingRect().height());
        m_qtLogo->hide();
        m_sequential = 0;
    }

    ~EasingSimulation() { delete m_qtLogo; }

    QGraphicsScene *scene() { return &m_scene; }
    void show() { m_qtLogo->show(); }
    void hide() { m_qtLogo->hide(); }
    void reset() { m_qtLogo->setPos(0,0); }
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
    void animate(int duration, QEasingCurve curve) {
        reset();
        m_sequential = new QSequentialAnimationGroup;
        connect(m_sequential,SIGNAL(finished()),this,SIGNAL(finished()));
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


EasingContextPane::EasingContextPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EasingContextPane)
{
    ui->setupUi(this);

    m_simulation = new EasingSimulation(this, 210);
    ui->graphicsView->setScene(m_simulation->scene());

    m_easingGraph = new EasingGraph(this);
    m_easingGraph->raise();
    setLinear();

    ui->playButton->setIcon(QIcon(":/playicon.png"));



    setGraphDisplayMode(GraphMode);

    connect(m_simulation,SIGNAL(finished()),this,SLOT(switchToGraph()));
}

EasingContextPane::~EasingContextPane()
{
    delete ui;
}


bool EasingContextPane::acceptsType(const QString &typeName)
{
    return typeName.contains("NumberAnimation") ||
            typeName.contains("PropertyAnimation") ||
            typeName.contains("ColorAnimation") ||
            typeName.contains("RotationAnimation");
}

void EasingContextPane::setProperties(QmlJS::PropertyReader *propertyReader)
{
    m_easingGraph->setGeometry(ui->graphicsView->geometry().adjusted(2,2,-2,-2));
    QString newEasingType = QString("Linear");
    if (propertyReader->hasProperty(QLatin1String("easing.type"))) {
        newEasingType = propertyReader->readProperty(QLatin1String("easing.type")).toString();
        if (newEasingType.contains("."))
            newEasingType = newEasingType.right(newEasingType.length() - newEasingType.indexOf(".") - 1);
    }

    m_easingGraph->setEasingName(newEasingType);
    ui->easingShapeComboBox->setCurrentIndex(ui->easingShapeComboBox->findText(m_easingGraph->easingShape()));
    ui->easingExtremesComboBox->setCurrentIndex(ui->easingExtremesComboBox->findText(m_easingGraph->easingExtremes()));


    if (propertyReader->hasProperty(QLatin1String("easing.period"))) {
        qreal period = propertyReader->readProperty(QLatin1String("easing.period")).toDouble();
        if (period < ui->periodSpinBox->minimum() || period > ui->periodSpinBox->maximum())
            ui->periodSpinBox->setValue(ui->periodSpinBox->minimum());
        else
            ui->periodSpinBox->setValue(period);
    }
    else
        ui->periodSpinBox->setValue(0.3);

    if (propertyReader->hasProperty(QLatin1String("easing.amplitude"))) {
        qreal amplitude = propertyReader->readProperty(QLatin1String("easing.amplitude")).toDouble();
        if (amplitude < ui->amplitudeSpinBox->minimum() || amplitude > ui->amplitudeSpinBox->maximum())
            ui->amplitudeSpinBox->setValue(ui->amplitudeSpinBox->minimum());
        else
            ui->amplitudeSpinBox->setValue(amplitude);
    }
    else
        ui->amplitudeSpinBox->setValue(1.0);

    if (propertyReader->hasProperty(QLatin1String("easing.overshoot"))) {
        qreal overshoot = propertyReader->readProperty(QLatin1String("easing.overshoot")).toDouble();
        if (overshoot < ui->overshootSpinBox->minimum() || overshoot > ui->overshootSpinBox->maximum())
            ui->overshootSpinBox->setValue(ui->overshootSpinBox->minimum());
        else
            ui->overshootSpinBox->setValue(overshoot);
    }
    else
        ui->overshootSpinBox->setValue(1.70158);

    if (propertyReader->hasProperty(QLatin1String("duration"))) {
        qreal duration = propertyReader->readProperty(QLatin1String("duration")).toInt();
        if (duration < ui->durationSpinBox->minimum() || duration > ui->durationSpinBox->maximum())
            ui->durationSpinBox->setValue(ui->durationSpinBox->minimum());
        else
            ui->durationSpinBox->setValue(duration);
    }
    else
        ui->durationSpinBox->setValue(250);
}

void EasingContextPane::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void EasingContextPane::setGraphDisplayMode(GraphDisplayMode newMode)
{
    m_displayMode = newMode;
    switch (newMode) {
    case GraphMode: {
//            ui->graphSelectButton->setIcon(QIcon(":/simulicon.png"));
            m_simulation->hide();
            m_easingGraph->show();
            break;
        }
    case SimulationMode: {
//            ui->graphSelectButton->setIcon(QIcon(":/graphicon.png"));
            m_simulation->show();
            m_easingGraph->hide();
            break;
        }
    default: break;
    }

}

void EasingContextPane::startAnimation()
{
    if (m_simulation->running())
        m_simulation->stop();
    else {
        m_simulation->animate(ui->durationSpinBox->value(), m_easingGraph->easingCurve());
        ui->playButton->setIcon(QIcon(":/stopicon.png"));
    }

}

void EasingContextPane::switchToGraph()
{
    ui->playButton->setIcon(QIcon(":/playicon.png"));
    setGraphDisplayMode(GraphMode);
}

void EasingContextPane::setOthers()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);

//    ui->durationSpinBox->setSuffix(" ms");
//    ui->durationSpinBox->setGeometry(0,65,90,23);
//    ui->easingExtremesComboBox->setGeometry(90,64,75,25);
//    ui->easingShapeComboBox->setGeometry(165,64,115,25);
//    ui->easingExtremesComboBox->show();
//    ui->amplitudeSpinBox->hide();
//    ui->periodSpinBox->hide();
//    ui->overshootSpinBox->hide();
}

void EasingContextPane::setLinear()
{
    ui->easingExtremesComboBox->setEnabled(false);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);
}

void EasingContextPane::setBack()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(false);
    ui->overshootSpinBox->setEnabled(true);
    ui->periodSpinBox->setEnabled(false);
}

void EasingContextPane::setElastic()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(true);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(true);
}

void EasingContextPane::setBounce()
{
    ui->easingExtremesComboBox->setEnabled(true);
    ui->amplitudeSpinBox->setEnabled(true);
    ui->overshootSpinBox->setEnabled(false);
    ui->periodSpinBox->setEnabled(false);
}

} //QmlDesigner

void QmlDesigner::EasingContextPane::on_durationSpinBox_valueChanged(int newValue)
{
    m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
    emit propertyChanged(QLatin1String("duration"), newValue);
}

void QmlDesigner::EasingContextPane::on_easingShapeComboBox_currentIndexChanged(QString newShape)
{
    if (newShape=="Linear")
        setLinear();
    else if (newShape=="Bounce")
        setBounce();
    else if (newShape=="Elastic")
        setElastic();
    else if (newShape=="Back")
        setBack();
    else
        setOthers();

    if (m_easingGraph->easingShape() != newShape) {
        m_easingGraph->setEasingShape(newShape);
        // reload easing parameters
        m_easingGraph->setAmplitude(ui->amplitudeSpinBox->value());
        m_easingGraph->setPeriod(ui->periodSpinBox->value());
        m_easingGraph->setOvershoot(ui->overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlDesigner::EasingContextPane::on_easingExtremesComboBox_currentIndexChanged(QString newExtremes)
{
    if (m_easingGraph->easingExtremes() != newExtremes) {
        m_easingGraph->setEasingExtremes(newExtremes);
        m_easingGraph->setAmplitude(ui->amplitudeSpinBox->value());
        m_easingGraph->setPeriod(ui->periodSpinBox->value());
        m_easingGraph->setOvershoot(ui->overshootSpinBox->value());
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.type"), QVariant(QLatin1String("Easing.")+m_easingGraph->easingName()));
    }
}

void QmlDesigner::EasingContextPane::on_amplitudeSpinBox_valueChanged(double newAmplitude)
{
    if ((newAmplitude != m_easingGraph->amplitude()) &&
        (m_easingGraph->easingShape()=="Bounce" || m_easingGraph->easingShape()=="Elastic")) {
        m_easingGraph->setAmplitude(newAmplitude);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.amplitude"), newAmplitude);
    }
}

void QmlDesigner::EasingContextPane::on_periodSpinBox_valueChanged(double newPeriod)
{
    if ((newPeriod != m_easingGraph->period()) && (m_easingGraph->easingShape()=="Elastic")) {
        m_easingGraph->setPeriod(newPeriod);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.period"), newPeriod);
    }

}

void QmlDesigner::EasingContextPane::on_overshootSpinBox_valueChanged(double newOvershoot)
{
    if ((newOvershoot != m_easingGraph->overshoot()) && (m_easingGraph->easingShape()=="Back")) {
        m_easingGraph->setOvershoot(newOvershoot);
        m_simulation->updateCurve(m_easingGraph->easingCurve(),ui->durationSpinBox->value());
        emit propertyChanged(QLatin1String("easing.overshoot"), newOvershoot);
    }
}

void QmlDesigner::EasingContextPane::on_graphSelectButton_clicked()
{
    setGraphDisplayMode(m_displayMode==GraphMode?SimulationMode:GraphMode);
}

void QmlDesigner::EasingContextPane::on_playButton_clicked()
{
    setGraphDisplayMode(SimulationMode);
    startAnimation();
}

#include "easingcontextpane.moc"
