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
// \file	qanBehaviour.h
// \author	benoit@destrat.io
// \date	2016 04 04
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QObject>
#include <QQuickItem>

// QuickQanava headers
#include "./gtpo/observer.h"
#include "./qanEdge.h"

namespace qan { // ::qan

class Node;
class Edge;

/*! \brief Define node behaviour interface to react to node topology events (QuickQanava adapter for gtpo::dynamic_node_nehaviour<>).
 *
 * Use qan::Node::installBehaviour() method to install a custom behaviour on a host node to be notified
 * and react to topological changes.
 *
 * \code
 * const auto node = graph.insertNode();
 * node->installBehaviour(std::make_unique<MyBehaviour>());
 * \endcode
 *
 *  \nosubgrouping
 */
class NodeBehaviour : public QObject,
                      public gtpo::node_observer<qan::Node, qan::Edge>
{
    Q_OBJECT

    /*! \name NodeBehaviour Management *///------------------------------------
    //@{
public:
    explicit NodeBehaviour(const std::string& name, QObject* parent = nullptr);
    virtual ~NodeBehaviour() override { }
    NodeBehaviour(const NodeBehaviour&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Behaviour Host Management *///-----------------------------------
    //@{
public:
    //! Shortcut to getHost() != nullptr.
    inline  auto    hasHost() const noexcept -> bool { return getHost() != nullptr; }

    /*! \brief Node behaviour target node.
     *
     * Behaviour is set for a host node via qan::Node::installBehaviour(), it can be changed
     * dynamically after it has been installed (read-only property).
     */
    Q_PROPERTY(qan::Node* host READ getHost NOTIFY hostChanged FINAL)
    inline qan::Node*       getHost() noexcept { return _host.data(); }
    inline const qan::Node* getHost() const noexcept { return _host.data(); }
    //! \brief Set behaviour host node to \c host (can't be nullptr), disconnect any signal connected from an old existing host and this.
    virtual void            setHost(qan::Node* host);
private:
    //! \copydoc host
    QPointer<qan::Node> _host{nullptr};
signals:
    //! \copydoc host
    void                hostChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Notification Interface *///--------------------------------------
    //@{
public:
    virtual void    on_in_node_inserted(qan::Node& target, qan::Node& weakInNode, const qan::Edge& edge) noexcept override;
    //! \copydoc gtpo::dynamic_node_nehaviour::inNodeRemoved()
    virtual void    on_in_node_removed(qan::Node& target, qan::Node& weakInNode, const qan::Edge& edge) noexcept override;
    //! \copydoc gtpo::dynamic_node_nehaviour::inNodeRemoved()
    virtual void    on_in_node_removed(qan::Node& target) noexcept override { Q_UNUSED(target); }

    //! \copydoc gtpo::dynamic_node_nehaviour::outNodeInserted()
    virtual void    on_out_node_inserted(qan::Node& target, qan::Node& weakOutNode, const qan::Edge& edge) noexcept override;
    //! \copydoc gtpo::dynamic_node_nehaviour::outNodeRemoved()
    virtual void    on_out_node_removed(qan::Node& target, qan::Node& weakOutNode, const qan::Edge& edge) noexcept override;
    //! \copydoc gtpo::dynamic_node_nehaviour::outNodeRemoved()
    virtual void    on_out_node_removed(qan::Node& target) noexcept override { Q_UNUSED(target); }

protected:
    virtual void    inNodeInserted(qan::Node& inNode, qan::Edge& edge) noexcept { Q_UNUSED(inNode); Q_UNUSED(edge); }
    virtual void    inNodeRemoved(qan::Node& inNode, qan::Edge& edge) noexcept { Q_UNUSED(inNode); Q_UNUSED(edge); }

    virtual void    outNodeInserted(qan::Node& outNode, qan::Edge& edge) noexcept { Q_UNUSED(outNode); Q_UNUSED(edge); }
    virtual void    outNodeRemoved(qan::Node& outNode, qan::Edge& edge) noexcept { Q_UNUSED(outNode); Q_UNUSED(edge); }
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::NodeBehaviour)

