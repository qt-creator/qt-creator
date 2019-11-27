/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "curveeditorstyledialog.h"
#include "colorcontrol.h"
#include "curveeditorstyle.h"

#include <QDebug>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace DesignTools {

QHBoxLayout *createRow(const QString &title, QWidget *widget)
{
    auto *label = new QLabel(title);
    label->setFixedWidth(200);
    label->setAlignment(Qt::AlignRight);

    auto *box = new QHBoxLayout;
    box->addWidget(label);
    box->addWidget(widget);
    return box;
}

CurveEditorStyleDialog::CurveEditorStyleDialog(CurveEditorStyle &style, QWidget *parent)
    : QDialog(parent)
    , m_printButton(new QPushButton("Print"))
    , m_background(new ColorControl(style.backgroundBrush.color()))
    , m_backgroundAlternate(new ColorControl(style.backgroundAlternateBrush.color()))
    , m_fontColor(new ColorControl(style.fontColor))
    , m_gridColor(new ColorControl(style.gridColor))
    , m_canvasMargin(new QDoubleSpinBox())
    , m_zoomInWidth(new QSpinBox())
    , m_zoomInHeight(new QSpinBox())
    , m_timeAxisHeight(new QDoubleSpinBox())
    , m_timeOffsetLeft(new QDoubleSpinBox())
    , m_timeOffsetRight(new QDoubleSpinBox())
    , m_rangeBarColor(new ColorControl(style.rangeBarCapsColor))
    , m_rangeBarCapsColor(new ColorControl(style.rangeBarCapsColor))
    , m_valueAxisWidth(new QDoubleSpinBox())
    , m_valueOffsetTop(new QDoubleSpinBox())
    , m_valueOffsetBottom(new QDoubleSpinBox())
    , m_handleSize(new QDoubleSpinBox())
    , m_handleLineWidth(new QDoubleSpinBox())
    , m_handleColor(new ColorControl(style.handleStyle.color))
    , m_handleSelectionColor(new ColorControl(style.handleStyle.selectionColor))
    , m_keyframeSize(new QDoubleSpinBox())
    , m_keyframeColor(new ColorControl(style.keyframeStyle.color))
    , m_keyframeSelectionColor(new ColorControl(style.keyframeStyle.selectionColor))
    , m_curveWidth(new QDoubleSpinBox())
    , m_curveColor(new ColorControl(style.curveStyle.color))
    , m_curveSelectionColor(new ColorControl(style.curveStyle.selectionColor))
    , m_treeMargins(new QDoubleSpinBox())
    , m_playheadWidth(new QDoubleSpinBox())
    , m_playheadRadius(new QDoubleSpinBox())
    , m_playheadColor(new ColorControl(style.playhead.color))

{
    setWindowFlag(Qt::Tool, true);

    m_canvasMargin->setValue(style.canvasMargin);
    m_zoomInWidth->setValue(style.zoomInWidth);
    m_zoomInHeight->setValue(style.zoomInHeight);
    m_zoomInHeight->setMaximum(9000);

    m_timeAxisHeight->setValue(style.timeAxisHeight);
    m_timeOffsetLeft->setValue(style.timeOffsetLeft);
    m_timeOffsetRight->setValue(style.timeOffsetRight);
    m_valueAxisWidth->setValue(style.valueAxisWidth);
    m_valueOffsetTop->setValue(style.valueOffsetTop);
    m_valueOffsetBottom->setValue(style.valueOffsetBottom);
    m_handleSize->setValue(style.handleStyle.size);
    m_handleLineWidth->setValue(style.handleStyle.lineWidth);
    m_keyframeSize->setValue(style.keyframeStyle.size);
    m_curveWidth->setValue(style.curveStyle.width);
    m_treeMargins->setValue(style.treeItemStyle.margins);
    m_playheadWidth->setValue(style.playhead.width);
    m_playheadRadius->setValue(style.playhead.radius);

    connect(m_printButton, &QPushButton::released, this, &CurveEditorStyleDialog::printStyle);

    auto intChanged = [this](int) { emitStyleChanged(); };
    auto doubleChanged = [this](double) { emitStyleChanged(); };
    auto colorChanged = [this]() { emitStyleChanged(); };

    auto intSignal = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
    auto doubleSignal = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    connect(m_background, &ColorControl::valueChanged, colorChanged);
    connect(m_backgroundAlternate, &ColorControl::valueChanged, colorChanged);
    connect(m_fontColor, &ColorControl::valueChanged, colorChanged);
    connect(m_gridColor, &ColorControl::valueChanged, colorChanged);
    connect(m_canvasMargin, doubleSignal, doubleChanged);
    connect(m_zoomInWidth, intSignal, intChanged);
    connect(m_zoomInHeight, intSignal, intChanged);
    connect(m_timeAxisHeight, doubleSignal, doubleChanged);
    connect(m_timeOffsetLeft, doubleSignal, doubleChanged);
    connect(m_timeOffsetRight, doubleSignal, doubleChanged);
    connect(m_rangeBarColor, &ColorControl::valueChanged, colorChanged);
    connect(m_rangeBarCapsColor, &ColorControl::valueChanged, colorChanged);
    connect(m_valueAxisWidth, doubleSignal, doubleChanged);
    connect(m_valueOffsetTop, doubleSignal, doubleChanged);
    connect(m_valueOffsetBottom, doubleSignal, doubleChanged);
    connect(m_handleSize, doubleSignal, doubleChanged);
    connect(m_handleLineWidth, doubleSignal, doubleChanged);
    connect(m_handleColor, &ColorControl::valueChanged, colorChanged);
    connect(m_handleSelectionColor, &ColorControl::valueChanged, colorChanged);
    connect(m_keyframeSize, doubleSignal, doubleChanged);
    connect(m_keyframeColor, &ColorControl::valueChanged, colorChanged);
    connect(m_keyframeSelectionColor, &ColorControl::valueChanged, colorChanged);
    connect(m_curveWidth, doubleSignal, doubleChanged);
    connect(m_curveColor, &ColorControl::valueChanged, colorChanged);
    connect(m_curveSelectionColor, &ColorControl::valueChanged, colorChanged);
    connect(m_treeMargins, doubleSignal, doubleChanged);
    connect(m_playheadWidth, doubleSignal, doubleChanged);
    connect(m_playheadRadius, doubleSignal, doubleChanged);
    connect(m_playheadColor, &ColorControl::valueChanged, colorChanged);

    auto *box = new QVBoxLayout;
    box->addLayout(createRow("Background Color", m_background));
    box->addLayout(createRow("Alternate Background Color", m_backgroundAlternate));
    box->addLayout(createRow("Font Color", m_fontColor));
    box->addLayout(createRow("Grid Color", m_gridColor));
    box->addLayout(createRow("Canvas Margin", m_canvasMargin));
    box->addLayout(createRow("Zoom In Width", m_zoomInWidth));
    box->addLayout(createRow("Zoom In Height", m_zoomInHeight));
    box->addLayout(createRow("Time Axis Height", m_timeAxisHeight));
    box->addLayout(createRow("Time Axis Left Offset", m_timeOffsetLeft));
    box->addLayout(createRow("Time Axis Right Offset", m_timeOffsetRight));
    box->addLayout(createRow("Range Bar Color", m_rangeBarColor));
    box->addLayout(createRow("Range Bar Caps Color", m_rangeBarCapsColor));
    box->addLayout(createRow("Value Axis Width", m_valueAxisWidth));
    box->addLayout(createRow("Value Axis Top Offset", m_valueOffsetTop));
    box->addLayout(createRow("Value Axis Bottom Offset", m_valueOffsetBottom));
    box->addLayout(createRow("Handle Size", m_handleSize));
    box->addLayout(createRow("Handle Line Width", m_handleLineWidth));
    box->addLayout(createRow("Handle Color", m_handleColor));
    box->addLayout(createRow("Handle Selection Color", m_handleSelectionColor));
    box->addLayout(createRow("Keyframe Size", m_keyframeSize));
    box->addLayout(createRow("Keyframe Color", m_keyframeColor));
    box->addLayout(createRow("Keyframe Selection Color", m_keyframeSelectionColor));
    box->addLayout(createRow("Curve Width", m_curveWidth));
    box->addLayout(createRow("Curve Color", m_curveColor));
    box->addLayout(createRow("Curve Selection Color", m_curveSelectionColor));
    box->addLayout(createRow("Treeview margins", m_treeMargins));
    box->addLayout(createRow("Playhead width", m_playheadWidth));
    box->addLayout(createRow("Playhead radius", m_playheadRadius));
    box->addLayout(createRow("Playhead color", m_playheadColor));

    box->addWidget(m_printButton);
    setLayout(box);
}

CurveEditorStyle CurveEditorStyleDialog::style() const
{
    CurveEditorStyle style;
    style.backgroundBrush = QBrush(m_background->value());
    style.backgroundAlternateBrush = QBrush(m_backgroundAlternate->value());
    style.fontColor = m_fontColor->value();
    style.gridColor = m_gridColor->value();
    style.canvasMargin = m_canvasMargin->value();
    style.zoomInWidth = m_zoomInWidth->value();
    style.zoomInHeight = m_zoomInHeight->value();
    style.timeAxisHeight = m_timeAxisHeight->value();
    style.timeOffsetLeft = m_timeOffsetLeft->value();
    style.timeOffsetRight = m_timeOffsetRight->value();
    style.rangeBarColor = m_rangeBarColor->value();
    style.rangeBarCapsColor = m_rangeBarCapsColor->value();
    style.valueAxisWidth = m_valueAxisWidth->value();
    style.valueOffsetTop = m_valueOffsetTop->value();
    style.valueOffsetBottom = m_valueOffsetBottom->value();
    style.handleStyle.size = m_handleSize->value();
    style.handleStyle.lineWidth = m_handleLineWidth->value();
    style.handleStyle.color = m_handleColor->value();
    style.handleStyle.selectionColor = m_handleSelectionColor->value();
    style.keyframeStyle.size = m_keyframeSize->value();
    style.keyframeStyle.color = m_keyframeColor->value();
    style.keyframeStyle.selectionColor = m_keyframeSelectionColor->value();
    style.curveStyle.width = m_curveWidth->value();
    style.curveStyle.color = m_curveColor->value();
    style.curveStyle.selectionColor = m_curveSelectionColor->value();
    style.treeItemStyle.margins = m_treeMargins->value();
    style.playhead.width = m_playheadWidth->value();
    style.playhead.radius = m_playheadRadius->value();
    style.playhead.color = m_playheadColor->value();

    return style;
}

void CurveEditorStyleDialog::emitStyleChanged()
{
    emit styleChanged(style());
}

void CurveEditorStyleDialog::printStyle()
{
    auto toString = [](const QColor &color) {
        QString tmp
            = QString("QColor(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());
        return qPrintable(tmp);
    };

    CurveEditorStyle s = style();
    qDebug() << "";
    qDebug().nospace() << "CurveEditorStyle out;";
    qDebug().nospace() << "out.backgroundBrush = QBrush(" << toString(s.backgroundBrush.color())
                       << ");";
    qDebug().nospace() << "out.backgroundAlternateBrush = QBrush("
                       << toString(s.backgroundAlternateBrush.color()) << ");";
    qDebug().nospace() << "out.fontColor = " << toString(s.fontColor) << ";";
    qDebug().nospace() << "out.gridColor = " << toString(s.gridColor) << ";";
    qDebug().nospace() << "out.canvasMargin = " << s.canvasMargin << ";";
    qDebug().nospace() << "out.zoomInWidth = " << s.zoomInWidth << ";";
    qDebug().nospace() << "out.zoomInHeight = " << s.zoomInHeight << ";";
    qDebug().nospace() << "out.timeAxisHeight = " << s.timeAxisHeight << ";";
    qDebug().nospace() << "out.timeOffsetLeft = " << s.timeOffsetLeft << ";";
    qDebug().nospace() << "out.timeOffsetRight = " << s.timeOffsetRight << ";";
    qDebug().nospace() << "out.rangeBarColor = " << toString(s.rangeBarColor) << ";";
    qDebug().nospace() << "out.rangeBarCapsColor = " << toString(s.rangeBarCapsColor) << ";";
    qDebug().nospace() << "out.valueAxisWidth = " << s.valueAxisWidth << ";";
    qDebug().nospace() << "out.valueOffsetTop = " << s.valueOffsetTop << ";";
    qDebug().nospace() << "out.valueOffsetBottom = " << s.valueOffsetBottom << ";";
    qDebug().nospace() << "out.handleStyle.size = " << s.handleStyle.size << ";";
    qDebug().nospace() << "out.handleStyle.lineWidth = " << s.handleStyle.lineWidth << ";";
    qDebug().nospace() << "out.handleStyle.color = " << toString(s.handleStyle.color) << ";";
    qDebug().nospace() << "out.handleStyle.selectionColor = "
                       << toString(s.handleStyle.selectionColor) << ";";
    qDebug().nospace() << "out.keyframeStyle.size = " << s.keyframeStyle.size << ";";
    qDebug().nospace() << "out.keyframeStyle.color = " << toString(s.keyframeStyle.color) << ";";
    qDebug().nospace() << "out.keyframeStyle.selectionColor = "
                       << toString(s.keyframeStyle.selectionColor) << ";";
    qDebug().nospace() << "out.curveStyle.width = " << s.curveStyle.width << ";";
    qDebug().nospace() << "out.curveStyle.color = " << toString(s.curveStyle.color) << ";";
    qDebug().nospace() << "out.curveStyle.selectionColor = "
                       << toString(s.curveStyle.selectionColor) << ";";
    qDebug().nospace() << "out.treeItemStyle.margins = " << s.treeItemStyle.margins << ";";
    qDebug().nospace() << "out.playheadStyle.width = " << s.playhead.width << ";";
    qDebug().nospace() << "out.playheadStyle.radius = " << s.playhead.radius << ";";
    qDebug().nospace() << "out.playheadStyle.color = " << toString(s.playhead.color) << ";";
    qDebug().nospace() << "return out;";
    qDebug() << "";
}

} // End namespace DesignTools.
