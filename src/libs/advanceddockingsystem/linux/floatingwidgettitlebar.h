/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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

#include <QFrame>

namespace ADS {

class FloatingDockContainer;
class FloatingWidgetTitleBarPrivate;

/**
 * Titlebar for floating widgets to capture non client are mouse events.
 * Linux does not support NonClientArea mouse events like
 * QEvent::NonClientAreaMouseButtonPress. Because these events are required
 * for the docking system to work properly, we use our own titlebar here to
 * capture the required mouse events.
 */
class FloatingWidgetTitleBar : public QFrame
{
    Q_OBJECT
private:
    FloatingWidgetTitleBarPrivate *d; ///< private data (pimpl)

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

public:
    using Super = QWidget;
    explicit FloatingWidgetTitleBar(FloatingDockContainer *parent = nullptr);

    /**
     * Virtual Destructor
     */
    ~FloatingWidgetTitleBar() override;

    /**
     * Enables / disables the window close button.
     */
    void enableCloseButton(bool enable);

    /**
     * Sets the window title, that means, the text of the internal tile label.
     */
    void setTitle(const QString &text);

    /**
     * Update stylesheet style if a property changes
     */
    void updateStyle();

signals:
    /**
     * This signal is emitted, if the close button is clicked.
     */
    void closeRequested();
};

} // namespace ADS
