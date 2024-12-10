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
// \file    qcmContainerModel.h
// \author  benoit@destrat.io
// \date    2016 11 25
//-----------------------------------------------------------------------------

#pragma once

// Std headers
#include <memory>
#include <unordered_map>

// Qt headers
#include <QObject>
#include <QDebug>
#include <QQmlEngine>   // Q_QML_DECLARE_TYPE, qmlEngine()
#include <QAbstractListModel>

namespace qcm { // ::qcm

struct ItemDispatcherBase {
    using unsupported_type          = std::integral_constant<int, 0>;

    using non_ptr_type              = std::integral_constant<int, 1>;
    using ptr_type                  = std::integral_constant<int, 2>;
    using ptr_qobject_type          = std::integral_constant<int, 3>;

    // Note: Not actually used, extension for later QPointer support
    using q_ptr_type                = std::integral_constant<int, 4>;

    using shared_ptr_type           = std::integral_constant<int, 5>;
    using shared_ptr_qobject_type   = std::integral_constant<int, 6>;

    using weak_ptr_type             = std::integral_constant<int, 7>;
    using weak_ptr_qobject_type     = std::integral_constant<int, 8>;

    template <typename T>
    inline static constexpr auto debug_type() -> const char* { return "unsupported"; }
};

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::non_ptr_type>() -> const char* { return "non_ptr_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::ptr_type>() -> const char* { return "ptr_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::ptr_qobject_type>() -> const char* { return "ptr_qobject_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::q_ptr_type>() -> const char* { return "q_ptr_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::shared_ptr_type>() -> const char* { return "shared_ptr_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::shared_ptr_qobject_type>() -> const char* { return "shared_ptr_qobject_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::weak_ptr_type>() -> const char* { return "weak_ptr_type"; }

template <>
inline auto ItemDispatcherBase::debug_type<ItemDispatcherBase::weak_ptr_qobject_type>() -> const char* { return "weak_ptr_qobject_type"; }

template <typename Item_type>
struct ItemDispatcher : public ItemDispatcherBase {
    // QObject and non copy constructible POD are unsupported
    using type = typename std::conditional<std::is_base_of<QObject, Item_type >::value ||
                                           !std::is_copy_constructible<Item_type>::value,
                                                                                ItemDispatcherBase::unsupported_type,
                                                                                ItemDispatcherBase::non_ptr_type
                                          >::type;
};


template <typename Item_type>
struct ItemDispatcher<Item_type*>  : public ItemDispatcherBase {
    using type = typename std::conditional< std::is_base_of< QObject, typename std::remove_pointer< Item_type >::type >::value,
                                                                                ItemDispatcherBase::ptr_qobject_type,
                                                                                ItemDispatcherBase::ptr_type
                                            >::type;
};

template <typename Item_type>
struct ItemDispatcher<QPointer<Item_type>>  : public ItemDispatcherBase {
    using type = ItemDispatcherBase::q_ptr_type;
};

template <typename Item_type>
struct ItemDispatcher<std::shared_ptr<Item_type>>  : public ItemDispatcherBase {
    using type = typename std::conditional<std::is_base_of<QObject, Item_type>::value,
                                           ItemDispatcherBase::shared_ptr_qobject_type,
                                           ItemDispatcherBase::shared_ptr_type
                                          >::type;
};

template <typename Item_type>
struct ItemDispatcher<std::weak_ptr<Item_type>>  : public ItemDispatcherBase {
    using type = typename std::conditional<std::is_base_of<QObject, Item_type>::value,
                                                                       ItemDispatcherBase::weak_ptr_qobject_type,
                                                                       ItemDispatcherBase::weak_ptr_type
                                            >::type;
};

/*! \brief Reference on an abstrat container model allowing to access a qcm::ContainerModel directly from QML.
 *
 * \code
 * Button {
 *   text: "remove"
 *   onClicked: { testModel.insert(object) }
 * }
 * ListView {
 *   id: testList
 *   model: testModel
 * }
 * \endcode
 *
 * \note ContainerModel is very simlar in meaning and interface to a read/write QQmlListReference,
 * please refer to Qt documentation.
 */
class ContainerModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ContainerModel() : QAbstractListModel{nullptr} {}
    virtual ~ContainerModel() override = default;
    ContainerModel(const ContainerModel&) = delete;
    ContainerModel& operator=(const ContainerModel&) = delete;
    ContainerModel(ContainerModel&&) = delete;

    /*! \name Qt Abstract Model Interface *///---------------------------------
    //@{
public:
    virtual int         rowCount(const QModelIndex& parent = QModelIndex{}) const override { Q_UNUSED(parent); return 0; }
    virtual QVariant    data(const QModelIndex& index, int role = Qt::DisplayRole) const override { Q_UNUSED(index); Q_UNUSED(role); return QVariant{}; }

public:
    enum PropertiesRoles {
        ItemDataRole = Qt::UserRole + 1
    };
protected:
    virtual QHash<int, QByteArray>  roleNames() const override {
        return {{static_cast<int>(ItemDataRole),   "itemData"}};
    }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Display Role Monitoring *///-------------------------------------
    //@{
public:
    /*! \brief When the container is a container of QObject*, the data() method will return the content of property
     *  \c displayRoleProperty for both Qt::DisplayRole and "itemLabel" dispay role.
     *
     * To use a container of QObject* with a Q_PROPERTY( QString label, ...) property, use the following
     * configuration:
     * \code
     * qcm::ContainerModel< QVector, QObject* > myContainer;
     * myContainer->setItemDisplayRole( QStringLiteral( "label" ) );    // Expose that as myContainer context property in QML
     *
     * // Then from QML, a ComboBox (for example) might be configured as:
     * ComboBox {
     *   model: myContainer
     *   textRole: "itemLabel"
     * }
     * \endcode
     */
    void            setItemDisplayRole(const QString& displayRoleProperty) noexcept { _displayRoleProperty = displayRoleProperty; }
protected:
    const QString&  getItemDisplayRole() const { return _displayRoleProperty; }
private:
    QString         _displayRoleProperty{QStringLiteral("label")};
protected slots:
    void            itemDisplayPropertyChanged() {
        QObject* qItem = sender();
        if (qItem == nullptr)
            return;
        int qItemIndex = indexOf(qItem);
        if (qItemIndex >= 0) {
            QModelIndex itemIndex{index(qItemIndex)};
            if (itemIndex.isValid())
                emit dataChanged(itemIndex, itemIndex);
        } else
            disconnect(qItem,  nullptr,
                       this,   nullptr);
    }
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
public:
    // Necessary to invoke QAbstractModel methods from concrete qcm::Container<>...
    friend class AbstractContainer;
protected:
    template <typename... Args>
    inline void    fwdBeginInsertRows(Args... args) noexcept { beginInsertRows(std::forward<Args>(args)...); }
    inline void    fwdEndInsertRows() noexcept { endInsertRows(); }
    inline void    fwdEmitLengthChanged() noexcept { emitLengthChanged(); }

    template <typename... Args>
    inline void    fwdBeginRemoveRows(Args... args) noexcept { beginRemoveRows(std::forward<Args>(args)...); }
    inline void    fwdEndRemoveRows() noexcept { endRemoveRows(); }

    inline void    fwdBeginResetModel() noexcept { beginResetModel(); }
    inline void    fwdEndResetModel() noexcept { endResetModel(); }
    //-------------------------------------------------------------------------

    /*! \name QML Container Interface *///-------------------------------------
    //@{
public:
    //! Similar to ES6 array.length.
    Q_PROPERTY(int length READ getLength NOTIFY lengthChanged FINAL)
    //! \copydoc length
    Q_INVOKABLE int getLength() const noexcept { return rowCount(QModelIndex{}); }
protected:
    //! Shortcut to emit lengthChanged() signal.
    inline void     emitLengthChanged() noexcept { emit lengthChanged(); }
signals:
    //! \sa length
    void            lengthChanged();

public:
    Q_INVOKABLE virtual bool        append(QObject *object) const { Q_UNUSED(object); return false; }
    Q_INVOKABLE virtual void        remove(QObject *object) const { Q_UNUSED(object); }
    Q_INVOKABLE virtual QObject*    at(int index) const { Q_UNUSED(index); return nullptr; }
    Q_INVOKABLE virtual bool        contains(QObject *object) const { return indexOf(object) >= 0; }
    Q_INVOKABLE virtual int         indexOf(QObject* item) const { Q_UNUSED(item); return 0; }
    Q_INVOKABLE virtual bool        clear() const { return false; }
    //@}
    //-------------------------------------------------------------------------
};

template <class Container, class T>
class ContainerModelImpl : public qcm::ContainerModel
{
public:
    explicit ContainerModelImpl(Container& container) :
        qcm::ContainerModel{},
        _container(container) { }
    ContainerModelImpl(const ContainerModelImpl<Container, T>&) = delete;
private:
    Container&    _container;

    /*! \name Qt Abstract Model Interface *///---------------------------------
    //@{
public:
    virtual int         rowCount(const QModelIndex& parent = QModelIndex{}) const override {
        return (parent.isValid() ? 0 :
                                   static_cast<int>(_container.size()));
    }
    virtual QVariant    data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (index.row() >= 0 &&
            index.row() < static_cast<int>(_container.size())) {
            if (role == Qt::DisplayRole)
                return dataDisplayRole(index.row(), typename ItemDispatcher<T>::type{});
            else if (role == ContainerModel::ItemDataRole)
                return dataItemRole(index.row(), typename ItemDispatcher<T>::type{});
        }
        return QVariant{};
    }
    //@}
    //-------------------------------------------------------------------------

    /*! \name Container/Model Mapping *///-------------------------------------
    //@{
public:
    using ObjectItemMap = std::unordered_map<const QObject*, T>;
    const ObjectItemMap&    getqObjectItemMap() const { return _qObjectItemMap; }
public:
    //! Maintain a mapping between smart<T> and T* when T is a QObject.
    mutable ObjectItemMap   _qObjectItemMap;

protected:
    inline auto dataDisplayRole(int row, ItemDispatcherBase::non_ptr_type)      const -> QVariant {
        return QVariant{_container.at(row)};
    }
    inline auto dataDisplayRole(int, ItemDispatcherBase::unsupported_type)      const -> QVariant {
        return QVariant{};
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::ptr_type)          const -> QVariant {
        T ptrItem = at(row);
        return (ptrItem != nullptr ? QVariant::fromValue<std::remove_pointer<T>::type>(*ptrItem) :
                                     QVariant{});
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::ptr_qobject_type)  const -> QVariant {
        T item = qobject_cast<T>(at(row));
        monitorItem(item);
        return (item != nullptr ? item->property(getItemDisplayRole( ).toLatin1()) :
                                  QVariant{});
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::q_ptr_type)        const -> QVariant {
        const auto item = at(row);
        monitorItem(item);
        return (item ? item->property(getItemDisplayRole().toLatin1()) :
                       QVariant{});
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::shared_ptr_type)   const -> QVariant {
        T item = at(row);
        return (item ? QVariant::fromValue<std::remove_pointer<typename T::element_type>::type>(*item.get()) :
                       QVariant{} );
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::shared_ptr_qobject_type)   const -> QVariant {
        QObject* qItem = at(row);
        monitorItem(qItem);
        return (qItem != nullptr ? qItem->property(getItemDisplayRole().toLatin1()) :
                                   QVariant{});
    }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::weak_ptr_type)             const -> QVariant { Q_UNUSED(row); return QVariant{}; }
    inline auto dataDisplayRole(int row, ItemDispatcherBase::weak_ptr_qobject_type)     const -> QVariant {
        T item = _container.at(row);
        monitorItem(item.lock().get());
        return (item.expired() ||
                item.lock() == nullptr ? QVariant{} :
                                         item.lock()->property( getItemDisplayRole().toLatin1()));
    }

private:
    //! Catch an item label property change notify signal and force model update for it's index.
    void    monitorItem(QObject* item) const {
        if (item == nullptr)
            return;
        QMetaProperty displayProperty = item->metaObject()->property(
                    item->metaObject()->indexOfProperty(getItemDisplayRole().toLatin1()));
        if (displayProperty.isValid() &&
            displayProperty.hasNotifySignal()) {
            QMetaMethod displayPropertyChanged = displayProperty.notifySignal();
            // Note 20161125: Direct connection (without method(indexOfSlot()) call is impossible, there is no existing QObject::connect
            // overload taking (QObject*, QMetaMethod, QObject*, pointer on method).
            QMetaMethod itemDisplayPropertyChangedSlot = metaObject()->method(metaObject()->indexOfSlot("itemDisplayPropertyChanged()"));
            auto c = connect(item,  displayPropertyChanged,
                             this,  itemDisplayPropertyChangedSlot);
        }
    }

private:
    inline auto dataItemRole(int row, ItemDispatcherBase::non_ptr_type)             const -> QVariant {
        T item = _container.at(row);
        return QVariant{item};
    }
    inline auto dataItemRole(int, ItemDispatcherBase::unsupported_type)             const -> QVariant { /* empty */ return QVariant{};  }

    inline auto dataItemRole(int, ItemDispatcherBase::ptr_type)                     const -> QVariant { /* empty */ return QVariant{};  }
    inline auto dataItemRole(int row, ItemDispatcherBase::ptr_qobject_type)         const -> QVariant {
        T item = qobject_cast<T>(at(row));
        if (item != nullptr)
            QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership );
        return QVariant::fromValue<T>(item);
    }
    inline auto dataItemRole(int row, ItemDispatcherBase::q_ptr_type)               const -> QVariant {
        const auto item = qobject_cast<QObject*>(at(row));
        if (item != nullptr)
            QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership );
        return item != nullptr ? QVariant::fromValue<QObject*>(item) :
                                 QVariant{};
    }

    inline auto dataItemRole(int, ItemDispatcherBase::shared_ptr_type)              const -> QVariant { /* empty */ return QVariant{};  }
    inline auto dataItemRole(int row, ItemDispatcherBase::shared_ptr_qobject_type)  const -> QVariant {
        T item = _container.at(row);
        auto qItem = qobject_cast<QObject*>(item.get());
        if (qItem != nullptr)
            QQmlEngine::setObjectOwnership(qItem, QQmlEngine::CppOwnership);
        return QVariant::fromValue<QObject*>(qItem);  // Might return a QVariant with an empty QObject*
    }

    inline auto dataItemRole(int, ItemDispatcherBase::weak_ptr_type)                const -> QVariant { /* empty */ return QVariant{}; }
    inline auto dataItemRole(int row, ItemDispatcherBase::weak_ptr_qobject_type)    const -> QVariant {
        T item = _container.at(row);
        auto qItem = (item.expired() ? nullptr : qobject_cast<QObject*>(item.lock().get()));
        if (qItem)
            QQmlEngine::setObjectOwnership(qItem, QQmlEngine::CppOwnership);
        return QVariant::fromValue<QObject*>(qItem);    // Might return a QVariant with an empty QObject*
    }
    //@}
    //-------------------------------------------------------------------------


public:
    virtual bool        append(QObject* object) const override {
        return appendImpl(object,
                          typename ItemDispatcher<typename Container::Item_type>::type{});
    }

    virtual void        remove(QObject* object) const override {
        if ( object != nullptr )
            removeImpl(object, typename ItemDispatcher<typename Container::Item_type>::type{});
    }

    virtual QObject*    at(int index) const override {
        auto item = atImpl(index, typename ItemDispatcher<typename Container::Item_type>::type{});
        QQmlEngine::setObjectOwnership(item, QQmlEngine::CppOwnership);
        return item;
    }

    virtual int         indexOf(QObject* item) const override {
        return indexOfImpl(item, typename ItemDispatcher<typename Container::Item_type>::type{});
    }

    virtual bool        clear() const override { _container.clear(); return true; }

private:
    inline auto appendImpl(QObject*, ItemDispatcherBase::non_ptr_type)              const -> bool { return false; }
    inline auto appendImpl(QObject*, ItemDispatcherBase::unsupported_type)          const -> bool { return false; }
    inline auto appendImpl(QObject*, ItemDispatcherBase::ptr_type)                  const -> bool { return false; }
    inline auto appendImpl(QObject* object, ItemDispatcherBase::ptr_qobject_type)   const -> bool {
        _container.append(reinterpret_cast<typename Container::Item_type>(object));
        return true;
    }
    inline auto appendImpl(QObject*, ItemDispatcherBase::q_ptr_type)                const -> bool {
        qWarning() << "Containers Model: Error: Using append() from QML is unsupported with an underlying container of QPointer.";
        return false;
    }
    inline auto appendImpl(QObject*, ItemDispatcherBase::shared_ptr_type)                   const ->bool { return false; }
    inline auto appendImpl(QObject* object, ItemDispatcherBase::shared_ptr_qobject_type)    const -> bool {
        (void)object;
        qWarning() << "Containers Model: Error: Using append() from QML is unsupported with an underlying container of std::shared_ptr.";
        return false;
    }
    inline auto appendImpl(QObject*, ItemDispatcherBase::weak_ptr_type )                    const -> bool { return false; }
    inline auto appendImpl(QObject*, ItemDispatcherBase::weak_ptr_qobject_type )            const -> bool {
        // Note 20161127: It is conceptually impossible to append a weak_ptr without adding also to master
        // container of shared_ptr (otherwise, it is expired before beeing added!)
        qWarning() << "Containers Model: Error: Using append() from QML is unsupported with an underlying container of std::weak_ptr.";
        return false;
    }

private:
    inline auto removeImpl(const QObject*, ItemDispatcherBase::non_ptr_type )                const -> void {}
    inline auto removeImpl(const QObject*, ItemDispatcherBase::unsupported_type )            const -> void {}
    inline auto removeImpl(const QObject*, ItemDispatcherBase::ptr_type )                    const -> void {}
    inline auto removeImpl(const QObject* object, ItemDispatcherBase::ptr_qobject_type)     const -> void {
        _container.removeAll(qobject_cast<typename Container::Item_type>(const_cast<QObject*>(object)));
    }
    inline auto removeImpl(const QObject* object, ItemDispatcherBase::q_ptr_type)       const -> void {
        const auto itemIter = _qObjectItemMap.find(object);
        if (itemIter != _qObjectItemMap.cend()) {
            const T t = itemIter->second;
            _container.removeAll(t);
            // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //_qObjectItemMap.erase(object);
        }
    }
    inline auto removeImpl(const QObject*, ItemDispatcherBase::shared_ptr_type )                const -> void {}
    inline auto removeImpl(const QObject* object, ItemDispatcherBase::shared_ptr_qobject_type)  const -> void {
        if (object == nullptr)
            return;
        const auto itemIter = _qObjectItemMap.find(object);
        if (itemIter != _qObjectItemMap.cend()) {
            const T t = itemIter->second;
            _container.removeAll(t);
        }
    }
    inline auto removeImpl(const QObject*, ItemDispatcherBase::weak_ptr_type)                   const -> void {}
    inline auto removeImpl(const QObject* object, ItemDispatcherBase::weak_ptr_qobject_type)    const -> void {
        if (object == nullptr)
            return;
        const auto itemIter = _qObjectItemMap.find(object);
        if (itemIter != _qObjectItemMap.cend()) {
            const T t = itemIter->second;
            _container.removeAll(t);
        }
    }

private:
    inline auto atImpl(int, ItemDispatcherBase::non_ptr_type)                   const -> QObject* { return nullptr; }
    inline auto atImpl(int, ItemDispatcherBase::unsupported_type)               const -> QObject* { return nullptr; }
    inline auto atImpl(int index, ItemDispatcherBase::ptr_type)                 const -> QObject* { return _container.at(index); }
    inline auto atImpl(int index, ItemDispatcherBase::ptr_qobject_type)         const -> QObject* { return _container.at(index); }
    inline auto atImpl(int index, ItemDispatcherBase::q_ptr_type)               const -> QObject* {
        auto itemPtr = _container.at(index);
        return (itemPtr ? itemPtr.data() : nullptr);
    }
    inline auto atImpl(int, ItemDispatcherBase::shared_ptr_type)                const -> QObject* { return nullptr; }
    inline auto atImpl(int index, ItemDispatcherBase::shared_ptr_qobject_type)  const -> QObject* {
        auto itemPtr = _container.at(index);
        return (itemPtr ? itemPtr.get() : nullptr);
    }
    inline auto atImpl(int, ItemDispatcherBase::weak_ptr_type)                  const -> QObject* { return nullptr; }
    inline auto atImpl(int index, ItemDispatcherBase::weak_ptr_qobject_type)    const -> QObject* {
        auto itemPtr = _container.at(index);
        return (!itemPtr.expired() ? itemPtr.lock().get() : nullptr);
    }

private:
    inline auto indexOfImpl(QObject*, ItemDispatcherBase::non_ptr_type)             const -> int { return -1; }
    inline auto indexOfImpl(QObject*, ItemDispatcherBase::unsupported_type)         const -> int { return -1;  }
    inline auto indexOfImpl(QObject* item, ItemDispatcherBase::ptr_type)            const -> int {
            return (item != nullptr ? _container.indexOf(qobject_cast<T>(item)) : -1);
    }
    inline auto indexOfImpl(QObject* item, ItemDispatcherBase::ptr_qobject_type)    const -> int {
        return (item != nullptr ? _container.indexOf(qobject_cast<T>(item)) : -1);
    }
    inline auto indexOfImpl(QObject* item, ItemDispatcherBase::q_ptr_type)          const -> int {
        if (item == nullptr)
            return -1;
        const auto& itemIter = _qObjectItemMap.find(item);
        if (itemIter != _qObjectItemMap.cend())
            return _container.indexOf(itemIter->second);
        return -1;
    }
    inline auto indexOfImpl(QObject*, ItemDispatcherBase::shared_ptr_type)              const -> int { return -1;  }
    inline auto indexOfImpl(QObject* item, ItemDispatcherBase::shared_ptr_qobject_type) const -> int {
        if (item == nullptr)
            return -1;
        const auto& itemIter = _qObjectItemMap.find(item);
        if (itemIter != _qObjectItemMap.cend())
            return _container.indexOf(itemIter->second);
        return -1;
    }
    inline auto indexOfImpl(QObject*, ItemDispatcherBase::weak_ptr_type)                const -> int { return -1; }
    inline auto indexOfImpl(QObject* item, ItemDispatcherBase::weak_ptr_qobject_type)   const -> int {
        if (item == nullptr)
            return -1;
        const auto& itemIter = _qObjectItemMap.find(item);
        if (itemIter != _qObjectItemMap.cend())
            return _container.indexOf(itemIter->second);
        return -1;
    }
};

} // ::qcm

QML_DECLARE_TYPE(qcm::ContainerModel)
QML_DECLARE_TYPE(const qcm::ContainerModel)
