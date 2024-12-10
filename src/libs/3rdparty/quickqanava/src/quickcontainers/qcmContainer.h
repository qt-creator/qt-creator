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
// This file is a part of the QuickContainers library.
//
// \file    qcmContainer.h
// \author  benoit@destrat.io
// \date    2015 06 20
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQmlEngine>           // QQmlEngine::setObjectOwnership()
#include <QAbstractListModel>

// QuickContainers headers
#include "qcmAbstractContainer.h"
#include "qcmAdapter.h"
#include "qcmContainerModel.h"

// Std headers
#include <memory>       // shared_ptr, weak_ptr
#include <type_traits>  // integral_constant
#include <utility>      // std::declval

QT_BEGIN_NAMESPACE

#ifndef COMPARE_WEAK_PTR
#define COMPARE_WEAK_PTR

//! Compare two std::weak_ptr that must have been checked for expired() and nullptr content (ie use_count()==0).
template < class T >
auto    qcm_compare_weak_ptr( const std::weak_ptr<T>& left, const std::weak_ptr<T>& right ) noexcept -> bool {
    return !left.owner_before( right ) && !right.owner_before( left );
}

template< typename T >
inline bool operator==(const std::weak_ptr<T>& e1, const std::weak_ptr<T>& e2)
{
    return qcm_compare_weak_ptr( e1, e2 );
}

template<typename T>
inline uint qHash(const std::weak_ptr<T>& t, uint seed ) /*noexcept( qHash(t) ) )*/
{
    return qHash( static_cast<T*>( t.lock().get() ), seed );
}
#endif

QT_END_NAMESPACE


namespace qcm { // ::qcm

/*! \brief Expose a standard container such as QList or QVector to a Qt QAbstractItemModel abstract list model easy to use in a QML application.
 *
 * Defining a item model for a QVector container of int or pointers on QObject:
 * \code
 *   using Ints = qcm::Container< QVector, int >;
 *   Ints ints;
 *   using QObjects = qcm::Container< QVector, QObject* >;
 *   QObjects qobjects;
 * \endcode
 *
 * \c ints or \c qobjects could then be used in any QML item model view such as ListView, ComboBox (with displayRole set
 * to "itemLabel") or GridView.
 *
 * \note Container containers could be iterated standard C++11 for(:) keyword. Qt \c for_each "keyword" usually
 * generate compilations errors for non const iterator because container model (volontarily)
 * does not define a copy constructor.
 *
 * Do not forget to declare the type to QML:
 * \code
 * using MyContainer = qcm::Container<Qlist, QObject>;
 * QML_DECLARE_TYPE( MyContainer )
 * \endcode
 *
 * When a container is defined with standard library smart pointers (currently std::weak_ptr or std::shared_ptr) or QPointer<>, the resulting types
 * must be declared with Q_DECLARE_METATYPE:
 * \code
 * #include <memory>
 *
 * Q_DECLARE_METATYPE( std::shared_ptr<QObject> )
 * using MyContainer = qcm::Container< QVector, std::shared_ptr<QObject> >;
 *
 * Q_DECLARE_METATYPE( std::weak_ptr<QObject> )
 * using MyContainer = qcm::Container< QVector, std::weak_ptr<QObject> >;
 * \endcode
 *
 * \note std::unique_ptr and QScopedPointer are not supported since they are not commonly used in containers. Support
 * for QSharedPointer and QWeakPointer is not planned (but easy to implement) since they are conceptually equivalent
 * to existing standard smart pointers: std::shared_ptr and std::weak_ptr.
 *
 * \warning Depending of your build configuration: method indexOf() specialization is O(n) for containers of std::weak_ptr (with
 * n the number of items in the underlining container, even if it is a sorted container defining fast lookup methods). Note
 * 20161125: There is actually a qobjectfriend == operator defined for weak_ptr in ... Qt namespace, be aware of it since it could
 * collide with user code (behaviour for expired weak_ptr is undefined...).
 *
 * \warning data() method for containers of objects inheriting QObject stored in std::shared_ptr<> or
 * std::weak_ptr<> return QVariant variants that could be converted back to QObject* with a qvariant_cast() (this
 * behaviour is "transparent" for the user on the QML side).
 *
 * \warning data() method will return a QVariant with a QObject* value when container is shared_ptr_qobject_type
 * or weak_ptr_qobject_type. In ptr_qobject_type, the returned QVariant will not be QObject but T*. Sometime QML
 * might have trouble making a distinction between a QVariant encoding a QObject* while it should be a concrete
 * QObject* subclass.
 */
template <template<typename...CArgs> class C, class T >
class Container : public AbstractContainer
{
    /*! \name Container Object Management *///------------------------
    //@{
public:
    explicit Container(QObject* parent = nullptr) :
        AbstractContainer{parent} { }
    virtual ~Container() {
        if (_model != nullptr) {
            delete _model;
            _model = nullptr;
        }
    }
    Container(const Container<C, T>& container) = delete;

protected:
    using ModelImpl = qcm::ContainerModelImpl<qcm::Container<C, T>, T>;
    virtual void createModel() override {
        _modelImpl = new ModelImpl{*this};
        _model = _modelImpl;
    }
private:
    QPointer<ModelImpl> _modelImpl = nullptr;
public:
    using   Item_type = T;  // Reused from qcm::ContainerModel
    //@}
    //-------------------------------------------------------------------------

    /*! \name STL Interface *///-----------------------------------------------
    //@{
public:
    using   const_pointer   = typename C<T>::const_pointer;
    using   const_reference = typename C<T>::const_reference;
    using   size_type       = typename C<T>::size_type;
    using   value_type      = typename C<T>::value_type;
    using   const_iterator  = typename C<T>::const_iterator;
    using   iterator        = typename C<T>::iterator;

public:
    inline auto    begin() const noexcept -> const_iterator { return _container.begin(); }
    inline auto    end() const noexcept -> const_iterator { return _container.end(); }

    inline auto    begin() noexcept -> iterator { return _container.begin(); }
    inline auto    end() noexcept -> iterator { return _container.end(); }

    inline auto    cbegin() const noexcept -> const_iterator { return _container.cbegin(); }
    inline auto    cend() const noexcept -> const_iterator { return _container.cend(); }

    //! Define a cast operator to C<T>&.
    inline operator C<T>&() noexcept { return _container; }

    //! Define a cast operator to const C<T>&.
    inline operator const C<T>&() const noexcept { return _container; }

public:
    inline auto     push_back(const T& value) -> void { append(value); }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Generic Interface *///-------------------------------------------
    //@{
private:
    inline auto     isNullPtr(T item, ItemDispatcherBase::unsupported_type )        const noexcept -> bool { return isNullPtr(item, ItemDispatcherBase::non_ptr_type{} ); }
    inline auto     isNullPtr(T item, ItemDispatcherBase::non_ptr_type )            const noexcept -> bool { Q_UNUSED(item); return false; }
    inline auto     isNullPtr(T item, ItemDispatcherBase::ptr_type )                const noexcept -> bool { return item == nullptr; }
    inline auto     isNullPtr(T item, ItemDispatcherBase::ptr_qobject_type )        const noexcept -> bool { return isNullPtr( item, ItemDispatcherBase::ptr_type{} ); }
    inline auto     isNullPtr(T item, ItemDispatcherBase::q_ptr_type )              const noexcept -> bool { return isNullPtr( item, ItemDispatcherBase::ptr_type{} ); }
    inline auto     isNullPtr(T item, ItemDispatcherBase::shared_ptr_type )         const noexcept -> bool { return !item; }
    inline auto     isNullPtr(T item, ItemDispatcherBase::shared_ptr_qobject_type ) const noexcept -> bool { return isNullPtr(item, ItemDispatcherBase::shared_ptr_type{} ); }
    inline auto     isNullPtr(T item, ItemDispatcherBase::weak_ptr_type )           const noexcept -> bool { return item.expired(); }
    inline auto     isNullPtr(T item, ItemDispatcherBase::weak_ptr_qobject_type )   const noexcept -> bool { return isNullPtr( item, ItemDispatcherBase::weak_ptr_type{} ); }
private:
    inline auto     getNullT(ItemDispatcherBase::unsupported_type )        const -> T { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::non_ptr_type )            const -> T  { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::ptr_type )                const -> T  { return nullptr; }
    inline auto     getNullT(ItemDispatcherBase::ptr_qobject_type )        const -> T  { return nullptr; }
    inline auto     getNullT(ItemDispatcherBase::q_ptr_type )              const -> T  { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::shared_ptr_type )         const -> T  { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::shared_ptr_qobject_type ) const -> T  { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::weak_ptr_type )           const -> T  { return T{}; }
    inline auto     getNullT(ItemDispatcherBase::weak_ptr_qobject_type )   const -> T  { return T{}; }

public:
    /*! \brief Shortcut to Container<T>::at() with bounding checking.
     *
     * \param i index of the element to access, if i < 0 std::out_of_range is thrown, if i >= container.size(), a nullptr or empty smart pointer is returned.
     */
    auto at(int i) const noexcept -> T {
        if (i < 0)
            return getNullT(typename ItemDispatcher<T>::type{});
        return atImpl(i, typename ItemDispatcher<T>::type{});
    }
private:
    inline auto atImpl(int i, ItemDispatcherBase::non_ptr_type)             const -> T {
        return _container.at(i);
    }
    inline auto atImpl(int i, ItemDispatcherBase::unsupported_type)         const -> T { return atImpl(i, ItemDispatcherBase::non_ptr_type{}); }

    inline auto atImpl(int i, ItemDispatcherBase::ptr_type)                 const -> T {
        return ( i < static_cast<int>(_container.size()) ? _container.at(i) : nullptr );
    }
    inline auto atImpl(int i, ItemDispatcherBase::ptr_qobject_type)         const -> T { return atImpl(i, ItemDispatcherBase::ptr_type{}); }
    inline auto atImpl(int i, ItemDispatcherBase::q_ptr_type)               const -> T { return atImpl(i, ItemDispatcherBase::ptr_type{}); }

    inline auto atImpl(int i, ItemDispatcherBase::shared_ptr_type)          const -> T {
        return (i < static_cast<int>(_container.size()) ? _container.at(i) : T{});
    }
    inline auto atImpl(int i, ItemDispatcherBase::shared_ptr_qobject_type)  const -> T { return atImpl(i, ItemDispatcherBase::shared_ptr_type{}); }

    inline auto atImpl(int i, ItemDispatcherBase::weak_ptr_type)            const -> T {
        return (i < static_cast<int>(_container.size()) ? _container.at(i) : T{});
    }
    inline auto atImpl(int i, ItemDispatcherBase::weak_ptr_qobject_type)    const -> T { return atImpl(i, ItemDispatcherBase::weak_ptr_type{}); }

public:
    //! Shortcut to Container<T>::reserve().
    void        reserve( std::size_t size ) { qcm::adapter<C,T>::reserve(_container, size); }

public:
    //! Shortcut to Container<T>::size().
    inline auto size( ) const noexcept -> decltype(std::declval<C<T>>().size()) { return _container.size( ); }

    void        append(const T& item) {
        if (isNullPtr(item, typename ItemDispatcher<T>::type{}))
            return;
        if (_model) {
            fwdBeginInsertRows(QModelIndex{},
                               static_cast<int>(_container.size()),
                               static_cast<int>(_container.size()));
            qcm::adapter<C, T>::append(_container, item);
            appendImpl(item, typename ItemDispatcher<T>::type{});
            fwdEndInsertRows();
            fwdEmitLengthChanged();
        } else {
            qcm::adapter<C, T>::append(_container, item);
            appendImpl(item, typename ItemDispatcher<T>::type{});
        }
    }

    //! Shortcut to Container<T>::insert().
    void        insert( const T& item, int i ) {
        if ( i < 0 ||           // i == 0      === prepend
             i > size() ||      // i == size() === append
             isNullPtr( item, typename ItemDispatcher<T>::type{} ) )
            return;
        if ( _model ) {
            fwdBeginInsertRows( QModelIndex{}, i, i );
            qcm::adapter<C,T>::insert(_container, item, i);
            appendImpl( item, typename ItemDispatcher<T>::type{} );
            fwdEndInsertRows( );
            fwdEmitLengthChanged();
        } else {
            qcm::adapter<C,T>::insert(_container, item, i);
            appendImpl( item, typename ItemDispatcher<T>::type{} );
        }
    }

private:
    inline auto appendImpl( const T&, ItemDispatcherBase::unsupported_type ) noexcept  -> void {}
    inline auto appendImpl( const T&, ItemDispatcherBase::non_ptr_type ) noexcept  -> void {}
    inline auto appendImpl( const T& item, ItemDispatcherBase::ptr_qobject_type ) noexcept  -> void {
        if (item != nullptr) {
            if (_modelImpl )
                _modelImpl->_qObjectItemMap.insert( { item, item } );
        }
    }
    inline auto appendImpl( const T& item, ItemDispatcherBase::q_ptr_type )   noexcept -> void {
        if ( _modelImpl &&
             item != nullptr )
            _modelImpl->_qObjectItemMap.insert( { item.data(), item } );
    }
    inline auto appendImpl( const T&, ItemDispatcherBase::shared_ptr_type ) noexcept  -> void {}
    inline auto appendImpl( const T& item, ItemDispatcherBase::shared_ptr_qobject_type ) noexcept -> void {
        // Note: deleted() signal of item is not monitored since calling delete on a std::shared_ptr manager
        // pointer throw a system exception anyway (at least on linux...)
        if ( _modelImpl &&
             item )
            _modelImpl->_qObjectItemMap.insert( { item.get(), item } );
    }
    inline auto appendImpl( const T&, ItemDispatcherBase::weak_ptr_type ) noexcept  -> void {}
    inline auto appendImpl(const T& item, ItemDispatcherBase::weak_ptr_qobject_type)   noexcept -> void {
        if (_modelImpl &&
            !item.expired())
            _modelImpl->_qObjectItemMap.insert({item.lock().get(), item});
    }

public:
    //! Shortcut to Container<T>::remove().
    void        removeAll(const T& item) {
        if (isNullPtr(item, typename ItemDispatcher<T>::type{}))
            return;
        const auto itemIndex = qcm::adapter<C,T>::indexOf(_container, item);
        if (itemIndex < 0)
            return;
        if (_model) {
            // FIXME: Model updating is actually quite buggy: removeAll might remove
            // items at multiple index, but model update is requested only for itemIndex...
            fwdBeginRemoveRows(QModelIndex{},
                               static_cast<int>(itemIndex),
                               static_cast<int>(itemIndex));
            removeImpl(item, typename ItemDispatcher<T>::type{});
            qcm::adapter<C,T>::removeAll(_container, item);
            fwdEndRemoveRows();
            fwdEmitLengthChanged();
        } else {
            qcm::adapter<C,T>::removeAll(_container, item);
        }
    }

private:
    inline auto removeImpl( const T&, ItemDispatcherBase::unsupported_type )               -> void {}
    inline auto removeImpl( const T&, ItemDispatcherBase::non_ptr_type )                   -> void {}
    inline auto removeImpl( const T&, ItemDispatcherBase::ptr_type )                       -> void {}
    inline auto removeImpl( const T& item, ItemDispatcherBase::ptr_qobject_type )          -> void {
        if (_modelImpl &&
            item != nullptr) {
            item->disconnect(0, _modelImpl, 0);
            _modelImpl->_qObjectItemMap.erase(item);
        }
    }
    inline auto removeImpl( const T& item, ItemDispatcherBase::q_ptr_type )                -> void {
        if ( _modelImpl && item != nullptr ) {
            item->disconnect( 0, _modelImpl, 0 );
            _modelImpl->_qObjectItemMap.erase(item.data());
        }
    }
    inline auto removeImpl( const T&, ItemDispatcherBase::shared_ptr_type )                -> void {}
    inline auto removeImpl( const T& item, ItemDispatcherBase::shared_ptr_qobject_type )   -> void {
        QObject* qObject = qobject_cast<QObject*>(item.get());
        if ( _modelImpl && qObject != nullptr ) {
            qObject->disconnect(0, _modelImpl, 0);
            _modelImpl->_qObjectItemMap.erase(qObject);
        }
    }
    inline auto removeImpl( const T&, ItemDispatcherBase::weak_ptr_type )                  -> void {}
    inline auto removeImpl( const T& item, ItemDispatcherBase::weak_ptr_qobject_type )     -> void {
        if (!_modelImpl || item.expired())
            return;
        QObject* qObject = qobject_cast<QObject*>(item.lock().get());
        if (qObject != nullptr) {
            qObject->disconnect(0, _modelImpl, 0);
            _modelImpl->_qObjectItemMap.erase(qObject);
        }
    }

public:
    inline  void    clear() noexcept {
        if (_model && _modelImpl) {
            fwdBeginResetModel();
            _modelImpl->_qObjectItemMap.clear();
            _container.clear();
            fwdEndResetModel();
            fwdEmitLengthChanged();
        } else
            _container.clear();
    }

public:
    /*! \brief Clear the container and optionally call delete on contained objects.
     *
     * Argument \c deleteContent is taken into account only in the following situations:
     * \li Delete will be called if \c deleteContent is true if \c T is a pointer type, or a pointer on QObject.
     * \li If \c T is a smart pointer (either a std::shared_ptr or std::weak_ptr), or a non pointer type (either QObject or POD), delete
     *      won't be called even if \c deleteContent is true.
     *
     * \arg deleteContent if true, delete will eventually be called on each container item before the container is cleared.
     */
    void    clear(bool deleteContent, bool notify = true) {
        if (_model && _modelImpl) {
            if (notify)
                fwdBeginResetModel();
            clearImpl(deleteContent, typename ItemDispatcher<T>::type{});
            _modelImpl->_qObjectItemMap.clear();
            _container.clear();
            if (notify) {
                fwdEndResetModel();
                fwdEmitLengthChanged();
            }
        } else {
            clearImpl(deleteContent, typename ItemDispatcher<T>::type{});
            _container.clear();
        }
    }

private:
    inline auto clearImpl( bool deleteContent, ItemDispatcherBase::ptr_type ) -> void {
        if (deleteContent) {
            for (const auto& p: std::as_const(_container))
                delete p;
        }
    }
    inline auto clearImpl( bool deleteContent, ItemDispatcherBase::ptr_qobject_type ) -> void {
        clearImpl( deleteContent, ItemDispatcherBase::ptr_type{} );
    }

public:
    /*! \brief Return true if the underlying container contains \c item (optimized for Qt containers, might be less fast on Std ones).
     *
     * \arg item    if nullptr, return false.
     */
    inline auto    contains(const T item) const -> bool { return qcm::adapter<C, T>::contains(_container, item); }

    /*! \brief Shortcut to Container<T>::indexOf(), return index of a given \c item element in this model container.
     *
     * \arg item    if nullptr, return -1.
     */
    inline auto    indexOf(T item) const -> int { return qcm::adapter<C, T>::indexOf(_container, item); }

private:
    C<T>                _container;
public:
    //! The preffered way of accessing internal list is to use cast operator with static_cast<const Container<T>>( Container ).
    inline const C<T>&  getContainer() const { return _container; }
protected:
    inline C<T>&        getContainer() { return _container; }
    //@}
    //-------------------------------------------------------------------------
};

} // ::qcm
