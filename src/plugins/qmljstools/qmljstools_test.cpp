// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljstoolsplugin.h"

#include <QLatin1String>

#include <coreplugin/icore.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsvalueowner.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <QtTest>

using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

void QmlJSToolsPlugin::test_basic()
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    const Utils::FilePath qmlFilePath = Core::ICore::resourcePath(
                                        "qmldesigner/itemLibraryQmlSources/ItemDelegate.qml");
    modelManager->updateSourceFiles(QList<Utils::FilePath>({qmlFilePath}), false);
    modelManager->test_joinAllThreads();

    Snapshot snapshot = modelManager->snapshot();
    Document::Ptr doc = snapshot.document(qmlFilePath);
    QVERIFY(doc && doc->isQmlDocument());

    ContextPtr context = Link(snapshot, ViewerContext(), LibraryInfo())();
    QVERIFY(context);

    const CppComponentValue *rectangleValue = context->valueOwner()->cppQmlTypes().objectByQualifiedName(
                QLatin1String("QtQuick"), QLatin1String("QDeclarativeRectangle"), LanguageUtils::ComponentVersion(2, 15));
    QVERIFY(rectangleValue);
    QVERIFY(!rectangleValue->isWritable(QLatin1String("border")));
    QVERIFY(rectangleValue->hasProperty(QLatin1String("border")));
    QVERIFY(rectangleValue->isPointer(QLatin1String("border")));
    QVERIFY(rectangleValue->isWritable(QLatin1String("color")));
    QVERIFY(!rectangleValue->isPointer(QLatin1String("color")));

    const ObjectValue *ovItem = context->lookupType(doc.data(), QStringList(QLatin1String("Item")));
    QVERIFY(ovItem);
    QCOMPARE(ovItem->className(), QLatin1String("Item"));
    QCOMPARE(context->imports(doc.data())->info(QLatin1String("Item"), context.data()).name(), QLatin1String("QtQuick"));

    const ObjectValue *ovProperty = context->lookupType(doc.data(), QStringList() << QLatin1String("Item") << QLatin1String("states"));
    QVERIFY(ovProperty);
    QCOMPARE(ovProperty->className(), QLatin1String("State"));

    const CppComponentValue *qmlItemValue = value_cast<CppComponentValue>(ovItem);
    QVERIFY(qmlItemValue);
    QCOMPARE(qmlItemValue->defaultPropertyName(), QLatin1String("data"));
    QCOMPARE(qmlItemValue->propertyType(QLatin1String("state")), QLatin1String("string"));

    const ObjectValue *ovState = context->lookupType(doc.data(), QStringList(QLatin1String("State")));
    const CppComponentValue *qmlState2Value = value_cast<CppComponentValue>(ovState);
    QCOMPARE(qmlState2Value->className(), QLatin1String("State"));

    const ObjectValue *ovImage = context->lookupType(doc.data(), QStringList(QLatin1String("Image")));
    const CppComponentValue *qmlImageValue = value_cast<CppComponentValue>(ovImage);
    QCOMPARE(qmlImageValue->className(), QLatin1String("Image"));
    QCOMPARE(qmlImageValue->propertyType(QLatin1String("source")), QLatin1String("QUrl"));
}

} // namespace Internal
} // namespace QmlJSTools
