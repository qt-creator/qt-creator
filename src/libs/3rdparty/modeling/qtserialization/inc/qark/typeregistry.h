/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QARK_TYPEREGISTRY_H
#define QARK_TYPEREGISTRY_H

#include "qmt/infrastructure/qmtassert.h"

#include <exception>
#include <typeinfo>
#include <type_traits>

#include <QString>
#include <QHash>


namespace qark {

class unregistered_type :
        public std::exception
{
};

class abstract_type :
        public std::exception
{
};


namespace registry {

// we use a template to allow definition of static variables in header
template<int N>
class TypeNameMaps {
public:

    typedef QHash<QString, QString> map_type;

public:

    static map_type &get_name_to_uid_map() { return *typeid_name_to_uid_map; }

    static map_type &get_uid_to_name_map() { return *typeid_uid_to_name_map; }

#if !defined(QT_NO_DEBUG)
    static bool has_name_to_uid_map() { return typeid_name_to_uid_map != 0; }

    static bool has_uid_to_name_map() { return typeid_uid_to_name_map != 0; }
#endif

protected:

    static void init()
    {
        static bool initialized = false;
        static map_type name_to_uid_map;
        static map_type uid_to_name_map;

        if (!initialized) {
            typeid_name_to_uid_map = &name_to_uid_map;
            typeid_uid_to_name_map = &uid_to_name_map;
            initialized = true;
        }
    }

private:

    static map_type *typeid_name_to_uid_map;

    static map_type *typeid_uid_to_name_map;
};

template<int N>
typename TypeNameMaps<N>::map_type *TypeNameMaps<N>::typeid_name_to_uid_map;

template<int N>
typename TypeNameMaps<N>::map_type *TypeNameMaps<N>::typeid_uid_to_name_map;


template<class T>
class TypeNameRegistry :
        public TypeNameMaps<0>
{

    typedef TypeNameMaps<0> base;

private:

    static int __static_init;

private:
    static int __init(const QString &name)
    {
        base::init();
        QMT_CHECK(!base::get_name_to_uid_map().contains(QLatin1String(typeid(T).name())) || base::get_name_to_uid_map().value(QLatin1String(typeid(T).name())) == name);
        QMT_CHECK(!base::get_uid_to_name_map().contains(name) || base::get_uid_to_name_map().value(name) == QLatin1String(typeid(T).name()));
        base::get_name_to_uid_map().insert(QLatin1String(typeid(T).name()), name);
        base::get_uid_to_name_map().insert(name, QLatin1String(typeid(T).name()));
        return 0;
    }
};



template<class Archive, class BASE>
class TypeRegistry {
public:

    struct type_info {

        typedef Archive &(*save_func_type)(Archive &, BASE * const &p);
        typedef Archive &(*load_func_type)(Archive &, BASE * &p);

        explicit type_info()
            : save_func(0),
              load_func(0)
        {
        }

        explicit type_info(save_func_type sfunc, load_func_type lfunc)
            : save_func(sfunc),
              load_func(lfunc)
        {
        }

        bool operator==(const type_info &rhs) const
        {
            return save_func == rhs.save_func && load_func == rhs.load_func;
        }

        save_func_type save_func;
        load_func_type load_func;
    };

    typedef QHash<QString, type_info> map_type;

public:

    static map_type &get_map() { return *map; }

#if !defined(QT_NO_DEBUG)
    static bool has_map() { return map != 0; }
#endif

protected:

    static void init() {
        static bool initialized = false;
        static map_type the_map;

        if (!initialized) {
            map = &the_map;
            initialized = true;
        }
    }

private:

    static map_type *map;
};

template<class Archive, class BASE>
typename TypeRegistry<Archive, BASE>::map_type *TypeRegistry<Archive,BASE>::map;


template<class Archive, class BASE, class DERIVED>
class DerivedTypeRegistry :
        public TypeRegistry<Archive, BASE>
{

    typedef TypeRegistry<Archive, BASE> base;

    typedef Archive &(*save_func_type)(Archive &, BASE * const &);

    typedef Archive &(*load_func_type)(Archive &, BASE * &);

private:

    static int __static_init;

private:
    static int __init(save_func_type sfunc, load_func_type lfunc)
    {
        base::init();
        QMT_CHECK(!base::get_map().contains(QLatin1String(typeid(DERIVED).name())) || base::get_map().value(QLatin1String(typeid(DERIVED).name())) == typename base::type_info(sfunc, lfunc));
        base::get_map().insert(QLatin1String(typeid(DERIVED).name()), typename base::type_info(sfunc, lfunc));
        return 0;
    }
};

template<class Archive, class BASE, class DERIVED>
Archive &save_pointer(Archive &ar, BASE * const &p)
{
    DERIVED &t = dynamic_cast<DERIVED &>(*p);
    ar << t;
    return ar;
}

template<class Archive, class BASE, class DERIVED>
Archive &load_pointer(Archive &ar, BASE *&p)
{
    DERIVED *t = new DERIVED();
    ar >> *t;
    p = t;
    return ar;
}

template<class Archive, class T>
typename std::enable_if<!std::is_abstract<T>::value, void>::type load_non_virtual_pointer(Archive &archive, T *&p)
{
    registry::load_pointer<Archive, T, T>(archive, p);
}

template<class Archive, class T>
typename std::enable_if<std::is_abstract<T>::value, void>::type load_non_virtual_pointer(Archive &archive, T *&p)
{
    (void) archive;
    (void) p;

    throw abstract_type();
}

inline QString demangle_typename(const char *mangled_name)
{
    // TODO convert compiler specific mangled_name into human readable type name
    return QLatin1String(mangled_name);
}

inline QString flatten_typename(const char *type_name)
{
    // convert C++ type name into simple identifier (no extra characters)
    return QString(QLatin1String(type_name)).replace(QChar(QLatin1Char(':')), QLatin1String("-"));
}

}

template<class T>
QString get_type_uid()
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::has_name_to_uid_map in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::has_name_to_uid_map()), "get_type_uid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::get_name_to_uid_map().contains(QLatin1String(typeid(T).name()))), "get_type_uid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangle_typename(typeid(T).name()))));
#endif
    return registry::TypeNameRegistry<T>::get_name_to_uid_map().value(QLatin1String(typeid(T).name()));
}

template<class T>
QString get_type_uid(const T &t)
{
    Q_UNUSED(t);
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::has_name_to_uid_map in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::has_name_to_uid_map()), "get_type_uid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::get_name_to_uid_map().contains(QLatin1String(typeid(t).name()))), "get_type_uid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangle_typename(typeid(t).name()))));
#endif
    return registry::TypeNameRegistry<T>::get_name_to_uid_map().value(QLatin1String(typeid(t).name()));
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive, T>::type_info get_type_info(const T &t)
{
    Q_UNUSED(t);
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::has_name_to_uid_map in Qt >= 5.5
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::has_map()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::get_type_info(const T&)")).arg(get_type_uid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(get_type_uid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::get_map()[QLatin1String(typeid(t).name())];
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive,T>::type_info get_type_info(const QString &uid)
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::has_name_to_uid_map in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::has_uid_to_name_map()), "get_type_info<T>(const QString &)", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::has_map()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::get_type_info(const QString &)")).arg(get_type_uid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(get_type_uid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::get_map().value(registry::TypeNameRegistry<T>::get_uid_to_name_map().value(uid));
}

}


#define QARK_TYPE_STRING(T) #T

#define QARK_REGISTER_TYPE_NAME(T, NAME) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::__static_init = qark::registry::TypeNameRegistry<T>::__init(QLatin1String(NAME));

#define QARK_REGISTER_TYPE(T) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::__static_init = qark::registry::TypeNameRegistry<T>::__init(qark::registry::flatten_typename(QARK_TYPE_STRING(T)));

#define QARK_REGISTER_DERIVED_CLASS(INARCHIVE, OUTARCHIVE, DERIVED, BASE) \
    template<> \
    int qark::registry::DerivedTypeRegistry<INARCHIVE,BASE,DERIVED>::__static_init = \
    qark::registry::DerivedTypeRegistry<INARCHIVE, BASE, DERIVED>::__init(0, qark::registry::load_pointer<INARCHIVE, BASE, DERIVED>); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::__static_init = \
    qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::__init(qark::registry::save_pointer<OUTARCHIVE, BASE, DERIVED>, 0); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>::__static_init = \
    qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>:: \
    __init(qark::registry::save_pointer<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>, 0);

#endif // QARK_TYPEREGISTRY_H
