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
// \file	qanNode.h
// \author	benoit@destrat.io
// \date	2004 February 15
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickItem>
#include <QPointF>
#include <QPolygonF>

// QuickQanava headers
#include "./gtpo/node.h"
#include "./qanEdge.h"
#include "./qanStyle.h"
#include "./qanBehaviour.h"
#include "./qanTableCell.h"

namespace qan { // ::qan

class NodeBehaviour;
class Graph;
class Group;
class Edge;
class NodeItem;
class PortItem;

/*! \brief Base class for modelling nodes with attributes and an in/out edges list in a qan::Graph graph.
 *
 * \note If your application does not need drag'n'drop support (ie group insertion via dra'n'drop or VisualConnector are not used nor necessary), consider disabling
 * drag'n'drop support by setting the \c acceptsDrops and \c droppable properties to false, it could improve performances significantly.
 *
 * \nosubgrouping
*/
class Node : public gtpo::node<QObject,
                               qan::Graph,
                               qan::Node,
                               qan::Edge,
                               qan::Group>
{
    /*! \name Node Object Management *///--------------------------------------
    //@{
    Q_OBJECT
public:
    using super_t = gtpo::node<QObject, qan::Graph, qan::Node, qan::Edge, qan::Group>;

    //! Node constructor.
    explicit Node(QObject* parent=nullptr);
    virtual ~Node();
    Node(const Node&) = delete;

public:
    Q_PROPERTY(qan::Graph* graph READ getGraph CONSTANT FINAL)
    //! Shortcut to gtpo::node<>::getGraph().
    qan::Graph*         getGraph() noexcept;
    //! \copydoc getGraph()
    const qan::Graph*   getGraph() const noexcept;

public:
    /*!
     * \note only label is taken into account for equality comparison.
     */
    bool    operator==(const qan::Node& right) const;

public:
    Q_PROPERTY(qan::NodeItem* item READ getItem CONSTANT)
    qan::NodeItem*          getItem() noexcept;
    const qan::NodeItem*    getItem() const noexcept;
    virtual void            setItem(qan::NodeItem* nodeItem) noexcept;
protected:
    QPointer<qan::NodeItem> _item;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node Static Factories *///---------------------------------------
    //@{
public:
    /*! \brief Return the default delegate QML component that should be used to generate node \c item.
     *
     *  \arg engine QML engine used to create delegate component.
     *  \return Default delegate component or nullptr (when nullptr is returned, QuickQanava default to Qan.Node component).
     */
    static  QQmlComponent*      delegate(QQmlEngine& engine, QObject* parent = nullptr) noexcept;

    /*! \brief Return the default style that should be used with qan::Node.
     *
     *  \return Default style or nullptr (when nullptr is returned, qan::StyleManager default node style will be used).
     */
    static  qan::NodeStyle*     style(QObject* parent = nullptr) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Topology Interface *///------------------------------------------
    //@{
public:
    //! Read-only abstract item model of this node in nodes.
    Q_PROPERTY(QAbstractItemModel* inNodes READ qmlGetInNodes CONSTANT FINAL)
    QAbstractItemModel* qmlGetInNodes() const;

public:
    Q_PROPERTY(int  inDegree READ getInDegree NOTIFY inDegreeChanged FINAL)
    int     getInDegree() const;
signals:
    void    inDegreeChanged();

public:
    //! Read-only abstract item model of this node out nodes.
    Q_PROPERTY(QAbstractItemModel* outNodes READ qmlGetOutNodes CONSTANT FINAL)
    QAbstractItemModel* qmlGetOutNodes() const;

public:
    Q_PROPERTY(int  outDegree READ getOutDegree NOTIFY outDegreeChanged FINAL)
    int     getOutDegree() const;
signals:
    void    outDegreeChanged();

public:
    //! Read-only abstract item model of this node out nodes.
    Q_PROPERTY(QAbstractItemModel* outEdges READ qmlGetOutEdges CONSTANT FINAL)
    QAbstractItemModel* qmlGetOutEdges() const;

public:
    //! Get this node level 0 adjacent edges (ie sum of node in edges and out edges).
    virtual std::unordered_set<qan::Edge*>  collectAdjacentEdges() const;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Behaviours Management *///---------------------------------------
    //@{
public:
    virtual void    installBehaviour(std::unique_ptr<qan::NodeBehaviour> behaviour);
    //@}
    //-------------------------------------------------------------------------

    /*! \name Appearance Management *///---------------------------------------
    //@{
public:
    Q_PROPERTY(QString label READ getLabel WRITE setLabel NOTIFY labelChanged FINAL)
    bool            setLabel(const QString& label);
    QString         getLabel() const { return _label; }
private:
    QString         _label = "";
signals:
    void            labelChanged();

public:
    /*! \brief A protected node can't dragged by user (default to unprotected).
     *
     * Might be usefull to prevent user inputs when the node is laid out automatically.
     *
     * Contrary to `enabled` and 'locked' properties: double click, right click and selection is
     * active when protected.
     *
     * \note nodeDoubleClicked(), nodeRightClicked() signal are still emitted from protected node.
     */
    Q_PROPERTY(bool isProtected READ getIsProtected WRITE setIsProtected NOTIFY isProtectedChanged FINAL)
    bool            setIsProtected(bool isProtected);
    bool            getIsProtected() const { return _isProtected; }
private:
    bool            _isProtected = false;
signals:
    void            isProtectedChanged();

public:
    /*! \brief A locked node can't be selected / dragged by user (node are unlocked by default).
     *
     * Might be usefull to prevent user inputs when the node is laid out automatically.
     *
     * Contrary to `enabled` property, using `locked` still allow to receive right click events, for
     * example to use a context menu.
     *
     * \note nodeRightClicked() signal is still emitted from locked node when node is double clicked.
     */
    Q_PROPERTY(bool locked READ getLocked WRITE setLocked NOTIFY lockedChanged FINAL)
    virtual bool    setLocked(bool locked);
    bool            getLocked() const { return _locked; }
private:
    bool            _locked = false;
signals:
    void            lockedChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node Group Management *///---------------------------------------
    //@{
public:
    /*! \brief Node (or group) parent group.
     *
     * \note nullptr if group or node is ungrouped.
     */
    Q_PROPERTY(qan::Group* group READ getGroup FINAL)
    const qan::Group*    getGroup() const { return get_group(); }
    qan::Group*          getGroup() { return get_group(); }
    Q_INVOKABLE bool     hasGroup() const { return get_group() != nullptr; }

    //! Shortcut to base is_group() (ie return true if this node is a group and castable to qan::Group)..
    Q_INVOKABLE bool     isGroup() const { return is_group(); }

public:
    Q_PROPERTY(qan::TableCell* cell READ getCell NOTIFY cellChanged FINAL)
    const qan::TableCell*   getCell() const { return _cell.data(); }
    qan::TableCell*         getCell() { return _cell; }
    bool                    setCell(qan::TableCell* cell);
protected:
    QPointer<qan::TableCell>    _cell;
signals:
    void                    cellChanged();
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::Node)
QML_DECLARE_TYPE(const qan::Node)
