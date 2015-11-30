/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "contextpanewidgetrectangle.h"
#include "ui_contextpanewidgetrectangle.h"
#include "contextpanewidget.h"
#include "customcolordialog.h"
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsutils.h>
#include <QDebug>

namespace QmlEditorWidgets {

ContextPaneWidgetRectangle::ContextPaneWidgetRectangle(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPaneWidgetRectangle),
    m_gradientLineDoubleClicked(false),
    m_gradientTimer(-1),
    m_enableGradientEditing(true)
{
    ui->setupUi(this);

    ui->colorColorButton->setShowArrow(false);
    ui->borderColorButton->setShowArrow(false);

    connect(ui->colorColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneWidgetRectangle::onColorButtonToggled);
    connect(ui->borderColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneWidgetRectangle::onBorderColorButtonToggled);

    connect(ui->colorSolid, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onColorSolidClicked);
    connect(ui->borderSolid, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onBorderSolidClicked);

    connect(ui->colorNone, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onColorNoneClicked);
    connect(ui->borderNone, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onBorderNoneClicked);

    connect(ui->colorGradient, &QToolButton::clicked, this, &ContextPaneWidgetRectangle::onGradientClicked);

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::accepted,
            this, &ContextPaneWidgetRectangle::onColorDialogApplied);
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::rejected,
            this, &ContextPaneWidgetRectangle::onColorDialogCancled);

    connect(ui->gradientLine, &QmlEditorWidgets::GradientLine::openColorDialog,
            this, &ContextPaneWidgetRectangle::onGradientLineDoubleClicked);
    connect(ui->gradientLine, &QmlEditorWidgets::GradientLine::gradientChanged,
            this, &ContextPaneWidgetRectangle::onUpdateGradient);
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
        QString str = propertyReader->readProperty(QLatin1String("color")).toString();
        if (QmlJS::toQColor(str).alpha() == 0)
            m_none = true;
        ui->colorColorButton->setColor(str);

    } else {
        ui->colorColorButton->setColor(QLatin1String("white"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.color"))) {
        ui->borderColorButton->setColor(propertyReader->readProperty(QLatin1String("border.color")).toString());
        m_hasBorder = true;
    } else {
        ui->borderColorButton->setColor(QLatin1String("transparent"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.width")))
        m_hasBorder = true;

    ui->colorSolid->setChecked(true);
    ui->borderNone->setChecked(true);
    ui->borderSolid->setChecked(m_hasBorder);

    if (m_none)
        ui->colorNone->setChecked(true);

    ui->gradientLabel->setEnabled(true);
    ui->gradientLine->setEnabled(true);

    if (m_hasGradient && isGradientEditingEnabled()) {
        bool isBound;
        ui->colorGradient->setChecked(true);
        ui->gradientLine->setGradient(propertyReader->parseGradient(QLatin1String("gradient"), &isBound));
        if (isBound) {
            ui->gradientLabel->setEnabled(false);
            ui->gradientLine->setEnabled(false);
            ui->colorColorButton->setColor(QLatin1String("invalidColor"));
        }
    } else {
        ui->gradientLine->setEnabled(false);
        ui->gradientLabel->setEnabled(false);
        setColor();
    }

    if (m_gradientTimer > 0) {
        killTimer(m_gradientTimer);
        m_gradientTimer = -1;
    }

    ui->colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::enabableGradientEditing(bool b)
{
    m_enableGradientEditing = b;
}

void ContextPaneWidgetRectangle::onBorderColorButtonToggled(bool flag)
{
    if (flag) {
        ui->colorColorButton->setChecked(false);
        m_gradientLineDoubleClicked = false;
    }
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(ui->borderColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->borderColorButton->convertedColor());
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
    parentContextWidget->colorDialog()->setupColor(ui->colorColorButton->convertedColor());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneWidgetRectangle::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (ui->colorColorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color()); //write back color
    if (ui->borderColorButton->isChecked())
        emit  propertyChanged(QLatin1String("border.color"),parentContextWidget->colorDialog()->color()); //write back color
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
        QLinearGradient gradient;
        QGradientStops stops;
        stops.append(QGradientStop(0, ui->colorColorButton->convertedColor()));
        stops.append(QGradientStop(1, Qt::white));
        gradient.setStops(stops);
        ui->gradientLine->setEnabled(true);
        ui->gradientLine->setGradient(gradient);
    }
}

void ContextPaneWidgetRectangle::onColorNoneClicked()
{
    if (ui->colorNone->isChecked()) {
        ui->colorGradient->setEnabled(isGradientEditingEnabled());
        emit removeAndChangeProperty(QLatin1String("gradient"), QLatin1String("color"),
                                     QLatin1String("transparent"), true);
    }
    ui->colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::onColorSolidClicked()
{
    if (ui->colorSolid->isChecked()) {
        ui->gradientLine->setEnabled(false);
        emit removeAndChangeProperty(QLatin1String("gradient"), QLatin1String("color"),
                                     QLatin1String("\"black\""), true);
    }
    ui->colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::onBorderNoneClicked()
{
    if (ui->borderNone->isChecked()) {
        emit removeProperty(QLatin1String("border.color"));
        emit removeProperty(QLatin1String("border.width"));//###
    }
}

void ContextPaneWidgetRectangle::onBorderSolidClicked()
{
    if (ui->borderSolid->isChecked())
        emit propertyChanged(QLatin1String("border.color"), QLatin1String("\"black\""));
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
        QString str = QLatin1String("Gradient {\n");
        foreach (const QGradientStop &stop, gradient.stops()) {
            str += QLatin1String("GradientStop {\n");
            str += QLatin1String("position: ") + QString::number(stop.first, 'f', 2) + QLatin1String(";\n");
            str += QLatin1String("color: ") + QLatin1String("\"") + stop.second.name() + QLatin1String("\";\n");
            str += QLatin1String("}\n");
        }
        str += QLatin1Char('}');
        emit propertyChanged(QLatin1String("gradient"), str);
    }
}

void ContextPaneWidgetRectangle::setColor()
{
    QLinearGradient gradient;
    QGradientStops stops;
    QColor color = ui->colorColorButton->convertedColor();
    stops.append(QGradientStop(0, color));
    stops.append(QGradientStop(1, color));
    gradient.setStops(stops);
    ui->gradientLine->setGradient(gradient);
}

} //QmlDesigner
