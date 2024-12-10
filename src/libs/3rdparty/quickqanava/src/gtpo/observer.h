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
// \file	observer.h
// \author	benoit@destrat.io
// \date	2016 02 08
//-----------------------------------------------------------------------------

#pragma once

// STD headers
#include <cstddef>          // std::size_t
#include <functional>       // std::function
#include <vector>
#include <memory>
#include <utility>          // c++14 std::index_sequence

// GTPO headers
#include "./observable.h"

namespace gtpo { // ::gtpo

/*! \brief Base class for concrete observers.
 */
template <class target_t>
class observer
{
public:
    observer() noexcept { }
    ~observer() noexcept { }

   observer(const observer& rhs) = default;
   observer& operator=(const observer& rhs) = default;
   observer(observer&& rhs) noexcept = default;
   observer& operator=(observer&& rhs) noexcept = default;

public:
    target_t*   get_target() noexcept { return _target; }
    void        set_target(target_t* target) { _target = target; }
protected:
    target_t*   _target = nullptr;

public:
    inline auto getName() const noexcept -> const std::string& { return _name; }
protected:
    inline auto setName(const std::string& name)  noexcept -> void { _name = name; }
private:
    std::string _name = "";

public:
    /*! \brief Enable this behaviour until it is disabled again with a call to disable().
     *
     * \note enabling a disabled behaviour does not apply behaviour on changes made
     *       while it was disabled.
     * \sa isEnabled() for enabled state description.
     */
    inline auto     enable() noexcept -> void { _enabled = true; }
    /*! \brief Disable this behaviour until it is enabled back wit a call to enable().
     *
     * \sa isEnabled() for enabled state description.
     */
    inline auto     disable() noexcept -> void { _enabled = false; }
    /*! \brief Return the actual "enabled" state for this behaviour.
     *
     *  This method could be usefull in very specific use cases, such as serialization or insertion of a large number
     *  of nodes in a graph or group where this behaviour is applied.
     *  A behaviour is enabled by default after its creation.
     *
     *  \return true if enable() has been called, false if disable() has been called.
     */
    inline auto     isEnabled() const noexcept -> bool { return _enabled; }

protected:
    bool            _enabled = true;
};


/*! \brief Abstract interface for dynamic (virtual) node behaviours.
 *
 */
template <class node_t, class edge_t>
class node_observer : public observer<node_t>
{
public:
    template<class, class>
    friend class gtpo::observable_node;

    node_observer() noexcept : observer<node_t>{} { }
    virtual ~node_observer() = default;
    node_observer(const node_observer<node_t, edge_t>& ) = delete;
    node_observer& operator=(const node_observer<node_t, edge_t>&) = delete;
    node_observer(node_observer<node_t, edge_t>&&) = delete;
    node_observer& operator=(node_observer<node_t, edge_t>&&) = delete;

protected:
    //! \brief Called immediatly after an in-edge with \c source has been inserted.
    virtual void    on_in_node_inserted(node_t& target, node_t& source, const edge_t& edge)  noexcept { static_cast<void>(target); static_cast<void>(source); static_cast<void>(edge); }

    //! \brief Called when an in-edge with \c source is about to be removed.
    virtual void    on_in_node_removed(node_t& target, node_t& source, const edge_t& edge)  noexcept { static_cast<void>(target); static_cast<void>(source); static_cast<void>(edge); }

    //! \brief Called immediatly after an in node has been removed.
    virtual void    on_in_node_removed(node_t& target)  noexcept { static_cast<void>(target); }

    //! \brief Called immediatly after an out-edge with \c destination has been inserted.
    virtual void    on_out_node_inserted(node_t& target, node_t& destination, const edge_t& edge)  noexcept { static_cast<void>(target); static_cast<void>(destination); static_cast<void>(edge); }

    //! \brief Called when an out-edge with \c destination is about to be removed.
    virtual void    on_out_node_removed(node_t& target, node_t& destination, const edge_t& edge)  noexcept { static_cast<void>(target); static_cast<void>(destination); static_cast<void>(edge); }

    //! \brief Called immediatly after an out-edge has been removed.
    virtual void    on_out_node_removed(node_t& target)  noexcept { static_cast<void>(target); }
};


/*! \brief Enable dynamic observers on gtpo::graph<>.
 *
 * \note Add enable_graph_behaviour<> to gtpo::config::graph_behaviours tuple to enable dynamic
 * graph behaviours, otherwise, any gtpo::dynamic_graph_behaviour added using gtpo::graph::add_dynamic_graph_behaviour()
 * method won't be notified of changes in topology.
 */
template <class graph_t, class node_t, class edge_t, class group_t>
class graph_observer : public observer<graph_t>
{
public:
    template<class, class, class, class>
    friend class gtpo::observable_graph;

    using this_t = gtpo::graph_observer<graph_t, node_t, edge_t, group_t>;
    graph_observer() noexcept : gtpo::observer<graph_t>{} {}
    virtual ~graph_observer() noexcept = default;
    graph_observer(const this_t&) = delete;
    graph_observer& operator=(const this_t&) = delete;

    /*! \name Graph Dynamic (virtual) Notification Interface *///--------------
    //@{
protected:
    //! Called immediatly after \c node has been inserted in graph.
    virtual void    on_node_inserted(node_t& node) noexcept { static_cast<void>(node); }
    //! Called immediatly before \c node is removed from graph.
    virtual void    on_node_removed(node_t& node) noexcept { static_cast<void>(node); }

    //! Called immediatly after \c group has been inserted in graph.
    virtual void    on_group_inserted(node_t& group) noexcept { static_cast<void>(group); }
    //! Called immediatly before \c group is removed from graph.
    virtual void    on_group_removed(node_t& group) noexcept { static_cast<void>(group); }

protected:
    //! Called immediatly after \c edge has been inserted.
    virtual void    on_edge_inserted(edge_t& edge) noexcept { static_cast<void>(edge); }
    //! Called when \c edge is about to be removed.
    virtual void    on_edge_removed(edge_t& edge) noexcept { static_cast<void>(edge); }
    //@}
    //-------------------------------------------------------------------------
};
} // ::gtpo
