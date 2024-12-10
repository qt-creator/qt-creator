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
// \file	graph.h
// \author	benoit@destrat.io
// \date	2016 01 22
//-----------------------------------------------------------------------------

#pragma once

// STD headers
#include <unordered_set>
#include <cassert>
#include <iterator>         // std::back_inserter

// GTpo headers
#include "./container_adapter.h"
#include "./observable.h"
#include "./observer.h"

/*! \brief GTPO for Generic Graph ToPolOgy.
 */
namespace gtpo { // ::gtpo

/*! \brief Weighted directed graph using a node-list, edge-list representation.
 *
 * \note Root nodes could be considered as "mother vertices" as described in graph theory of directed graphs ... but
 * the actual implementation in GTpo has no protection for more than 0 degree cycles... When graph has cycles with
 * degree > 1, we could actually have strongly connected components ... with no root nodes !
 *
 * \note See http://en.cppreference.com/w/cpp/language/dependent_name for
 *       typename X::template T c++11 syntax and using Nodes = typename config_t::template node_container_t< Node* >;
 *
 */
template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
class graph : public graph_base_t,
              public gtpo::observable_graph<graph_base_t, node_t, edge_t, group_t>
{
    /*! \name Graph Management *///--------------------------------------------
    //@{
public:
    using graph_t           = graph<graph_base_t, node_t, group_t, edge_t>;

    using nodes_t           = qcm::Container<QVector, node_t*>;
    using nodes_search_t    = QSet<node_t*>;

    using groups_t          = qcm::Container<QVector, group_t*>;

    using edges_t           = qcm::Container<QVector, edge_t*>;
    using edges_search_t    = QSet<edge_t*>;

    //! User friendly shortcut type to graph gtpo::observable<> base class.
    using observable_base_t =  gtpo::observable_graph<graph_base_t, node_t, edge_t, group_t>;

public:
    using size_type  = std::size_t;

    graph() noexcept :
        graph_base_t{},
        observable_base_t{} { }

    template <class B>
    explicit graph(B* parent) noexcept :
        graph_base_t{parent},
        observable_base_t{} { }

    virtual ~graph();

    graph(const graph&) = delete;
    graph& operator=(const graph&) = delete;

    /*! \brief Clear the graph from all its content (nodes, edges, groups, behaviours).
     *
     * \note Graph behaviours are cleared after the topology, if you do not want to take into account topology
     * changes when clearing the graph, disable all behaviours before calling clear().
     */
    void    clear() noexcept;

    /*! \brief Test if this graph is empty, and empty graph has no nodes nor groups.
     *
     * \return true if the graph is empty, false otherwise.
     */
    auto    is_empty() const noexcept -> bool { return get_node_count() == 0 && get_group_count() == 0; }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Node Management *///---------------------------------------
    //@{
public:
    /*! \brief Create node and insert it in the graph an return a reference to it.
     * \return a pointer to the created node (graph has ownership for the node).
     */
    auto    create_node() -> node_t*;

    /*! \brief Insert an already existing \c node in graph (graph take \c node ownership).
     *
     * If your nodes must be created outside of GTpo (ie not with the create_node() method),
     * the only way of giving node ownership to GTpo is trought the insert_node method.
     */
    auto    insert_node(node_t* node) -> bool;

    /*! \brief Remove node \c node from graph.
     *
     * Complexity depends on config_t::node_container_t.
     * \note If \c node is actually grouped in a group, it will first be ungrouped before
     * beeing removed (any group behaviour will also be notified that the node is ungrouped).
     */
    auto    remove_node(node_t* node ) -> bool;

    //! Return the number of nodes actually registered in graph.
    inline auto get_node_count() const -> size_type { return _nodes.size(); }
    //! Return the number of root nodes (actually registered in graph)ie nodes with a zero in degree).
    inline auto get_root_node_count() const -> size_type { return _root_nodes.size(); }

    /*! \brief Install a given \c node in the root node cache.
     *
     * This method should not be directly used by an end user until you have deeply
     * modified graph topology with non gtpo::graph<> methods.
     *
     */
    auto    install_root_node(node_t* node) -> void;
    /*! \brief Test if a given \c node is a root node.
     *
     * This method is safer than testing node->get_in_degree()==0, since it check
     * \c node in degree and its presence in the internal root node cache.
     *
     * \return true if \c node is a root node, false otherwise.
     */
    auto    is_root_node(const node_t* node) const -> bool;

    //! Expose the base class method of the same name:
    using   graph_base_t::contains;

    //! Use fast search container to find if a given \c node is part of this graph.
    auto    contains(const node_t* node) const -> bool;

    //! Graph main nodes container.
    inline auto     get_nodes() const -> const nodes_t& { return _nodes; }
    //! Return a const begin iterator over graph shared_node_t nodes.
    inline auto     begin() const -> typename nodes_t::const_iterator { return _nodes.begin( ); }
    //! Return a const begin iterator over graph shared_node_t nodes.
    inline auto     end() const -> typename nodes_t::const_iterator { return _nodes.end( ); }

    //! Return a const begin iterator over graph shared_node_t nodes.
    inline auto     cbegin() const -> typename nodes_t::const_iterator { return _nodes.cbegin(); }
    //! Return a const end iterator over graph shared_node_t nodes.
    inline auto     cend() const -> typename nodes_t::const_iterator { return _nodes.cend(); }

    //! Graph root nodes container.
    inline auto     get_root_nodes() const -> const nodes_t& { return _root_nodes; }

private:
    nodes_t         _nodes;
    nodes_t         _root_nodes;
    nodes_search_t  _nodes_search;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Edge Management *///---------------------------------------
    //@{
public:
    /*! \brief Create and insert a directed edge between \c source and \c destination node.
     *
     * Complexity is O(1).
     * \return the inserted edge (if an error occurs return nullptr).
     */
    auto        insert_edge(node_t* source, node_t* destination) -> edge_t*;

    /*! \brief Insert a directed edge created outside of GTpo into the graph.
     *
     * \param edge must have a valid source and destination set otherwise a bad topology exception will be thrown.
     * \sa insert_node()
     */
    auto        insert_edge(edge_t* edge) -> bool;

    /*! \brief Remove first directed edge found between \c source and \c destination node.
     *
     * If the current graph<> config_t::edge_container_t and config_t::node_container_t allow parrallel edges support, the first
     * edge found between \c source and \c destination will be removed.
     *
     * Complexity is O(edge count) at worst.
     */
    auto        remove_edge(node_t* source, node_t* destination) -> bool;

    /*! \brief Remove all directed edge between \c source and \c destination node.
     *
     * If the current graph<> config_t::edge_container_t and config_t::node_container_t allow parrallel edges support, the first
     * edge found between \c source and \c destination will be removed.
     *
     * Worst case complexity is O(edge count).
     */
    auto        remove_all_edges(node_t* source, node_t* destination) -> bool;

    /*! \brief Remove directed edge \c edge.
     *
     * Worst case complexity is O(edge count).
     */
    auto        remove_edge(edge_t* edge) -> bool;

    /*! \brief Look for the first directed edge between \c source and \c destination and return it.
     *
     * Worst case complexity is O(edge count).
     * \return A shared reference on edge, en empty shared reference otherwise (result == false).
     * \throw noexcept.
     */
    auto        find_edge(const node_t* source, const node_t* destination) const -> edge_t*;
    /*! \brief Test if a directed edge exists between nodes \c source and \c destination.
     *
     * This method only test a 1 degree relationship (ie a direct edge between \c source
     * and \c destination). Worst case complexity is O(edge count).
     * \throw noexcept.
     */
    auto        has_edge(const node_t* source, const node_t* destination) const -> bool;
    /*! \brief Look for the first directed restricted hyper edge between \c source node and \c destination edge and return it.
     *
     * Worst case complexity is O(edge count).
     * \return A shared reference on edge, en empty shared reference otherwise (result == false).
     * \throw noexcept.
     */
    auto        find_edge(const node_t* source, const edge_t* destination) const -> edge_t*;

    //! Return the number of edges currently existing in graph.
    auto        get_edge_count() const noexcept -> unsigned int { return static_cast<int>( _edges.size() ); }
    /*! \brief Return the number of (parallel) directed edges between nodes \c source and \c destination.
     *
     * Graph edge_container_t should support multiple insertions (std::vector, std::list) to enable
     * parrallel edge support, otherwise, get_edge_count() will always return 1 or 0.
     *
     * This method only test a 1 degree relationship (ie a direct edge between \c source
     * and \c destination). Worst case complexity is O(edge count).
     */
    auto        get_edge_count(node_t* source, node_t* destination) const -> unsigned int;

    //! Use fast search container to find if a given \c edge is part of this graph.
    auto        contains(edge_t* edge) const -> bool;

    //! Graph main edges container.
    inline auto get_edges() const noexcept -> const edges_t& { return _edges; }
private:
    edges_t         _edges;
    edges_search_t  _edges_search;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Graph Group Management *///--------------------------------------
    //@{
public:
    /*! \brief Insert a node group into the graph.
     */
    auto            insert_group(group_t* group) -> bool;

    /*! \brief Remove node group \c group.
     *
     * Group content (ie group's nodes) are not removed from the graph, but ungrouped (ie moved from
     * the group to the graph).
     *
     * Worst case complexity is O(group count).
     */
    auto            remove_group(group_t* group) -> bool;

    //! Return true if a given group \c group is registered in the graph.
    auto            has_group(const group_t* group) const -> bool;

    //! Return the number of edges currently existing in graph.
    inline auto     get_group_count() const -> int { return static_cast<int>( _groups.size() ); }

    //! Graph main edges container.
    inline auto     get_groups() const -> const groups_t& { return _groups; }

    /*! \brief Insert an existing node \c node in group \c group.
     *
     * \note \c node get_group() will return \c group if grouping succeed.
     */
    auto            group_node(node_t* node, group_t* group) -> bool;

    /*! \brief Insert an existing node \c weakNode in group \c weakGroup group.
     *
     * \note If a behaviour has been installed with gtpo::group::add_dynamic_group_behaviour(), behaviour's
     * node_removed() will be called.
     *
     * \note \c node getGroup() will return an expired weak pointer if ungroup succeed.
     */
    auto            ungroup_node(node_t* node, node_t* group) -> bool;

private:
    groups_t        _groups;
    //@}
    //-------------------------------------------------------------------------
};

} // ::gtpo

#include "./graph.hpp"
