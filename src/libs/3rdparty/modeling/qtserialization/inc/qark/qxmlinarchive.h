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

#ifndef QARK_QXMLINARCHIVE_H
#define QARK_QXMLINARCHIVE_H

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

class QXmlInArchive :
        public ArchiveBasics
{
public:

    class FileFormatException :
        public std::exception
    {
    };

    class UnexpectedForwardReference :
        public std::exception
    {
    };

    static const bool inArchive = true;
    static const bool outArchive = false;

private:

    struct XmlTag;

    class Node {
    public:
        typedef QList<Node *> childrenType;

    public:
        virtual ~Node() { qDeleteAll(m_children); }

        const childrenType &getChildren() const { return m_children; }

        virtual QString getQualifiedName() const = 0;

        virtual void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        void append(Node *node) { m_children.push_back(node); }

    private:
        childrenType m_children;
    };

    class TagNode :
        public Node
    {
    public:
        explicit TagNode(const Tag &tag) : m_tag(tag) { }

        const Tag &getTag() const { return m_tag; }

        QString getQualifiedName() const { return m_tag.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Tag m_tag;
    };

    template<class T>
    class ObjectNode :
        public Node
    {
    public:
        explicit ObjectNode(const Object<T> &object) : m_object(object) { }

        QString getQualifiedName() const { return m_object.getQualifiedName(); }

        Object<T> &getObject() { return m_object; }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Object<T> m_object;
    };

    template<class T, class U>
    class BaseNode :
        public Node
    {
    public:
        explicit BaseNode(const Base<T, U> &base) : m_base(base) { }

        QString getQualifiedName() const { return m_base.getQualifiedName(); }

        Base<T, U> &getBase() { return m_base; }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Base<T, U> m_base;
    };

    template<class T>
    class AttrNode :
        public Node
    {
    public:
        explicit AttrNode(const Attr<T> &attr) : m_attr(attr) { }

        QString getQualifiedName() const { return m_attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        Attr<T> &getAttribute() { return m_attr; }

    private:
        Attr<T> m_attr;
    };

    template<class U, typename T>
    class SetterAttrNode :
        public Node
    {
    public:
        explicit SetterAttrNode(const SetterAttr<U, T> &attr) : m_attr(attr) { }

        QString getQualifiedName() const { return m_attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetterAttr<U, T> &getAttribute() { return m_attr; }

    private:
        SetterAttr<U, T> m_attr;
    };

    template<class U, typename T, typename V>
    class GetterSetterAttrNode :
        public Node
    {
    public:
        explicit GetterSetterAttrNode(const GetterSetterAttr<U, T, V> &attr) : m_attr(attr) { }

        QString getQualifiedName() const { return m_attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetterSetterAttr<U, T, V> &getAttribute() { return m_attr; }

    private:
        GetterSetterAttr<U, T, V> m_attr;
    };

    template<class U, typename T>
    class SetFuncAttrNode :
        public Node
    {
    public:
        explicit SetFuncAttrNode(const SetFuncAttr<U, T> &attr) : m_attr(attr) { }

        QString getQualifiedName() const { return m_attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetFuncAttr<U, T> &getAttribute() { return m_attr; }

    private:
        SetFuncAttr<U, T> m_attr;
    };

    template<class U, typename T, typename V>
    class GetSetFuncAttrNode :
        public Node
    {
    public:
        explicit GetSetFuncAttrNode(const GetSetFuncAttr<U, T, V> &attr) : m_attr(attr) { }

        QString getQualifiedName() const { return m_attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetSetFuncAttr<U, T, V> &getAttribute() { return m_attr; }

    private:
        GetSetFuncAttr<U, T, V> m_attr;
    };

    template<class T>
    class RefNode :
        public Node
    {
    public:
        explicit RefNode(const Ref<T> &ref) : m_ref(ref) { }

        QString getQualifiedName() const { return m_ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        Ref<T> &getReference() { return m_ref; }

    private:
        Ref<T> m_ref;
    };

    template<class U, typename T>
    class SetterRefNode :
        public Node
    {
    public:
        explicit SetterRefNode(const SetterRef<U, T> &ref) : m_ref(ref) { }

        QString getQualifiedName() const { return m_ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetterRef<U, T> &getReference() { return m_ref; }

    private:
        SetterRef<U, T> m_ref;
    };

    template<class U, typename T, typename V>
    class GetterSetterRefNode :
        public Node
    {
    public:
        explicit GetterSetterRefNode(const GetterSetterRef<U, T, V> &ref) : m_ref(ref) { }

        QString getQualifiedName() const { return m_ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetterSetterRef<U, T, V> &getReference() { return m_ref; }

    private:
        GetterSetterRef<U, T, V> m_ref;
    };

    template<class U, typename T>
    class SetFuncRefNode :
        public Node
    {
    public:
        explicit SetFuncRefNode(const SetFuncRef<U, T> &ref) : m_ref(ref) { }

        QString getQualifiedName() const { return m_ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetFuncRef<U, T> &getReference() { return m_ref; }

    private:
        SetFuncRef<U, T> m_ref;
    };

    template<class U, typename T, typename V>
    class GetSetFuncRefNode :
        public Node
    {
    public:
        explicit GetSetFuncRefNode(const GetSetFuncRef<U, T, V> &ref) : m_ref(ref) { }

        QString getQualifiedName() const { return m_ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetSetFuncRef<U, T, V> &getReference() { return m_ref; }

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

public:

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
            if (m_stream.tokenType() != QXmlStreamReader::EndDocument) {
                throw FileFormatException();
            }
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

public:

    void append(const Tag &tag)
    {
        TagNode *node = new TagNode(tag);
        if (!m_nodeStack.empty()) {
            m_nodeStack.top()->append(node);
        }
        m_nodeStack.push(node);
    }

    template<class T>
    void append(const Object<T> &object)
    {
        ObjectNode<T> *node = new ObjectNode<T>(object);
        if (!m_nodeStack.empty()) {
            m_nodeStack.top()->append(node);
        }
        m_nodeStack.push(node);
    }

    void append(const End &)
    {
        Node *node = m_nodeStack.pop();
        if (m_nodeStack.empty()) {
            XmlTag xmlTag = readTag();
            if (xmlTag.m_tagName != node->getQualifiedName() || xmlTag.m_endTag) {
                throw FileFormatException();
            }
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

public:

#define QARK_READNUMBER(T, M) \
    void read(T *i) \
    { \
        QString s = m_stream.readElementText(); \
        m_endTagWasRead = true; \
        bool ok = false; \
        *i = s.M(&ok); \
        if (!ok) { \
            throw FileFormatException(); \
        } \
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
        if (s == QLatin1String("true")) {
            *b = true;
        } else if (s == QLatin1String("false")) {
            *b = false;
        } else {
            throw FileFormatException();
        }
    }

    void read(QString *s)
    {
        *s = m_stream.readElementText();
        m_endTagWasRead = true;
    }

public:

    enum ReferenceKind {
        NULLPOINTER,
        POINTER,
        INSTANCE
    };

    struct ReferenceTag {
        explicit ReferenceTag(ReferenceKind k = NULLPOINTER, const QString &string = QLatin1String("")) : kind(k), typeName(string) { }

        ReferenceKind kind;
        QString typeName;
    };

    ReferenceTag readReferenceTag()
    {
        XmlTag tag = readTag();
        if (tag.m_tagName == QLatin1String("null")) {
            return ReferenceTag(NULLPOINTER);
        } else if (tag.m_tagName == QLatin1String("reference")) {
            return ReferenceTag(POINTER);
        } else if (tag.m_tagName == QLatin1String("instance")) {
            return ReferenceTag(INSTANCE, tag.m_attributes.value(QLatin1String("type")));
        } else {
            throw FileFormatException();
        }
    }

    void readReferenceEndTag(ReferenceKind kind)
    {
        XmlTag tag = readTag();
        if (!tag.m_endTag) {
            throw FileFormatException();
        } else if (tag.m_tagName == QLatin1String("null") && kind != NULLPOINTER) {
            throw FileFormatException();
        } else if (tag.m_tagName == QLatin1String("reference") && kind != POINTER) {
            throw FileFormatException();
        } else if (tag.m_tagName == QLatin1String("instance") && kind != INSTANCE) {
            throw FileFormatException();
        }
    }

    template<typename T>
    void read(T *&p)
    {
        impl::ObjectId id;
        int i;
        read(&i);
        id.set(i);
        if (m_loadingRefMap.hasObject(id)) {
            p = m_loadingRefMap.getObject<T *>(id);
        } else {
            throw UnexpectedForwardReference();
        }
    }

private:

    struct XmlTag {
        XmlTag() : m_endTag(false) { }
        QString m_tagName;
        bool m_endTag;
        impl::ObjectId m_id;
        QHash<QString, QString> m_attributes;
    };

    void readChildren(Node *node) {
        for (;;) {
            XmlTag xmlTag = readTag();
            if (xmlTag.m_endTag) {
                if (xmlTag.m_tagName != node->getQualifiedName()) {
                    throw FileFormatException();
                }
                return;
            } else {
                bool foundTag = false;
                for (Node::childrenType::const_iterator it = node->getChildren().begin(); it != node->getChildren().end(); ++it) {
                    if ((*it)->getQualifiedName() == xmlTag.m_tagName) {
                        foundTag = true;
                        (*it)->accept(*this, xmlTag);
                    }
                }
                if (!foundTag) {
                    skipUntilEndOfTag(xmlTag);
                }
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
        if (tag.m_id.isValid() && node->getObject().getObject() != 0) {
            m_loadingRefMap.addObject(tag.m_id, node->getObject().getObject());
        }
        readChildren(node);
    }

    template<class T, class U>
    void visit(BaseNode<T, U> *node, const XmlTag &)
    {
        load(*this, node->getBase().getBase(), node->getBase().getParameters());
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getBase().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class T>
    void visit(AttrNode<T> *node, const XmlTag &)
    {
        load(*this, *node->getAttribute().getValue(), node->getAttribute().getParameters());
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<typename T>
    void visit(RefNode<T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value = T();
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            *node->getReference().getValue() = value;
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V const &> *node, const XmlTag &)
    {
        m_currentRefNode = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (m_currentRefNode != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            m_currentRefNode = 0;
        }
        XmlTag xmlTag = readTag();
        if (!xmlTag.m_endTag || xmlTag.m_tagName != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

private:

    inline XmlTag readTag();

    inline void skipUntilEndOfTag(const XmlTag &xmlTag);

private:
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
        if (m_stream.tokenType() != QXmlStreamReader::EndElement) {
            throw FileFormatException();
        }
        xmlTag.m_tagName = m_stream.name().toString();
        xmlTag.m_endTag = true;
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
                    if (!ok) {
                        throw FileFormatException();
                    }
                    xmlTag.m_id = impl::ObjectId(id);
                } else {
                    xmlTag.m_attributes.insert(attribute.name().toString(), attribute.value().toString());
                }
            }

            return xmlTag;
        case QXmlStreamReader::EndElement:
            xmlTag.m_tagName = m_stream.name().toString();
            xmlTag.m_endTag = true;
            return xmlTag;
        case QXmlStreamReader::Comment:
            // intentionally left blank
            break;
        case QXmlStreamReader::Characters:
            if (!m_stream.isWhitespace()) {
                throw FileFormatException();
            }
            break;
        default:
            throw FileFormatException();
        }
    }
    throw FileFormatException();
}

void QXmlInArchive::skipUntilEndOfTag(const XmlTag &xmlTag)
{
    if (m_endTagWasRead) {
        throw FileFormatException();
    }
    int depth = 1;
    while (!m_stream.atEnd()) {
        switch (m_stream.readNext()) {
        case QXmlStreamReader::StartElement:
            ++depth;
            break;
        case QXmlStreamReader::EndElement:
            --depth;
            if (depth == 0) {
                if (m_stream.name().toString() != xmlTag.m_tagName) {
                    throw FileFormatException();
                }
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

}

#endif // QARK_XMLINARCHIVE_H
