// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace QmlDesigner {

namespace StyleEditor {

class ColorControl : public QWidget
{
    Q_OBJECT

public:
    ColorControl();

    ColorControl(const QColor &color, QWidget *parent = nullptr);

    ~ColorControl() override;

    QColor value() const;

    void setValue(const QColor &val);

protected:
    bool event(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

signals:
    void valueChanged();

private:
    QColor m_color;
};

} // End namespace StyleEditor.

} // End namespace QmlDesigner.
