// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "canvasstyledialog.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace QmlDesigner {

CanvasStyleDialog::CanvasStyleDialog(const CanvasStyle &style, QWidget *parent)
    : QDialog(parent)
    , m_aspect(new QDoubleSpinBox(this))
    , m_thinLineWidth(new QDoubleSpinBox(this))
    , m_thickLineWidth(new QDoubleSpinBox(this))
    , m_thinLineColor(new ColorControl(style.thinLineColor, this))
    , m_thickLineColor(new ColorControl(style.thickLineColor, this))
    , m_handleSize(new QDoubleSpinBox(this))
    , m_handleLineWidth(new QDoubleSpinBox(this))
    , m_endPointColor(new ColorControl(style.endPointColor, this))
    , m_interPointColor(new ColorControl(style.interPointColor, this))
    , m_curveWidth(new QDoubleSpinBox(this))
{
    m_aspect->setValue(style.aspect);
    m_thinLineWidth->setValue(style.thinLineWidth);
    m_thickLineWidth->setValue(style.thickLineWidth);
    m_handleSize->setValue(style.handleSize);
    m_handleLineWidth->setValue(style.handleLineWidth);
    m_curveWidth->setValue(style.curveWidth);

    int labelWidth = QFontMetrics(this->font()).horizontalAdvance("Inter Handle ColorXX");
    auto addControl = [labelWidth](QVBoxLayout *layout, const QString &name, QWidget *control) {
        auto *hbox = new QHBoxLayout;

        QLabel *label = new QLabel(name);
        label->setAlignment(Qt::AlignLeft);
        label->setFixedWidth(labelWidth);

        hbox->addWidget(label);
        hbox->addWidget(control);
        layout->addLayout(hbox);
    };

    auto layout = new QVBoxLayout;
    addControl(layout, "Aspect Ratio", m_aspect);

    addControl(layout, "Thin Line Width", m_thinLineWidth);
    addControl(layout, "Thin Line Color", m_thinLineColor);

    addControl(layout, "Thick Line Width", m_thickLineWidth);
    addControl(layout, "Thick Line Color", m_thickLineColor);

    addControl(layout, "Handle Size", m_handleSize);
    addControl(layout, "Handle Line Width", m_handleLineWidth);
    addControl(layout, "End Handle Color", m_endPointColor);
    addControl(layout, "Inter Handle Color", m_interPointColor);

    addControl(layout, "Curve Width", m_curveWidth);

    setLayout(layout);

    auto emitValueChanged = [this]() {
        CanvasStyle out;
        out.aspect = m_aspect->value();
        out.thinLineWidth = m_thinLineWidth->value();
        out.thickLineWidth = m_thickLineWidth->value();
        out.thinLineColor = m_thinLineColor->value();
        out.thickLineColor = m_thickLineColor->value();
        out.handleSize = m_handleSize->value();
        out.handleLineWidth = m_handleLineWidth->value();
        out.endPointColor = m_endPointColor->value();
        out.interPointColor = m_interPointColor->value();
        out.curveWidth = m_curveWidth->value();
        emit styleChanged(out);
    };

    connect(m_aspect, &QDoubleSpinBox::valueChanged, this, emitValueChanged);

    connect(m_thinLineWidth, &QDoubleSpinBox::valueChanged, this, emitValueChanged);
    connect(m_thickLineWidth, &QDoubleSpinBox::valueChanged, this, emitValueChanged);

    connect(m_thinLineColor, &ColorControl::valueChanged, this, emitValueChanged);
    connect(m_thickLineColor, &ColorControl::valueChanged, this, emitValueChanged);

    connect(m_handleSize, &QDoubleSpinBox::valueChanged, this, emitValueChanged);
    connect(m_handleLineWidth, &QDoubleSpinBox::valueChanged, this, emitValueChanged);

    connect(m_endPointColor, &ColorControl::valueChanged, this, emitValueChanged);
    connect(m_interPointColor, &ColorControl::valueChanged, this, emitValueChanged);

    connect(m_curveWidth, &QDoubleSpinBox::valueChanged, this, emitValueChanged);
}

} // namespace QmlDesigner
