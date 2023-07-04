// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinecontrols.h"
#include "timelinepropertyitem.h"

#include <coreplugin/icore.h>

#include <QColorDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>

#include <theme.h>

#include <limits>

namespace QmlDesigner {

TimelineControl *createTimelineControl(const NodeMetaInfo &metaInfo)
{
    if (metaInfo.isFloat())
        return new FloatControl;
    if (metaInfo.isColor())
        return new ColorControl;

    return nullptr;
}

FloatControl::FloatControl()
    : QDoubleSpinBox(nullptr)
{
    setValue(0.0);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    setFrame(false);
    setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);

    setMinimum(std::numeric_limits<float>::lowest());
    setMaximum(std::numeric_limits<float>::max());

    QColor bg = Theme::getColor(Theme::DScontrolBackground);

    auto p = palette();
    p.setColor(QPalette::Text, Theme::instance()->color(Utils::Theme::PanelTextColorLight));
    p.setColor(QPalette::Base, bg.darker(110));
    setPalette(p);

    m_timer.setInterval(100);
    m_timer.setSingleShot(true);

    auto startTimer = [this]( ) { m_timer.start(); };
    auto deferredSlot = [this]( ) { emit controlValueChanged(QVariant(this->value())); };

    QObject::connect(this, &QDoubleSpinBox::editingFinished, &m_timer, startTimer);
    QObject::connect(&m_timer, &QTimer::timeout, deferredSlot);
}

FloatControl::~FloatControl() = default;

QWidget *FloatControl::widget()
{
    return this;
}

void FloatControl::connect(TimelinePropertyItem *item)
{
    QObject::connect(this,
                     &FloatControl::controlValueChanged,
                     item,
                     &TimelinePropertyItem::changePropertyValue);
}

QVariant FloatControl::controlValue() const
{
    return QVariant(value());
}

void FloatControl::setControlValue(const QVariant &value)
{
    if (value.typeId() != QMetaType::Float && value.typeId() != QMetaType::Double)
        return;

    QSignalBlocker blocker(this);
    setValue(value.toDouble());
}

void FloatControl::setSize(int width, int height)
{
    setFixedWidth(width);
    setFixedHeight(height);
}

ColorControl::ColorControl()
    : QWidget(nullptr)
    , m_color(Qt::black)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(20);
}

ColorControl::ColorControl(const QColor &color, QWidget *parent)
    : QWidget(parent)
    , m_color(color)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(20);
}

ColorControl::~ColorControl() = default;

QWidget *ColorControl::widget()
{
    return this;
}

void ColorControl::connect(TimelinePropertyItem *item)
{
    QObject::connect(this,
                     &ColorControl::controlValueChanged,
                     item,
                     &TimelinePropertyItem::changePropertyValue);
}

QVariant ColorControl::controlValue() const
{
    return QVariant(value());
}

void ColorControl::setControlValue(const QVariant &value)
{
    if (value.typeId() != QMetaType::QColor)
        return;

    m_color = qvariant_cast<QColor>(value);
}

void ColorControl::setSize(int width, int height)
{
    setFixedWidth(width);
    setFixedHeight(height);
}

QColor ColorControl::value() const
{
    return m_color;
}

bool ColorControl::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (auto helpEvent = static_cast<const QHelpEvent *>(event)) {
            QToolTip::showText(helpEvent->globalPos(), m_color.name());
            return true;
        }
    }
    return QWidget::event(event);
}

void ColorControl::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(event->rect(), m_color);
}

void ColorControl::mouseReleaseEvent(QMouseEvent *event)
{
    QColor color = QColorDialog::getColor(m_color, Core::ICore::dialogParent());

    event->accept();

    if (color.isValid() && color != m_color) {
        m_color = color;
        update();
        emit valueChanged();
        emit controlValueChanged(QVariant(m_color));
    }
}

void ColorControl::mousePressEvent(QMouseEvent *event)
{
    // Needed to make the mouseRelease Event work if this
    // widget is embedded inside a QGraphicsProxyWidget.
    QWidget::mousePressEvent(event);
    event->accept();
}

} // End namespace QmlDesigner.
