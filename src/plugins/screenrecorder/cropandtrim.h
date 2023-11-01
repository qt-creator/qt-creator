// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ffmpegutils.h"

#include <utils/styledbar.h>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace ScreenRecorder {

class CropScene : public QWidget
{
    Q_OBJECT

public:
    CropScene(QWidget *parent = nullptr);

    QRect cropRect() const;
    void setCropRect(const QRect &rect);
    bool fullySelected() const;
    void setFullySelected();
    QRect fullRect() const;
    void setImage(const QImage &image);
    QImage croppedImage() const;

    const static int lineWidth = 1;

signals:
    void cropRectChanged(const QRect &cropRect);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    enum MarginEditing {
        EdgeLeft,
        EdgeTop,
        EdgeRight,
        EdgeBottom,
        Free,
        Move,
    };

    void initMouseInteraction(const QPoint &pos);
    void updateBuffer();
    QPoint toImagePos(const QPoint &widgetCoordinate) const;
    QRect activeMoveArea() const;

    const static int m_gripWidth = 8;
    QRect m_cropRect;
    const QImage *m_image = nullptr;
    QImage m_buffer;

    struct MouseInteraction {
        bool dragging = false;
        MarginEditing margin;
        QPoint startImagePos;
        QPoint clickOffset; // Due to m_gripWidth, the mouse pointer is not precicely on the drag
            // line. Maintain the offset while dragging, to avoid an initial jump.
        Qt::CursorShape cursorShape = Qt::ArrowCursor;
    } m_mouse;
};

class CropAndTrimWidget : public Utils::StyledBar
{
    Q_OBJECT

public:
    CropAndTrimWidget(QWidget *parent = nullptr);

    void setClip(const ClipInfo &clip);

signals:
    void cropRectChanged(const QRect &rect);
    void trimRangeChanged(FrameRange range);

private:
    void updateWidgets();

    QToolButton *m_button;

    ClipInfo m_clipInfo;
    QRect m_cropRect;
    int m_currentFrame = 0;
    FrameRange m_trimRange;
    CropSizeWarningIcon *m_cropSizeWarningIcon;
};

} // namespace ScreenRecorder
