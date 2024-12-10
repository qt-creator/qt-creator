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
// This file is a part of the GTpo software library.
//
// \file	node.h
// \author	benoit@destrat.io
// \date	2016 01 22
//-----------------------------------------------------------------------------

#pragma once

// STD headers
#include <unordered_set>
#include <cassert>
#include <iterator>         // std::back_inserter

// GTpo headers
#include "./graph_property.h"
#include "./observable.h"
#include "./observer.h"
#include "./container_adapter.h"

// QuickContainers headers
#include "../quickcontainers/qcmContainer.h"

namespace gtpo { // ::gtpo

/*! \brief Base class for modelling nodes with an in/out edges list in a gtpo::graph graph.
 *
 * \nosubgrouping
 */
template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
class node : public node_base_t,
             public graph_property_impl<graph_t>,
             public gtpo::observable_node<node_t, edge_t>
{
    /*! \name Node Management *///---------------------------------------------
    //@{
public:
    friend graph_t;   // graph need access to graph_property_impl<>::set_graph()
    using nodes_t   = qcm::Container<QVector, node_t*>;

    //! User friendly shortcut type to node gtpo::observable<> base class.
    using observable_base_t =  gtpo::observable_node<node_t, edge_t>;

    node(node_base_t* parent = nullptr) noexcept : node_base_t{parent} { }
    virtual ~node() noexcept {
        _in_edges.clear();
        _out_edges.clear();
        _in_nodes.clear(/*clearContent=*/false, /*notify=*/false);
        _out_nodes.clear(/*clearContent=*/false, /*notify=*/false);
        if (this->_graph != nullptr) {
            std::cerr << "gtpo::node<>::~node(): Warning: Node has been destroyed before beeing removed from the graph." << std::endl;
        }
        this->_graph = nullptr;
    }
    /*! \brief Default copy constructor does _nothing_.
     *
     * \note Node topology is not copied since a node identity from a topology point of
     * view is it's pointer adress. Copy constructor is defined to allow copying node
     * properties, topology (in/out edges and group membership is not copied), see documentation
     * for a more detailled description of restrictions on move and copy.
     */
    node(const node& node ) {
        static_cast<void>(node);
    }
    node& operator=(node const&) = delete;

    auto    add_node_observer(std::unique_ptr<gtpo::node_observer<node_t, edge_t>> observer) -> void {
        if (observer)
            observer->set_target(reinterpret_cast<node_t*>(this));
        observable_base_t::add_observer(std::move(observer));
    }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node Edges Management *///---------------------------------------
    //@{
public:
    using edges_t    = qcm::Container<QVector, edge_t*>;

    /*! \brief Insert edge \c outEdge as an out edge for this node.
     *
     * \note if \c outEdge source node is different from this node, it is set to this node.
     */
    auto    add_out_edge(edge_t* outEdge) -> bool;
    /*! \brief Insert edge \c inEdge as an in edge for \c node.
     *
     * \note if \c inEdge destination node is different from \c node, it is automatically set to \c node.
     */
    auto    add_in_edge(edge_t* inEdge ) -> bool;
    /*! \brief Remove edge \c outEdge from this node out edges.     *
     */
    auto    remove_out_edge(const edge_t* outEdge) -> bool;
    /*! \brief Remove edge \c inEdge from this node in edges.
     */
    auto    remove_in_edge(const edge_t* inEdge) -> bool;

    inline auto get_in_edges() const noexcept -> const edges_t& { return _in_edges; }
    inline auto get_out_edges() const noexcept -> const edges_t& { return _out_edges; }

    inline auto get_in_nodes() const noexcept -> const nodes_t& { return _in_nodes; }
    inline auto get_out_nodes() const noexcept -> const nodes_t& { return _out_nodes; }

    inline auto get_in_degree() const noexcept -> unsigned int { return static_cast<int>( _in_edges.size() ); }
    inline auto get_out_degree() const noexcept -> unsigned int { return static_cast<int>( _out_edges.size() ); }
private:
    edges_t     _in_edges;
    edges_t     _out_edges;
    nodes_t     _in_nodes;
    nodes_t     _out_nodes;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Node Edges Management *///---------------------------------------
    //@{
public:
    inline auto set_group(const group_t* group) noexcept -> void { _group = const_cast<group_t*>(group); }
    inline auto get_group() noexcept -> group_t* { return _group; }
    inline auto get_group() const noexcept -> const group_t* { return _group; }
private:
    group_t*    _group = nullptr;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Group-node Support *///------------------------------------------
    //@{
public:
    inline auto is_group() const noexcept -> bool { return _is_group; }

    //! Return group's nodes.
    inline auto get_nodes() const noexcept -> const nodes_t& { return _nodes; }

    //! Return group's nodes.
    inline auto get_nodes() -> nodes_t& { return _nodes; }

    //! Return true if group contains \c node.
    auto        has_node(const node_t* node) const noexcept -> bool;

    //! Return group registered node count.
    inline auto get_node_count() const noexcept -> int { return static_cast< int >( _nodes.size() ); }

protected:
    inline  auto set_is_group(bool is_group) noexcept -> void { _is_group = is_group; }

    inline  auto group_nodes() const noexcept -> const nodes_t& { return _nodes; }
private:
    bool        _is_group = false;
    nodes_t     _nodes;
    //@}
    //-------------------------------------------------------------------------
};

} // ::gtpo

#include "./node.hpp"

