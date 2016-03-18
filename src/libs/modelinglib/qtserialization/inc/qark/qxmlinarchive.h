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
#include "impl/objectid.h"
#include "impl/loadingrefmap.h"

#include <QList>
#include <QStack>
#include <QHash>
#include <QXmlStreamReader>
#include <exception>

namespace qark {

class QXmlInArchive : public ArchiveBasics
{
public:
    class FileFormatException : public std::exception
    {
    };

    class UnexpectedForwardReference : public std::exception
    {
    };

    static const bool inArchive = true;
    static const bool outArchive = false;

private:
    class XmlTag;

    class Node
    {
    public:
        typedef QList<Node *> ChildrenType;

        virtual ~Node() { qDeleteAll(m_children); }

        const ChildrenType &children() const { return m_children; }
        virtual QString qualifiedName() const = 0;
        virtual void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }
        void append(Node *node) { m_children.append(node); }

    private:
        ChildrenType m_children;
    };

    class TagNode : public Node
    {
    public:
        explicit TagNode(const Tag &tag) : m_tag(tag) { }

        const Tag &tag() const { return m_tag; }
        QString qualifiedName() const override { return m_tag.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag)  override
        {
            visitor.visit(this, tag);
        }

    private:
        Tag m_tag;
    };

    template<class T>
    class ObjectNode : public Node
    {
    public:
        explicit ObjectNode(const Object<T> &object) : m_object(object) { }

        QString qualifiedName() const override { return m_object.qualifiedName(); }
        Object<T> &object() { return m_object; }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }

    private:
        Object<T> m_object;
    };

    template<class T, class U>
    class BaseNode : public Node
    {
    public:
        explicit BaseNode(const Base<T, U> &base) : m_base(base) { }

        QString qualifiedName() const override { return m_base.qualifiedName(); }
        Base<T, U> &base() { return m_base; }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }

    private:
        Base<T, U> m_base;
    };

    template<class T>
    class AttrNode : public Node
    {
    public:
        explicit AttrNode(const Attr<T> &attr) : m_attr(attr) { }

        QString qualifiedName() const override { return m_attr.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        Attr<T> &attribute() { return m_attr; }

    private:
        Attr<T> m_attr;
    };

    template<class U, typename T>
    class SetterAttrNode : public Node
    {
    public:
        explicit SetterAttrNode(const SetterAttr<U, T> &attr) : m_attr(attr) { }

        QString qualifiedName() const override { return m_attr.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        SetterAttr<U, T> &attribute() { return m_attr; }

    private:
        SetterAttr<U, T> m_attr;
    };

    template<class U, typename T, typename V>
    class GetterSetterAttrNode : public Node
    {
    public:
        explicit GetterSetterAttrNode(const GetterSetterAttr<U, T, V> &attr) : m_attr(attr) { }

        QString qualifiedName() const override { return m_attr.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        GetterSetterAttr<U, T, V> &attribute() { return m_attr; }

    private:
        GetterSetterAttr<U, T, V> m_attr;
    };

    template<class U, typename T>
    class SetFuncAttrNode : public Node
    {
    public:
        explicit SetFuncAttrNode(const SetFuncAttr<U, T> &attr) : m_attr(attr) { }

        QString qualifiedName() const override { return m_attr.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        SetFuncAttr<U, T> &attribute() { return m_attr; }

    private:
        SetFuncAttr<U, T> m_attr;
    };

    template<class U, typename T, typename V>
    class GetSetFuncAttrNode : public Node
    {
    public:
        explicit GetSetFuncAttrNode(const GetSetFuncAttr<U, T, V> &attr) : m_attr(attr) { }

        QString qualifiedName() const override { return m_attr.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        GetSetFuncAttr<U, T, V> &attribute() { return m_attr; }

    private:
        GetSetFuncAttr<U, T, V> m_attr;
    };

    template<class T>
    class RefNode : public Node
    {
    public:
        explicit RefNode(const Ref<T> &ref) : m_ref(ref) { }

        QString qualifiedName() const override { return m_ref.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        Ref<T> &reference() { return m_ref; }

    private:
        Ref<T> m_ref;
    };

    template<class U, typename T>
    class SetterRefNode : public Node
    {
    public:
        explicit SetterRefNode(const SetterRef<U, T> &ref) : m_ref(ref) { }

        QString qualifiedName() const override { return m_ref.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        SetterRef<U, T> &reference() { return m_ref; }

    private:
        SetterRef<U, T> m_ref;
    };

    template<class U, typename T, typename V>
    class GetterSetterRefNode : public Node
    {
    public:
        explicit GetterSetterRefNode(const GetterSetterRef<U, T, V> &ref) : m_ref(ref) { }

        QString qualifiedName() const override { return m_ref.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        GetterSetterRef<U, T, V> &reference() { return m_ref; }

    private:
        GetterSetterRef<U, T, V> m_ref;
    };

    template<class U, typename T>
    class SetFuncRefNode : public Node
    {
    public:
        explicit SetFuncRefNode(const SetFuncRef<U, T> &ref) : m_ref(ref) { }

        QString qualifiedName() const override { return m_ref.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        SetFuncRef<U, T> &reference() { return m_ref; }

    private:
        SetFuncRef<U, T> m_ref;
    };

    template<class U, typename T, typename V>
    class GetSetFuncRefNode : public Node
    {
    public:
        explicit GetSetFuncRefNode(const GetSetFuncRef<U, T, V> &ref) : m_ref(ref) { }

        QString qualifiedName() const override { return m_ref.qualifiedName(); }
        void accept(QXmlInArchive &visitor, const XmlTag &tag) override
        {
            visitor.visit(this, tag);
        }
        GetSetFuncRef<U, T, V> &reference() { return m_ref; }

    private:
        GetSetFuncRef<U, T, V> m_ref;
    };

public:
    explicit QXmlInArchive(QXmlStreamReader &stream)
        : m_stream(stream),
          m_endTagWasRead(false),
          m_currentRefNode(0)
    {
    }

    ~QXmlInArchive()
    {
    }

    void beginDocument()
    {
        while (!m_stream.atEnd()) {
            switch (m_stream.readNext()) {
            case QXmlStreamReader::StartDocument:
                return;
            case QXmlStreamReader::Comment:
                break;
            default:
                throw FileFormatException();
            }
        }
        throw FileFormatException();
    }

    void endDocument()
    {
        if (m_endTagWasRead) {
            if (m_stream.tokenType() != QXmlStreamReader::EndDocument)
                throw FileFormatException();
        } else {
            while (!m_stream.atEnd()) {
                switch (m_stream.readNext()) {
                case QXmlStreamReader::EndDocument:
                    return;
                case QXmlStreamReader::Comment:
                    break;
                default:
                    throw FileFormatException();
                }
            }
            throw FileFormatException();
        }
    }

    void append(const Tag &tag)
    {
        auto node = new TagNode(tag);
        if (!m_nodeStack.empty())
            m_nodeStack.top()->append(node);
        m_nodeStack.push(node);
    }

    template<class T>
    void append(const Object<T> &object)
    {
        auto node = new ObjectNode<T>(object);
        if (!m_nodeStack.empty())
            m_nodeStack.top()->append(node);
        m_nodeStack.push(node);
    }

    void append(const End &)
    {
        Node *node = m_nodeStack.pop();
        if (m_nodeStack.empty()) {
            XmlTag xmlTag = readTag();
            if (xmlTag.m_tagName != node->qualifiedName() || xmlTag.m_isEndTag)
                throw FileFormatException();
            node->accept(*this, xmlTag);
            delete node;
        }
    }

    template<class T, class U>
    void append(const Base<T, U> &base)
    {
        m_nodeStack.top()->append(new BaseNode<T, U>(base));
    }

    template<typename T>
    void append(const Attr<T> &attr)
    {
        m_nodeStack.top()->append(new AttrNode<T>(attr));
    }

    template<class U, typename T>
    void append(const SetterAttr<U, T> &attr)
    {
        m_nodeStack.top()->append(new SetterAttrNode<U, T>(attr));
    }

    template<class U, typename T, typename V>
    void append(const GetterSetterAttr<U, T, V> &attr)
    {
        m_nodeStack.top()->append(new GetterSetterAttrNode<U, T, V>(attr));
    }

    template<class U, typename T>
    void append(const SetFuncAttr<U, T> &attr)
    {
        m_nodeStack.top()->append(new SetFuncAttrNode<U, T>(attr));
    }

    template<class U, typename T, typename V>
    void append(const GetSetFuncAttr<U, T, V> &attr)
    {
        m_nodeStack.top()->append(new GetSetFuncAttrNode<U, T, V>(attr));
    }

    template<typename T>
    void append(const Ref<T> &ref)
    {
        m_nodeStack.top()->append(new RefNode<T>(ref));
    }

    template<class U, typename T>
    void append(const SetterRef<U, T> &ref)
    {
        m_nodeStack.top()->append(new SetterRefNode<U, T>(ref));
    }

    template<class U, typename T, typename V>
    void append(const GetterSetterRef<U, T, V> &ref)
    {
        m_nodeStack.top()->append(new GetterSetterRefNode<U, T, V>(ref));
    }

    template<class U, typename T>
    void append(const SetFuncRef<U, T> &ref)
    {
        m_nodeStack.top()->append(new SetFuncRefNode<U, T>(ref));
    }

    template<class U, typename T, typename V>
    void append(const GetSetFuncRef<U, T, V> &ref)
    {
        m_nodeStack.top()->append(new GetSetFuncRefNode<U, T, V>(ref));
    }

#define QARK_READNUMBER(T, M) \
    void read(T *i) \
    { \
        QString s = m_stream.readElementText(); \
        m_endTagWasRead = true; \
        bool ok = false; \
        *i = s.M(&ok); \
        if (!ok) \
            throw FileFormatException(); \
    }

    QARK_READNUMBER(char, toInt)
    QARK_READNUMBER(signed char, toInt)
    QARK_READNUMBER(unsigned char, toUInt)
    QARK_READNUMBER(short int, toShort)
    QARK_READNUMBER(unsigned short int, toUShort)
    QARK_READNUMBER(int, toInt)
    QARK_READNUMBER(unsigned int, toUInt)
    QARK_READNUMBER(long int, toLong)
    QARK_READNUMBER(unsigned long int, toULong)
    QARK_READNUMBER(long long int, toLongLong)
    QARK_READNUMBER(unsigned long long int, toULongLong)
    QARK_READNUMBER(float, toFloat)
    QARK_READNUMBER(double, toDouble)

#undef QARK_READNUMBER

    void read(bool *b)
    {
        QString s = m_stream.readElementText();
        m_endTagWasRead = true;
        if (s == QLatin1String("true"))
            *b = true;
        else if (s == QLatin1String("false"))
            *b = false;
        else
            throw FileFormatException();
    }

    void read(QString *s)
    {
        *s = m_stream.readElementText();
        m_endTagWasRead = true;
    }

    enum ReferenceKind {
        Nullpointer,
        Pointer,
        Instance
    };

    class ReferenceTag
    {
    public:
        explicit ReferenceTag(ReferenceKind k = Nullpointer,
                              const QString &string = QLatin1String(""))
            : kind(k),
              typeName(string)
        {
        }

        ReferenceKind kind;
        QString typeName;
    };

    ReferenceTag readReferenceTag()
    {
        XmlTag tag = readTag();
        if (tag.m_tagName == QLatin1String("null"))
            return ReferenceTag(Nullpointer);
        else if (tag.m_tagName == QLatin1String("reference"))
            return ReferenceTag(Pointer);
        else if (tag.m_tagName == QLatin1String("instance"))
            return ReferenceTag(Instance, tag.m_attributes.value(QLatin1String("type")));
        else
            throw FileFormatException();
    }

    void readReferenceEndTag(ReferenceKind kind)
    {
        XmlTag tag = readTag();
        if (!tag.m_isEndTag)
            throw FileFormatException();
        else if (tag.m_tagName == QLatin1String("null") && kind != Nullpointer)
            throw FileFormatException();
        else if (tag.m_tagName == QLatin1String("reference") && kind != Pointer)
            throw FileFormatException();
        else if (tag.m_tagName == QLatin1String("instance") && kind != Instance)
            throw FileFormatException();
    }

    template<typename T>
    void read(T *&p)
    {
        impl::ObjectId id;
        int i;
        read(&i);
        id.set(i);
        if (m_loadingRefMap.hasObject(id))
            p = m_loadingRefMap.object<T *>(id);
        else
            throw UnexpectedForwardReference();
    }

private:

    class XmlTag
    {
    public:
        QString m_tagName;
        bool m_isEndTag = false;
        impl::ObjectId m_id;
        QHash<QString, QString> m_attributes;
    };

    void readChildren(Node *node) {
        for (;;) {
            XmlTag xmlTag = readTag();
            if (xmlTag.m_isEndTag) {
                if (xmlTag.m_tagName != node->qualifiedName())
                    throw FileFormatException();
                return;
            } else {
                bool foundTag = false;
                for (auto it = node->children().begin();
                     it != node->children().end(); ++it)
                {
                    if ((*it)->qualifiedName() == xmlTag.m_tagName) {
                        foundTag = true;
                        (*it)->accept(*this, xmlTag);
                    }
                }
                if (!foundTag)
                    skipUntilEndOfTag(xmlTag);
            }
        }
    }

    void visit(Node *, const XmlTag &)
    {
        throw FileFormatException();
    }

    void visit(TagNode *node, const XmlTag &)
    {
        readChildren(node);
    }

    template<class T>
    void visit(ObjectNode<T> *node, const XmlTag &tag)
    {
        if (tag.m_id.isValid() && node->object().object() != 0)
            m_loadingRefMap.addObject(tag.m_id, node->object().object());
        readChildren(node);
    }

    template<class T, class U>
    void visit(BaseNode<T, U> *node, const XmlTag &)
    {
        load(*this, node->base().base(), node->base().parameters());
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->base().qualifiedName())
            throw FileFormatException();
    }

    template<class T>
    void visit(AttrNode<T> *node, const XmlTag &)
    {
        load(*this, *node->attribute().value(), node->attribute().parameters());
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().object().*(node->attribute().setter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().object().*(node->attribute().setter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().object().*(node->attribute().setter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().object().*(node->attribute().setter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().setterFunc())(node->attribute().object(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().setterFunc())(node->attribute().object(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().setterFunc())(node->attribute().object(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->attribute().parameters());
        (node->attribute().setterFunc())(node->attribute().object(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->attribute().qualifiedName())
            throw FileFormatException();
    }

    template<typename T>
    void visit(RefNode<T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value = T();
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            *node->reference().value() = value;
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().object().*(node->reference().setter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().object().*(node->reference().setter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().object().*(node->reference().setter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().object().*(node->reference().setter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().setterFunc())(node->reference().object(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().setterFunc())(node->reference().object(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().setterFunc())(node->reference().object(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->reference().parameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->reference().setterFunc())(node->reference().object(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_isEndTag || xmlTag.m_tagName != node->reference().qualifiedName())
            throw FileFormatException();
    }

    inline XmlTag readTag();
    inline void skipUntilEndOfTag(const XmlTag &xmlTag);

    QXmlStreamReader &m_stream;
    bool m_endTagWasRead;
    QStack<Node *> m_nodeStack;
    impl::LoadingRefMap m_loadingRefMap;
    Node *m_currentRefNode;
};

QXmlInArchive::XmlTag QXmlInArchive::readTag()
{
    XmlTag xmlTag;

    if (m_endTagWasRead) {
        if (m_stream.tokenType() != QXmlStreamReader::EndElement)
            throw FileFormatException();
        xmlTag.m_tagName = m_stream.name().toString();
        xmlTag.m_isEndTag = true;
        m_endTagWasRead = false;
        return xmlTag;
    }

    while (!m_stream.atEnd()) {
        switch (m_stream.readNext()) {
        case QXmlStreamReader::StartElement:
            xmlTag.m_tagName = m_stream.name().toString();
            foreach (const QXmlStreamAttribute &attribute, m_stream.attributes()) {
                if (attribute.name() == QLatin1String("id")) {
                    bool ok = false;
                    int id = attribute.value().toString().toInt(&ok);
                    if (!ok)
                        throw FileFormatException();
                    xmlTag.m_id = impl::ObjectId(id);
                } else {
                    xmlTag.m_attributes.insert(attribute.name().toString(),
                                               attribute.value().toString());
                }
            }

            return xmlTag;
        case QXmlStreamReader::EndElement:
            xmlTag.m_tagName = m_stream.name().toString();
            xmlTag.m_isEndTag = true;
            return xmlTag;
        case QXmlStreamReader::Comment:
            // intentionally left blank
            break;
        case QXmlStreamReader::Characters:
            if (!m_stream.isWhitespace())
                throw FileFormatException();
            break;
        default:
            throw FileFormatException();
        }
    }
    throw FileFormatException();
}

void QXmlInArchive::skipUntilEndOfTag(const XmlTag &xmlTag)
{
    if (m_endTagWasRead)
        throw FileFormatException();
    int depth = 1;
    while (!m_stream.atEnd()) {
        switch (m_stream.readNext()) {
        case QXmlStreamReader::StartElement:
            ++depth;
            break;
        case QXmlStreamReader::EndElement:
            --depth;
            if (depth == 0) {
                if (m_stream.name().toString() != xmlTag.m_tagName)
                    throw FileFormatException();
                return;
            }
            break;
        case QXmlStreamReader::Comment:
            // intentionally left blank
            break;
        case QXmlStreamReader::Characters:
            // intentionally left blank
            break;
        default:
            throw FileFormatException();
        }
    }
    throw FileFormatException();
}

} // namespace qark
