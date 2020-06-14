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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "ads_globals.h"

#include <QFrame>
#include <QHash>
#include <QPointer>
#include <QRect>

QT_BEGIN_NAMESPACE
class QGridLayout;
QT_END_NAMESPACE

namespace ADS {

class DockOverlayPrivate;
class DockOverlayCross;

/**
 * DockOverlay paints a translucent rectangle over another widget. The geometry
 * of the rectangle is based on the mouse location.
 */
class ADS_EXPORT DockOverlay : public QFrame
{
    Q_OBJECT
private:
    DockOverlayPrivate *d; //< private data class
    friend class DockOverlayPrivate;
    friend class DockOverlayCross;

public:
    using Super = QFrame;

    enum eMode { ModeDockAreaOverlay, ModeContainerOverlay };

    /**
     * Creates a dock overlay
     */
    DockOverlay(QWidget *parent, eMode Mode = ModeDockAreaOverlay);

    /**
     * Virtual destructor
     */
    ~DockOverlay() override;

    /**
     * Configures the areas that are allowed for docking
     */
    void setAllowedAreas(DockWidgetAreas areas);

    /**
     * Returns flags with all allowed drop areas
     */
    DockWidgetAreas allowedAreas() const;

    /**
     * Returns the drop area under the current cursor location
     */
    DockWidgetArea dropAreaUnderCursor() const;

    /**
     * This function returns the same like dropAreaUnderCursor() if this
     * overlay is not hidden and if drop preview is enabled and returns
     * InvalidDockWidgetArea if it is hidden or drop preview is disabled.
     */
    DockWidgetArea visibleDropAreaUnderCursor() const;

    /**
     * Show the drop overly for the given target widget
     */
    DockWidgetArea showOverlay(QWidget *target);

    /**
     * Hides the overlay
     */
    void hideOverlay();

    /**
     * Enables / disables the semi transparent overlay rectangle that represents
     * the future area of the dropped widget
     */
    void enableDropPreview(bool enable);

    /**
     * Returns true if drop preview is enabled
     */
    bool dropPreviewEnabled() const;

    /**
     * The drop overlay rectangle for the target area
     */
    QRect dropOverlayRect() const;

    /**
     * Handle polish events
     */
    bool event(QEvent *event) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
};

class DockOverlayCrossPrivate;
/**
 * DockOverlayCross shows a cross with 5 different drop area possibilities.
 * I could have handled everything inside DockOverlay, but because of some
 * styling issues it's better to have a separate class for the cross.
 * You can style the cross icon using the property system.
 * \code
 * ADS--DockOverlayCross
 * {
 *     qproperty-iconFrameColor: palette(highlight);
 *     qproperty-iconBackgroundColor: palette(base);
 *     qproperty-iconOverlayColor: palette(highlight);
 *     qproperty-iconArrowColor: rgb(227, 227, 227);
 *     qproperty-iconShadowColor: rgb(0, 0, 0);
 * }
 * \endcode
 * Or you can use the iconColors property to pass in AARRGGBB values as
 * hex string like shown in the example below.
 * \code
 * ADS--DockOverlayCross
 * {
 *     qproperty-iconColors: "Frame=#ff3d3d3d Background=#ff929292 Overlay=#1f3d3d3d Arrow=#ffb4b4b4 Shadow=#40474747";
 * }
 * \endcode
 */
class DockOverlayCross : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString iconColors READ iconColors WRITE setIconColors)
    Q_PROPERTY(QColor iconFrameColor READ iconColor WRITE setIconFrameColor)
    Q_PROPERTY(QColor iconBackgroundColor READ iconColor WRITE setIconBackgroundColor)
    Q_PROPERTY(QColor iconOverlayColor READ iconColor WRITE setIconOverlayColor)
    Q_PROPERTY(QColor iconArrowColor READ iconColor WRITE setIconArrowColor)
    Q_PROPERTY(QColor iconShadowColor READ iconColor WRITE setIconShadowColor)

public:
    enum eIconColor {
        FrameColor,            ///< the color of the frame of the small window icon
        WindowBackgroundColor, ///< the background color of the small window in the icon
        OverlayColor,          ///< the color that shows the overlay (the dock side) in the icon
        ArrowColor,            ///< the arrow that points into the direction
        ShadowColor ///< the color of the shadow rectangle that is painted below the icons
    };

private:
    DockOverlayCrossPrivate *d;
    friend class DockOverlayCrossPrivate;
    friend class DockOverlay;

protected:
    /**
     * This function returns an empty string and is only here to silence
     * moc
     */
    QString iconColors() const;

    /**
     * This is a dummy function for the property system
     */
    QColor iconColor() const { return QColor(); }
    void setIconFrameColor(const QColor &color) { setIconColor(FrameColor, color); }
    void setIconBackgroundColor(const QColor &color) { setIconColor(WindowBackgroundColor, color); }
    void setIconOverlayColor(const QColor &color) { setIconColor(OverlayColor, color); }
    void setIconArrowColor(const QColor &color) { setIconColor(ArrowColor, color); }
    void setIconShadowColor(const QColor &color) { setIconColor(ShadowColor, color); }

public:
    /**
     * Creates an overlay cross for the given overlay
     */
    DockOverlayCross(DockOverlay *overlay);

    /**
     * Virtual destructor
     */
    ~DockOverlayCross() override;

    /**
      * Sets a certain icon color
      */
    void setIconColor(eIconColor colorIndex, const QColor &color);

    /**
     * Returns the icon color given by ColorIndex
     */
    QColor iconColor(eIconColor colorIndex) const;

    /**
     * Returns the dock widget area depending on the current cursor location.
     * The function checks, if the mouse cursor is inside of any drop indicator
     * widget and returns the corresponding DockWidgetArea.
     */
    DockWidgetArea cursorLocation() const;

    /**
     * Sets up the overlay cross for the given overlay mode
     */
    void setupOverlayCross(DockOverlay::eMode mode);

    /**
     * Recreates the overlay icons.
     */
    void updateOverlayIcons();

    /**
     * Resets and updates the
     */
    void reset();

    /**
     * Updates the current position
     */
    void updatePosition();

    /**
     * A string with all icon colors to set.
     * You can use this property to style the overly icon via CSS stylesheet
     * file. The colors are set via a color identifier and a hex AARRGGBB value like
     * in the example below.
     * \code
     * ADS--DockOverlayCross
     * {
     *     qproperty-iconColors: "Frame=#ff3d3d3d Background=#ff929292 Overlay=#1f3d3d3d Arrow=#ffb4b4b4 Shadow=#40474747";
     * }
     */
    void setIconColors(const QString &colors);

protected:
    void showEvent(QShowEvent *event) override;
    void setAreaWidgets(const QHash<DockWidgetArea, QWidget *> &widgets);
}; // DockOverlayCross

} // namespace ADS
