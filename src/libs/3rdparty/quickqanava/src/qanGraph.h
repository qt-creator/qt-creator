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
// \file	qanGraph.h
// \author	benoit@destrat.io
// \date	2004 February 15
//-----------------------------------------------------------------------------

#pragma once

#include "./gtpo/node.h"
#include "./gtpo/graph.h"

// Qt headers
#include <QString>
#include <QQuickItem>
#include <QQmlParserStatus>
#include <QSharedPointer>
#include <QAbstractListModel>

// QuickQanava headers
#include "./qanUtils.h"
#include "./qanStyleManager.h"
#include "./qanEdge.h"
#include "./qanNode.h"
#include "./qanGroup.h"
#include "./qanTableGroup.h"
#include "./qanNavigable.h"
#include "./qanSelectable.h"
#include "./qanConnector.h"


//! Main QuickQanava namespace
namespace qan { // ::qan

class Graph;
class GraphView;
class Node;
class Connector;
class PortItem;

/*! \brief Main interface to manage graph topology.
 *
 * Visual connection of nodes:
 *   \li Visual connection of nodes with VisualConnector is enabled by setting the \c connectorEnabled property to \c true (default to true).
 *   \li When an edge is created with the visual node connector, the signal \c connectorEdgeInserted with the newly inserted \c edge as an argument.

 * \nosubgrouping
 */
class Graph : public gtpo::graph<QQuickItem, qan::Node, qan::Group, qan::Edge>
{
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)

    using super_t = gtpo::graph<QQuickItem, qan::Node, qan::Group, qan::Edge>;
    friend class qan::Selectable;

    /*! \name Graph Object Management *///-------------------------------------
    //@{
public:
    //! Graph default constructor.
    explicit Graph(QQuickItem* parent = nullptr) noexcept;
    /*! \brief Graph default destructor.
     *
     * Graph is a factory for inserted nodes and edges, even if they have been created trought
     * QML delegates, they will be destroyed with the graph they have been created in.
     */
    virtual ~Graph() override;
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&&) = delete;
    Graph& operator=(Graph&&) = delete;
public:
    virtual void    classBegin() override;

    //! QQmlParserStatus Component.onCompleted() overload to initialize default graph delegate in a valid QQmlEngine.
    virtual void    componentComplete() override;

public:
    //! \copydoc getGraphView()
    Q_PROPERTY(QQuickItem*  graphView READ qmlGetGraphView NOTIFY graphViewChanged FINAL)
    //! \copydoc getGraphView()
    QQuickItem*             qmlGetGraphView();
    //! qan::GrapvView where this graph is registered (a graph may have only one view).
    qan::GraphView*         getGraphView();
    //! \copydoc getGraphView()
    const qan::GraphView*   getGraphView() const;
    //! \copydoc getGraphView()
    void                    setGraphView(qan::GraphView* graphView);
signals:
    //! \copydoc getGraphView()
    void                    graphViewChanged();
private:
    //! \copydoc getGraphView()
    qan::GraphView*         _graphView = nullptr;

public:
    //! \copydoc getContainerItem()
    Q_PROPERTY(QQuickItem*      containerItem READ getContainerItem NOTIFY containerItemChanged FINAL)
    /*! \brief Quick item used as a parent for all graphics item "factored" by this graph (default to this).
     *
     * \note Container item should be initialized at startup, any change will _not_ be refelected to existing
     * graphics items.
     */
    inline QQuickItem*          getContainerItem() noexcept { return _containerItem.data(); }
    //! \copydoc getContainerItem()
    inline const QQuickItem*    getContainerItem() const noexcept { return _containerItem.data(); }
    //! \copydoc getContainerItem()
    void                        setContainerItem(QQuickItem* containerItem);
signals:
    //! \copydoc getContainerItem()
    void                        containerItemChanged();
private:
    //! \copydoc getContainerItem()
    QPointer<QQuickItem>        _containerItem;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Management *///--------------------------------------------
    //@{
public:
    /*! \brief Clear this graph topology and styles.
     *
     */
    Q_INVOKABLE virtual void    clearGraph() noexcept;

    /*! \brief If unsure, use clearGraph() to clear the graph.
     *
     * Warning: clear() is not virtual, ensure you are calling clear() after a correct cast to the terminal
     * graph type you are targetting.
     */
    void                        clear() noexcept;

public:
    /*! \brief Similar to QQuickItem::childAt() method, except that it take edge bounding shape into account.
     *
     * Using childAt() method will most of the time return qan::Edge items since childAt() use bounding boxes
     * for item detection.
     *
     * \return nullptr if there is no child at requested position, or a QQuickItem that can be casted qan::Node, qan::Edge or qan::Group with qobject_cast<>.
     */
    Q_INVOKABLE QQuickItem* graphChildAt(qreal x, qreal y) const;

    /*! \brief Similar to QQuickItem::childAt() method, except that it only take groups into account (and is hence faster, but still O(n)).
     *
     * \arg except Return every compatible group except \c except (can be nullptr).
     */
    Q_INVOKABLE qan::Group* groupAt(const QPointF& p, const QSizeF& s, const QQuickItem* except = nullptr) const;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Visual connection Management *///--------------------------------
    //@{
public:
    /*! \brief Set the visual connector source node.
     *
     * \note If \c sourceNode is nullptr, visual connector is hidden.
     */
    Q_INVOKABLE void    setConnectorSource(qan::Node* sourceNode) noexcept;
signals:
    //! \copydoc hlg::Connector::requestEdgeCreation
    void                connectorRequestEdgeCreation(qan::Node* src, QObject* dst,
                                                     qan::PortItem* srcPortItem, qan::PortItem* dstPortItem);
    //! \copydoc hlg::Connector::edgeInserted
    void                connectorEdgeInserted(qan::Edge* edge);

public:
    //! Alias to VisualConnector::edgeColor property (default to Black).
    Q_PROPERTY(QColor connectorEdgeColor READ getConnectorEdgeColor WRITE setConnectorEdgeColor NOTIFY connectorEdgeColorChanged FINAL)
    QColor      getConnectorEdgeColor() const noexcept { return _connectorEdgeColor; }
    void        setConnectorEdgeColor(QColor connectorEdgeColor) noexcept;
signals:
    void        connectorEdgeColorChanged();
private:
    QColor      _connectorEdgeColor{Qt::black};

public:
    //! Alias to VisualConnector::connectorColor property (default to DodgerBlue).
    Q_PROPERTY(QColor connectorColor READ getConnectorColor WRITE setConnectorColor NOTIFY connectorColorChanged FINAL)
    inline QColor   getConnectorColor() const noexcept { return _connectorColor; }
    void            setConnectorColor(QColor connectorColor) noexcept;
signals:
    void            connectorColorChanged();
private:
    QColor          _connectorColor{30, 144, 255};  // dodgerblue=rgb(30, 144, 255)

public:
    //! Alias to VisualConnector::createDefaultEdge (default to true).
    Q_PROPERTY(bool connectorCreateDefaultEdge READ getConnectorCreateDefaultEdge WRITE setConnectorCreateDefaultEdge NOTIFY connectorCreateDefaultEdgeChanged FINAL)
    inline bool     getConnectorCreateDefaultEdge() const noexcept { return _connectorCreateDefaultEdge; }
    void            setConnectorCreateDefaultEdge(bool connectorCreateDefaultEdge) noexcept;
signals:
    void            connectorCreateDefaultEdgeChanged();
private:
    bool            _connectorCreateDefaultEdge = true;

public:
    //! Alias to qan::Connector::connectorItem property (default to nullptr, ie default connector item).
    Q_PROPERTY(QQuickItem* connectorItem READ getConnectorItem WRITE setConnectorItem NOTIFY connectorItemChanged FINAL)
    inline QQuickItem*      getConnectorItem() const noexcept { return _connectorItem; }
    void                    setConnectorItem( QQuickItem* connectorItem ) noexcept;
signals:
    void                    connectorItemChanged();
private:
    QPointer<QQuickItem>    _connectorItem{nullptr};

public:
    //! Enable or disable visual connector of nodes in the graph (default to false).
    Q_PROPERTY(bool connectorEnabled READ getConnectorEnabled WRITE setConnectorEnabled NOTIFY connectorEnabledChanged FINAL)
    inline bool     getConnectorEnabled() const noexcept { return _connectorEnabled; }
    void            setConnectorEnabled(bool connectorEnabled) noexcept;
signals:
    void            connectorEnabledChanged();
private:
    bool            _connectorEnabled = false;

public:
    //! Control node used as a connector when \c connectorEnabled is set to true (might be nullptr).
    Q_PROPERTY(qan::Connector* connector READ getConnector NOTIFY connectorChanged FINAL)
    qan::Connector*             getConnector() noexcept;
private:
    QScopedPointer<qan::Connector> _connector{};
signals:
    void                        connectorChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Delegates Management *///----------------------------------------
    //@{
public:
    //! Default delegate for qan::Node and Qan.Node nodes.
    Q_PROPERTY(QQmlComponent* nodeDelegate READ getNodeDelegate WRITE setNodeDelegate NOTIFY nodeDelegateChanged FINAL)
    inline QQmlComponent*   getNodeDelegate() noexcept { return _nodeDelegate.get(); }
protected:
    void                    setNodeDelegate(QQmlComponent* nodeDelegate) noexcept;
    void                    setNodeDelegate(std::unique_ptr<QQmlComponent> nodeDelegate) noexcept;
signals:
    void                    nodeDelegateChanged();
private:
    std::unique_ptr<QQmlComponent> _nodeDelegate;

public:
    //! Default delegate for qan::Edge and Qan.Edge edges.
    Q_PROPERTY(QQmlComponent* edgeDelegate READ getEdgeDelegate WRITE setEdgeDelegate NOTIFY edgeDelegateChanged FINAL)
    inline QQmlComponent*   getEdgeDelegate() noexcept { return _edgeDelegate.get(); }
protected:
    void                    setEdgeDelegate(QQmlComponent* edgeDelegate) noexcept;
    void                    setEdgeDelegate(std::unique_ptr<QQmlComponent> edgeDelegate) noexcept;
signals:
    void                    edgeDelegateChanged();
private:
    std::unique_ptr<QQmlComponent> _edgeDelegate;

public:
    //! Default delegate for qan::Group and Qan.Group groups.
    Q_PROPERTY(QQmlComponent* groupDelegate READ getGroupDelegate WRITE setGroupDelegate NOTIFY groupDelegateChanged FINAL)
    inline QQmlComponent*   getGroupDelegate() noexcept { return _groupDelegate.get(); }
protected:
    void                    setGroupDelegate(QQmlComponent* groupDelegate) noexcept;
    void                    setGroupDelegate(std::unique_ptr<QQmlComponent> groupDelegate) noexcept;
signals:
    void                    groupDelegateChanged();
private:
    std::unique_ptr<QQmlComponent> _groupDelegate;

protected:
    //! Create a _styleable_ graph primitive using the given delegate \c component with either a source \c node or \c edge.
    QQuickItem*             createFromComponent(QQmlComponent* component,
                                                qan::Style& style,
                                                qan::Node* node = nullptr,
                                                qan::Edge* edge = nullptr,
                                                qan::Group* group = nullptr ) noexcept;

    //! Shortcut to createComponent(), mainly used in Qan.StyleList View to generate item for style pre visualization.
    Q_INVOKABLE QQuickItem* createFromComponent(QQmlComponent* component, qan::Style* style);

public:
    /*! \brief QML component used to create qan::NodeItem or qan::GroupItem \c selectionItem, could be dynamically changed from either c++ or QML.
     *
     *  \note Using setSelectionDelegate(nullptr) from c++ or Qan.Graph.selectionDelegate=null from QML is valid, QuickQanava will
     *  default to a basic selection item delegate.
     */
    Q_PROPERTY(QQmlComponent* selectionDelegate READ getSelectionDelegate WRITE setSelectionDelegate NOTIFY selectionDelegateChanged FINAL)
    //! \copydoc selectionDelegate
    inline QQmlComponent*   getSelectionDelegate() noexcept { return _selectionDelegate.get(); }
protected:
    //! \copydoc selectionDelegate
    void                    setSelectionDelegate(QQmlComponent* selectionDelegate) noexcept;
    //! \copydoc selectionDelegate
    void                    setSelectionDelegate(std::unique_ptr<QQmlComponent> selectionDelegate) noexcept;
signals:
    //! \copydoc selectionDelegate
    void                    selectionDelegateChanged();
public: // should be considered private
    /*! \brief Create a concrete QQuickItem using the current \c selectionDelegate (private API).
     *
     * \arg parent Returned selection item is automatically reparented to \c parent (could be nullptr).
     * \return A selection item or nullptr if graph \c selectionDelegate is invalid, ownershipd goes to the caller with QmlEngine::CppOwnership, might be nullptr.
     */
    QQuickItem*             createSelectionItem(QQuickItem* parent);
protected:

    struct QObjectDeleteLater {
        void operator()(QObject *o) {
            o->deleteLater();
        }
    };
    template<typename T>
    using unique_qptr = std::unique_ptr<T, QObjectDeleteLater>;

    std::unique_ptr<QQmlComponent>  _selectionDelegate{nullptr};
private:
    //! Secure factory for QML components, errors are reported on stderr.
    std::unique_ptr<QQmlComponent>  createComponent(const QString& url);
    //! Secure utility to create a QQuickItem from a given QML component \c component (might issue warning if component is nullptr or not successfully loaded).
    QQuickItem*                     createItemFromComponent(QQmlComponent* component);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Node Management *///---------------------------------------
    //@{
public:
    /*! \brief Insert an already existing node, proxy to GTpo graph insertNode().
     *
     * \warning This method is mainly for tests purposes since inserted node delegate and style is
     * left unconfigured. Handle with a lot of care only to insert "pure topology" non visual nodes.
     * \note trigger nodeInserted() signal after insertion and generate a call to onNodeInserted().
     */
    auto    insertNonVisualNode(Node* node) -> bool;

    /*! \brief Insert a new node in this graph and return a pointer on it, or \c nullptr if creation fails.
     *
     * A default node delegate must have been registered with registerNodeDelegate() if
     * \c nodeComponent is unspecified (ie \c nullptr); it is done automatically if
     * Qan.Graph is used, with a rectangular node delegate for default node.
     *
     * \note trigger nodeInserted() signal after insertion and generate a call to onNodeInserted().
     * \note graph has ownership for returned node.
     */
    Q_INVOKABLE qan::Node*  insertNode(QQmlComponent* nodeComponent = nullptr, qan::NodeStyle* nodeStyle = nullptr);

    /*! \brief Insert a node using Node_t::delegate() and Node_t::style(), if no delegate is defined, default on graph \c nodeDelegate.
     *
     * \note trigger nodeInserted() signal after insertion and generate a call to onNodeInserted().
     */
    template <class Node_t>
    Node_t*                 insertNode(QQmlComponent* nodeComponent = nullptr, qan::NodeStyle* nodeStyle = nullptr);

    /*! \brief Same semantic than insertNode<>() but for non visual nodes.
     *
     * \note trigger nodeInserted() signal after insertion and generate a call to onNodeInserted().
     */
    template <class Node_t>
    Node_t*                 insertNonVisualNode();

    /*! \brief Insert and existing node with a specific delegate component and a custom style.
     *
     * \warning \c node ownership is set to Cpp in current QmlEngine.
     *
     * \return true if \c node has been successfully inserted.
     */
    bool                    insertNode(Node* node,
                                       QQmlComponent* nodeComponent = nullptr,
                                       qan::NodeStyle* nodeStyle = nullptr);

    /*! \brief Remove node \c node from this graph. Shortcut to gtpo::GenGraph<>::removeNode().
     *
     *  \arg force if force is true, even locked or protected are removed (default to false).
     */
    Q_INVOKABLE bool        removeNode(qan::Node* node, bool force = false);

    //! Shortcut to gtpo::GenGraph<>::getNodeCount().
    Q_INVOKABLE int         getNodeCount() const noexcept;

    //! Return true if \c node is registered in graph.
    bool                    hasNode(const qan::Node* node) const;

public:
    //! Access the list of nodes with an abstract item model interface.
    Q_PROPERTY(QAbstractItemModel* nodes READ getNodesModel CONSTANT FINAL)
    QAbstractItemModel*     getNodesModel() const { return get_nodes().model(); }

protected:

    /*! \brief Notify user immediately after a new node \c node has been inserted in graph.
     *
     * \warning Since groups are node, onNodeInserted() is also emitted when insertGroup() is called.
     * \note Signal nodeInserted() is emitted at the same time.
     * \note Default implementation is empty.
     */
    virtual void    onNodeInserted(qan::Node& node);

    /*! \brief Notify user immediately before a node \c node is removed.
     *
     * \warning Since groups are node, onNodeRemoved() is also emitted when removeGroup() is called.
     * \note Signal nodeRemoved() is emitted at the same time.
     * \note Default implementation is empty.
     */
    virtual void    onNodeRemoved(qan::Node& node);

signals:

    //! \copydoc onNodeInserted()
    void            nodeInserted(qan::Node* node);

    //! \copydoc onNodeRemoved()
    void            nodeRemoved(qan::Node* node);

signals:
    /*! \brief Emitted whenever a node registered in this graph is clicked.
     */
    void            nodeClicked(qan::Node* node, QPointF pos);
    /*! \brief Emitted whenever a node registered in this graph is right clicked.
     */
    void            nodeRightClicked(qan::Node* node, QPointF pos);
    /*! \brief Emitted whenever a node registered in this graph is double clicked.
     */
    void            nodeDoubleClicked(qan::Node* node, QPointF pos);

signals:
    //! \brief Emitted _after_ a node (or a group...) has been grouped.
    void            nodeGrouped(qan::Node* node, qan::Group* group);
    //! \brief Emitted _after_ a node (or a group...) has been un grouped.
    void            nodeUngrouped(qan::Node* node, qan::Group* group);

    /*! \brief Emitted immediatly _before_ a node is moved.
     */
    void            nodeAboutToBeMoved(qan::Node* node);
    /*! \brief Emitted _after_ a node has been moved.
     */
    void            nodeMoved(qan::Node* node);

    /*! \brief Emitted immediatly _before_ a selection of nodes is moved.
     *  `nodes` might also include groups. Trigerred when selection is
     *  dragged by mouse or using keyboard keys.
     */
    void            nodesAboutToBeMoved(std::vector<qan::Node*> nodes);
    /*! \brief Emitted _after_ a selection of nodes has been moved.
     *  `nodes` might also include groups. Trigerred when selection is
     *  dragged by mouse or using keyboard keys.
     */
    void            nodesMoved(std::vector<qan::Node*> nodes);

    //! \brief Emitted immediately _before_ a node is resized.
    void            nodeAboutToBeResized(qan::Node* node);
    //! \brief Emitted _after_ a node has been resized.
    void            nodeResized(qan::Node* node);

    //! \brief Emitted immediately _before_ a group is resized.
    void            groupAboutToBeResized(qan::Group* group);
    //! \brief Emitted _after_ a group has been resized.
    void            groupResized(qan::Group* group);

    //! Emitted when a node setLabel() method is called.
    void            nodeLabelChanged(qan::Node* node);
    //-------------------------------------------------------------------------

    /*! \name Graph Edge Management *///---------------------------------------
    //@{
public:
    //! QML shortcut to insertEdge() method.
    Q_INVOKABLE qan::Edge*  insertEdge(QObject* source, QObject* destination, QQmlComponent* edgeComponent = nullptr);

    //! Shortcut to gtpo::GenGraph<>::insertEdge().
    virtual qan::Edge*      insertEdge(qan::Node* source, qan::Node* destination, QQmlComponent* edgeComponent = nullptr);

    //! Bind an existing edge source to a visual out port from QML.
    Q_INVOKABLE void        bindEdgeSource(qan::Edge* edge, qan::PortItem* outPort) noexcept;

    //! Bind an existing edge destination to a visual in port from QML.
    Q_INVOKABLE void        bindEdgeDestination(qan::Edge* edge, qan::PortItem* inPort) noexcept;

    //! Shortcut to bindEdgeSource() and bindEdgeDestination() for edge on \c outPort and \c inPort.
    Q_INVOKABLE void        bindEdge(qan::Edge* edge, qan::PortItem* outPort, qan::PortItem* inPort) noexcept;

    /*! \brief Test if an edge source is actually bindable to a given port.
     *
     * This method could be used to check if an edge is bindable to a given port
     * _before_ creating the edge and calling bindEdgeSource(). Port \c multiplicity
     * and \c connectable properties are taken into account to return a result.
     *
     * Example: for an out port with \c Single \c multiplicity where an out edge already
     * exists, this method returns false.
     */
    virtual bool            isEdgeSourceBindable(const qan::PortItem& outPort) const noexcept;

    /*! \brief Test if an edge destination is bindable to a visual in port.
     *
     * Same behaviour than isEdgeSourceBindable() for edge destination port.
     */
    virtual bool            isEdgeDestinationBindable(const qan::PortItem& inPort) const noexcept;

    //! Bind an existing edge source to a visual out port.
    virtual void            bindEdgeSource(qan::Edge& edge, qan::PortItem& outPort) noexcept;

    //! Bind an existing edge destination to a visual in port.
    virtual void            bindEdgeDestination(qan::Edge& edge, qan::PortItem& inPort) noexcept;

public:
    template <class Edge_t>
    qan::Edge*              insertEdge(qan::Node& src, qan::Node* dstNode, QQmlComponent* edgeComponent = nullptr);
private:
    /*! \brief Internal utility used to insert an existing edge \c edge to either a destination \c dstNode node OR edge \c dstEdge.
     *
     * \note insertEdgeImpl() will automatically create \c edge graphical delegate using \c edgeComponent and \c style.
     */
    bool                    configureEdge(qan::Edge& source, QQmlComponent& edgeComponent, qan::EdgeStyle& style,
                                          qan::Node& src, qan::Node* dst);
public:
    template <class Edge_t>
    qan::Edge*              insertNonVisualEdge(qan::Node& src, qan::Node* dstNode);

public:
    //! Shortcut to gtpo::GenGraph<>::removeEdge().
    Q_INVOKABLE virtual bool    removeEdge(qan::Node* source, qan::Node* destination);

    //! Shortcut to gtpo::GenGraph<>::removeEdge().
    Q_INVOKABLE virtual bool    removeEdge(qan::Edge* edge, bool force = false);

    //! Return true if there is at least one directed edge between \c source and \c destination (Shortcut to gtpo::GenGraph<>::hasEdge()).
    Q_INVOKABLE bool        hasEdge(const qan::Node* source, const qan::Node* destination) const;

    //! Return true if edge is in graph.
    Q_INVOKABLE bool        hasEdge(const qan::Edge* edge) const;

public:
    //! Access the list of edges with an abstract item model interface.
    Q_PROPERTY( QAbstractItemModel* edges READ getEdgesModel CONSTANT FINAL )
    QAbstractItemModel* getEdgesModel() const { return get_edges().model(); }

signals:
    /*! \brief Emitted whenever a node registered in this graph is clicked.
     *
     *  \sa nodeClicked()
     */
    void            edgeClicked(qan::Edge* edge, QPointF pos);
    /*! \brief Emitted whenever a node registered in this graph is right clicked.
     *
     *  \sa nodeRightClicked()
     */
    void            edgeRightClicked(qan::Edge* edge, QPointF pos);
    /*! \brief Emitted whenever a node registered in this graph is double clicked.
     *
     *  \sa nodeDoubleClicked()
     */
    void            edgeDoubleClicked(qan::Edge* edge, QPointF pos);

    /*! \brief Emitted _after_ an edge has been inserted (usually with insertEdge()).
     */
    void            edgeInserted(qan::Edge* edge);

    /*! \brief Emitted immediately _before_ an edge is removed.
     */
    void            onEdgeRemoved(qan::Edge* edge);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Group Management *///--------------------------------------
    //@{
public:
    //! Shortcut to gtpo::GenGraph<>::insertGroup().
    Q_INVOKABLE virtual qan::Group* insertGroup(QQmlComponent* groupComponent = nullptr);

    //! Insert a `qan::TableGroup` table.
    Q_INVOKABLE virtual qan::Group* insertTable(int cols, int rows);
signals:
    //! Emitted when a table rows/cols are modified or moved.
    void    tableModified(qan::TableGroup*);

public:
    /*! \brief Insert a new group in this graph and return a pointer on it, or \c nullptr if creation fails.
     *
     * If group insertion fails, method return \c false, no exception is thrown.
     *
     * If \c groupComponent is unspecified (ie \c nullptr), default qan::Graph::groupDelegate is
     * used.
     *
     * \note trigger nodeInserted() signal after insertion and generate a call to onNodeInserted().
     * \note graph keep ownership of the returned node.
     */
    bool                    insertGroup(Group* group,
                                        QQmlComponent* groupComponent = nullptr,
                                        qan::NodeStyle* groupStyle = nullptr);

    //! Insert a group using its static delegate() and style() factories.
    template <class Group_t>
    qan::Group*             insertGroup(QQmlComponent* groupComponent = nullptr);

    //! Insert a group using its static delegate() and style() factories.
    template <class TableGroup_t>
    qan::Group*             insertTable(int cols, int rows);

    /*! Shortcut to gtpo::GenGraph<>::removeGroup().
     *
     *  \note When remove content is set to true group nodes (and any eventual subgroup
     *  is removed too). When false (default behaviour), group nodes are reparented to
     *  graph root.
     */
    Q_INVOKABLE virtual void    removeGroup(qan::Group* group, bool removeContent = false, bool force = false);

protected:
    void        removeGroupContent_rec(qan::Group* group);

public:
    //! Return true if \c group is registered in graph.
    bool                    hasGroup(qan::Group* group) const;

    //! Shortcut to gtpo::GenGraph<>::getGroupCount().
    Q_INVOKABLE int         getGroupCount() const { return get_group_count(); }

    /*! \brief Group a node \c node inside \c group group.
     *
     * To disable node item coordinates transformation to group item, set transform to false then
     * manually position node item (usefull when programmatically grouping node for persistence for example).
     *
     * `groupCell` might be used to request grouping in a specific cell when `group` is a `qan::TableGroup`.
     *
     * \sa gtpo::GenGraph::groupNode()
     */
    Q_INVOKABLE virtual bool    groupNode(qan::Group* group, qan::Node* node, qan::TableCell* groupCell = nullptr,
                                          bool transform = true) noexcept;

    //! Ungroup node \c node from group \c group (using nullptr for \c group ungroup node from it's current group without further topology checks).
    Q_INVOKABLE virtual bool    ungroupNode(qan::Node* node, qan::Group* group = nullptr, bool transform = true) noexcept;

signals:

    /*! \brief Emitted when a group registered in this graph is clicked.
     */
    void            groupClicked(qan::Group* group, QPointF pos);
    /*! \brief Emitted when a group registered in this graph is right clicked.
     */
    void            groupRightClicked(qan::Group* group, QPointF pos);
    /*! \brief Emitted when a group registered in this graph is double clicked.
     */
    void            groupDoubleClicked(qan::Group* group, QPointF pos);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Selection Management *///----------------------------------------
    //@{
public:
    /*! \brief Graph node selection policy (default to \c SelectOnClick);
     * \li \c NoSelection: Selection of nodes is disabled.
     * \li \c SelectOnClick (default): Node is selected when clicked, multiple selection is allowed with CTRL.
     * \li \c SelectOnCtrlClick: Node is selected only when CTRL is pressed, multiple selection is allowed with CTRL.
     */
    enum class SelectionPolicy : int {
        NoSelection         = 1,
        SelectOnClick       = 2,
        SelectOnCtrlClick   = 4
    };
    Q_ENUM(SelectionPolicy)

public:
    /*! \brief Current graph selection policy.
     *
     * \warning setting NoSeleciton will clear the actual \c selectedNodes model.
     */
    Q_PROPERTY(SelectionPolicy selectionPolicy READ getSelectionPolicy WRITE setSelectionPolicy NOTIFY selectionPolicyChanged FINAL)
    void                    setSelectionPolicy(SelectionPolicy selectionPolicy);
    SelectionPolicy         getSelectionPolicy() const { return _selectionPolicy; }
private:
    SelectionPolicy         _selectionPolicy = SelectionPolicy::SelectOnClick;
signals:
    void                    selectionPolicyChanged();

public:
    /*! \brief Enable mutliple selection (default to true ie multiple selection with CTRL is enabled).
     *
     * \warning setting \c multipleSelectionEnabled to \c false will clear the actual \c selectedNodes model.
     */
    Q_PROPERTY(bool multipleSelectionEnabled READ getMultipleSelectionEnabled WRITE setMultipleSelectionEnabled NOTIFY multipleSelectionEnabledChanged FINAL)
    auto                setMultipleSelectionEnabled(bool multipleSelectionEnabled) -> bool;
    auto                getMultipleSelectionEnabled() const -> bool { return _multipleSelectionEnabled; }
private:
    bool                _multipleSelectionEnabled = true;
signals:
    void                multipleSelectionEnabledChanged();


public:
    //! Color for the node selection hilgither component (default to dodgerblue).
    Q_PROPERTY(QColor selectionColor READ getSelectionColor WRITE setSelectionColor NOTIFY selectionColorChanged FINAL)
    void            setSelectionColor(QColor selectionColor) noexcept;
    inline QColor   getSelectionColor() const noexcept { return _selectionColor; }
private:
    QColor          _selectionColor{30, 144, 255};  // dodgerblue=rgb(30, 144, 255)
signals:
    void            selectionColorChanged();

public:
    //! Selection hilgither item stroke width (default to 3.0).
    Q_PROPERTY(qreal selectionWeight READ getSelectionWeight WRITE setSelectionWeight NOTIFY selectionWeightChanged FINAL)
    void            setSelectionWeight(qreal selectionWeight) noexcept;
    inline qreal    getSelectionWeight() const noexcept { return _selectionWeight; }
private:
    qreal           _selectionWeight = 3.;
signals:
    void            selectionWeightChanged();

public:
    //! Margin between the selection hilgither item and a selected item (default to 3.0).
    Q_PROPERTY(qreal selectionMargin READ getSelectionMargin WRITE setSelectionMargin NOTIFY selectionMarginChanged FINAL)
    void            setSelectionMargin(qreal selectionMargin) noexcept;
    inline qreal    getSelectionMargin() const noexcept { return _selectionMargin; }
private:
    qreal           _selectionMargin = 3.;
signals:
    void            selectionMarginChanged();

protected:
    /*! \brief Force a call to qan::Selectable::configureSelectionItem() call on all currently selected primitives (either nodes or group).
     */
    void            configureSelectionItems() noexcept;

public:
    /*! \brief Request insertion of a node in the current selection according to current policy and return true if the node was successfully selected.
     *
     * \note A call to selectNode() on an already selected node will deselect it, to set an "absolute" selection, use setNodeSelected().
     *
     * \note If \c selectionPolicy is set to Qan.AbstractGraph.NoSelection or SelextionPolicy::NoSelection,
     * method will always return false.
     */
    bool                selectNode(qan::Node& node, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    Q_INVOKABLE bool    selectNode(qan::Node* node, Qt::KeyboardModifiers modifiers);
    Q_INVOKABLE bool    selectNode(qan::Node* node);

    //! Set the node selection state (graph selectionPolicy is not taken into account).
    void                setNodeSelected(qan::Node& node, bool selected);
    //! \copydoc setNodeSelected
    Q_INVOKABLE void    setNodeSelected(qan::Node* node, bool selected);

    //! Similar to selectNode() for qan::Group (internally group is a node).
    bool                selectGroup(qan::Group& group, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    Q_INVOKABLE bool    selectGroup(qan::Group* group);

    //! Set an edge selection state (usefull to unselect).
    void                setEdgeSelected(qan::Edge* edge, bool selected);

    //! Similar to selectNode() for qan::Edge.
    bool                selectEdge(qan::Edge& edge, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    Q_INVOKABLE bool    selectEdge(qan::Edge* edge);

    /*! \brief Add a node in the current selection.
     */
    void            addToSelection(qan::Node& node);
    //! \copydoc addToSelection
    void            addToSelection(qan::Group& group);
    //! \copydoc addToSelection
    void            addToSelection(qan::Edge& edge);

    //! Remove a node from the selection.
    void            removeFromSelection(qan::Node& node);
    //! \copydoc removeFromSelection
    void            removeFromSelection(qan::Group& node);
    //! \copydoc removeFromSelection
    void            removeFromSelection(QQuickItem* item);

    //! Select all graph content (nodes, groups and edges).
    Q_INVOKABLE void    selectAll();

    //! Remove all selected nodes and groups and clear selection.
    Q_INVOKABLE void    removeSelection();

    //! Clear the current selection.
    Q_INVOKABLE void    clearSelection();

    //! Return true if either a nodes, groups, edges (or multiple nodes, groups, edges) is selected.
    Q_INVOKABLE bool    hasSelection() const;

    //! Return true if multiple nodes, groups or edges are selected.
    Q_INVOKABLE bool    hasMultipleSelection() const;

public:
    using SelectedNodes = qcm::Container<std::vector, QPointer<qan::Node>>;

    //! Read-only list model of currently selected nodes.
    Q_PROPERTY(QAbstractItemModel* selectedNodes READ getSelectedNodesModel NOTIFY selectedNodesChanged FINAL)  // In fact non-notifiable, avoid QML warning
    QAbstractItemModel* getSelectedNodesModel() { return qobject_cast<QAbstractItemModel*>(_selectedNodes.model()); }

    inline auto         getSelectedNodes() -> SelectedNodes& { return _selectedNodes; }
    inline auto         getSelectedNodes() const -> const SelectedNodes& { return _selectedNodes; }
private:
    SelectedNodes       _selectedNodes;
signals:
    void                selectedNodesChanged();
    //! Emitted whenever nodes/groups/edge selection change.
    void                selectionChanged();

public:
    using SelectedGroups = qcm::Container<std::vector, QPointer<qan::Group>>;

    //! Read-only list model of currently selected groups.
    Q_PROPERTY(QAbstractItemModel* selectedGroups READ getSelectedGroupsModel NOTIFY selectedGroupsChanged FINAL)   // In fact non-notifiable, avoid QML warning
    QAbstractItemModel* getSelectedGroupsModel() { return qobject_cast<QAbstractItemModel*>(_selectedGroups.model()); }

    inline auto         getSelectedGroups() -> SelectedGroups& { return _selectedGroups; }
    inline auto         getSelectedGroups() const -> const SelectedGroups& { return _selectedGroups; }
private:
    SelectedGroups      _selectedGroups;
signals:
    void                selectedGroupsChanged();

public:
    using SelectedEdges = qcm::Container<std::vector, QPointer<qan::Edge>>;

    //! \copydoc _selectedEdges
    Q_PROPERTY(QAbstractItemModel* selectedEdges READ getSelectedEdgesModel NOTIFY selectedEdgesChanged FINAL)   // In fact non-notifiable, avoid QML warning
    //! \copydoc _selectedEdges
    QAbstractItemModel* getSelectedEdgesModel() { return qobject_cast<QAbstractItemModel*>(_selectedEdges.model()); }
    //! \copydoc _selectedEdges
    inline auto         getSelectedEdges() -> SelectedEdges& { return _selectedEdges; }
    //! \copydoc _selectedEdges
    inline auto         getSelectedEdges() const -> const SelectedEdges& { return _selectedEdges; }
private:
    //! Read-only list model of currently selected edges.
    SelectedEdges       _selectedEdges;
signals:
    //! \copydoc _selectedEdges
    void                selectedEdgesChanged();

protected:
    //! \brief Return a vector of currently selected nodes/groups items.
    std::vector<QQuickItem*>    getSelectedItems() const;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Grid and Alignment Management *///-------------------------------
    //@{
public:
    //! Snap dragged objects (both groups and nodes) to grid based on \c snapGridSize (default to false is no snap).
    Q_PROPERTY(bool snapToGrid READ getSnapToGrid WRITE setSnapToGrid NOTIFY snapToGridChanged FINAL)
    bool            setSnapToGrid(bool snapToGrid) noexcept;
    bool            getSnapToGrid() const noexcept { return _snapToGrid; }
private:
    bool            _snapToGrid = false;
signals:
    void            snapToGridChanged();

public:
    //! Snap to grid size (default to 10x10), active only when \c snapToGrid is true.
    Q_PROPERTY(QSizeF snapToGridSize READ getSnapToGridSize WRITE setSnapToGridSize NOTIFY snapToGridSizeChanged FINAL)
    bool            setSnapToGridSize(QSizeF snapToGridSize) noexcept;
    QSizeF          getSnapToGridSize() const noexcept { return _snapToGridSize; }
private:
    QSizeF          _snapToGridSize{10., 10.};
signals:
    void            snapToGridSizeChanged();

public:
    //! \brief Align selected nodes/groups items horizontal center.
    Q_INVOKABLE void    alignSelectionHorizontalCenter();
    //! \brief Align selected nodes/groups items right.
    Q_INVOKABLE void    alignSelectionRight();
    //! \brief Align selected nodes/groups items left.
    Q_INVOKABLE void    alignSelectionLeft();
    //! \brief Align selected nodes/groups items top.
    Q_INVOKABLE void    alignSelectionTop();
    //! \brief Align selected nodes/groups items bottom.
    Q_INVOKABLE void    alignSelectionBottom();
protected:
    //! \brief Align \c items horizontal center.
    void    alignHorizontalCenter(std::vector<QQuickItem*>&& items);
    //! \brief Align \c items right.
    void    alignRight(std::vector<QQuickItem*>&& items);
    //! \brief Align \c items left.
    void    alignLeft(std::vector<QQuickItem*>&& items);
    //! \brief Align \c items top.
    void    alignTop(std::vector<QQuickItem*>&& items);
    //! \brief Align \c items bottom.
    void    alignBottom(std::vector<QQuickItem*>&& items);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Style Management *///--------------------------------------------
    //@{
public:
    /*! \brief Graph style manager (ie list of style applicable to graph primitive).
     */
    Q_PROPERTY(qan::StyleManager* styleManager READ getStyleManager CONSTANT FINAL)
    qan::StyleManager*       getStyleManager() noexcept { return &_styleManager; }
    const qan::StyleManager* getStyleManager() const noexcept { return &_styleManager; }
private:
    qan::StyleManager       _styleManager;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Port/Dock Management *///----------------------------------------
    //@{
public:

    /*! QML interface for adding an in port to node \c node using default port delegate.
     *
     * \note might return nullptr if std::bad_alloc is thrown internally or \c node is invalid or nullptr.
     * \note After creation, you should configure node multiplicity (hability to have multiple connections
     * per port), connecivity default to qan::PortItem::Multiple.
     *
     * \param node  port host node.
     * \param dock  port dock, default to left for in port (either Dock::Top, Dock::Bottom, Dock::Right, Dock::Left).
     * \param type  either an in, out or in/out port (default to in/out).
     * \param label port visible label.
     * \param id    Optional unique ID for the port (caller must ensure uniqueness), could be used to retrieve ports using qan::NodeItem::getPort().
     */
    Q_INVOKABLE qan::PortItem*  insertPort(qan::Node* node,
                                           qan::NodeItem::Dock dock,
                                           qan::PortItem::Type portType = qan::PortItem::Type::InOut,
                                           QString label = "",
                                           QString id = "" ) noexcept;

    /*! Remove a port from a node.
     *
     * \param node  port host node.
     * \param port  node's port to remove.
     */
    Q_INVOKABLE void        removePort(qan::Node* node, qan::PortItem* port) noexcept;

public:
    //! Default delegate for node in/out port.
    Q_PROPERTY(QQmlComponent* portDelegate READ getPortDelegate WRITE qmlSetPortDelegate NOTIFY portDelegateChanged FINAL)
    //! \copydoc portDelegate
    inline QQmlComponent*   getPortDelegate() noexcept { return _portDelegate.get(); }
protected:
    //! \copydoc portDelegate
    void                    qmlSetPortDelegate(QQmlComponent* portDelegate) noexcept;
    //! \copydoc portDelegate
    void                    setPortDelegate(std::unique_ptr<QQmlComponent> portDelegate) noexcept;
signals:
    //! \copydoc portDelegate
    void                    portDelegateChanged();
private:
    //! \copydoc portDelegate
    std::unique_ptr<QQmlComponent> _portDelegate;

signals:
    /*! \brief Emitted whenever a port node registered in this graph is clicked.
     */
    void            portClicked(qan::PortItem* port, QPointF pos);
    /*! \brief Emitted whenever a port node registered in this graph is right clicked.
     */
    void            portRightClicked(qan::PortItem* port, QPointF pos);

public:
    //! Default delegate for horizontal (either NodeItem::Dock::Top or NodeItem::Dock::Bottom) docks.
    Q_PROPERTY(QQmlComponent* horizontalDockDelegate READ getHorizontalDockDelegate WRITE setHorizontalDockDelegate NOTIFY horizontalDockDelegateChanged FINAL)
    //! \copydoc horizontalDockDelegate
    inline QQmlComponent*   getHorizontalDockDelegate() noexcept { return _horizontalDockDelegate.get(); }
protected:
    //! \copydoc horizontalDockDelegate
    void                    setHorizontalDockDelegate(QQmlComponent* horizontalDockDelegate) noexcept;
    //! \copydoc horizontalDockDelegate
    void                    setHorizontalDockDelegate(std::unique_ptr<QQmlComponent> horizontalDockDelegate) noexcept;
signals:
    //! \copydoc horizontalDockDelegate
    void                    horizontalDockDelegateChanged();
private:
    //! \copydoc horizontalDockDelegate
    std::unique_ptr<QQmlComponent> _horizontalDockDelegate;

public:
    //! Default delegate for vertical (either NodeItem::Dock::Left or NodeItem::Dock::Right) docks.
    Q_PROPERTY(QQmlComponent* verticalDockDelegate READ getVerticalDockDelegate WRITE setVerticalDockDelegate NOTIFY verticalDockDelegateChanged FINAL)
    //! \copydoc horizontalDockDelegate
    inline QQmlComponent*   getVerticalDockDelegate() noexcept { return _verticalDockDelegate.get(); }
protected:
    //! \copydoc horizontalDockDelegate
    void                    setVerticalDockDelegate(QQmlComponent* verticalDockDelegate) noexcept;
    //! \copydoc horizontalDockDelegate
    void                    setVerticalDockDelegate(std::unique_ptr<QQmlComponent> verticalDockDelegate) noexcept;
signals:
    //! \copydoc horizontalDockDelegate
    void                    verticalDockDelegateChanged();
private:
    //! \copydoc verticalDockDelegate
    std::unique_ptr<QQmlComponent> _verticalDockDelegate;

protected:
    //! Create a dock item from an existing dock item delegate.
    QPointer<QQuickItem>     createDockFromDelegate(qan::NodeItem::Dock dock, qan::Node& node) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Stacking Management *///-----------------------------------------
    //@{
public:
    /*! \brief Send a graphic item (either a node or a group) to top.
     *
     * \note When item is a group, the node is moved to top inside it's group, and group is also send
     * to top.
     * \note Method is non const as it might affect \c maxZ property.
     */
    Q_INVOKABLE void    sendToFront(QQuickItem* item);

    //! Send a graphic item (either a node or a group) to back.
    Q_INVOKABLE void    sendToBack(QQuickItem* item);

public:
    /*! \brief Iterate over all graph container items, find and update the minZ and maxZ property.
     *
     * \note O(N) with N beeing the graph item count (might be quite costly, mainly defined to update
     * maxZ after in serialization for example).
     */
    Q_INVOKABLE void    updateMinMaxZ() noexcept;

    /*! \brief Maximum global z for nodes and groups (ie top-most item).
     *
     * \note By global we mean that z value for a node parented to a group is parent(s) group(s)
     * a plus item z.
     */
    Q_PROPERTY(qreal    maxZ READ getMaxZ WRITE setMaxZ NOTIFY maxZChanged FINAL)
    qreal               getMaxZ() const noexcept;
    void                setMaxZ(const qreal maxZ) noexcept;
private:
    qreal               _maxZ = 0.;
signals:
    void                maxZChanged();

public:
    //! Add 1. to \c maxZ and return the new \c maxZ.
    qreal               nextMaxZ() noexcept;
    //! Update maximum z value if \c z is greater than actual \c maxZ value.
    Q_INVOKABLE void    updateMaxZ(const qreal z) noexcept;

public:
    //! \brief Minimum global z for nodes and groups (ie bottom-less item).
    Q_PROPERTY(qreal    minZ READ getMinZ WRITE setMinZ NOTIFY minZChanged FINAL)
    qreal               getMinZ() const noexcept;
    void                setMinZ(const qreal minZ) noexcept;
private:
    qreal               _minZ = 0.;
signals:
    void                minZChanged();

public:
    //! Substract 1. to \c minZ and return the new \c minZ.
    qreal               nextMinZ() noexcept;
    //! Update minimum z value if \c z is less than actual \c minZ value.
    Q_INVOKABLE void    updateMinZ(const qreal z) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Topology Algorithms *///-----------------------------------------
    //@{
public:
    /*! \brief Synchronously collect all graph nodes of \c node using DFS.
     *
     * \warning this method is synchronous and recursive.
     */
    std::vector<QPointer<const qan::Node>>   collectRootNodes() const noexcept;

    /*! \brief Synchronously collect all sub-nodes of graph root nodes using DFS.
     *
     * \warning this method is synchronous and recursive.
     */
    std::vector<const qan::Node*>   collectDfs(bool collectGroup = false) const noexcept;

    /*! \brief Synchronously collect all child nodes of \c node using DFS.
     *
     * \note \c node is automatically added to the result and returned as the first
     * node of the return set.
     * \warning this method is synchronous and recursive.
     */
    std::vector<const qan::Node*>   collectDfs(const qan::Node& node, bool collectGroup = false) const noexcept;

    //! Collect all out nodes of \c nodes using DFS, return an unordered set of subnodes (nodes in node are _not_ in returned set).
    auto    collectSubNodes(const QVector<qan::Node*> nodes, bool collectGroup = false) const noexcept -> std::unordered_set<const qan::Node*>;

private:
    void    collectDfsRec(const qan::Node*,
                          std::unordered_set<const qan::Node*>& marks,
                          std::vector<const qan::Node*>& childs,
                          bool collectGroup) const noexcept;

public:
    //! Return a set of all edges strongly connected to a set of nodes (ie where source AND destination is in \c nodes).
    auto    collectInnerEdges(const std::vector<const qan::Node*>& nodes) const -> std::unordered_set<const qan::Edge*>;

public:
    /*! \brief Recursively collect all "neighbours" of \c node, neighbours are nodes in the same group or the same parent groups.
     *
     * Neighbours are not linked from a topology point of view by edges, only by common group membership.
     *
     * Exemple:
     *             +---------------------------+
     *             | G2                        |
     * +--------+  |     +---------------+     |
     * |G3      |  |     |       G1      |     |
     * |   N4---+--+-----+->N1       N2  | N3  |
     * |        |  |     |               |     |
     * |   N5   |  |     +---------------+     |
     * +--------+  |                           |
     *             +---------------------------+
     *
     * Neighbours of N1: [N1, N2, G1, G2, N3]  note presence of N3 in N1 parent group.
     * Neighbours of N4: [N4, N5, G3]
     *
     * \warning this method is synchronous and recursive.
     */
    std::vector<const qan::Node*>   collectNeighbours(const qan::Node& node) const;

    //! FIXME.
    std::vector<const qan::Node*>   collectGroups(const qan::Node& node) const;

    /*! \brief Synchronously collect all parent nodes of \c node.
     *
     * \note All ancestors "neighbours" nodes are also added to set.
     * \note \c node is _not_ added to result.
     * \sa collectNeighbours()
     * \warning this method is synchronous and recursive.
     */
    std::vector<const qan::Node*>   collectAncestors(const qan::Node& node) const;

    //! FIXME.
    std::vector<const qan::Node*>   collectChilds(const qan::Node& node) const;

public:
    //! \copydoc isAncestor()
    Q_INVOKABLE bool        isAncestor(qan::Node* node, qan::Node* candidate) const;

    /*! \brief Return true if \c candidate node is an ancestor of given \c node.
     *
     * \warning this method is synchronous and recursive.
     * \return true if \c candidate is an ancestor of \c node (ie \c node is an out
     * node of \c candidate at any degree).
     */
    bool                    isAncestor(const qan::Node& node, const qan::Node& candidate) const noexcept;

public:
    /*! Collect all nodes and groups contained in given groups.
     *
     * \note Recursively collect \c groups nodes and sub groups and group sub group nodes.
     */
    auto    collectGroupsNodes(const QVector<const qan::Group*>& groups) const noexcept -> std::unordered_set<const qan::Node*>;
protected:

    // Recursive utility for collectGroupsNodes().
    auto    collectGroupNodes_rec(const qan::Group* group, std::unordered_set<const qan::Node*>& nodes) const -> void;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

#include "./qanGraph.hpp"

QML_DECLARE_TYPE(qan::Graph)
