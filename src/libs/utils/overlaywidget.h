// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QWidget>

#include <functional>
#include <memory>

namespace Utils {

namespace Internal {
class OverlayWidgetPrivate;
}

class QTCREATOR_UTILS_EXPORT OverlayWidget : public QWidget
{
public:
    using PaintFunction = std::function<void(QWidget *, QPainter &, QPaintEvent *)>;
    using ResizeFunction = std::function<void(QWidget *, QSize)>;

    explicit OverlayWidget(QWidget *parent = nullptr);
    ~OverlayWidget();

    void attachToWidget(QWidget *parent);
    void setPaintFunction(const PaintFunction &paint);
    void setResizeFunction(const ResizeFunction &resize);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void paintEvent(QPaintEvent *ev) override;

private:
    void resizeToParent();

    std::unique_ptr<Internal::OverlayWidgetPrivate> d;
};

} // namespace Utils
