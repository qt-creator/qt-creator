// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QString>
#include <QStringList>
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

static QString getValue(const QString &data,
                        const QString &re,
                        const QString &defaultValue = QString::number(0))
{
    const QRegularExpressionMatch m = QRegularExpression(re).match(data);
    return m.hasMatch() ? m.captured(1) : defaultValue;
}

struct TestData
{
    TestData(Document::MutablePtr document, int nSemanticMessages, int nStaticMessages)
        : doc(document)
        , semanticMessages(nSemanticMessages)
        , staticMessages(nStaticMessages)
    {}
    Document::MutablePtr doc;
    const int semanticMessages;
    const int staticMessages;
};

static TestData testData(const QString &path)
{
    QFile file(path);
    Utils::FilePath pathPath = Utils::FilePath::fromString(path);
    file.open(QFile::ReadOnly | QFile::Text);
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    Document::MutablePtr doc = Document::create(pathPath, Dialect::Qml);
    doc->setSource(content);
    doc->parse();
    const QString nSemantic = getValue(content, "//\\s*ExpectedSemanticMessages: (\\d+)");
    const QString nStatic = getValue(content, "//\\s*ExpectedStaticMessages: (\\d+)");
    return TestData(doc, nSemantic.toInt(), nStatic.toInt());
}

static QStringList readSkipList(const QDir &dir, const QString &filename)
{
    QFile f(dir.absoluteFilePath(filename));
    QStringList result;

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    while (!f.atEnd()) {
        const QString s = QString::fromUtf8(f.readLine().trimmed());
        if (!s.isEmpty())
            result << dir.absoluteFilePath(s);
    }
    return result;
}

void printUnexpectedMessages(const QmlJSTools::SemanticInfo &info, int nSemantic, int nStatic)
{
    if (nSemantic == 0 && info.semanticMessages.length() > 0)
        for (auto msg: info.semanticMessages)
            qDebug() << msg.message;
    if (nStatic == 0 && info.staticAnalysisMessages.length() > 0)
        for (auto msg: info.staticAnalysisMessages)
            qDebug() << msg.message;
    return;
}

class tst_Ecmascript : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_data();
    void test();

private:
    QFileInfoList m_files;
    QStringList m_basePaths;
};

void tst_Ecmascript::initTestCase()
{
    QDir sampledir(TESTSRCDIR "/samples/");

    QDirIterator it(sampledir, QDirIterator::Subdirectories);

    QStringList skipList = readSkipList(sampledir, QLatin1String("skip.txt"));
    while (it.hasNext()) {
        QString path = it.next();
        if (skipList.contains(path))
            continue;
        QFileInfo f(path);
        if (f.isFile() && f.suffix() == QLatin1String("js"))
            m_files << f;
    }

    m_basePaths.append(QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath));

    if (!ModelManagerInterface::instance())
        new ModelManagerInterface;

    if (!ExtensionSystem::PluginManager::instance())
        new ExtensionSystem::PluginManager;
}

void tst_Ecmascript::test_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<int>("nSemanticMessages");
    QTest::addColumn<int>("nStaticMessages");


    for (const QFileInfo& f: m_files)
        QTest::newRow(f.fileName().toLatin1().data()) << f.absoluteFilePath();
}

void tst_Ecmascript::test()
{
    QFETCH(QString, filename);

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    PathsAndLanguages lPaths;
    QStringList paths(m_basePaths);
    for (auto p: paths)
        lPaths.maybeInsert(Utils::FilePath::fromString(p), Dialect::Qml);
    ModelManagerInterface::importScan(ModelManagerInterface::workingCopy(), lPaths,
                                      ModelManagerInterface::instance(), false);

    TestData data = testData(filename);
    Document::MutablePtr doc = data.doc;
    int nExpectedSemanticMessages = data.semanticMessages;
    int nExpectedStaticMessages = data.staticMessages;
    QVERIFY(!doc->source().isEmpty());

    Snapshot snapshot = modelManager->snapshot();

    QmlJSTools::SemanticInfo semanticInfo;
    semanticInfo.document = doc;
    semanticInfo.snapshot = snapshot;

    Link link(semanticInfo.snapshot, modelManager->defaultVContext(doc->language(), doc),
              modelManager->builtins(doc));

    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    ScopeChain *scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.setRootScopeChain(QSharedPointer<const ScopeChain>(scopeChain));

    Check checker(doc, semanticInfo.context);
    semanticInfo.staticAnalysisMessages = checker();

    printUnexpectedMessages(semanticInfo, nExpectedSemanticMessages, nExpectedStaticMessages);

    QCOMPARE(semanticInfo.semanticMessages.length(), nExpectedSemanticMessages);
    QCOMPARE(semanticInfo.staticAnalysisMessages.length(), nExpectedStaticMessages);
}

QTEST_GUILESS_MAIN(tst_Ecmascript)

#include "tst_ecmascript7.moc"
