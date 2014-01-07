/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "cpptoolsplugin.h"

#include "cppmodelmanagerinterface.h"
#include "typehierarchybuilder.h"

#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/Overview.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

namespace {

bool hierarchySorter(const TypeHierarchy &h1, const TypeHierarchy &h2)
{
    Overview oo;
    return oo.prettyName(h1.symbol()->name()) < oo.prettyName(h2.symbol()->name());
}

QString toString(const TypeHierarchy &hierarchy, int indent = 0)
{
    Symbol *symbol = hierarchy.symbol();
    QString result = QString(indent, QLatin1Char(' '))
        + Overview().prettyName(symbol->name()) + QLatin1Char('\n');

    QList<TypeHierarchy> sortedHierarchy = hierarchy.hierarchy();
    qSort(sortedHierarchy.begin(), sortedHierarchy.end(), hierarchySorter);
    foreach (TypeHierarchy childHierarchy, sortedHierarchy)
        result += toString(childHierarchy, indent + 2);
    return result;
}

class FindFirstClassInDocument: private SymbolVisitor
{
public:
    FindFirstClassInDocument() : m_clazz(0) {}

    Class *operator()(const Document::Ptr &document)
    {
        accept(document->globalNamespace());
        return m_clazz;
    }

private:
    bool preVisit(Symbol *symbol)
    {
        if (m_clazz)
            return false;

        if (Class *c = symbol->asClass()) {
            m_clazz = c;
            return false;
        }

        return true;
    }

    Class *m_clazz;
};

struct TestDocument
{
public:
    TestDocument(const QString &fileName, const QString &contents)
        : fileName(fileName), contents(contents) {}

    QString fileName;
    QString contents;
};

class TestCase
{
public:
    TestCase(const QList<TestDocument> &documents, const QString &expectedHierarchy)
        : m_modelManager(CppModelManagerInterface::instance())
        , m_documents(documents)
        , m_expectedHierarchy(expectedHierarchy)
    {
        QVERIFY(m_modelManager->snapshot().isEmpty());
    }

    void run()
    {
        // Write files
        QStringList filePaths;
        foreach (const TestDocument &document, m_documents) {
            const QString filePath = QDir::tempPath() + QLatin1Char('/') + document.fileName;
            Utils::FileSaver documentSaver(filePath);
            documentSaver.write(document.contents.toUtf8());
            documentSaver.finalize();
            filePaths << filePath;
        }

        // Parse files
        m_modelManager->updateSourceFiles(filePaths).waitForFinished();

        // Get class for which to generate the hierarchy
        const Snapshot snapshot = m_modelManager->snapshot();
        QVERIFY(!snapshot.isEmpty());
        const Document::Ptr firstDocument = snapshot.document(filePaths.first());
        Class *clazz = FindFirstClassInDocument()(firstDocument);
        QVERIFY(clazz);

        // Generate and compare hierarchies
        TypeHierarchyBuilder typeHierarchyBuilder(clazz, snapshot);
        const TypeHierarchy hierarchy = typeHierarchyBuilder.buildDerivedTypeHierarchy();

        const QString actualHierarchy = toString(hierarchy);
//        Uncomment for updating/generating reference data:
//        qDebug() << actualHierarchy;
        QCOMPARE(actualHierarchy, m_expectedHierarchy);
    }

    ~TestCase()
    {
        m_modelManager->GC();
        QVERIFY(m_modelManager->snapshot().isEmpty());
    }

private:
    CppModelManagerInterface *m_modelManager;
    QList<TestDocument> m_documents;
    QString m_expectedHierarchy;
};

} // anonymous namespace

Q_DECLARE_METATYPE(QList<TestDocument>)

void CppToolsPlugin::test_typehierarchy_data()
{
    QTest::addColumn<QList<TestDocument> >("documents");
    QTest::addColumn<QString>("expectedHierarchy");

    typedef QLatin1String _;

    QTest::newRow("basic-single-document")
        << (QList<TestDocument>()
            << TestDocument(_("a.h"),
                            _("class A {};\n"
                              "class B : public A {};\n"
                              "class C1 : public B {};\n"
                              "class C2 : public B {};\n"
                              "class D : public C1 {};\n")))
        << QString::fromLatin1(
            "A\n"
            "  B\n"
            "    C1\n"
            "      D\n"
            "    C2\n" );

    QTest::newRow("basic-multiple-documents")
        << (QList<TestDocument>()
            << TestDocument(_("a.h"),
                            _("class A {};"))
            << TestDocument(_("b.h"),
                            _("#include \"a.h\"\n"
                              "class B : public A {};"))
            << TestDocument(_("c1.h"),
                            _("#include \"b.h\"\n"
                              "class C1 : public B {};"))
            << TestDocument(_("c2.h"),
                            _("#include \"b.h\"\n"
                              "class C2 : public B {};"))
            << TestDocument(_("d.h"),
                            _("#include \"c1.h\"\n"
                              "class D : public C1 {};")))
        << QString::fromLatin1(
            "A\n"
            "  B\n"
            "    C1\n"
            "      D\n"
            "    C2\n"
            );
}

void CppToolsPlugin::test_typehierarchy()
{
    QFETCH(QList<TestDocument>, documents);
    QFETCH(QString, expectedHierarchy);

    TestCase testCase(documents, expectedHierarchy);
    testCase.run();
}
