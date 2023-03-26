// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinecontrols.h"

#include <theme.h>

#include <QColor>
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QDoubleSpinBox);

namespace QmlDesigner {

struct CanvasStyle
{
    qreal aspect = 1.5;

    qreal thinLineWidth = 0.3;
    qreal thickLineWidth = 2.5;

    QColor thinLineColor = Theme::getColor(Theme::DSscrollBarHandle);
    QColor thickLineColor = Theme::getColor(Theme::DSscrollBarHandle);

    qreal handleSize = 7.0;
    qreal handleLineWidth = 2.0;

    QColor endPointColor = Theme::getColor(Theme::IconsWarningToolBarColor);
    QColor interPointColor = Theme::getColor(Theme::DSerrorColor);

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
