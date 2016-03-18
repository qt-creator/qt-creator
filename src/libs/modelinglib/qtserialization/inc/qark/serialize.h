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

#include "tag.h"
#include "baseclass.h"
#include "attribute.h"
#include "reference.h"
#include "access.h"
#include "typeregistry.h"

#include "serialize_pointer.h"
#include "serialize_basic.h"
#include "serialize_container.h"
#include "serialize_enum.h"

#include <type_traits>

// qark is (Q)t(Ar)chiving(K)it
namespace qark {

template<class Archive, class T>
inline Archive &operator<<(Archive &archive, const T &t)
{
    save(archive, t, Parameters());
    return archive;
}

template<class Archive, class T>
inline Archive &operator>>(Archive &archive, T &t)
{
    load(archive, t, Parameters());
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(Archive &archive, T &t)
{
    return archive << t;
}

template<class Archive, class T>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(Archive &archive, T &t)
{
    return archive >> t;
}

template<class Archive, class T>
inline Archive &operator<<(Archive &archive, T (*f)())
{
    archive << f();
    return archive;
}

template<class Archive, class T>
inline Archive &operator>>(Archive &archive, T (*f)())
{
    archive >> f();
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(Archive &archive, T (*f)())
{
    return archive << f;
}

template<class Archive, class T>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(Archive &archive, T (*f)())
{
    return archive >> f;
}

template<class Archive>
inline Archive &operator<<(Archive &archive, const Tag &tag)
{
    archive.beginElement(tag);
    return archive;
}

template<class Archive>
inline Archive &operator>>(Archive &archive, const Tag &tag)
{
    archive.append(tag);
    return archive;
}

template<class Archive>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const Tag &tag)
{
    return archive << tag;
}

template<class Archive>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const Tag &tag)
{
    return archive >> tag;
}

template<class Archive, class T>
inline Archive &operator<<(Archive &archive, const Object<T> &object)
{
    archive.beginElement(object);
    return archive;
}

template<class Archive, class T>
inline Archive &operator>>(Archive &archive, const Object<T> &object)
{
    archive.append(object);
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const Object<T> &object)
{
    return archive << object;
}

template<class Archive, class T>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const Object<T> &object)
{
    return archive >> object;
}

template<class Archive>
inline Archive &operator<<(Archive &archive, const End &end)
{
    archive.endElement(end);
    return archive;
}

template<class Archive>
inline Archive &operator>>(Archive &archive, const End &end)
{
    archive.append(end);
    return archive;
}

template<class Archive>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const End &end)
{
    return archive << end;
}

template<class Archive>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const End &end)
{
    return archive >> end;
}

template<class Archive, class T, class U>
Archive &operator<<(Archive &archive, const Base<T, U> &base)
{
    archive.beginBase(base);
    archive << base.base();
    archive.endBase(base);
    return archive;
}

template<class Archive, class T, class U>
Archive &operator>>(Archive &archive, const Base<T, U> &base)
{
    archive.append(base);
    return archive;
}

template<class Archive, class T, class U>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const Base<T, U> &base)
{
    return archive << base;
}

template<class Archive, class T, class U>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const Base<T, U> &base)
{
    return archive >> base;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Attr<T> &attr)
{
    archive.beginAttribute(attr);
    save(archive, *attr.value(), attr.parameters());
    archive.endAttribute(attr);
    return archive;
}

template<class Archive, typename T>
Archive &operator>>(Archive &archive, const Attr<T> &attr)
{
    archive.append(attr);
    return archive;
}

template<class Archive, typename T>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const Attr<T> &attr)
{
    return archive << attr;
}

template<class Archive, typename T>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const Attr<T> &attr)
{
    return archive >> attr;
}

template<class Archive, class U, typename T>
typename std::enable_if<!std::is_abstract<U>::value, Archive &>::type
operator<<(Archive &archive, const GetterAttr<U, T> &attr)
{
    if (!((attr.object().*(attr.getter()))() == (U().*(attr.getter()))())) {
        archive.beginAttribute(attr);
        save(archive, (attr.object().*(attr.getter()))(), attr.parameters());
        archive.endAttribute(attr);
    }
    return archive;
}

template<class Archive, class U, typename T>
typename std::enable_if<std::is_abstract<U>::value, Archive &>::type
operator<<(Archive &archive, const GetterAttr<U, T> &attr)
{
    archive.beginAttribute(attr);
    save(archive, (attr.object().*(attr.getter()))(), attr.parameters());
    archive.endAttribute(attr);
    return archive;
}

template<class Archive, class U, typename T>
Archive &operator>>(Archive &archive, const SetterAttr<U, T> &attr)
{
    archive.append(attr);
    return archive;
}

// TODO find possibility to avoid error message if T does not have an operator==
template<class Archive, class U, typename T, typename V>
typename std::enable_if<!std::is_abstract<U>::value, Archive &>::type
operator<<(Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    if (!((attr.object().*(attr.getter()))() == (U().*(attr.getter()))())) {
        archive.beginAttribute(attr);
        save(archive, (attr.object().*(attr.getter()))(), attr.parameters());
        archive.endAttribute(attr);
    }
    return archive;
}

// TODO Check for default values, e.g. using T()
template<class Archive, class U, typename T, typename V>
typename std::enable_if<std::is_abstract<U>::value, Archive &>::type
operator<<(Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    archive.beginAttribute(attr);
    save(archive, (attr.object().*(attr.getter()))(), attr.parameters());
    archive.endAttribute(attr);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator>>(Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    archive.append(attr);
    return archive;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    return archive << attr;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    return archive >> attr;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetFuncAttr<U, T> &attr)
{
    archive.beginAttribute(attr);
    save(archive, ((*attr.getterFunc())(attr.object())), attr.parameters());
    archive.endAttribute(attr);
    return archive;
}

template<class Archive, class U, typename T>
Archive &operator>>(Archive &archive, const SetFuncAttr<U, T> &attr)
{
    archive.append(attr);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator<<(Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    archive.beginAttribute(attr);
    save(archive, ((*attr.getterFunc())(attr.object())), attr.parameters());
    archive.endAttribute(attr);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator>>(Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    archive.append(attr);
    return archive;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    return archive << attr;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    return archive >> attr;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Ref<T *> &ref)
{
    archive.beginReference(ref);
    save(archive, *ref.value(), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Ref<T * const> &ref)
{
    archive.beginReference(ref);
    save(archive, *ref.value(), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, typename T>
Archive &operator>>(Archive &archive, const Ref<T> &ref)
{
    archive.append(ref);
    return archive;
}

template<class Archive, typename T>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const Ref<T *> &ref)
{
    archive.beginReference(ref);
    save(archive, *ref.value(), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, typename T>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const Ref<T> &ref)
{
    return archive >> ref;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetterRef<U, T> &ref)
{
    archive.beginReference(ref);
    save(archive, (ref.object().*(ref.getter()))(), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, class U, typename T>
Archive &operator>>(Archive &archive, const SetterRef<U, T> &ref)
{
    archive.append(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator<<(Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    archive.beginReference(ref);
    save(archive, (ref.object().*(ref.getter()))(), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator>>(Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    archive.append(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    return archive << ref;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    return archive >> ref;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetFuncRef<U, T> &ref)
{
    archive.beginReference(ref);
    save(archive, ref.getterFunc()(ref.object()), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, class U, typename T>
Archive &operator>>(Archive &archive, const SetFuncRef<U, T> &ref)
{
    archive.append(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator<<(Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    archive.beginReference(ref);
    save(archive, ref.getterFunc()(ref.object()), ref.parameters());
    archive.endReference(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
Archive &operator>>(Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    archive.append(ref);
    return archive;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::outArchive, Archive &>::type operator||(
        Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    return archive << ref;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::inArchive, Archive &>::type operator||(
        Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    return archive >> ref;
}

} // namespace qark
