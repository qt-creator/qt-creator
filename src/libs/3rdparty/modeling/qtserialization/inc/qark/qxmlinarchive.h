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

    static const bool in_archive = true;
    static const bool out_archive = false;

private:

    struct XmlTag;

    class Node {
    public:
        typedef QList<Node *> children_type;

    public:
        virtual ~Node() { qDeleteAll(_children); }

        const children_type &getChildren() const { return _children; }

        virtual QString getQualifiedName() const = 0;

        virtual void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        void append(Node *node) { _children.push_back(node); }

    private:
        children_type _children;
    };

    class TagNode :
        public Node
    {
    public:
        explicit TagNode(const Tag &tag) : _tag(tag) { }

        const Tag &getTag() const { return _tag; }

        QString getQualifiedName() const { return _tag.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Tag _tag;
    };

    template<class T>
    class ObjectNode :
        public Node
    {
    public:
        explicit ObjectNode(const Object<T> &object) : _object(object) { }

        QString getQualifiedName() const { return _object.getQualifiedName(); }

        Object<T> &getObject() { return _object; }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Object<T> _object;
    };

    template<class T, class U>
    class BaseNode :
        public Node
    {
    public:
        explicit BaseNode(const Base<T, U> &base) : _base(base) { }

        QString getQualifiedName() const { return _base.getQualifiedName(); }

        Base<T, U> &getBase() { return _base; }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

    private:
        Base<T, U> _base;
    };

    template<class T>
    class AttrNode :
        public Node
    {
    public:
        explicit AttrNode(const Attr<T> &attr) : _attr(attr) { }

        QString getQualifiedName() const { return _attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        Attr<T> &getAttribute() { return _attr; }

    private:
        Attr<T> _attr;
    };

    template<class U, typename T>
    class SetterAttrNode :
        public Node
    {
    public:
        explicit SetterAttrNode(const SetterAttr<U, T> &attr) : _attr(attr) { }

        QString getQualifiedName() const { return _attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetterAttr<U, T> &getAttribute() { return _attr; }

    private:
        SetterAttr<U, T> _attr;
    };

    template<class U, typename T, typename V>
    class GetterSetterAttrNode :
        public Node
    {
    public:
        explicit GetterSetterAttrNode(const GetterSetterAttr<U, T, V> &attr) : _attr(attr) { }

        QString getQualifiedName() const { return _attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetterSetterAttr<U, T, V> &getAttribute() { return _attr; }

    private:
        GetterSetterAttr<U, T, V> _attr;
    };

    template<class U, typename T>
    class SetFuncAttrNode :
        public Node
    {
    public:
        explicit SetFuncAttrNode(const SetFuncAttr<U, T> &attr) : _attr(attr) { }

        QString getQualifiedName() const { return _attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetFuncAttr<U, T> &getAttribute() { return _attr; }

    private:
        SetFuncAttr<U, T> _attr;
    };

    template<class U, typename T, typename V>
    class GetSetFuncAttrNode :
        public Node
    {
    public:
        explicit GetSetFuncAttrNode(const GetSetFuncAttr<U, T, V> &attr) : _attr(attr) { }

        QString getQualifiedName() const { return _attr.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetSetFuncAttr<U, T, V> &getAttribute() { return _attr; }

    private:
        GetSetFuncAttr<U, T, V> _attr;
    };

    template<class T>
    class RefNode :
        public Node
    {
    public:
        explicit RefNode(const Ref<T> &ref) : _ref(ref) { }

        QString getQualifiedName() const { return _ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        Ref<T> &getReference() { return _ref; }

    private:
        Ref<T> _ref;
    };

    template<class U, typename T>
    class SetterRefNode :
        public Node
    {
    public:
        explicit SetterRefNode(const SetterRef<U, T> &ref) : _ref(ref) { }

        QString getQualifiedName() const { return _ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetterRef<U, T> &getReference() { return _ref; }

    private:
        SetterRef<U, T> _ref;
    };

    template<class U, typename T, typename V>
    class GetterSetterRefNode :
        public Node
    {
    public:
        explicit GetterSetterRefNode(const GetterSetterRef<U, T, V> &ref) : _ref(ref) { }

        QString getQualifiedName() const { return _ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetterSetterRef<U, T, V> &getReference() { return _ref; }

    private:
        GetterSetterRef<U, T, V> _ref;
    };

    template<class U, typename T>
    class SetFuncRefNode :
        public Node
    {
    public:
        explicit SetFuncRefNode(const SetFuncRef<U, T> &ref) : _ref(ref) { }

        QString getQualifiedName() const { return _ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        SetFuncRef<U, T> &getReference() { return _ref; }

    private:
        SetFuncRef<U, T> _ref;
    };

    template<class U, typename T, typename V>
    class GetSetFuncRefNode :
        public Node
    {
    public:
        explicit GetSetFuncRefNode(const GetSetFuncRef<U, T, V> &ref) : _ref(ref) { }

        QString getQualifiedName() const { return _ref.getQualifiedName(); }

        void accept(QXmlInArchive &visitor, const XmlTag &tag) { visitor.visit(this, tag); }

        GetSetFuncRef<U, T, V> &getReference() { return _ref; }

    private:
        GetSetFuncRef<U, T, V> _ref;
    };

public:

    explicit QXmlInArchive(QXmlStreamReader &stream)
        : _stream(stream),
          _end_tag_was_read(false),
          _current_ref_node(0)
    {
    }

    ~QXmlInArchive()
    {
    }

public:

    void beginDocument()
    {
        while (!_stream.atEnd()) {
            switch (_stream.readNext()) {
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
        if (_end_tag_was_read) {
            if (_stream.tokenType() != QXmlStreamReader::EndDocument) {
                throw FileFormatException();
            }
        } else {
            while (!_stream.atEnd()) {
                switch (_stream.readNext()) {
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
        if (!_node_stack.empty()) {
            _node_stack.top()->append(node);
        }
        _node_stack.push(node);
    }

    template<class T>
    void append(const Object<T> &object)
    {
        ObjectNode<T> *node = new ObjectNode<T>(object);
        if (!_node_stack.empty()) {
            _node_stack.top()->append(node);
        }
        _node_stack.push(node);
    }

    void append(const End &)
    {
        Node *node = _node_stack.pop();
        if (_node_stack.empty()) {
            XmlTag xml_tag = readTag();
            if (xml_tag._tag_name != node->getQualifiedName() || xml_tag._end_tag) {
                throw FileFormatException();
            }
            node->accept(*this, xml_tag);
            delete node;
        }
    }

    template<class T, class U>
    void append(const Base<T, U> &base)
    {
        _node_stack.top()->append(new BaseNode<T, U>(base));
    }

    template<typename T>
    void append(const Attr<T> &attr)
    {
        _node_stack.top()->append(new AttrNode<T>(attr));
    }

    template<class U, typename T>
    void append(const SetterAttr<U, T> &attr)
    {
        _node_stack.top()->append(new SetterAttrNode<U, T>(attr));
    }

    template<class U, typename T, typename V>
    void append(const GetterSetterAttr<U, T, V> &attr)
    {
        _node_stack.top()->append(new GetterSetterAttrNode<U, T, V>(attr));
    }

    template<class U, typename T>
    void append(const SetFuncAttr<U, T> &attr)
    {
        _node_stack.top()->append(new SetFuncAttrNode<U, T>(attr));
    }

    template<class U, typename T, typename V>
    void append(const GetSetFuncAttr<U, T, V> &attr)
    {
        _node_stack.top()->append(new GetSetFuncAttrNode<U, T, V>(attr));
    }

    template<typename T>
    void append(const Ref<T> &ref)
    {
        _node_stack.top()->append(new RefNode<T>(ref));
    }

    template<class U, typename T>
    void append(const SetterRef<U, T> &ref)
    {
        _node_stack.top()->append(new SetterRefNode<U, T>(ref));
    }

    template<class U, typename T, typename V>
    void append(const GetterSetterRef<U, T, V> &ref)
    {
        _node_stack.top()->append(new GetterSetterRefNode<U, T, V>(ref));
    }

    template<class U, typename T>
    void append(const SetFuncRef<U, T> &ref)
    {
        _node_stack.top()->append(new SetFuncRefNode<U, T>(ref));
    }

    template<class U, typename T, typename V>
    void append(const GetSetFuncRef<U, T, V> &ref)
    {
        _node_stack.top()->append(new GetSetFuncRefNode<U, T, V>(ref));
    }

public:

#define QARK_READNUMBER(T, M) \
    void read(T *i) \
    { \
        QString s = _stream.readElementText(); \
        _end_tag_was_read = true; \
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
        QString s = _stream.readElementText();
        _end_tag_was_read = true;
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
        *s = _stream.readElementText();
        _end_tag_was_read = true;
    }

public:

    enum ReferenceKind {
        NULLPOINTER,
        POINTER,
        INSTANCE
    };

    struct ReferenceTag {
        explicit ReferenceTag(ReferenceKind k = NULLPOINTER, const QString &string = QLatin1String("")) : kind(k), type_name(string) { }

        ReferenceKind kind;
        QString type_name;
    };

    ReferenceTag readReferenceTag()
    {
        XmlTag tag = readTag();
        if (tag._tag_name == QLatin1String("null")) {
            return ReferenceTag(NULLPOINTER);
        } else if (tag._tag_name == QLatin1String("reference")) {
            return ReferenceTag(POINTER);
        } else if (tag._tag_name == QLatin1String("instance")) {
            return ReferenceTag(INSTANCE, tag._attributes.value(QLatin1String("type")));
        } else {
            throw FileFormatException();
        }
    }

    void readReferenceEndTag(ReferenceKind kind)
    {
        XmlTag tag = readTag();
        if (!tag._end_tag) {
            throw FileFormatException();
        } else if (tag._tag_name == QLatin1String("null") && kind != NULLPOINTER) {
            throw FileFormatException();
        } else if (tag._tag_name == QLatin1String("reference") && kind != POINTER) {
            throw FileFormatException();
        } else if (tag._tag_name == QLatin1String("instance") && kind != INSTANCE) {
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
        if (_loading_ref_map.hasObject(id)) {
            p = _loading_ref_map.getObject<T *>(id);
        } else {
            throw UnexpectedForwardReference();
        }
    }

private:

    struct XmlTag {
        XmlTag() : _end_tag(false) { }
        QString _tag_name;
        bool _end_tag;
        impl::ObjectId _id;
        QHash<QString, QString> _attributes;
    };

    void readChildren(Node *node) {
        for (;;) {
            XmlTag xml_tag = readTag();
            if (xml_tag._end_tag) {
                if (xml_tag._tag_name != node->getQualifiedName()) {
                    throw FileFormatException();
                }
                return;
            } else {
                bool found_tag = false;
                for (Node::children_type::const_iterator it = node->getChildren().begin(); it != node->getChildren().end(); ++it) {
                    if ((*it)->getQualifiedName() == xml_tag._tag_name) {
                        found_tag = true;
                        (*it)->accept(*this, xml_tag);
                    }
                }
                if (!found_tag) {
                    skipUntilEndOfTag(xml_tag);
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
        if (tag._id.isValid() && node->getObject().getObject() != 0) {
            _loading_ref_map.addObject(tag._id, node->getObject().getObject());
        }
        readChildren(node);
    }

    template<class T, class U>
    void visit(BaseNode<T, U> *node, const XmlTag &)
    {
        load(*this, node->getBase().getBase(), node->getBase().getParameters());
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getBase().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class T>
    void visit(AttrNode<T> *node, const XmlTag &)
    {
        load(*this, *node->getAttribute().getValue(), node->getAttribute().getParameters());
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getObject().*(node->getAttribute().getSetter()))(value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, T> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncAttrNode<U, const T &> *node, const XmlTag &)
    {
        T value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, V> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncAttrNode<U, T, const V &> *node, const XmlTag &)
    {
        V value;
        load(*this, value, node->getAttribute().getParameters());
        (node->getAttribute().getSetFunc())(node->getAttribute().getObject(), value);
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getAttribute().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<typename T>
    void visit(RefNode<T> *node, const XmlTag &)
    {
        _current_ref_node = node;
        T value = T();
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            *node->getReference().getValue() = value;
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T> *node, const XmlTag &)
    {
        _current_ref_node = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetterRefNode<U, T const &> *node, const XmlTag &)
    {
        _current_ref_node = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V> *node, const XmlTag &)
    {
        _current_ref_node = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetterSetterRefNode<U, T, V const &> *node, const XmlTag &)
    {
        _current_ref_node = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getObject().*(node->getReference().getSetter()))(value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T> *node, const XmlTag &)
    {
        _current_ref_node = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T>
    void visit(SetFuncRefNode<U, T const &> *node, const XmlTag &)
    {
        _current_ref_node = node;
        T value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V> *node, const XmlTag &)
    {
        _current_ref_node = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

    template<class U, typename T, typename V>
    void visit(GetSetFuncRefNode<U, T, V const &> *node, const XmlTag &)
    {
        _current_ref_node = node;
        V value;
        load(*this, value, node->getReference().getParameters());
        if (_current_ref_node != 0) { // ref node was not consumed by forward reference
            (node->getReference().getSetFunc())(node->getReference().getObject(), value);
            _current_ref_node = 0;
        }
        XmlTag xml_tag = readTag();
        if (!xml_tag._end_tag || xml_tag._tag_name != node->getReference().getQualifiedName()) {
            throw FileFormatException();
        }
    }

private:

    inline XmlTag readTag();

    inline void skipUntilEndOfTag(const XmlTag &xml_tag);

private:
    QXmlStreamReader &_stream;
    bool _end_tag_was_read;
    QStack<Node *> _node_stack;
    impl::LoadingRefMap _loading_ref_map;
    Node *_current_ref_node;
};


QXmlInArchive::XmlTag QXmlInArchive::readTag()
{
    XmlTag xml_tag;

    if (_end_tag_was_read) {
        if (_stream.tokenType() != QXmlStreamReader::EndElement) {
            throw FileFormatException();
        }
        xml_tag._tag_name = _stream.name().toString();
        xml_tag._end_tag = true;
        _end_tag_was_read = false;
        return xml_tag;
    }

    while (!_stream.atEnd()) {
        switch (_stream.readNext()) {
        case QXmlStreamReader::StartElement:
            xml_tag._tag_name = _stream.name().toString();
            foreach (const QXmlStreamAttribute &attribute, _stream.attributes()) {
                if (attribute.name() == QLatin1String("id")) {
                    bool ok = false;
                    int id = attribute.value().toString().toInt(&ok);
                    if (!ok) {
                        throw FileFormatException();
                    }
                    xml_tag._id = impl::ObjectId(id);
                } else {
                    xml_tag._attributes.insert(attribute.name().toString(), attribute.value().toString());
                }
            }

            return xml_tag;
        case QXmlStreamReader::EndElement:
            xml_tag._tag_name = _stream.name().toString();
            xml_tag._end_tag = true;
            return xml_tag;
        case QXmlStreamReader::Comment:
            // intentionally left blank
            break;
        case QXmlStreamReader::Characters:
            if (!_stream.isWhitespace()) {
                throw FileFormatException();
            }
            break;
        default:
            throw FileFormatException();
        }
    }
    throw FileFormatException();
}

void QXmlInArchive::skipUntilEndOfTag(const XmlTag &xml_tag)
{
    if (_end_tag_was_read) {
        throw FileFormatException();
    }
    int depth = 1;
    while (!_stream.atEnd()) {
        switch (_stream.readNext()) {
        case QXmlStreamReader::StartElement:
            ++depth;
            break;
        case QXmlStreamReader::EndElement:
            --depth;
            if (depth == 0) {
                if (_stream.name().toString() != xml_tag._tag_name) {
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
