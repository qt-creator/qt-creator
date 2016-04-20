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

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <limits.h>
#include <string.h>

#include "json.h"


//#define PARSER_DEBUG
#ifdef PARSER_DEBUG
static int indent = 0;
#define BEGIN std::cerr << std::string(4*indent++, ' ').data() << " pos=" << current
#define END --indent
#define DEBUG std::cerr << std::string(4*indent, ' ').data()
#else
#define BEGIN if (1) ; else std::cerr
#define END do {} while (0)
#define DEBUG if (1) ; else std::cerr
#endif

static const int nestingLimit = 1024;


namespace Json {
namespace Internal {

/*
  This defines a binary data structure for Json data. The data structure is optimised for fast reading
  and minimum allocations. The whole data structure can be mmap'ed and used directly.

  In most cases the binary structure is not as space efficient as a utf8 encoded text representation, but
  much faster to access.

  The size requirements are:

  String: 4 bytes header + 2*(string.length())

  Values: 4 bytes + size of data (size can be 0 for some data)
    bool: 0 bytes
    double: 8 bytes (0 if integer with less than 27bits)
    string: see above
    array: size of array
    object: size of object
  Array: 12 bytes + 4*length + size of Value data
  Object: 12 bytes + 8*length + size of Key Strings + size of Value data

  For an example such as

    {                                           // object: 12 + 5*8                   = 52
         "firstName": "John",                   // key 12, value 8                    = 20
         "lastName" : "Smith",                  // key 12, value 8                    = 20
         "age"      : 25,                       // key 8, value 0                     = 8
         "address"  :                           // key 12, object below               = 140
         {                                      // object: 12 + 4*8
             "streetAddress": "21 2nd Street",  // key 16, value 16
             "city"         : "New York",       // key 8, value 12
             "state"        : "NY",             // key 8, value 4
             "postalCode"   : "10021"           // key 12, value 8
         },                                     // object total: 128
         "phoneNumber":                         // key: 16, value array below         = 172
         [                                      // array: 12 + 2*4 + values below: 156
             {                                  // object 12 + 2*8
               "type"  : "home",                // key 8, value 8
               "number": "212 555-1234"         // key 8, value 16
             },                                 // object total: 68
             {                                  // object 12 + 2*8
               "type"  : "fax",                 // key 8, value 8
               "number": "646 555-4567"         // key 8, value 16
             }                                  // object total: 68
         ]                                      // array total: 156
    }                                           // great total:                         412 bytes

    The uncompressed text file used roughly 500 bytes, so in this case we end up using about
    the same space as the text representation.

    Other measurements have shown a slightly bigger binary size than a compact text
    representation where all possible whitespace was stripped out.
*/

class Array;
class Object;
class Value;
class Entry;

template<int pos, int width>
class qle_bitfield
{
public:
    uint32_t val;

    enum {
        mask = ((1u << width) - 1) << pos
    };

    void operator=(uint32_t t) {
        uint32_t i = val;
        i &= ~mask;
        i |= t << pos;
        val = i;
    }
    operator uint32_t() const {
        uint32_t t = val;
        t &= mask;
        t >>= pos;
        return t;
    }
    bool operator!() const { return !operator uint32_t(); }
    bool operator==(uint32_t t) { return uint32_t(*this) == t; }
    bool operator!=(uint32_t t) { return uint32_t(*this) != t; }
    bool operator<(uint32_t t) { return uint32_t(*this) < t; }
    bool operator>(uint32_t t) { return uint32_t(*this) > t; }
    bool operator<=(uint32_t t) { return uint32_t(*this) <= t; }
    bool operator>=(uint32_t t) { return uint32_t(*this) >= t; }
    void operator+=(uint32_t i) { *this = (uint32_t(*this) + i); }
    void operator-=(uint32_t i) { *this = (uint32_t(*this) - i); }
};

template<int pos, int width>
class qle_signedbitfield
{
public:
    uint32_t val;

    enum {
        mask = ((1u << width) - 1) << pos
    };

    void operator=(int t) {
        uint32_t i = val;
        i &= ~mask;
        i |= t << pos;
        val = i;
    }
    operator int() const {
        uint32_t i = val;
        i <<= 32 - width - pos;
        int t = (int) i;
        t >>= pos;
        return t;
    }

    bool operator!() const { return !operator int(); }
    bool operator==(int t) { return int(*this) == t; }
    bool operator!=(int t) { return int(*this) != t; }
    bool operator<(int t) { return int(*this) < t; }
    bool operator>(int t) { return int(*this) > t; }
    bool operator<=(int t) { return int(*this) <= t; }
    bool operator>=(int t) { return int(*this) >= t; }
    void operator+=(int i) { *this = (int(*this) + i); }
    void operator-=(int i) { *this = (int(*this) - i); }
};

typedef uint32_t offset;

// round the size up to the next 4 byte boundary
int alignedSize(int size) { return (size + 3) & ~3; }

static int qStringSize(const std::string &ba)
{
    int l = 4 + static_cast<int>(ba.length());
    return alignedSize(l);
}

// returns INT_MAX if it can't compress it into 28 bits
static int compressedNumber(double d)
{
    // this relies on details of how ieee floats are represented
    const int exponent_off = 52;
    const uint64_t fraction_mask = 0x000fffffffffffffull;
    const uint64_t exponent_mask = 0x7ff0000000000000ull;

    uint64_t val;
    memcpy (&val, &d, sizeof(double));
    int exp = (int)((val & exponent_mask) >> exponent_off) - 1023;
    if (exp < 0 || exp > 25)
        return INT_MAX;

    uint64_t non_int = val & (fraction_mask >> exp);
    if (non_int)
        return INT_MAX;

    bool neg = (val >> 63) != 0;
    val &= fraction_mask;
    val |= ((uint64_t)1 << 52);
    int res = (int)(val >> (52 - exp));
    return neg ? -res : res;
}

static void toInternal(char *addr, const char *data, int size)
{
    memcpy(addr, &size, 4);
    memcpy(addr + 4, data, size);
}

class String
{
public:
    String(const char *data) { d = (Data *)data; }

    struct Data {
        int length;
        char utf8[1];
    };

    Data *d;

    void operator=(const std::string &ba)
    {
        d->length = static_cast<int>(ba.length());
        memcpy(d->utf8, ba.data(), ba.length());
    }

    bool operator==(const std::string &ba) const {
        return toString() == ba;
    }
    bool operator!=(const std::string &str) const {
        return !operator==(str);
    }
    bool operator>=(const std::string &str) const {
        // ###
        return toString() >= str;
    }

    bool operator==(const String &str) const {
        if (d->length != str.d->length)
            return false;
        return !memcmp(d->utf8, str.d->utf8, d->length);
    }
    bool operator<(const String &other) const;
    bool operator>=(const String &other) const { return !(*this < other); }

    std::string toString() const {
        return std::string(d->utf8, d->length);
    }

};

bool String::operator<(const String &other) const
{
    int alen = d->length;
    int blen = other.d->length;
    int l = std::min(alen, blen);
    char *a = d->utf8;
    char *b = other.d->utf8;

    while (l-- && *a == *b)
        a++,b++;
    if (l==-1)
        return (alen < blen);
    return (unsigned char)(*a) < (unsigned char)(*b);
}

static void copyString(char *dest, const std::string &str)
{
    String string(dest);
    string = str;
}


/*
 Base is the base class for both Object and Array. Both classe work more or less the same way.
 The class starts with a header (defined by the struct below), then followed by data (the data for
 values in the Array case and Entry's (see below) for objects.

 After the data a table follows (tableOffset points to it) containing Value objects for Arrays, and
 offsets from the beginning of the object to Entry's in the case of Object.

 Entry's in the Object's table are lexicographically sorted by key in the table(). This allows the usage
 of a binary search over the keys in an Object.
 */
class Base
{
public:
    uint32_t size;
    union {
        uint32_t _dummy;
        qle_bitfield<0, 1> is_object;
        qle_bitfield<1, 31> length;
    };
    offset tableOffset;
    // content follows here

    bool isObject() const { return !!is_object; }
    bool isArray() const { return !isObject(); }

    offset *table() const { return (offset *) (((char *) this) + tableOffset); }

    int reserveSpace(uint32_t dataSize, int posInTable, uint32_t numItems, bool replace);
    void removeItems(int pos, int numItems);
};

class Object : public Base
{
public:
    Entry *entryAt(int i) const {
        return reinterpret_cast<Entry *>(((char *)this) + table()[i]);
    }
    int indexOf(const std::string &key, bool *exists);

    bool isValid() const;
};


class Value
{
public:
    enum {
        MaxSize = (1<<27) - 1
    };
    union {
        uint32_t _dummy;
        qle_bitfield<0, 3> type;
        qle_bitfield<3, 1> intValue;
        qle_bitfield<4, 1> _; // Ex-latin1Key
        qle_bitfield<5, 27> value;  // Used as offset in case of Entry(?)
        qle_signedbitfield<5, 27> int_value;
    };

    char *data(const Base *b) const { return ((char *)b) + value; }
    int usedStorage(const Base *b) const;

    bool toBoolean() const { return value != 0; }
    double toDouble(const Base *b) const;
    std::string toString(const Base *b) const;
    Base *base(const Base *b) const;

    bool isValid(const Base *b) const;

    static int requiredStorage(JsonValue &v, bool *compressed);
    static uint32_t valueToStore(const JsonValue &v, uint32_t offset);
    static void copyData(const JsonValue &v, char *dest, bool compressed);
};

class Array : public Base
{
public:
    Value at(int i) const { return *(Value *) (table() + i); }
    Value &operator[](int i) { return *(Value *) (table() + i); }

    bool isValid() const;
};

class Entry {
public:
    Value value;
    // key
    // value data follows key

    int size() const
    {
        int s = sizeof(Entry);
        s += sizeof(uint32_t) + (*(int *) ((const char *)this + sizeof(Entry)));
        return alignedSize(s);
    }

    int usedStorage(Base *b) const
    {
        return size() + value.usedStorage(b);
    }

    String shallowKey() const
    {
        return String((const char *)this + sizeof(Entry));
    }

    std::string key() const
    {
        return shallowKey().toString();
    }

    bool operator==(const std::string &key) const;
    bool operator!=(const std::string &key) const { return !operator==(key); }
    bool operator>=(const std::string &key) const { return shallowKey() >= key; }

    bool operator==(const Entry &other) const;
    bool operator>=(const Entry &other) const;
};

bool operator<(const std::string &key, const Entry &e)
{
    return e >= key;
}


class Header
{
public:
    uint32_t tag; // 'qbjs'
    uint32_t version; // 1
    Base *root() { return (Base *)(this + 1); }
};


double Value::toDouble(const Base *b) const
{
    // assert(type == JsonValue::Double);
    if (intValue)
        return int_value;

    double d;
    memcpy(&d, (const char *)b + value, 8);
    return d;
}

std::string Value::toString(const Base *b) const
{
    String s(data(b));
    return s.toString();
}

Base *Value::base(const Base *b) const
{
    // assert(type == JsonValue::Array || type == JsonValue::Object);
    return reinterpret_cast<Base *>(data(b));
}

class AtomicInt
{
public:
    bool ref() { return ++x != 0; }
    bool deref() { return --x != 0; }
    int load() { return x.load(std::memory_order_seq_cst); }
private:
    std::atomic<int> x { 0 };
};


class SharedString
{
public:
    AtomicInt ref;
    std::string s;
};

class Data {
public:
    enum Validation {
        Unchecked,
        Validated,
        Invalid
    };

    AtomicInt ref;
    int alloc;
    union {
        char *rawData;
        Header *header;
    };
    uint32_t compactionCounter : 31;
    uint32_t ownsData : 1;

    Data(char *raw, int a)
        : alloc(a), rawData(raw), compactionCounter(0), ownsData(true)
    {
    }
    Data(int reserved, JsonValue::Type valueType)
        : rawData(0), compactionCounter(0), ownsData(true)
    {
        // assert(valueType == JsonValue::Array || valueType == JsonValue::Object);

        alloc = sizeof(Header) + sizeof(Base) + reserved + sizeof(offset);
        header = (Header *)malloc(alloc);
        header->tag = JsonDocument::BinaryFormatTag;
        header->version = 1;
        Base *b = header->root();
        b->size = sizeof(Base);
        b->is_object = (valueType == JsonValue::Object);
        b->tableOffset = sizeof(Base);
        b->length = 0;
    }
    ~Data()
    { if (ownsData) free(rawData); }

    uint32_t offsetOf(const void *ptr) const { return (uint32_t)(((char *)ptr - rawData)); }

    JsonObject toObject(Object *o) const
    {
        return JsonObject(const_cast<Data *>(this), o);
    }

    JsonArray toArray(Array *a) const
    {
        return JsonArray(const_cast<Data *>(this), a);
    }

    Data *clone(Base *b, int reserve = 0)
    {
        int size = sizeof(Header) + b->size;
        if (b == header->root() && ref.load() == 1 && alloc >= size + reserve)
            return this;

        if (reserve) {
            if (reserve < 128)
                reserve = 128;
            size = std::max(size + reserve, size *2);
        }
        char *raw = (char *)malloc(size);
        memcpy(raw + sizeof(Header), b, b->size);
        Header *h = (Header *)raw;
        h->tag = JsonDocument::BinaryFormatTag;
        h->version = 1;
        Data *d = new Data(raw, size);
        d->compactionCounter = (b == header->root()) ? compactionCounter : 0;
        return d;
    }

    void compact();
    bool valid() const;

private:
    Data(const Data &);
    void operator=(const Data &);
};


void objectToJson(const Object *o, std::string &json, int indent, bool compact = false);
void arrayToJson(const Array *a, std::string &json, int indent, bool compact = false);

class Parser
{
public:
    Parser(const char *json, int length);

    JsonDocument parse(JsonParseError *error);

    class ParsedObject
    {
    public:
        ParsedObject(Parser *p, int pos) : parser(p), objectPosition(pos) {
            offsets.reserve(64);
        }
        void insert(uint32_t offset);

        Parser *parser;
        int objectPosition;
        std::vector<uint32_t> offsets;

        Entry *entryAt(size_t i) const {
            return reinterpret_cast<Entry *>(parser->data + objectPosition + offsets[i]);
        }
    };


private:
    void eatBOM();
    bool eatSpace();
    char nextToken();

    bool parseObject();
    bool parseArray();
    bool parseMember(int baseOffset);
    bool parseString();
    bool parseEscapeSequence();
    bool parseValue(Value *val, int baseOffset);
    bool parseNumber(Value *val, int baseOffset);

    void addChar(char c) {
        const int pos = reserveSpace(1);
        data[pos] = c;
    }

    const char *head;
    const char *json;
    const char *end;

    char *data;
    int dataLength;
    int current;
    int nestingLevel;
    JsonParseError::ParseError lastError;

    int reserveSpace(int space) {
        if (current + space >= dataLength) {
            dataLength = 2*dataLength + space;
            data = (char *)realloc(data, dataLength);
        }
        int pos = current;
        current += space;
        return pos;
    }
};

} // namespace Internal

using namespace Internal;

/*!
    \class JsonValue
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The JsonValue class encapsulates a value in JSON.

    A value in JSON can be one of 6 basic types:

    JSON is a format to store structured data. It has 6 basic data types:

    \list
    \li bool JsonValue::Bool
    \li double JsonValue::Double
    \li string JsonValue::String
    \li array JsonValue::Array
    \li object JsonValue::Object
    \li null JsonValue::Null
    \endlist

    A value can represent any of the above data types. In addition, JsonValue has one special
    flag to represent undefined values. This can be queried with isUndefined().

    The type of the value can be queried with type() or accessors like isBool(), isString(), and so on.
    Likewise, the value can be converted to the type stored in it using the toBool(), toString() and so on.

    Values are strictly typed internally and contrary to QVariant will not attempt to do any implicit type
    conversions. This implies that converting to a type that is not stored in the value will return a default
    constructed return value.

    \section1 JsonValueRef

    JsonValueRef is a helper class for JsonArray and JsonObject.
    When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the element in the JsonArray or JsonObject
    from which you got the reference.

    The following methods return JsonValueRef:
    \list
    \li \l {JsonArray}::operator[](int i)
    \li \l {JsonObject}::operator[](const QString & key) const
    \endlist

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    Creates a JsonValue of type \a type.

    The default is to create a Null value.
 */
JsonValue::JsonValue(Type type)
    : ui(0), d(0), t(type)
{
}

/*!
    \internal
 */
JsonValue::JsonValue(Internal::Data *data, Internal::Base *base, const Internal::Value &v)
    : d(0), t((Type)(uint32_t)v.type)
{
    switch (t) {
    case Undefined:
    case Null:
        dbl = 0;
        break;
    case Bool:
        b = v.toBoolean();
        break;
    case Double:
        dbl = v.toDouble(base);
        break;
    case String: {
        stringData = new Internal::SharedString;
        stringData->s = v.toString(base);
        stringData->ref.ref();
        break;
    }
    case Array:
    case Object:
        d = data;
        this->base = v.base(base);
        break;
    }
    if (d)
        d->ref.ref();
}

/*!
    Creates a value of type Bool, with value \a b.
 */
JsonValue::JsonValue(bool b)
    : d(0), t(Bool)
{
    this->b = b;
}

/*!
    Creates a value of type Double, with value \a n.
 */
JsonValue::JsonValue(double n)
    : d(0), t(Double)
{
    this->dbl = n;
}

/*!
    \overload
    Creates a value of type Double, with value \a n.
 */
JsonValue::JsonValue(int n)
    : d(0), t(Double)
{
    this->dbl = n;
}

/*!
    \overload
    Creates a value of type Double, with value \a n.
    NOTE: the integer limits for IEEE 754 double precision data is 2^53 (-9007199254740992 to +9007199254740992).
    If you pass in values outside this range expect a loss of precision to occur.
 */
JsonValue::JsonValue(int64_t n)
    : d(0), t(Double)
{
    this->dbl = double(n);
}

/*!
    Creates a value of type String, with value \a s.
 */
JsonValue::JsonValue(const std::string &s)
    : d(0), t(String)
{
    stringData = new Internal::SharedString;
    stringData->s = s;
    stringData->ref.ref();
}

JsonValue::JsonValue(const char *s)
    : d(0), t(String)
{
    stringData = new Internal::SharedString;
    stringData->s = s;
    stringData->ref.ref();
}

/*!
    Creates a value of type Array, with value \a a.
 */
JsonValue::JsonValue(const JsonArray &a)
    : d(a.d), t(Array)
{
    base = a.a;
    if (d)
        d->ref.ref();
}

/*!
    Creates a value of type Object, with value \a o.
 */
JsonValue::JsonValue(const JsonObject &o)
    : d(o.d), t(Object)
{
    base = o.o;
    if (d)
        d->ref.ref();
}


/*!
    Destroys the value.
 */
JsonValue::~JsonValue()
{
    if (t == String && stringData && !stringData->ref.deref())
        free(stringData);

    if (d && !d->ref.deref())
        delete d;
}

/*!
    Creates a copy of \a other.
 */
JsonValue::JsonValue(const JsonValue &other)
    : t(other.t)
{
    d = other.d;
    ui = other.ui;
    if (d)
        d->ref.ref();

    if (t == String && stringData)
        stringData->ref.ref();
}

/*!
    Assigns the value stored in \a other to this object.
 */
JsonValue &JsonValue::operator=(const JsonValue &other)
{
    if (t == String && stringData && !stringData->ref.deref())
        free(stringData);

    t = other.t;
    dbl = other.dbl;

    if (d != other.d) {

        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();

    }

    if (t == String && stringData)
        stringData->ref.ref();

    return *this;
}

/*!
    \fn bool JsonValue::isNull() const

    Returns \c true if the value is null.
*/

/*!
    \fn bool JsonValue::isBool() const

    Returns \c true if the value contains a boolean.

    \sa toBool()
 */

/*!
    \fn bool JsonValue::isDouble() const

    Returns \c true if the value contains a double.

    \sa toDouble()
 */

/*!
    \fn bool JsonValue::isString() const

    Returns \c true if the value contains a string.

    \sa toString()
 */

/*!
    \fn bool JsonValue::isArray() const

    Returns \c true if the value contains an array.

    \sa toArray()
 */

/*!
    \fn bool JsonValue::isObject() const

    Returns \c true if the value contains an object.

    \sa toObject()
 */

/*!
    \fn bool JsonValue::isUndefined() const

    Returns \c true if the value is undefined. This can happen in certain
    error cases as e.g. accessing a non existing key in a JsonObject.
 */


/*!
    \enum JsonValue::Type

    This enum describes the type of the JSON value.

    \value Null     A Null value
    \value Bool     A boolean value. Use toBool() to convert to a bool.
    \value Double   A double. Use toDouble() to convert to a double.
    \value String   A string. Use toString() to convert to a QString.
    \value Array    An array. Use toArray() to convert to a JsonArray.
    \value Object   An object. Use toObject() to convert to a JsonObject.
    \value Undefined The value is undefined. This is usually returned as an
                    error condition, when trying to read an out of bounds value
                    in an array or a non existent key in an object.
*/

/*!
    Returns the type of the value.

    \sa JsonValue::Type
 */


/*!
    Converts the value to a bool and returns it.

    If type() is not bool, the \a defaultValue will be returned.
 */
bool JsonValue::toBool(bool defaultValue) const
{
    if (t != Bool)
        return defaultValue;
    return b;
}

/*!
    Converts the value to an int and returns it.

    If type() is not Double or the value is not a whole number,
    the \a defaultValue will be returned.
 */
int JsonValue::toInt(int defaultValue) const
{
    if (t == Double && int(dbl) == dbl)
        return int(dbl);
    return defaultValue;
}

/*!
    Converts the value to a double and returns it.

    If type() is not Double, the \a defaultValue will be returned.
 */
double JsonValue::toDouble(double defaultValue) const
{
    if (t != Double)
        return defaultValue;
    return dbl;
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, the \a defaultValue will be returned.
 */
std::string JsonValue::toString(const std::string &defaultValue) const
{
    if (t != String)
        return defaultValue;
    return stringData->s;
}

/*!
    Converts the value to an array and returns it.

    If type() is not Array, the \a defaultValue will be returned.
 */
JsonArray JsonValue::toArray(const JsonArray &defaultValue) const
{
    if (!d || t != Array)
        return defaultValue;

    return JsonArray(d, static_cast<Internal::Array *>(base));
}

/*!
    \overload

    Converts the value to an array and returns it.

    If type() is not Array, a \l{JsonArray::}{JsonArray()} will be returned.
 */
JsonArray JsonValue::toArray() const
{
    return toArray(JsonArray());
}

/*!
    Converts the value to an object and returns it.

    If type() is not Object, the \a defaultValue will be returned.
 */
JsonObject JsonValue::toObject(const JsonObject &defaultValue) const
{
    if (!d || t != Object)
        return defaultValue;

    return JsonObject(d, static_cast<Internal::Object *>(base));
}

/*!
    \overload

    Converts the value to an object and returns it.

    If type() is not Object, the \l {JsonObject::}{JsonObject()} will be returned.
*/
JsonObject JsonValue::toObject() const
{
    return toObject(JsonObject());
}

/*!
    Returns \c true if the value is equal to \a other.
 */
bool JsonValue::operator==(const JsonValue &other) const
{
    if (t != other.t)
        return false;

    switch (t) {
    case Undefined:
    case Null:
        break;
    case Bool:
        return b == other.b;
    case Double:
        return dbl == other.dbl;
    case String:
        return toString() == other.toString();
    case Array:
        if (base == other.base)
            return true;
        if (!base)
            return !other.base->length;
        if (!other.base)
            return !base->length;
        return JsonArray(d, static_cast<Internal::Array *>(base))
                == JsonArray(other.d, static_cast<Internal::Array *>(other.base));
    case Object:
        if (base == other.base)
            return true;
        if (!base)
            return !other.base->length;
        if (!other.base)
            return !base->length;
        return JsonObject(d, static_cast<Internal::Object *>(base))
                == JsonObject(other.d, static_cast<Internal::Object *>(other.base));
    }
    return true;
}

/*!
    Returns \c true if the value is not equal to \a other.
 */
bool JsonValue::operator!=(const JsonValue &other) const
{
    return !(*this == other);
}

/*!
    \internal
 */
void JsonValue::detach()
{
    if (!d)
        return;

    Internal::Data *x = d->clone(base);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    base = static_cast<Internal::Object *>(d->header->root());
}


/*!
    \class JsonValueRef
    \inmodule QtCore
    \reentrant
    \brief The JsonValueRef class is a helper class for JsonValue.

    \internal

    \ingroup json

    When you get an object of type JsonValueRef, if you can assign to it,
    the assignment will apply to the character in the string from
    which you got the reference. That is its whole purpose in life.

    You can use it exactly in the same way as a reference to a JsonValue.

    The JsonValueRef becomes invalid once modifications are made to the
    string: if you want to keep the character, copy it into a JsonValue.

    Most of the JsonValue member functions also exist in JsonValueRef.
    However, they are not explicitly documented here.
*/


JsonValueRef &JsonValueRef::operator=(const JsonValue &val)
{
    if (is_object)
        o->setValueAt(index, val);
    else
        a->replace(index, val);

    return *this;
}

JsonValueRef &JsonValueRef::operator=(const JsonValueRef &ref)
{
    if (is_object)
        o->setValueAt(index, ref);
    else
        a->replace(index, ref);

    return *this;
}

JsonArray JsonValueRef::toArray() const
{
    return toValue().toArray();
}

JsonObject JsonValueRef::toObject() const
{
    return toValue().toObject();
}

JsonValue JsonValueRef::toValue() const
{
    if (!is_object)
        return a->at(index);
    return o->valueAt(index);
}

/*!
    \class JsonArray
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The JsonArray class encapsulates a JSON array.

    A JSON array is a list of values. The list can be manipulated by inserting and
    removing JsonValue's from the array.

    A JsonArray can be converted to and from a QVariantList. You can query the
    number of entries with size(), insert(), and removeAt() entries from it
    and iterate over its content using the standard C++ iterator pattern.

    JsonArray is an implicitly shared class and shares the data with the document
    it has been created from as long as it is not being modified.

    You can convert the array to and from text based JSON through JsonDocument.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \typedef JsonArray::Iterator

    Qt-style synonym for JsonArray::iterator.
*/

/*!
    \typedef JsonArray::ConstIterator

    Qt-style synonym for JsonArray::const_iterator.
*/

/*!
    \typedef JsonArray::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::value_type

    Typedef for JsonValue. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::difference_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::pointer

    Typedef for JsonValue *. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::const_pointer

    Typedef for const JsonValue *. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::reference

    Typedef for JsonValue &. Provided for STL compatibility.
*/

/*!
    \typedef JsonArray::const_reference

    Typedef for const JsonValue &. Provided for STL compatibility.
*/

/*!
    Creates an empty array.
 */
JsonArray::JsonArray()
    : d(0), a(0)
{
}

JsonArray::JsonArray(std::initializer_list<JsonValue> args)
    : d(0), a(0)
{
    for (auto i = args.begin(); i != args.end(); ++i)
        append(*i);
}

/*!
    \fn JsonArray::JsonArray(std::initializer_list<JsonValue> args)
    \since 5.4
    Creates an array initialized from \a args initialization list.

    JsonArray can be constructed in a way similar to JSON notation,
    for example:
    \code
    JsonArray array = { 1, 2.2, QString() };
    \endcode
 */

/*!
    \internal
 */
JsonArray::JsonArray(Internal::Data *data, Internal::Array *array)
    : d(data), a(array)
{
    // assert(data);
    // assert(array);
    d->ref.ref();
}

/*!
    Deletes the array.
 */
JsonArray::~JsonArray()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Creates a copy of \a other.

    Since JsonArray is implicitly shared, the copy is shallow
    as long as the object doesn't get modified.
 */
JsonArray::JsonArray(const JsonArray &other)
{
    d = other.d;
    a = other.a;
    if (d)
        d->ref.ref();
}

/*!
    Assigns \a other to this array.
 */
JsonArray &JsonArray::operator=(const JsonArray &other)
{
    if (d != other.d) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();
    }
    a = other.a;

    return *this;
}

/*! \fn JsonArray &JsonArray::operator+=(const JsonValue &value)

    Appends \a value to the array, and returns a reference to the array itself.

    \since 5.3
    \sa append(), operator<<()
*/

/*! \fn JsonArray JsonArray::operator+(const JsonValue &value) const

    Returns an array that contains all the items in this array followed
    by the provided \a value.

    \since 5.3
    \sa operator+=()
*/

/*! \fn JsonArray &JsonArray::operator<<(const JsonValue &value)

    Appends \a value to the array, and returns a reference to the array itself.

    \since 5.3
    \sa operator+=(), append()
*/

/*!
    Returns the number of values stored in the array.
 */
int JsonArray::size() const
{
    if (!d)
        return 0;

    return (int)a->length;
}

/*!
    \fn JsonArray::count() const

    Same as size().

    \sa size()
*/

/*!
    Returns \c true if the object is empty. This is the same as size() == 0.

    \sa size()
 */
bool JsonArray::isEmpty() const
{
    if (!d)
        return true;

    return !a->length;
}

/*!
    Returns a JsonValue representing the value for index \a i.

    The returned JsonValue is \c Undefined, if \a i is out of bounds.

 */
JsonValue JsonArray::at(int i) const
{
    if (!a || i < 0 || i >= (int)a->length)
        return JsonValue(JsonValue::Undefined);

    return JsonValue(d, a, a->at(i));
}

/*!
    Returns the first value stored in the array.

    Same as \c at(0).

    \sa at()
 */
JsonValue JsonArray::first() const
{
    return at(0);
}

/*!
    Returns the last value stored in the array.

    Same as \c{at(size() - 1)}.

    \sa at()
 */
JsonValue JsonArray::last() const
{
    return at(a ? (a->length - 1) : 0);
}

/*!
    Inserts \a value at the beginning of the array.

    This is the same as \c{insert(0, value)} and will prepend \a value to the array.

    \sa append(), insert()
 */
void JsonArray::prepend(const JsonValue &value)
{
    insert(0, value);
}

/*!
    Inserts \a value at the end of the array.

    \sa prepend(), insert()
 */
void JsonArray::append(const JsonValue &value)
{
    insert(a ? (int)a->length : 0, value);
}

/*!
    Removes the value at index position \a i. \a i must be a valid
    index position in the array (i.e., \c{0 <= i < size()}).

    \sa insert(), replace()
 */
void JsonArray::removeAt(int i)
{
    if (!a || i < 0 || i >= (int)a->length)
        return;

    detach();
    a->removeItems(i, 1);
    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(a->length) / 2u)
        compact();
}

/*! \fn void JsonArray::removeFirst()

    Removes the first item in the array. Calling this function is
    equivalent to calling \c{removeAt(0)}. The array must not be empty. If
    the array can be empty, call isEmpty() before calling this
    function.

    \sa removeAt(), removeLast()
*/

/*! \fn void JsonArray::removeLast()

    Removes the last item in the array. Calling this function is
    equivalent to calling \c{removeAt(size() - 1)}. The array must not be
    empty. If the array can be empty, call isEmpty() before calling
    this function.

    \sa removeAt(), removeFirst()
*/

/*!
    Removes the item at index position \a i and returns it. \a i must
    be a valid index position in the array (i.e., \c{0 <= i < size()}).

    If you don't use the return value, removeAt() is more efficient.

    \sa removeAt()
 */
JsonValue JsonArray::takeAt(int i)
{
    if (!a || i < 0 || i >= (int)a->length)
        return JsonValue(JsonValue::Undefined);

    JsonValue v(d, a, a->at(i));
    removeAt(i); // detaches
    return v;
}

/*!
    Inserts \a value at index position \a i in the array. If \a i
    is \c 0, the value is prepended to the array. If \a i is size(), the
    value is appended to the array.

    \sa append(), prepend(), replace(), removeAt()
 */
void JsonArray::insert(int i, const JsonValue &value)
{
    // assert (i >= 0 && i <= (a ? (int)a->length : 0));
    JsonValue val = value;

    bool compressed;
    int valueSize = Internal::Value::requiredStorage(val, &compressed);

    detach(valueSize + sizeof(Internal::Value));

    if (!a->length)
        a->tableOffset = sizeof(Internal::Array);

    int valueOffset = a->reserveSpace(valueSize, i, 1, false);
    if (!valueOffset)
        return;

    Internal::Value &v = (*a)[i];
    v.type = (val.t == JsonValue::Undefined ? JsonValue::Null : val.t);
    v.intValue = compressed;
    v.value = Internal::Value::valueToStore(val, valueOffset);
    if (valueSize)
        Internal::Value::copyData(val, (char *)a + valueOffset, compressed);
}

/*!
    \fn JsonArray::iterator JsonArray::insert(iterator before, const JsonValue &value)

    Inserts \a value before the position pointed to by \a before, and returns an iterator
    pointing to the newly inserted item.

    \sa erase(), insert()
*/

/*!
    \fn JsonArray::iterator JsonArray::erase(iterator it)

    Removes the item pointed to by \a it, and returns an iterator pointing to the
    next item.

    \sa removeAt()
*/

/*!
    Replaces the item at index position \a i with \a value. \a i must
    be a valid index position in the array (i.e., \c{0 <= i < size()}).

    \sa operator[](), removeAt()
 */
void JsonArray::replace(int i, const JsonValue &value)
{
    // assert (a && i >= 0 && i < (int)(a->length));
    JsonValue val = value;

    bool compressed;
    int valueSize = Internal::Value::requiredStorage(val, &compressed);

    detach(valueSize);

    if (!a->length)
        a->tableOffset = sizeof(Internal::Array);

    int valueOffset = a->reserveSpace(valueSize, i, 1, true);
    if (!valueOffset)
        return;

    Internal::Value &v = (*a)[i];
    v.type = (val.t == JsonValue::Undefined ? JsonValue::Null : val.t);
    v.intValue = compressed;
    v.value = Internal::Value::valueToStore(val, valueOffset);
    if (valueSize)
        Internal::Value::copyData(val, (char *)a + valueOffset, compressed);

    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(a->length) / 2u)
        compact();
}

/*!
    Returns \c true if the array contains an occurrence of \a value, otherwise \c false.

    \sa count()
 */
bool JsonArray::contains(const JsonValue &value) const
{
    for (int i = 0; i < size(); i++) {
        if (at(i) == value)
            return true;
    }
    return false;
}

/*!
    Returns the value at index position \a i as a modifiable reference.
    \a i must be a valid index position in the array (i.e., \c{0 <= i <
    size()}).

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the character in the JsonArray of JsonObject
    from which you got the reference.

    \sa at()
 */
JsonValueRef JsonArray::operator[](int i)
{
    // assert(a && i >= 0 && i < (int)a->length);
    return JsonValueRef(this, i);
}

/*!
    \overload

    Same as at().
 */
JsonValue JsonArray::operator[](int i) const
{
    return at(i);
}

/*!
    Returns \c true if this array is equal to \a other.
 */
bool JsonArray::operator==(const JsonArray &other) const
{
    if (a == other.a)
        return true;

    if (!a)
        return !other.a->length;
    if (!other.a)
        return !a->length;
    if (a->length != other.a->length)
        return false;

    for (int i = 0; i < (int)a->length; ++i) {
        if (JsonValue(d, a, a->at(i)) != JsonValue(other.d, other.a, other.a->at(i)))
            return false;
    }
    return true;
}

/*!
    Returns \c true if this array is not equal to \a other.
 */
bool JsonArray::operator!=(const JsonArray &other) const
{
    return !(*this == other);
}

/*! \fn JsonArray::iterator JsonArray::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the array.

    \sa constBegin(), end()
*/

/*! \fn JsonArray::const_iterator JsonArray::begin() const

    \overload
*/

/*! \fn JsonArray::const_iterator JsonArray::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the array.

    \sa begin(), constEnd()
*/

/*! \fn JsonArray::iterator JsonArray::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the array.

    \sa begin(), constEnd()
*/

/*! \fn const_iterator JsonArray::end() const

    \overload
*/

/*! \fn JsonArray::const_iterator JsonArray::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the array.

    \sa constBegin(), end()
*/

/*! \fn void JsonArray::push_back(const JsonValue &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{JsonArray::append()}{append(value)} and will append \a value to the array.
*/

/*! \fn void JsonArray::push_front(const JsonValue &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{JsonArray::prepend()}{prepend(value)} and will prepend \a value to the array.
*/

/*! \fn void JsonArray::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst(). The array must not be empty. If the array can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn void JsonArray::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast(). The array must not be empty. If the array can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn bool JsonArray::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns \c true if the array is empty.
*/

/*! \class JsonArray::iterator
    \inmodule QtCore
    \brief The JsonArray::iterator class provides an STL-style non-const iterator for JsonArray.

    JsonArray::iterator allows you to iterate over a JsonArray
    and to modify the array item associated with the
    iterator. If you want to iterate over a const JsonArray, use
    JsonArray::const_iterator instead. It is generally a good practice to
    use JsonArray::const_iterator on a non-const JsonArray as well, unless
    you need to change the JsonArray through the iterator. Const
    iterators are slightly faster and improves code readability.

    The default JsonArray::iterator constructor creates an uninitialized
    iterator. You must initialize it using a JsonArray function like
    JsonArray::begin(), JsonArray::end(), or JsonArray::insert() before you can
    start iterating.

    Most JsonArray functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with JsonArray. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be
    aware that any non-const function call performed on the JsonArray
    will render all existing iterators undefined.

    \sa JsonArray::const_iterator
*/

/*! \typedef JsonArray::iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef JsonArray::iterator::difference_type

    \internal
*/

/*! \typedef JsonArray::iterator::value_type

    \internal
*/

/*! \typedef JsonArray::iterator::reference

    \internal
*/

/*! \typedef JsonArray::iterator::pointer

    \internal
*/

/*! \fn JsonArray::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa JsonArray::begin(), JsonArray::end()
*/

/*! \fn JsonArray::iterator::iterator(JsonArray *array, int index)
    \internal
*/

/*! \fn JsonValueRef JsonArray::iterator::operator*() const


    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment.

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the character in the JsonArray of JsonObject
    from which you got the reference.
*/

/*! \fn JsonValueRef *JsonArray::iterator::operator->() const

    Returns a pointer to a modifiable reference to the current item.
*/

/*! \fn JsonValueRef JsonArray::iterator::operator[](int j) const

    Returns a modifiable reference to the item at offset \a j from the
    item pointed to by this iterator (the item at position \c{*this + j}).

    This function is provided to make JsonArray iterators behave like C++
    pointers.

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the character in the JsonArray of JsonObject
    from which you got the reference.

    \sa operator+()
*/

/*!
    \fn bool JsonArray::iterator::operator==(const iterator &other) const
    \fn bool JsonArray::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool JsonArray::iterator::operator!=(const iterator &other) const
    \fn bool JsonArray::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool JsonArray::iterator::operator<(const iterator& other) const
    \fn bool JsonArray::iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::iterator::operator<=(const iterator& other) const
    \fn bool JsonArray::iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::iterator::operator>(const iterator& other) const
    \fn bool JsonArray::iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::iterator::operator>=(const iterator& other) const
    \fn bool JsonArray::iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn JsonArray::iterator &JsonArray::iterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the
    next item in the array and returns an iterator to the new current
    item.

    Calling this function on JsonArray::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn JsonArray::iterator JsonArray::iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the
    next item in the array and returns an iterator to the previously
    current item.
*/

/*! \fn JsonArray::iterator &JsonArray::iterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on JsonArray::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn JsonArray::iterator JsonArray::iterator::operator--(int)

    \overload

    The postfix -- operator, \c{it--}, makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn JsonArray::iterator &JsonArray::iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn JsonArray::iterator &JsonArray::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*! \fn JsonArray::iterator JsonArray::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*! \fn JsonArray::iterator JsonArray::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*! \fn int JsonArray::iterator::operator-(iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \class JsonArray::const_iterator
    \inmodule QtCore
    \brief The JsonArray::const_iterator class provides an STL-style const iterator for JsonArray.

    JsonArray::const_iterator allows you to iterate over a
    JsonArray. If you want to modify the JsonArray as
    you iterate over it, use JsonArray::iterator instead. It is generally a
    good practice to use JsonArray::const_iterator on a non-const JsonArray
    as well, unless you need to change the JsonArray through the
    iterator. Const iterators are slightly faster and improves
    code readability.

    The default JsonArray::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a JsonArray
    function like JsonArray::constBegin(), JsonArray::constEnd(), or
    JsonArray::insert() before you can start iterating.

    Most JsonArray functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with JsonArray. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    Multiple iterators can be used on the same array. However, be
    aware that any non-const function call performed on the JsonArray
    will render all existing iterators undefined.

    \sa JsonArray::iterator
*/

/*! \fn JsonArray::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa JsonArray::constBegin(), JsonArray::constEnd()
*/

/*! \fn JsonArray::const_iterator::const_iterator(const JsonArray *array, int index)
    \internal
*/

/*! \typedef JsonArray::const_iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef JsonArray::const_iterator::difference_type

    \internal
*/

/*! \typedef JsonArray::const_iterator::value_type

    \internal
*/

/*! \typedef JsonArray::const_iterator::reference

    \internal
*/

/*! \typedef JsonArray::const_iterator::pointer

    \internal
*/

/*! \fn JsonArray::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn JsonArray::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn JsonValue JsonArray::const_iterator::operator*() const

    Returns the current item.
*/

/*! \fn JsonValue *JsonArray::const_iterator::operator->() const

    Returns a pointer to the current item.
*/

/*! \fn JsonValue JsonArray::const_iterator::operator[](int j) const

    Returns the item at offset \a j from the item pointed to by this iterator (the item at
    position \c{*this + j}).

    This function is provided to make JsonArray iterators behave like C++
    pointers.

    \sa operator+()
*/

/*! \fn bool JsonArray::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn bool JsonArray::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn bool JsonArray::const_iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::const_iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::const_iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool JsonArray::const_iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn JsonArray::const_iterator &JsonArray::const_iterator::operator++()

    The prefix ++ operator, \c{++it}, advances the iterator to the
    next item in the array and returns an iterator to the new current
    item.

    Calling this function on JsonArray::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn JsonArray::const_iterator JsonArray::const_iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{it++}, advances the iterator to the
    next item in the array and returns an iterator to the previously
    current item.
*/

/*! \fn JsonArray::const_iterator &JsonArray::const_iterator::operator--()

    The prefix -- operator, \c{--it}, makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on JsonArray::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn JsonArray::const_iterator JsonArray::const_iterator::operator--(int)

    \overload

    The postfix -- operator, \c{it--}, makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn JsonArray::const_iterator &JsonArray::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn JsonArray::const_iterator &JsonArray::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*! \fn JsonArray::const_iterator JsonArray::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*! \fn JsonArray::const_iterator JsonArray::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*! \fn int JsonArray::const_iterator::operator-(const_iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/


/*!
    \internal
 */
void JsonArray::detach(uint32_t reserve)
{
    if (!d) {
        d = new Internal::Data(reserve, JsonValue::Array);
        a = static_cast<Internal::Array *>(d->header->root());
        d->ref.ref();
        return;
    }
    if (reserve == 0 && d->ref.load() == 1)
        return;

    Internal::Data *x = d->clone(a, reserve);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    a = static_cast<Internal::Array *>(d->header->root());
}

/*!
    \internal
 */
void JsonArray::compact()
{
    if (!d || !d->compactionCounter)
        return;

    detach();
    d->compact();
    a = static_cast<Internal::Array *>(d->header->root());
}

/*!
    \class JsonObject
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The JsonObject class encapsulates a JSON object.

    A JSON object is a list of key value pairs, where the keys are unique strings
    and the values are represented by a JsonValue.

    A JsonObject can be converted to and from a QVariantMap. You can query the
    number of (key, value) pairs with size(), insert(), and remove() entries from it
    and iterate over its content using the standard C++ iterator pattern.

    JsonObject is an implicitly shared class, and shares the data with the document
    it has been created from as long as it is not being modified.

    You can convert the object to and from text based JSON through JsonDocument.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \typedef JsonObject::Iterator

    Qt-style synonym for JsonObject::iterator.
*/

/*!
    \typedef JsonObject::ConstIterator

    Qt-style synonym for JsonObject::const_iterator.
*/

/*!
    \typedef JsonObject::key_type

    Typedef for QString. Provided for STL compatibility.
*/

/*!
    \typedef JsonObject::mapped_type

    Typedef for JsonValue. Provided for STL compatibility.
*/

/*!
    \typedef JsonObject::size_type

    Typedef for int. Provided for STL compatibility.
*/


/*!
    Constructs an empty JSON object.

    \sa isEmpty()
 */
JsonObject::JsonObject()
    : d(0), o(0)
{
}

JsonObject::JsonObject(std::initializer_list<std::pair<std::string, JsonValue> > args)
    : d(0), o(0)
{
    for (auto i = args.begin(); i != args.end(); ++i)
        insert(i->first, i->second);
}

/*!
    \fn JsonObject::JsonObject(std::initializer_list<QPair<QString, JsonValue> > args)
    \since 5.4
    Constructs a JsonObject instance initialized from \a args initialization list.
    For example:
    \code
    JsonObject object
    {
        {"property1", 1},
        {"property2", 2}
    };
    \endcode
*/

/*!
    \internal
 */
JsonObject::JsonObject(Internal::Data *data, Internal::Object *object)
    : d(data), o(object)
{
    // assert(d);
    // assert(o);
    d->ref.ref();
}

/*!
    This method replaces part of the JsonObject(std::initializer_list<QPair<QString, JsonValue>> args) body.
    The constructor needs to be inline, but we do not want to leak implementation details
    of this class.
    \note this method is called for an uninitialized object
    \internal
 */

/*!
    Destroys the object.
 */
JsonObject::~JsonObject()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Creates a copy of \a other.

    Since JsonObject is implicitly shared, the copy is shallow
    as long as the object does not get modified.
 */
JsonObject::JsonObject(const JsonObject &other)
{
    d = other.d;
    o = other.o;
    if (d)
        d->ref.ref();
}

/*!
    Assigns \a other to this object.
 */
JsonObject &JsonObject::operator=(const JsonObject &other)
{
    if (d != other.d) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();
    }
    o = other.o;

    return *this;
}

/*!
    Returns a list of all keys in this object.

    The list is sorted lexographically.
 */
JsonObject::Keys JsonObject::keys() const
{
    Keys keys;
    if (!d)
        return keys;

    keys.reserve(o->length);
    for (uint32_t i = 0; i < o->length; ++i) {
        Internal::Entry *e = o->entryAt(i);
        keys.push_back(e->key().data());
    }

    return keys;
}

/*!
    Returns the number of (key, value) pairs stored in the object.
 */
int JsonObject::size() const
{
    if (!d)
        return 0;

    return o->length;
}

/*!
    Returns \c true if the object is empty. This is the same as size() == 0.

    \sa size()
 */
bool JsonObject::isEmpty() const
{
    if (!d)
        return true;

    return !o->length;
}

/*!
    Returns a JsonValue representing the value for the key \a key.

    The returned JsonValue is JsonValue::Undefined if the key does not exist.

    \sa JsonValue, JsonValue::isUndefined()
 */
JsonValue JsonObject::value(const std::string &key) const
{
    if (!d)
        return JsonValue(JsonValue::Undefined);

    bool keyExists;
    int i = o->indexOf(key, &keyExists);
    if (!keyExists)
        return JsonValue(JsonValue::Undefined);
    return JsonValue(d, o, o->entryAt(i)->value);
}

/*!
    Returns a JsonValue representing the value for the key \a key.

    This does the same as value().

    The returned JsonValue is JsonValue::Undefined if the key does not exist.

    \sa value(), JsonValue, JsonValue::isUndefined()
 */
JsonValue JsonObject::operator[](const std::string &key) const
{
    return value(key);
}

/*!
    Returns a reference to the value for \a key.

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the element in the JsonArray or JsonObject
    from which you got the reference.

    \sa value()
 */
JsonValueRef JsonObject::operator[](const std::string &key)
{
    // ### somewhat inefficient, as we lookup the key twice if it doesn't yet exist
    bool keyExists = false;
    int index = o ? o->indexOf(key, &keyExists) : -1;
    if (!keyExists) {
        iterator i = insert(key, JsonValue());
        index = i.i;
    }
    return JsonValueRef(this, index);
}

/*!
    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the key \a key, then that item's value
    is replaced with \a value.

    Returns an iterator pointing to the inserted item.

    If the value is JsonValue::Undefined, it will cause the key to get removed
    from the object. The returned iterator will then point to end().

    \sa remove(), take(), JsonObject::iterator, end()
 */
JsonObject::iterator JsonObject::insert(const std::string &key, const JsonValue &value)
{
    if (value.t == JsonValue::Undefined) {
        remove(key);
        return end();
    }
    JsonValue val = value;

    bool isIntValue;
    int valueSize = Internal::Value::requiredStorage(val, &isIntValue);

    int valueOffset = sizeof(Internal::Entry) + Internal::qStringSize(key);
    int requiredSize = valueOffset + valueSize;

    detach(requiredSize + sizeof(Internal::offset)); // offset for the new index entry

    if (!o->length)
        o->tableOffset = sizeof(Internal::Object);

    bool keyExists = false;
    int pos = o->indexOf(key, &keyExists);
    if (keyExists)
        ++d->compactionCounter;

    uint32_t off = o->reserveSpace(requiredSize, pos, 1, keyExists);
    if (!off)
        return end();

    Internal::Entry *e = o->entryAt(pos);
    e->value.type = val.t;
    e->value.intValue = isIntValue;
    e->value.value = Internal::Value::valueToStore(val, static_cast<uint32_t>((char *)e - (char *)o)
                                                   + valueOffset);
    Internal::copyString((char *)(e + 1), key);
    if (valueSize)
        Internal::Value::copyData(val, (char *)e + valueOffset, isIntValue);

    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(o->length) / 2u)
        compact();

    return iterator(this, pos);
}

/*!
    Removes \a key from the object.

    \sa insert(), take()
 */
void JsonObject::remove(const std::string &key)
{
    if (!d)
        return;

    bool keyExists;
    int index = o->indexOf(key, &keyExists);
    if (!keyExists)
        return;

    detach();
    o->removeItems(index, 1);
    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(o->length) / 2u)
        compact();
}

/*!
    Removes \a key from the object.

    Returns a JsonValue containing the value referenced by \a key.
    If \a key was not contained in the object, the returned JsonValue
    is JsonValue::Undefined.

    \sa insert(), remove(), JsonValue
 */
JsonValue JsonObject::take(const std::string &key)
{
    if (!o)
        return JsonValue(JsonValue::Undefined);

    bool keyExists;
    int index = o->indexOf(key, &keyExists);
    if (!keyExists)
        return JsonValue(JsonValue::Undefined);

    JsonValue v(d, o, o->entryAt(index)->value);
    detach();
    o->removeItems(index, 1);
    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(o->length) / 2u)
        compact();

    return v;
}

/*!
    Returns \c true if the object contains key \a key.

    \sa insert(), remove(), take()
 */
bool JsonObject::contains(const std::string &key) const
{
    if (!o)
        return false;

    bool keyExists;
    o->indexOf(key, &keyExists);
    return keyExists;
}

/*!
    Returns \c true if \a other is equal to this object.
 */
bool JsonObject::operator==(const JsonObject &other) const
{
    if (o == other.o)
        return true;

    if (!o)
        return !other.o->length;
    if (!other.o)
        return !o->length;
    if (o->length != other.o->length)
        return false;

    for (uint32_t i = 0; i < o->length; ++i) {
        Internal::Entry *e = o->entryAt(i);
        JsonValue v(d, o, e->value);
        if (other.value(e->key()) != v)
            return false;
    }

    return true;
}

/*!
    Returns \c true if \a other is not equal to this object.
 */
bool JsonObject::operator!=(const JsonObject &other) const
{
    return !(*this == other);
}

/*!
    Removes the (key, value) pair pointed to by the iterator \a it
    from the map, and returns an iterator to the next item in the
    map.

    \sa remove()
 */
JsonObject::iterator JsonObject::erase(JsonObject::iterator it)
{
    // assert(d && d->ref.load() == 1);
    if (it.o != this || it.i < 0 || it.i >= (int)o->length)
        return iterator(this, o->length);

    int index = it.i;

    o->removeItems(index, 1);
    ++d->compactionCounter;
    if (d->compactionCounter > 32u && d->compactionCounter >= unsigned(o->length) / 2u)
        compact();

    // iterator hasn't changed
    return it;
}

/*!
    Returns an iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns end().
 */
JsonObject::iterator JsonObject::find(const std::string &key)
{
    bool keyExists = false;
    int index = o ? o->indexOf(key, &keyExists) : 0;
    if (!keyExists)
        return end();
    detach();
    return iterator(this, index);
}

/*! \fn JsonObject::const_iterator JsonObject::find(const QString &key) const

    \overload
*/

/*!
    Returns a const iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns constEnd().
 */
JsonObject::const_iterator JsonObject::constFind(const std::string &key) const
{
    bool keyExists = false;
    int index = o ? o->indexOf(key, &keyExists) : 0;
    if (!keyExists)
        return end();
    return const_iterator(this, index);
}

/*! \fn int JsonObject::count() const

    \overload

    Same as size().
*/

/*! \fn int JsonObject::length() const

    \overload

    Same as size().
*/

/*! \fn JsonObject::iterator JsonObject::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the object.

    \sa constBegin(), end()
*/

/*! \fn JsonObject::const_iterator JsonObject::begin() const

    \overload
*/

/*! \fn JsonObject::const_iterator JsonObject::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the object.

    \sa begin(), constEnd()
*/

/*! \fn JsonObject::iterator JsonObject::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the object.

    \sa begin(), constEnd()
*/

/*! \fn JsonObject::const_iterator JsonObject::end() const

    \overload
*/

/*! \fn JsonObject::const_iterator JsonObject::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the object.

    \sa constBegin(), end()
*/

/*!
    \fn bool JsonObject::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty(), returning \c true if the object is empty; otherwise
    returning \c false.
*/

/*! \class JsonObject::iterator
    \inmodule QtCore
    \ingroup json
    \reentrant
    \since 5.0

    \brief The JsonObject::iterator class provides an STL-style non-const iterator for JsonObject.

    JsonObject::iterator allows you to iterate over a JsonObject
    and to modify the value (but not the key) stored under
    a particular key. If you want to iterate over a const JsonObject, you
    should use JsonObject::const_iterator. It is generally good practice to
    use JsonObject::const_iterator on a non-const JsonObject as well, unless you
    need to change the JsonObject through the iterator. Const iterators are
    slightly faster, and improve code readability.

    The default JsonObject::iterator constructor creates an uninitialized
    iterator. You must initialize it using a JsonObject function like
    JsonObject::begin(), JsonObject::end(), or JsonObject::find() before you can
    start iterating.

    Multiple iterators can be used on the same object. Existing iterators will however
    become dangling once the object gets modified.

    \sa JsonObject::const_iterator, {JSON Support in Qt}, {JSON Save Game Example}
*/

/*! \typedef JsonObject::iterator::difference_type

    \internal
*/

/*! \typedef JsonObject::iterator::iterator_category

    A synonym for \e {std::bidirectional_iterator_tag} indicating
    this iterator is a bidirectional iterator.
*/

/*! \typedef JsonObject::iterator::reference

    \internal
*/

/*! \typedef JsonObject::iterator::value_type

    \internal
*/

/*! \fn JsonObject::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa JsonObject::begin(), JsonObject::end()
*/

/*! \fn JsonObject::iterator::iterator(JsonObject *obj, int index)
    \internal
*/

/*! \fn QString JsonObject::iterator::key() const

    Returns the current item's key.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling JsonObject::erase()
    followed by JsonObject::insert().

    \sa value()
*/

/*! \fn JsonValueRef JsonObject::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment.

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the element in the JsonArray or JsonObject
    from which you got the reference.

    \sa key(), operator*()
*/

/*! \fn JsonValueRef JsonObject::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    The return value is of type JsonValueRef, a helper class for JsonArray
    and JsonObject. When you get an object of type JsonValueRef, you can
    use it as if it were a reference to a JsonValue. If you assign to it,
    the assignment will apply to the element in the JsonArray or JsonObject
    from which you got the reference.

    \sa key()
*/

/*! \fn JsonValueRef *JsonObject::iterator::operator->() const

    Returns a pointer to a modifiable reference to the current item.
*/

/*!
    \fn bool JsonObject::iterator::operator==(const iterator &other) const
    \fn bool JsonObject::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool JsonObject::iterator::operator!=(const iterator &other) const
    \fn bool JsonObject::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator++()

    The prefix ++ operator, \c{++i}, advances the iterator to the
    next item in the object and returns an iterator to the new current
    item.

    Calling this function on JsonObject::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{i++}, advances the iterator to the
    next item in the object and returns an iterator to the previously
    current item.
*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator--()

    The prefix -- operator, \c{--i}, makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on JsonObject::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator--(int)

    \overload

    The postfix -- operator, \c{i--}, makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    \sa operator-()

*/

/*! \fn JsonObject::iterator JsonObject::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    \sa operator+()
*/

/*! \fn JsonObject::iterator &JsonObject::iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    \sa operator-=(), operator+()
*/

/*! \fn JsonObject::iterator &JsonObject::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*!
    \class JsonObject::const_iterator
    \inmodule QtCore
    \ingroup json
    \since 5.0
    \brief The JsonObject::const_iterator class provides an STL-style const iterator for JsonObject.

    JsonObject::const_iterator allows you to iterate over a JsonObject.
    If you want to modify the JsonObject as you iterate
    over it, you must use JsonObject::iterator instead. It is generally
    good practice to use JsonObject::const_iterator on a non-const JsonObject as
    well, unless you need to change the JsonObject through the iterator.
    Const iterators are slightly faster and improve code
    readability.

    The default JsonObject::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a JsonObject
    function like JsonObject::constBegin(), JsonObject::constEnd(), or
    JsonObject::find() before you can start iterating.

    Multiple iterators can be used on the same object. Existing iterators
    will however become dangling if the object gets modified.

    \sa JsonObject::iterator, {JSON Support in Qt}, {JSON Save Game Example}
*/

/*! \typedef JsonObject::const_iterator::difference_type

    \internal
*/

/*! \typedef JsonObject::const_iterator::iterator_category

    A synonym for \e {std::bidirectional_iterator_tag} indicating
    this iterator is a bidirectional iterator.
*/

/*! \typedef JsonObject::const_iterator::reference

    \internal
*/

/*! \typedef JsonObject::const_iterator::value_type

    \internal
*/

/*! \fn JsonObject::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa JsonObject::constBegin(), JsonObject::constEnd()
*/

/*! \fn JsonObject::const_iterator::const_iterator(const JsonObject *obj, int index)
    \internal
*/

/*! \fn JsonObject::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QString JsonObject::const_iterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*! \fn JsonValue JsonObject::const_iterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*! \fn JsonValue JsonObject::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn JsonValue *JsonObject::const_iterator::operator->() const

    Returns a pointer to the current item.
*/

/*! \fn bool JsonObject::const_iterator::operator==(const const_iterator &other) const
    \fn bool JsonObject::const_iterator::operator==(const iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn bool JsonObject::const_iterator::operator!=(const const_iterator &other) const
    \fn bool JsonObject::const_iterator::operator!=(const iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn JsonObject::const_iterator JsonObject::const_iterator::operator++()

    The prefix ++ operator, \c{++i}, advances the iterator to the
    next item in the object and returns an iterator to the new current
    item.

    Calling this function on JsonObject::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn JsonObject::const_iterator JsonObject::const_iterator::operator++(int)

    \overload

    The postfix ++ operator, \c{i++}, advances the iterator to the
    next item in the object and returns an iterator to the previously
    current item.
*/

/*! \fn JsonObject::const_iterator &JsonObject::const_iterator::operator--()

    The prefix -- operator, \c{--i}, makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on JsonObject::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn JsonObject::const_iterator JsonObject::const_iterator::operator--(int)

    \overload

    The postfix -- operator, \c{i--}, makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn JsonObject::const_iterator JsonObject::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. If \a j is negative, the iterator goes backward.

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn JsonObject::const_iterator JsonObject::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. If \a j is negative, the iterator goes forward.

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn JsonObject::const_iterator &JsonObject::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. If \a j is negative, the
    iterator goes backward.

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn JsonObject::const_iterator &JsonObject::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. If \a j is negative,
    the iterator goes forward.

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/


/*!
    \internal
 */
void JsonObject::detach(uint32_t reserve)
{
    if (!d) {
        d = new Internal::Data(reserve, JsonValue::Object);
        o = static_cast<Internal::Object *>(d->header->root());
        d->ref.ref();
        return;
    }
    if (reserve == 0 && d->ref.load() == 1)
        return;

    Internal::Data *x = d->clone(o, reserve);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    o = static_cast<Internal::Object *>(d->header->root());
}

/*!
    \internal
 */
void JsonObject::compact()
{
    if (!d || !d->compactionCounter)
        return;

    detach();
    d->compact();
    o = static_cast<Internal::Object *>(d->header->root());
}

/*!
    \internal
 */
std::string JsonObject::keyAt(int i) const
{
    // assert(o && i >= 0 && i < (int)o->length);

    Internal::Entry *e = o->entryAt(i);
    return e->key();
}

/*!
    \internal
 */
JsonValue JsonObject::valueAt(int i) const
{
    if (!o || i < 0 || i >= (int)o->length)
        return JsonValue(JsonValue::Undefined);

    Internal::Entry *e = o->entryAt(i);
    return JsonValue(d, o, e->value);
}

/*!
    \internal
 */
void JsonObject::setValueAt(int i, const JsonValue &val)
{
    // assert(o && i >= 0 && i < (int)o->length);

    Internal::Entry *e = o->entryAt(i);
    insert(e->key(), val);
}


/*! \class JsonDocument
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The JsonDocument class provides a way to read and write JSON documents.

    JsonDocument is a class that wraps a complete JSON document and can read and
    write this document both from a UTF-8 encoded text based representation as well
    as Qt's own binary format.

    A JSON document can be converted from its text-based representation to a JsonDocument
    using JsonDocument::fromJson(). toJson() converts it back to text. The parser is very
    fast and efficient and converts the JSON to the binary representation used by Qt.

    Validity of the parsed document can be queried with !isNull()

    A document can be queried as to whether it contains an array or an object using isArray()
    and isObject(). The array or object contained in the document can be retrieved using
    array() or object() and then read or manipulated.

    A document can also be created from a stored binary representation using fromBinaryData() or
    fromRawData().

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
 * Constructs an empty and invalid document.
 */
JsonDocument::JsonDocument()
    : d(0)
{
}

/*!
 * Creates a JsonDocument from \a object.
 */
JsonDocument::JsonDocument(const JsonObject &object)
    : d(0)
{
    setObject(object);
}

/*!
 * Constructs a JsonDocument from \a array.
 */
JsonDocument::JsonDocument(const JsonArray &array)
    : d(0)
{
    setArray(array);
}

/*!
    \internal
 */
JsonDocument::JsonDocument(Internal::Data *data)
    : d(data)
{
    // assert(d);
    d->ref.ref();
}

/*!
 Deletes the document.

 Binary data set with fromRawData is not freed.
 */
JsonDocument::~JsonDocument()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
 * Creates a copy of the \a other document.
 */
JsonDocument::JsonDocument(const JsonDocument &other)
{
    d = other.d;
    if (d)
        d->ref.ref();
}

/*!
 * Assigns the \a other document to this JsonDocument.
 * Returns a reference to this object.
 */
JsonDocument &JsonDocument::operator=(const JsonDocument &other)
{
    if (d != other.d) {
        if (d && !d->ref.deref())
            delete d;
        d = other.d;
        if (d)
            d->ref.ref();
    }

    return *this;
}

/*! \enum JsonDocument::DataValidation

  This value is used to tell JsonDocument whether to validate the binary data
  when converting to a JsonDocument using fromBinaryData() or fromRawData().

  \value Validate Validate the data before using it. This is the default.
  \value BypassValidation Bypasses data validation. Only use if you received the
  data from a trusted place and know it's valid, as using of invalid data can crash
  the application.
  */

/*!
 Creates a JsonDocument that uses the first \a size bytes from
 \a data. It assumes \a data contains a binary encoded JSON document.
 The created document does not take ownership of \a data and the caller
 has to guarantee that \a data will not be deleted or modified as long as
 any JsonDocument, JsonObject or JsonArray still references the data.

 \a data has to be aligned to a 4 byte boundary.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 Returns a JsonDocument representing the data.

 \sa rawData(), fromBinaryData(), isNull(), DataValidation
 */
JsonDocument JsonDocument::fromRawData(const char *data, int size, DataValidation validation)
{
    if (std::uintptr_t(data) & 3) {
        std::cerr <<"JsonDocument::fromRawData: data has to have 4 byte alignment\n";
        return JsonDocument();
    }

    Internal::Data *d = new Internal::Data((char *)data, size);
    d->ownsData = false;

    if (validation != BypassValidation && !d->valid()) {
        delete d;
        return JsonDocument();
    }

    return JsonDocument(d);
}

/*!
  Returns the raw binary representation of the data
  \a size will contain the size of the returned data.

  This method is useful to e.g. stream the JSON document
  in it's binary form to a file.
 */
const char *JsonDocument::rawData(int *size) const
{
    if (!d) {
        *size = 0;
        return 0;
    }
    *size = d->alloc;
    return d->rawData;
}

/*!
 Creates a JsonDocument from \a data.

 \a validation decides whether the data is checked for validity before being used.
 By default the data is validated. If the \a data is not valid, the method returns
 a null document.

 \sa toBinaryData(), fromRawData(), isNull(), DataValidation
 */
JsonDocument JsonDocument::fromBinaryData(const std::string &data, DataValidation validation)
{
    if (data.size() < (int)(sizeof(Internal::Header) + sizeof(Internal::Base)))
        return JsonDocument();

    Internal::Header h;
    memcpy(&h, data.data(), sizeof(Internal::Header));
    Internal::Base root;
    memcpy(&root, data.data() + sizeof(Internal::Header), sizeof(Internal::Base));

    // do basic checks here, so we don't try to allocate more memory than we can.
    if (h.tag != JsonDocument::BinaryFormatTag || h.version != 1u ||
        sizeof(Internal::Header) + root.size > (uint32_t)data.size())
        return JsonDocument();

    const uint32_t size = sizeof(Internal::Header) + root.size;
    char *raw = (char *)malloc(size);
    if (!raw)
        return JsonDocument();

    memcpy(raw, data.data(), size);
    Internal::Data *d = new Internal::Data(raw, size);

    if (validation != BypassValidation && !d->valid()) {
        delete d;
        return JsonDocument();
    }

    return JsonDocument(d);
}

/*!
    \enum JsonDocument::JsonFormat

    This value defines the format of the JSON byte array produced
    when converting to a JsonDocument using toJson().

    \value Indented Defines human readable output as follows:
        \code
        {
            "Array": [
                true,
                999,
                "string"
            ],
            "Key": "Value",
            "null": null
        }
        \endcode

    \value Compact Defines a compact output as follows:
        \code
        {"Array":[true,999,"string"],"Key":"Value","null":null}
        \endcode
  */

/*!
    Converts the JsonDocument to a UTF-8 encoded JSON document in the provided \a format.

    \sa fromJson(), JsonFormat
 */
#ifndef QT_JSON_READONLY
std::string JsonDocument::toJson(JsonFormat format) const
{
    std::string json;

    if (!d)
        return json;

    if (d->header->root()->isArray())
        Internal::arrayToJson(static_cast<Internal::Array *>(d->header->root()), json, 0, (format == Compact));
    else
        Internal::objectToJson(static_cast<Internal::Object *>(d->header->root()), json, 0, (format == Compact));

    return json;
}
#endif

/*!
 Parses a UTF-8 encoded JSON document and creates a JsonDocument
 from it.

 \a json contains the json document to be parsed.

 The optional \a error variable can be used to pass in a JsonParseError data
 structure that will contain information about possible errors encountered during
 parsing.

 \sa toJson(), JsonParseError
 */
JsonDocument JsonDocument::fromJson(const std::string &json, JsonParseError *error)
{
    Internal::Parser parser(json.data(), static_cast<int>(json.length()));
    return parser.parse(error);
}

/*!
    Returns \c true if the document doesn't contain any data.
 */
bool JsonDocument::isEmpty() const
{
    if (!d)
        return true;

    return false;
}

/*!
 Returns a binary representation of the document.

 The binary representation is also the native format used internally in Qt,
 and is very efficient and fast to convert to and from.

 The binary format can be stored on disk and interchanged with other applications
 or computers. fromBinaryData() can be used to convert it back into a
 JSON document.

 \sa fromBinaryData()
 */
std::string JsonDocument::toBinaryData() const
{
    if (!d || !d->rawData)
        return std::string();

    return std::string(d->rawData, d->header->root()->size + sizeof(Internal::Header));
}

/*!
    Returns \c true if the document contains an array.

    \sa array(), isObject()
 */
bool JsonDocument::isArray() const
{
    if (!d)
        return false;

    Internal::Header *h = (Internal::Header *)d->rawData;
    return h->root()->isArray();
}

/*!
    Returns \c true if the document contains an object.

    \sa object(), isArray()
 */
bool JsonDocument::isObject() const
{
    if (!d)
        return false;

    Internal::Header *h = (Internal::Header *)d->rawData;
    return h->root()->isObject();
}

/*!
    Returns the JsonObject contained in the document.

    Returns an empty object if the document contains an
    array.

    \sa isObject(), array(), setObject()
 */
JsonObject JsonDocument::object() const
{
    if (d) {
        Internal::Base *b = d->header->root();
        if (b->isObject())
            return JsonObject(d, static_cast<Internal::Object *>(b));
    }
    return JsonObject();
}

/*!
    Returns the JsonArray contained in the document.

    Returns an empty array if the document contains an
    object.

    \sa isArray(), object(), setArray()
 */
JsonArray JsonDocument::array() const
{
    if (d) {
        Internal::Base *b = d->header->root();
        if (b->isArray())
            return JsonArray(d, static_cast<Internal::Array *>(b));
    }
    return JsonArray();
}

/*!
    Sets \a object as the main object of this document.

    \sa setArray(), object()
 */
void JsonDocument::setObject(const JsonObject &object)
{
    if (d && !d->ref.deref())
        delete d;

    d = object.d;

    if (!d) {
        d = new Internal::Data(0, JsonValue::Object);
    } else if (d->compactionCounter || object.o != d->header->root()) {
        JsonObject o(object);
        if (d->compactionCounter)
            o.compact();
        else
            o.detach();
        d = o.d;
        d->ref.ref();
        return;
    }
    d->ref.ref();
}

/*!
    Sets \a array as the main object of this document.

    \sa setObject(), array()
 */
void JsonDocument::setArray(const JsonArray &array)
{
    if (d && !d->ref.deref())
        delete d;

    d = array.d;

    if (!d) {
        d = new Internal::Data(0, JsonValue::Array);
    } else if (d->compactionCounter || array.a != d->header->root()) {
        JsonArray a(array);
        if (d->compactionCounter)
            a.compact();
        else
            a.detach();
        d = a.d;
        d->ref.ref();
        return;
    }
    d->ref.ref();
}

/*!
    Returns \c true if the \a other document is equal to this document.
 */
bool JsonDocument::operator==(const JsonDocument &other) const
{
    if (d == other.d)
        return true;

    if (!d || !other.d)
        return false;

    if (d->header->root()->isArray() != other.d->header->root()->isArray())
        return false;

    if (d->header->root()->isObject())
        return JsonObject(d, static_cast<Internal::Object *>(d->header->root()))
                == JsonObject(other.d, static_cast<Internal::Object *>(other.d->header->root()));
    else
        return JsonArray(d, static_cast<Internal::Array *>(d->header->root()))
                == JsonArray(other.d, static_cast<Internal::Array *>(other.d->header->root()));
}

/*!
 \fn bool JsonDocument::operator!=(const JsonDocument &other) const

    returns \c true if \a other is not equal to this document
 */

/*!
    returns \c true if this document is null.

    Null documents are documents created through the default constructor.

    Documents created from UTF-8 encoded text or the binary format are
    validated during parsing. If validation fails, the returned document
    will also be null.
 */
bool JsonDocument::isNull() const
{
    return (d == 0);
}


static void objectContentToJson(const Object *o, std::string &json, int indent, bool compact);
static void arrayContentToJson(const Array *a, std::string &json, int indent, bool compact);

static uint8_t hexdig(uint32_t u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

static std::string escapedString(const std::string &in)
{
    std::string ba;
    ba.reserve(in.length());

    auto src = in.begin();
    auto end = in.end();

    while (src != end) {
        uint8_t u = (*src++);
        if (u < 0x20 || u == 0x22 || u == 0x5c) {
            ba.push_back('\\');
            switch (u) {
            case 0x22:
                ba.push_back('"');
                break;
            case 0x5c:
                ba.push_back('\\');
                break;
            case 0x8:
                ba.push_back('b');
                break;
            case 0xc:
                ba.push_back('f');
                break;
            case 0xa:
                ba.push_back('n');
                break;
            case 0xd:
                ba.push_back('r');
                break;
            case 0x9:
                ba.push_back('t');
                break;
            default:
                ba.push_back('u');
                ba.push_back('0');
                ba.push_back('0');
                ba.push_back(hexdig(u>>4));
                ba.push_back(hexdig(u & 0xf));
           }
        } else {
            ba.push_back(u);
        }
    }

    return ba;
}

static void valueToJson(const Base *b, const Value &v, std::string &json, int indent, bool compact)
{
    JsonValue::Type type = (JsonValue::Type)(uint32_t)v.type;
    switch (type) {
    case JsonValue::Bool:
        json += v.toBoolean() ? "true" : "false";
        break;
    case JsonValue::Double: {
        const double d = v.toDouble(b);
        if (std::isfinite(d)) {
            // +2 to format to ensure the expected precision
            const int n = std::numeric_limits<double>::digits10 + 2;
            char buf[30] = {0};
            sprintf(buf, "%.*g", n, d);
            // Hack:
            if (buf[0] == '-' && buf[1] == '0' && buf[2] == '\0')
                json += "0";
            else
                json += buf;
        } else {
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        }
        break;
    }
    case JsonValue::String:
        json += '"';
        json += escapedString(v.toString(b));
        json += '"';
        break;
    case JsonValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(static_cast<Array *>(v.base(b)), json, indent + (compact ? 0 : 1), compact);
        json += std::string(4*indent, ' ');
        json += ']';
        break;
    case JsonValue::Object:
        json += compact ? "{" : "{\n";
        objectContentToJson(static_cast<Object *>(v.base(b)), json, indent + (compact ? 0 : 1), compact);
        json += std::string(4*indent, ' ');
        json += '}';
        break;
    case JsonValue::Null:
    default:
        json += "null";
    }
}

static void arrayContentToJson(const Array *a, std::string &json, int indent, bool compact)
{
    if (!a || !a->length)
        return;

    std::string indentString(4*indent, ' ');

    uint32_t i = 0;
    while (1) {
        json += indentString;
        valueToJson(a, a->at(i), json, indent, compact);

        if (++i == a->length) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}

static void objectContentToJson(const Object *o, std::string &json, int indent, bool compact)
{
    if (!o || !o->length)
        return;

    std::string indentString(4*indent, ' ');

    uint32_t i = 0;
    while (1) {
        Entry *e = o->entryAt(i);
        json += indentString;
        json += '"';
        json += escapedString(e->key());
        json += compact ? "\":" : "\": ";
        valueToJson(o, e->value, json, indent, compact);

        if (++i == o->length) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}

namespace Internal {

void objectToJson(const Object *o, std::string &json, int indent, bool compact)
{
    json.reserve(json.size() + (o ? (int)o->size : 16));
    json += compact ? "{" : "{\n";
    objectContentToJson(o, json, indent + (compact ? 0 : 1), compact);
    json += std::string(4*indent, ' ');
    json += compact ? "}" : "}\n";
}

void arrayToJson(const Array *a, std::string &json, int indent, bool compact)
{
    json.reserve(json.size() + (a ? (int)a->size : 16));
    json += compact ? "[" : "[\n";
    arrayContentToJson(a, json, indent + (compact ? 0 : 1), compact);
    json += std::string(4*indent, ' ');
    json += compact ? "]" : "]\n";
}

}



/*!
    \class JsonParseError
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The JsonParseError class is used to report errors during JSON parsing.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \enum JsonParseError::ParseError

    This enum describes the type of error that occurred during the parsing of a JSON document.

    \value NoError                  No error occurred
    \value UnterminatedObject       An object is not correctly terminated with a closing curly bracket
    \value MissingNameSeparator     A comma separating different items is missing
    \value UnterminatedArray        The array is not correctly terminated with a closing square bracket
    \value MissingValueSeparator    A colon separating keys from values inside objects is missing
    \value IllegalValue             The value is illegal
    \value TerminationByNumber      The input stream ended while parsing a number
    \value IllegalNumber            The number is not well formed
    \value IllegalEscapeSequence    An illegal escape sequence occurred in the input
    \value IllegalUTF8String        An illegal UTF8 sequence occurred in the input
    \value UnterminatedString       A string wasn't terminated with a quote
    \value MissingObject            An object was expected but couldn't be found
    \value DeepNesting              The JSON document is too deeply nested for the parser to parse it
    \value DocumentTooLarge         The JSON document is too large for the parser to parse it
    \value GarbageAtEnd             The parsed document contains additional garbage characters at the end

*/

/*!
    \variable JsonParseError::error

    Contains the type of the parse error. Is equal to JsonParseError::NoError if the document
    was parsed correctly.

    \sa ParseError, errorString()
*/


/*!
    \variable JsonParseError::offset

    Contains the offset in the input string where the parse error occurred.

    \sa error, errorString()
*/

using namespace Internal;

Parser::Parser(const char *json, int length)
    : head(json), json(json), data(0), dataLength(0), current(0), nestingLevel(0), lastError(JsonParseError::NoError)
{
    end = json + length;
}



/*

begin-array     = ws %x5B ws  ; [ left square bracket

begin-object    = ws %x7B ws  ; { left curly bracket

end-array       = ws %x5D ws  ; ] right square bracket

end-object      = ws %x7D ws  ; } right curly bracket

name-separator  = ws %x3A ws  ; : colon

value-separator = ws %x2C ws  ; , comma

Insignificant whitespace is allowed before or after any of the six
structural characters.

ws = *(
          %x20 /              ; Space
          %x09 /              ; Horizontal tab
          %x0A /              ; Line feed or New line
          %x0D                ; Carriage return
      )

*/

enum {
    Space = 0x20,
    Tab = 0x09,
    LineFeed = 0x0a,
    Return = 0x0d,
    BeginArray = 0x5b,
    BeginObject = 0x7b,
    EndArray = 0x5d,
    EndObject = 0x7d,
    NameSeparator = 0x3a,
    ValueSeparator = 0x2c,
    Quote = 0x22
};

void Parser::eatBOM()
{
    // eat UTF-8 byte order mark
    if (end - json > 3
            && (unsigned char)json[0] == 0xef
            && (unsigned char)json[1] == 0xbb
            && (unsigned char)json[2] == 0xbf)
        json += 3;
}

bool Parser::eatSpace()
{
    while (json < end) {
        if (*json > Space)
            break;
        if (*json != Space &&
            *json != Tab &&
            *json != LineFeed &&
            *json != Return)
            break;
        ++json;
    }
    return (json < end);
}

char Parser::nextToken()
{
    if (!eatSpace())
        return 0;
    char token = *json++;
    switch (token) {
    case BeginArray:
    case BeginObject:
    case NameSeparator:
    case ValueSeparator:
    case EndArray:
    case EndObject:
        eatSpace();
    case Quote:
        break;
    default:
        token = 0;
        break;
    }
    return token;
}

/*
    JSON-text = object / array
*/
JsonDocument Parser::parse(JsonParseError *error)
{
#ifdef PARSER_DEBUG
    indent = 0;
    std::cerr << ">>>>> parser begin";
#endif
    // allocate some space
    dataLength = static_cast<int>(std::max(end - json, std::ptrdiff_t(256)));
    data = (char *)malloc(dataLength);

    // fill in Header data
    Header *h = (Header *)data;
    h->tag = JsonDocument::BinaryFormatTag;
    h->version = 1u;

    current = sizeof(Header);

    eatBOM();
    char token = nextToken();

    DEBUG << std::hex << (uint32_t)token;
    if (token == BeginArray) {
        if (!parseArray())
            goto error;
    } else if (token == BeginObject) {
        if (!parseObject())
            goto error;
    } else {
        lastError = JsonParseError::IllegalValue;
        goto error;
    }

    eatSpace();
    if (json < end) {
        lastError = JsonParseError::GarbageAtEnd;
        goto error;
    }

    END;
    {
        if (error) {
            error->offset = 0;
            error->error = JsonParseError::NoError;
        }
        Data *d = new Data(data, current);
        return JsonDocument(d);
    }

error:
#ifdef PARSER_DEBUG
    std::cerr << ">>>>> parser error";
#endif
    if (error) {
        error->offset = static_cast<int>(json - head);
        error->error  = lastError;
    }
    free(data);
    return JsonDocument();
}


void Parser::ParsedObject::insert(uint32_t offset)
{
    const Entry *newEntry = reinterpret_cast<const Entry *>(parser->data + objectPosition + offset);
    size_t min = 0;
    size_t n = offsets.size();
    while (n > 0) {
        size_t half = n >> 1;
        size_t middle = min + half;
        if (*entryAt(middle) >= *newEntry) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < offsets.size() && *entryAt(min) == *newEntry) {
        offsets[min] = offset;
    } else {
        offsets.insert(offsets.begin() + min, offset);
    }
}

/*
    object = begin-object [ member *( value-separator member ) ]
    end-object
*/

bool Parser::parseObject()
{
    if (++nestingLevel > nestingLimit) {
        lastError = JsonParseError::DeepNesting;
        return false;
    }

    int objectOffset = reserveSpace(sizeof(Object));
    BEGIN << "parseObject pos=" << objectOffset << current << json;

    ParsedObject parsedObject(this, objectOffset);

    char token = nextToken();
    while (token == Quote) {
        int off = current - objectOffset;
        if (!parseMember(objectOffset))
            return false;
        parsedObject.insert(off);
        token = nextToken();
        if (token != ValueSeparator)
            break;
        token = nextToken();
        if (token == EndObject) {
            lastError = JsonParseError::MissingObject;
            return false;
        }
    }

    DEBUG << "end token=" << token;
    if (token != EndObject) {
        lastError = JsonParseError::UnterminatedObject;
        return false;
    }

    DEBUG << "numEntries" << parsedObject.offsets.size();
    int table = objectOffset;
    // finalize the object
    if (parsedObject.offsets.size()) {
        int tableSize = static_cast<int>(parsedObject.offsets.size()) * sizeof(uint32_t);
        table = reserveSpace(tableSize);
        memcpy(data + table, &*parsedObject.offsets.begin(), tableSize);
    }

    Object *o = (Object *)(data + objectOffset);
    o->tableOffset = table - objectOffset;
    o->size = current - objectOffset;
    o->is_object = true;
    o->length = static_cast<int>(parsedObject.offsets.size());

    DEBUG << "current=" << current;
    END;

    --nestingLevel;
    return true;
}

/*
    member = string name-separator value
*/
bool Parser::parseMember(int baseOffset)
{
    int entryOffset = reserveSpace(sizeof(Entry));
    BEGIN << "parseMember pos=" << entryOffset;

    if (!parseString())
        return false;
    char token = nextToken();
    if (token != NameSeparator) {
        lastError = JsonParseError::MissingNameSeparator;
        return false;
    }
    Value val;
    if (!parseValue(&val, baseOffset))
        return false;

    // finalize the entry
    Entry *e = (Entry *)(data + entryOffset);
    e->value = val;

    END;
    return true;
}

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
bool Parser::parseArray()
{
    BEGIN << "parseArray";

    if (++nestingLevel > nestingLimit) {
        lastError = JsonParseError::DeepNesting;
        return false;
    }

    int arrayOffset = reserveSpace(sizeof(Array));

    std::vector<Value> values;
    values.reserve(64);

    if (!eatSpace()) {
        lastError = JsonParseError::UnterminatedArray;
        return false;
    }
    if (*json == EndArray) {
        nextToken();
    } else {
        while (1) {
            Value val;
            if (!parseValue(&val, arrayOffset))
                return false;
            values.push_back(val);
            char token = nextToken();
            if (token == EndArray)
                break;
            else if (token != ValueSeparator) {
                if (!eatSpace())
                    lastError = JsonParseError::UnterminatedArray;
                else
                    lastError = JsonParseError::MissingValueSeparator;
                return false;
            }
        }
    }

    DEBUG << "size =" << values.size();
    int table = arrayOffset;
    // finalize the object
    if (values.size()) {
        int tableSize = static_cast<int>(values.size() * sizeof(Value));
        table = reserveSpace(tableSize);
        memcpy(data + table, values.data(), tableSize);
    }

    Array *a = (Array *)(data + arrayOffset);
    a->tableOffset = table - arrayOffset;
    a->size = current - arrayOffset;
    a->is_object = false;
    a->length = static_cast<int>(values.size());

    DEBUG << "current=" << current;
    END;

    --nestingLevel;
    return true;
}

/*
value = false / null / true / object / array / number / string

*/

bool Parser::parseValue(Value *val, int baseOffset)
{
    BEGIN << "parse Value" << json;
    val->_dummy = 0;

    switch (*json++) {
    case 'n':
        if (end - json < 4) {
            lastError = JsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'u' &&
            *json++ == 'l' &&
            *json++ == 'l') {
            val->type = JsonValue::Null;
            DEBUG << "value: null";
            END;
            return true;
        }
        lastError = JsonParseError::IllegalValue;
        return false;
    case 't':
        if (end - json < 4) {
            lastError = JsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'r' &&
            *json++ == 'u' &&
            *json++ == 'e') {
            val->type = JsonValue::Bool;
            val->value = true;
            DEBUG << "value: true";
            END;
            return true;
        }
        lastError = JsonParseError::IllegalValue;
        return false;
    case 'f':
        if (end - json < 5) {
            lastError = JsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'a' &&
            *json++ == 'l' &&
            *json++ == 's' &&
            *json++ == 'e') {
            val->type = JsonValue::Bool;
            val->value = false;
            DEBUG << "value: false";
            END;
            return true;
        }
        lastError = JsonParseError::IllegalValue;
        return false;
    case Quote: {
        val->type = JsonValue::String;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = JsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        if (!parseString())
            return false;
        val->intValue = false;
        DEBUG << "value: string";
        END;
        return true;
    }
    case BeginArray:
        val->type = JsonValue::Array;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = JsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        if (!parseArray())
            return false;
        DEBUG << "value: array";
        END;
        return true;
    case BeginObject:
        val->type = JsonValue::Object;
        if (current - baseOffset >= Value::MaxSize) {
            lastError = JsonParseError::DocumentTooLarge;
            return false;
        }
        val->value = current - baseOffset;
        if (!parseObject())
            return false;
        DEBUG << "value: object";
        END;
        return true;
    case EndArray:
        lastError = JsonParseError::MissingObject;
        return false;
    default:
        --json;
        if (!parseNumber(val, baseOffset))
            return false;
        DEBUG << "value: number";
        END;
    }

    return true;
}





/*
        number = [ minus ] int [ frac ] [ exp ]
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 / %x45            ; e E
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero / ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0

*/

bool Parser::parseNumber(Value *val, int baseOffset)
{
    BEGIN << "parseNumber" << json;
    val->type = JsonValue::Double;

    const char *start = json;
    bool isInt = true;

    // minus
    if (json < end && *json == '-')
        ++json;

    // int = zero / ( digit1-9 *DIGIT )
    if (json < end && *json == '0') {
        ++json;
    } else {
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // frac = decimal-point 1*DIGIT
    if (json < end && *json == '.') {
        isInt = false;
        ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // exp = e [ minus / plus ] 1*DIGIT
    if (json < end && (*json == 'e' || *json == 'E')) {
        isInt = false;
        ++json;
        if (json < end && (*json == '-' || *json == '+'))
            ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    if (json >= end) {
        lastError = JsonParseError::TerminationByNumber;
        return false;
    }

    if (isInt) {
        char *endptr = const_cast<char *>(json);
        long long int n = strtoll(start, &endptr, 0);
        if (endptr != start && n < (1<<25) && n > -(1<<25)) {
            val->int_value = int(n);
            val->intValue = true;
            END;
            return true;
        }
    }

    char *endptr = const_cast<char *>(json);
    double d = strtod(start, &endptr);

    if (start == endptr || std::isinf(d)) {
        lastError = JsonParseError::IllegalNumber;
        return false;
    }

    int pos = reserveSpace(sizeof(double));
    memcpy(data + pos, &d, sizeof(double));
    if (current - baseOffset >= Value::MaxSize) {
        lastError = JsonParseError::DocumentTooLarge;
        return false;
    }
    val->value = pos - baseOffset;
    val->intValue = false;

    END;
    return true;
}

/*

        string = quotation-mark *char quotation-mark

        char = unescaped /
               escape (
                   %x22 /          ; "    quotation mark  U+0022
                   %x5C /          ; \    reverse solidus U+005C
                   %x2F /          ; /    solidus         U+002F
                   %x62 /          ; b    backspace       U+0008
                   %x66 /          ; f    form feed       U+000C
                   %x6E /          ; n    line feed       U+000A
                   %x72 /          ; r    carriage return U+000D
                   %x74 /          ; t    tab             U+0009
                   %x75 4HEXDIG )  ; uXXXX                U+XXXX

        escape = %x5C              ; \

        quotation-mark = %x22      ; "

        unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
static bool addHexDigit(char digit, uint32_t *result)
{
    *result <<= 4;
    if (digit >= '0' && digit <= '9')
        *result |= (digit - '0');
    else if (digit >= 'a' && digit <= 'f')
        *result |= (digit - 'a') + 10;
    else if (digit >= 'A' && digit <= 'F')
        *result |= (digit - 'A') + 10;
    else
        return false;
    return true;
}

bool Parser::parseEscapeSequence()
{
    DEBUG << "scan escape" << (char)*json;
    const char escaped = *json++;
    switch (escaped) {
    case '"':
        addChar('"'); break;
    case '\\':
        addChar('\\'); break;
    case '/':
        addChar('/'); break;
    case 'b':
        addChar(0x8); break;
    case 'f':
        addChar(0xc); break;
    case 'n':
        addChar(0xa); break;
    case 'r':
        addChar(0xd); break;
    case 't':
        addChar(0x9); break;
    case 'u': {
        uint32_t c = 0;
        if (json > end - 4)
            return false;
        for (int i = 0; i < 4; ++i) {
            if (!addHexDigit(*json, &c))
                return false;
            ++json;
        }
        if (c < 0x80) {
            addChar(c);
            break;
        }
        if (c < 0x800) {
            addChar(192 + c / 64);
            addChar(128 + c % 64);
            break;
        }
        if (c - 0xd800u < 0x800) {
            return false;
        }
        if (c < 0x10000) {
            addChar(224 + c / 4096);
            addChar(128 + c / 64 % 64);
            addChar(128 + c % 64);
            break;
        }
        if (c < 0x110000) {
            addChar(240 + c / 262144);
            addChar(128 + c / 4096 % 64);
            addChar(128 + c / 64 % 64);
            addChar(128 + c % 64);
            break;
        }
        return false;
    }
    default:
        // this is not as strict as one could be, but allows for more Json files
        // to be parsed correctly.
        addChar(escaped);
        break;
    }
    return true;
}

bool Parser::parseString()
{
    const char *inStart = json;

    // First try quick pass without escapes.
    if (true) {
        while (1) {
            if (json >= end) {
                ++json;
                lastError = JsonParseError::UnterminatedString;
                return false;
            }

            const char c = *json;
            if (c == '"') {
                // write string length and padding.
                const int len = static_cast<int>(json - inStart);
                const int pos = reserveSpace(4 + alignedSize(len));
                toInternal(data + pos, inStart, len);
                END;

                ++json;
                return true;
            }

            if (c == '\\')
                break;
            ++json;
        }
    }

    // Try again with escapes.
    const int outStart = reserveSpace(4);
    json = inStart;
    while (1) {
        if (json >= end) {
            ++json;
            lastError = JsonParseError::UnterminatedString;
            return false;
        }

        if (*json == '"') {
            ++json;
            // write string length and padding.
            *(int *)(data + outStart) = current - outStart - 4;
            reserveSpace((4 - current) & 3);
            END;
            return true;
        }

        if (*json == '\\') {
            ++json;
            if (json >= end || !parseEscapeSequence()) {
                lastError = JsonParseError::IllegalEscapeSequence;
                return false;
            }
        } else {
            addChar(*json++);
        }
    }
}

namespace Internal {

static const Base emptyArray = { sizeof(Base), { 0 }, 0 };
static const Base emptyObject = { sizeof(Base), { 0 }, 0 };


void Data::compact()
{
    // assert(sizeof(Value) == sizeof(offset));

    if (!compactionCounter)
        return;

    Base *base = header->root();
    int reserve = 0;
    if (base->is_object) {
        Object *o = static_cast<Object *>(base);
        for (int i = 0; i < (int)o->length; ++i)
            reserve += o->entryAt(i)->usedStorage(o);
    } else {
        Array *a = static_cast<Array *>(base);
        for (int i = 0; i < (int)a->length; ++i)
            reserve += (*a)[i].usedStorage(a);
    }

    int size = sizeof(Base) + reserve + base->length*sizeof(offset);
    int alloc = sizeof(Header) + size;
    Header *h = (Header *) malloc(alloc);
    h->tag = JsonDocument::BinaryFormatTag;
    h->version = 1;
    Base *b = h->root();
    b->size = size;
    b->is_object = header->root()->is_object;
    b->length = base->length;
    b->tableOffset = reserve + sizeof(Array);

    int offset = sizeof(Base);
    if (b->is_object) {
        Object *o = static_cast<Object *>(base);
        Object *no = static_cast<Object *>(b);

        for (int i = 0; i < (int)o->length; ++i) {
            no->table()[i] = offset;

            const Entry *e = o->entryAt(i);
            Entry *ne = no->entryAt(i);
            int s = e->size();
            memcpy(ne, e, s);
            offset += s;
            int dataSize = e->value.usedStorage(o);
            if (dataSize) {
                memcpy((char *)no + offset, e->value.data(o), dataSize);
                ne->value.value = offset;
                offset += dataSize;
            }
        }
    } else {
        Array *a = static_cast<Array *>(base);
        Array *na = static_cast<Array *>(b);

        for (int i = 0; i < (int)a->length; ++i) {
            const Value &v = (*a)[i];
            Value &nv = (*na)[i];
            nv = v;
            int dataSize = v.usedStorage(a);
            if (dataSize) {
                memcpy((char *)na + offset, v.data(a), dataSize);
                nv.value = offset;
                offset += dataSize;
            }
        }
    }
    // assert(offset == (int)b->tableOffset);

    free(header);
    header = h;
    this->alloc = alloc;
    compactionCounter = 0;
}

bool Data::valid() const
{
    if (header->tag != JsonDocument::BinaryFormatTag || header->version != 1u)
        return false;

    bool res = false;
    if (header->root()->is_object)
        res = static_cast<Object *>(header->root())->isValid();
    else
        res = static_cast<Array *>(header->root())->isValid();

    return res;
}


int Base::reserveSpace(uint32_t dataSize, int posInTable, uint32_t numItems, bool replace)
{
    // assert(posInTable >= 0 && posInTable <= (int)length);
    if (size + dataSize >= Value::MaxSize) {
        fprintf(stderr, "Json: Document too large to store in data structure %d %d %d\n", (uint32_t)size, dataSize, Value::MaxSize);
        return 0;
    }

    offset off = tableOffset;
    // move table to new position
    if (replace) {
        memmove((char *)(table()) + dataSize, table(), length*sizeof(offset));
    } else {
        memmove((char *)(table() + posInTable + numItems) + dataSize, table() + posInTable, (length - posInTable)*sizeof(offset));
        memmove((char *)(table()) + dataSize, table(), posInTable*sizeof(offset));
    }
    tableOffset += dataSize;
    for (int i = 0; i < (int)numItems; ++i)
        table()[posInTable + i] = off;
    size += dataSize;
    if (!replace) {
        length += numItems;
        size += numItems * sizeof(offset);
    }
    return off;
}

void Base::removeItems(int pos, int numItems)
{
    // assert(pos >= 0 && pos <= (int)length);
    if (pos + numItems < (int)length)
        memmove(table() + pos, table() + pos + numItems, (length - pos - numItems)*sizeof(offset));
    length -= numItems;
}

int Object::indexOf(const std::string &key, bool *exists)
{
    int min = 0;
    int n = length;
    while (n > 0) {
        int half = n >> 1;
        int middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < (int)length && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

bool Object::isValid() const
{
    if (tableOffset + length*sizeof(offset) > size)
        return false;

    std::string lastKey;
    for (uint32_t i = 0; i < length; ++i) {
        offset entryOffset = table()[i];
        if (entryOffset + sizeof(Entry) >= tableOffset)
            return false;
        Entry *e = entryAt(i);
        int s = e->size();
        if (table()[i] + s > tableOffset)
            return false;
        std::string key = e->key();
        if (key < lastKey)
            return false;
        if (!e->value.isValid(this))
            return false;
        lastKey = key;
    }
    return true;
}

bool Array::isValid() const
{
    if (tableOffset + length*sizeof(offset) > size)
        return false;

    for (uint32_t i = 0; i < length; ++i) {
        if (!at(i).isValid(this))
            return false;
    }
    return true;
}


bool Entry::operator==(const std::string &key) const
{
    return shallowKey() == key;
}

bool Entry::operator==(const Entry &other) const
{
    return shallowKey() == other.shallowKey();
}

bool Entry::operator>=(const Entry &other) const
{
    return shallowKey() >= other.shallowKey();
}


int Value::usedStorage(const Base *b) const
{
    int s = 0;
    switch (type) {
    case JsonValue::Double:
        if (intValue)
            break;
        s = sizeof(double);
        break;
    case JsonValue::String: {
        char *d = data(b);
        s = sizeof(int) + (*(int *)d);
        break;
    }
    case JsonValue::Array:
    case JsonValue::Object:
        s = base(b)->size;
        break;
    case JsonValue::Null:
    case JsonValue::Bool:
    default:
        break;
    }
    return alignedSize(s);
}

bool Value::isValid(const Base *b) const
{
    int offset = 0;
    switch (type) {
    case JsonValue::Double:
        if (intValue)
            break;
        // fall through
    case JsonValue::String:
    case JsonValue::Array:
    case JsonValue::Object:
        offset = value;
        break;
    case JsonValue::Null:
    case JsonValue::Bool:
    default:
        break;
    }

    if (!offset)
        return true;
    if (offset + sizeof(uint32_t) > b->tableOffset)
        return false;

    int s = usedStorage(b);
    if (!s)
        return true;
    if (s < 0 || offset + s > (int)b->tableOffset)
        return false;
    if (type == JsonValue::Array)
        return static_cast<Array *>(base(b))->isValid();
    if (type == JsonValue::Object)
        return static_cast<Object *>(base(b))->isValid();
    return true;
}

/*!
    \internal
 */
int Value::requiredStorage(JsonValue &v, bool *compressed)
{
    *compressed = false;
    switch (v.t) {
    case JsonValue::Double:
        if (Internal::compressedNumber(v.dbl) != INT_MAX) {
            *compressed = true;
            return 0;
        }
        return sizeof(double);
    case JsonValue::String: {
        std::string s = v.toString().data();
        *compressed = false;
        return Internal::qStringSize(s);
    }
    case JsonValue::Array:
    case JsonValue::Object:
        if (v.d && v.d->compactionCounter) {
            v.detach();
            v.d->compact();
            v.base = static_cast<Internal::Base *>(v.d->header->root());
        }
        return v.base ? v.base->size : sizeof(Internal::Base);
    case JsonValue::Undefined:
    case JsonValue::Null:
    case JsonValue::Bool:
        break;
    }
    return 0;
}

/*!
    \internal
 */
uint32_t Value::valueToStore(const JsonValue &v, uint32_t offset)
{
    switch (v.t) {
    case JsonValue::Undefined:
    case JsonValue::Null:
        break;
    case JsonValue::Bool:
        return v.b;
    case JsonValue::Double: {
        int c = Internal::compressedNumber(v.dbl);
        if (c != INT_MAX)
            return c;
    }
        // fall through
    case JsonValue::String:
    case JsonValue::Array:
    case JsonValue::Object:
        return offset;
    }
    return 0;
}

/*!
    \internal
 */

void Value::copyData(const JsonValue &v, char *dest, bool compressed)
{
    switch (v.t) {
    case JsonValue::Double:
        if (!compressed)
            memcpy(dest, &v.ui, 8);
        break;
    case JsonValue::String: {
        std::string str = v.toString();
        Internal::copyString(dest, str);
        break;
    }
    case JsonValue::Array:
    case JsonValue::Object: {
        const Internal::Base *b = v.base;
        if (!b)
            b = (v.t == JsonValue::Array ? &emptyArray : &emptyObject);
        memcpy(dest, b, b->size);
        break;
    }
    default:
        break;
    }
}

} // namespace Internal
} // namespace Json
