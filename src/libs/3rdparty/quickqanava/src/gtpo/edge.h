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
// \file	edge.h
// \author	benoit@destrat.io
// \date	2017 03 04
//-----------------------------------------------------------------------------

#pragma once

// STD headers
#include <unordered_set>
#include <cassert>
#include <iterator>         // std::back_inserter

// GTpo headers
#include "./graph_property.h"

namespace gtpo { // ::gtpo

/*! Directed edge linking two nodes in a graph.
 *
 * \nosubgrouping
 */
template <class edge_base_t,
          class graph_t,
          class node_t>
class edge : public edge_base_t,
             public graph_property_impl<graph_t>
{
    /*! \name Edge Construction *///-------------------------------------------
    //@{
public:
    using edge_t    = edge<edge_base_t, graph_t, node_t>;

    edge(edge_base_t* parent = nullptr) noexcept : edge_base_t{parent} {}
    explicit edge(const node_t* src, const node_t* dst) :
        edge_base_t{}, _src{src}, _dst{dst} { }
    virtual ~edge() {
        if (graph_property_impl<graph_t>::_graph != nullptr)
            std::cerr << "gtpo::edge<>::~edge(): Warning: an edge has been deleted before beeing " <<
                         "removed from the graph." << std::endl;
    }
    edge(const edge&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Edge Meta Properties *///----------------------------------------
    //@{
public:
    //! Get the edge current serializable property (false=not serializable, for example a control node).
    inline  auto    get_serializable() const -> bool { return _serializable; }
    //! Shortcut to get_serializable().
    inline  auto    is_serializable() const -> bool { return get_serializable(); }
    //! Change the edge serializable property (it will not trigger an edge changed call in graph behaviour).
    inline  auto    set_serializable(bool serializable) -> void { _serializable = serializable; }
private:
    //! Edge serializable property (default to true ie serializable).
    bool            _serializable = true;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Source / Destination Management *///-----------------------------
    //@{
public:
    inline auto set_src(node_t* src) noexcept -> void { _src = src; }
    inline auto set_dst(node_t* dst) noexcept -> void { _dst = dst; }
    inline auto get_src() noexcept -> node_t* { return _src; }
    inline auto get_src() const noexcept -> const node_t* { return _src; }
    inline auto get_dst() noexcept -> node_t* { return _dst; }
    inline auto get_dst() const noexcept -> const node_t* { return _dst; }
private:
    node_t* _src = nullptr;
    node_t* _dst = nullptr;
    //@}
    //-------------------------------------------------------------------------
};

} // ::gtpo

