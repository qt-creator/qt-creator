// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

TimelineControl *createTimelineControl(const NodeMetaInfo &metaInfo);

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
