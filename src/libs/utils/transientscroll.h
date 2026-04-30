// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QScrollBar>

QT_BEGIN_NAMESPACE
class QAbstractScrollArea;
QT_END_NAMESPACE

namespace Utils {

class ScrollBarPrivate;

class QTCREATOR_UTILS_EXPORT ScrollBar : public QScrollBar
{
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

    friend class TransientScrollAreaSupport;
};

namespace TransientScrollArea {
QTCREATOR_UTILS_EXPORT void support(QAbstractScrollArea *scrollArea);
} // namespace TransientScrollArea

namespace GlobalTransient {
QTCREATOR_UTILS_EXPORT void support(QWidget *widget);
} // namespace GlobalTransient

}
