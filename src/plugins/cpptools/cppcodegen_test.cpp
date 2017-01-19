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

#include "cpptoolsplugin.h"
#include "cpptoolstestcase.h"
#include "insertionpointlocator.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>

#include <QtTest>
#include <QDebug>

/*!
    Tests for various parts of the code generation. Well, okay, currently it only
    tests the InsertionPointLocator.
 */
using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

namespace {

Document::Ptr createDocument(const QString filePath, const QByteArray text,
                             unsigned expectedGlobalSymbolCount)
{
    Document::Ptr document = Document::create(filePath);
    document->setUtf8Source(text);
    document->check();
    QTC_ASSERT(document->diagnosticMessages().isEmpty(), return Document::Ptr());
    QTC_ASSERT(document->globalSymbolCount() == expectedGlobalSymbolCount, return Document::Ptr());

    return document;
}

Document::Ptr createDocumentAndFile(Tests::TemporaryDir *temporaryDir,
                                    const QByteArray relativeFilePath,
                                    const QByteArray text,
                                    unsigned expectedGlobalSymbolCount)
{
    QTC_ASSERT(temporaryDir, return Document::Ptr());
    const QString absoluteFilePath = temporaryDir->createFile(relativeFilePath, text);
    QTC_ASSERT(!absoluteFilePath.isEmpty(), return Document::Ptr());

    return createDocument(absoluteFilePath, text, expectedGlobalSymbolCount);
}

} // anonymous namespace

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
    Document::Ptr doc = createDocument(QLatin1String("public_in_empty_class"), src, 1U);
    QVERIFY(doc);

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
    Document::Ptr doc = createDocument(QLatin1String("public_in_nonempty_class"), src, 1U);
    QVERIFY(doc);

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
    Document::Ptr doc = createDocument(QLatin1String("public_before_protected"), src, 1U);
    QVERIFY(doc);

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
    Document::Ptr doc = createDocument(QLatin1String("private_after_protected"), src, 1U);
    QVERIFY(doc);

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
    Document::Ptr doc = createDocument(QLatin1String("protected_in_nonempty_class"), src, 1U);
    QVERIFY(doc);

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
    Document::Ptr doc = createDocument(QLatin1String("protected_betwee_public_and_private"), src, 1U);
    QVERIFY(doc);

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

    Document::Ptr doc = createDocument(QLatin1String("qtdesigner_integration"), src, 2U);
    QVERIFY(doc);

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
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "};\n"
            "\n";
    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 1U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = "\n"
            "int x;\n"  // line 1
            "\n";
    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 1U);
    QVERIFY(sourceDocument);

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
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
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
    QCOMPARE(loc.line(), 3U);
    QCOMPARE(loc.column(), 1U);
}

void CppToolsPlugin::test_codegen_definition_first_member()
{
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "};\n"
            "\n";
    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 1U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = QString::fromLatin1(
            "\n"
            "#include \"%1/file.h\"\n" // line 1
            "int x;\n"
            "\n"
            "void Foo::bar()\n" // line 4
            "{\n"
            "\n"
            "}\n"
            "\n"
            "int y;\n").arg(temporaryDir.path()).toLatin1();
    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 3U);
    QVERIFY(sourceDocument);
    sourceDocument->addIncludeFile(Document::Include(QLatin1String("file.h"),
                                                     headerDocument->fileName(), 1,
                                                     Client::IncludeLocal));

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
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
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.line(), 4U);
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.suffix(), QLatin1String("\n\n"));
    QCOMPARE(loc.prefix(), QString());
}

void CppToolsPlugin::test_codegen_definition_last_member()
{
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "};\n"
            "\n";
    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 1U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = QString::fromLatin1(
            "\n"
            "#include \"%1/file.h\"\n" // line 1
            "int x;\n"
            "\n"
            "void Foo::foo()\n" // line 4
            "{\n"
            "\n"
            "}\n" // line 7
            "\n"
            "int y;\n").arg(temporaryDir.path()).toLatin1();

    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 3U);
    QVERIFY(sourceDocument);
    sourceDocument->addIncludeFile(Document::Include(QLatin1String("file.h"),
                                                     headerDocument->fileName(), 1,
                                                     Client::IncludeLocal));

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
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
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.line(), 7U);
    QCOMPARE(loc.column(), 2U);
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
}

void CppToolsPlugin::test_codegen_definition_middle_member()
{
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "void car();\n" // line 5
            "};\n"
            "\n";

    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 1U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = QString::fromLatin1(
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
            "int y;\n").arg(Utils::TemporaryDirectory::masterDirectoryPath()).toLatin1();

    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 4U);
    QVERIFY(sourceDocument);
    sourceDocument->addIncludeFile(Document::Include(QLatin1String("file.h"),
                                                     headerDocument->fileName(), 1,
                                                     Client::IncludeLocal));

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
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
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.line(), 7U);
    QCOMPARE(loc.column(), 2U);
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
}

void CppToolsPlugin::test_codegen_definition_middle_member_surrounded_by_undefined()
{
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "void baz();\n" // line 5
            "void car();\n" // line 6
            "};\n"
            "\n";
    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 1U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = QString::fromLatin1(
            "\n"
            "#include \"%1/file.h\"\n" // line 1
            "int x;\n"
            "\n"
            "void Foo::car()\n" // line 4
            "{\n"
            "\n"
            "}\n"
            "\n"
            "int y;\n").arg(temporaryDir.path()).toLatin1();
    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 3U);
    QVERIFY(sourceDocument);
    sourceDocument->addIncludeFile(Document::Include(QLatin1String("file.h"),
                                                     headerDocument->fileName(), 1,
                                                     Client::IncludeLocal));

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 4U);
    Declaration *decl = foo->memberAt(1)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 4U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl);
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.line(), 4U);
    QCOMPARE(loc.column(), 1U);
    QCOMPARE(loc.prefix(), QString());
    QCOMPARE(loc.suffix(), QLatin1String("\n\n"));
}

void CppToolsPlugin::test_codegen_definition_member_specific_file()
{
    Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QByteArray headerText = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "void foo();\n" // line 3
            "void bar();\n" // line 4
            "void baz();\n" // line 5
            "};\n"
            "\n"
            "void Foo::bar()\n"
            "{\n"
            "\n"
            "}\n";
    Document::Ptr headerDocument = createDocumentAndFile(&temporaryDir, "file.h", headerText, 2U);
    QVERIFY(headerDocument);

    const QByteArray sourceText = QString::fromLatin1(
            "\n"
            "#include \"%1/file.h\"\n" // line 1
            "int x;\n"
            "\n"
            "void Foo::foo()\n" // line 4
            "{\n"
            "\n"
            "}\n" // line 7
            "\n"
            "int y;\n").arg(temporaryDir.path()).toLatin1();
    Document::Ptr sourceDocument = createDocumentAndFile(&temporaryDir, "file.cpp", sourceText, 3U);
    QVERIFY(sourceDocument);
    sourceDocument->addIncludeFile(Document::Include(QLatin1String("file.h"),
                                                     headerDocument->fileName(), 1,
                                                     Client::IncludeLocal));

    Snapshot snapshot;
    snapshot.insert(headerDocument);
    snapshot.insert(sourceDocument);

    Class *foo = headerDocument->globalSymbolAt(0)->asClass();
    QVERIFY(foo);
    QCOMPARE(foo->line(), 1U);
    QCOMPARE(foo->column(), 7U);
    QCOMPARE(foo->memberCount(), 3U);
    Declaration *decl = foo->memberAt(2)->asDeclaration();
    QVERIFY(decl);
    QCOMPARE(decl->line(), 5U);
    QCOMPARE(decl->column(), 6U);

    CppRefactoringChanges changes(snapshot);
    InsertionPointLocator find(changes);
    QList<InsertionLocation> locList = find.methodDefinition(decl, true, sourceDocument->fileName());
    QVERIFY(locList.size() == 1);
    InsertionLocation loc = locList.first();
    QCOMPARE(loc.fileName(), sourceDocument->fileName());
    QCOMPARE(loc.line(), 7U);
    QCOMPARE(loc.column(), 2U);
    QCOMPARE(loc.prefix(), QLatin1String("\n\n"));
    QCOMPARE(loc.suffix(), QString());
}
