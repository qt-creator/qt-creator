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

#include <nodemetainfo.h>

#include <QDoubleSpinBox>
#include <QTimer>
#include <QWidget>

namespace QmlDesigner {

class TimelinePropertyItem;

class TimelineControl
{
public:
    virtual ~TimelineControl() = default;

    virtual QWidget *widget() = 0;

    virtual void connect(TimelinePropertyItem *scene) = 0;

    virtual QVariant controlValue() const = 0;

    virtual void setControlValue(const QVariant &value) = 0;

    virtual void setSize(int width, int height) = 0;
};

TimelineControl *createTimelineControl(const TypeName &name);

class FloatControl : public QDoubleSpinBox, public TimelineControl
{
    Q_OBJECT

public:
    FloatControl();

    ~FloatControl() override;

    QWidget *widget() override;

    void connect(TimelinePropertyItem *scene) override;

    QVariant controlValue() const override;

    void setControlValue(const QVariant &value) override;

    void setSize(int width, int height) override;

signals:
    void controlValueChanged(const QVariant &value);

private:
    QTimer m_timer;
};

class ColorControl : public QWidget, public TimelineControl
{
    Q_OBJECT

public:
    ColorControl();

    ColorControl(const QColor &color, QWidget *parent = nullptr);

    ~ColorControl() override;

    QWidget *widget() override;

    void connect(TimelinePropertyItem *item) override;

    QVariant controlValue() const override;

    void setControlValue(const QVariant &value) override;

    void setSize(int width, int height) override;

    QColor value() const;

protected:
    bool event(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

signals:
    void valueChanged();

    void controlValueChanged(const QVariant &value);

private:
    QColor m_color;
};

} // namespace QmlDesigner
