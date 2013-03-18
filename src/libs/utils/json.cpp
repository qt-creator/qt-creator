/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "json.h"

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QStringBuilder>
#include <QDebug>
#include <QScriptEngine>

using namespace Utils;


JsonValue::JsonValue(Kind kind)
    : m_kind(kind)
{}

JsonValue::~JsonValue()
{}

JsonValue *JsonValue::create(const QString &s)
{
    QScriptEngine engine;
    QScriptValue jsonParser = engine.evaluate(QLatin1String("JSON.parse"));
    QScriptValue value = jsonParser.call(QScriptValue(), QScriptValueList() << s);
    if (engine.hasUncaughtException() || !value.isValid())
        return 0;

    return build(value.toVariant());
}

QString JsonValue::kindToString(JsonValue::Kind kind)
{
    if (kind == String)
        return QLatin1String("string");
    if (kind == Double)
        return QLatin1String("number");
    if (kind == Int)
        return QLatin1String("integer");
    if (kind == Object)
        return QLatin1String("object");
    if (kind == Array)
        return QLatin1String("array");
    if (kind == Boolean)
        return QLatin1String("boolean");
    if (kind == Null)
        return QLatin1String("null");

    return QLatin1String("unkown");
}

JsonValue *JsonValue::build(const QVariant &variant)
{
    switch (variant.type()) {

    case QVariant::List: {
        JsonArrayValue *newValue = new JsonArrayValue;
        foreach (const QVariant &element, variant.toList())
            newValue->addElement(build(element));
        return newValue;
    }

    case QVariant::Map: {
        JsonObjectValue *newValue = new JsonObjectValue;
        const QVariantMap variantMap = variant.toMap();
        for (QVariantMap::const_iterator it = variantMap.begin(); it != variantMap.end(); ++it)
            newValue->addMember(it.key(), build(it.value()));
        return newValue;
    }

    case QVariant::String:
        return new JsonStringValue(variant.toString());

    case QVariant::Int:
        return new JsonIntValue(variant.toInt());

    case QVariant::Double:
        return new JsonDoubleValue(variant.toDouble());

    case QVariant::Bool:
        return new JsonBooleanValue(variant.toBool());

    case QVariant::Invalid:
        return new JsonNullValue;

    default:
        break;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////

const QString JsonSchema::kType(QLatin1String("type"));
const QString JsonSchema::kProperties(QLatin1String("properties"));
const QString JsonSchema::kPatternProperties(QLatin1String("patternProperties"));
const QString JsonSchema::kAdditionalProperties(QLatin1String("additionalProperties"));
const QString JsonSchema::kItems(QLatin1String("items"));
const QString JsonSchema::kAdditionalItems(QLatin1String("additionalItems"));
const QString JsonSchema::kRequired(QLatin1String("required"));
const QString JsonSchema::kDependencies(QLatin1String("dependencies"));
const QString JsonSchema::kMinimum(QLatin1String("minimum"));
const QString JsonSchema::kMaximum(QLatin1String("maximum"));
const QString JsonSchema::kExclusiveMinimum(QLatin1String("exclusiveMinimum"));
const QString JsonSchema::kExclusiveMaximum(QLatin1String("exclusiveMaximum"));
const QString JsonSchema::kMinItems(QLatin1String("minItems"));
const QString JsonSchema::kMaxItems(QLatin1String("maxItems"));
const QString JsonSchema::kUniqueItems(QLatin1String("uniqueItems"));
const QString JsonSchema::kPattern(QLatin1String("pattern"));
const QString JsonSchema::kMinLength(QLatin1String("minLength"));
const QString JsonSchema::kMaxLength(QLatin1String("maxLength"));
const QString JsonSchema::kTitle(QLatin1String("title"));
const QString JsonSchema::kDescription(QLatin1String("description"));
const QString JsonSchema::kExtends(QLatin1String("extends"));
const QString JsonSchema::kRef(QLatin1String("$ref"));

JsonSchema::JsonSchema(JsonObjectValue *rootObject, const JsonSchemaManager *manager)
    : m_manager(manager)
{
    enter(rootObject);
}

bool JsonSchema::isTypeConstrained() const
{
    // Simple types
    if (JsonStringValue *sv = getStringValue(kType, currentValue()))
        return isCheckableType(sv->value());

    // Union types
    if (JsonArrayValue *av = getArrayValue(kType, currentValue())) {
        QTC_ASSERT(currentIndex() != -1, return false);
        QTC_ASSERT(av->elements().at(currentIndex())->kind() == JsonValue::String, return false);
        JsonStringValue *sv = av->elements().at(currentIndex())->toString();
        return isCheckableType(sv->value());
    }

    return false;
}

bool JsonSchema::acceptsType(const QString &type) const
{
    // Simple types
    if (JsonStringValue *sv = getStringValue(kType, currentValue()))
        return typeMatches(sv->value(), type);

    // Union types
    if (JsonArrayValue *av = getArrayValue(kType, currentValue())) {
        QTC_ASSERT(currentIndex() != -1, return false);
        QTC_ASSERT(av->elements().at(currentIndex())->kind() == JsonValue::String, return false);
        JsonStringValue *sv = av->elements().at(currentIndex())->toString();
        return typeMatches(sv->value(), type);
    }

    return false;
}

QStringList JsonSchema::validTypes(JsonObjectValue *v)
{
    QStringList all;

    if (JsonStringValue *sv = getStringValue(kType, v))
        all.append(sv->value());

    if (JsonObjectValue *ov = getObjectValue(kType, v))
        return validTypes(ov);

    if (JsonArrayValue *av = getArrayValue(kType, v)) {
        foreach (JsonValue *v, av->elements()) {
            if (JsonStringValue *sv = v->toString())
                all.append(sv->value());
            else if (JsonObjectValue *ov = v->toObject())
                all.append(validTypes(ov));
        }
    }

    return all;
}

bool JsonSchema::typeMatches(const QString &expected, const QString &actual)
{
    if (expected == QLatin1String("number") && actual == QLatin1String("integer"))
        return true;

    return expected == actual;
}

bool JsonSchema::isCheckableType(const QString &s)
{
    if (s == QLatin1String("string")
            || s == QLatin1String("number")
            || s == QLatin1String("integer")
            || s == QLatin1String("boolean")
            || s == QLatin1String("object")
            || s == QLatin1String("array")
            || s == QLatin1String("null")) {
        return true;
    }

    return false;
}

QStringList JsonSchema::validTypes() const
{
    return validTypes(currentValue());
}

bool JsonSchema::hasTypeSchema() const
{
    return getObjectValue(kType, currentValue());
}

void JsonSchema::enterNestedTypeSchema()
{
    QTC_ASSERT(hasTypeSchema(), return);

    enter(getObjectValue(kType, currentValue()));
}

QStringList JsonSchema::properties(JsonObjectValue *v) const
{
    typedef QHash<QString, JsonValue *>::ConstIterator MemberConstIterator;

    QStringList all;

    if (JsonObjectValue *ov = getObjectValue(kProperties, v)) {
        const MemberConstIterator cend = ov->members().constEnd();
        for (MemberConstIterator it = ov->members().constBegin(); it != cend; ++it)
            if (hasPropertySchema(it.key()))
                all.append(it.key());
    }

    if (JsonObjectValue *base = resolveBase(v))
        all.append(properties(base));

    return all;
}

QStringList JsonSchema::properties() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Object)), return QStringList());

    return properties(currentValue());
}

JsonObjectValue *JsonSchema::propertySchema(const QString &property,
                                                      JsonObjectValue *v) const
{
    if (JsonObjectValue *ov = getObjectValue(kProperties, v)) {
        JsonValue *member = ov->member(property);
        if (member && member->kind() == JsonValue::Object)
            return member->toObject();
    }

    if (JsonObjectValue *base = resolveBase(v))
        return propertySchema(property, base);

    return 0;
}

bool JsonSchema::hasPropertySchema(const QString &property) const
{
    return propertySchema(property, currentValue());
}

void JsonSchema::enterNestedPropertySchema(const QString &property)
{
    QTC_ASSERT(hasPropertySchema(property), return);

    JsonObjectValue *schema = propertySchema(property, currentValue());

    enter(schema);
}

/*!
 * An array schema is allowed to have its *items* specification in the form of another schema
 * or in the form of an array of schemas [Sec. 5.5]. This methods checks whether this is case
 * in which the items are a schema.
 *
 * \return whether or not the items from the array are a schema
 */
bool JsonSchema::hasItemSchema() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return getObjectValue(kItems, currentValue());
}

void JsonSchema::enterNestedItemSchema()
{
    QTC_ASSERT(hasItemSchema(), return);

    enter(getObjectValue(kItems, currentValue()));
}

/*!
 * An array schema is allowed to have its *items* specification in the form of another schema
 * or in the form of an array of schemas [Sec. 5.5]. This methods checks whether this is case
 * in which the items are an array of schemas.
 *
 * \return whether or not the items from the array are a an array of schemas
 */
bool JsonSchema::hasItemArraySchema() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return getArrayValue(kItems, currentValue());
}

int JsonSchema::itemArraySchemaSize() const
{
    QTC_ASSERT(hasItemArraySchema(), return false);

    return getArrayValue(kItems, currentValue())->size();
}

/*!
 * When evaluating the items of an array it might be necessary to "enter" a particular schema,
 * since this API assumes that there's always a valid schema in context (the one the user is
 * interested on). This shall only happen if the item at the supplied array index is of type
 * object, which is then assumed to be a schema.
 *
 * The method also marks the context as being inside an array evaluation.
 *
 * \return whether it was necessary to "enter" a schema for the supplied array index, false if index is out of bounds
 */
bool JsonSchema::maybeEnterNestedArraySchema(int index)
{
    QTC_ASSERT(itemArraySchemaSize(), return false);
    QTC_ASSERT(index >= 0 && index < itemArraySchemaSize(), return false);

    JsonValue *v = getArrayValue(kItems, currentValue())->elements().at(index);

    return maybeEnter(v, Array, index);
}

/*!
 * The type of a schema can be specified in the form of a union type, which is basically an
 * array of allowed types for the particular instance [Sec. 5.1]. This method checks whether
 * the current schema is one of such.
 *
 * \return whether or not the current schema specifies a union type
 */
bool JsonSchema::hasUnionSchema() const
{
    return getArrayValue(kType, currentValue());
}

int JsonSchema::unionSchemaSize() const
{
    return getArrayValue(kType, currentValue())->size();
}

/*!
 * When evaluating union types it might be necessary to "enter" a particular schema, since this
 * API assumes that there's always a valid schema in context (the one the user is interested on).
 * This shall only happen if the item at the supplied union index, which is then assumed to be
 * a schema.
 *
 * The method also marks the context as being inside an union evaluation.
 *
 * \param index
 * \return whether or not it was necessary to "enter" a schema for the supplied union index
 */
bool JsonSchema::maybeEnterNestedUnionSchema(int index)
{
    QTC_ASSERT(unionSchemaSize(), return false);
    QTC_ASSERT(index >= 0 && index < unionSchemaSize(), return false);

    JsonValue *v = getArrayValue(kType, currentValue())->elements().at(index);

    return maybeEnter(v, Union, index);
}

void JsonSchema::leaveNestedSchema()
{
    QTC_ASSERT(!m_schemas.isEmpty(), return);

    leave();
}

bool JsonSchema::required() const
{
    if (JsonBooleanValue *bv = getBooleanValue(kRequired, currentValue()))
        return bv->value();

    return false;
}

bool JsonSchema::hasMinimum() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    return getDoubleValue(kMinimum, currentValue());
}

double JsonSchema::minimum() const
{
    QTC_ASSERT(hasMinimum(), return 0);

    return getDoubleValue(kMinimum, currentValue())->value();
}

bool JsonSchema::hasExclusiveMinimum()
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    if (JsonBooleanValue *bv = getBooleanValue(kExclusiveMinimum, currentValue()))
        return bv->value();

    return false;
}

bool JsonSchema::hasMaximum() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    return getDoubleValue(kMaximum, currentValue());
}

double JsonSchema::maximum() const
{
    QTC_ASSERT(hasMaximum(), return 0);

    return getDoubleValue(kMaximum, currentValue())->value();
}

bool JsonSchema::hasExclusiveMaximum()
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    if (JsonBooleanValue *bv = getBooleanValue(kExclusiveMaximum, currentValue()))
        return bv->value();

    return false;
}

QString JsonSchema::pattern() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return QString());

    if (JsonStringValue *sv = getStringValue(kPattern, currentValue()))
        return sv->value();

    return QString();
}

int JsonSchema::minimumLength() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return -1);

    if (JsonDoubleValue *dv = getDoubleValue(kMinLength, currentValue()))
        return dv->value();

    return -1;
}

int JsonSchema::maximumLength() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return -1);

    if (JsonDoubleValue *dv = getDoubleValue(kMaxLength, currentValue()))
        return dv->value();

    return -1;
}

bool JsonSchema::hasAdditionalItems() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return currentValue()->member(kAdditionalItems);
}

bool JsonSchema::maybeSchemaName(const QString &s)
{
    if (s.isEmpty() || s == QLatin1String("any"))
        return false;

    return !isCheckableType(s);
}

JsonObjectValue *JsonSchema::rootValue() const
{
    QTC_ASSERT(!m_schemas.isEmpty(), return 0);

    return m_schemas.first().m_value;
}

JsonObjectValue *JsonSchema::currentValue() const
{
    QTC_ASSERT(!m_schemas.isEmpty(), return 0);

    return m_schemas.last().m_value;
}

int JsonSchema::currentIndex() const
{
    QTC_ASSERT(!m_schemas.isEmpty(), return 0);

    return m_schemas.last().m_index;
}

void JsonSchema::evaluate(EvaluationMode eval, int index)
{
    QTC_ASSERT(!m_schemas.isEmpty(), return);

    m_schemas.last().m_eval = eval;
    m_schemas.last().m_index = index;
}

void JsonSchema::enter(JsonObjectValue *ov, EvaluationMode eval, int index)
{
    Context context;
    context.m_eval = eval;
    context.m_index = index;
    context.m_value = resolveReference(ov);

    m_schemas.push_back(context);
}

bool JsonSchema::maybeEnter(JsonValue *v, EvaluationMode eval, int index)
{
    evaluate(eval, index);

    if (v->kind() == JsonValue::Object) {
        enter(v->toObject());
        return true;
    }

    if (v->kind() == JsonValue::String) {
        const QString &s = v->toString()->value();
        if (maybeSchemaName(s)) {
            JsonSchema *schema = m_manager->schemaByName(s);
            if (schema) {
                enter(schema->rootValue());
                return true;
            }
        }
    }

    return false;
}

void JsonSchema::leave()
{
    QTC_ASSERT(!m_schemas.isEmpty(), return);

    m_schemas.pop_back();
}

JsonObjectValue *JsonSchema::resolveReference(JsonObjectValue *ov) const
{
    if (JsonStringValue *sv = getStringValue(kRef, ov)) {
        JsonSchema *referenced = m_manager->schemaByName(sv->value());
        if (referenced)
            return referenced->rootValue();
    }

    return ov;
}

JsonObjectValue *JsonSchema::resolveBase(JsonObjectValue *ov) const
{
    if (JsonValue *v = ov->member(kExtends)) {
        if (v->kind() == JsonValue::String) {
            JsonSchema *schema = m_manager->schemaByName(v->toString()->value());
            if (schema)
                return schema->rootValue();
        } else if (v->kind() == JsonValue::Object) {
            return resolveReference(v->toObject());
        }
    }

    return 0;
}

JsonStringValue *JsonSchema::getStringValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return 0;

    return v->toString();
}

JsonObjectValue *JsonSchema::getObjectValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return 0;

    return v->toObject();
}

JsonBooleanValue *JsonSchema::getBooleanValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return 0;

    return v->toBoolean();
}

JsonArrayValue *JsonSchema::getArrayValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return 0;

    return v->toArray();
}

JsonDoubleValue *JsonSchema::getDoubleValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return 0;

    return v->toDouble();
}


///////////////////////////////////////////////////////////////////////////////

JsonSchemaManager::JsonSchemaManager(const QStringList &searchPaths)
    : m_searchPaths(searchPaths)
{
    foreach (const QString &path, m_searchPaths) {
        QDir dir(path);
        if (!dir.exists() && !dir.mkpath(path))
            continue;
        dir.setNameFilters(QStringList(QLatin1String("*.json")));
        foreach (const QFileInfo &fi, dir.entryInfoList())
            m_schemas.insert(fi.baseName(), JsonSchemaData(fi.absoluteFilePath()));
    }
}

JsonSchemaManager::~JsonSchemaManager()
{
    foreach (const JsonSchemaData &schemaData, m_schemas)
        delete schemaData.m_schema;
}

/*!
 * \brief JsonManager::schemaForFile
 *
 * Try to find a JSON schema to which the supplied file can be validated against. According
 * to the specification, how the schema/instance association is done is implementation defined.
 * Currently we use a quite naive approach which is simply based on file names. Specifically,
 * if one opens a foo.json file we'll look for a schema named foo.json. We should probably
 * investigate alternative settings later.
 *
 * \param fileName - JSON file to be validated
 * \return a valid schema or 0
 */
JsonSchema *JsonSchemaManager::schemaForFile(const QString &fileName) const
{
    QString baseName(QFileInfo(fileName).baseName());

    return schemaByName(baseName);
}

JsonSchema *JsonSchemaManager::schemaByName(const QString &baseName) const
{
    QHash<QString, JsonSchemaData>::iterator it = m_schemas.find(baseName);
    if (it == m_schemas.end()) {
        foreach (const QString &path, m_searchPaths) {
            QFileInfo candidate(path % baseName % QLatin1String(".json"));
            if (candidate.exists()) {
                m_schemas.insert(baseName, candidate.absoluteFilePath());
                break;
            }
        }
    }

    it = m_schemas.find(baseName);
    if (it == m_schemas.end())
        return 0;

    JsonSchemaData *schemaData = &it.value();
    if (!schemaData->m_schema) {
         // Schemas are built on-demand.
        QFileInfo currentSchema(schemaData->m_absoluteFileName);
        Q_ASSERT(currentSchema.exists());
        if (schemaData->m_lastParseAttempt.isNull()
                || schemaData->m_lastParseAttempt < currentSchema.lastModified()) {
            schemaData->m_schema = parseSchema(currentSchema.absoluteFilePath());
        }
    }

    return schemaData->m_schema;
}

JsonSchema *JsonSchemaManager::parseSchema(const QString &schemaFileName) const
{
    FileReader reader;
    if (reader.fetch(schemaFileName, QIODevice::Text)) {
        const QString &contents = QString::fromUtf8(reader.data());
        JsonValue *json = JsonValue::create(contents);
        if (json && json->kind() == JsonValue::Object)
            return new JsonSchema(json->toObject(), this);
    }

    return 0;
}
