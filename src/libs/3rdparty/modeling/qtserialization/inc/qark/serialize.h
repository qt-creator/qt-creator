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

#ifndef QARK_SERIALIZE_H
#define QARK_SERIALIZE_H

#include "tag.h"
#include "baseclass.h"
#include "attribute.h"
#include "reference.h"
#include "access.h"
#include "typeregistry.h"

#include "serialize_basic.h"
#include "serialize_container.h"
#include "serialize_enum.h"

#include <type_traits>


// qark is (Q)t(Ar)chiving(K)it
namespace qark {

template<class Archive, class T>
inline Archive &operator<<(Archive &archive, const T &t)
{
    save(archive, t);
    return archive;
}

template<class Archive, class T>
inline Archive &operator>>(Archive &archive, T &t)
{
    load(archive, t);
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, T &t)
{
    save(archive, (const T &) t);
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, T &t)
{
    load(archive, t);
    return archive;
}

template<class Archive, class T>
inline Archive &operator<<(Archive &archive, T *p)
{
    if (p) {
        if (archive.isReference(p)) {
            archive.beginPointer();
            archive.write(p);
            archive.endPointer();
        } else {
            if (typeid(*p) == typeid(T)) {
                archive.beginInstance();
                registry::save_pointer<Archive, T, T>(archive, p);
                archive.endInstance();
            } else {
                archive.beginInstance(get_type_uid(*p));
                //typename registry::TypeRegistry<Archive, typename qark::non_const<T>::type>::type_info type_data
                //        = get_type_info<Archive, typename qark::non_const<T>::type>(*p);
                typename registry::TypeRegistry<Archive, T>::type_info type_data = get_type_info<Archive, T>(*p);
                if (type_data.save_func == 0) {
                    throw unregistered_type();
                } else {
                    type_data.save_func(archive, p);
                }
                archive.endInstance();
            }
        }
    } else {
        archive.beginNullPointer();
        archive.endNullPointer();
    }
    return archive;
}

template<class Archive, class T>
inline Archive &operator>>(Archive &archive, T *&p)
{
    typename Archive::ReferenceTag ref_tag = archive.readReferenceTag();
    switch (ref_tag.kind) {
    case Archive::NULLPOINTER:
        p = 0;
        break;
    case Archive::POINTER:
        archive.read(p);
        break;
    case Archive::INSTANCE:
        if (ref_tag.type_name.isEmpty()) {
            registry::load_non_virtual_pointer<Archive,T>(archive, p);
        } else {
            typename registry::TypeRegistry<Archive, T>::type_info type_data = get_type_info<Archive, T>(ref_tag.type_name);
            if (type_data.load_func == 0) {
                throw unregistered_type();
            } else {
                type_data.load_func(archive, p);
            }
        }
        break;
    }
    archive.readReferenceEndTag(ref_tag.kind);
    return archive;
}

template<class Archive, class T>
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, T *&p)
{
    return archive << p;
}

template<class Archive, class T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, T *&p)
{
    return archive >> p;
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, T (*f)())
{
    return archive << f;
}

template<class Archive, class T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, T (*f)())
{
    return archive >> f;
}

template<class Archive>
inline Archive &operator<<(Archive &archive, void (*f)(Archive &))
{
    f(archive);
    return archive;
}

template<class Archive>
inline Archive &operator>>(Archive &archive, void (*f)(Archive &))
{
    f(archive);
    return archive;
}

template<class Archive>
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, void (*f)(Archive &))
{
    return archive << f;
}

template<class Archive>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, void (*f)(Archive &))
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const Tag &tag)
{
    return archive << tag;
}

template<class Archive>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const Tag &tag)
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const Object<T> &object)
{
    return archive << object;
}

template<class Archive, class T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const Object<T> &object)
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const End &end)
{
    return archive << end;
}

template<class Archive>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const End &end)
{
    return archive >> end;
}

template<class Archive, class T, class U>
Archive &operator<<(Archive &archive, const Base<T, U> &base)
{
    archive.beginBase(base);
    archive << base.getBase();
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const Base<T, U> &base)
{
    return archive << base;
}

template<class Archive, class T, class U>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const Base<T, U> &base)
{
    return archive >> base;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Attr<T> &attr)
{
    archive.beginAttribute(attr);
    archive << *attr.getValue();
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const Attr<T> &attr)
{
    return archive << attr;
}

template<class Archive, typename T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const Attr<T> &attr)
{
    return archive >> attr;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetterAttr<U, T> &attr)
{
    archive.beginAttribute(attr);
    archive << (attr.getObject().*(attr.getGetter()))();
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
    if (!((attr.getObject().*(attr.getGetter()))() == (U().*(attr.getGetter()))())) {
        archive.beginAttribute(attr);
        archive << (attr.getObject().*(attr.getGetter()))();
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
    archive << (attr.getObject().*(attr.getGetter()))();
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    return archive << attr;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const GetterSetterAttr<U, T, V> &attr)
{
    return archive >> attr;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetFuncAttr<U, T> &attr)
{
    archive.beginAttribute(attr);
    archive << ((*attr.getGetFunc())(attr.getObject()));
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
    archive << ((*attr.getGetFunc())(attr.getObject()));
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    return archive << attr;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const GetSetFuncAttr<U, T, V> &attr)
{
    return archive >> attr;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Ref<T *> &ref)
{
    archive.beginReference(ref);
    archive << *ref.getValue();
    archive.endReference(ref);
    return archive;
}

template<class Archive, typename T>
Archive &operator<<(Archive &archive, const Ref<T * const> &ref)
{
    archive.beginReference(ref);
    archive << *ref.getValue();
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const Ref<T *> &ref)
{
    archive.beginReference(ref);
    archive << *ref.getValue();
    archive.endReference(ref);
    return archive;
}

template<class Archive, typename T>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const Ref<T> &ref)
{
    return archive >> ref;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetterRef<U, T> &ref)
{
    archive.beginReference(ref);
    archive << (ref.getObject().*(ref.getGetter()))();
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
    archive << (ref.getObject().*(ref.getGetter()))();
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    return archive << ref;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const GetterSetterRef<U, T, V> &ref)
{
    return archive >> ref;
}

template<class Archive, class U, typename T>
Archive &operator<<(Archive &archive, const GetFuncRef<U, T> &ref)
{
    archive.beginReference(ref);
    archive << ref.getGetFunc()(ref.getObject());
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
    archive << ref.getGetFunc()(ref.getObject());
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
typename std::enable_if<Archive::out_archive, Archive &>::type operator||(Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    return archive << ref;
}

template<class Archive, class U, typename T, typename V>
typename std::enable_if<Archive::in_archive, Archive &>::type operator||(Archive &archive, const GetSetFuncRef<U, T, V> &ref)
{
    return archive >> ref;
}

}

#endif // QARK_SERIALIZE_H
