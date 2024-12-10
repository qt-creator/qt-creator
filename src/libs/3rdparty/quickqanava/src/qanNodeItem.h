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
// \file	qanNodeItem.h
// \author	benoit@destrat.io
// \date	2016 03 04
//-----------------------------------------------------------------------------

#pragma once

// Std headers
#include <cstddef>  // std::size_t
#include <array>

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>
#include <QDrag>
#include <QPointer>

// QuickQanava headers
#include "./qanStyle.h"
#include "./qanNode.h"
#include "./qanSelectable.h"
#include "./qanDraggable.h"
#include "./qanAbstractDraggableCtrl.h"

namespace qan { // ::qan

class Node;
class Graph;

/*! \brief Base class for modelling nodes with attributes and an in/out edges list in a qan::Graph graph.
 *
 * \note If your application does not need drag'n'drop support (ie group insertion via dra'n'drop or VisualConnector are not used nor necessary), consider disabling
 * drag'n'drop support by setting the \c acceptsDrops and \c droppable properties to false, it could improve performances significantly.
 *
 * Bounding shape generation could be done in the following two ways:
 * - Using the default behavior for rectangular node with \c complexBoundingShape set to false (default value), bounding
 * shape is automatically generated on node \c width or \c height change in \c generateDefaultBoundingShape().
 * - Using dedicated code by setting \c complexBoundingShape to true and with a call to \c setBoundingShape() from a custom
 * \c onRequestUpdateBoundingShape() signal handler.
 *
 * Signal \c requestUpdateBoundingShape won't be emitted for non complex bounding shape.
 *
 * Optionally, you could choose to set \c complexBoundingShape to false and override \c generateDefaultBoundingShape() method.
 *
 * \warning NodeItem \c objectName property is set to "qan::NodeItem" and should not be changed in subclasses.
 *
 * \nosubgrouping
 */
class NodeItem : public QQuickItem,
                 public qan::Selectable,
                 public qan::Draggable
{
    /*! \name Node Object Management *///--------------------------------------
    //@{
    Q_OBJECT
    Q_INTERFACES(qan::Selectable)
    Q_INTERFACES(qan::Draggable)
    QML_ELEMENT
public:
    //! Node constructor.
    explicit NodeItem(QQuickItem* parent = nullptr);
    virtual ~NodeItem() override;
    NodeItem(const NodeItem&) = delete;
    NodeItem& operator=(const NodeItem&) = delete;
    NodeItem(NodeItem&&) = delete;
    NodeItem& operator=(NodeItem&&) = delete;
    //@}
    //-------------------------------------------------------------------------


    /*! \name Topology Management *///-----------------------------------------
    //@{
public:
    Q_PROPERTY(qan::Node* node READ getNode CONSTANT FINAL)
    auto        getNode() noexcept -> qan::Node*;
    auto        getNode() const noexcept -> const qan::Node*;
    auto        setNode(qan::Node* node) noexcept -> void;
private:
    QPointer<qan::Node> _node{nullptr};

public:
    //! Secure shortcut to getNode().getGraph().
    Q_PROPERTY(qan::Graph* graph READ getGraph CONSTANT)
    //! \copydoc graph
    auto    setGraph(qan::Graph* graph) -> void;
protected:
    auto    getGraph() const -> const qan::Graph*;
    auto    getGraph() -> qan::Graph*;
private:
    QPointer<qan::Graph>    _graph;

public:
    //! Node minimum size, default to "100 x 45" (node could not be visually resized below this size if \c resizable property is true).
    Q_PROPERTY(QSizeF minimumSize READ getMinimumSize WRITE setMinimumSize NOTIFY minimumSizeChanged FINAL)
    //! \copydoc minimumSize
    const QSizeF&   getMinimumSize() const noexcept { return _minimumSize; }
    //! \copydoc minimumSize
    bool            setMinimumSize(QSizeF minimumSize) noexcept;
private:
    QSizeF          _minimumSize{100., 45.};
signals:
    //! \internal
    void            minimumSizeChanged();

public:
    //! Utility function to ease initialization from c++, call setX(), setY(), setWidth() and setHEight() with the content of \c rect bounding rect.
    auto            setRect(const QRectF& r) noexcept -> void;    
    //@}
    //-------------------------------------------------------------------------


    /*! \name Collapse Management *///-----------------------------------------
    //@{
public:
    //! \brief True when the node (usually a group) is collapsed (content of a collapsed group is hidden, leaving just an header bar with a +/- collapse control).
    Q_PROPERTY(bool collapsed READ getCollapsed WRITE setCollapsed NOTIFY collapsedChanged FINAL)
    inline bool     getCollapsed() const noexcept { return _collapsed; }
    virtual void    setCollapsed(bool collapsed) noexcept;
private:
    bool        _collapsed = false;
signals:
    void        collapsedChanged();
public:
    Q_INVOKABLE virtual void    collapseAncestors(bool collapsed = true);
    Q_INVOKABLE virtual void    collapseChilds(bool collapsed = true);
    //@}
    //-------------------------------------------------------------------------


    /*! \name Selection Management *///----------------------------------------
    //@{
public:
    //! Set this property to false to disable node selection (default to true, ie node are selectable by default).
    Q_PROPERTY(bool selectable READ getSelectable WRITE setSelectable NOTIFY selectableChanged FINAL)
    Q_PROPERTY(bool selected READ getSelected WRITE setSelected NOTIFY selectedChanged FINAL)
    //! \brief Item used to hilight selection (usually a Rectangle quick item).
    Q_PROPERTY(QQuickItem* selectionItem READ getSelectionItem WRITE setSelectionItem NOTIFY selectionItemChanged FINAL)
protected:
    virtual void    emitSelectableChanged() override { emit selectableChanged(); }
    virtual void    emitSelectedChanged() override { emit selectedChanged(); }
    virtual void    emitSelectionItemChanged() override { emit selectionItemChanged(); }
signals:
    void            selectableChanged();
    void            selectedChanged();
    void            selectionItemChanged();

protected slots:
    virtual void    onWidthChanged();
    virtual void    onHeightChanged();
    //@}
    //-------------------------------------------------------------------------


    /*! \name Node Configuration *///------------------------------------------
    //@{
public:
    //! Enable or disable node item resizing (default to true, ie node is resizable).
    Q_PROPERTY(bool resizable READ getResizable WRITE setResizable NOTIFY resizableChanged FINAL)
    //! \copydoc resizable
    inline bool     getResizable() const noexcept { return _resizable; }
    //! \copydoc resizable
    void            setResizable(bool resizable) noexcept;
protected:
    //! \copydoc resizable
    bool            _resizable = true;
signals:
    //! \copydoc resizable
    void            resizableChanged();

public:
    /*! \brief Set the node "allowed" resizing ratio when visual resizing is enabled with \c resizable (default to -1.).
     *
     * Ratio is witdh / height ratio that is allowed for visual resizing operations.
     *
     * Ration conservation is disabled if \c ration is < 0.
     */
    Q_PROPERTY(qreal ratio READ getRatio WRITE setRatio NOTIFY ratioChanged FINAL)
    //! \copydoc ratio
    inline qreal    getRatio() const noexcept { return _ratio; }
    //! \copydoc ratio
    void            setRatio(qreal ratio) noexcept;
protected:
    //! \copydoc ratio
    qreal           _ratio = -1.;
signals:
    //! \copydoc ratio
    void            ratioChanged();

public:
    //! Define how the GraphView VisualConnector interact with this node.
    enum class Connectable {
        //! Node is fully visually connectable.
        Connectable,
        //! Node is visually connectable only from another node.
        InConnectable,
        //! Node can only be connected visually to another node.
        OutConnectable,
        //! Node cannot be visually connected from another node.
        UnConnectable,
    };
    Q_ENUM(Connectable)

    /*! \brief Enable or disable visual connection for a specific node (default to Connectable, ie visually connectable).
     *
     * \note When set to InConnectable or Unconnectable, visual node connector will not appear when the node
     * is clicked. Connecting node programmatically is still possible with qan::Graph::insertEdge().
     *
     * \note Disabling visual node connection will not prevent visual connection
     * from/to node ports.
     *
     * \sa Connectable
     */
    Q_PROPERTY(Connectable connectable READ getConnectable WRITE setConnectable NOTIFY connectableChanged FINAL)
    //! \copydoc connectable
    inline Connectable  getConnectable() const noexcept { return _connectable; }
    //! \copydoc connectable
    void            setConnectable(Connectable connectable) noexcept;
protected:
    //! \copydoc connectable
    Connectable     _connectable = Connectable::Connectable;
signals:
    //! \copydoc connectable
    void            connectableChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Draggable Management *///----------------------------------------
    //@{
public:
    qan::AbstractDraggableCtrl&                 draggableCtrl();
protected:
    std::unique_ptr<qan::AbstractDraggableCtrl> _draggableCtrl;
public:
    //! \copydoc qan::Draggable::_draggable
    Q_PROPERTY(bool draggable READ getDraggable WRITE setDraggable NOTIFY draggableChanged FINAL)
    //! \copydoc qan::Draggable::_dragged
    Q_PROPERTY(bool dragged READ getDragged WRITE setDragged NOTIFY draggedChanged FINAL)
    //! \copydoc qan::Draggable::_dropable
    Q_PROPERTY(bool droppable READ getDroppable WRITE setDroppable NOTIFY droppableChanged FINAL)
    //! \copydoc qan::Draggable::_acceptDrops
    Q_PROPERTY(bool acceptDrops READ getAcceptDrops WRITE setAcceptDrops NOTIFY acceptDropsChanged FINAL)

protected:
    virtual void    emitDraggableChanged() override { emit draggableChanged(); }
    virtual void    emitDraggedChanged() override { emit draggedChanged(); }
    virtual void    emitAcceptDropsChanged() override { emit acceptDropsChanged(); }
    virtual void    emitDroppableChanged() override { emit droppableChanged(); }

signals:
    void            draggableChanged();
    void            draggedChanged();
    void            droppableChanged();
    void            acceptDropsChanged();

public:
    //! Define an orientation contrain on node dragging.
    enum class DragOrientation: unsigned int {
        DragAll         = 0,  //! All is no constrain on dragging: drag in all directions
        DragVertical    = 1,  //! Drag only horizontally
        DragHorizontal  = 2   //! Drag only vertically
    };
    Q_ENUM(DragOrientation)
    //! \copydoc getDragOrientation()
    Q_PROPERTY(DragOrientation dragOrientation READ getDragOrientation WRITE setDragOrientation NOTIFY dragOrientationChanged FINAL)
    //! \copydoc getDragOrientation()
    bool                    setDragOrientation(DragOrientation dragOrientation) noexcept;
    //! Define a constrain on node/group dragging, default to no constrain, set to horizontal or vertical to constrain dragging on one orientation.
    DragOrientation         getDragOrientation() const noexcept { return _dragOrientation; }
protected:
    //! \copydoc getDragOrientation()
    DragOrientation         _dragOrientation = DragOrientation::DragAll;
signals:
    //! \copydoc getDragOrientation()
    void                    dragOrientationChanged();

protected:
    //! Internally used to manage drag and drop over nodes, override with caution, and call base class implementation.
    virtual void    dragEnterEvent(QDragEnterEvent* event) override;
    //! Internally used to manage drag and drop over nodes, override with caution, and call base class implementation.
    virtual void    dragMoveEvent(QDragMoveEvent* event) override;
    //! Internally used to manage drag and drop over nodes, override with caution, and call base class implementation.
    virtual void    dragLeaveEvent(QDragLeaveEvent* event) override;
    //! Internally used to accept style drops.
    virtual void    dropEvent(QDropEvent* event) override;

    virtual void    mouseDoubleClickEvent(QMouseEvent* event) override;
    virtual void    mouseMoveEvent(QMouseEvent* event) override;
    virtual void    mousePressEvent(QMouseEvent* event) override;
    virtual void    mouseReleaseEvent(QMouseEvent* event) override;
    //@}
    //-------------------------------------------------------------------------


    /*! \name Style Management *///--------------------------------------------
    //@{
public:
    //! Node current style (this property is never null, a default style is returned when no style has been manually set).
    Q_PROPERTY(qan::NodeStyle* style READ getStyle WRITE setStyle NOTIFY styleChanged FINAL)
    void                        setStyle(qan::NodeStyle* style) noexcept;
    //! Generic interface for qan::DraggableCtrl<>::handleDropEvent().
    void                        setItemStyle(qan::Style* style) noexcept;
    //! \copydoc style
    inline qan::NodeStyle*      getStyle() const noexcept { return _style.data(); }
private:
    //! \copydoc style
    QPointer<qan::NodeStyle>    _style;
signals:
    //! \copydoc style
    void                        styleChanged();
private slots:
    //! Called when this node style is destroyed, remove any existing binding.
    void                        styleDestroyed(QObject* style);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Intersection Shape Management *///-------------------------------
    //@{
signals:
    //! Emitted whenever the node is clicked (even at the start of a dragging operation).
    void    nodeClicked(qan::NodeItem* node, QPointF p);
    //! Emitted whenever the node is double clicked.
    void    nodeDoubleClicked(qan::NodeItem* node, QPointF p);
    //! Emitted whenever the node is right clicked.
    void    nodeRightClicked(qan::NodeItem* node, QPointF p);

public:
    /*! \brief Polygon used for mouse event clipping, and edge arrow clipping (in item local coordinates).
     *
     * An intersection shape is automatically generated for rectangular nodes, it can be sets by the user
     * manually with setIntersectionShape() or setIntersectionShapeFromQml() if the node graphical representation is
     * not a rectangle.
     * \sa \ref custom
     */
    Q_PROPERTY(QPolygonF boundingShape READ getBoundingShape WRITE setBoundingShape NOTIFY boundingShapeChanged FINAL)
    QPolygonF           getBoundingShape();
    Q_INVOKABLE void    setBoundingShape(const QPolygonF& boundingShape);

public slots:
    //! Generate a default bounding shape (rounded rectangle) and set it as current bounding shape.
    Q_INVOKABLE void    setDefaultBoundingShape();
signals:
    void                boundingShapeChanged();
    //! signal is Emitted when the bounding shape become invalid and should be regenerated from QML.
    void                requestUpdateBoundingShape();
public:
    QPolygonF           generateDefaultBoundingShape() const;
private:
    QPolygonF           _boundingShape;
protected:
    /*! \brief Invoke this method from a concrete node component in QML for non rectangular nodes.
     * \code
     * // In a component "inheriting" from QanNode:
     *
     * // Define a property in your component
     * property var polygon: new Array( )
     *
     * // In Component.onCompleted():
     * polygon.push( Qt.point( 0, 0 ) )
     * polygon.push( Qt.point( 10, 0 ) )
     * polygon.push( Qt.point( 10, 10 ) )
     * polygon.push( Qt.point( 0, 10 ) )
     * polygon.push( Qt.point( 0, 0 ) )
     * setBoundingShapeFrom( polygon );
     * \endcode
     *
     * \sa isInsideBoundingShape()
     */
    Q_INVOKABLE virtual void    setBoundingShape(QVariantList boundingPolygon);

    /*! \brief Test if a point in the node local CCS is inside the current intersection shape.
     *
     * Usefull to accept or reject mouse drag event in QML custom node components.
     * \code
     *  // In the MouseArea component used to drag your node component (with drag.target sets correctly):
     *  onPressed : {
     *   mouse.accepted = ( isInsideBoundingShape( Qt.point( mouse.x, mouse.y ) ) ? true : false )
     *  }
     * \endcode
     *  \sa setBoundShapeFrom()
     */
    Q_INVOKABLE virtual bool    isInsideBoundingShape(QPointF p);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Port/Dock Layout Management *///---------------------------------
    //@{
public:
    using PortItems = qcm::Container<QVector, QQuickItem*>;    // Using QQuickItem instead of qan::PortItem because MSVC15 does not fully support complete c++14 forward declarations

    //! Look for a port with a given \c id (or nullptr if no such port exists).
    Q_INVOKABLE qan::PortItem*  findPort(const QString& portId) const noexcept;

    //! Read-only list model of this node ports (either in or out).
    Q_PROPERTY(QAbstractListModel* ports READ getPortsModel CONSTANT FINAL)
    QAbstractListModel* getPortsModel() { return qobject_cast<QAbstractListModel*>(_ports.getModel()); }
    inline auto         getPorts() noexcept -> PortItems& { return _ports; }
    inline auto         getPorts() const noexcept -> const PortItems& { return _ports; }

    //! Force update of all node ports edges (#145).
    Q_INVOKABLE void    updatePortsEdges();
private:
    PortItems           _ports;

public:
    //! Define port dock type/index/position.
    enum class Dock : unsigned int {
        Left    = 0,
        Top     = 1,
        Right   = 2,
        Bottom  = 3
    };
    Q_ENUM(Dock)

public:
    //! Item left dock (usually build from qan::Graph::verticalDockDelegate).
    Q_PROPERTY(QQuickItem* leftDock READ getLeftDock WRITE setLeftDock NOTIFY leftDockChanged FINAL)
    //! \copydoc leftDock
    void                    setLeftDock(QQuickItem* leftDock) noexcept;
    //! \copydoc leftDock
    inline QQuickItem*      getLeftDock() noexcept { return _dockItems[static_cast<std::size_t>(Dock::Left)].data(); }
signals:
    //! \copydoc leftDock
    void                    leftDockChanged();

public:
    //! Item top dock (usually build from qan::Graph::horizontalDockDelegate).
    Q_PROPERTY(QQuickItem* topDock READ getTopDock WRITE setTopDock NOTIFY topDockChanged FINAL)
    //! \copydoc topDock
    void                    setTopDock(QQuickItem* topDock) noexcept;
    //! \copydoc topDock
    inline QQuickItem*      getTopDock() noexcept { return _dockItems[static_cast<std::size_t>(Dock::Top)].data(); }
signals:
    //! \copydoc topDock
    void                    topDockChanged();

public:
    //! Item right dock (usually build from qan::Graph::verticalDockDelegate).
    Q_PROPERTY(QQuickItem* rightDock READ getRightDock WRITE setRightDock NOTIFY rightDockChanged FINAL)
    //! \copydoc rightDock
    void                    setRightDock(QQuickItem* rightDock) noexcept;
    //! \copydoc rightDock
    inline QQuickItem*      getRightDock() noexcept { return _dockItems[static_cast<std::size_t>(Dock::Right)].data(); }
signals:
    //! \copydoc rightDock
    void                    rightDockChanged();

public:
    //! Item bottom dock (usually build from qan::Graph::horizontalDockDelegate).
    Q_PROPERTY(QQuickItem* bottomDock READ getBottomDock WRITE setBottomDock NOTIFY bottomDockChanged FINAL)
    //! \copydoc bottomDock
    void                    setBottomDock(QQuickItem* bottomDock) noexcept;
    //! \copydoc bottomDock
    inline QQuickItem*      getBottomDock() noexcept { return _dockItems[static_cast<std::size_t>(Dock::Bottom)].data(); }
signals:
    //! \copydoc bottomDock
    void                    bottomDockChanged();

public:
    //! Get \c dock dock item (warning: no bound checking, might return nullptr).
    QQuickItem*             getDock(Dock dock) noexcept { return _dockItems[static_cast<std::size_t>(dock)].data(); }

    //! Set \c dock dock item to \c dockItem.
    void                    setDock(Dock dock, QQuickItem* dockItem) noexcept;
protected:
    //! Set dock parent item, "hostNodeItem" and "dockType" properties.
    void                    configureDock(QQuickItem& dockItem, const Dock) noexcept;
private:
    static constexpr unsigned int dockCount = 4;
    std::array<QPointer<QQuickItem>, dockCount> _dockItems;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::NodeItem)
Q_DECLARE_METATYPE(qan::NodeItem::Connectable)
Q_DECLARE_METATYPE(qan::NodeItem::Dock)
