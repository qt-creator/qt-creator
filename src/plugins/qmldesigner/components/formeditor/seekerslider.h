// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QIcon>

namespace QmlDesigner {

class SeekerSlider : public QWidget
{
    Q_OBJECT
public:
    SeekerSlider(QWidget *parentWidget);
    int position() const;
    int maxPosition() const
    {
        return m_maxPosition;
    }

    void setMaxPosition(int pos)
    {
        m_maxPosition = qMax(0, pos);
    }

Q_SIGNALS:
    void positionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_position = 0;
    int m_startPos = 0;
    int m_sliderPos = 0;
    int m_sliderHalfWidth = 0;
    int m_maxPosition = 30;
    bool m_moving = false;
    int m_bgWidth;
    int m_bgHeight;
    int m_handleWidth;
    int m_handleHeight;
    QIcon m_bgIcon;
    QIcon m_handleIcon;
};

} // namespace QmlDesigner
