/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmljstoolsplugin.h"

#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>
#include <QApplication>
#include <QSettings>
#include <QFileInfo>

#include <coreplugin/icore.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljslink.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljstools/qmljsrefactoringchanges.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <QtTest>

using namespace QmlJS;
using namespace QmlJSTools;

void QmlJSTools::Internal::QmlJSToolsPlugin::test_basic()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    const QString welcomescreenRootPath = Core::ICore::resourcePath() + "/welcomescreen/welcomescreen.qml";
    modelManager->updateSourceFiles(QStringList(welcomescreenRootPath), false);
    modelManager->joinAllThreads();

    Snapshot snapshot = modelManager->snapshot();
    Document::Ptr doc = snapshot.document(welcomescreenRootPath);
    QVERIFY(doc && doc->isQmlDocument());

    ContextPtr context = Link(snapshot, QStringList(), LibraryInfo())();
    QVERIFY(context);

    const CppComponentValue *rectangleValue = context->valueOwner()->cppQmlTypes().objectByQualifiedName(
                "QtQuick", "QDeclarative1Rectangle", LanguageUtils::ComponentVersion(1, 0));
    QVERIFY(rectangleValue);
    QVERIFY(!rectangleValue->isWritable("border"));
    QVERIFY(rectangleValue->hasProperty("border"));
    QVERIFY(rectangleValue->isPointer("border"));
    QVERIFY(rectangleValue->isWritable("color"));
    QVERIFY(!rectangleValue->isPointer("color"));

    const ObjectValue *ovItem = context->lookupType(doc.data(), QStringList() << "Item");
    QVERIFY(ovItem);
    QCOMPARE(ovItem->className(), QString("Item"));
    QCOMPARE(context->imports(doc.data())->info("Item", context.data()).name(), QString("QtQuick"));

    const ObjectValue *ovProperty = context->lookupType(doc.data(), QStringList() << "Item" << "states");
    QVERIFY(ovProperty);
    QCOMPARE(ovProperty->className(), QString("State"));

    const CppComponentValue * qmlItemValue = value_cast<CppComponentValue>(ovItem);
    QVERIFY(qmlItemValue);
    QCOMPARE(qmlItemValue->defaultPropertyName(), QString("data"));
    QCOMPARE(qmlItemValue->propertyType("state"), QString("string"));

    const ObjectValue *ovState = context->lookupType(doc.data(), QStringList() << "State");
    const CppComponentValue * qmlState2Value = value_cast<CppComponentValue>(ovState);
    QCOMPARE(qmlState2Value->className(), QString("State"));

    const ObjectValue *ovImage = context->lookupType(doc.data(), QStringList() << "Image");
    const CppComponentValue * qmlImageValue = value_cast<CppComponentValue>(ovImage);
    QCOMPARE(qmlImageValue->className(), QString("Image"));
    QCOMPARE(qmlImageValue->propertyType("source"), QString("QUrl"));
}
