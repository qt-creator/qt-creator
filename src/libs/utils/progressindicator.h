/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "utils_global.h"

#include <QTimer>
#include <QWidget>

#include <functional>
#include <memory>

namespace Utils {

namespace Internal { class ProgressIndicatorPrivate; }

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

    virtual void setIndicatorSize(ProgressIndicatorSize size);
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

class QTCREATOR_UTILS_EXPORT ProgressIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressIndicator(ProgressIndicatorSize size, QWidget *parent = nullptr);

    void setIndicatorSize(ProgressIndicatorSize size);

    QSize sizeHint() const final;

    void attachToWidget(QWidget *parent);

protected:
    void paintEvent(QPaintEvent *) final;
    void showEvent(QShowEvent *) final;
    void hideEvent(QHideEvent *) final;
    bool eventFilter(QObject *obj, QEvent *ev) final;

private:
    void resizeToParent();

    ProgressIndicatorPainter m_paint;
};

} // Utils
