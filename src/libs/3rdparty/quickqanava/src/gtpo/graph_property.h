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
// \file	graph_property.h
// \author	benoit@destrat.io
// \date	2018 06 25
//-----------------------------------------------------------------------------

#pragma once

// Std headers
#include <cstddef>
#include <iostream>     // std::cout

namespace gtpo { // ::gtpo

/*! \brief Add support for a pointer on owning graph for primitive (either a node, edge or group).
 *
 * \note The pointer on graph is intentionally a raw pointer to avoid any "smart pointer overhead". Moreover,
 * GTpo is inherently designed to be used from Qt/QML who is not managing memory with std::make_shared, hence,
 * using a std::weak_ptr or inheriting graph from std::enable_shared_from_this would be a nonsense...
 */
template<class graph_t>
class graph_property_impl
{
public:
    friend graph_t;   // graph need access to graph_property_impl<>::set_graph()

    graph_property_impl() noexcept = default;
    ~graph_property_impl() noexcept = default;
    graph_property_impl(const graph_property_impl<graph_t>&) noexcept = default;
    graph_property_impl& operator=(const graph_property_impl<graph_t>&) noexcept = default;
    graph_property_impl(graph_property_impl<graph_t>&&) noexcept = default;
    graph_property_impl& operator=(graph_property_impl<graph_t>&&) noexcept = default;

public:
    inline  graph_t*        get_graph() noexcept { return _graph; }
    inline  const graph_t*  get_graph() const noexcept { return _graph; }
    inline  void            set_graph(graph_t* graph) noexcept { _graph = graph; }
    inline  void            set_graph(void* graph) noexcept { _graph = reinterpret_cast<graph_t*>(graph); }
    inline  void            set_graph(std::nullptr_t) noexcept { _graph = nullptr; }
public:
    // Note: This is the only raw pointer in GTpo.
    graph_t*                _graph = nullptr;
};

} // ::gtpo

