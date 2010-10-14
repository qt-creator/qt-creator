/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/


#include <QScopedPointer>
#include <QLatin1String>
#include <QGraphicsObject>

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljslookupcontext.h>
#include <qmljstools/qmljsrefactoringchanges.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <QtTest>

using namespace QmlJS;
using namespace QmlJSTools;

class tst_Basic : public QObject
{
    Q_OBJECT
public:
    tst_Basic();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMetaInfo();
};

class TestModelManager : public Internal::ModelManager
{
public:
    virtual ~TestModelManager() {}
    void loadFile(QString fileName)
    {
        refreshSourceFiles(QStringList() << fileName, false).waitForFinished();
    }
};

tst_Basic::tst_Basic()
{

}

void tst_Basic::initTestCase()
{
}

void tst_Basic::cleanupTestCase()
{
}

void tst_Basic::testMetaInfo()
{
    TestModelManager modelManager;
    modelManager.loadFile(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");

    Snapshot snapshot = modelManager.snapshot();
    Document::Ptr doc = snapshot.document(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");
    QVERIFY(doc && doc->isQmlDocument());

    LookupContext::Ptr lookupContext = LookupContext::create(doc, snapshot, QList<AST::Node>());

    Interpreter::Value *v = lookupContext->evaluate();
    Interpreter::ObjectValue *ov = lookupContext->context()->lookupType(doc, QStringList() << "Qt" << "Rectangle");
    qDebug() << ov->className();
    doc->bind(); // per-doc data

    qDebug() << lookupContext->context()->scopeChain().qmlTypes->importInfo("Rectangle", lookupContext->context()).name();

//    Snapshot snapshot;
//    QmlJSRefactoringChanges changes(&modelManager, snapshot);
//    QmlJSRefactoringFile file = changes.file(QString(QTCREATORDIR) + "/tests/auto/qml/qmldesigner/data/fx/usingmybutton.qml");
//    QVERIFY(file.isValid());

//    snapshot.insert(file.qmljsDocument());

}

QTEST_MAIN(tst_Basic);

#include "tst_basic.moc"
