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
#include "cpptoolstestcase.h"
#include "typehierarchybuilder.h"

#include <cplusplus/Overview.h>
#include <cplusplus/SymbolVisitor.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

Q_DECLARE_METATYPE(QList<Tests::TestDocument>)

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

private:
    Class *m_clazz;
};

class TypeHierarchyBuilderTestCase : public CppTools::Tests::TestCase
{
public:
    TypeHierarchyBuilderTestCase(const QList<Tests::TestDocument> &documents,
                                 const QString &expectedHierarchy)
    {
        QVERIFY(succeededSoFar());

        // Write files
        QStringList filePaths;
        foreach (const Tests::TestDocument &document, documents) {
            QVERIFY(document.writeToDisk());
            filePaths << document.filePath();
        }

        // Parse files
        QVERIFY(parseFiles(filePaths));
        const Snapshot snapshot = globalSnapshot();

        // Get class for which to generate the hierarchy
        const Document::Ptr firstDocument = snapshot.document(filePaths.first());
        QVERIFY(firstDocument);
        QVERIFY(firstDocument->diagnosticMessages().isEmpty());
        Class *clazz = FindFirstClassInDocument()(firstDocument);
        QVERIFY(clazz);

        // Generate and compare hierarchies
        TypeHierarchyBuilder typeHierarchyBuilder(clazz, snapshot);
        const TypeHierarchy hierarchy = typeHierarchyBuilder.buildDerivedTypeHierarchy();

        const QString actualHierarchy = toString(hierarchy);
//        Uncomment for updating/generating reference data:
//        qDebug() << actualHierarchy;
        QCOMPARE(actualHierarchy, expectedHierarchy);
    }
};

} // anonymous namespace

void CppToolsPlugin::test_typehierarchy_data()
{
    QTest::addColumn<QList<Tests::TestDocument> >("documents");
    QTest::addColumn<QString>("expectedHierarchy");

    typedef Tests::TestDocument TestDocument;

    QTest::newRow("basic-single-document")
        << (QList<TestDocument>()
            << TestDocument("a.h",
                            "class A {};\n"
                            "class B : public A {};\n"
                            "class C1 : public B {};\n"
                            "class C2 : public B {};\n"
                            "class D : public C1 {};\n"))
        << QString::fromLatin1(
            "A\n"
            "  B\n"
            "    C1\n"
            "      D\n"
            "    C2\n" );

    QTest::newRow("basic-multiple-documents")
        << (QList<TestDocument>()
            << TestDocument("a.h",
                            "class A {};")
            << TestDocument("b.h",
                            "#include \"a.h\"\n"
                            "class B : public A {};")
            << TestDocument("c1.h",
                            "#include \"b.h\"\n"
                            "class C1 : public B {};")
            << TestDocument("c2.h",
                            "#include \"b.h\"\n"
                            "class C2 : public B {};")
            << TestDocument("d.h",
                            "#include \"c1.h\"\n"
                            "class D : public C1 {};"))
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
    QFETCH(QList<Tests::TestDocument>, documents);
    QFETCH(QString, expectedHierarchy);

    TypeHierarchyBuilderTestCase(documents, expectedHierarchy);
}
