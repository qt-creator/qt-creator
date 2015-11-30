/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

    const QString welcomescreenRootPath = Core::ICore::resourcePath() + QLatin1String("/welcomescreen/welcomescreen.qml");
    modelManager->updateSourceFiles(QStringList(welcomescreenRootPath), false);
    modelManager->joinAllThreads();

    Snapshot snapshot = modelManager->snapshot();
    Document::Ptr doc = snapshot.document(welcomescreenRootPath);
    QVERIFY(doc && doc->isQmlDocument());

    ContextPtr context = Link(snapshot, ViewerContext(), LibraryInfo())();
    QVERIFY(context);

    const CppComponentValue *rectangleValue = context->valueOwner()->cppQmlTypes().objectByQualifiedName(
                QLatin1String("QtQuick"), QLatin1String("QDeclarativeRectangle"), LanguageUtils::ComponentVersion(2, 1));
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
