// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsoncheck.h"

#include <qmljs/parser/qmljsast_p.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QLatin1String>
#include <QRegularExpression>

#include <cmath>

using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;
using namespace Utils;

namespace QmlJS {

JsonCheck::JsonCheck(Document::Ptr doc)
    : m_doc(doc)
    , m_schema(nullptr)
{
    QTC_CHECK(m_doc->ast());
}

JsonCheck::~JsonCheck()
{}

QList<Message> JsonCheck::operator ()(JsonSchema *schema)
{
    QTC_ASSERT(schema, return {});

    m_schema = schema;

    m_analysis.push(AnalysisData());
    processSchema(m_doc->ast());
    const AnalysisData &analysis = m_analysis.pop();

    return analysis.m_messages;
}

bool JsonCheck::preVisit(Node *ast)
{
    if (!m_firstLoc.isValid()) {
        if (ExpressionNode *expr = ast->expressionCast())
            m_firstLoc = expr->firstSourceLocation();
    }

    m_analysis.push(AnalysisData());

    return true;
}

void JsonCheck::postVisit(Node *)
{
    const AnalysisData &previous = m_analysis.pop();
    if (previous.m_messages.isEmpty())
        analysis()->m_hasMatch = true;
    else
        analysis()->m_messages.append(previous.m_messages);
    analysis()->m_ranking += previous.m_ranking;
}

bool JsonCheck::visit(AST::TemplateLiteral *ast)
{
    Node::accept(ast->expression, this);
    return true;
}

bool JsonCheck::visit(ObjectPattern *ast)
{
    if (!proceedCheck(JsonValue::Object, ast->lbraceToken))
        return false;

    analysis()->boostRanking();

    const QStringList &properties = m_schema->properties();
    if (properties.isEmpty())
        return false;

    QSet<QString> propertiesFound;
    for (PatternPropertyList *it = ast->properties; it; it = it->next) {
        PatternProperty *assignment = AST::cast<AST::PatternProperty *>(it->property);
        StringLiteralPropertyName *literalName = cast<StringLiteralPropertyName *>(assignment->name);
        if (literalName) {
            const QString &propertyName = literalName->id.toString();
            if (m_schema->hasPropertySchema(propertyName)) {
                analysis()->boostRanking();
                propertiesFound.insert(propertyName);
                // Sec. 5.2: "... each property definition's value MUST be a schema..."
                m_schema->enterNestedPropertySchema(propertyName);
                processSchema(assignment->initializer);
                m_schema->leaveNestedSchema();
            } else {
                analysis()->m_messages.append(Message(ErrInvalidPropertyName,
                                                      literalName->firstSourceLocation(),
                                                      propertyName, QString(),
                                                      false));
            }
        }  else {
            analysis()->m_messages.append(Message(ErrStringValueExpected,
                                                  assignment->name->firstSourceLocation(),
                                                  QString(), QString(),
                                                  false));
        }
    }

    QStringList missing;
    for (const QString &property : properties) {
        if (!propertiesFound.contains(property)) {
            m_schema->enterNestedPropertySchema(property);
            if (m_schema->required())
                missing.append(property);
            m_schema->leaveNestedSchema();
        }
    }
    if (!missing.isEmpty()) {
        analysis()->m_messages.append(Message(ErrMissingRequiredProperty,
                                              ast->firstSourceLocation(),
                                              missing.join(QLatin1String(", ")),
                                              QString(),
                                              false));
    } else {
        analysis()->boostRanking();
    }

    return false;
}

bool JsonCheck::visit(ArrayPattern *ast)
{
    if (!proceedCheck(JsonValue::Array, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    if (m_schema->hasItemSchema()) {
        // Sec. 5.5: "When this attribute value is a schema... all the items in the array MUST
        // be valid according to the schema."
        m_schema->enterNestedItemSchema();
        for (PatternElementList *element = ast->elements; element; element = element->next)
            processSchema(element->element->initializer);
        m_schema->leaveNestedSchema();
    } else if (m_schema->hasItemArraySchema()) {
        // Sec. 5.5: "When this attribute value is an array of schemas... each position in the
        // instance array MUST conform to the schema in the corresponding position for this array."
        int current = 0;
        const int arraySize = m_schema->itemArraySchemaSize();
        for (PatternElementList *element = ast->elements; element; element = element->next, ++current) {
            if (current < arraySize) {
                if (m_schema->maybeEnterNestedArraySchema(current)) {
                    processSchema(element->element->initializer);
                    m_schema->leaveNestedSchema();
                } else {
                    Node::accept(element->element->initializer, this);
                }
            } else {
                // TODO: Handle additionalItems.
            }
        }
        if (current < arraySize
                || (current > arraySize && !m_schema->hasAdditionalItems())) {
            analysis()->m_messages.append(Message(ErrInvalidArrayValueLength,
                                                  ast->firstSourceLocation(),
                                                  QString::number(arraySize), QString(), false));
        }
    }

    return false;
}

bool JsonCheck::visit(NullExpression *ast)
{
    if (proceedCheck(JsonValue::Null, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(TrueLiteral *ast)
{
    if (!proceedCheck(JsonValue::Boolean, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(FalseLiteral *ast)
{
    if (!proceedCheck(JsonValue::Boolean, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    return false;
}

bool JsonCheck::visit(NumericLiteral *ast)
{
    double dummy;
    if (std::abs(std::modf(ast->value, &dummy)) > 0.000000001) {
        if (!proceedCheck(JsonValue::Double, ast->firstSourceLocation()))
            return false;
    } else if (!proceedCheck(JsonValue::Int, ast->firstSourceLocation())) {
        return false;
    }

    analysis()->boostRanking();

    if (m_schema->hasMinimum()) {
        double minValue = m_schema->minimum();
        if (ast->value < minValue) {
            analysis()->m_messages.append(Message(ErrLargerNumberValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(minValue), QString(), false));
        } else if (m_schema->hasExclusiveMinimum()
                   && std::abs(ast->value - minValue) > 0.000000001) {
            analysis()->m_messages.append(Message(ErrMinimumNumberValueIsExclusive,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
        } else {
            analysis()->boostRanking();
        }
    }

    if (m_schema->hasMaximum()) {
        double maxValue = m_schema->maximum();
        if (ast->value > maxValue) {
            analysis()->m_messages.append(Message(ErrSmallerNumberValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(maxValue), QString(), false));
        } else if (m_schema->hasExclusiveMaximum()) {
            analysis()->m_messages.append(Message(ErrMaximumNumberValueIsExclusive,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
        } else {
            analysis()->boostRanking();
        }
    }

    return false;
}

bool JsonCheck::visit(StringLiteral *ast)
{
    if (!proceedCheck(JsonValue::String, ast->firstSourceLocation()))
        return false;

    analysis()->boostRanking();

    const QStringView literal = ast->value;

    const QString &pattern = m_schema->pattern();
    if (!pattern.isEmpty()) {
        const QRegularExpression regExp(pattern);
        if (regExp.match(literal.toString()).hasMatch()) {
            analysis()->m_messages.append(Message(ErrInvalidStringValuePattern,
                                                  ast->firstSourceLocation(),
                                                  QString(), QString(), false));
            return false;
        }
        analysis()->boostRanking(3); // Treat string patterns with higher weight.
    }

    int expectedLength = m_schema->minimumLength();
    if (expectedLength != -1) {
        if (literal.length() < expectedLength) {
            analysis()->m_messages.append(Message(ErrLongerStringValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(expectedLength), QString(),
                                                  false));
        } else {
            analysis()->boostRanking();
        }
    }

    expectedLength = m_schema->maximumLength();
    if (expectedLength != -1) {
        if (literal.length() > expectedLength) {
            analysis()->m_messages.append(Message(ErrShorterStringValueExpected,
                                                  ast->firstSourceLocation(),
                                                  QString::number(expectedLength), QString(),
                                                  false));
        } else {
            analysis()->boostRanking();
        }
    }

    return false;
}

void JsonCheck::throwRecursionDepthError()
{
    analysis()->m_messages.append(Message(ErrHitMaximumRecursion,
                                          SourceLocation(),
                                          QString(), QString(), false));

}

static QString formatExpectedTypes(QStringList all)
{
    all.removeDuplicates();
    return all.join(QLatin1String(", or "));
}

void JsonCheck::processSchema(Node *ast)
{
    if (m_schema->hasTypeSchema()) {
        m_schema->enterNestedTypeSchema();
        processSchema(ast);
        m_schema->leaveNestedSchema();
    } else if (m_schema->hasUnionSchema()) {
        // Sec. 5.1: "... value is valid if it is of the same type as one of the simple
        // type definitions, or valid by one of the schemas, in the array."
        int bestRank = 0;
        QList<StaticAnalysis::Message> bestErrorGuess;
        int current = 0;
        const int unionSize = m_schema->unionSchemaSize();
        m_analysis.push(AnalysisData());
        for (; current < unionSize; ++current) {
            if (m_schema->maybeEnterNestedUnionSchema(current)) {
                processSchema(ast);
                m_schema->leaveNestedSchema();
            } else {
                Node::accept(ast, this);
            }
            if (analysis()->m_hasMatch)
                break;

            if (analysis()->m_ranking >= bestRank) {
                bestRank = analysis()->m_ranking;
                bestErrorGuess = analysis()->m_messages;
            }
            analysis()->m_ranking = 0;
            analysis()->m_messages.clear();
        }
        m_analysis.pop();

        if (current == unionSize) {
            // When we don't have a match for a union typed schema, we try to "guess" which
            // particular item from the union the user tried to represent. The one with the best
            // ranking wins.
            if (bestRank > 0) {
                analysis()->m_messages.append(bestErrorGuess);
            } else {
                analysis()->m_messages.append(Message(ErrDifferentValueExpected,
                                                      ast->firstSourceLocation(),
                                                      formatExpectedTypes(m_schema->validTypes()),
                                                      QString(),
                                                      false));
            }
        }
    } else {
        Node::accept(ast, this);
    }
}

bool JsonCheck::proceedCheck(JsonValue::Kind kind, const SourceLocation &location)
{
    if (!m_firstLoc.isValid())
        return false;

    if (!m_schema->isTypeConstrained())
        return false;

    if (!m_schema->acceptsType(JsonValue::kindToString(kind))) {
        analysis()->m_messages.append(Message(ErrDifferentValueExpected,
                                              location,
                                              formatExpectedTypes(m_schema->validTypes()),
                                              QString(),
                                              false));
        return false;
    }

    return true;
}

JsonCheck::AnalysisData *JsonCheck::analysis()
{
    return &m_analysis.top();
}


JsonMemoryPool::~JsonMemoryPool()
{
    for (char *obj : std::as_const(_objs)) {
        reinterpret_cast<JsonValue *>(obj)->~JsonValue();
        delete[] obj;
    }
}

JsonValue::JsonValue(Kind kind)
    : m_kind(kind)
{}

JsonValue::~JsonValue() = default;

JsonValue *JsonValue::create(const QString &s, JsonMemoryPool *pool)
{
    const QJsonDocument document = QJsonDocument::fromJson(s.toUtf8());
    if (document.isNull())
        return nullptr;

    return build(document.toVariant(), pool);
}

void *JsonValue::operator new(size_t size, JsonMemoryPool *pool)
{ return pool->allocate(size); }

void JsonValue::operator delete(void *)
{ }

void JsonValue::operator delete(void *, JsonMemoryPool *)
{ }

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

    return QLatin1String("unknown");
}

JsonValue *JsonValue::build(const QVariant &variant, JsonMemoryPool *pool)
{
    switch (variant.typeId()) {

    case QVariant::List: {
        auto newValue = new (pool) JsonArrayValue;
        const QList<QVariant> list = variant.toList();
        for (const QVariant &element : list)
            newValue->addElement(build(element, pool));
        return newValue;
    }

    case QVariant::Map: {
        auto newValue = new (pool) JsonObjectValue;
        const QVariantMap variantMap = variant.toMap();
        for (QVariantMap::const_iterator it = variantMap.begin(); it != variantMap.end(); ++it)
            newValue->addMember(it.key(), build(it.value(), pool));
        return newValue;
    }

    case QVariant::String:
        return new (pool) JsonStringValue(variant.toString());

    case QVariant::Int:
        return new (pool) JsonIntValue(variant.toInt());

    case QVariant::Double:
        return new (pool) JsonDoubleValue(variant.toDouble());

    case QVariant::Bool:
        return new (pool) JsonBooleanValue(variant.toBool());

    case QVariant::Invalid:
        return new (pool) JsonNullValue;

    default:
        break;
    }

    return nullptr;
}


///////////////////////////////////////////////////////////////////////////////

QString JsonSchema::kType() { return QStringLiteral("type"); }
QString JsonSchema::kProperties() { return QStringLiteral("properties"); }
QString JsonSchema::kPatternProperties() { return QStringLiteral("patternProperties"); }
QString JsonSchema::kAdditionalProperties() { return QStringLiteral("additionalProperties"); }
QString JsonSchema::kItems() { return QStringLiteral("items"); }
QString JsonSchema::kAdditionalItems() { return QStringLiteral("additionalItems"); }
QString JsonSchema::kRequired() { return QStringLiteral("required"); }
QString JsonSchema::kDependencies() { return QStringLiteral("dependencies"); }
QString JsonSchema::kMinimum() { return QStringLiteral("minimum"); }
QString JsonSchema::kMaximum() { return QStringLiteral("maximum"); }
QString JsonSchema::kExclusiveMinimum() { return QStringLiteral("exclusiveMinimum"); }
QString JsonSchema::kExclusiveMaximum() { return QStringLiteral("exclusiveMaximum"); }
QString JsonSchema::kMinItems() { return QStringLiteral("minItems"); }
QString JsonSchema::kMaxItems() { return QStringLiteral("maxItems"); }
QString JsonSchema::kUniqueItems() { return QStringLiteral("uniqueItems"); }
QString JsonSchema::kPattern() { return QStringLiteral("pattern"); }
QString JsonSchema::kMinLength() { return QStringLiteral("minLength"); }
QString JsonSchema::kMaxLength() { return QStringLiteral("maxLength"); }
QString JsonSchema::kTitle() { return QStringLiteral("title"); }
QString JsonSchema::kDescription() { return QStringLiteral("description"); }
QString JsonSchema::kExtends() { return QStringLiteral("extends"); }
QString JsonSchema::kRef() { return QStringLiteral("$ref"); }

JsonSchema::JsonSchema(JsonObjectValue *rootObject, const JsonSchemaManager *manager)
    : m_manager(manager)
{
    enter(rootObject);
}

bool JsonSchema::isTypeConstrained() const
{
    // Simple types
    if (JsonStringValue *sv = getStringValue(kType(), currentValue()))
        return isCheckableType(sv->value());

    // Union types
    if (JsonArrayValue *av = getArrayValue(kType(), currentValue())) {
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
    if (JsonStringValue *sv = getStringValue(kType(), currentValue()))
        return typeMatches(sv->value(), type);

    // Union types
    if (JsonArrayValue *av = getArrayValue(kType(), currentValue())) {
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

    if (JsonStringValue *sv = getStringValue(kType(), v))
        all.append(sv->value());

    if (JsonObjectValue *ov = getObjectValue(kType(), v))
        return validTypes(ov);

    if (JsonArrayValue *av = getArrayValue(kType(), v)) {
        const QList<JsonValue *> elements = av->elements();
        for (JsonValue *v : elements) {
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
    return s == QLatin1String("string")
        || s == QLatin1String("number")
        || s == QLatin1String("integer")
        || s == QLatin1String("boolean")
        || s == QLatin1String("object")
        || s == QLatin1String("array")
        || s == QLatin1String("null");
}

QStringList JsonSchema::validTypes() const
{
    return validTypes(currentValue());
}

bool JsonSchema::hasTypeSchema() const
{
    return getObjectValue(kType(), currentValue());
}

void JsonSchema::enterNestedTypeSchema()
{
    QTC_ASSERT(hasTypeSchema(), return);

    enter(getObjectValue(kType(), currentValue()));
}

QStringList JsonSchema::properties(JsonObjectValue *v) const
{
    using Members = QHash<QString, JsonValue *>;

    QStringList all;

    if (JsonObjectValue *ov = getObjectValue(kProperties(), v)) {
        const Members members = ov->members();
        const Members::ConstIterator cend = members.constEnd();
        for (Members::ConstIterator it = members.constBegin(); it != cend; ++it)
            if (hasPropertySchema(it.key()))
                all.append(it.key());
    }

    if (JsonObjectValue *base = resolveBase(v))
        all.append(properties(base));

    return all;
}

QStringList JsonSchema::properties() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Object)), return {});

    return properties(currentValue());
}

JsonObjectValue *JsonSchema::propertySchema(const QString &property,
                                                      JsonObjectValue *v) const
{
    if (JsonObjectValue *ov = getObjectValue(kProperties(), v)) {
        JsonValue *member = ov->member(property);
        if (member && member->kind() == JsonValue::Object)
            return member->toObject();
    }

    if (JsonObjectValue *base = resolveBase(v))
        return propertySchema(property, base);

    return nullptr;
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
 * An array schema is allowed to have its \e items specification in the form of
 * another schema
 * or in the form of an array of schemas [Sec. 5.5]. This functions checks whether this is case
 * in which the items are a schema.
 *
 * Returns whether or not the items from the array are a schema.
 */
bool JsonSchema::hasItemSchema() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return getObjectValue(kItems(), currentValue());
}

void JsonSchema::enterNestedItemSchema()
{
    QTC_ASSERT(hasItemSchema(), return);

    enter(getObjectValue(kItems(), currentValue()));
}

/*!
 * An array schema is allowed to have its \e items specification in the form of another schema
 * or in the form of an array of schemas [Sec. 5.5]. This functions checks whether this is case
 * in which the items are an array of schemas.
 *
 * Returns whether or not the items from the array are a an array of schemas.
 */
bool JsonSchema::hasItemArraySchema() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return getArrayValue(kItems(), currentValue());
}

int JsonSchema::itemArraySchemaSize() const
{
    QTC_ASSERT(hasItemArraySchema(), return false);

    return getArrayValue(kItems(), currentValue())->size();
}

/*!
 * When evaluating the items of an array it might be necessary to \e enter a
 * particular schema,
 * since this API assumes that there's always a valid schema in context (the one the user is
 * interested on). This shall only happen if the item at the supplied array index is of type
 * object, which is then assumed to be a schema.
 *
 * The function also marks the context as being inside an array evaluation.
 *
 * Returns whether it was necessary to enter a schema for the supplied
 * array \a index, false if index is out of bounds.
 */
bool JsonSchema::maybeEnterNestedArraySchema(int index)
{
    QTC_ASSERT(itemArraySchemaSize(), return false);
    QTC_ASSERT(index >= 0 && index < itemArraySchemaSize(), return false);

    JsonValue *v = getArrayValue(kItems(), currentValue())->elements().at(index);

    return maybeEnter(v, Array, index);
}

/*!
 * The type of a schema can be specified in the form of a union type, which is basically an
 * array of allowed types for the particular instance [Sec. 5.1]. This function checks whether
 * the current schema is one of such.
 *
 * Returns whether or not the current schema specifies a union type.
 */
bool JsonSchema::hasUnionSchema() const
{
    return getArrayValue(kType(), currentValue());
}

int JsonSchema::unionSchemaSize() const
{
    return getArrayValue(kType(), currentValue())->size();
}

/*!
 * When evaluating union types it might be necessary to enter a particular
 * schema, since this
 * API assumes that there's always a valid schema in context (the one the user is interested on).
 * This shall only happen if the item at the supplied union \a index, which is then assumed to be
 * a schema.
 *
 * The function also marks the context as being inside an union evaluation.
 *
 * Returns whether or not it was necessary to enter a schema for the
 * supplied union index.
 */
bool JsonSchema::maybeEnterNestedUnionSchema(int index)
{
    QTC_ASSERT(unionSchemaSize(), return false);
    QTC_ASSERT(index >= 0 && index < unionSchemaSize(), return false);

    JsonValue *v = getArrayValue(kType(), currentValue())->elements().at(index);

    return maybeEnter(v, Union, index);
}

void JsonSchema::leaveNestedSchema()
{
    QTC_ASSERT(!m_schemas.isEmpty(), return);

    leave();
}

bool JsonSchema::required() const
{
    if (JsonBooleanValue *bv = getBooleanValue(kRequired(), currentValue()))
        return bv->value();

    return false;
}

bool JsonSchema::hasMinimum() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    return getDoubleValue(kMinimum(), currentValue());
}

double JsonSchema::minimum() const
{
    QTC_ASSERT(hasMinimum(), return 0);

    return getDoubleValue(kMinimum(), currentValue())->value();
}

bool JsonSchema::hasExclusiveMinimum()
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    if (JsonBooleanValue *bv = getBooleanValue(kExclusiveMinimum(), currentValue()))
        return bv->value();

    return false;
}

bool JsonSchema::hasMaximum() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    return getDoubleValue(kMaximum(), currentValue());
}

double JsonSchema::maximum() const
{
    QTC_ASSERT(hasMaximum(), return 0);

    return getDoubleValue(kMaximum(), currentValue())->value();
}

bool JsonSchema::hasExclusiveMaximum()
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Int)), return false);

    if (JsonBooleanValue *bv = getBooleanValue(kExclusiveMaximum(), currentValue()))
        return bv->value();

    return false;
}

QString JsonSchema::pattern() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return QString());

    if (JsonStringValue *sv = getStringValue(kPattern(), currentValue()))
        return sv->value();

    return QString();
}

int JsonSchema::minimumLength() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return -1);

    if (JsonDoubleValue *dv = getDoubleValue(kMinLength(), currentValue()))
        return dv->value();

    return -1;
}

int JsonSchema::maximumLength() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::String)), return -1);

    if (JsonDoubleValue *dv = getDoubleValue(kMaxLength(), currentValue()))
        return dv->value();

    return -1;
}

bool JsonSchema::hasAdditionalItems() const
{
    QTC_ASSERT(acceptsType(JsonValue::kindToString(JsonValue::Array)), return false);

    return currentValue()->member(kAdditionalItems());
}

bool JsonSchema::maybeSchemaName(const QString &s)
{
    if (s.isEmpty() || s == QLatin1String("any"))
        return false;

    return !isCheckableType(s);
}

JsonObjectValue *JsonSchema::rootValue() const
{
    QTC_ASSERT(!m_schemas.isEmpty(), return nullptr);

    return m_schemas.first().m_value;
}

JsonObjectValue *JsonSchema::currentValue() const
{
    QTC_ASSERT(!m_schemas.isEmpty(), return nullptr);

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
    if (JsonStringValue *sv = getStringValue(kRef(), ov)) {
        JsonSchema *referenced = m_manager->schemaByName(sv->value());
        if (referenced)
            return referenced->rootValue();
    }

    return ov;
}

JsonObjectValue *JsonSchema::resolveBase(JsonObjectValue *ov) const
{
    if (JsonValue *v = ov->member(kExtends())) {
        if (v->kind() == JsonValue::String) {
            JsonSchema *schema = m_manager->schemaByName(v->toString()->value());
            if (schema)
                return schema->rootValue();
        } else if (v->kind() == JsonValue::Object) {
            return resolveReference(v->toObject());
        }
    }

    return nullptr;
}

JsonStringValue *JsonSchema::getStringValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return nullptr;

    return v->toString();
}

JsonObjectValue *JsonSchema::getObjectValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return nullptr;

    return v->toObject();
}

JsonBooleanValue *JsonSchema::getBooleanValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return nullptr;

    return v->toBoolean();
}

JsonArrayValue *JsonSchema::getArrayValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return nullptr;

    return v->toArray();
}

JsonDoubleValue *JsonSchema::getDoubleValue(const QString &name, JsonObjectValue *value)
{
    JsonValue *v = value->member(name);
    if (!v)
        return nullptr;

    return v->toDouble();
}


///////////////////////////////////////////////////////////////////////////////

JsonSchemaManager::JsonSchemaManager(const QStringList &searchPaths)
    : m_searchPaths(searchPaths)
{
    for (const QString &path : searchPaths) {
        QDir dir(path);
        if (!dir.exists())
            continue;
        dir.setNameFilters(QStringList(QLatin1String("*.json")));
        const QList<QFileInfo> entries = dir.entryInfoList();
        for (const QFileInfo &fi : entries)
            m_schemas.insert(fi.baseName(), JsonSchemaData(fi.absoluteFilePath()));
    }
}

JsonSchemaManager::~JsonSchemaManager()
{
    for (const JsonSchemaData &schemaData : std::as_const(m_schemas))
        delete schemaData.m_schema;
}

/*!
 * Tries to find a JSON schema to validate \a fileName against. According
 * to the specification, how the schema/instance association is done is implementation defined.
 * Currently we use a quite naive approach which is simply based on file names. Specifically,
 * if one opens a foo.json file we'll look for a schema named foo.json. We should probably
 * investigate alternative settings later.
 *
 * Returns a valid schema or 0.
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
        for (const QString &path : m_searchPaths) {
            QFileInfo candidate(path + baseName + ".json");
            if (candidate.exists()) {
                m_schemas.insert(baseName, candidate.absoluteFilePath());
                break;
            }
        }
    }

    it = m_schemas.find(baseName);
    if (it == m_schemas.end())
        return nullptr;

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
    if (reader.fetch(FilePath::fromString(schemaFileName), QIODevice::Text)) {
        const QString &contents = QString::fromUtf8(reader.data());
        JsonValue *json = JsonValue::create(contents, &m_pool);
        if (json && json->kind() == JsonValue::Object)
            return new JsonSchema(json->toObject(), this);
    }

    return nullptr;
}

} // QmlJs
