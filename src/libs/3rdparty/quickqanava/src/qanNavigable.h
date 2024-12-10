/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanNavigable.h
// \author	benoit@destrat.io
// \date	2015 07 19
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>

// QuickQanava headers
#include "./qanGrid.h"

namespace qan { // ::qan

/*! \brief Provide a pannable/zoomable container for quick items.
 *
 * Child items must manually set their parent property to this area \c containerItem:
 * Example code for navigating an image:
 * \code
 * // Don't forget to register the component:
 * // C++: qmlRegisterType<qan::Navigable>("Qanava", 1, 0, "Navigable");
 * // QML: import QuickQanava as Qan
 * Qan.Navigable {
 *   anchors.fill: parent
 *   clip: true     // Don't set clipping if Navigable is anchored directly to your main window
 *   Image {
 *       parent: imageView.containerItem    // Any direct child must manually set its parent item
 *       id: image
 *       source: "qrc:/myimage.png"
 *   }
 * }
 * \endcode
 *
 * A "navigable area" could easily be controlled with a slider control:
 * \code
 * // QML: import QuickQanava as Qan
 *
 * Qan.Navigable {
 *   id: navigable
 *   clip: true
 *   Slider {
 *     anchors.top: navigable.top
 *     anchors.horizontalCenter: navigable.horizontalCenter
 *     width: navigable.width / 2
 *     maximumValue: navigable.zoomMax > 0. ? navigable.zoomMax : 10.
 *     minimumValue: navigable.zoomMin
 *     stepSize: 0.0   // Non 0 stepSize will create binding loops until you protect the onValueChanged with a fuzzyCompare against zoom+-stepSize
 *     value: navigable.zoom
 *     onValueChanged: { navigable.zoom = value }
 *   }
 * }
 * \endcode
 *
 * Setting the \c autoFitMode to \c Navigable::AutoFit has the following effects:
 * \copydetails qan::Navigable::AutoFit
 *
 * \note For the math behind "zoom on a point", see:
 * http://stackoverflow.com/questions/2916081/zoom-in-on-a-point-using-scale-and-translate
 * and
 * http://stackoverflow.com/questions/16657397/scale-qgraphicsitem-about-an-arbitrary-point
 */
class Navigable : public QQuickItem
{
    /*! \name Navigable Object Management *///---------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit Navigable(QQuickItem* parent = nullptr);
    virtual ~Navigable() override;
    Navigable(const Navigable&) = delete;
    //@}
    //-------------------------------------------------------------------------


    /*! \name Navigation Management *///---------------------------------------
    //@{
public:
    /*! \brief Enable or disable navigation (navigation is enabled by default).
     */
    Q_PROPERTY(bool navigable READ getNavigable WRITE setNavigable NOTIFY navigableChanged FINAL)
    //! \sa navigable
    inline bool     getNavigable() const noexcept { return _navigable; }
    //! \sa navigable
    void            setNavigable(bool navigable) noexcept;
private:
    //! \copydoc navigable
    bool            _navigable = true;
signals:
    //! \sa navigable
    void            navigableChanged();

public:
    /*! \brief Parent container for area child items.
     *
     * Items added as child of the area must manually update their parents property to \c containerItem
     *
     * Example code for navigating an image:
     * \code
     * // Don't forget to register the component:
     * // C++: qmlRegisterType<qan::Navigable>("Qanava", 1, 0, "Navigable");
     * // QML: import QuickQanava 2.0
     * Qan.Navigable {
     *   anchors.fill: parent
     *   clip: true     // Don't set clipping if Navigable is anchored directly to your main window
     *   Image {
     *       parent: imageView.containerItem    // Any direct child must manually set its parent item
     *       id: image
     *       source: "qrc:/myimage.png"
     *       Component.onCompleted: {
     *         // Eventually, fit the image in view: navigable.fitContentInView(), where navigable is Navigable id
     *       }
     *   }
     * }
     * \endcode
     *
     */
    Q_PROPERTY(QQuickItem*  containerItem READ getContainerItem CONSTANT FINAL)
    //! \sa containerItem
    inline QQuickItem*      getContainerItem() noexcept { return _containerItem.data(); }
private:
    QPointer<QQuickItem>    _containerItem = nullptr;

public:
    //! Internally used for scrollbar and navigable preview, consider private.
    Q_PROPERTY(QQuickItem*  virtualItem READ getVirtualItem CONSTANT FINAL)
    //! \sa virtualItem
    inline QQuickItem*      getVirtualItem() noexcept { return _virtualItem.data(); }
private:
    QPointer<QQuickItem>    _virtualItem = nullptr;
protected slots:
    //! Update the virtual item Br to follow a container item grow.
    void    updateVirtualBr(const QRectF& containerChildrenRect);

public:
    //! \copydoc getViewRect().
    Q_PROPERTY(QRectF viewRect READ getViewRect WRITE setViewRect NOTIFY viewRectChanged FINAL)
    /*! Set the size of the navigable scrollbar area.
     *
     * Default to {-1000, -750, 2000, 1500}.
     *
     * Setting a small viewRect will limit the area available to scroll the navigable
     * using scrollbars.
     * Setting a viewRect smaller that navigable childrenRect has no effect.
     * Mapped internally to virtualItem bounding rect.
     */
    QRectF          getViewRect() const;
    //! \copydoc getViewRect().
    void            setViewRect(const QRectF& viewRect);
signals:
    void            viewRectChanged();

public:
    //! Center the view on graph content center and set a 1.0 zoom.
    Q_INVOKABLE void    center();

    //! Center the view on a given child item (zoom level is not modified).
    Q_INVOKABLE void    centerOn(QQuickItem* item);

    //! Center the view on a given position
    Q_INVOKABLE void    centerOnPosition(QPointF position);

    //! Move to \c position (position will be be at top left corner).
    Q_INVOKABLE void    moveTo(QPointF position);

    /*! Fit the area content (\c containerItem children) in view and update current zoom level.
     *
     * Area content will be fitted in view even if current AutoFitMode is NoAutoFit.
     * \sa autoFitMode
     *
     * \note When fitting an invisible view, this method might be unable to
     * find real width/height, \c forceWidth and \c forceHeight might be
     * used to provide a valid size, < 0. are automatically ignored.
     */
    Q_INVOKABLE void    fitContentInView(qreal forceWidth = -1., qreal forceHeight = -1.);

signals:
    //! Navigable has bee modified following a user interaction (not emitted from programmatic modification).
    void    navigated();

public:
    //! \brief Auto fitting mode.
    enum AutoFitMode
    {
        //! No auto-fitting (default).
        NoAutoFit,
        /*! Auto fit area content when the area is resized without user modified zoom or pan, and eventually auto center content.
         *
         * Setting \c autoFitMode to \c AutoFit has the following effects:
         * \li Content will be fitted automatically when the view is resized if user has not already applied a custom zoom
         * or pan.
         * \li If the user has applied a custom zoom or pan, the content will be automatically centered if the transformed
         * container item is smaller than navigable area.
         * \li Content will be anchored to the left or the right view borders when its width is larger than the navigable area, but there
         * is a space "in panning" between content and view border.
         */
        AutoFit
    };
    Q_ENUM(AutoFitMode)

    /*! \brief Current auto-fit mode (default to NoAutoFit).    */
    Q_PROPERTY(AutoFitMode autoFitMode READ getAutoFitMode WRITE setAutoFitMode NOTIFY autoFitModeChanged FINAL)
    //! \sa autoFitMode
    AutoFitMode getAutoFitMode() const { return _autoFitMode; }
    //! \sa autoFitMode
    void        setAutoFitMode(AutoFitMode autoFitMode);
private:
    //! \copydoc autoFitMode
    AutoFitMode _autoFitMode{NoAutoFit};
signals:
    //! \sa autoFitMode
    void        autoFitModeChanged();
private:
    //! Flag set to true if area panning has been modified since the last fitContentInView() call.
    bool        _panModified = false;
    //! Flag set to true if area zoom has been modified since the last fitContentInView() call.
    bool        _zoomModified = false;

public:
    /*! \brief Zoom incrementation delta (default to 0.05,ie 5%).
     */
    Q_PROPERTY(qreal zoomIncrement READ getZoomIncrement WRITE setZoomIncrement NOTIFY zoomIncrementChanged FINAL)
    //! \sa zoomIncrement
    qreal       getZoomIncrement() const { return _zoomIncrement; }
    //! \sa zoomIncrement
    void        setZoomIncrement(qreal zoomIncrement) noexcept { _zoomIncrement = zoomIncrement; emit zoomIncrementChanged(); }
private:
    //! \copydoc zoomIncrement
    qreal       _zoomIncrement = 0.05;
signals:
    //! \sa zoomIncrement
    void        zoomIncrementChanged();

public:
    /*! \brief Area current zoom level (default to 1.0).
     *
     * Setting a zoom value outside the (minZoom, maxZoom) range has no effect.
     *
     * Accessing zoom directly from QML is safe, since setZoom() is protected against binding loops.
     */
    Q_PROPERTY(qreal zoom READ getZoom WRITE setZoom NOTIFY zoomChanged FINAL)
    //! \sa zoom
    inline qreal        getZoom() const noexcept { return _zoom; }
    /*! \brief Set navigable area current zoom (zoom is applied on current \c zoomOrigin).
     *
     * \note To avoid QML binding loops, this setter is protected against setting the same value multiple times.
     * \sa zoom
     */
    Q_INVOKABLE void    setZoom(qreal zoom);
    //! Set area current zoom centered on a given \c center point.
    Q_INVOKABLE void    zoomOn(QPointF center, qreal zoom);
    //! Return true if zoom is valid (ie it is different from the actual zoom and in the (minZoom, maxZoom) range.
    bool                isValidZoom(qreal zoom) const;
private:
    //! \copydoc zoom
    qreal       _zoom = 1.0;
signals:
    //! \sa zoom
    void        zoomChanged();

public:
    /*! \brief Origin point where any zoom set view setZoom or \c zoom will be applied (default to QQuickItem::Center).
     */
    Q_PROPERTY(QQuickItem::TransformOrigin zoomOrigin READ getZoomOrigin WRITE setZoomOrigin NOTIFY zoomOriginChanged FINAL)
    //! \sa zoomOrigin
    QQuickItem::TransformOrigin       getZoomOrigin() const noexcept { return _zoomOrigin; }
    /*! \brief Set navigable area current zoom origin (either QQuickItem::TopLeft or QQuickItem::Center).
     *
     * \note Zooming initiated via mouse wheel is always applied at current mouse position.
     * \sa zoom
     */
    void        setZoomOrigin(QQuickItem::TransformOrigin zoomOrigin);
private:
    //! \copydoc zoomOrigin
    QQuickItem::TransformOrigin _zoomOrigin = QQuickItem::Center;
signals:
    //! \sa zoomOrigin
    void        zoomOriginChanged();

public:
    //! Area maximum zoom level (-1 = no maximum zoom limitation, 1.0 = no zoom allowed, >1.0 = zoomMax*100% maximum zoom).
    Q_PROPERTY(qreal zoomMax READ getZoomMax WRITE setZoomMax NOTIFY zoomMaxChanged FINAL)
    //! \sa zoomMax
    qreal       getZoomMax() const noexcept { return _zoomMax; }
    //! \sa zoomMax
    void        setZoomMax(qreal zoomMax);
private:
    //! \copydoc zoomMax
    qreal       _zoomMax = -1.0;
signals:
    //! \sa zoomMax
    void        zoomMaxChanged();

public:
    //! Area minimum zoom level, default to 0.1 (10% zoom), zoomMin can't be <= 0.
    Q_PROPERTY(qreal zoomMin READ getZoomMin WRITE setZoomMin NOTIFY zoomMinChanged FINAL)
    //! \sa zoomMin
    qreal       getZoomMin() const noexcept { return _zoomMin; }
    //! \sa zoomMin
    void        setZoomMin(qreal zoomMin);
private:
    //! \copydoc zoomMin
    qreal       _zoomMin = 0.09;  // Max 10% zoom with default zoom in/out thresold
signals:
    //! \sa zoomMin
    void        zoomMinChanged();

signals:
    //! Emitted whenever the mouse is clicked in the container.
    void    clicked(QVariant pos);

    //! Emitted whenever the mouse is right clicked in the container.
    void    rightClicked(QVariant pos);

    //! Emitted whenever the container item is scaled (zoomed) or panned.
    void    containerItemModified();

protected:
    //! Called when the mouse is clicked in the container (base implementation empty).
    virtual void    navigableClicked(QPointF pos, QPointF globalPos) { Q_UNUSED(pos); }
    //! Called when the mouse is right clicked in the container (base implementation empty).
    virtual void    navigableRightClicked(QPointF pos, QPointF globalPos) { Q_UNUSED(pos); }
    //! Called when the container item is scaled (zoomed) or panned (base implementation empty).
    virtual void    navigableContainerItemModified() { }

public:
    //! True when the navigable conctent area is actually dragged.
    Q_PROPERTY(bool dragActive READ getDragActive WRITE setDragActive NOTIFY dragActiveChanged FINAL)
    //! \copydoc dragActive
    inline bool getDragActive() const noexcept { return _dragActive; }
    //! \copydoc dragActive
    void        setDragActive(bool dragActive) noexcept;
private:
    //! \copydoc dragActive
    bool        _dragActive{false};
signals:
    //! \copydoc dragActive
    void        dragActiveChanged();

protected:
    virtual void    geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    virtual void    mouseMoveEvent(QMouseEvent* event) override;
    virtual void    mousePressEvent(QMouseEvent* event) override;
    virtual void    mouseReleaseEvent(QMouseEvent* event) override;
    virtual void    wheelEvent(QWheelEvent* event) override;

protected:
    bool        _leftButtonPressed = false;
    QPointF     _lastPan{};
    //@}
    //-------------------------------------------------------------------------


    /*! \name Selection Rectangle Management *///------------------------------
    //@{
public:
    /*! \brief Enable or disable visual selection rect (default to true, ie selection is enabled).
     */
    Q_PROPERTY(bool selectionRectEnabled READ getSelectionRectEnabled WRITE setSelectionRectEnabled NOTIFY selectionRectEnabledChanged FINAL)
    //! \sa selectionRectEnabled
    inline bool     getSelectionRectEnabled() const noexcept { return _selectionRectEnabled; }
    //! \sa selectionRectEnabled
    void            setSelectionRectEnabled(bool selectionRectEnabled) noexcept;
private:
    //! \copydoc selectionRectEnabled
    bool            _selectionRectEnabled = true;
signals:
    //! \sa selectionRectEnabled
    void            selectionRectEnabledChanged();

public:
    /*! \brief Selection rectangle is activated instead of view panning when CTRL key is used while clicking and dragging.
     *
     * Selection rectangle can be any QQuickItem* but is usually a Rectangle component initialized in a concrete component or subclass.
     * Selection rectangle is not owned by this qan::Navigable object, it might be a QML owned object.
     */
    Q_PROPERTY(QQuickItem* selectionRectItem READ getSelectionRectItem WRITE setSelectionRectItem NOTIFY selectionRectChanged FINAL)
    //! \copydoc selectionRectItem
    inline QQuickItem*  getSelectionRectItem() { return _selectionRectItem; }
    //! \copydoc selectionRectItem
    void                setSelectionRectItem(QQuickItem* selectionRectItem);
private:
    //! \copydoc selectionRectItem
    QPointer<QQuickItem>    _selectionRectItem = nullptr;
signals:
    void    selectionRectChanged();

private:
    //! \copydoc selectionRectItem
    bool        _ctrlLeftButtonPressed = false;

    //! \copydoc selectionRectItem
    bool        _selectRectActive = false;

    //! \copydoc selectionRectItem
    QPointF     _lastSelectRect{};
    QPointF     _startSelectRect{};

protected:
    //! Called when the selectionRectItem is activated, ie it's geometry has changed, \c rect is in containerItem space.
    virtual void    selectionRectActivated(const QRectF& rect);

    //! Called when the selectionRectItem interaction ends.
    virtual void    selectionRectEnd();
    //@}
    //-------------------------------------------------------------------------


    /*! \name Grid Management *///---------------------------------------------
    //@{
public:
    /*! \brief User defined background grid.
     *
     * Grid is automatically updated on zoom/pan or navigable content view modification.
     *
     * \note may be nullptr (undefined in QML).
     */
    Q_PROPERTY(qan::Grid* grid READ getGrid WRITE setGrid NOTIFY gridChanged FINAL)
    //! \copydoc grid
    qan::Grid*          getGrid() noexcept { return _grid.data(); }
    const qan::Grid*    getGrid() const noexcept { return _grid.data(); }
    void                setGrid(qan::Grid* grid) noexcept;
private:
    //! Force update of grid.
    void                updateGrid() noexcept;
    //! \copydoc grid
    QPointer<qan::Grid> _grid;

    std::unique_ptr<qan::Grid>   _defaultGrid;
signals:
    //! \copydoc grid
    void                gridChanged();
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::Navigable)
