/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "parameters.h"

#include "qmt/infrastructure/qmtassert.h"

#include <exception>
#include <typeinfo>
#include <type_traits>

#include <QString>
#include <QHash>

namespace qark {

class UnregisteredType : public std::exception
{
};

class AbstractType : public std::exception
{
};

namespace registry {

// we use a template to allow definition of static variables in header
template<int N>
class TypeNameMaps
{
public:
    typedef QHash<QString, QString> MapType;

    static MapType &nameToUidMap() { return *typeidNameToUidMap; }
    static MapType &uidToNameMap() { return *typeidUidToNameMap; }

#if !defined(QT_NO_DEBUG)
    static bool hasNameToUidMap() { return typeidNameToUidMap != nullptr; }
    static bool hasUidToNameMap() { return typeidUidToNameMap != nullptr; }
#endif

protected:
    static void init()
    {
        static bool initialized = false;
        static MapType nameToUidMap;
        static MapType uidToNameMap;

        if (!initialized) {
            typeidNameToUidMap = &nameToUidMap;
            typeidUidToNameMap = &uidToNameMap;
            initialized = true;
        }
    }

private:
    static MapType *typeidNameToUidMap;
    static MapType *typeidUidToNameMap;
};

template<int N>
typename TypeNameMaps<N>::MapType *TypeNameMaps<N>::typeidNameToUidMap;

template<int N>
typename TypeNameMaps<N>::MapType *TypeNameMaps<N>::typeidUidToNameMap;

template<class T>
class TypeNameRegistry : public TypeNameMaps<0>
{
    typedef TypeNameMaps<0> Base;

    static int staticInit;

    static int init(const QString &name)
    {
        Base::init();
        QMT_CHECK(!Base::nameToUidMap().contains(QLatin1String(typeid(T).name()))
                  || Base::nameToUidMap().value(QLatin1String(typeid(T).name())) == name);
        QMT_CHECK(!Base::uidToNameMap().contains(name)
                  || Base::uidToNameMap().value(name) == QLatin1String(typeid(T).name()));
        Base::nameToUidMap().insert(QLatin1String(typeid(T).name()), name);
        Base::uidToNameMap().insert(name, QLatin1String(typeid(T).name()));
        return 0;
    }
};

template<class Archive, class BASE>
class TypeRegistry
{
public:
    class TypeInfo
    {
    public:
        typedef Archive &(*SaveFuncType)(Archive &, BASE * const &p);
        typedef Archive &(*LoadFuncType)(Archive &, BASE * &p);

        explicit TypeInfo()
        {
        }

        explicit TypeInfo(SaveFuncType sfunc, LoadFuncType lfunc)
            : m_saveFunc(sfunc),
              m_loadFunc(lfunc)
        {
        }

        bool operator==(const TypeInfo &rhs) const
        {
            return m_saveFunc == rhs.m_saveFunc && m_loadFunc == rhs.m_loadFunc;
        }

        SaveFuncType m_saveFunc = nullptr;
        LoadFuncType m_loadFunc = nullptr;
    };

    typedef QHash<QString, TypeInfo> MapType;

    static MapType &map() { return *m_map; }

#if !defined(QT_NO_DEBUG)
    static bool hasMap() { return m_map != nullptr; }
#endif

protected:
    static void init() {
        static bool initialized = false;
        static MapType theMap;

        if (!initialized) {
            m_map = &theMap;
            initialized = true;
        }
    }

private:
    static MapType *m_map;
};

template<class Archive, class BASE>
typename TypeRegistry<Archive, BASE>::MapType *TypeRegistry<Archive,BASE>::m_map;

template<class Archive, class BASE, class DERIVED>
class DerivedTypeRegistry : public TypeRegistry<Archive, BASE>
{
    typedef TypeRegistry<Archive, BASE> Base;
    typedef Archive &(*SaveFuncType)(Archive &, BASE * const &);
    typedef Archive &(*LoadFuncType)(Archive &, BASE * &);

    static int staticInit;

    static int init(SaveFuncType sfunc, LoadFuncType lfunc)
    {
        Base::init();
        QMT_CHECK(!Base::map().contains(QLatin1String(typeid(DERIVED).name()))
                  || Base::map().value(QLatin1String(typeid(DERIVED).name())) == typename Base::TypeInfo(sfunc, lfunc));
        Base::map().insert(QLatin1String(typeid(DERIVED).name()), typename Base::TypeInfo(sfunc, lfunc));
        return 0;
    }
};

template<class Archive, class BASE, class DERIVED>
Archive &savePointer(Archive &ar, BASE * const &p)
{
    DERIVED &t = dynamic_cast<DERIVED &>(*p);
    save(ar, t, Parameters());
    return ar;
}

template<class Archive, class BASE, class DERIVED>
Archive &loadPointer(Archive &ar, BASE *&p)
{
    auto t = new DERIVED();
    load(ar, *t, Parameters());
    p = t;
    return ar;
}

template<class Archive, class T>
typename std::enable_if<!std::is_abstract<T>::value, void>::type loadNonVirtualPointer(
        Archive &archive, T *&p)
{
    registry::loadPointer<Archive, T, T>(archive, p);
}

template<class Archive, class T>
typename std::enable_if<std::is_abstract<T>::value, void>::type loadNonVirtualPointer(
        Archive &archive, T *&p)
{
    (void) archive;
    (void) p;

    throw AbstractType();
}

inline QString demangleTypename(const char *mangledName)
{
    // TODO convert compiler specific mangledName into human readable type name
    return QLatin1String(mangledName);
}

inline QString flattenTypename(const char *typeName)
{
    // convert C++ type name into simple identifier (no extra characters)
    return QString(QLatin1String(typeName)).replace(QChar(QLatin1Char(':')), QLatin1String("-"));
}

} // namespace registry

template<class T>
QString typeUid()
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasNameToUidMap()),
                "typeUid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::nameToUidMap().contains(QLatin1String(typeid(T).name()))),
                "typeUid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangleTypename(typeid(T).name()))));
#endif
    return registry::TypeNameRegistry<T>::nameToUidMap().value(QLatin1String(typeid(T).name()));
}

template<class T>
QString typeUid(const T &t)
{
    Q_UNUSED(t); // avoid warning in some compilers
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasNameToUidMap()),
                "typeUid<T>()", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeNameRegistry<T>::nameToUidMap().contains(QLatin1String(typeid(t).name()))),
                "typeUid<T>()",
                qPrintable(QString(QLatin1String("type with typeid %1 is not registered. Use QARK_REGISTER_TYPE or QARK_REGISTER_TYPE_NAME.")).arg(registry::demangleTypename(typeid(t).name()))));
#endif
    return registry::TypeNameRegistry<T>::nameToUidMap().value(QLatin1String(typeid(t).name()));
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive, T>::TypeInfo typeInfo(const T &t)
{
    Q_UNUSED(t); // avoid warning in some compilers
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::hasMap()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::typeInfo(const T&)")).arg(typeUid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(typeUid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::map()[QLatin1String(typeid(t).name())];
}

template<class Archive, class T>
typename registry::TypeRegistry<Archive,T>::TypeInfo typeInfo(const QString &uid)
{
#if !defined(QT_NO_DEBUG) // avoid warning about unused function ::hasNameToUidMap in Qt >= 5.5
    QMT_CHECK_X((registry::TypeNameRegistry<T>::hasUidToNameMap()),
                "typeInfo<T>(const QString &)", "type maps are not correctly initialized");
    QMT_CHECK_X((registry::TypeRegistry<Archive,T>::hasMap()),
                qPrintable(QString(QLatin1String("TypeRegistry<Archive, %1>::typeInfo(const QString &)")).arg(typeUid<T>())),
                qPrintable(QString(QLatin1String("%1 is not a registered base class. Declare your derived classes with QARK_REGISTER_DERIVED_CLASS.")).arg(typeUid<T>())));
#endif
    return registry::TypeRegistry<Archive,T>::map().value(registry::TypeNameRegistry<T>::uidToNameMap().value(uid));
}

} // namespace qark

#define QARK_TYPE_STRING(T) #T

#define QARK_REGISTER_TYPE_NAME(T, NAME) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::staticInit = qark::registry::TypeNameRegistry<T>::init(QLatin1String(NAME));

#define QARK_REGISTER_TYPE(T) \
    template<> \
    int qark::registry::TypeNameRegistry<T>::staticInit = qark::registry::TypeNameRegistry<T>::init(qark::registry::flattenTypename(QARK_TYPE_STRING(T)));

#define QARK_REGISTER_DERIVED_CLASS(INARCHIVE, OUTARCHIVE, DERIVED, BASE) \
    template<> \
    int qark::registry::DerivedTypeRegistry<INARCHIVE,BASE,DERIVED>::staticInit = \
            qark::registry::DerivedTypeRegistry<INARCHIVE, BASE, DERIVED>::init(0, qark::registry::loadPointer<INARCHIVE, BASE, DERIVED>); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::staticInit = \
            qark::registry::DerivedTypeRegistry<OUTARCHIVE, BASE, DERIVED>::init(qark::registry::savePointer<OUTARCHIVE, BASE, DERIVED>, 0); \
    template<> \
    int qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>::staticInit = \
            qark::registry::DerivedTypeRegistry<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>:: \
                init(qark::registry::savePointer<OUTARCHIVE, typename std::add_const<BASE>::type, typename std::add_const<DERIVED>::type>, 0);
