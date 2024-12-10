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
// \file	observable.h
// \author	benoit@destrat.io
// \date	2016 02 08
//-----------------------------------------------------------------------------

#pragma once

// STD headers
#include <iostream>
#include <cstddef>          // std::size_t
#include <functional>       // std::function
#include <vector>
#include <memory>
#include <utility>          // c++14 std::index_sequence

namespace gtpo { // ::gtpo

/*! \brief Empty interface definition for graph primitives supporting observable concept.
 */
class abstract_observable {
public:
    abstract_observable() {}
    virtual ~abstract_observable() = default;

    abstract_observable(const abstract_observable&) = default;
    abstract_observable& operator=( const abstract_observable&) = default;
    abstract_observable(abstract_observable&&) = default;
    abstract_observable& operator=(abstract_observable&&) = default;
};

/*! \brief Base class for all type supporting observers (actually gtpo::graph and gtpo::group).
 *
 * \nosubgrouping
 */
template <class observer_t>
class observable : public abstract_observable
{
    /*! \name observable Object Management *///--------------------------------
    //@{
public:
    observable() : abstract_observable() { }
    virtual ~observable() noexcept {
        _observers.clear();
    }
    observable(const observable<observer_t>&) = default;
    observable& operator=(const observable<observer_t>&) = default;

public:
    //! Clear all registered behaviours (they are automatically deleted).
    inline  auto    clear() -> void { _observers.clear(); }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Dynamic (virtual) Observer Management *///-----------------------
    //@{
public:
    /*! \brief Add a behaviour to the primitive inheriting from this behaviourable (observable) interface.
     *
     * \note This is a sink method, Behaviourable get ownership for \c behaviour, since it is
     * difficult to "downcast" unique_ptr, use the following code for calling:
     * \code
     *   gtpo::graph<> sg;
     *   sg.addBehaviour( std::make_unique< MyBehaviour >( ) );
     * \endcode
     */
    inline auto     add_observer(std::unique_ptr<observer_t> observer) -> void {
        _observers.emplace_back(std::move(observer));
    }

    //! std::vector of std::unique_ptr pointers on behaviour.
    using observers_t = std::vector<std::unique_ptr<observer_t>>;

public:
    //! Return true if an observer is currently registered (ie getObservers().size() > 0).
    inline auto    hasObservers() const noexcept -> bool { return _observers.size() > 0; }

    //! Return a read only container of actually registered observers.
    inline auto    getObservers() const noexcept -> const observers_t& { return _observers; }

protected:
    observers_t    _observers;
    //@}
    //-------------------------------------------------------------------------
};


template <class target_t>
class observer;

template <class node_t, class edge_t>
class node_observer;

/*! \brief Observation interface for gtpo::node.
 *
 * \nosubgrouping
 */
template <class node_t, class edge_t>
class observable_node : public observable<gtpo::node_observer<node_t, edge_t>>
{
    /*! \name observable_node Object Management *///---------------------------
    //@{
public:
    using node_observer_t = gtpo::node_observer<node_t, edge_t>;
    using super_t = observable<gtpo::node_observer<node_t, edge_t>>;
    observable_node() noexcept : super_t{} { }
    ~observable_node() noexcept = default;
    observable_node(const observable_node<node_t, edge_t>&) = delete;
    observable_node& operator=(const observable_node<node_t, edge_t>&) = delete;

    /*! \name Notification Helper Methods *///---------------------------------
    //@{
public:
    auto    add_node_observer(std::unique_ptr<node_observer_t> observer) -> void {
        super_t::add_observer(std::move(observer));
    }

    auto    notify_in_node_inserted(node_t& target, node_t& node, const edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_in_node_inserted(target, node, edge);
    }

    auto    notify_in_node_removed(node_t& target, node_t& node, const edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_in_node_removed(target, node, edge);
    }

    auto    notify_in_node_removed(node_t& target) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_in_node_removed(target);
    }

    auto    notify_out_node_inserted(node_t& target, node_t& node, const edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_out_node_inserted(target, node, edge);
    }

    auto    notify_out_node_removed(node_t& target, node_t& node, const edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_out_node_removed(target, node, edge);
    }

    auto    notify_out_node_removed(node_t& target) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_out_node_removed(target);
    }
    //@}
    //-------------------------------------------------------------------------
};


template <class graph_t, class node_t, class edge_t, class group_t>
class graph_observer;

/*! \brief Enable static and dynamic behaviour support for gtpo::Graph.
 *
 * \nosubgrouping
 */
template <class graph_t, class node_t, class edge_t, class group_t>
class observable_graph : public observable<gtpo::graph_observer<graph_t, node_t, edge_t, group_t> >
{
    /*! \name behaviourable_graph Object Management *///------------------------
    //@{
public:
    using graph_observer_t = gtpo::graph_observer<graph_t, node_t, edge_t, group_t>;
    using super_t = observable<gtpo::graph_observer<graph_t, node_t, edge_t, group_t> >;
    observable_graph() : super_t{} { }
    virtual ~observable_graph() noexcept = default;
    observable_graph(const observable_graph<graph_t, node_t, edge_t, group_t>&) = delete;
    observable_graph& operator=(const observable_graph<graph_t, node_t, edge_t, group_t>&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Notification Helper Methods *///---------------------------------
    //@{
public:
    inline auto     add_graph_observer(std::unique_ptr<graph_observer_t> observer) -> void {
        super_t::add_observer(std::move(observer));
    }

    auto    notify_node_inserted(node_t& node) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_node_inserted(node);
    }

    auto    notify_node_removed(node_t& node) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_node_removed(node);
    }

    auto    notify_edge_inserted(edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_edge_inserted(edge);
    }

    auto    notify_edge_removed(edge_t& edge) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_edge_removed(edge);
    }

    auto    notify_group_inserted(group_t& group) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_group_inserted(group);
    }

    auto    notify_group_removed(group_t& group) noexcept -> void {
        for (auto& observer: super_t::_observers)
            if (observer)
                observer->on_group_removed(group);
    }
    //@}
    //-------------------------------------------------------------------------
};

} // ::gtpo
