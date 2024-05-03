// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QJsonArray>
#include <QStack>

QT_BEGIN_NAMESPACE
struct QJsonParseError;
QT_END_NAMESPACE

using JsonKey = std::variant<int, QString>; // Key can be either int (index) or string (property name)

using JsonKeyChain = QList<JsonKey>; // A chain of keys leading to a specific json value

namespace QmlDesigner {

struct CollectionObject
{
    QString name;
    QJsonArray array = {};
    JsonKeyChain keyChain = {};
    QStringList propertyOrder = {};
};

class JsonCollectionParser : public QmlJS::AST::Visitor
{
public:
    static QList<CollectionObject> parseCollectionObjects(const QByteArray &json,
                                                          QJsonParseError *error = nullptr);

private:
    JsonCollectionParser(const QString &jsonContent, const QList<JsonKeyChain> &keyChains);

    bool visit(QmlJS::AST::ObjectPattern *objectPattern) override;
    void endVisit(QmlJS::AST::ObjectPattern *objectPattern) override;

    bool visit(QmlJS::AST::PatternProperty *patternProperty) override;
    void endVisit(QmlJS::AST::PatternProperty *patternProperty) override;

    bool visit(QmlJS::AST::PatternElementList *patternElementList) override;
    void endVisit(QmlJS::AST::PatternElementList *patternElementList) override;

    bool visit(QmlJS::AST::PatternElement *patternElement) override;

    void checkPropertyUpdates(QStack<JsonKey> stack, const QStringList &objectProperties);

    void throwRecursionDepthError() override;

    QStack<JsonKey> keyStack;
    QStack<QStringList> propertyOrderStack;
    QMap<JsonKeyChain, QStringList> collectionPaths; // Key chains, Priorities
};

} // namespace QmlDesigner
