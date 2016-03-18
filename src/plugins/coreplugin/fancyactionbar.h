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

#include <QToolButton>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class FancyToolButton : public QToolButton
{
    Q_OBJECT

    Q_PROPERTY(float fader READ fader WRITE setFader)

public:
    FancyToolButton(QAction *action, QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    bool event(QEvent *e);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    float m_fader;
    float fader() { return m_fader; }
    void setFader(float value) { m_fader = value; update(); }

    static void hoverOverlay(QPainter *painter, const QRect &spanRect);

private:
    void actionChanged();
};

class FancyActionBar : public QWidget
{
    Q_OBJECT

public:
    FancyActionBar(QWidget *parent = 0);

    void paintEvent(QPaintEvent *event);
    void insertAction(int index, QAction *action);
    void addProjectSelector(QAction *action);
    QLayout *actionsLayout() const;
    QSize minimumSizeHint() const;

private:
    QVBoxLayout *m_actionsLayout;
};

} // namespace Internal
} // namespace Core
