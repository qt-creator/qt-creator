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
    Q_OBJECT
public:
    static void support(QAbstractScrollArea *scrollArea);
    virtual ~TransientScrollAreaSupport();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit TransientScrollAreaSupport(QAbstractScrollArea *scrollArea);

    ScrollAreaPrivate *d = nullptr;
};

class QTCREATOR_UTILS_EXPORT ScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    explicit ScrollBar(QWidget *parent = nullptr);
    virtual ~ScrollBar();

    QSize sizeHint() const override;

    virtual void flash();

protected:
    virtual void initStyleOption(QStyleOptionSlider *option) const override;
    bool event(QEvent *event) override;

private:
    ScrollBarPrivate *d = nullptr;
};

}
