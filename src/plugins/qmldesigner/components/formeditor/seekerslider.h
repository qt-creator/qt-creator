/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
