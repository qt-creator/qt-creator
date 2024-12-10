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
// \file	graph.hpp
// \author	benoit@destrat.io
// \date	2016 01 22
//-----------------------------------------------------------------------------

namespace gtpo { // ::gtpo

/* Graph Node Management *///--------------------------------------------------
template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
graph<graph_base_t, node_t,
      group_t, edge_t>::~graph()
{
    for (const auto node: _nodes) {
        node->_graph = nullptr;
        delete node;
    }
    for (const auto edge: _edges) {
        edge->_graph = nullptr;
        delete edge;
    }
    // Note: _nodes and _edges containers take care of deleting all ressources
    // calling clear() here lead to very subtle notification bugs when container
    // models are binded to view.
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
void    graph<graph_base_t, node_t,
              group_t, edge_t>::clear() noexcept
{
    // Note 20220424: First clear nodes/edges container, then delete
    // their content since destroyed() signal from contained items might
    // be catched trigerring uses of _nodes/_edges with already deleted content
    nodes_t nodes;
    std::copy(_nodes.begin(), _nodes.end(), std::back_inserter(nodes));
    _root_nodes.clear();
    _nodes_search.clear();
    _nodes.clear();
    for (const auto node: nodes) {
        node->_graph = nullptr;
        delete node;
    }
    edges_t edges;
    std::copy(_edges.begin(), _edges.end(), std::back_inserter(edges));
    _edges_search.clear();
    _edges.clear();
    for (const auto edge: edges) {
        edge->_graph = nullptr;
        delete edge;
    }

    // Clearing groups and behaviours (Not: group->_graph is resetted with nodes)
    _groups.clear();

    observable_base_t::clear();
}
//-----------------------------------------------------------------------------

/* Graph Node Management *///--------------------------------------------------
template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto graph<graph_base_t, node_t,
           group_t, edge_t>::create_node( ) -> node_t*
{
    std::unique_ptr<node_t> node;
    try {
        node = std::make_unique<node_t>();
    } catch (...) {
        std::cerr << "graph<>::create_node(): Error: can't create node." << std::endl;
    }
    return node ? node.release() : nullptr;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::insert_node(node_t* node) -> bool
{
    if (node == nullptr)
        return false;
    // Do not insert an already inserted node
    if (container_adapter<nodes_search_t>::contains(_nodes_search, node)) {
        std::cerr << "gtpo::graph<>::insert_node(): Error: node has already been inserted in graph." << std::endl;
        return false;
    }
    try {
        node->set_graph(this);
        container_adapter<nodes_t>::insert(node, _nodes);
        container_adapter<nodes_search_t>::insert(node, _nodes_search);
        container_adapter<nodes_t>::insert(node, _root_nodes);

        observable_base_t::notify_node_inserted(*node);
    } catch (...) {
        std::cerr << "gtpo::graph<>::insert_node(): Error: can't insert node in graph." << std::endl;
        return false;
    }
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::remove_node(node_t* node) -> bool
{
    if (node == nullptr)
        return false;

    // Eventually, ungroup node
    auto group = node->get_group();
    if (group != nullptr)
        ungroup_node(node, group);

    observable_base_t::notify_node_removed(*node);

    // Removing all orphant edges pointing to node.
    edges_t nodeInEdges;
    std::copy(node->get_in_edges().cbegin(), node->get_in_edges().cend(),
              std::back_inserter(nodeInEdges));
    for (auto inEdge: nodeInEdges)
        node->remove_in_edge(inEdge);
    for (auto inEdge: nodeInEdges)
        remove_edge(inEdge);

    edges_t nodeOutEdges;
    std::copy(node->get_out_edges().cbegin(), node->get_out_edges().cend(),
              std::back_inserter(nodeOutEdges));
    for (auto outEdge: nodeOutEdges)
        node->remove_out_edge(outEdge);
    for (auto outEdge: nodeOutEdges)
        remove_edge(outEdge);

    // Remove node from main graph containers (it will generate node destruction)
    container_adapter<nodes_search_t>::remove(node, _nodes_search);
    container_adapter<nodes_t>::remove(node, _root_nodes);
    node->set_graph(nullptr);
    container_adapter<nodes_t>::remove(node, _nodes);
    delete node;
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::install_root_node(node_t* node) -> void
{
    if (node == nullptr)
        return;
    if (node->get_in_degree() != 0) {
        std::cerr << "gtpo::graph<>::install_root_node(): Error: trying to set a node with non "
                     "0 in degree as a root node." << std::endl;
        return;
    }
    container_adapter<nodes_t>::insert(node, _root_nodes);
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::is_root_node(const node_t* node) const -> bool
{
    if (node == nullptr)
        return false;
    if (node->get_in_degree() != 0)   // Fast exit when node in degree != 0, it can't be a root node
        return false;
    return _root_nodes.contains(const_cast<node_t*>(node));
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::contains(const node_t* node) const -> bool
{
    if (node == nullptr)
        return false;
    return _nodes_search.contains(const_cast<node_t*>(node));
}
//-----------------------------------------------------------------------------

/* Graph Edge Management *///--------------------------------------------------
template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::insert_edge(node_t* source, node_t* destination) -> edge_t*
{
    if (source == nullptr ||
        destination == nullptr) {
        std::cerr << "gtpo::graph<>::create_edge(node, node): Insertion of edge failed, "
                     "either source or destination nodes are nullptr." << std::endl;
        return nullptr;
    }

    std::unique_ptr<edge_t> edge;
    try {
        edge = std::make_unique<edge_t>();
        edge->set_graph(this);

        container_adapter<edges_t>::insert(edge.get(), _edges);
        container_adapter<edges_search_t>::insert(edge.get(), _edges_search);
        edge->set_src(source);
        edge->set_dst(destination);

        source->add_out_edge(edge.get());
        destination->add_in_edge(edge.get());

        if (source != destination ) // If edge define is a trivial circuit, do not remove destination from root nodes
            container_adapter<nodes_t>::remove(destination, _root_nodes);    // Otherwise destination is no longer a root node

        observable_base_t::notify_edge_inserted(*edge);
    } catch ( ... ) {
        std::cerr << "gtpo::graph<>::create_edge(node,node): Insertion of edge "
                     "failed, source or destination nodes topology can't be modified." << std::endl;
    }
    return edge ? edge.release() : nullptr;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::insert_edge(edge_t* edge) -> bool
{
    if (edge == nullptr)
        return false;
    auto source = edge->get_src();
    if (source == nullptr ||
        edge->get_dst() == nullptr) {
        std::cerr << "gtpo::graph<>::insert_edge(): Error: Either source and/or "
                     "destination nodes are nullptr." << std::endl;
        return false;
    }
    edge->set_graph(this);
    container_adapter<edges_t>::insert(edge, _edges);
    container_adapter<edges_search_t>::insert(edge, _edges_search);
    try {
        source->add_out_edge(edge);
        auto destination = edge->get_dst();
        if (destination != nullptr) {
            destination->add_in_edge(edge);
            if (source != destination) // If edge define is a trivial circuit, do not remove destination from root nodes
                container_adapter<nodes_t>::remove(destination, _root_nodes);    // Otherwise destination is no longer a root node
        }
        observable_base_t::notify_edge_inserted(*edge);
    } catch ( ... ) {
        std::cerr << "gtpo::graph<>::create_edge(): Insertion of edge failed, source or "
                     "destination nodes topology can't be modified." << std::endl;
        return false;
    }
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::remove_edge(node_t* source, node_t* destination) -> bool
{
    if (source == nullptr ||
        destination == nullptr)
        return false;

    // Find the edge associed with source / destination
    if (_edges.size() == 0)
        return false; // Fast exit
    auto edgeIter = std::find_if(_edges.begin(), _edges.end(),
                                 [=](const edge_t* e ) {
                                    return (e->get_src() == source &&
                                            e->get_dst() == destination);
                                    }
                                 ); // std::find_if
    return edgeIter != _edges.end() ? remove_edge(*edgeIter) : false;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::remove_all_edges(node_t* source, node_t* destination) -> bool
{
    if (source == nullptr ||
        destination == nullptr)
        return false;

    auto limit = _edges.size();
    while (get_edge_count(source, destination) > 0 &&
           limit >= 0) {
        remove_edge(source, destination);
        limit--;
    }
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::remove_edge(edge_t* edge) -> bool
{
    if (edge == nullptr)
        return false;
    auto source = edge->get_src();
    auto destination = edge->get_dst();
    if (source == nullptr      ||           // Expecting a non null source and either a destination or an hyper destination
        destination == nullptr) {
        std::cerr << "gtpo::graph<>::remove_edge(): Error: Edge source or destination is/are nullptr." << std::endl;
        return false;
    }

    observable_base_t::notify_edge_removed(*edge);
    source->remove_out_edge(edge);
    destination->remove_in_edge(edge);

    edge->set_graph(nullptr);
    container_adapter<edges_t>::remove(edge, _edges);
    container_adapter<edges_search_t>::remove(edge, _edges_search);
    delete edge;
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::find_edge(const node_t* source, const node_t* destination) const -> edge_t*
{
    // Find the edge associed with source / destination
    auto edgeIter = std::find_if(_edges.begin(), _edges.end(),
                                 [=](const edge_t* e ) noexcept {
                                    return ((e->get_src() == source) &&
                                            (e->get_dst() == destination));
                                } );
    return (edgeIter != _edges.end() ? *edgeIter : nullptr);
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::has_edge(const node_t* source, const node_t* destination) const -> bool
{
    return (find_edge(source, destination) != nullptr);
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::get_edge_count(node_t* source, node_t* destination ) const -> unsigned int
{
    unsigned int edgeCount = 0;
    std::for_each(_edges.begin(), _edges.end(), [&](const edge_t* e ) {
                                                       if ((e->get_src() == source) &&
                                                           (e->get_dst() == destination))
                                                           ++edgeCount;
                                                   } );
    return edgeCount;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::contains(edge_t* edge ) const -> bool
{
    if (edge == nullptr)   // Fast exit.
        return false;
    auto edgeIter = std::find_if(_edges_search.cbegin(), _edges_search.cend(),
                                 [=](const edge_t* other_edge) { return (other_edge == edge); } );
    return edgeIter != _edges_search.cend();
}
//-----------------------------------------------------------------------------

/* Graph Group Management *///-------------------------------------------------
template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::insert_group(group_t* group) -> bool
{
    if (group == nullptr)
        return false;
    if (insert_node(group)) {
        group->set_graph(this);
        container_adapter<groups_t>::insert(group, _groups);
        observable_base_t::notify_group_inserted(*group);
        return true;
    }
    return false;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::remove_group(group_t* group) -> bool
{
    if (group == nullptr)
        return false;
    observable_base_t::notify_group_removed(*group);
    group->set_graph(nullptr);
    for (auto node : group->get_nodes())
        node->set_group(nullptr);
    container_adapter<groups_t>::remove(group, _groups);
    return remove_node(group);   // Group destroyed here
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::has_group(const group_t* group) const -> bool
{
    if (group == nullptr)
        return false;
    return _groups.contains(const_cast<group_t*>(group));
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::group_node(node_t* node, group_t* group) -> bool
{
    if (node == nullptr ||
        group == nullptr)
        return false;
    if (node->get_group() == group)
        return true;
    node->set_group(group);
    container_adapter<nodes_t>::insert(node, group->get_nodes());
    return true;
}

template <class graph_base_t,
          class node_t,
          class group_t,
          class edge_t>
auto    graph<graph_base_t, node_t,
              group_t, edge_t>::ungroup_node(node_t* node, node_t* group) -> bool
{
    if (node == nullptr ||
        group == nullptr)
        return false;
    if (node->get_group() == nullptr ||
        node->get_group() != group) {
        std::cerr << "gtpo::group<>::ungroup_node(): Error: trying to ungroup a node "
                     "that is not part of group." << std::endl;
        return false;
    }
    container_adapter<nodes_t>::remove(node, group->get_nodes());
    node->set_group(nullptr);  // Warning: group must remain valid while notify_node_removed() is called
    return true;
}
//-----------------------------------------------------------------------------

} // ::gtpo

