#include "contextpanewidgetrectangle.h"
#include "ui_contextpanewidgetrectangle.h"
#include "contextpanewidget.h"
#include <qmljs/qmljspropertyreader.h>
#include <QDebug>

namespace QmlDesigner {

ContextPaneWidgetRectangle::ContextPaneWidgetRectangle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPaneWidgetRectangle),
    m_gradientLineDoubleClicked(false),
    m_gradientTimer(-1)
{
    ui->setupUi(this);

    ui->colorColorButton->setShowArrow(false);
    ui->borderColorButton->setShowArrow(false);

    connect(ui->colorColorButton, SIGNAL(toggled(bool)), this, SLOT(onColorButtonToggled(bool)));
    connect(ui->borderColorButton, SIGNAL(toggled(bool)), this, SLOT(onBorderColorButtonToggled(bool)));

    connect(ui->colorSolid, SIGNAL(clicked()), this, SLOT(onColorSolidClicked()));
    connect(ui->borderSolid, SIGNAL(clicked()), this, SLOT(onBorderSolidClicked()));

    connect(ui->colorNone, SIGNAL(clicked()), this, SLOT(onColorNoneClicked()));
    connect(ui->borderNone, SIGNAL(clicked()), this, SLOT(onBorderNoneClicked()));

    connect(ui->colorGradient, SIGNAL(clicked()), this, SLOT(onGradientClicked()));

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), SIGNAL(accepted(QColor)), this, SLOT(onColorDialogApplied(QColor)));
    connect(parentContextWidget->colorDialog(), SIGNAL(rejected()), this, SLOT(onColorDialogCancled()));

    connect(ui->gradientLine, SIGNAL(openColorDialog(QPoint)), this, SLOT(onGradientLineDoubleClicked(QPoint)));
    connect(ui->gradientLine, SIGNAL(gradientChanged()), this, SLOT(onUpdateGradient()));
}

ContextPaneWidgetRectangle::~ContextPaneWidgetRectangle()
{
    delete ui;
}

void ContextPaneWidgetRectangle::setProperties(QmlJS::PropertyReader *propertyReader)
{    
    m_hasGradient = propertyReader->hasProperty(QLatin1String("gradient"));
    m_none = false;
    m_hasBorder = false;

    if (propertyReader->hasProperty(QLatin1String("color"))) {
        QString str = propertyReader->readProperty("color").toString();
        if (QColor(str).alpha() == 0)
            m_none = true;
        ui->colorColorButton->setColor(str);

    } else {
        ui->colorColorButton->setColor(QLatin1String("black"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.color"))) {
        ui->borderColorButton->setColor(propertyReader->readProperty("border.color").toString());
        m_hasBorder = true;
    } else {
        ui->borderColorButton->setColor(QLatin1String("black"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.width")))
        m_hasBorder = true;

    ui->colorSolid->setChecked(true);
    ui->borderNone->setChecked(true);
    ui->borderSolid->setChecked(m_hasBorder);

    if (m_none)
        ui->colorNone->setChecked(true);

    if (m_hasGradient) {
        ui->colorGradient->setChecked(true);
        ui->gradientLine->setEnabled(true);
        ui->gradientLabel->setEnabled(true);
        ui->gradientLine->setGradient(propertyReader->parseGradient("gradient"));
    } else {
        ui->gradientLine->setEnabled(false);
        ui->gradientLabel->setEnabled(false);
    }

    if (m_gradientTimer > 0) {
        killTimer(m_gradientTimer);
        m_gradientTimer = -1;
    }

}

void ContextPaneWidgetRectangle::onBorderColorButtonToggled(bool flag)
{
    if (flag) {
        ui->colorColorButton->setChecked(false);
        m_gradientLineDoubleClicked = false;
    }
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(ui->borderColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->borderColorButton->color());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneWidgetRectangle::onColorButtonToggled(bool flag )
{
    if (flag) {
        ui->borderColorButton->setChecked(false);
        m_gradientLineDoubleClicked = false;
    }
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(ui->colorColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->colorColorButton->color());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneWidgetRectangle::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (ui->colorColorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color());; //write back color
    if (ui->borderColorButton->isChecked())
        emit  propertyChanged(QLatin1String("border.color"),parentContextWidget->colorDialog()->color());; //write back color
    if (m_gradientLineDoubleClicked)
        ui->gradientLine->setActiveColor(parentContextWidget->colorDialog()->color());
    ui->colorColorButton->setChecked(false);
    ui->borderColorButton->setChecked(false);
    m_gradientLineDoubleClicked = false;
}

void ContextPaneWidgetRectangle::onColorDialogCancled()
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    ui->colorColorButton->setChecked(false);
    ui->borderColorButton->setChecked(false);
    m_gradientLineDoubleClicked = false;
}

void ContextPaneWidgetRectangle::onGradientClicked()
{
    if (ui->colorGradient->isChecked()) {
        m_hasGradient = true;
        emit removeAndChangeProperty("color", "gradient", " Gradient { }", false);
        ui->gradientLine->setEnabled(true);
        ui->gradientLabel->setEnabled(true);
    }
}

void ContextPaneWidgetRectangle::onColorNoneClicked()
{
    if (ui->colorNone->isChecked()) {        
        emit removeAndChangeProperty("gradient", "color", "transparent", true);
    }
}

void ContextPaneWidgetRectangle::onColorSolidClicked()
{
    if (ui->colorSolid->isChecked()) {
        emit removeAndChangeProperty("gradient", "color", "\"black\"", true);
    }
}

void ContextPaneWidgetRectangle::onBorderNoneClicked()
{
    if (ui->borderNone->isChecked()) {
        emit removeProperty("border.color");
        emit removeProperty("border.width");//###
    }
}

void ContextPaneWidgetRectangle::onBorderSolidClicked()
{
    if (ui->borderSolid->isChecked()) {
        emit propertyChanged("border.color", "\"black\"");
    }
}

void ContextPaneWidgetRectangle::onGradientLineDoubleClicked(const QPoint &p)
{
    m_gradientLineDoubleClicked = true;
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint pos = mapToGlobal(p);
    parentContextWidget->colorDialog()->setupColor(ui->gradientLine->activeColor());
    pos = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(pos);
    parentContextWidget->onShowColorDialog(true, pos);



}

void ContextPaneWidgetRectangle::onUpdateGradient()
{
    if (m_gradientTimer > 0)
        killTimer(m_gradientTimer);
    m_gradientTimer = startTimer(100);
}

void ContextPaneWidgetRectangle::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_gradientTimer) {
        killTimer(m_gradientTimer);
        m_gradientTimer = -1;

        QLinearGradient gradient = ui->gradientLine->gradient();
        QString str = "Gradient {\n";
        foreach (const QGradientStop &stop, gradient.stops()) {
            str += QLatin1String("GradientStop {\n");
            str += QLatin1String("position: ") + QString::number(stop.first, 'f', 2) + QLatin1String(";\n");
            str += QLatin1String("color: ") + QLatin1String("\"") + stop.second.name() + QLatin1String("\";\n");
            str += QLatin1String("}\n");
        }
        str += QLatin1String("}");
        emit propertyChanged("gradient", str);
    }
}

void ContextPaneWidgetRectangle::changeEvent(QEvent *e)
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

} //QmlDesigner
