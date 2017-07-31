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

#include "archivebasics.h"
#include "tag.h"
#include "baseclass.h"
#include "attribute.h"
#include "reference.h"
#include "impl/savingrefmap.h"

#include <QXmlStreamWriter>
#include <exception>

namespace qark {

class QXmlOutArchive : public ArchiveBasics
{
public:
    class UnsupportedForwardReference : public std::exception
    {
    };

    class DanglingReferences : public std::exception
    {
    };

    static const bool inArchive = false;
    static const bool outArchive = true;

    QXmlOutArchive(QXmlStreamWriter &stream)
        : m_stream(stream)
    {
    }

    ~QXmlOutArchive()
    {
    }

    template<typename T>
    void write(T *p)
    {
        if (!m_savingRefMap.hasDefinedRef(p))
            throw UnsupportedForwardReference();
        write(m_savingRefMap.ref(p).get());
    }

#define QARK_WRITENUMBER(T) \
    void write(T i) \
    { \
        m_stream.writeCharacters(QString::number(i)); \
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
        m_stream.writeCharacters(QLatin1String(b ? "true" : "false"));
    }

    void write(const QString &s)
    {
        m_stream.writeCharacters(s);
    }

    void beginDocument()
    {
        m_stream.writeStartDocument();
    }

    void endDocument()
    {
        m_stream.writeEndDocument();
        if (m_savingRefMap.countDanglingReferences() > 0)
            throw DanglingReferences();
    }

    void beginElement(const Tag &tag)
    {
        m_stream.writeStartElement(tag.qualifiedName());
    }

    template<class T>
    void beginElement(const Object<T> &object)
    {
        m_stream.writeStartElement(object.qualifiedName());
        // TODO implement key attribute
        // Currently qmodel files do not use references at all
        // so writing reference keys are not needed. If this
        // changes keys should be implemented as a generic
        // concept getting key from object (e.g. with a function
        // registered per type in typeregistry)
    }

    void endElement(const End &)
    {
        m_stream.writeEndElement();
    }

    template<class T, class U>
    void beginBase(const Base<T, U> &base)
    {
        m_stream.writeStartElement(base.qualifiedName());
    }

    template<class T, class U>
    void endBase(const Base<T, U> &)
    {
        m_stream.writeEndElement();
    }

    template<class T>
    void beginAttribute(const Attr<T> &attr)
    {
        m_stream.writeStartElement(attr.qualifiedName());
    }

    template<class T>
    void endAttribute(const Attr<T> &)
    {
        m_stream.writeEndElement();
    }

    template<class U, typename T>
    void beginAttribute(const GetterAttr<U, T> &attr)
    {
        m_stream.writeStartElement(attr.qualifiedName());
    }

    template<class U, typename T>
    void endAttribute(const GetterAttr<U, T> &)
    {
        m_stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginAttribute(const GetterSetterAttr<U, T, V> &attr)
    {
        m_stream.writeStartElement(attr.qualifiedName());
    }

    template<class U, typename T, typename V>
    void endAttribute(const GetterSetterAttr<U, T, V> &)
    {
        m_stream.writeEndElement();
    }

    template<class U, typename T>
    void beginAttribute(const GetFuncAttr<U, T> &attr)
    {
        m_stream.writeStartElement(attr.qualifiedName());
    }

    template<class U, typename T>
    void endAttribute(const GetFuncAttr<U, T> &)
    {
        m_stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginAttribute(const GetSetFuncAttr<U, T, V> &attr)
    {
        m_stream.writeStartElement(attr.qualifiedName());
    }

    template<class U, typename T, typename V>
    void endAttribute(const GetSetFuncAttr<U, T, V> &)
    {
        m_stream.writeEndElement();
    }

    template<class T>
    void beginReference(const Ref<T> &ref)
    {
        m_stream.writeStartElement(ref.qualifiedName());
        m_isNextPointerAReference = true;
    }

    template<class T>
    void endReference(const Ref<T> &)
    {
        m_isNextPointerAReference = false;
        m_stream.writeEndElement();
    }

    template<class U, typename T>
    void beginReference(const GetterRef<U, T> &ref)
    {
        m_stream.writeStartElement(ref.qualifiedName());
        m_isNextPointerAReference = true;
    }

    template<class U, typename T>
    void endReference(const GetterRef<U, T> &)
    {
        m_isNextPointerAReference = false;
        m_stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginReference(const GetterSetterRef<U, T, V> &ref)
    {
        m_stream.writeStartElement(ref.qualifiedName());
        m_isNextPointerAReference = true;
    }

    template<class U, typename T, typename V>
    void endReference(const GetterSetterRef<U, T, V> &)
    {
        m_isNextPointerAReference = false;
        m_stream.writeEndElement();
    }

    template<class U, typename T>
    void beginReference(const GetFuncRef<U, T> &ref)
    {
        m_stream.writeStartElement(ref.qualifiedName());
        m_isNextPointerAReference = true;
    }

    template<class U, typename T>
    void endReference(const GetFuncRef<U, T> &)
    {
        m_isNextPointerAReference = false;
        m_stream.writeEndElement();
    }

    template<class U, typename T, typename V>
    void beginReference(const GetSetFuncRef<U, T, V> &ref)
    {
        m_stream.writeStartElement(ref.qualifiedName());
        m_isNextPointerAReference = true;
    }

    template<class U, typename T, typename V>
    void endReference(const GetSetFuncRef<U, T, V> &)
    {
        m_isNextPointerAReference = false;
        m_stream.writeEndElement();
    }

    template<typename T>
    bool isReference(T *p)
    {
        return m_isNextPointerAReference || m_savingRefMap.hasDefinedRef(p);
    }

    void beginNullPointer()
    {
        m_stream.writeStartElement(QLatin1String("null"));
    }

    void endNullPointer()
    {
        m_stream.writeEndElement();
    }

    void beginPointer()
    {
        m_stream.writeStartElement(QLatin1String("reference"));
    }

    void endPointer()
    {
        m_stream.writeEndElement();
    }

    void beginInstance()
    {
        m_stream.writeStartElement(QLatin1String("instance"));
    }

    void beginInstance(const QString &typeUid)
    {
        m_stream.writeStartElement(QLatin1String("instance"));
        m_stream.writeAttribute(QLatin1String("type"), typeUid);
    }

    void endInstance()
    {
        m_stream.writeEndElement();
    }

private:
    QXmlStreamWriter &m_stream;
    impl::SavingRefMap m_savingRefMap;
    bool m_isNextPointerAReference = false;
};

} // namespace qark
