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
// \file	qanEdge.h
// \author	benoit@destrat.io
// \date	2004 February 15
//-----------------------------------------------------------------------------

#pragma once

// QuickQanava headers
#include "./gtpo/edge.h"
#include "./qanStyle.h"
#include "./qanNode.h"

Q_MOC_INCLUDE("./qanGraph.h")
Q_MOC_INCLUDE("./qanEdgeItem.h")
Q_MOC_INCLUDE("./qanNode.h")

namespace qan { // ::qan

class Graph;
class EdgeItem;
class Node;

/*! \brief Weighted directed edge linking two nodes in a graph.
 *
 * \nosubgrouping
 */
class Edge : public gtpo::edge<QObject, qan::Graph, Node>
{
    /*! \name Edge Object Management *///--------------------------------------
    //@{
    Q_OBJECT
public:
    using super_t = gtpo::edge<QObject, qan::Graph, Node>;

    //! Edge constructor with source, destination and weight initialization.
    explicit Edge(QObject* parent = nullptr);
    Edge(const Edge&) = delete;
    virtual ~Edge() override;

public:
    Q_PROPERTY(qan::Graph* graph READ getGraph CONSTANT FINAL)
    //! Shortcut to gtpo::edge<>::getGraph().
    qan::Graph*         getGraph() noexcept;
    //! \copydoc getGraph()
    const qan::Graph*   getGraph() const noexcept;

public:
    friend class qan::EdgeItem;

    Q_PROPERTY(qan::EdgeItem* item READ getItem CONSTANT)
    qan::EdgeItem*   getItem() noexcept;
    virtual void     setItem(qan::EdgeItem* edgeItem) noexcept;
private:
    QPointer<qan::EdgeItem> _item;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Edge Static Factories *///---------------------------------------
    //@{
public:
    /*! \brief Return the default delegate QML component that should be used to generate edge \c item.
     *
     *  \arg engine QML engine used to create delegate component.
     *  \return Default delegate component or nullptr (when nullptr is returned, QuickQanava default to Qan.Edge component).
     */
    static  QQmlComponent*      delegate(QQmlEngine& engine, QObject* parent = nullptr) noexcept;

    /*! \brief Return the default style that should be used with qan::Edge.
     *
     *  \return Default style or nullptr (when nullptr is returned, qan::StyleManager default edge style will be used).
     */
    static  qan::EdgeStyle*     style(QObject* parent = nullptr) noexcept;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Edge Topology Management *///------------------------------------
    //@{
public:
    Q_INVOKABLE qan::Node* getSource();
    Q_INVOKABLE qan::Node* getDestination();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Edge Properties Management *///----------------------------------
    //@{
public:
    //! \copydoc _label
    Q_PROPERTY(QString label READ getLabel WRITE setLabel NOTIFY labelChanged FINAL)
    //! \copydoc _label
    bool            setLabel(const QString& label);
    //! \copydoc _label
    const QString&  getLabel() const { return _label; }
protected:
    //! Edge label.
    QString         _label = QStringLiteral("");
signals:
    //! \copydoc _label
    void            labelChanged();

public:
    /*! \brief A protected edge can't be dragged by user (default to unprotected).
     *
     * Contrary to `enabled` and 'locked' properties: double click, right click and selection is
     * active when protected.
     *
     * \note edgeDoubleClicked(), edgeRightClicked() signal are still emitted from protected edge.
     */
    Q_PROPERTY(bool isProtected READ getIsProtected WRITE setIsProtected NOTIFY isProtectedChanged FINAL)
    bool            setIsProtected(bool isProtected);
    bool            getIsProtected() const { return _isProtected; }
private:
    bool            _isProtected = false;
signals:
    void            isProtectedChanged();

public:
    /*! \brief A locked edge can't be selected / dragged by user (default to false ie unlocked).
     *
     * Might be usefull to prevent user inputs when the edge is laid out automatically.
     */
    Q_PROPERTY(bool locked READ getLocked WRITE setLocked NOTIFY lockedChanged FINAL)
    bool            setLocked(bool locked);
    bool            getLocked() const { return _locked; }
private:
    bool            _locked = false;
signals:
    void            lockedChanged();

public:
    //! \copydoc _weight
    Q_PROPERTY(qreal weight READ getWeight WRITE setWeight NOTIFY weightChanged FINAL)
    //! \copydoc _weight
    qreal           getWeight() const { return _weight; }
    //! \copydoc _weight
    bool            setWeight(qreal weight);
protected:
    //! Edge weight (default to 1.0, range [-1., 1.0] is safe, otherwise setWeight() might not work).
    qreal           _weight = 1.0;
signals:
    //! \copydoc _weight
    void            weightChanged();
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::Edge)
