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

#include <QAbstractButton>

QT_FORWARD_DECLARE_CLASS(QGraphicsOpacityEffect)

namespace Utils {
class QTCREATOR_UTILS_EXPORT FadingPanel : public QWidget
{
    Q_OBJECT

public:
    FadingPanel(QWidget *parent = 0)
        : QWidget(parent)
    {}
    virtual void fadeTo(qreal value) = 0;
    virtual void setOpacity(qreal value) = 0;
};

class QTCREATOR_UTILS_EXPORT FadingWidget : public FadingPanel
{
    Q_OBJECT
public:
    FadingWidget(QWidget *parent = 0);
    void fadeTo(qreal value);
    qreal opacity();
    void setOpacity(qreal value);
protected:
    QGraphicsOpacityEffect *m_opacityEffect;
};

class QTCREATOR_UTILS_EXPORT DetailsButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float fader READ fader WRITE setFader)

public:
    DetailsButton(QWidget *parent = 0);

    QSize sizeHint() const;
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

protected:
    void paintEvent(QPaintEvent *e);
    bool event(QEvent *e);

private:
    QPixmap cacheRendering(const QSize &size, bool checked);
    QPixmap m_checkedPixmap;
    QPixmap m_uncheckedPixmap;
    float m_fader;
};
} // namespace Utils
