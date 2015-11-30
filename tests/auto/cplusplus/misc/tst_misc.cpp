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

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/findcdbbreakpoint.h>

#include <QtTest>
#include <QDebug>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_Misc: public QObject
{
    Q_OBJECT

private slots:
    void diagnosticClient_error();
    void diagnosticClient_warning();

    void findBreakpoints();
    void findBreakpoints2();
    void findBreakpoints3();

    void astPathOnGeneratedTokens();
};

void tst_Misc::diagnosticClient_error()
{
    const QByteArray src("\n"
                         "class Foo {}\n"
                         );
    Document::Ptr doc = Document::create("diagnosticClient_error");
    QVERIFY(!doc.isNull());
    doc->setUtf8Source(src);
    bool success = doc->parse(Document::ParseTranlationUnit);
    QVERIFY(success);

    QList<Document::DiagnosticMessage> diagnostics = doc->diagnosticMessages();
    QVERIFY(diagnostics.size() == 1);

    const Document::DiagnosticMessage &msg = diagnostics.at(0);
    QCOMPARE(msg.level(), (int) Document::DiagnosticMessage::Error);
    QCOMPARE(msg.line(), 2U);
    QCOMPARE(msg.column(), 1U);
}

void tst_Misc::diagnosticClient_warning()
{
    const QByteArray src("\n"
                         "using namespace ;\n"
                         );
    Document::Ptr doc = Document::create("diagnosticClient_warning");
    QVERIFY(!doc.isNull());
    doc->setUtf8Source(src);
    bool success = doc->parse(Document::ParseTranlationUnit);
    QVERIFY(success);

    QList<Document::DiagnosticMessage> diagnostics = doc->diagnosticMessages();
    QVERIFY(diagnostics.size() == 1);

    const Document::DiagnosticMessage &msg = diagnostics.at(0);
    QCOMPARE(msg.level(), (int) Document::DiagnosticMessage::Warning);
    QCOMPARE(msg.line(), 1U);
    QCOMPARE(msg.column(), 17U);
}

void tst_Misc::findBreakpoints()
{
    const QByteArray src("\n"                   // line 0
                         "class C {\n"
                         "  int a;\n"
                         "  C():\n"
                         "    a(0)\n"           // line 4
                         "  {\n"                // line 5
                         "  }\n"
                         "  void empty()\n"     // line 7
                         "  {\n"                // line 8
                         "  }\n"
                         "  void misc()    \n"
                         "  {              \n"  // line 11
                         "    if (         \n"  // line 12
                         "          a      \n"  // line 13
                         "        &&       \n"  // line 14
                         "          b      \n"  // line 15
                         "       )         \n"  // line 16
                         "    {            \n"  // line 17
                         "    }            \n"  // line 18
                         "    while (      \n"  // line 19
                         "          a      \n"  // line 20
                         "        &&       \n"  // line 21
                         "          b      \n"  // line 22
                         "       )         \n"  // line 23
                         "    {            \n"  // line 24
                         "    }            \n"  // line 25
                         "    do {         \n"  // line 26
                         "    }            \n"  // line 27
                         "    while (      \n"  // line 28
                         "          a      \n"  // line 39
                         "        &&       \n"  // line 30
                         "          b      \n"  // line 31
                         "       );        \n"  // line 32
                         "  }              \n"
                         "};               \n"
                         );
    Document::Ptr doc = Document::create("findContstructorBreakpoint");
    QVERIFY(!doc.isNull());
    doc->setUtf8Source(src);
    bool success = doc->parse();
    QVERIFY(success);
    QCOMPARE(doc->diagnosticMessages().size(), 0);
    FindCdbBreakpoint findBreakpoint(doc->translationUnit());

    QCOMPARE(findBreakpoint(0), 5U);
    QCOMPARE(findBreakpoint(7), 8U);
    QCOMPARE(findBreakpoint(11), 16U);
    QCOMPARE(findBreakpoint(17), 23U);
    QCOMPARE(findBreakpoint(18), 23U);
}

void tst_Misc::findBreakpoints2()
{
    const QByteArray src("\n"                   // line 0
                         "void foo() {\n"
                         "  int a = 2;\n"       // line 2
                         "  switch (x) {\n"     // line 3
                         "  case 1: {\n"        // line 4
                         "      int y = 2;\n"   // line 5
                         "      y++;\n"
                         "      break;\n"       // line 7
                         "  }\n"
                         "  }\n"
                         "}\n"
                         );
    Document::Ptr doc = Document::create("findSwitchBreakpoint");
    QVERIFY(!doc.isNull());
    doc->setUtf8Source(src);
    bool success = doc->parse();
    QVERIFY(success);
    QCOMPARE(doc->diagnosticMessages().size(), 0);
    FindCdbBreakpoint findBreakpoint(doc->translationUnit());

    QCOMPARE(findBreakpoint(0), 2U);
    QCOMPARE(findBreakpoint(1), 2U);
    QCOMPARE(findBreakpoint(2), 2U);
    QCOMPARE(findBreakpoint(3), 3U);
    QCOMPARE(findBreakpoint(4), 5U);
    QCOMPARE(findBreakpoint(5), 5U);
    QCOMPARE(findBreakpoint(6), 6U);
    QCOMPARE(findBreakpoint(7), 7U);
}

void tst_Misc::findBreakpoints3()
{
    const QByteArray src("\n"                       // line 0
                         "int foo() {\n"
                         "  try {\n"                // line 2
                         "    bar();\n"             // line 3
                         "  } catch (Mooze &m) {\n" // line 4
                         "    wooze();\n"           // line 5
                         "  }\n"
                         "  return 0;\n"            // line 7
                         "}\n"
                         );
    Document::Ptr doc = Document::create("findCatchBreakpoint");
    QVERIFY(!doc.isNull());
    doc->setUtf8Source(src);
    bool success = doc->parse();
    QVERIFY(success);
    QCOMPARE(doc->diagnosticMessages().size(), 0);
    FindCdbBreakpoint findBreakpoint(doc->translationUnit());

    QCOMPARE(findBreakpoint(2), 3U);
    QCOMPARE(findBreakpoint(3), 3U);
    QCOMPARE(findBreakpoint(4), 5U);
    QCOMPARE(findBreakpoint(5), 5U);
    QCOMPARE(findBreakpoint(7), 7U);
}

static Document::Ptr documentCreatedWithFastPreprocessor(const QByteArray source)
{
    Snapshot snapshot;
    auto document = snapshot.preprocessedDocument(source, QLatin1String("test.cpp"));
    document->check();
    return document;
}

void tst_Misc::astPathOnGeneratedTokens()
{
    const QByteArray source =
        "#define INT int\n"
        "#define S ;\n"
        "INT x S\n";
    const auto document = documentCreatedWithFastPreprocessor(source);
    ASTPath astPath(document);

    // Check start
    auto paths = astPath(3, 1);
    QCOMPARE(paths.size(), 0);

    // Check middle
    paths = astPath(3, 5);
    QCOMPARE(paths.size(), 5);
    QVERIFY(paths.at(0)->asTranslationUnit());
    QVERIFY(paths.at(1)->asSimpleDeclaration());
    QVERIFY(paths.at(2)->asDeclarator());
    QVERIFY(paths.at(3)->asDeclaratorId());
    QVERIFY(paths.at(4)->asSimpleName());

    // Check end
    for (auto i : { 7, 8, 9 }) {
        paths = astPath(3, i);
        QCOMPARE(paths.size(), 2);
        QVERIFY(paths.at(0)->asTranslationUnit());
        QVERIFY(paths.at(1)->asSimpleDeclaration());
    }
}

QTEST_MAIN(tst_Misc)
#include "tst_misc.moc"
