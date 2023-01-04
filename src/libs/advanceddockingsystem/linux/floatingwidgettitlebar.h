// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
