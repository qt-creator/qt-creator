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
// \file	qanGraph.cpp
// \author	benoit@destrat.io
// \date	2004 February 15
//-----------------------------------------------------------------------------

// Std headers
#include <memory>

// Qt headers
#include <QQmlProperty>
#include <QVariant>
#include <QQmlEngine>
#include <QQmlComponent>

// QuickQanava headers
#include "./qanUtils.h"
#include "./qanGraph.h"
#include "./qanGraphView.h"
#include "./qanNodeItem.h"
#include "./qanPortItem.h"
#include "./qanEdgeItem.h"
#include "./qanTableGroupItem.h"
#include "./qanGroup.h"
#include "./qanGroupItem.h"
#include "./qanConnector.h"

namespace qan { // ::qan

/* Graph Object Management *///------------------------------------------------
Graph::Graph(QQuickItem* parent) noexcept :
    super_t{parent}
{
    setContainerItem(this);
    setAntialiasing(true);
    setSmooth(true);
    // Note: do not accept mouse buttons, mouse events are captured in
    // GraphView containerItem.
}

Graph::~Graph()
{
    // Force diconnection of node/edges signals, it avoid
    // triggering code on this partially deleted graph when a node or
    // edge destroyed() signal is binded to something that try to access this
    // partially destroyed graph (for example a nodes/edges model...!).
    for (const auto node: get_nodes())
        node->disconnect(node, 0, 0, 0);
    for (const auto edge: get_edges())
        edge->disconnect(edge, 0, 0, 0);
}

void    Graph::classBegin()
{
    setPortDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/Port.qml")));
    setHorizontalDockDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/HorizontalDock.qml")));
    setVerticalDockDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/VerticalDock.qml")));
    setGroupDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/Group.qml")));
    // Note: Do not set a default node delegate, otherwise it would be used instead
    //  of qan::Node::delegate(), just let the user specify one.
    setEdgeDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/Edge.qml")));
    setSelectionDelegate(createComponent(QStringLiteral("qrc:/QuickQanava/SelectionItem.qml")));

    const auto engine = qmlEngine(this);
    if (engine != nullptr) {
        _styleManager.setStyleComponent(qan::Node::style(), qan::Node::delegate(*engine));
        _styleManager.setStyleComponent(qan::Edge::style(), qan::Edge::delegate(*engine));
    } else qWarning() << "qan::Graph::classBegin(): Error, no valid QML engine available.";
}

void    Graph::componentComplete()
{
    if (_connector)     // Initialize _connector just once, it looks like this method could be called multiple times (Qt 5.15...)
        return;
    const auto engine = qmlEngine(this);
    if (engine != nullptr) {
        // Visual connector initialization
        auto connectorComponent = std::make_unique<QQmlComponent>(engine, QStringLiteral("qrc:/QuickQanava/VisualConnector.qml"));
        if (connectorComponent) {
            qan::Style* style = qan::Connector::style(nullptr);
            if (style != nullptr) {
                _connector.reset(qobject_cast<qan::Connector*>(createFromComponent(connectorComponent.get(), *style, nullptr)));
                emit connectorChanged();
                if (_connector) {
                    _connector->setGraph(this);
                    _connector->setEnabled(getConnectorEnabled());
                    _connector->setVisible(false);
                    _connector->setProperty("edgeColor", getConnectorEdgeColor());
                    _connector->setProperty("connectorColor", getConnectorColor());
                    _connector->setProperty("createDefaultEdge", getConnectorCreateDefaultEdge());
                    if (getConnectorItem() != nullptr)
                        _connector->setConnectorItem(getConnectorItem());
                    connect(_connector.data(), &qan::Connector::requestEdgeCreation,
                            this,              &qan::Graph::connectorRequestEdgeCreation);
                    connect(_connector.data(), &qan::Connector::edgeInserted,
                            this,              &qan::Graph::connectorEdgeInserted);
                }
            } else qWarning() << "qan::Graph::componentComplete(): Error: No style available for connector creation.";
        }
    } else qWarning() << "qan::Graph::componentComplete(): Error: No QML engine available to register default QML delegates.";
}

QQuickItem*             Graph::qmlGetGraphView() { return _graphView; }
qan::GraphView*         Graph::getGraphView() { return _graphView; }
const qan::GraphView*   Graph::getGraphView() const { return _graphView; }
void    Graph::setGraphView(qan::GraphView* graphView)
{
    _graphView = graphView;
    emit graphViewChanged();
}

void    Graph::setContainerItem(QQuickItem* containerItem)
{
    // PRECONDITIONS:
    // containerItem can't be nullptr
    if (containerItem == nullptr) {
        qWarning() << "qan::Graph::setContainerItem(): Error, invalid container item.";
        return;
    }
    if (containerItem != nullptr &&
        containerItem != _containerItem.data()) {
        _containerItem = containerItem;
        emit containerItemChanged();
    }
}
//-----------------------------------------------------------------------------


void    Graph::clearGraph() noexcept
{
    qan::Graph::clear();
}

void    Graph::clear() noexcept
{
    _selectedNodes.clear();
    _selectedGroups.clear();
    _selectedEdges.clear();
    super_t::clear();
    _styleManager.clear();
}

QQuickItem* Graph::graphChildAt(qreal x, qreal y) const
{
    if (getContainerItem() == nullptr)
        return nullptr;
    const auto childrens = getContainerItem()->childItems();
    for (int i = childrens.count() - 1; i >= 0; --i) {
        QQuickItem* child = childrens.at(i);
        const QPointF point = mapToItem(child, QPointF{x, y});  // Map coordinates to the child element's coordinate space
        if (child != nullptr &&
            child->isVisible() &&
            child->contains(point) &&    // Note 20160508: childAt do not call contains()
            point.x() > -0.0001 &&
            child->width() > point.x() &&
            point.y() > -0.0001 &&
            child->height() > point.y()) {
            if (child->inherits("qan::TableGroupItem")) {
                const auto tableGroupItem = static_cast<qan::TableGroupItem*>(child);
                if (tableGroupItem != nullptr) {
                    for (const auto cell: tableGroupItem->getCells()) {
                        const auto point = mapToItem(cell, QPointF(x, y)); // Map coordinates to group child element's coordinate space
                        if (cell->isVisible() &&
                            cell->contains(point) &&
                            point.x() > -0.0001 &&
                            cell->width() > point.x() &&
                            point.y() > -0.0001 &&
                            cell->height() > point.y()) {
                            QQmlEngine::setObjectOwnership(cell->getItem(), QQmlEngine::CppOwnership);
                            return cell->getItem();
                        }
                    }
                }
            }
            if (child->inherits("qan::GroupItem")) {  // For group, look in group childs
                const auto groupItem = qobject_cast<qan::GroupItem*>(child);
                if (groupItem != nullptr &&
                    groupItem->getContainer() != nullptr) {
                    const QList<QQuickItem *> groupChildren = groupItem->getContainer()->childItems();
                    for (int gc = groupChildren.count() - 1; gc >= 0; --gc) {
                        QQuickItem* groupChild = groupChildren.at(gc);
                        const auto point = mapToItem(groupChild, QPointF(x, y)); // Map coordinates to group child element's coordinate space
                        if (groupChild->isVisible() &&
                            groupChild->contains(point) &&
                            point.x() > -0.0001 &&
                            groupChild->width() > point.x() &&
                            point.y() > -0.0001 &&
                            groupChild->height() > point.y()) {
                            QQmlEngine::setObjectOwnership(groupChild, QQmlEngine::CppOwnership);
                            return groupChild;
                        }
                    }
                }
            }
            QQmlEngine::setObjectOwnership(child, QQmlEngine::CppOwnership);
            return child;
        }
    }
    return nullptr;
}

qan::Group* Graph::groupAt(const QPointF& p, const QSizeF& s, const QQuickItem* except) const
{
    // PRECONDITIONS:
        // s must be valid
        // except can be nullptr
    if (!s.isValid())
        return nullptr;

    // Algorithm:
        // 1. Copy internal group list
        // 2. Order the container of group from maximum to minimum z
        // 3. Return the first group containing rect(p,s)

    // 1.
    std::vector<qan::Group*> groups;
    groups.reserve(static_cast<unsigned int>(get_groups().size()));
    for (const auto group : qAsConst(get_groups().getContainer())) {
        if (group != nullptr)
            groups.push_back(group);
    }

    // 2.
    std::sort(groups.begin(), groups.end(), [](const auto g1, const auto g2) -> bool {
        if (g1 == nullptr || g2 == nullptr)
            return false;
        const auto g1Item = g1->getItem();
        const auto g2Item = g2->getItem();
        if (g1Item == nullptr ||
            g2Item == nullptr)
            return false;
        const auto g1GlobalZ = qan::getItemGlobalZ_rec(g1Item);
        const auto g2GlobalZ = qan::getItemGlobalZ_rec(g2Item);
        return g1GlobalZ > g2GlobalZ;
    });

    // 3.
    if (getContainerItem() == nullptr)
            return nullptr;
    for (const auto group : qAsConst(groups)) {
        if (group &&
            group->getItem() != nullptr &&
            group->getItem() != except) {
            const auto groupItem = group->getGroupItem();
            if (groupItem == nullptr)
                continue;
            if (groupItem->getCollapsed())
                continue;  // Do not return collapsed groups

            const auto groupRect =  QRectF{groupItem->mapToItem(getContainerItem(), QPointF{0., 0.}),
                                           QSizeF{groupItem->width(), groupItem->height()}};
            const auto targetSize = groupItem->getStrictDrop() ? s :            // In non-strict mode (ie for TableGroup) target has not
                                                                 QSizeF{1,1};   // to be fully contained by group to trigger a drop.
            if (groupRect.contains(QRectF{p, targetSize})) {
                QQmlEngine::setObjectOwnership(group, QQmlEngine::CppOwnership);
                return group;
            }
        }
    } // for all groups
    return nullptr;
}
//-----------------------------------------------------------------------------


/* Visual connection Management *///-------------------------------------------
void    Graph::setConnectorSource(qan::Node* sourceNode) noexcept
{
    if (_connector) {
        if (sourceNode != nullptr)
            _connector->setSourceNode(sourceNode);
        _connector->setVisible(getConnectorEnabled());
        _connector->setEnabled(getConnectorEnabled());
    }
}

void    Graph::setConnectorEdgeColor(QColor connectorEdgeColor) noexcept
{
    if (connectorEdgeColor != _connectorEdgeColor) {
        _connectorEdgeColor = connectorEdgeColor;
        if (_connector)
            _connector->setProperty("edgeColor", connectorEdgeColor);
        emit connectorEdgeColorChanged();
    }
}

void    Graph::setConnectorColor(QColor connectorColor) noexcept
{
    if (connectorColor != _connectorColor) {
        _connectorColor = connectorColor;
        if (_connector)
            _connector->setProperty("connectorColor", connectorColor);
        emit connectorColorChanged();
    }
}

void    Graph::setConnectorCreateDefaultEdge(bool connectorCreateDefaultEdge) noexcept
{
    if (connectorCreateDefaultEdge != _connectorCreateDefaultEdge) {
        _connectorCreateDefaultEdge = connectorCreateDefaultEdge;
        if (_connector)
            _connector->setProperty("createDefaultEdge", connectorCreateDefaultEdge);
        emit connectorCreateDefaultEdgeChanged();
    }
}

void    Graph::setConnectorItem(QQuickItem* connectorItem) noexcept
{
    if (connectorItem != _connectorItem) {
        _connectorItem = connectorItem;
        if (_connectorItem &&
            _connector)
            _connector->setConnectorItem(_connectorItem.data());
        emit connectorItemChanged();
    }
}

void    Graph::setConnectorEnabled(bool connectorEnabled) noexcept
{
    if (connectorEnabled != _connectorEnabled) {
        _connectorEnabled = connectorEnabled;
        if (_connector) {
            _connector->setVisible(connectorEnabled);
            _connector->setEnabled(connectorEnabled);
        }
        emit connectorEnabledChanged();
    }
}
qan::Connector* Graph::getConnector() noexcept  {  return _connector.data(); }
//-----------------------------------------------------------------------------

/* Delegates Management *///---------------------------------------------------
void    Graph::setNodeDelegate(QQmlComponent* nodeDelegate) noexcept
{
    if (nodeDelegate != nullptr) {
        if (nodeDelegate != _nodeDelegate.get()) {
            _nodeDelegate.reset(nodeDelegate);
            QQmlEngine::setObjectOwnership(nodeDelegate, QQmlEngine::CppOwnership);
            emit nodeDelegateChanged();
        }
    }
}

void    Graph::setNodeDelegate(std::unique_ptr<QQmlComponent> nodeDelegate) noexcept
{
    setNodeDelegate(nodeDelegate.release());
}

void    Graph::setEdgeDelegate(QQmlComponent* edgeDelegate) noexcept
{
    QQmlEngine::setObjectOwnership( edgeDelegate, QQmlEngine::CppOwnership );
    setEdgeDelegate(std::unique_ptr<QQmlComponent>(edgeDelegate));
}

void    Graph::setEdgeDelegate(std::unique_ptr<QQmlComponent> edgeDelegate) noexcept
{
    if (edgeDelegate &&
        edgeDelegate != _edgeDelegate) {
        _edgeDelegate = std::move(edgeDelegate);
        emit edgeDelegateChanged();
    }
}

void    Graph::setGroupDelegate(QQmlComponent* groupDelegate) noexcept
{
    if (groupDelegate != nullptr) {
        if (groupDelegate != _groupDelegate.get()) {
            _groupDelegate.reset(groupDelegate);
            QQmlEngine::setObjectOwnership(groupDelegate, QQmlEngine::CppOwnership);
            emit groupDelegateChanged();
        }
    }
}

void    Graph::setGroupDelegate(std::unique_ptr<QQmlComponent> groupDelegate) noexcept
{
    setGroupDelegate(groupDelegate.release());
}

QQuickItem* Graph::createFromComponent(QQmlComponent* component,
                                       qan::Style& style,
                                       qan::Node* node,
                                       qan::Edge* edge,
                                       qan::Group* group) noexcept
{
    if (component == nullptr) {
        qWarning() << "qan::Graph::createFromComponent(): Error called with a nullptr delegate component.";
        return nullptr;
    }
    QQuickItem* item = nullptr;
    try {
        if (!component->isReady())
            throw qan::Error{ "Error delegate component is not ready." };

        const auto rootContext = qmlContext(this);
        if (rootContext == nullptr)
            throw qan::Error{ "Error can't access to local QML context." };
        QObject* object = component->beginCreate(rootContext);
        if (object == nullptr ||
            component->isError()) {
            if (object != nullptr)
                object->deleteLater();
            throw qan::Error{ "Failed to create a concrete QQuickItem from QML component:\n\t" +
                              component->errorString() };
        }
        // No error occurs
        if (node != nullptr) {
            const auto nodeItem = qobject_cast<qan::NodeItem*>(object);
            if (nodeItem != nullptr) {
                node->setItem(nodeItem);
                nodeItem->setNode(node);
                nodeItem->setGraph(this);
                nodeItem->setStyle(qobject_cast<qan::NodeStyle*>(&style));
                _styleManager.setStyleComponent(&style, component );
            }
        } else if (edge != nullptr) {
            const auto edgeItem = qobject_cast<qan::EdgeItem*>(object);
            if (edgeItem != nullptr) {
                edge->setItem(edgeItem);
                edgeItem->setEdge(edge);
                edgeItem->setGraph(this);
                edgeItem->setStyle(qobject_cast<qan::EdgeStyle*>(&style));
                _styleManager.setStyleComponent(edgeItem->getStyle(), component);
            }
        } else if (group != nullptr) {
            const auto groupItem = qobject_cast<qan::GroupItem*>(object);
            if (groupItem != nullptr) {
                group->setItem(groupItem);
                groupItem->setGraph(this);
                groupItem->setStyle(qobject_cast<qan::NodeStyle*>(&style));
                _styleManager.setStyleComponent(groupItem->getStyle(), component);
            }
        } else {
            const auto nodeItem = qobject_cast<qan::NodeItem*>(object); // Note 20170323: Usefull for Qan.StyleListView, where there
            if (nodeItem != nullptr)                                    // is a preview item, but now actual underlining node.
                nodeItem->setItemStyle(&style);
        }
        component->completeCreate();
        if (!component->isError()) {
            QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
            item = qobject_cast<QQuickItem*>(object);
            item->setVisible(true);
            item->setParentItem(getContainerItem());
        } // Note: There is no leak until cpp ownership is set
    } catch (const qan::Error& e) {
        Q_UNUSED(e)
        qWarning() << "qan::Graph::createFromComponent(): " << component->errors();
    } catch (const std::exception& e) {
        qWarning() << "qan::Graph::createFromComponent(): " << e.what();
    }
    return item;
}

QQuickItem* Graph::createFromComponent(QQmlComponent* component, qan::Style* style)
{
    return (component != nullptr &&
            style != nullptr ) ? createFromComponent( component, *style, nullptr, nullptr, nullptr ) :
                                 nullptr;
}

void Graph::setSelectionDelegate(QQmlComponent* selectionDelegate) noexcept
{
    // Note: Cpp ownership is voluntarily not set to avoid destruction of
    // objects owned from QML
    setSelectionDelegate(std::unique_ptr<QQmlComponent>(selectionDelegate));
}

void Graph::setSelectionDelegate(std::unique_ptr<QQmlComponent> selectionDelegate) noexcept
{
    auto delegateChanged = false;
    if (selectionDelegate) {
        if (selectionDelegate != _selectionDelegate) {
            _selectionDelegate = std::move(selectionDelegate);
            delegateChanged = true;
        }
    } else {    // Use QuickQanava default selection delegate
        _selectionDelegate = createComponent(QStringLiteral("qrc:/QuickQanava/SelectionItem.qml"));
        delegateChanged = true;
    }
    if (delegateChanged) {  // Update all existing delegates...
        // Note: It could be done in a more more 'generic' way!
        auto updateNodeSelectionItem = [this](auto& primitive) -> void {
            if (primitive != nullptr &&
                primitive->getItem() &&
                primitive->getItem()->getSelectionItem() != nullptr)   // Replace only existing selection items
                primitive->getItem()->setSelectionItem(this->createSelectionItem(primitive->getItem()));
        };
        auto updateGroupSelectionItem = [this](auto& primitive) -> void {
            if (primitive != nullptr &&
                primitive->getItem() &&
                primitive->getItem()->getSelectionItem() != nullptr)   // Replace only existing selection items
                primitive->getItem()->setSelectionItem(this->createSelectionItem(primitive->getItem()));
        };
        std::for_each(get_groups().begin(), get_groups().end(), updateGroupSelectionItem);
        std::for_each(get_nodes().begin(), get_nodes().end(), updateNodeSelectionItem);
        emit selectionDelegateChanged();
    }
}

QQuickItem* Graph::createSelectionItem(QQuickItem* parent)
{
    const auto edgeItem = qobject_cast<qan::EdgeItem*>(parent);
    if (edgeItem != nullptr)    // Edge selection item is managed directly in EdgeTemplate.qml
        return nullptr;
    const auto selectionItem = createItemFromComponent(_selectionDelegate.get());
    if (selectionItem != nullptr) {
        selectionItem->setEnabled(false); // Avoid node/edge/group selection problems
        selectionItem->setState("UNSELECTED");
        selectionItem->setVisible(true);
        QQmlEngine::setObjectOwnership(selectionItem, QQmlEngine::CppOwnership);
        if (parent != nullptr) {
            selectionItem->setParentItem(parent);
            selectionItem->setZ(1.0);
        }
        return selectionItem;
    }
    return nullptr;
}

std::unique_ptr<QQmlComponent>  Graph::createComponent(const QString& url)
{
    // PRECONDITIONS
        // url could not be empty
    if (url.isEmpty()) {
        qWarning() << "qan::Graph::createComponent(): Error: Empty url.";
        return std::unique_ptr<QQmlComponent>();
    }

    QQmlEngine* engine = qmlEngine(this);
    std::unique_ptr<QQmlComponent> component;
    if (engine != nullptr) {
        try {
            component = std::make_unique<QQmlComponent>(engine, url);
            if (!component->isReady()   ||
                component->isError()    ||
                component->isNull()) {
                qWarning() << "qan::Graph::createComponent(): Error while creating component from URL " << url;
                qWarning() << "\tQML Component status=" << component->status();
                qWarning() << "\tQML Component errors=" << component->errors();
            }
            return component;
        } catch (...) { // Might be std::bad_alloc
            qWarning() << "qan::Graph::createComponent(): Error while creating component " << url;
        }
    } else qWarning() << "qan::Graph::createComponent(): No access to QML engine.";

    return component;
}

QQuickItem* Graph::createItemFromComponent(QQmlComponent* component)
{
    // PRECONDITIONS:
        // component should not be nullptr, warning issued
    if (component == nullptr) {
        qWarning() << "qan::Graph::createItemFromComponent(): Error called with a nullptr delegate component.";
        return nullptr;
    }
    QQuickItem* item = nullptr;
    try {
        if (!component->isReady())
            throw qan::Error{"Error delegate component is not ready."};

        const auto rootContext = qmlContext(this);
        if (rootContext == nullptr)
            throw qan::Error{"Error can't access to local QML context."};
        QObject* object = component->beginCreate(rootContext);
        if (object == nullptr ||
            component->isError()) {
            if (object != nullptr)
                object->deleteLater();
            throw qan::Error{ "Failed to create a concrete QQuickItem from QML component:\n\t" +
                              component->errorString() };
        }
        component->completeCreate();
        if (!component->isError()) {
            QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
            item = qobject_cast<QQuickItem*>(object);
            item->setVisible(true);
            item->setParentItem(getContainerItem());
        } // Note: No leak until cpp ownership is set
    } catch (const qan::Error& e) {
        qWarning() << "qan::Graph::createItemFromComponent(): " << e.getMsg() << "\n" << component->errors();
    } catch (const std::exception& e) {
        qWarning() << "qan::Graph::createItemFromComponent(): " << e.what() << "\n" << component->errors();
    }
    return QPointer<QQuickItem>{item};
}
//-----------------------------------------------------------------------------

/* Graph Factories *///--------------------------------------------------------
auto    Graph::insertNonVisualNode(Node* node) -> bool
{
    if (node == nullptr) {
        qWarning() << "Graph::insertNonVisualNode(): Error: node is nullptr.";
        return false;
    }
    if (super_t::insert_node(node)) {
        onNodeInserted(*node);
        emit nodeInserted(node);
        return true;
    }
    return false;
}

qan::Node*  Graph::insertNode(QQmlComponent* nodeComponent, qan::NodeStyle* nodeStyle)
{
    return insertNode<qan::Node>(nodeComponent, nodeStyle);
}

bool    Graph::insertNode(Node* node, QQmlComponent* nodeComponent, qan::NodeStyle* nodeStyle)
{
    // PRECONDITIONS:
        // node must be dereferencable
        // nodeComponent and nodeStyle can be nullptr
    if (node == nullptr)
        return false;

    if (nodeComponent == nullptr)
        nodeComponent = _nodeDelegate.get(); // If no delegate component is specified, try the default node delegate
    if (nodeComponent != nullptr &&
        nodeComponent->isError()) {
        qWarning() << "qan::Graph::insertNode(): Component error: " << nodeComponent->errors();
        return false;
    }
    try {
        QQmlEngine::setObjectOwnership(node, QQmlEngine::CppOwnership);
        qan::NodeItem* nodeItem = nullptr;
        if (nodeComponent != nullptr &&
            nodeStyle != nullptr) {
            _styleManager.setStyleComponent(nodeStyle, nodeComponent);
            nodeItem = static_cast<qan::NodeItem*>(createFromComponent(nodeComponent,
                                                                       *nodeStyle,
                                                                       node));
        }
        if (nodeItem != nullptr) {
            nodeItem->setNode(node);
            nodeItem->setGraph(this);
            node->setItem(nodeItem);
            auto notifyNodeClicked = [this] (qan::NodeItem* nodeItem, QPointF p) {
                if ( nodeItem != nullptr && nodeItem->getNode() != nullptr )
                    emit this->nodeClicked(nodeItem->getNode(), p);
            };
            connect(nodeItem,   &qan::NodeItem::nodeClicked,
                    this,       notifyNodeClicked);

            auto notifyNodeRightClicked = [this] (qan::NodeItem* nodeItem, QPointF p) {
                if ( nodeItem != nullptr && nodeItem->getNode() != nullptr )
                    emit this->nodeRightClicked(nodeItem->getNode(), p);
            };
            connect(nodeItem,   &qan::NodeItem::nodeRightClicked,
                    this,       notifyNodeRightClicked);

            auto notifyNodeDoubleClicked = [this] (qan::NodeItem* nodeItem, QPointF p) {
                if ( nodeItem != nullptr && nodeItem->getNode() != nullptr )
                    emit this->nodeDoubleClicked(nodeItem->getNode(), p);
            };
            connect(nodeItem, &qan::NodeItem::nodeDoubleClicked,
                    this,     notifyNodeDoubleClicked);
            node->setItem(nodeItem);
            {   // Send item to front
                const auto z = nextMaxZ();
                nodeItem->setZ(z);
            }
        }
        super_t::insert_node(node);
    } catch (const qan::Error& e) {
        qWarning() << "qan::Graph::insertNode(): Error: " << e.getMsg();
        return false; // node eventually destroyed by shared_ptr
    }
    catch (...) {
        qWarning() << "qan::Graph::insertNode(): Error: Topology error.";
        return false; // node eventually destroyed by shared_ptr
    }
    if (node != nullptr) {       // Notify user.
        onNodeInserted(*node);
        emit nodeInserted(node);
    }
    return true;
}

bool    Graph::removeNode(qan::Node* node, bool force)
{
    // PRECONDITIONS:
        // node can't be nullptr
        // node can't be protected or locked
    if (node == nullptr)
        return false;
    if (!force &&
         (node->getIsProtected() ||
          node->getLocked()))
        return false;

    onNodeRemoved(*node);
    emit nodeRemoved(node);
    if (_selectedNodes.contains(node))
        _selectedNodes.removeAll(node);
    return super_t::remove_node(node);  // warning node pointer now invalid
}

int     Graph::getNodeCount() const noexcept { return super_t::get_node_count(); }

bool    Graph::hasNode(const qan::Node* node) const { return super_t::contains(node); }

void    Graph::onNodeInserted(qan::Node& node) { Q_UNUSED(node) /* Nil */ }

void    Graph::onNodeRemoved(qan::Node& node){ Q_UNUSED(node) /* Nil */ }
//-----------------------------------------------------------------------------

/* Graph Edge Management *///--------------------------------------------------
qan::Edge*  Graph::insertEdge(QObject* source, QObject* destination, QQmlComponent* edgeComponent)
{
    qan::Edge* edge = nullptr;
    auto sourceNode = qobject_cast<qan::Node*>(source);
    if (sourceNode != nullptr) {
        const auto destNode = qobject_cast<qan::Node*>(destination);
        if (destNode != nullptr)
            edge = insertEdge(sourceNode, destNode, edgeComponent);
        else if (qobject_cast<qan::Group*>(destination) != nullptr)
            edge = insertEdge(sourceNode, qobject_cast<qan::Group*>(destination), edgeComponent);
        else if (qobject_cast<qan::Edge*>(destination) != nullptr)
            edge = insertEdge(sourceNode, qobject_cast<qan::Edge*>(destination), edgeComponent);
    }
    if (edge != nullptr) {
        QQmlEngine::setObjectOwnership(edge, QQmlEngine::CppOwnership);
        emit edgeInserted(edge);
    } else
        qWarning() << "qan::Graph::insertEdge(): Error: Unable to find a valid insertEdge() method for arguments " << source << " and " << destination;
    return edge;
}

qan::Edge*  Graph::insertEdge(qan::Node* source, qan::Node* destination, QQmlComponent* edgeComponent)
{
    // PRECONDITION;
        // source and destination can't be nullptr
    if (source == nullptr ||
        destination == nullptr)
        return nullptr;
    return insertEdge<qan::Edge>(*source, destination, edgeComponent);
}

void    Graph::bindEdgeSource(qan::Edge* edge, qan::PortItem* outPort) noexcept
{
    // PRECONDITIONS:
        // edge and outport must be non nullptr
    if ( edge == nullptr ||
         outPort == nullptr )
        return;
    bindEdgeSource(*edge, *outPort);
}

void    Graph::bindEdgeDestination(qan::Edge* edge, qan::PortItem* inPort ) noexcept
{
    // PRECONDITIONS:
        // edge and outport must be non nullptr
    if ( edge == nullptr ||
         inPort == nullptr )
        return;
    bindEdgeDestination(*edge, *inPort);
}

void    Graph::bindEdge(qan::Edge* edge, qan::PortItem* outPort, qan::PortItem* inPort ) noexcept
{
    bindEdgeDestination(edge, inPort);
    bindEdgeSource(edge, outPort );
}

bool    Graph::isEdgeSourceBindable( const qan::PortItem& outPort) const noexcept
{
    // To allow an edge source to be binded to a port, we must have an outport
    if ( outPort.getType() != qan::PortItem::Type::Out &&
         outPort.getType() != qan::PortItem::Type::InOut )
        return false;

    // Do not connect an edge to a port that has Single multiplicity and
    // already has an in edge
    if ( outPort.getMultiplicity() == qan::PortItem::Multiplicity::Multiple )
        return true;    // Fast exit
    if ( outPort.getMultiplicity() == qan::PortItem::Multiplicity::Single ) {
        const auto outPortInDegree = outPort.getOutEdgeItems().size();
        if ( outPortInDegree == 0 )
            return true;
    }
    return false;
}

bool    Graph::isEdgeDestinationBindable(const qan::PortItem& inPort) const noexcept
{
    // To allow an edge destination to be binded to a port, we must have an in port
    if (inPort.getType() != qan::PortItem::Type::In &&
        inPort.getType() != qan::PortItem::Type::InOut)
        return false;

    // Do not connect an edge to a port that has Single multiplicity and
    // already has an in edge
    if (inPort.getMultiplicity() == qan::PortItem::Multiplicity::Multiple)
        return true;    // Fast exit
    if (inPort.getMultiplicity() == qan::PortItem::Multiplicity::Single) {
        const auto inPortInDegree = inPort.getInEdgeItems().size();
        if (inPortInDegree == 0)
            return true;
    }
    return false;
}

void    Graph::bindEdgeSource(qan::Edge& edge, qan::PortItem& outPort) noexcept
{
    // PRECONDITION:
        // edge must have an associed item
    auto edgeItem = edge.getItem();
    if (edgeItem == nullptr)
        return;

    if (isEdgeSourceBindable(outPort)) {
        edgeItem->setSourceItem(&outPort);
        outPort.addOutEdgeItem(*edgeItem);
    }
}

void    Graph::bindEdgeDestination(qan::Edge& edge, qan::PortItem& inPort) noexcept
{
    // PRECONDITION:
        // edge must have an associed item
    auto edgeItem = edge.getItem();
    if (edgeItem == nullptr)
        return;

    if (isEdgeDestinationBindable(inPort)) {
        edgeItem->setDestinationItem(&inPort);
        inPort.addInEdgeItem(*edgeItem);
    }
}

bool    Graph::configureEdge(qan::Edge& edge, QQmlComponent& edgeComponent, qan::EdgeStyle& style,
                             qan::Node& src, qan::Node* dst)
{
    _styleManager.setStyleComponent(&style, &edgeComponent);
    auto edgeItem = qobject_cast< qan::EdgeItem* >(createFromComponent(&edgeComponent, style, nullptr, &edge));
    if (edgeItem == nullptr) {
        qWarning() << "qan::Graph::insertEdge(): Warning: Edge creation from QML delegate failed.";
        return false;
    }
    edge.setItem(edgeItem);
    edgeItem->setSourceItem(src.getItem());
    if (dst != nullptr)
        edgeItem->setDestinationItem(dst->getItem());

    edge.set_src(&src);
    if (dst != nullptr)
        edge.set_dst(dst);

    auto notifyEdgeClicked = [this] (qan::EdgeItem* edgeItem, QPointF p) {
        if (edgeItem != nullptr && edgeItem->getEdge() != nullptr)
            emit this->edgeClicked(edgeItem->getEdge(), p);
    };
    connect(edgeItem, &qan::EdgeItem::edgeClicked,
            this,     notifyEdgeClicked);

    auto notifyEdgeRightClicked = [this] (qan::EdgeItem* edgeItem, QPointF p) {
        if (edgeItem != nullptr && edgeItem->getEdge() != nullptr)
            emit this->edgeRightClicked(edgeItem->getEdge(), p);
    };
    connect(edgeItem,   &qan::EdgeItem::edgeRightClicked,
            this,       notifyEdgeRightClicked);

    auto notifyEdgeDoubleClicked = [this] (qan::EdgeItem* edgeItem, QPointF p) {
        if (edgeItem != nullptr && edgeItem->getEdge() != nullptr)
            emit this->edgeDoubleClicked(edgeItem->getEdge(), p);
    };
    connect(edgeItem, &qan::EdgeItem::edgeDoubleClicked,
            this,     notifyEdgeDoubleClicked);
    return true;
}

bool    Graph::removeEdge(qan::Node* source, qan::Node* destination) {
    return super_t::remove_edge(source, destination);
}
bool    Graph::removeEdge(qan::Edge* edge, bool force) {
    if (edge == nullptr)
        return false;
    if (!force &&
        (edge->getIsProtected() ||
         edge->getLocked()))
        return false;
    _selectedEdges.removeAll(edge);
    emit onEdgeRemoved(edge);
    return super_t::remove_edge(edge);
}

bool    Graph::hasEdge(const qan::Node* source, const qan::Node* destination) const
{
    if (source == nullptr || destination == nullptr)
        return false;
    return super_t::has_edge(source, destination);
}

bool    Graph::hasEdge(const qan::Edge* edge) const { return hasEdge(edge->get_src(), edge->get_dst()); }
//-----------------------------------------------------------------------------

/* Graph Group Management *///-------------------------------------------------
qan::Group* Graph::insertGroup(QQmlComponent* groupComponent)
{
    return insertGroup<qan::Group>(groupComponent);
}

qan::Group* Graph::insertTable(int cols, int rows)
{
    return insertTable<qan::TableGroup>(cols, rows);
}

bool    Graph::insertGroup(Group* group, QQmlComponent* groupComponent, qan::NodeStyle* groupStyle)
{
    // PRECONDITIONS:
        // group can't be nullptr
        // groupComponent and groupStyle can be nullptr
    if (group == nullptr)
        return false;
    QQmlEngine::setObjectOwnership(group, QQmlEngine::CppOwnership);

    qan::GroupItem* groupItem = nullptr;
    if (groupComponent == nullptr)
        groupComponent = _groupDelegate.get();

    if (groupStyle == nullptr)
        groupStyle = qobject_cast<qan::NodeStyle*>(qan::Group::style());
    if (groupStyle != nullptr &&
        groupComponent != nullptr) {
        // FIXME: Group styles are still not well supported (20170317)
        //_styleManager.setStyleComponent(style, edgeComponent);
        groupItem = static_cast<qan::GroupItem*>(createFromComponent(groupComponent,
                                                                     *groupStyle,
                                                                     nullptr,
                                                                     nullptr, group));
    }

    if (!super_t::insert_group(group))
        qWarning() << "qan::Graph::insertGroup(): Error: Internal topology error.";

    if (groupItem != nullptr) {  // groupItem shouldn't be null, but throw only a warning to run concrete unit tests.
        groupItem->setGroup(group);
        groupItem->setGraph(this);
        group->setItem(groupItem);

        auto notifyGroupClicked = [this] (qan::GroupItem* groupItem, QPointF p) {
            if (groupItem != nullptr && groupItem->getGroup() != nullptr)
                emit this->groupClicked(groupItem->getGroup(), p);
        };
        connect(groupItem,  &qan::GroupItem::groupClicked,
                this,       notifyGroupClicked);

        auto notifyGroupRightClicked = [this] (qan::GroupItem* groupItem, QPointF p) {
            if (groupItem != nullptr && groupItem->getGroup() != nullptr)
                emit this->groupRightClicked(groupItem->getGroup(), p);
        };
        connect(groupItem, &qan::GroupItem::groupRightClicked,
                this,      notifyGroupRightClicked);

        auto notifyGroupDoubleClicked = [this] (qan::GroupItem* groupItem, QPointF p) {
            if (groupItem != nullptr && groupItem->getGroup() != nullptr)
                emit this->groupDoubleClicked(groupItem->getGroup(), p);
        };
        connect(groupItem, &qan::GroupItem::groupDoubleClicked,
                this,      notifyGroupDoubleClicked);

        { // Send group item to front
            const auto z = nextMaxZ();
            groupItem->setZ(z);
        }
    }
    if (group != nullptr) {       // Notify user.
        onNodeInserted(*group);
        emit nodeInserted(group);
    }

    return true;
}

void    Graph::removeGroup(qan::Group* group, bool removeContent, bool force)
{
    if (group == nullptr)
        return;
    if (!force &&
        (group->getIsProtected() ||
         group->getLocked()))
        return;

    if (!removeContent) {
        // Reparent all group childrens (ie node) to graph before destructing
        // the group otherwise all child items get destructed too
        for (auto node : group->get_nodes()) {
            auto qanNode = qobject_cast<qan::Node*>(node);
            if (qanNode != nullptr &&
                qanNode->getItem() != nullptr &&
                group->getGroupItem() != nullptr )
                group->getGroupItem()->ungroupNodeItem(qanNode->getItem());
        }

        onNodeRemoved(*group);      // group are node, notify group
        emit nodeRemoved(group);    // removed as a node

        if (_selectedNodes.contains(group))
            _selectedNodes.removeAll(group);
        if (_selectedGroups.contains(group))
            _selectedGroups.removeAll(group);
        remove_group(group);
    } else {
        removeGroupContent_rec(group);
    }
}

void    Graph::removeGroupContent_rec(qan::Group* group)
{
    // Remove group sub group and node, starting from leafs
    for (auto& subNode : group->get_nodes()) {
        const auto qanSubNode = qobject_cast<qan::Node*>(subNode);
        if (qanSubNode == nullptr)
            continue;
        if (qanSubNode->isGroup()) {
            const auto qanSubGroup = qobject_cast<qan::Group*>(subNode);
            removeGroupContent_rec(qanSubGroup);
        } else
            removeNode(qanSubNode);
    }

    onNodeRemoved(*group);      // group are node, notify group
    emit nodeRemoved(group);    // removed as a node

    if (_selectedNodes.contains(group))
        _selectedNodes.removeAll(group);

    super_t::remove_group(group);
}

bool    Graph::hasGroup(qan::Group* group) const
{
    if (group == nullptr)
        return false;
    return super_t::has_group(group);
}

bool    qan::Graph::groupNode(qan::Group* group, qan::Node* node, qan::TableCell* groupCell, bool transform) noexcept
{
    //qWarning() << "qan::Graph::groupNode(): group=" << group << "  node=" << node << "  groupCell=" << groupCell;
    // PRECONDITIONS:
        // group and node can't be nullptr
    if (group == nullptr ||
        node == nullptr)
        return false;
    if (static_cast<const QObject*>(group) == static_cast<const QObject*>(node)) {
        qWarning() << "qan::Graph::groupNode(): Error, can't group a group in itself.";
        return false;
    }
    try {
        super_t::group_node(node, group);
        if (node->get_group() == group &&  // Check that group insertion succeed
            group->getGroupItem() != nullptr &&
            node->getItem() != nullptr ) {
            group->getGroupItem()->groupNodeItem(node->getItem(), groupCell, transform);
            emit nodeGrouped(node, group);
        }
        return true;
    } catch (...) { qWarning() << "qan::Graph::groupNode(): Topology error."; }
    return false;
}

bool    qan::Graph::ungroupNode(qan::Node* node, qan::Group* group, bool transform) noexcept
{
    // PRECONDITIONS:
        // node can't be nullptr
        // group can be nullptr
        // if group is nullptr node->getGroup() can't be nullptr
        // if group is not nullptr group should not be different from node->getGroup()
    if (node == nullptr)
        return false;
    if (group == nullptr &&
        node->get_group() == nullptr)
        return false;
    if (group != nullptr &&
        group != node->get_group())
        return false;
    group = node->get_group();
    if (group != nullptr &&
        node != nullptr) {
        try {
            if (group->getGroupItem())
                group->getGroupItem()->ungroupNodeItem(node->getItem(), transform);
            super_t::ungroup_node(node, group);
            emit nodeUngrouped(node, group);
            const auto tableGroup = qobject_cast<qan::TableGroup*>(group);
            if (tableGroup != nullptr)  // Note: Specific handling of table, table maintain
                emit tableModified(tableGroup);  // reference to node in it's cell, force an update
            if (node != nullptr &&
                node->getItem() != nullptr) {
                // Update node z to maxZ: otherwise an undroupped node might be behind it's host group.
                const auto z = nextMaxZ();
                node->getItem()->setZ(z);
            }
            return true;
        } catch (...) { qWarning() << "qan::Graph::ungroupNode(): Topology error."; }
    }
    return false;
}
//-----------------------------------------------------------------------------


/* Selection Management *///---------------------------------------------------
void    Graph::setSelectionPolicy(SelectionPolicy selectionPolicy)
{
    if (selectionPolicy == _selectionPolicy)  // Binding loop protection
        return;
    _selectionPolicy = selectionPolicy;
    if (selectionPolicy == SelectionPolicy::NoSelection)
        clearSelection();
    emit selectionPolicyChanged();
}

auto    Graph::setMultipleSelectionEnabled(bool multipleSelectionEnabled) -> bool
{
    if (multipleSelectionEnabled != _multipleSelectionEnabled) {
        _multipleSelectionEnabled = multipleSelectionEnabled;
        if (multipleSelectionEnabled == false)
            clearSelection();
        emit multipleSelectionEnabledChanged();
        return true;
    }
    return false;
}

void    Graph::setSelectionColor(QColor selectionColor) noexcept
{
    if (selectionColor != _selectionColor) {
        _selectionColor = selectionColor;
        configureSelectionItems();
        emit selectionColorChanged();
    }
}

void    Graph::setSelectionWeight(qreal selectionWeight) noexcept
{
    if (!qFuzzyCompare(1. + selectionWeight, 1. + _selectionWeight)) {
        _selectionWeight = selectionWeight;
        configureSelectionItems();
        emit selectionWeightChanged();
    }
}

void    Graph::setSelectionMargin(qreal selectionMargin) noexcept
{
    if (!qFuzzyCompare(1.0 + selectionMargin, 1.0 + _selectionMargin)) {
        _selectionMargin = selectionMargin;
        configureSelectionItems();
        emit selectionMarginChanged();
    }
}

void    Graph::configureSelectionItems() noexcept
{
    // PRECONDITIONS: None
    for (auto node : _selectedNodes)
        if (node != nullptr &&
            node->getItem() != nullptr)
            node->getItem()->configureSelectionItem();
    for (auto group : _selectedGroups)
        if (group != nullptr &&
            group->getItem() != nullptr)
            group->getItem()->configureSelectionItem();
}

namespace impl { // qan::impl

// Generic utility to select either node or groups (is primitive with a qan::Selectable interface)
template <class Primitive_t>
bool    selectPrimitive(Primitive_t& primitive,
                        Qt::KeyboardModifiers modifiers,
                        qan::Graph& graph)
{
    if (graph.getSelectionPolicy() == qan::Graph::SelectionPolicy::NoSelection)
        return false;

    bool primitiveSelected = false;
    const bool ctrlPressed = modifiers & Qt::ControlModifier;
    if (primitive.getItem() == nullptr)
        return false;
    // Note 20230408: Primitive with `isProtected` or `locked` status can be selected, it's up to the user
    // to protect them accordingly in user code.
    if (primitive.getLocked())
        return false;

    if (primitive.getItem()->getSelected()) {
        if (ctrlPressed)          // Click on a selected node + CTRL = deselect node
            primitive.getItem()->setSelected(false);
            // Note: graph.removeFromSelection() is called from primitive qan::Selectable::setSelected()
    } else {
        switch (graph.getSelectionPolicy()) {
        case qan::Graph::SelectionPolicy::SelectOnClick:
            primitiveSelected = true;        // Click on an unselected node with SelectOnClick = select node
            if (!ctrlPressed)
                graph.clearSelection();
            break;
        case qan::Graph::SelectionPolicy::SelectOnCtrlClick:
            primitiveSelected = ctrlPressed; // Click on an unselected node with CTRL pressed and SelectOnCtrlClick = select node
            break;
        case qan::Graph::SelectionPolicy::NoSelection: break;
        }
    }
    if (primitiveSelected) {
        if (!graph.getMultipleSelectionEnabled())
            graph.clearSelection();
        graph.addToSelection(primitive);
        return true;
    }
    return false;
}


// Generic utility to set selection state for either node or groups (is primitive with a qan::Selectable interface)
template <class Primitive_t>
void    setPrimitiveSelected(Primitive_t& primitive,
                             bool selected,
                             qan::Graph& graph)
{
    // Note: graph.selectionPolicy is not taken into account
    if (primitive.getItem() == nullptr)
        return;
    if (selected &&             // Allow only "deselect" for locked primitives
        primitive.getLocked())
        return;
    primitive.getItem()->setSelected(selected); // Note: graph.removeFromSelection() is called from primitive qan::Selectable::setSelected()
    if (selected)
        graph.addToSelection(primitive);
    return;
}

} // qan::impl

bool    Graph::selectNode(qan::Node& node, Qt::KeyboardModifiers modifiers) { return impl::selectPrimitive<qan::Node>(node, modifiers, *this); }
bool    Graph::selectNode(qan::Node* node, Qt::KeyboardModifiers modifiers) { return (node != nullptr ? selectNode(*node, modifiers) : false); }
bool    Graph::selectNode(qan::Node* node) { return (node != nullptr ? selectNode(*node) : false); }

void    Graph::setNodeSelected(qan::Node& node, bool selected)
{
    if (node.isGroup())
        impl::setPrimitiveSelected<qan::Group>(dynamic_cast<qan::Group&>(node), selected, *this);
    else
        impl::setPrimitiveSelected<qan::Node>(node, selected, *this);
}

void    Graph::setNodeSelected(qan::Node* node, bool selected)
{
    if (node == nullptr)
        return;
    if (node->isGroup())
        impl::setPrimitiveSelected<qan::Group>(dynamic_cast<qan::Group&>(*node), selected, *this);
    else
        impl::setPrimitiveSelected<qan::Node>(*node, selected, *this);
}

bool    Graph::selectGroup(qan::Group& group, Qt::KeyboardModifiers modifiers) { return impl::selectPrimitive<qan::Group>(group, modifiers, *this); }
bool    Graph::selectGroup(qan::Group* group) { return group != nullptr ? selectGroup(*group) : false; }

void    Graph::setEdgeSelected(qan::Edge* edge, bool selected)
{
    if (edge == nullptr)
        return;
    impl::setPrimitiveSelected<qan::Edge>(*edge, selected, *this);
}

bool    Graph::selectEdge(qan::Edge& edge, Qt::KeyboardModifiers modifiers) { return impl::selectPrimitive<qan::Edge>(edge, modifiers, *this); }
bool    Graph::selectEdge(qan::Edge* edge) { return edge != nullptr ? selectEdge(*edge) : false; }

template <class Primitive_t>
void    addToSelectionImpl(QPointer<Primitive_t> primitive,
                        qcm::Container<std::vector, QPointer<Primitive_t>>& selectedPrimitives,
                           qan::Graph& graph)
{
    if (!primitive)
        return;
    const auto primitiveItem = primitive->getItem();
    if (!primitiveItem)
        return;
    if (!selectedPrimitives.contains(primitive.data())) {
        selectedPrimitives.append(primitive);

        QObject::connect(primitive.data(),  &Primitive_t::destroyed,
                         &graph,            [&selectedPrimitives, primitive]() {
                             selectedPrimitives.removeAll(primitive.data());
                         });
        primitiveItem->setSelected(true);
        // Eventually, create and configure node item selection item
        if (primitiveItem->getSelectionItem() == nullptr)
            primitiveItem->setSelectionItem(graph.createSelectionItem(primitiveItem));   // Safe, any argument might be nullptr
        // Note 20220329: primitive.getItem()->configureSelectionItem() is called from setSelectionItem()
    }
}

void    Graph::addToSelection(qan::Node& node) {
    addToSelectionImpl<qan::Node>(QPointer<qan::Node>(&node), _selectedNodes, *this);
    emit selectionChanged();
}
void    Graph::addToSelection(qan::Group& group) {
    addToSelectionImpl<qan::Group>(&group, _selectedGroups, *this);
    emit selectionChanged();
}
void    Graph::addToSelection(qan::Edge& edge)
{
    // Note 20221002: Do not use addToSelectionImpl<>() since it has no support for QVector<QPointer<qan::Edge>>
    if (!_selectedEdges.contains(&edge)) {
        _selectedEdges.append(QPointer<qan::Edge>{&edge});
        if (edge.getItem() != nullptr) {
            edge.getItem()->setSelected(true);
            // Eventually, create and configure node item selection item
            if (edge.getItem()->getSelectionItem() == nullptr)
                edge.getItem()->setSelectionItem(createSelectionItem(edge.getItem()));   // Safe, any argument might be nullptr
            // Note 20220329: primitive.getItem()->configureSelectionItem() is called from setSelectionItem()
        }
        emit selectionChanged();
    }
}

template <class Primitive_t>
void    removeFromSelectionImpl(QPointer<Primitive_t> primitive,
                                qcm::Container<std::vector, QPointer<Primitive_t>>& selectedPrimitives)
{
    if (selectedPrimitives.contains(primitive.data()))
        selectedPrimitives.removeAll(primitive.data());
}

void    Graph::removeFromSelection(qan::Node& node) {
    removeFromSelectionImpl<qan::Node>(&node, _selectedNodes);
    emit selectionChanged();
}
void    Graph::removeFromSelection(qan::Group& group) {
    removeFromSelectionImpl<qan::Group>(&group, _selectedGroups);
    emit selectionChanged();
}

// Note: Called from
void    Graph::removeFromSelection(QQuickItem* item) {
    const auto nodeItem = qobject_cast<qan::NodeItem*>(item);
    if (nodeItem != nullptr &&
        nodeItem->getNode() != nullptr) {
        _selectedNodes.removeAll(nodeItem->getNode());
        emit selectionChanged();
    } else {
        const auto groupItem = qobject_cast<qan::GroupItem*>(item);
        if (groupItem != nullptr &&
            groupItem->getGroup() != nullptr) {
            _selectedGroups.removeAll(groupItem->getGroup());
            emit selectionChanged();
        } else {
            const auto edgeItem = qobject_cast<qan::EdgeItem*>(item);
            if (edgeItem != nullptr &&
                edgeItem->getEdge() != nullptr) {
                _selectedEdges.removeAll(edgeItem->getEdge());
                emit selectionChanged();
            }
        }
    }
}

void    Graph::selectAll()
{
    for (const auto node: get_nodes()) {
        if (node != nullptr)
            selectNode(*node, Qt::ControlModifier);
    }
    emit selectionChanged();
}

void    Graph::removeSelection()
{
    const auto& selectedNodes = getSelectedNodes();
    for (const auto& node: qAsConst(selectedNodes))
        if (node &&
            !node->getIsProtected() &&
            !node->getLocked())
            removeNode(node);

    const auto& selectedGroups = getSelectedGroups();
    for (const auto& group: qAsConst(selectedGroups))
        if (group &&
            !group->getIsProtected() &&
            !group->getLocked())
            removeGroup(group);

    const auto& selectedEdges = getSelectedEdges();
    for (const auto& edge: qAsConst(selectedEdges))
        if (edge &&
            !edge->getIsProtected() &&
            !edge->getLocked())
            removeEdge(edge);

    clearSelection();
}

void    Graph::clearSelection()
{
    // Note: getItem()->setSelected() actually _modify_ content
    // of _selectedNodes and _selectedGroups, a deep copy of theses
    // container is necessary to avoid iterating on a vector that
    // has changed while iterator has been modified.
    SelectedNodes selectedNodesCopy;
    std::copy(_selectedNodes.cbegin(),
               _selectedNodes.cend(),
               std::back_inserter(selectedNodesCopy));
    for (auto node : selectedNodesCopy)
        if (node != nullptr &&
            node->getItem() != nullptr)
            node->getItem()->setSelected(false);
    _selectedNodes.clear();

    SelectedGroups selectedGroupsCopy;
    std::copy(_selectedGroups.cbegin(),
              _selectedGroups.cend(),
              std::back_inserter(selectedGroupsCopy));
    for (auto group : selectedGroupsCopy)
        if (group != nullptr &&
            group->getItem() != nullptr)
            group->getItem()->setSelected(false);
    _selectedGroups.clear();

    SelectedEdges selectedEdgesCopy;
    std::copy(_selectedEdges.cbegin(),
              _selectedEdges.cend(),
              std::back_inserter(selectedEdgesCopy));
    for (auto& edge : qAsConst(selectedEdgesCopy))
        if (edge != nullptr &&
            edge->getItem() != nullptr)
            edge->getItem()->setSelected(false);
    _selectedEdges.clear();

    emit selectionChanged();
}

bool    Graph::hasSelection() const
{
    return (_selectedNodes.size() >= 1 ||
            _selectedGroups.size() >= 1 ||
            _selectedEdges.size() >= 1);
}

bool    Graph::hasMultipleSelection() const
{
    return (_selectedNodes.size() +
            _selectedGroups.size() +
            _selectedEdges.size()) > 1;
}

std::vector<QQuickItem*>    Graph::getSelectedItems() const
{
    using item_vector_t = std::vector<QQuickItem*>;
    item_vector_t items;
    items.reserve(static_cast<item_vector_t::size_type>(_selectedNodes.size() + _selectedGroups.size()));
    for (const auto& selectedNode: _selectedNodes) {
        if (selectedNode->getItem() != nullptr)
            items.push_back(selectedNode->getItem());
    }
    for (const auto& selectedGroup: _selectedGroups) {
        if (selectedGroup->getItem() != nullptr)
            items.push_back(selectedGroup->getItem());
    }
    return items;   // Expect RVA
}
//-----------------------------------------------------------------------------


/* Alignment Management *///---------------------------------------------------
bool    Graph::setSnapToGrid(bool snapToGrid) noexcept
{
    if (snapToGrid != _snapToGrid) {
        _snapToGrid = snapToGrid;
        emit snapToGridChanged();
        return true;
    }
    return false;
}
bool    Graph::setSnapToGridSize(QSizeF snapToGridSize) noexcept
{
    if (snapToGridSize != _snapToGridSize) {
        _snapToGridSize = snapToGridSize;
        emit snapToGridSizeChanged();
        return true;
    }
    return false;
}

void    Graph::alignSelectionHorizontalCenter() { alignHorizontalCenter(getSelectedItems()); }

void    Graph::alignSelectionRight() { alignRight(getSelectedItems()); }

void    Graph::alignSelectionLeft() { alignLeft(getSelectedItems()); }

void    Graph::alignSelectionTop() { alignTop(getSelectedItems()); }

void    Graph::alignSelectionBottom() { alignBottom(getSelectedItems()); }

void    Graph::alignHorizontalCenter(std::vector<QQuickItem*>&& items)
{
    if (items.size() <= 1)
        return;

    // ALGORITHM:
        // Get min left and max right.
        // Compute center of min left and max right
        // Align all items on this center
    qreal maxRight = std::numeric_limits<qreal>::lowest();
    qreal minLeft = std::numeric_limits<qreal>::max();
    for (const auto item: items) {
        maxRight = std::max(maxRight, item->x() + item->width());
        minLeft = std::min(minLeft, item->x());
    }

    qreal center = minLeft + (maxRight - minLeft) / 2.;
    for (auto item: items){
        const auto nodeItem = qobject_cast<qan::NodeItem*>(item);  // Works for qan::GroupItem*
        if (nodeItem != nullptr)
            emit nodeAboutToBeMoved(nodeItem->getNode());
        item->setX(center - (item->width() / 2.));
        if (nodeItem != nullptr)
            emit nodeMoved(nodeItem->getNode());
    }
}

void    Graph::alignRight(std::vector<QQuickItem*>&& items)
{
    if (items.size() <= 1)
        return;
    qreal maxRight = std::numeric_limits<qreal>::lowest();
    for (const auto item: items)
        maxRight = std::max(maxRight, item->x() + item->width());
    for (auto item: items) {
        const auto nodeItem = qobject_cast<qan::NodeItem*>(item);  // Works for qan::GroupItem*
        if (nodeItem != nullptr)
            emit nodeAboutToBeMoved(nodeItem->getNode());
        item->setX(maxRight - item->width());
        if (nodeItem != nullptr)
            emit nodeMoved(nodeItem->getNode());
    }
}

void    Graph::alignLeft(std::vector<QQuickItem*>&& items)
{
    if (items.size() <= 1)
        return;
    qreal minLeft = std::numeric_limits<qreal>::max();
    for (const auto item: items)
        minLeft = std::min(minLeft, item->x());
    for (auto item: items) {
        const auto nodeItem = qobject_cast<qan::NodeItem*>(item);  // Works for qan::GroupItem*
        if (nodeItem != nullptr)
            emit nodeAboutToBeMoved(nodeItem->getNode());
        item->setX(minLeft);
        if (nodeItem != nullptr)
            emit nodeMoved(nodeItem->getNode());
    }
}

void    Graph::alignTop(std::vector<QQuickItem*>&& items)
{
    if (items.size() <= 1)
        return;
    qreal minTop = std::numeric_limits<qreal>::max();
    for (const auto item: items)
        minTop = std::min(minTop, item->y());
    for (auto item: items) {
        const auto nodeItem = qobject_cast<qan::NodeItem*>(item);  // Works for qan::GroupItem*
        if (nodeItem != nullptr)
            emit nodeAboutToBeMoved(nodeItem->getNode());
        item->setY(minTop);
        if (nodeItem != nullptr)
            emit nodeMoved(nodeItem->getNode());
    }
}

void    Graph::alignBottom(std::vector<QQuickItem*>&& items)
{
    if (items.size() <= 1)
        return;
    qreal maxBottom = std::numeric_limits<qreal>::lowest();
    for (const auto item: items)
        maxBottom = std::max(maxBottom, item->y() + item->height());
    for (auto item: items) {
        const auto nodeItem = qobject_cast<qan::NodeItem*>(item);  // Works for qan::GroupItem*
        if (nodeItem != nullptr)
            emit nodeAboutToBeMoved(nodeItem->getNode());
        item->setY(maxBottom - item->height());
        if (nodeItem != nullptr)
            emit nodeMoved(nodeItem->getNode());
    }
}
//-----------------------------------------------------------------------------


/* Port/Dock Management *///---------------------------------------------------
qan::PortItem*  Graph::insertPort(qan::Node* node,
                                  qan::NodeItem::Dock dockType,
                                  qan::PortItem::Type portType,
                                  QString label,
                                  QString id) noexcept
{
    // PRECONDITIONS:
        // node can't be nullptr
        // node must have an item (to access node style)
        // default _portDelegate must be valid
    if (node == nullptr ||
        node->getItem() == nullptr)
        return nullptr;
    if (!_portDelegate) {
        qWarning() << "qan::Graph::insertPort(): no default port delegate available.";
        return nullptr;
    }

    qan::PortItem* portItem = nullptr;
    const auto nodeStyle = node->getItem()->getStyle();     // Use node style for dock item
    if (nodeStyle) {
        portItem = qobject_cast<qan::PortItem*>(createFromComponent(_portDelegate.get(), *nodeStyle ));
        // Note 20190501: CppOwnership is set in createFromComponen()
        if (portItem != nullptr) {
            portItem->setType(portType);
            portItem->setLabel(label);
            portItem->setId(id);
            portItem->setDockType(dockType);

            // Configure port mouse events forwarding to qan::Graph
            const auto notifyPortClicked = [this] (qan::NodeItem* nodeItem, QPointF p) {
                const auto portItem = qobject_cast<qan::PortItem*>(nodeItem);
                if (portItem != nullptr &&
                    portItem->getNode() != nullptr)
                    emit this->portClicked(portItem, p);
            };
            connect(portItem, &qan::NodeItem::nodeClicked,
                    this,     notifyPortClicked );

            const auto notifyPortRightClicked = [this] (qan::NodeItem* nodeItem, QPointF p) {
                const auto portItem = qobject_cast<qan::PortItem*>(nodeItem);
                if (portItem != nullptr &&
                    portItem->getNode() != nullptr)
                    emit this->portRightClicked(portItem, p);
            };
            connect(portItem,   &qan::NodeItem::nodeRightClicked,
                    this,       notifyPortRightClicked);

            if (node->getItem() != nullptr) {
                portItem->setNode(node); // portitem node in fact map to this concrete node.
                node->getItem()->getPorts().append(portItem);
                auto dockItem = node->getItem()->getDock(dockType);
                if (dockItem == nullptr) {
                    // Create a dock item from the default dock delegate
                    dockItem = createDockFromDelegate(dockType, *node);
                    if (dockItem != nullptr)
                        node->getItem()->setDock(dockType, dockItem);
                }
                if (dockItem != nullptr)
                    portItem->setParentItem(dockItem);
                else {
                    portItem->setParentItem(node->getItem());
                    portItem->setZ(1.5);    // 1.5 because port item should be on top of selection item and under node resizer (selection item z=1.0, resizer z=2.0)
                }
            }
        }
    }
    return portItem;
}

void    Graph::removePort(qan::Node* node, qan::PortItem* port) noexcept
{
    if ( node == nullptr ||             // PRECONDITION: node must have a graphical, non visual nodes can't have ports
         node->getItem() == nullptr )
        return;
    if ( port == nullptr )              // PRECONDITION: port can't be nullptr
        return;

    auto removeConnectEdge = [this, port](auto edge) {
        if (edge != nullptr &&
            edge->getItem() != nullptr &&
            (edge->getItem()->getSourceItem() == port ||
             edge->getItem()->getDestinationItem() == port ))
            this->removeEdge(edge);
    };
    std::for_each(node->get_in_edges().begin(), node->get_in_edges().end(), removeConnectEdge);
    std::for_each(node->get_out_edges().begin(), node->get_out_edges().end(), removeConnectEdge);

    auto& ports = node->getItem()->getPorts();
    if (ports.contains(port))
        ports.removeAll(port);
    port->deleteLater();        // Note: port is owned by ports qcm::Container
}

void    Graph::qmlSetPortDelegate(QQmlComponent* portDelegate) noexcept
{
    if ( portDelegate != _portDelegate.get() ) {
        if ( portDelegate != nullptr )
            QQmlEngine::setObjectOwnership( portDelegate, QQmlEngine::CppOwnership );
        _portDelegate.reset(portDelegate);
        emit portDelegateChanged();
    }
}
void    Graph::setPortDelegate(std::unique_ptr<QQmlComponent> portDelegate) noexcept { qmlSetPortDelegate(portDelegate.release()); }

void    Graph::setHorizontalDockDelegate(QQmlComponent* horizontalDockDelegate) noexcept
{
    if ( horizontalDockDelegate != nullptr ) {
        if ( horizontalDockDelegate != _horizontalDockDelegate.get() ) {
            _horizontalDockDelegate.reset(horizontalDockDelegate);
            QQmlEngine::setObjectOwnership( horizontalDockDelegate, QQmlEngine::CppOwnership );
            emit horizontalDockDelegateChanged();
        }
    }
}
void    Graph::setHorizontalDockDelegate(std::unique_ptr<QQmlComponent> horizontalDockDelegate) noexcept { setHorizontalDockDelegate(horizontalDockDelegate.release()); }

void    Graph::setVerticalDockDelegate(QQmlComponent* verticalDockDelegate) noexcept
{
    if ( verticalDockDelegate != nullptr ) {
        if ( verticalDockDelegate != _verticalDockDelegate.get() ) {
            _verticalDockDelegate.reset(verticalDockDelegate);
            QQmlEngine::setObjectOwnership( verticalDockDelegate, QQmlEngine::CppOwnership );
            emit verticalDockDelegateChanged();
        }
    }
}
void    Graph::setVerticalDockDelegate(std::unique_ptr<QQmlComponent> verticalDockDelegate) noexcept { setVerticalDockDelegate(verticalDockDelegate.release()); }

QPointer<QQuickItem> Graph::createDockFromDelegate(qan::NodeItem::Dock dock, qan::Node& node) noexcept
{
    using Dock = qan::NodeItem::Dock;
    if (dock == Dock::Left ||
        dock == Dock::Right) {
        if (_verticalDockDelegate) {
            auto verticalDock = createItemFromComponent(_verticalDockDelegate.get());
            verticalDock->setParentItem(node.getItem());
            verticalDock->setProperty("hostNodeItem",
                                      QVariant::fromValue(node.getItem()));
            verticalDock->setProperty("dockType",
                                      QVariant::fromValue(dock));
            return verticalDock;
        }
    } else if (dock == Dock::Top ||
               dock == Dock::Bottom) {
        if (_horizontalDockDelegate) {
            auto horizontalDock = createItemFromComponent(_horizontalDockDelegate.get());
            horizontalDock->setParentItem(node.getItem());
            horizontalDock->setProperty("hostNodeItem",
                                        QVariant::fromValue(node.getItem()));
            horizontalDock->setProperty("dockType",
                                        QVariant::fromValue(dock));
            return horizontalDock;
        }
    }
    return QPointer<QQuickItem>{nullptr};
}
//-----------------------------------------------------------------------------

/* Stacking Management *///----------------------------------------------------
void    Graph::sendToFront(QQuickItem* item)
{
    if (item == nullptr)
        return;

    qan::GroupItem* groupItem = qobject_cast<qan::GroupItem*>(item);
    qan::NodeItem* nodeItem = qobject_cast<qan::NodeItem*>(item);
    if (nodeItem == nullptr)
        return;     // item must be a nodeItem or a groupItem

    // Algorithm:
        // 1. If item is an ungrouped node OR a root group: update maxZ and set item.z to maxZ.
        // 2. If item is a group (or is a node inside a group):
            // 2.1 Collect all parents groups.
            // 2.2 For all parents groups: get group parent item childs maximum (local) z, update group z to maximum value + 1.

    // Return a vector groups ordered from the outer group to the root group
    const auto collectParentGroups_rec = [](qan::GroupItem* groupItem) -> std::vector<qan::GroupItem*> {

        const auto impl = [](std::vector<qan::GroupItem*>& groups,      // Recursive implementation
                             qan::GroupItem* groupItem,
                             const auto& self) -> void {
            if (groupItem == nullptr)
                return;
            groups.push_back(groupItem);
            const auto parentGroup = groupItem->getGroup() != nullptr ? groupItem->getGroup()->getGroup() :
                                                                        nullptr;
            auto parentGroupItem = parentGroup != nullptr ? parentGroup->getGroupItem() : nullptr;
            if (parentGroupItem != nullptr)
                self(groups, parentGroupItem, self);
        };

        std::vector<qan::GroupItem*> groups;
        if (groupItem == nullptr)   // PRECONDITIONS: groupItem can't be nullptr
            return groups;
        impl(groups, groupItem, impl);
        return groups;
    };

    const auto graphContainerItem = getContainerItem();
    if (graphContainerItem == nullptr) {
        qWarning() << "qan::Graph::sendToFront(): Can't sent an item to front in a graph with no container item.";
        return;
    }

    if (nodeItem != nullptr &&      // 1. If item is an ungrouped node OR a root group: update maxZ and set item.z to maxZ.
        groupItem == nullptr) {
        nodeItem->setZ(nextMaxZ());
    } else if (groupItem != nullptr &&      // 1.
               groupItem->parentItem() == graphContainerItem ) {
        groupItem->setZ(nextMaxZ());
    } else if (groupItem != nullptr) {
        // 2. If item is a group (or is a node inside a group)
        const auto groups = collectParentGroups_rec(groupItem);       // 2.1 Collect all parents groups.
        for (const auto groupItem : groups) {
            if (groupItem == nullptr)
                continue;
            const auto groupParentItem = groupItem->parentItem();
            if (groupParentItem == nullptr)
                continue;       // Should not happen, a group necessary have a parent item.
            groupItem->setZ(nextMaxZ());
        } // For all group items
    }
}

void    Graph::sendToBack(QQuickItem* item)
{
    if (item == nullptr)
        return;
    qan::NodeItem* nodeItem = qobject_cast<qan::NodeItem*>(item);
    if (nodeItem == nullptr)
        return;     // item must be a nodeItem or a groupItem (qan::GroupItem is a qan::NodeItem)
    // Note 20240522: For sendToBack(), there is no need to be as agressive as in sendToFront(),
    // juste set the node/group z to minimal z, there is no need to host group to back.
    const auto z = nextMinZ();
    nodeItem->setZ(z);
}

void    Graph::updateMinMaxZ() noexcept
{
    {
        auto maxZIt = std::max_element(get_nodes().cbegin(), get_nodes().cend(),
                                             [](const auto a, const auto b) {
                                                 return a->getItem() != nullptr &&
                                                        b->getItem() != nullptr &&
                                                        a->getItem()->z() < b->getItem()->z();
                                             });

        const auto maxZ = maxZIt != get_nodes().end() &&
                          (*maxZIt)->getItem() != nullptr ? (*maxZIt)->getItem()->z() : 0.;
        setMaxZ(maxZ);
    }

    {
        auto minZIt = std::min_element(get_nodes().cbegin(), get_nodes().cend(),
                                       [](const auto a, const auto b) {
                                           return a->getItem() != nullptr &&
                                                  b->getItem() != nullptr &&
                                                  a->getItem()->z() < b->getItem()->z();
                                       });

        const auto minZ = minZIt != get_nodes().end() &&
                                  (*minZIt)->getItem() != nullptr ? (*minZIt)->getItem()->z() : 0.;

        setMinZ(minZ);
    }
}

qreal   Graph::getMaxZ() const noexcept { return _maxZ; }
void    Graph::setMaxZ(const qreal maxZ) noexcept
{
    _maxZ = std::min(200000., maxZ);
    emit maxZChanged();
}

qreal   Graph::nextMaxZ() noexcept
{
    _maxZ += 1.;
    emit maxZChanged();
    return _maxZ;
}

void    Graph::updateMaxZ(const qreal z) noexcept
{
    if (z > _maxZ)
        setMaxZ(z);
}

qreal   Graph::getMinZ() const noexcept { return _minZ; }
void    Graph::setMinZ(const qreal minZ) noexcept
{
    _minZ = std::max(-200000., minZ);
    emit minZChanged();
}

qreal   Graph::nextMinZ() noexcept
{
    _minZ -= 1.;
    emit minZChanged();
    return _minZ;
}

void    Graph::updateMinZ(const qreal z) noexcept
{
    if (z < _minZ)
        setMinZ(z);
}
//-----------------------------------------------------------------------------


/* Topology Algorithms *///----------------------------------------------------
std::vector<QPointer<const qan::Node>>  Graph::collectRootNodes() const noexcept
{
    std::vector<QPointer<const qan::Node>> roots;
    auto& rootNodes = get_root_nodes();
    roots.reserve(static_cast<unsigned long>(rootNodes.size()));
    for (const auto rootNode : rootNodes)
        roots.push_back(QPointer<const qan::Node>(rootNode));
    return roots;
}

std::vector<const qan::Node*>   Graph::collectDfs(bool collectGroup) const noexcept
{
    std::vector<const qan::Node*> nodes;
    std::unordered_set<const qan::Node*> marks;
    auto& rootNodes = get_root_nodes();
    for (const auto rootNode : rootNodes)
        collectDfsRec(rootNode, marks,
                      nodes, collectGroup);
    return nodes;
}

std::vector<const qan::Node*>   Graph::collectDfs(const qan::Node& node, bool collectGroup) const noexcept
{
    std::vector<const qan::Node*> childs;
    std::unordered_set<const qan::Node*> marks;
    if (collectGroup &&
        node.isGroup()) {
        const auto group = qobject_cast<const qan::Group*>(&node);
        if (group != nullptr) {
            for (const auto groupNode : group->get_nodes())
                collectDfsRec(qobject_cast<qan::Node*>(groupNode), marks,
                              childs, collectGroup);
        }
    }
    for (const auto& outNode : node.get_out_nodes())
        collectDfsRec(qobject_cast<qan::Node*>(outNode), marks,
                      childs, collectGroup);
    return childs;
}

auto    Graph::collectSubNodes(const QVector<qan::Node*> nodes, bool collectGroup) const noexcept -> std::unordered_set<const qan::Node*>
{
    std::unordered_set<const qan::Node*> r;
    for (const auto node: nodes) {
        if (node == nullptr)
            continue;
        std::vector<const qan::Node*> subNodes = collectDfs(*node, collectGroup);
        if (subNodes.size() > 0)
            r.insert(subNodes.cbegin(), subNodes.cend());
    }
    return r;
}

void    Graph::collectDfsRec(const qan::Node* node,
                             std::unordered_set<const qan::Node*>& marks,
                             std::vector<const qan::Node*>& childs,
                             bool collectGroup) const noexcept
{
    if (node == nullptr)
        return;
    if (marks.find(node) != marks.end())    // Do not collect on already visited
        return;                             // branchs
    marks.insert(node);
    childs.push_back(node);
    if (collectGroup &&
        node->isGroup()) {
        const auto group = qobject_cast<const qan::Group*>(node);
        if (group) {
            for (const auto groupNode : group->get_nodes())
                collectDfsRec(qobject_cast<const qan::Node*>(groupNode), marks,
                              childs, collectGroup);
        }
    }
    for (const auto outNode : node->get_out_nodes()) {
        collectDfsRec(qobject_cast<const qan::Node*>(outNode), marks,
                      childs, collectGroup);
    }
}

auto    Graph::collectInnerEdges(const std::vector<const qan::Node*>& nodes) const -> std::unordered_set<const qan::Edge*>
{
    // Algorithm:
        // 0. Index nodes
        // 1. For every nodes:
            // 1.1 Collect all out edge where dst is part of nodes
            // 1.2 Collect all in edge where src is part of nodes
    std::unordered_set<const qan::Edge*>  innerEdges;
    if (nodes.size() == 0)
        return innerEdges;
    std::unordered_set<const qan::Node*>  nodesSet(nodes.cbegin(), nodes.cend());

    std::unordered_set<qan::Edge*>  edges;
    for (const auto node: nodes) {
        const auto& inEdges = node->get_in_edges();
        for (const auto inEdge: inEdges) {
            if (inEdge != nullptr &&
                nodesSet.find(inEdge->getDestination()) != nodesSet.end())
                innerEdges.insert(inEdge);
        }

        const auto& outEdges = node->get_out_edges();
        for (const auto outEdge: outEdges) {
            if (outEdge != nullptr &&
                nodesSet.find(outEdge->getSource()) != nodesSet.end())
                innerEdges.insert(outEdge);
        }
    }
    return innerEdges;
}

std::vector<const qan::Node*>   Graph::collectNeighbours(const qan::Node& node) const
{
    const auto collectNeighboursDfs_rec = [](const qan::Node* visited,
            std::vector<const qan::Node*>& neighbours,
            std::unordered_set<const qan::Node*>& marks,
            const auto& lambda) {
        if (visited == nullptr)
            return;
        if (marks.find(visited) != marks.end())    // Do not collect on already visited
            return;                                // branchs
        marks.insert(visited);
        neighbours.push_back(visited);

        // Collect group neighbours
        const auto group = qobject_cast<const qan::Group*>(visited);
        if (visited->isGroup() &&
            group != nullptr) {
            for (const auto groupNode : group->get_nodes())  // Collect all nodes in a group
                lambda(groupNode, neighbours, marks,
                       lambda);
        }
        // Collect group parent group neighbours
        const auto nodeGroup = visited->getGroup();
        if (nodeGroup != nullptr)
            lambda(nodeGroup, neighbours, marks,
                   lambda);
    };

    std::vector<const qan::Node*> neighbours;
    std::unordered_set<const qan::Node*> marks;
    collectNeighboursDfs_rec(&node, neighbours, marks,
                            collectNeighboursDfs_rec);

    return neighbours;
}

std::vector<const qan::Node*>   Graph::collectGroups(const qan::Node& node) const
{
    std::unordered_set<const qan::Node*> marks;
    std::vector<const qan::Node*> groups;

    const auto collectGroups_rec = [](const qan::Node* visited,
                                      std::vector<const qan::Node*>& groups,
                                      std::unordered_set<const qan::Node*>& marks,
                                      const auto& lambda) {
        if (visited == nullptr)
            return;
        if (marks.find(visited) != marks.end()) // Do not collect on already visited
            return;                             // group
        marks.insert(visited);
        if (visited->isGroup())
            groups.push_back(visited);
        if (visited->getGroup() != nullptr)
            lambda(visited->getGroup(), groups,
                   marks, lambda);
    };

    const auto group = node.getGroup();
    if (group != nullptr)
        collectGroups_rec(group, groups,
                          marks, collectGroups_rec);
    return groups;
}

std::vector<const qan::Node*>   Graph::collectAncestors(const qan::Node& node) const
{
    // ALGORITHM:
      // 0. Collect node neighbour.
      // 1. Collect ancestors of neighbors.
      // 2. Remove original neighbors from ancestors.

    const auto collectAncestorsDfs_rec = [](const qan::Node* visited,
            std::vector<const qan::Node*>& parents,
            std::unordered_set<const qan::Node*>& marks,
            const auto& lambda) {
        if (visited == nullptr)
            return;
        if (marks.find(visited) != marks.end())    // Do not collect on already visited
            return;                                // branchs
        marks.insert(visited);
        parents.push_back(visited);

        if (visited->getGroup() != nullptr)
            parents.push_back(visited->getGroup());

        // 1.1 Collect ancestor
        for (const auto inNode : visited->get_in_nodes())
            lambda(inNode, parents, marks,
                   lambda);
    };

    std::vector<const qan::Node*> parents;
    std::unordered_set<const qan::Node*> marks;
    auto excepts = collectGroups(node);
    excepts.push_back(&node);
    //qWarning() << "qan::Graph::collectAncestors(): excepts.size=" << excepts.size();

    // 0. Collect target nodes
    std::vector<const qan::Node*> targetNodes;
    if (node.isGroup()) {
        const auto neighbours = collectNeighbours(node);
        targetNodes = neighbours;
        std::copy(neighbours.cbegin(), neighbours.cend(), std::back_inserter(excepts));
    } else {
        const auto& inNodes = node.get_in_nodes();
        for (const auto& inNode: inNodes)
            targetNodes.push_back(const_cast<const qan::Node*>(inNode));
    }

    // 1. Collect target nodes ancestors or group neighbours ancestors
    for (const auto targetNode: targetNodes)
        collectAncestorsDfs_rec(targetNode, parents, marks,
                                collectAncestorsDfs_rec);

    // 2. Remove protected nodes from ancestors
    parents.erase(std::remove_if(parents.begin(), parents.end(),
                                 [&excepts](auto e) -> bool {
        return std::find(excepts.begin(), excepts.end(), e) != excepts.end();
    }), parents.end());
    return parents;
}

std::vector<const qan::Node*>   Graph::collectChilds(const qan::Node& node) const
{
    // ALGORITHM:
      // 0. Collect node neighbour.
      // 1. Collect childs of neighbours.
      // 2. Remove protected nodes from childs.

    const auto collectChildsDfs_rec = [](const qan::Node* visited,
            std::vector<const qan::Node*>& childs,
            std::unordered_set<const qan::Node*>& marks,
            const auto& lambda) {
        if (visited == nullptr)
            return;
        if (marks.find(visited) != marks.end())    // Do not collect on already visited
            return;                                // branchs
        marks.insert(visited);
        childs.push_back(visited);

        if (visited->getGroup() != nullptr)
            childs.push_back(visited->getGroup());

        // 1.1 Collect childs
        for (const auto outNode : visited->get_out_nodes())
            lambda(outNode, childs, marks,
                   lambda);
    };

    std::vector<const qan::Node*> childs;
    std::unordered_set<const qan::Node*> marks;

    // 0. Collect node neighbours
    auto excepts = collectGroups(node);
    excepts.push_back(&node);
    //qWarning() << "qan::Graph::collectChilds(): excepts.size=" << excepts.size();

    std::vector<const qan::Node*> targetNodes;
    if (node.isGroup()) {
        const auto neighbours = collectNeighbours(node);
        targetNodes = neighbours;
        std::copy(neighbours.cbegin(), neighbours.cend(), std::back_inserter(excepts));
    } else {
        const auto& outNodes = node.get_out_nodes();
        for (const auto& outNode: outNodes)
            targetNodes.push_back(const_cast<const qan::Node*>(outNode));
    }

    // 1. Collect node childs
    for (const auto target: targetNodes)
        collectChildsDfs_rec(target, childs, marks,
                             collectChildsDfs_rec);

    // 2. Remove protected nodes from child
    childs.erase(std::remove_if(childs.begin(), childs.end(),
                                [&excepts](auto e) -> bool {
        return std::find(excepts.begin(), excepts.end(), e) != excepts.end();
    }), childs.end());
    return childs;
}

bool    Graph::isAncestor(qan::Node* node, qan::Node* candidate) const
{
    if (node != nullptr && candidate != nullptr)
        return isAncestor(*node, *candidate);
    return false;
}

bool    Graph::isAncestor(const qan::Node& node, const qan::Node& candidate) const noexcept
{
    std::unordered_set<const qan::Node*> marks;
    marks.insert(&node);

    const auto isAncestorDfs_rec = [&node](const qan::Node* visited, const qan::Node& candidate,
                                      std::unordered_set<const qan::Node*>& marks,
                                      const auto& lambda) {
        if (visited == nullptr)
            return false;
        if (visited == &node)    // Circuit detection
            return false;
        if (visited == &candidate)
            return true;
        if (marks.find(visited) != marks.end())    // Do not collect on already visited
            return false;                       // branchs
        marks.insert(visited);
        for (const auto inNode : visited->get_in_nodes())
            if (lambda(inNode, candidate,
                       marks, lambda))
                return true;
        return false;
    };

    for (const auto inNode : node.get_in_nodes()) {
        if (isAncestorDfs_rec(inNode, candidate,
                              marks, isAncestorDfs_rec))
            return true;
    }
    return false;
}

auto    Graph::collectGroupsNodes(const QVector<const qan::Group*>& groups) const noexcept -> std::unordered_set<const qan::Node*>
{
    std::unordered_set<const qan::Node*> r;
    for (const auto group: qAsConst(groups))  // Collect all group nodes and their sub groups nodes
        if (group != nullptr)           // recursively
            collectGroupNodes_rec(group, r);
    return r;
}

auto    Graph::collectGroupNodes_rec(const qan::Group* group, std::unordered_set<const qan::Node*>& nodes) const -> void
{
    if (group == nullptr)
        return;

    for (const auto& groupNode : group->get_nodes()) {
        if (groupNode == nullptr)
            continue;
        nodes.insert(groupNode);
        if (groupNode->isGroup()) {
            const auto groupGroup = qobject_cast<const qan::Group*>(groupNode);
            if (groupGroup != nullptr)
                collectGroupNodes_rec(groupGroup, nodes);
        }
    }
}
//-----------------------------------------------------------------------------


} // ::qan
