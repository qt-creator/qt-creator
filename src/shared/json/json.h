/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef JSONVALUE_H
#define JSONVALUE_H

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace Json {

class JsonArray;
class JsonObject;

namespace Internal {
class Data;
class Base;
class Object;
class Header;
class Array;
class Value;
class Entry;
class SharedString;
class Parser;
}

class JsonValue
{
public:
    enum Type {
        Null =  0x0,
        Bool = 0x1,
        Double = 0x2,
        String = 0x3,
        Array = 0x4,
        Object = 0x5,
        Undefined = 0x80
    };

    JsonValue(Type = Null);
    JsonValue(bool b);
    JsonValue(double n);
    JsonValue(int n);
    JsonValue(int64_t n);
    JsonValue(const std::string &s);
    JsonValue(const char *s);
    JsonValue(const JsonArray &a);
    JsonValue(const JsonObject &o);

    ~JsonValue();

    JsonValue(const JsonValue &other);
    JsonValue &operator =(const JsonValue &other);

    Type type() const { return t; }
    bool isNull() const { return t == Null; }
    bool isBool() const { return t == Bool; }
    bool isDouble() const { return t == Double; }
    bool isString() const { return t == String; }
    bool isArray() const { return t == Array; }
    bool isObject() const { return t == Object; }
    bool isUndefined() const { return t == Undefined; }

    bool toBool(bool defaultValue = false) const;
    int toInt(int defaultValue = 0) const;
    double toDouble(double defaultValue = 0) const;
    std::string toString(const std::string &defaultValue = std::string()) const;
    JsonArray toArray() const;
    JsonArray toArray(const JsonArray &defaultValue) const;
    JsonObject toObject() const;
    JsonObject toObject(const JsonObject &defaultValue) const;

    bool operator==(const JsonValue &other) const;
    bool operator!=(const JsonValue &other) const;

private:
    // avoid implicit conversions from char * to bool
    JsonValue(const void *) : t(Null) {}
    friend class Internal::Value;
    friend class JsonArray;
    friend class JsonObject;

    JsonValue(Internal::Data *d, Internal::Base *b, const Internal::Value& v);

    void detach();

    union {
        uint64_t ui;
        bool b;
        double dbl;
        Internal::SharedString *stringData;
        Internal::Base *base;
    };
    Internal::Data *d; // needed for Objects and Arrays
    Type t;
};

class JsonValueRef
{
public:
    JsonValueRef(JsonArray *array, int idx)
        : a(array), is_object(false), index(idx) {}
    JsonValueRef(JsonObject *object, int idx)
        : o(object), is_object(true), index(idx) {}

    operator JsonValue() const { return toValue(); }
    JsonValueRef &operator=(const JsonValue &val);
    JsonValueRef &operator=(const JsonValueRef &val);

    JsonValue::Type type() const { return toValue().type(); }
    bool isNull() const { return type() == JsonValue::Null; }
    bool isBool() const { return type() == JsonValue::Bool; }
    bool isDouble() const { return type() == JsonValue::Double; }
    bool isString() const { return type() == JsonValue::String; }
    bool isArray() const { return type() == JsonValue::Array; }
    bool isObject() const { return type() == JsonValue::Object; }
    bool isUndefined() const { return type() == JsonValue::Undefined; }

    std::string toString() const { return toValue().toString(); }
    JsonArray toArray() const;
    JsonObject toObject() const;

    bool toBool(bool defaultValue = false) const { return toValue().toBool(defaultValue); }
    int toInt(int defaultValue = 0) const { return toValue().toInt(defaultValue); }
    double toDouble(double defaultValue = 0) const { return toValue().toDouble(defaultValue); }
    std::string toString(const std::string &defaultValue) const { return toValue().toString(defaultValue); }

    bool operator==(const JsonValue &other) const { return toValue() == other; }
    bool operator!=(const JsonValue &other) const { return toValue() != other; }

private:
    JsonValue toValue() const;

    union {
        JsonArray *a;
        JsonObject *o;
    };
    uint32_t is_object : 1;
    uint32_t index : 31;
};

class JsonValuePtr
{
    JsonValue value;
public:
    explicit JsonValuePtr(const JsonValue& val)
        : value(val) {}

    JsonValue& operator*() { return value; }
    JsonValue* operator->() { return &value; }
};

class JsonValueRefPtr
{
    JsonValueRef valueRef;
public:
    JsonValueRefPtr(JsonArray *array, int idx)
        : valueRef(array, idx) {}
    JsonValueRefPtr(JsonObject *object, int idx)
        : valueRef(object, idx)  {}

    JsonValueRef& operator*() { return valueRef; }
    JsonValueRef* operator->() { return &valueRef; }
};



class JsonArray
{
public:
    JsonArray();
    JsonArray(std::initializer_list<JsonValue> args);

    ~JsonArray();

    JsonArray(const JsonArray &other);
    JsonArray &operator=(const JsonArray &other);

    int size() const;
    int count() const { return size(); }

    bool isEmpty() const;
    JsonValue at(int i) const;
    JsonValue first() const;
    JsonValue last() const;

    void prepend(const JsonValue &value);
    void append(const JsonValue &value);
    void removeAt(int i);
    JsonValue takeAt(int i);
    void removeFirst() { removeAt(0); }
    void removeLast() { removeAt(size() - 1); }

    void insert(int i, const JsonValue &value);
    void replace(int i, const JsonValue &value);

    bool contains(const JsonValue &element) const;
    JsonValueRef operator[](int i);
    JsonValue operator[](int i) const;

    bool operator==(const JsonArray &other) const;
    bool operator!=(const JsonArray &other) const;

    class const_iterator;

    class iterator {
    public:
        JsonArray *a;
        int i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef int difference_type;
        typedef JsonValue value_type;
        typedef JsonValueRef reference;
        typedef JsonValueRefPtr pointer;

        iterator() : a(nullptr), i(0) { }
        explicit iterator(JsonArray *array, int index) : a(array), i(index) { }

        JsonValueRef operator*() const { return JsonValueRef(a, i); }
        JsonValueRefPtr operator->() const { return JsonValueRefPtr(a, i); }
        JsonValueRef operator[](int j) const { return JsonValueRef(a, i + j); }

        bool operator==(const iterator &o) const { return i == o.i; }
        bool operator!=(const iterator &o) const { return i != o.i; }
        bool operator<(const iterator& other) const { return i < other.i; }
        bool operator<=(const iterator& other) const { return i <= other.i; }
        bool operator>(const iterator& other) const { return i > other.i; }
        bool operator>=(const iterator& other) const { return i >= other.i; }
        bool operator==(const const_iterator &o) const { return i == o.i; }
        bool operator!=(const const_iterator &o) const { return i != o.i; }
        bool operator<(const const_iterator& other) const { return i < other.i; }
        bool operator<=(const const_iterator& other) const { return i <= other.i; }
        bool operator>(const const_iterator& other) const { return i > other.i; }
        bool operator>=(const const_iterator& other) const { return i >= other.i; }
        iterator &operator++() { ++i; return *this; }
        iterator operator++(int) { iterator n = *this; ++i; return n; }
        iterator &operator--() { i--; return *this; }
        iterator operator--(int) { iterator n = *this; i--; return n; }
        iterator &operator+=(int j) { i+=j; return *this; }
        iterator &operator-=(int j) { i-=j; return *this; }
        iterator operator+(int j) const { return iterator(a, i+j); }
        iterator operator-(int j) const { return iterator(a, i-j); }
        int operator-(iterator j) const { return i - j.i; }
    };
    friend class iterator;

    class const_iterator {
    public:
        const JsonArray *a;
        int i;
        typedef std::random_access_iterator_tag  iterator_category;
        typedef std::ptrdiff_t difference_type;
        typedef JsonValue value_type;
        typedef JsonValue reference;
        typedef JsonValuePtr pointer;

        const_iterator() : a(nullptr), i(0) { }
        explicit const_iterator(const JsonArray *array, int index) : a(array), i(index) { }
        const_iterator(const iterator &o) : a(o.a), i(o.i) {}

        JsonValue operator*() const { return a->at(i); }
        JsonValuePtr operator->() const { return JsonValuePtr(a->at(i)); }
        JsonValue operator[](int j) const { return a->at(i+j); }
        bool operator==(const const_iterator &o) const { return i == o.i; }
        bool operator!=(const const_iterator &o) const { return i != o.i; }
        bool operator<(const const_iterator& other) const { return i < other.i; }
        bool operator<=(const const_iterator& other) const { return i <= other.i; }
        bool operator>(const const_iterator& other) const { return i > other.i; }
        bool operator>=(const const_iterator& other) const { return i >= other.i; }
        const_iterator &operator++() { ++i; return *this; }
        const_iterator operator++(int) { const_iterator n = *this; ++i; return n; }
        const_iterator &operator--() { i--; return *this; }
        const_iterator operator--(int) { const_iterator n = *this; i--; return n; }
        const_iterator &operator+=(int j) { i+=j; return *this; }
        const_iterator &operator-=(int j) { i-=j; return *this; }
        const_iterator operator+(int j) const { return const_iterator(a, i+j); }
        const_iterator operator-(int j) const { return const_iterator(a, i-j); }
        int operator-(const_iterator j) const { return i - j.i; }
    };
    friend class const_iterator;

    // stl style
    iterator begin() { detach(); return iterator(this, 0); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator constBegin() const { return const_iterator(this, 0); }
    iterator end() { detach(); return iterator(this, size()); }
    const_iterator end() const { return const_iterator(this, size()); }
    const_iterator constEnd() const { return const_iterator(this, size()); }
    iterator insert(iterator before, const JsonValue &value) { insert(before.i, value); return before; }
    iterator erase(iterator it) { removeAt(it.i); return it; }

    void push_back(const JsonValue &t) { append(t); }
    void push_front(const JsonValue &t) { prepend(t); }
    void pop_front() { removeFirst(); }
    void pop_back() { removeLast(); }
    bool empty() const { return isEmpty(); }
    typedef int size_type;
    typedef JsonValue value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef JsonValueRef reference;
    typedef JsonValue const_reference;
    typedef int difference_type;

private:
    friend class Internal::Data;
    friend class JsonValue;
    friend class JsonDocument;

    JsonArray(Internal::Data *data, Internal::Array *array);
    void compact();
    void detach(uint32_t reserve = 0);

    Internal::Data *d;
    Internal::Array *a;
};


class JsonObject
{
public:
    JsonObject();
    JsonObject(std::initializer_list<std::pair<std::string, JsonValue> > args);
    ~JsonObject();

    JsonObject(const JsonObject &other);
    JsonObject &operator =(const JsonObject &other);

    typedef std::vector<std::string> Keys;
    Keys keys() const;
    int size() const;
    int count() const { return size(); }
    int length() const { return size(); }
    bool isEmpty() const;

    JsonValue value(const std::string &key) const;
    JsonValue operator[] (const std::string &key) const;
    JsonValueRef operator[] (const std::string &key);

    void remove(const std::string &key);
    JsonValue take(const std::string &key);
    bool contains(const std::string &key) const;

    bool operator==(const JsonObject &other) const;
    bool operator!=(const JsonObject &other) const;

    class const_iterator;

    class iterator
    {
        friend class const_iterator;
        friend class JsonObject;
        JsonObject *o;
        int i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef int difference_type;
        typedef JsonValue value_type;
        typedef JsonValueRef reference;

        iterator() : o(nullptr), i(0) {}
        iterator(JsonObject *obj, int index) : o(obj), i(index) {}

        std::string key() const { return o->keyAt(i); }
        JsonValueRef value() const { return JsonValueRef(o, i); }
        JsonValueRef operator*() const { return JsonValueRef(o, i); }
        JsonValueRefPtr operator->() const { return JsonValueRefPtr(o, i); }
        bool operator==(const iterator &other) const { return i == other.i; }
        bool operator!=(const iterator &other) const { return i != other.i; }

        iterator &operator++() { ++i; return *this; }
        iterator operator++(int) { iterator r = *this; ++i; return r; }
        iterator &operator--() { --i; return *this; }
        iterator operator--(int) { iterator r = *this; --i; return r; }
        iterator operator+(int j) const
        { iterator r = *this; r.i += j; return r; }
        iterator operator-(int j) const { return operator+(-j); }
        iterator &operator+=(int j) { i += j; return *this; }
        iterator &operator-=(int j) { i -= j; return *this; }

    public:
        bool operator==(const const_iterator &other) const { return i == other.i; }
        bool operator!=(const const_iterator &other) const { return i != other.i; }
    };
    friend class iterator;

    class const_iterator
    {
        friend class iterator;
        const JsonObject *o;
        int i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef int difference_type;
        typedef JsonValue value_type;
        typedef JsonValue reference;

        const_iterator() : o(nullptr), i(0) {}
        const_iterator(const JsonObject *obj, int index)
            : o(obj), i(index) {}
        const_iterator(const iterator &other)
            : o(other.o), i(other.i) {}

        std::string key() const { return o->keyAt(i); }
        JsonValue value() const { return o->valueAt(i); }
        JsonValue operator*() const { return o->valueAt(i); }
        JsonValuePtr operator->() const { return JsonValuePtr(o->valueAt(i)); }
        bool operator==(const const_iterator &other) const { return i == other.i; }
        bool operator!=(const const_iterator &other) const { return i != other.i; }

        const_iterator &operator++() { ++i; return *this; }
        const_iterator operator++(int) { const_iterator r = *this; ++i; return r; }
        const_iterator &operator--() { --i; return *this; }
        const_iterator operator--(int) { const_iterator r = *this; --i; return r; }
        const_iterator operator+(int j) const
        { const_iterator r = *this; r.i += j; return r; }
        const_iterator operator-(int j) const { return operator+(-j); }
        const_iterator &operator+=(int j) { i += j; return *this; }
        const_iterator &operator-=(int j) { i -= j; return *this; }

        bool operator==(const iterator &other) const { return i == other.i; }
        bool operator!=(const iterator &other) const { return i != other.i; }
    };
    friend class const_iterator;

    // STL style
    iterator begin() { detach(); return iterator(this, 0); }
    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator constBegin() const { return const_iterator(this, 0); }
    iterator end() { detach(); return iterator(this, size()); }
    const_iterator end() const { return const_iterator(this, size()); }
    const_iterator constEnd() const { return const_iterator(this, size()); }
    iterator erase(iterator it);

    // more Qt
    iterator find(const std::string &key);
    const_iterator find(const std::string &key) const { return constFind(key); }
    const_iterator constFind(const std::string &key) const;
    iterator insert(const std::string &key, const JsonValue &value);

    // STL compatibility
    typedef JsonValue mapped_type;
    typedef std::string key_type;
    typedef int size_type;

    bool empty() const { return isEmpty(); }

private:
    friend class Internal::Data;
    friend class JsonValue;
    friend class JsonDocument;
    friend class JsonValueRef;

    JsonObject(Internal::Data *data, Internal::Object *object);
    void detach(uint32_t reserve = 0);
    void compact();

    std::string keyAt(int i) const;
    JsonValue valueAt(int i) const;
    void setValueAt(int i, const JsonValue &val);

    Internal::Data *d;
    Internal::Object *o;
};

struct JsonParseError
{
    enum ParseError {
        NoError = 0,
        UnterminatedObject,
        MissingNameSeparator,
        UnterminatedArray,
        MissingValueSeparator,
        IllegalValue,
        TerminationByNumber,
        IllegalNumber,
        IllegalEscapeSequence,
        IllegalUTF8String,
        UnterminatedString,
        MissingObject,
        DeepNesting,
        DocumentTooLarge,
        GarbageAtEnd
    };

    int        offset;
    ParseError error;
};

class JsonDocument
{
public:
    static const uint32_t BinaryFormatTag = ('q') | ('b' << 8) | ('j' << 16) | ('s' << 24);
    JsonDocument();
    explicit JsonDocument(const JsonObject &object);
    explicit JsonDocument(const JsonArray &array);
    ~JsonDocument();

    JsonDocument(const JsonDocument &other);
    JsonDocument &operator =(const JsonDocument &other);

    enum DataValidation {
        Validate,
        BypassValidation
    };

    static JsonDocument fromRawData(const char *data, int size, DataValidation validation = Validate);
    const char *rawData(int *size) const;

    static JsonDocument fromBinaryData(const std::string &data, DataValidation validation  = Validate);
    std::string toBinaryData() const;

    enum JsonFormat {
        Indented,
        Compact
    };

    static JsonDocument fromJson(const std::string &json, JsonParseError *error = nullptr);

    std::string toJson(JsonFormat format = Indented) const;

    bool isEmpty() const;
    bool isArray() const;
    bool isObject() const;

    JsonObject object() const;
    JsonArray array() const;

    void setObject(const JsonObject &object);
    void setArray(const JsonArray &array);

    bool operator==(const JsonDocument &other) const;
    bool operator!=(const JsonDocument &other) const { return !(*this == other); }

    bool isNull() const;

private:
    friend class JsonValue;
    friend class Internal::Data;
    friend class Internal::Parser;

    JsonDocument(Internal::Data *data);

    Internal::Data *d;
};

} // namespace Json

#endif // JSONVALUE_H
