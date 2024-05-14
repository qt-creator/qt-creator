// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionjsonparser.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <utils/algorithm.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace QmlDesigner {

/**
 * @brief A json object is a plain object, if it has only primitive properties (not arrays or objects)
 * @return true if @param jsonObject is a plain object
 */
inline static bool isPlainObject(const QJsonObject &jsonObj)
{
    return !Utils::anyOf(jsonObj, [](const QJsonValueConstRef &val) {
        return val.isArray() || val.isObject();
    });
}

static bool isPlainObject(const QJsonValueConstRef &value)
{
    if (!value.isObject())
        return false;
    return isPlainObject(value.toObject());
}

static QJsonArray parsePlainObject(const QJsonObject &jsonObj)
{
    QJsonObject result;
    auto item = jsonObj.constBegin();
    const auto itemEnd = jsonObj.constEnd();
    while (item != itemEnd) {
        QJsonValueConstRef ref = item.value();
        if (!ref.isArray() && !ref.isObject())
            result.insert(item.key(), ref);
        ++item;
    }
    if (!result.isEmpty())
        return QJsonArray{result};

    return {};
}

static QJsonArray parseArray(const QJsonArray &array,
                             QList<CollectionObject> &plainCollections,
                             JsonKeyChain &chainTracker)
{
    chainTracker.append(0);
    QJsonArray plainArray;
    int i = -1;
    for (const QJsonValueConstRef &item : array) {
        chainTracker.last() = ++i;
        if (isPlainObject(item)) {
            const QJsonObject plainObject = item.toObject();
            if (plainObject.count())
                plainArray.append(plainObject);
        } else if (item.isArray()) {
            parseArray(item.toArray(), plainCollections, chainTracker);
        }
    }
    chainTracker.removeLast();
    return plainArray;
}

static void parseObject(const QJsonObject &jsonObj,
                        QList<CollectionObject> &plainCollections,
                        JsonKeyChain &chainTracker)
{
    chainTracker.append(QString{});
    auto item = jsonObj.constBegin();
    const auto itemEnd = jsonObj.constEnd();
    while (item != itemEnd) {
        chainTracker.last() = item.key();
        QJsonValueConstRef ref = item.value();
        QJsonArray parsedArray;
        if (ref.isArray()) {
            parsedArray = parseArray(ref.toArray(), plainCollections, chainTracker);
        } else if (ref.isObject()) {
            if (isPlainObject(ref))
                parsedArray = parsePlainObject(ref.toObject());
            else
                parseObject(ref.toObject(), plainCollections, chainTracker);
        }
        if (!parsedArray.isEmpty())
            plainCollections.append({item.key(), parsedArray, chainTracker});
        ++item;
    }
    chainTracker.removeLast();
}

static QList<CollectionObject> parseDocument(const QJsonDocument &document,
                                             const QString &defaultName = "Model")
{
    QList<CollectionObject> plainCollections;
    JsonKeyChain chainTracker;
    if (document.isObject()) {
        const QJsonObject documentObject = document.object();
        if (isPlainObject(documentObject)) {
            QJsonArray parsedArray = parsePlainObject(documentObject);
            if (!parsedArray.isEmpty())
                plainCollections.append({defaultName, parsedArray});
        } else {
            parseObject(document.object(), plainCollections, chainTracker);
        }
    } else {
        QJsonArray parsedArray = parseArray(document.array(), plainCollections, chainTracker);
        if (!parsedArray.isEmpty())
            plainCollections.append({defaultName, parsedArray, {0}});
    }
    return plainCollections;
}

QList<CollectionObject> JsonCollectionParser::parseCollectionObjects(const QByteArray &json,
                                                                     QJsonParseError *error)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (error)
        *error = parseError;

    if (parseError.error != QJsonParseError::NoError)
        return {};

    QList<CollectionObject> allCollections = parseDocument(document);
    QList<JsonKeyChain> keyChains = Utils::transform(allCollections, [](const CollectionObject &obj) {
        return obj.keyChain;
    });

    JsonCollectionParser jsonVisitor(QString::fromLatin1(json), keyChains);

    for (CollectionObject &collection : allCollections)
        collection.propertyOrder = jsonVisitor.collectionPaths.value(collection.keyChain);

    return allCollections;
}

JsonCollectionParser::JsonCollectionParser(const QString &jsonContent,
                                           const QList<JsonKeyChain> &keyChains)
{
    for (const JsonKeyChain &chain : keyChains)
        collectionPaths.insert(chain, {});

    QmlJS::Document::MutablePtr newDoc = QmlJS::Document::create(Utils::FilePath::fromString(
                                                                     "<expression>"),
                                                                 QmlJS::Dialect::Json);

    newDoc->setSource(jsonContent);
    newDoc->parseExpression();

    if (!newDoc->isParsedCorrectly())
        return;

    newDoc->ast()->accept(this);
}

bool JsonCollectionParser::visit([[maybe_unused]] QmlJS::AST::ObjectPattern *objectPattern)
{
    propertyOrderStack.push({});
    return true;
}

void JsonCollectionParser::endVisit([[maybe_unused]] QmlJS::AST::ObjectPattern *objectPattern)

{
    if (!propertyOrderStack.isEmpty()) {
        QStringList objectProperties = propertyOrderStack.top();
        propertyOrderStack.pop();
        checkPropertyUpdates(keyStack, objectProperties);
    }
}

bool JsonCollectionParser::visit(QmlJS::AST::PatternProperty *patternProperty)
{
    const QString propertyName = patternProperty->name->asString();
    if (!propertyOrderStack.isEmpty())
        propertyOrderStack.top().append(propertyName);

    keyStack.push(propertyName);
    return true;
}

void JsonCollectionParser::endVisit(QmlJS::AST::PatternProperty *patternProperty)
{
    const QString propertyName = patternProperty->name->asString();

    if (auto curIndex = std::get_if<QString>(&keyStack.top())) {
        if (*curIndex == propertyName)
            keyStack.pop();
    }
}

bool JsonCollectionParser::visit([[maybe_unused]] QmlJS::AST::PatternElementList *patternElementList)
{
    keyStack.push(-1);
    return true;
}

void JsonCollectionParser::endVisit([[maybe_unused]] QmlJS::AST::PatternElementList *patternElementList)
{
    if (std::get_if<int>(&keyStack.top()))
        keyStack.pop();
}

bool JsonCollectionParser::visit([[maybe_unused]] QmlJS::AST::PatternElement *patternElement)
{
    if (auto curIndex = std::get_if<int>(&keyStack.top()))
        *curIndex += 1;
    return true;
}

void JsonCollectionParser::checkPropertyUpdates(QStack<JsonKey> stack,
                                                const QStringList &objectProperties)
{
    bool shouldUpdate = collectionPaths.contains(stack);
    if (!shouldUpdate && !stack.isEmpty()) {
        if (std::get_if<int>(&stack.top())) {
            stack.pop();
            shouldUpdate = collectionPaths.contains(stack);
        }
    }
    if (!shouldUpdate)
        return;

    QStringList propertyList = collectionPaths.value(stack);
    QSet<QString> allKeys;
    for (const QString &val : std::as_const(propertyList))
        allKeys.insert(val);

    std::optional<QString> prevVal;
    for (const QString &val : objectProperties) {
        if (!allKeys.contains(val)) {
            if (prevVal.has_value()) {
                const int idx = propertyList.indexOf(prevVal);
                propertyList.insert(idx + 1, val);
            } else {
                propertyList.append(val);
            }
            allKeys.insert(val);
        }
        prevVal = val;
    }
    collectionPaths.insert(stack, propertyList);
}

void JsonCollectionParser::throwRecursionDepthError()
{
    qWarning() << __FUNCTION__ << "Recursion Depth Error";
}

} // namespace QmlDesigner
