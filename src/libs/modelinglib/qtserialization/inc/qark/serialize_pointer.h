// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "typeregistry.h"

namespace qark {

template<class Archive, class T>
inline void save(Archive &archive, T *p, const Parameters &)
{
    if (p) {
        if (archive.isReference(p)) {
            archive.beginPointer();
            archive.write(p);
            archive.endPointer();
        } else {
            if (typeid(*p) == typeid(T)) {
                archive.beginInstance();
                registry::savePointer<Archive, T, T>(archive, p);
                archive.endInstance();
            } else {
                archive.beginInstance(typeUid(*p));
                typename registry::TypeRegistry<Archive, T>::TypeInfo typeData = typeInfo<Archive, T>(*p);
                if (!typeData.m_saveFunc)
                    throw UnregisteredType();
                else
                    typeData.m_saveFunc(archive, p);
                archive.endInstance();
            }
        }
    } else {
        archive.beginNullPointer();
        archive.endNullPointer();
    }
}

template<class Archive, class T>
void load(Archive &archive, T *&p, const Parameters &)
{
    typename Archive::ReferenceTag refTag = archive.readReferenceTag();
    switch (refTag.kind) {
    case Archive::Nullpointer:
        p = nullptr;
        break;
    case Archive::Pointer:
        archive.read(p);
        break;
    case Archive::Instance:
        if (refTag.typeName.isEmpty()) {
            registry::loadNonVirtualPointer<Archive,T>(archive, p);
        } else {
            typename registry::TypeRegistry<Archive, T>::TypeInfo typeData = typeInfo<Archive, T>(refTag.typeName);
            if (!typeData.m_loadFunc)
                throw UnregisteredType();
            else
                typeData.m_loadFunc(archive, p);
        }
        break;
    }
    archive.readReferenceEndTag(refTag.kind);
}

} // namespace qark
