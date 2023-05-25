// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contextpanewidgetrectangle.h"

#include "colorbutton.h"
#include "contextpanewidget.h"
#include "customcolordialog.h"
#include "gradientline.h"
#include "qmleditorwidgetstr.h"

#include <qmljs/qmljspropertyreader.h>
#include <utils/layoutbuilder.h>
#include <qmljs/qmljsutils.h>

#include <QButtonGroup>
#include <QDebug>
#include <QLabel>
#include <QTimerEvent>
#include <QToolButton>

namespace QmlEditorWidgets {

ContextPaneWidgetRectangle::ContextPaneWidgetRectangle(QWidget *parent)
    : QWidget(parent)
{
    const auto toolButton = [] (const QString &icon) {
        auto result = new QToolButton;
        result->setAutoExclusive(true);
        result->setCheckable(true);
        result->setFixedSize(30, 30);
        result->setIcon(QIcon(":/qmldesigner/images/" + icon + ".png"));
        return result;
    };

    const auto colorButton = [] {
        auto result = new ColorButton;
        result->setCheckable(true);
        result->setShowArrow(false);
        return result;
    };

    m_gradientLabel = new QLabel(Tr::tr("Gradient"));
    m_gradientLabel->setAlignment(Qt::AlignBottom);
    m_gradientLine = new GradientLine;
    m_gradientLine->setMinimumWidth(240);

    m_colorColorButton = colorButton();
    m_colorSolid = toolButton("icon_color_solid");
    m_colorGradient = toolButton("icon_color_gradient");
    m_colorNone = toolButton("icon_color_none");
    auto colorButtons = new QButtonGroup(this);
    colorButtons->addButton(m_colorSolid);
    colorButtons->addButton(m_colorGradient);
    colorButtons->addButton(m_colorNone);

    m_borderColorButton = colorButton();
    m_borderSolid = toolButton("icon_color_solid");
    m_borderNone = toolButton("icon_color_none");
    auto borderButtons = new QButtonGroup(this);
    borderButtons->addButton(m_borderSolid);
    borderButtons->addButton(m_borderNone);

    using namespace Layouting;
    Grid {
        m_gradientLabel, m_gradientLine, br,
        Tr::tr("Color"), Row { m_colorColorButton, m_colorSolid, m_colorGradient, m_colorNone, st, }, br,
        Tr::tr("Border"), Row { m_borderColorButton, m_borderSolid, m_borderNone, st, }, br,
    }.attachTo(this);

    connect(m_colorColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneWidgetRectangle::onColorButtonToggled);
    connect(m_borderColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneWidgetRectangle::onBorderColorButtonToggled);

    connect(m_colorSolid, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onColorSolidClicked);
    connect(m_borderSolid, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onBorderSolidClicked);

    connect(m_colorNone, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onColorNoneClicked);
    connect(m_borderNone, &QToolButton::clicked,
            this, &ContextPaneWidgetRectangle::onBorderNoneClicked);

    connect(m_colorGradient, &QToolButton::clicked, this, &ContextPaneWidgetRectangle::onGradientClicked);

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::accepted,
            this, &ContextPaneWidgetRectangle::onColorDialogApplied);
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::rejected,
            this, &ContextPaneWidgetRectangle::onColorDialogCancled);

    connect(m_gradientLine, &QmlEditorWidgets::GradientLine::openColorDialog,
            this, &ContextPaneWidgetRectangle::onGradientLineDoubleClicked);
    connect(m_gradientLine, &QmlEditorWidgets::GradientLine::gradientChanged,
            this, &ContextPaneWidgetRectangle::onUpdateGradient);
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
        m_colorColorButton->setColor(str);

    } else {
        m_colorColorButton->setColor(QLatin1String("white"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.color"))) {
        m_borderColorButton->setColor(propertyReader->readProperty(QLatin1String("border.color")).toString());
        m_hasBorder = true;
    } else {
        m_borderColorButton->setColor(QLatin1String("transparent"));
    }

    if (propertyReader->hasProperty(QLatin1String("border.width")))
        m_hasBorder = true;

    m_colorSolid->setChecked(true);
    m_borderNone->setChecked(true);
    m_borderSolid->setChecked(m_hasBorder);

    if (m_none)
        m_colorNone->setChecked(true);

    m_gradientLabel->setEnabled(true);
    m_gradientLine->setEnabled(true);

    if (m_hasGradient && isGradientEditingEnabled()) {
        bool isBound;
        m_colorGradient->setChecked(true);
        m_gradientLine->setGradient(propertyReader->parseGradient(QLatin1String("gradient"), &isBound));
        if (isBound) {
            m_gradientLabel->setEnabled(false);
            m_gradientLine->setEnabled(false);
            m_colorColorButton->setColor(QLatin1String("invalidColor"));
        }
    } else {
        m_gradientLine->setEnabled(false);
        m_gradientLabel->setEnabled(false);
        setColor();
    }

    if (m_gradientTimer > 0) {
        killTimer(m_gradientTimer);
        m_gradientTimer = -1;
    }

    m_colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::enabableGradientEditing(bool b)
{
    m_enableGradientEditing = b;
}

void ContextPaneWidgetRectangle::onBorderColorButtonToggled(bool flag)
{
    if (flag) {
        m_colorColorButton->setChecked(false);
        m_gradientLineDoubleClicked = false;
    }
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(m_borderColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(m_borderColorButton->convertedColor());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneWidgetRectangle::onColorButtonToggled(bool flag )
{
    if (flag) {
        m_borderColorButton->setChecked(false);
        m_gradientLineDoubleClicked = false;
    }
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(m_colorColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(m_colorColorButton->convertedColor());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneWidgetRectangle::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (m_colorColorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color()); //write back color
    if (m_borderColorButton->isChecked())
        emit  propertyChanged(QLatin1String("border.color"),parentContextWidget->colorDialog()->color()); //write back color
    if (m_gradientLineDoubleClicked)
        m_gradientLine->setActiveColor(parentContextWidget->colorDialog()->color());
    m_colorColorButton->setChecked(false);
    m_borderColorButton->setChecked(false);
    m_gradientLineDoubleClicked = false;
}

void ContextPaneWidgetRectangle::onColorDialogCancled()
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    m_colorColorButton->setChecked(false);
    m_borderColorButton->setChecked(false);
    m_gradientLineDoubleClicked = false;
}

void ContextPaneWidgetRectangle::onGradientClicked()
{
    if (m_colorGradient->isChecked()) {
        m_hasGradient = true;
        QLinearGradient gradient;
        QGradientStops stops;
        stops.append(QGradientStop(0, m_colorColorButton->convertedColor()));
        stops.append(QGradientStop(1, Qt::white));
        gradient.setStops(stops);
        m_gradientLine->setEnabled(true);
        m_gradientLine->setGradient(gradient);
    }
}

void ContextPaneWidgetRectangle::onColorNoneClicked()
{
    if (m_colorNone->isChecked()) {
        m_colorGradient->setEnabled(isGradientEditingEnabled());
        emit removeAndChangeProperty(QLatin1String("gradient"), QLatin1String("color"),
                                     QLatin1String("\"transparent\""), true);
    }
    m_colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::onColorSolidClicked()
{
    if (m_colorSolid->isChecked()) {
        m_gradientLine->setEnabled(false);
        emit removeAndChangeProperty(QLatin1String("gradient"), QLatin1String("color"),
                                     QLatin1String("\"black\""), true);
    }
    m_colorGradient->setEnabled(isGradientEditingEnabled());
}

void ContextPaneWidgetRectangle::onBorderNoneClicked()
{
    if (m_borderNone->isChecked()) {
        emit removeProperty(QLatin1String("border.color"));
        emit removeProperty(QLatin1String("border.width"));//###
    }
}

void ContextPaneWidgetRectangle::onBorderSolidClicked()
{
    if (m_borderSolid->isChecked())
        emit propertyChanged(QLatin1String("border.color"), QLatin1String("\"black\""));
}

void ContextPaneWidgetRectangle::onGradientLineDoubleClicked(const QPoint &p)
{
    m_gradientLineDoubleClicked = true;
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint pos = mapToGlobal(p);
    parentContextWidget->colorDialog()->setupColor(m_gradientLine->activeColor());
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

        QLinearGradient gradient = m_gradientLine->gradient();
        QString str = QLatin1String("Gradient {\n");
        const QGradientStops stops = gradient.stops();
        for (const QGradientStop &stop : stops) {
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
    QColor color = m_colorColorButton->convertedColor();
    stops.append(QGradientStop(0, color));
    stops.append(QGradientStop(1, color));
    gradient.setStops(stops);
    m_gradientLine->setGradient(gradient);
}

} //QmlDesigner
