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

#ifndef QARK_SERIALIZE_POINTER_H
#define QARK_SERIALIZE_POINTER_H

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
                if (typeData.m_saveFunc == 0)
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
        p = 0;
        break;
    case Archive::Pointer:
        archive.read(p);
        break;
    case Archive::Instance:
        if (refTag.typeName.isEmpty()) {
            registry::loadNonVirtualPointer<Archive,T>(archive, p);
        } else {
            typename registry::TypeRegistry<Archive, T>::TypeInfo typeData = typeInfo<Archive, T>(refTag.typeName);
            if (typeData.m_loadFunc == 0)
                throw UnregisteredType();
            else
                typeData.m_loadFunc(archive, p);
        }
        break;
    }
    archive.readReferenceEndTag(refTag.kind);
}

} // namespace qark

#endif // QARK_SERIALIZE_POINTER_H

