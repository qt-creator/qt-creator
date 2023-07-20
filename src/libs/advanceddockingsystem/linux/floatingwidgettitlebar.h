// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>
#include <QIcon>

namespace ADS {

class FloatingDockContainer;
class FloatingWidgetTitleBarPrivate;

/**
 * Title bar for floating widgets to capture non client area mouse events.
 * Linux does not support NonClientArea mouse events like
 * QEvent::NonClientAreaMouseButtonPress. Because these events are required for the docking system
 * to work properly, we use our own titlebar here to capture the required mouse events.
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
    void mouseDoubleClickEvent(QMouseEvent *event) override;

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

    /**
     * Change the maximize button icon according to current windows state
     */
    void setMaximizedIcon(bool maximized);

signals:
    /**
     * This signal is emitted, if the close button is clicked.
     */
    void closeRequested();

    /**
    * This signal is emitted, if the maximize button is clicked.
    */
    void maximizeRequested();
};

} // namespace ADS
