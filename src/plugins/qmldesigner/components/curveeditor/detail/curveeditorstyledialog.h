
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace StyleEditor {
class ColorControl;
}

struct CurveEditorStyle;

class CurveEditorStyleDialog : public QDialog
{
    Q_OBJECT

signals:
    void styleChanged(const CurveEditorStyle &style);

public:
    CurveEditorStyleDialog(CurveEditorStyle &style, QWidget *parent = nullptr);

    CurveEditorStyle style() const;

private:
    void emitStyleChanged();

    void printStyle();

private:
    QPushButton *m_printButton;

    StyleEditor::ColorControl *m_background;

    StyleEditor::ColorControl *m_backgroundAlternate;

    StyleEditor::ColorControl *m_fontColor;

    StyleEditor::ColorControl *m_gridColor;

    QDoubleSpinBox *m_canvasMargin;

    QSpinBox *m_zoomInWidth;

    QSpinBox *m_zoomInHeight;

    QDoubleSpinBox *m_timeAxisHeight;

    QDoubleSpinBox *m_timeOffsetLeft;

    QDoubleSpinBox *m_timeOffsetRight;

    StyleEditor::ColorControl *m_rangeBarColor;

    StyleEditor::ColorControl *m_rangeBarCapsColor;

    QDoubleSpinBox *m_valueAxisWidth;

    QDoubleSpinBox *m_valueOffsetTop;

    QDoubleSpinBox *m_valueOffsetBottom;

    // HandleItem
    QDoubleSpinBox *m_handleSize;

    QDoubleSpinBox *m_handleLineWidth;

    StyleEditor::ColorControl *m_handleColor;

    StyleEditor::ColorControl *m_handleSelectionColor;

    // KeyframeItem
    QDoubleSpinBox *m_keyframeSize;

    StyleEditor::ColorControl *m_keyframeColor;

    StyleEditor::ColorControl *m_keyframeSelectionColor;

    // CurveItem
    QDoubleSpinBox *m_curveWidth;

    StyleEditor::ColorControl *m_curveColor;

    StyleEditor::ColorControl *m_curveSelectionColor;

    // TreeItem
    QDoubleSpinBox *m_treeMargins;

    // Playhead
    QDoubleSpinBox *m_playheadWidth;

    QDoubleSpinBox *m_playheadRadius;

    StyleEditor::ColorControl *m_playheadColor;
};

} // End namespace QmlDesigner.
