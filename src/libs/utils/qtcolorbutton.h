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

#include <QToolButton>

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtColorButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(bool backgroundCheckered READ isBackgroundCheckered WRITE setBackgroundCheckered)
    Q_PROPERTY(bool alphaAllowed READ isAlphaAllowed WRITE setAlphaAllowed)
    Q_PROPERTY(QColor color READ color WRITE setColor)
public:
    QtColorButton(QWidget *parent = 0);
    ~QtColorButton();

    bool isBackgroundCheckered() const;
    void setBackgroundCheckered(bool checkered);

    bool isAlphaAllowed() const;
    void setAlphaAllowed(bool allowed);

    QColor color() const;

public slots:
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);
protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
#endif
private:
    class QtColorButtonPrivate *d_ptr;
    friend class QtColorButtonPrivate;
};

} // namespace Utils
