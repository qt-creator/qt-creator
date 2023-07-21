// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#pragma once

#include "ads_globals.h"

#include <QPushButton>

namespace ADS {

/**
 * ADS specific push button class with orientation support
 */
class ADS_EXPORT PushButton : public QPushButton
{
    Q_OBJECT

public:
    enum Orientation {
        Horizontal,
        VerticalTopToBottom,
        VerticalBottomToTop
    };

    using QPushButton::QPushButton;

    virtual QSize sizeHint() const override;

    /**
     * Returns the current orientation
     */
    Orientation buttonOrientation() const;

    /**
     * Set the orientation of this button
     */
    void setButtonOrientation(Orientation orientation);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    Orientation m_orientation = Horizontal;
}; // class PushButton

} // namespace ADS
