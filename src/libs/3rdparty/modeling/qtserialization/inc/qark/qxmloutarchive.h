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

#ifndef QARK_QXMLOUTARCHIVE_H
#define QARK_QXMLOUTARCHIVE_H

#include "archivebasics.h"
#include "tag.h"
#include "baseclass.h"
#include "attribute.h"
#include "reference.h"
#include "impl/savingrefmap.h"

#include <QXmlStreamWriter>
#include <exception>


namespace qark {

class QXmlOutArchive :
        public ArchiveBasics
{
public:

    class UnsupportedForwardReference :
            public std::exception
    {
    };

    class DanglingReferences :
            public std::exception
    {
    };

    static const bool in_archive = false;
    static const bool out_archive = true;

public:

    QXmlOutArchive(QXmlStreamWriter &stream)
        : _stream(stream),
          _next_pointer_is_reference(false)
    {
    }

    ~QXmlOutArchive()
    {
        if (_saving_ref_map.countDanglingReferences() > 0) {
            throw DanglingReferences();
        }
    }

public:

    template<typename T>
    void write(T *p)
    {
        if (!_saving_ref_map.hasDefinedRef(p)) {
            throw UnsupportedForwardReference();
        }
        write(_saving_ref_map.getRef(p).get());
    }

#define QARK_WRITENUMBER(T) \
    void write(T i) \
    { \
        _stream.writeCharacters(QString::number(i)); \
    }

    QARK_WRITENUMBER(char)
    QARK_WRITENUMBER(signed char)
    QARK_WRITENUMBER(unsigned char)
    QARK_WRITENUMBER(short int)
    QARK_WRITENUMBER(unsigned short int)
    QARK_WRITENUMBER(int)
    QARK_WRITENUMBER(unsigned int)
    QARK_WRITENUMBER(long int)
    QARK_WRITENUMBER(unsigned long int)
    QARK_WRITENUMBER(long long int)
    QARK_WRITENUMBER(unsigned long long int)
    QARK_WRITENUMBER(float)
    QARK_WRITENUMBER(double)

#undef QARK_WRITENUMBER

    void write(bool b)
    {
        _stream.writeCharacters(QLatin1String(b ? "true" : "false"));
    }

    void write(const QString &s)
    {
        _stream.writeCharacters(s);
    }

public:

    void beginDocument()
    {
        _stream.writeStartDocument();
    }

    void endDocument()
    {
        _stream.writeEndDocument();
    }

    void beginElement(const Tag &tag)
    {
        _stream.writeStartElement(tag.getQualifiedName());
    }

    template<class T>
    void beginElement(const Object<T> &object)
    {
        _stream.writeStartElement(object.getQualifiedName());
        _stream.writeAttribute(QLatin1String("id"), QString::number(_saving_ref_map.getRef(object.getObject(), true).get()));
    }

    void endElement(const End &)
    {
        _stream.writeEndElement();
    }

    template<class T, class U>
    void beginBase(const Base<T, U> &base)
    {
        _stream.writeStartElement(base.getQualifiedName());
    }

    template<class T, class U>
    void endBase(const Base<T, U> &)
    {
        _stream.writeEndElement();
    }

    template<class T>
    void beginAttribute(const Attr<T> &attr)
    {
        _stream.writeStartElement(attr.getQualifiedName());
    }

    template<class T>
    void endAttribute(const Attr<T> &)
    {
        _stream.writeEndElement();
    }

    template<class U, typename T>
    void beginAttribute(const GetterAttr<U, T> &attr)
    {
        _stream.writeStartElement(attr.getQualifiedName());
    }

    template<class U, typename T>
    void endAttribute(const GetterAttr<U, T> &)
    {
        _stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginAttribute(const GetterSetterAttr<U, T, V> &attr)
    {
        _stream.writeStartElement(attr.getQualifiedName());
    }

    template<class U, typename T, typename V>
    void endAttribute(const GetterSetterAttr<U, T, V> &)
    {
        _stream.writeEndElement();
    }

    template<class U, typename T>
    void beginAttribute(const GetFuncAttr<U, T> &attr)
    {
        _stream.writeStartElement(attr.getQualifiedName());
    }

    template<class U, typename T>
    void endAttribute(const GetFuncAttr<U, T> &)
    {
        _stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginAttribute(const GetSetFuncAttr<U, T, V> &attr)
    {
        _stream.writeStartElement(attr.getQualifiedName());
    }

    template<class U, typename T, typename V>
    void endAttribute(const GetSetFuncAttr<U, T, V> &)
    {
        _stream.writeEndElement();
    }

    template<class T>
    void beginReference(const Ref<T> &ref)
    {
        _stream.writeStartElement(ref.getQualifiedName());
        _next_pointer_is_reference = true;
    }

    template<class T>
    void endReference(const Ref<T> &)
    {
        _next_pointer_is_reference = false;
        _stream.writeEndElement();
    }

    template<class U, typename T>
    void beginReference(const GetterRef<U, T> &ref)
    {
        _stream.writeStartElement(ref.getQualifiedName());
        _next_pointer_is_reference = true;
    }

    template<class U, typename T>
    void endReference(const GetterRef<U, T> &)
    {
        _next_pointer_is_reference = false;
        _stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginReference(const GetterSetterRef<U, T, V> &ref)
    {
        _stream.writeStartElement(ref.getQualifiedName());
        _next_pointer_is_reference = true;
    }

    template<class U, typename T, typename V>
    void endReference(const GetterSetterRef<U, T, V> &)
    {
        _next_pointer_is_reference = false;
        _stream.writeEndElement();
    }

    template<class U, typename T>
    void beginReference(const GetFuncRef<U, T> &ref)
    {
        _stream.writeStartElement(ref.getQualifiedName());
        _next_pointer_is_reference = true;
    }

    template<class U, typename T>
    void endReference(const GetFuncRef<U, T> &)
    {
        _next_pointer_is_reference = false;
        _stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginReference(const GetSetFuncRef<U, T, V> &ref)
    {
        _stream.writeStartElement(ref.getQualifiedName());
        _next_pointer_is_reference = true;
    }

    template<class U, typename T, typename V>
    void endReference(const GetSetFuncRef<U, T, V> &)
    {
        _next_pointer_is_reference = false;
        _stream.writeEndElement();
    }

    template<typename T>
    bool isReference(T *p)
    {
        return _next_pointer_is_reference || _saving_ref_map.hasDefinedRef(p);
    }

    void beginNullPointer()
    {
        _stream.writeStartElement(QLatin1String("null"));
    }

    void endNullPointer()
    {
        _stream.writeEndElement();
    }

    void beginPointer()
    {
        _stream.writeStartElement(QLatin1String("reference"));
    }

    void endPointer()
    {
        _stream.writeEndElement();
    }

    void beginInstance()
    {
        _stream.writeStartElement(QLatin1String("instance"));
    }

    void beginInstance(const QString &type_uid)
    {
        _stream.writeStartElement(QLatin1String("instance"));
        _stream.writeAttribute(QLatin1String("type"), type_uid);
    }

    void endInstance()
    {
        _stream.writeEndElement();
    }

private:
    QXmlStreamWriter &_stream;
    impl::SavingRefMap _saving_ref_map;
    bool _next_pointer_is_reference;
};

}

#endif // QARK_QXMLOUTARCHIVE_H
