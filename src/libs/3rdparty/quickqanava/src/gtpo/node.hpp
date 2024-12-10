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
// This file is a part of the GTpo software.
//
// \file	node.hpp
// \author	benoit@destrat.io
// \date	2016 01 22
//-----------------------------------------------------------------------------

namespace gtpo { // ::gtpo

/* node Edges Management *///-----------------------------------------------
template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
auto node<node_base_t,
          graph_t,
          node_t,
          edge_t,
          group_t>::add_out_edge(edge_t* outEdge) -> bool
{
    if (outEdge == nullptr)
        return false;
    auto outEdgeSrc = outEdge->get_src();
    if (!outEdgeSrc ||
        outEdgeSrc != this)  // Out edge source should point to target node
        outEdge->set_src(reinterpret_cast<node_t*>(this));
    container_adapter<edges_t>::insert(outEdge, _out_edges);
    if (outEdge->get_dst() != nullptr) {
        container_adapter<nodes_t>::insert(outEdge->get_dst(), _out_nodes);
        observable_base_t::notify_out_node_inserted(*reinterpret_cast<node_t*>(this),
                                                    *outEdge->get_dst(), *outEdge);
        return true;
    }
    return false;
}

template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
auto node<node_base_t,
          graph_t,
          node_t,
          edge_t,
          group_t>::add_in_edge(edge_t* in_edge) -> bool
{
    if (in_edge == nullptr)
        return false;
    auto in_edge_dst = in_edge->get_dst();
    if (!in_edge_dst ||
            in_edge_dst != this) // In edge destination should point to target node
        in_edge->set_dst(reinterpret_cast<node_t*>(this));
    container_adapter<edges_t>::insert(in_edge, _in_edges);
    if (in_edge->get_src() != nullptr) {
        container_adapter<nodes_t>::insert(in_edge->get_src(), _in_nodes);
        observable_base_t::notify_in_node_inserted(*reinterpret_cast<node_t*>(this),
                                                   *in_edge->get_src(), *in_edge);
    }
    return true;
}

template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
auto node<node_base_t,
          graph_t,
          node_t, edge_t,
          group_t>::remove_out_edge(const edge_t* outEdge) -> bool
{
    if (outEdge == nullptr)
        return false;
    auto outEdgeSrc = outEdge->get_src();
    if (outEdgeSrc == nullptr) {
        std::cerr << "gtpo::node<>::remove_out_edge(): Error: Out edge source is nullptr or different from this node." << std::endl;
        return false;
    }

    auto outEdgeDst = outEdge->get_dst();
    if (outEdgeDst != nullptr) {
        observable_base_t::notify_out_node_removed(*reinterpret_cast<node_t*>(this),
                                                   *const_cast<node_t*>(outEdge->get_dst()), *outEdge);
    }
    container_adapter<edges_t>::remove(const_cast<edge_t*>(outEdge), _out_edges);
    container_adapter<nodes_t>::remove(const_cast<node_t*>(outEdge->get_dst()), _out_nodes);
    if (get_in_degree() == 0) {
        graph_t* graph = this->get_graph();
        if (graph != nullptr)
            graph->install_root_node(reinterpret_cast<node_t*>(this));
    }
    observable_base_t::notify_out_node_removed(*reinterpret_cast<node_t*>(this));
    return true;
}

template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
auto node<node_base_t,
          graph_t,
          node_t, edge_t,
          group_t>::remove_in_edge(const edge_t* inEdge) -> bool
{
    if (inEdge == nullptr) {
        std::cerr << "gtpo::node<>::remove_in_edge(): Error: In edge is nullptr." << std::endl;
        return false;
    }
    auto in_edge_dst = inEdge->get_dst();
    if (in_edge_dst == nullptr ||
        in_edge_dst != this) {      // in edge dst must be this node
        std::cerr << "gtpo::node<>::remove_in_edge(): Error: In edge destination is nullptr or different from this node." << std::endl;
        return false;
    }

    auto inEdgeSrc = inEdge->get_src();
    if (inEdgeSrc == nullptr) {
        std::cerr << "gtpo::node<>::remove_in_edge(): Error: In edge source is expired." << std::endl;
        return false;
    }
    observable_base_t::notify_in_node_removed(*reinterpret_cast<node_t*>(this),
                                              *const_cast<node_t*>(inEdge->get_src()), *inEdge);
    container_adapter<edges_t>::remove(const_cast<edge_t*>(inEdge), _in_edges);
    container_adapter<nodes_t>::remove(const_cast<node_t*>(inEdge->get_src()), _in_nodes);
    if (get_in_degree() == 0) {
        graph_t* graph = this->get_graph();
        if (graph != nullptr)
            graph->install_root_node(reinterpret_cast<node_t*>(this));
    }

    observable_base_t::notify_in_node_removed(*reinterpret_cast<node_t*>(this));
    return true;
}
//-----------------------------------------------------------------------------

/* Group Nodes Management *///-------------------------------------------------
template <class node_base_t,
          class graph_t,
          class node_t,
          class edge_t,
          class group_t>
auto node<node_base_t,
          graph_t,
          node_t, edge_t,
          group_t>::has_node(const node_t* node ) const noexcept -> bool
{
    if (node == nullptr)
        return false;
    auto nodeIter = std::find_if(_nodes.begin(), _nodes.end(),
                                 [=](const node_t* candidate_node){ return (node == candidate_node); } );
    return nodeIter != _nodes.end();
}
//-----------------------------------------------------------------------------

} // ::gtpo
