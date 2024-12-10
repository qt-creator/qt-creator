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
// \file	qanGraphView.h
// \author	benoit@destrat.io
// \date	2016 08 15
//-----------------------------------------------------------------------------

#pragma once

// QuickQanava headers
#include "./qanGraph.h"
#include "./qanGroup.h"
#include "./qanNavigable.h"
#include "./qanPortItem.h"

// Qt headers
#include <QQuickItem>
#include <QPointer>

namespace qan { // ::qan

/*! \brief Abstract view component for the qan::Graph class.
 *
 * \nosubgrouping
*/
class GraphView : public qan::Navigable
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AbstractGraphView)

    /*! \name GraphView Object Management *///---------------------------------
    //@{
public:
    //! GraphView default constructor.
    explicit GraphView(QQuickItem* parent = nullptr);
    virtual ~GraphView() override = default;
    GraphView(const GraphView&) = delete;

public:
    //! Graph that should be displayed in this graph view.
    Q_PROPERTY(qan::Graph*  graph READ getGraph WRITE setGraph NOTIFY graphChanged FINAL)
    void                    setGraph(qan::Graph* graph);
    inline qan::Graph*      getGraph() const noexcept { return _graph.data(); }
private:
    QPointer<qan::Graph>    _graph = nullptr;
signals:
    void                    graphChanged();
    //@}
    //-------------------------------------------------------------------------


    /*! \name GraphView Interactions Management *///---------------------------
    //@{
protected:
    //! Called when the mouse is clicked in the container (base implementation empty).
    virtual void    navigableClicked(QPointF pos, QPointF globalPos) override;
    virtual void    navigableRightClicked(QPointF pos, QPointF globalPos) override;

    //! Utilisty method to convert a given \c url to a local file path (if possible, otherwise return an empty string).
    Q_INVOKABLE QString urlToLocalFile(QUrl url) const noexcept;

signals:
    void            connectorChanged();

    void            rightClicked(QPointF pos, QPointF globalPos);

    void            nodeClicked(qan::Node* node, QPointF pos);
    void            nodeRightClicked(qan::Node* node, QPointF pos);
    void            nodeDoubleClicked(qan::Node* node, QPointF pos);

    void            portClicked(qan::PortItem* port, QPointF pos);
    void            portRightClicked(qan::PortItem* port, QPointF pos);

    void            edgeClicked(qan::Edge* edge, QPointF pos);
    void            edgeRightClicked(qan::Edge* edge, QPointF pos);
    void            edgeDoubleClicked(qan::Edge* edge, QPointF pos);

    void            groupClicked(qan::Group* group, QPointF pos);
    void            groupRightClicked(qan::Group* group, QPointF pos);
    void            groupDoubleClicked(qan::Group* group, QPointF pos);
    //@}
    //-------------------------------------------------------------------------


    /*! \name Selection Rectangle Management *///------------------------------
    //@{
protected:
    //! \copydoc qan::Navigable::selectionRectActivated()
    virtual void    selectionRectActivated(const QRectF& rect) override;

    //! \copydoc qan::Navigable::selectionRectEnd()
    virtual void    selectionRectEnd() override;
private:
    QSet<QQuickItem*>   _selectedItems;

protected:
    virtual void    keyPressEvent(QKeyEvent *event) override;
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::GraphView)
