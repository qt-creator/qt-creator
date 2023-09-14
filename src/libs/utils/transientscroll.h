// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QScrollBar>

QT_BEGIN_NAMESPACE
class QAbstractScrollArea;
QT_END_NAMESPACE

namespace Utils {

class ScrollAreaPrivate;
class ScrollBarPrivate;

class QTCREATOR_UTILS_EXPORT TransientScrollAreaSupport : public QObject
{
public:
    static void support(QAbstractScrollArea *scrollArea);
    static void supportWidget(QWidget *widget);
    virtual ~TransientScrollAreaSupport();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit TransientScrollAreaSupport(QAbstractScrollArea *scrollArea);

    ScrollAreaPrivate *d = nullptr;
};

class QTCREATOR_UTILS_EXPORT ScrollBar : public QScrollBar
{

    friend class ScrollAreaPrivate;

public:
    explicit ScrollBar(QWidget *parent = nullptr);
    virtual ~ScrollBar();

    virtual void flash();

    bool setFocused(const bool &focused);

protected:
    void initStyleOption(QStyleOptionSlider *option) const override;
    bool event(QEvent *event) override;

private:
    bool setAdjacentVisible(const bool &visible);
    bool setAdjacentHovered(const bool &hovered);
    bool setViewPortInteraction(const bool &hovered);

    ScrollBarPrivate *d = nullptr;
};

class QTCREATOR_UTILS_EXPORT GlobalTransientSupport : public QObject
{
    Q_OBJECT
public:
    static void support(QWidget *widget);

private:
    GlobalTransientSupport();
    static GlobalTransientSupport *instance();
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
};
}
