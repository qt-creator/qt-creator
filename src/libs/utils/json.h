/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "utils_global.h"

#include <QHash>
#include <QVector>
#include <QStringList>
#include <QDateTime>

QT_FORWARD_DECLARE_CLASS(QVariant)

namespace Utils {

class JsonStringValue;
class JsonDoubleValue;
class JsonIntValue;
class JsonObjectValue;
class JsonArrayValue;
class JsonBooleanValue;
class JsonNullValue;

class QTCREATOR_UTILS_EXPORT JsonMemoryPool
{
public:
    ~JsonMemoryPool();

    inline void *allocate(size_t size)
    {
        char *obj = new char[size];
        _objs.append(obj);
        return obj;
    }

private:
    QVector<char *> _objs;
};

/*!
 * \brief The JsonValue class
 */
class QTCREATOR_UTILS_EXPORT JsonValue
{
public:
    enum Kind {
        String,
        Double,
        Int,
        Object,
        Array,
        Boolean,
        Null,
        Unknown
    };

    virtual ~JsonValue();

    Kind kind() const { return m_kind; }
    static QString kindToString(Kind kind);

    virtual JsonStringValue *toString() { return 0; }
    virtual JsonDoubleValue *toDouble() { return 0; }
    virtual JsonIntValue *toInt() { return 0; }
    virtual JsonObjectValue *toObject() { return 0; }
    virtual JsonArrayValue *toArray() { return 0; }
    virtual JsonBooleanValue *toBoolean() { return 0; }
    virtual JsonNullValue *toNull() { return 0; }

    static JsonValue *create(const QString &s, JsonMemoryPool *pool);
    void *operator new(size_t size, JsonMemoryPool *pool);
    void operator delete(void *);
    void operator delete(void *, JsonMemoryPool *);

protected:
    JsonValue(Kind kind);

private:
    static JsonValue *build(const QVariant &varixant, JsonMemoryPool *pool);

    Kind m_kind;
};


/*!
 * \brief The JsonStringValue class
 */
class QTCREATOR_UTILS_EXPORT JsonStringValue : public JsonValue
{
public:
    JsonStringValue(const QString &value)
        : JsonValue(String)
        , m_value(value)
    {}

    virtual JsonStringValue *toString() { return this; }

    const QString &value() const { return m_value; }

private:
    QString m_value;
};


/*!
 * \brief The JsonDoubleValue class
 */
class QTCREATOR_UTILS_EXPORT JsonDoubleValue : public JsonValue
{
public:
    JsonDoubleValue(double value)
        : JsonValue(Double)
        , m_value(value)
    {}

    virtual JsonDoubleValue *toDouble() { return this; }

    double value() const { return m_value; }

private:
    double m_value;
};

/*!
 * \brief The JsonIntValue class
 */
class QTCREATOR_UTILS_EXPORT JsonIntValue : public JsonValue
{
public:
    JsonIntValue(int value)
        : JsonValue(Int)
        , m_value(value)
    {}

    virtual JsonIntValue *toInt() { return this; }

    int value() const { return m_value; }

private:
    int m_value;
};


/*!
 * \brief The JsonObjectValue class
 */
class QTCREATOR_UTILS_EXPORT JsonObjectValue : public JsonValue
{
public:
    JsonObjectValue()
        : JsonValue(Object)
    {}

    virtual JsonObjectValue *toObject() { return this; }

    void addMember(const QString &name, JsonValue *value) { m_members.insert(name, value); }
    bool hasMember(const QString &name) const { return m_members.contains(name); }
    JsonValue *member(const QString &name) const { return m_members.value(name); }
    QHash<QString, JsonValue *> members() const { return m_members; }
    bool isEmpty() const { return m_members.isEmpty(); }

protected:
    JsonObjectValue(Kind kind)
        : JsonValue(kind)
    {}

private:
    QHash<QString, JsonValue *> m_members;
};


/*!
 * \brief The JsonArrayValue class
 */
class QTCREATOR_UTILS_EXPORT JsonArrayValue : public JsonValue
{
public:
    JsonArrayValue()
        : JsonValue(Array)
    {}

    virtual JsonArrayValue *toArray() { return this; }

    void addElement(JsonValue *value) { m_elements.append(value); }
    QList<JsonValue *> elements() const { return m_elements; }
    int size() const { return m_elements.size(); }

private:
    QList<JsonValue *> m_elements;
};


/*!
 * \brief The JsonBooleanValue class
 */
class QTCREATOR_UTILS_EXPORT JsonBooleanValue : public JsonValue
{
public:
    JsonBooleanValue(bool value)
        : JsonValue(Boolean)
        , m_value(value)
    {}

    virtual JsonBooleanValue *toBoolean() { return this; }

    bool value() const { return m_value; }

private:
    bool m_value;
};

class QTCREATOR_UTILS_EXPORT JsonNullValue : public JsonValue
{
public:
    JsonNullValue()
        : JsonValue(Null)
    {}

    virtual JsonNullValue *toNull() { return this; }
};

class JsonSchemaManager;

/*!
 * \brief The JsonSchema class
 *
 * [NOTE: This is an incomplete implementation and a work in progress.]
 *
 * This class provides an interface for traversing and evaluating a JSON schema, as described
 * in the draft http://tools.ietf.org/html/draft-zyp-json-schema-03.
 *
 * JSON schemas are recursive in concept. This means that a particular attribute from a schema
 * might be also another schema. Therefore, the basic working principle of this API is that
 * from within some schema, one can investigate its attributes and if necessary "enter" a
 * corresponding nested schema. Afterwards, it's expected that one would "leave" such nested
 * schema.
 *
 * All functions assume that the current "context" is a valid schema. Once an instance of this
 * class is created the root schema is put on top of the stack.
 *
 */
class QTCREATOR_UTILS_EXPORT JsonSchema
{
public:
    bool isTypeConstrained() const;
    bool acceptsType(const QString &type) const;
    QStringList validTypes() const;

    // Applicable on schemas of any type.
    bool required() const;

    bool hasTypeSchema() const;
    void enterNestedTypeSchema();

    bool hasUnionSchema() const;
    int unionSchemaSize() const;
    bool maybeEnterNestedUnionSchema(int index);

    void leaveNestedSchema();

    // Applicable on schemas of type number/integer.
    bool hasMinimum() const;
    bool hasMaximum() const;
    bool hasExclusiveMinimum();
    bool hasExclusiveMaximum();
    double minimum() const;
    double maximum() const;

    // Applicable on schemas of type string.
    QString pattern() const;
    int minimumLength() const;
    int maximumLength() const;

    // Applicable on schemas of type object.
    QStringList properties() const;
    bool hasPropertySchema(const QString &property) const;
    void enterNestedPropertySchema(const QString &property);

    // Applicable on schemas of type array.
    bool hasAdditionalItems() const;

    bool hasItemSchema() const;
    void enterNestedItemSchema();

    bool hasItemArraySchema() const;
    int itemArraySchemaSize() const;
    bool maybeEnterNestedArraySchema(int index);

private:
    friend class JsonSchemaManager;
    JsonSchema(JsonObjectValue *rootObject, const JsonSchemaManager *manager);
    Q_DISABLE_COPY(JsonSchema)

    enum EvaluationMode {
        Normal,
        Array,
        Union
    };

    void enter(JsonObjectValue *ov, EvaluationMode eval = Normal, int index = -1);
    bool maybeEnter(JsonValue *v, EvaluationMode eval, int index);
    void evaluate(EvaluationMode eval, int index);
    void leave();

    JsonObjectValue *resolveReference(JsonObjectValue *ov) const;
    JsonObjectValue *resolveBase(JsonObjectValue *ov) const;

    JsonObjectValue *currentValue() const;
    int currentIndex() const;

    JsonObjectValue *rootValue() const;

    static JsonStringValue *getStringValue(const QString &name, JsonObjectValue *value);
    static JsonObjectValue *getObjectValue(const QString &name, JsonObjectValue *value);
    static JsonBooleanValue *getBooleanValue(const QString &name, JsonObjectValue *value);
    static JsonArrayValue *getArrayValue(const QString &name, JsonObjectValue *value);
    static JsonDoubleValue *getDoubleValue(const QString &name, JsonObjectValue *value);

    static QStringList validTypes(JsonObjectValue *v);
    static bool typeMatches(const QString &expected, const QString &actual);
    static bool isCheckableType(const QString &s);

    QStringList properties(JsonObjectValue *v) const;
    JsonObjectValue *propertySchema(const QString &property, JsonObjectValue *v) const;
    // TODO: Similar functions for other attributes which require looking into base schemas.

    static bool maybeSchemaName(const QString &s);

    static QString kType();
    static QString kProperties();
    static QString kPatternProperties();
    static QString kAdditionalProperties();
    static QString kItems();
    static QString kAdditionalItems();
    static QString kRequired();
    static QString kDependencies();
    static QString kMinimum();
    static QString kMaximum();
    static QString kExclusiveMinimum();
    static QString kExclusiveMaximum();
    static QString kMinItems();
    static QString kMaxItems();
    static QString kUniqueItems();
    static QString kPattern();
    static QString kMinLength();
    static QString kMaxLength();
    static QString kTitle();
    static QString kDescription();
    static QString kExtends();
    static QString kRef();

    struct Context
    {
        JsonObjectValue *m_value;
        EvaluationMode m_eval;
        int m_index;
    };

    QVector<Context> m_schemas;
    const JsonSchemaManager *m_manager;
};


/*!
 * \brief The JsonSchemaManager class
 */
class QTCREATOR_UTILS_EXPORT JsonSchemaManager
{
public:
    JsonSchemaManager(const QStringList &searchPaths);
    ~JsonSchemaManager();

    JsonSchema *schemaForFile(const QString &fileName) const;
    JsonSchema *schemaByName(const QString &baseName) const;

private:
    struct JsonSchemaData
    {
        JsonSchemaData(const QString &absoluteFileName, JsonSchema *schema = 0)
            : m_absoluteFileName(absoluteFileName)
            , m_schema(schema)
        {}
        QString m_absoluteFileName;
        JsonSchema *m_schema;
        QDateTime m_lastParseAttempt;
    };

    JsonSchema *parseSchema(const QString &schemaFileName) const;

    QStringList m_searchPaths;
    mutable QHash<QString, JsonSchemaData> m_schemas;
    mutable JsonMemoryPool m_pool;
};

} // namespace Utils
