// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "overlaywidget.h"

#include <QTimer>

#include <functional>
#include <memory>

namespace Utils {

enum class ProgressIndicatorSize
{
    Small,
    Medium,
    Large
};

class QTCREATOR_UTILS_EXPORT ProgressIndicatorPainter
{
public:
    using UpdateCallback = std::function<void()>;

    ProgressIndicatorPainter(ProgressIndicatorSize size);
    virtual ~ProgressIndicatorPainter() = default;

    void setIndicatorSize(ProgressIndicatorSize size);
    ProgressIndicatorSize indicatorSize() const;

    void setUpdateCallback(const UpdateCallback &cb);

    QSize size() const;

    void paint(QPainter &painter, const QRect &rect) const;

    void startAnimation();
    void stopAnimation();

protected:
    void nextAnimationStep();

private:
    ProgressIndicatorSize m_size = ProgressIndicatorSize::Small;
    int m_rotationStep = 45;
    int m_rotation = 0;
    QTimer m_timer;
    QPixmap m_pixmap;
    UpdateCallback m_callback;
};

class QTCREATOR_UTILS_EXPORT ProgressIndicator : public OverlayWidget
{
public:
    explicit ProgressIndicator(ProgressIndicatorSize size, QWidget *parent = nullptr);

    void setIndicatorSize(ProgressIndicatorSize size);

    QSize sizeHint() const final;

protected:
    void showEvent(QShowEvent *) final;
    void hideEvent(QHideEvent *) final;

private:
    ProgressIndicatorPainter m_paint;
};

} // Utils
