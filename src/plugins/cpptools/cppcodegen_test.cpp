/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "insertionpointlocator.h"
#include "cpptoolsplugin.h"

#include <utils/fileutils.h>

#include <QtTest>
#include <QDebug>
#include <QDir>

/*!
    Tests for various parts of the code generation. Well, okay, currently it only
    tests the InsertionPointLocator.
 */
using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

/*!
    Should insert at line 3, column 1, with "public:\n" as prefix and without suffix.
 */
void CppToolsPlugin::test_codegen_public_in_empty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "};\n"
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("public_in_empty_class"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Public);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("public:\n"));
    QVERIFY(loc.suffix().isEmpty());
    QCOMPARE(loc.line(), 3U);
    QCOMPARE(loc.column(), 1U);
}

/*!
    Should insert at line 3, column 1, without prefix and without suffix.
 */
void CppToolsPlugin::test_codegen_public_in_nonempty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "};\n"        // line 4
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("public_in_nonempty_class"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Public);
    QVERIFY(loc.isValid());
    QVERIFY(loc.prefix().isEmpty());
    QVERIFY(loc.suffix().isEmpty());
    QCOMPARE(loc.line(), 4U);
    QCOMPARE(loc.column(), 1U);
}

/*!
    Should insert at line 3, column 1, with "public:\n" as prefix and "\n suffix.
 */
void CppToolsPlugin::test_codegen_public_before_protected()
{
    const QByteArray src = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "protected:\n" // line 3
            "};\n"
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("public_before_protected"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Public);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("public:\n"));
    QCOMPARE(loc.suffix(), QLatin1String("\n"));
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.line(), 3U);
}

/*!
    Should insert at line 4, column 1, with "private:\n" as prefix and without
    suffix.
 */
void CppToolsPlugin::test_codegen_private_after_protected()
{
    const QByteArray src = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "protected:\n" // line 3
            "};\n"
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("private_after_protected"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Private);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("private:\n"));
    QVERIFY(loc.suffix().isEmpty());
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.line(), 4U);
}

/*!
    Should insert at line 4, column 1, with "protected:\n" as prefix and without
    suffix.
 */
void CppToolsPlugin::test_codegen_protected_in_nonempty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "};\n"        // line 4
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("protected_in_nonempty_class"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Protected);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("protected:\n"));
    QVERIFY(loc.suffix().isEmpty());
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.line(), 4U);
}

/*!
    Should insert at line 4, column 1, with "protected\n" as prefix and "\n" suffix.
 */
void CppToolsPlugin::test_codegen_protected_between_public_and_private()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "private:\n"  // line 4
            "};\n"        // line 5
            "\n";

    Document::Ptr doc = Document::create(QLatin1String("protected_betwee_public_and_private"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 1U);

    Class *foo = doc->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::Protected);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("protected:\n"));
    QCOMPARE(loc.suffix(), QLatin1String("\n"));
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.line(), 4U);
}

/*!
    Should insert at line 18, column 1, with "private slots:\n" as prefix and "\n"
    as suffix.

    This is the typical Qt Designer case, with test-input like what the integration
    generates.
 */
void CppToolsPlugin::test_codegen_qtdesigner_integration()
{
    const QByteArray src = "/**** Some long (C)opyright notice ****/\n"
            "#ifndef MAINWINDOW_H\n"
            "#define MAINWINDOW_H\n"
            "\n"
            "#include <QMainWindow>\n"
            "\n"
            "namespace Ui {\n"
            "    class MainWindow;\n"
            "}\n"
            "\n"
            "class MainWindow : public QMainWindow\n" // line 10
            "{\n"
            "    Q_OBJECT\n"
            "\n"
            "public:\n" // line 14
            "    explicit MainWindow(QWidget *parent = 0);\n"
            "    ~MainWindow();\n"
            "\n"
            "private:\n" // line 18
            "    Ui::MainWindow *ui;\n"
            "};\n"
            "\n"
            "#endif // MAINWINDOW_H\n";

    Document::Ptr doc = Document::create(QLatin1String("qtdesigner_integration"));
    doc->setUtf8Source(src);
    doc->parse();
    doc->check();

    QCOMPARE(doc->diagnosticMessages().size(), 0);
    QCOMPARE(doc->globalSymbolCount(), 2U);

    Class *foo = doc->globalSymbolAt(1)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 10U);
    QCOMPARE(foo->column(), 7U);

    Snapshot snapshot;
    snapshot.insert(doc);
    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    InsertionLocation loc = find.methodDeclarationInClass(
                doc->fileName(),
                foo,
                InsertionPointLocator::PrivateSlot);
    QVERIFY(loc.isValid());
    QCOMPARE(loc.prefix(), QLatin1String("private slots:\n"));
    QCOMPARE(loc.suffix(), QLatin1String("\n"));
    QCOMPARE(loc.line(), 18U);
    QCOMPARE(loc.column(), 1U);
}

void CppToolsPlugin::test_codegen_definition_empty_class()
{
    const QByteArray srcText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "};\n"
            "\n";

    const QByteArray dstText = "\n"
            "int x;\n"  // line 1
            "\n";

    Document::Ptr src = Document::create(QDir::tempPath() + QLatin1String("/file.h"));
    Utils::FileSaver srcSaver(src->fileName());
    srcSaver.write(srcText);
    srcSaver.finalize();
    src->setUtf8Source(srcText);
    src->parse();
    src->check();
    QCOMPARE(src->diagnosticMessages().size(), 0);
    QCOMPARE(src->globalSymbolCount(), 1U);

    Document::Ptr dst = Document::create(QDir::tempPath() + QLatin1String("/file.cpp"));
    Utils::FileSaver dstSaver(dst->fileName());
    dstSaver.write(dstText);
    dstSaver.finalize();
    dst->setUtf8Source(dstText);
    dst->parse();
    dst->check();
    QCOMPARE(dst->diagnosticMessages().size(), 0);
    QCOMPARE(dst->globalSymbolCount(), 1U);

    Snapshot snapshot;
    snapshot.insert(src);
    snapshot.insert(dst);

    Class *foo = src->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 1U);
    Declaration *decl = foo->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 3U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl);
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), dst->fileName());
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
    QCOMPARE(loc.line(), 3U);
    QCOMPARE(loc.column(), 1U);
}

void CppToolsPlugin::test_codegen_definition_first_member()
{
    const QByteArray srcText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "};\n"
            "\n";

    const QByteArray dstText = QString::fromLatin1(
                "\n"
                "#include \"%1/file.h\"\n" // line 1
                "int x;\n"
                "\n"
                "void Foo::bar()\n" // line 4
                "{\n"
                "\n"
                "}\n"
                "\n"
                "int y;\n").arg(QDir::tempPath()).toLatin1();

    Document::Ptr src = Document::create(QDir::tempPath() + QLatin1String("/file.h"));
    Utils::FileSaver srcSaver(src->fileName());
    srcSaver.write(srcText);
    srcSaver.finalize();
    src->setUtf8Source(srcText);
    src->parse();
    src->check();
    QCOMPARE(src->diagnosticMessages().size(), 0);
    QCOMPARE(src->globalSymbolCount(), 1U);

    Document::Ptr dst = Document::create(QDir::tempPath() + QLatin1String("/file.cpp"));
    dst->addIncludeFile(src->fileName(), 1);
    Utils::FileSaver dstSaver(dst->fileName());
    dstSaver.write(dstText);
    dstSaver.finalize();
    dst->setUtf8Source(dstText);
    dst->parse();
    dst->check();
    QCOMPARE(dst->diagnosticMessages().size(), 0);
    QCOMPARE(dst->globalSymbolCount(), 3U);

    Snapshot snapshot;
    snapshot.insert(src);
    snapshot.insert(dst);

    Class *foo = src->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 2U);
    Declaration *decl = foo->memberAt(0)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 3U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl);
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), dst->fileName());
    QCOMPARE(loc.line(), 4U);
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.suffix(), QLatin1String("\n\n"));
    QCOMPARE(loc.prefix(), QString());
}

void CppToolsPlugin::test_codegen_definition_last_member()
{
    const QByteArray srcText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "};\n"
            "\n";

    const QByteArray dstText = QString::fromLatin1(
                "\n"
                "#include \"%1/file.h\"\n" // line 1
                "int x;\n"
                "\n"
                "void Foo::foo()\n" // line 4
                "{\n"
                "\n"
                "}\n" // line 7
                "\n"
                "int y;\n").arg(QDir::tempPath()).toLatin1();

    Document::Ptr src = Document::create(QDir::tempPath() + QLatin1String("/file.h"));
    Utils::FileSaver srcSaver(src->fileName());
    srcSaver.write(srcText);
    srcSaver.finalize();
    src->setUtf8Source(srcText);
    src->parse();
    src->check();
    QCOMPARE(src->diagnosticMessages().size(), 0);
    QCOMPARE(src->globalSymbolCount(), 1U);

    Document::Ptr dst = Document::create(QDir::tempPath() + QLatin1String("/file.cpp"));
    dst->addIncludeFile(src->fileName(), 1);
    Utils::FileSaver dstSaver(dst->fileName());
    dstSaver.write(dstText);
    dstSaver.finalize();
    dst->setUtf8Source(dstText);
    dst->parse();
    dst->check();
    QCOMPARE(dst->diagnosticMessages().size(), 0);
    QCOMPARE(dst->globalSymbolCount(), 3U);

    Snapshot snapshot;
    snapshot.insert(src);
    snapshot.insert(dst);

    Class *foo = src->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 2U);
    Declaration *decl = foo->memberAt(1)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 4U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl);
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), dst->fileName());
    QCOMPARE(loc.line(), 7U);
    QCOMPARE(loc.column(), 2U);
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
}

void CppToolsPlugin::test_codegen_definition_middle_member()
{
    const QByteArray srcText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "void car();\n" // line 5
            "};\n"
            "\n";

    const QByteArray dstText = QString::fromLatin1(
                "\n"
                "#include \"%1/file.h\"\n" // line 1
                "int x;\n"
                "\n"
                "void Foo::foo()\n" // line 4
                "{\n"
                "\n"
                "}\n" // line 7
                "\n"
                "void Foo::car()\n" // line 9
                "{\n"
                "\n"
                "}\n"
                "\n"
                "int y;\n").arg(QDir::tempPath()).toLatin1();

    Document::Ptr src = Document::create(QDir::tempPath() + QLatin1String("/file.h"));
    Utils::FileSaver srcSaver(src->fileName());
    srcSaver.write(srcText);
    srcSaver.finalize();
    src->setUtf8Source(srcText);
    src->parse();
    src->check();
    QCOMPARE(src->diagnosticMessages().size(), 0);
    QCOMPARE(src->globalSymbolCount(), 1U);

    Document::Ptr dst = Document::create(QDir::tempPath() + QLatin1String("/file.cpp"));
    dst->addIncludeFile(src->fileName(), 1);
    Utils::FileSaver dstSaver(dst->fileName());
    dstSaver.write(dstText);
    dstSaver.finalize();
    dst->setUtf8Source(dstText);
    dst->parse();
    dst->check();
    QCOMPARE(dst->diagnosticMessages().size(), 0);
    QCOMPARE(dst->globalSymbolCount(), 4U);

    Snapshot snapshot;
    snapshot.insert(src);
    snapshot.insert(dst);

    Class *foo = src->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 3U);
    Declaration *decl = foo->memberAt(1)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 4U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl);
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), dst->fileName());
    QCOMPARE(loc.line(), 7U);
    QCOMPARE(loc.column(), 2U);
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
}
