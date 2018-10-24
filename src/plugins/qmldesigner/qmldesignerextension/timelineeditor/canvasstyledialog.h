/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#pragma once

#include "timelinecontrols.h"

#include <QColor>
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox);

namespace QmlDesigner {

struct CanvasStyle
{
    qreal aspect = 1.5;

    qreal thinLineWidth = 0.3;
    qreal thickLineWidth = 2.5;

    QColor thinLineColor = qRgb(0x99, 0x99, 0x99);
    QColor thickLineColor = qRgb(0x5f, 0x5f, 0x5f);

    qreal handleSize = 7.0;
    qreal handleLineWidth = 2.0;

    QColor endPointColor = qRgb(0xd6, 0xd3, 0x51);
    QColor interPointColor = qRgb(0xce, 0x17, 0x17);

    qreal curveWidth = 3.0;
};

class CanvasStyleDialog : public QDialog
{
    Q_OBJECT

public:
    CanvasStyleDialog(const CanvasStyle &style, QWidget *parent = nullptr);

signals:
    void styleChanged(const CanvasStyle &style);

private:
    QDoubleSpinBox *m_aspect;

    QDoubleSpinBox *m_thinLineWidth;
    QDoubleSpinBox *m_thickLineWidth;

    ColorControl *m_thinLineColor;
    ColorControl *m_thickLineColor;

    QDoubleSpinBox *m_handleSize;
    QDoubleSpinBox *m_handleLineWidth;

    ColorControl *m_endPointColor;
    ColorControl *m_interPointColor;

    QDoubleSpinBox *m_curveWidth;
};

} // namespace QmlDesigner
