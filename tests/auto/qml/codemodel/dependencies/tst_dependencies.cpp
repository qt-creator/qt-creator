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

#include <QString>
#include <QStringList>
#include <QFutureInterface>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLibraryInfo>
#include <QtTest>

#include <QDebug>

#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsviewercontext.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsimportdependencies.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljssemanticinfo.h>
#include <extensionsystem/pluginmanager.h>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJS::StaticAnalysis;

static Document::MutablePtr readDocument(const QString &path)
{
    Document::MutablePtr doc = Document::create(path, Dialect::Qml);
    QFile file(doc->fileName());
    file.open(QFile::ReadOnly | QFile::Text);
    doc->setSource(file.readAll());
    file.close();
    doc->parse();
    return doc;
}

class tst_Dependencies : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_data();
    void test();

private:
    QString m_path;
    QStringList m_basePaths;
};

void tst_Dependencies::initTestCase()
{
    QLoggingCategory::setFilterRules(QLatin1String("qtc.*.debug=false"));
    m_path = QCoreApplication::applicationDirPath() + QLatin1Literal("/samples");

    m_basePaths.append(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath));

    if (!ModelManagerInterface::instance())
        new ModelManagerInterface;

    if (!ExtensionSystem::PluginManager::instance())
        new ExtensionSystem::PluginManager;
}

void tst_Dependencies::test_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("nSemanticMessages");
    QTest::addColumn<int>("nStaticMessages");

    QTest::newRow("ApplicationWindow")
            << QString(m_path + QLatin1String("/001_ApplicationWindow.qml"))
            << 0
            << 0;
    QTest::newRow("Window")
            << QString(m_path + QLatin1String("/002_Window.qml"))
            << 0
            << 1;
}

void tst_Dependencies::test()
{
    QFETCH(QString, filename);
    QFETCH(int, nSemanticMessages);
    QFETCH(int, nStaticMessages);

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    QFutureInterface<void> result;
    PathsAndLanguages lPaths;
    QStringList paths(m_basePaths);
    paths << m_path;
    for (auto p: paths)
        lPaths.maybeInsert(Utils::FileName::fromString(p), Dialect::Qml);
    ModelManagerInterface::importScan(result, ModelManagerInterface::workingCopy(), lPaths,
                                      ModelManagerInterface::instance(), false);


    Document::MutablePtr doc = readDocument(filename);
    QVERIFY(!doc->source().isEmpty());

    Snapshot snapshot = modelManager->snapshot();

    QmlJSTools::SemanticInfo semanticInfo;
    semanticInfo.document = doc;
    semanticInfo.snapshot = snapshot;

    Link link(semanticInfo.snapshot, modelManager->defaultVContext(doc->language(), doc), modelManager->builtins(doc));

    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    ScopeChain *scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.setRootScopeChain(QSharedPointer<const ScopeChain>(scopeChain));

    Check checker(doc, semanticInfo.context);
    semanticInfo.staticAnalysisMessages = checker();

    QCOMPARE(semanticInfo.semanticMessages.length(), nSemanticMessages);
    QCOMPARE(semanticInfo.staticAnalysisMessages.length(), nStaticMessages);
}

QTEST_MAIN(tst_Dependencies)

#include "tst_dependencies.moc"
