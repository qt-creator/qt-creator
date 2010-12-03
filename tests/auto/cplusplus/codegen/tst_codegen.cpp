#include <AST.h>
#include <Control.h>
#include <CppDocument.h>
#include <DiagnosticClient.h>
#include <Scope.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Bind.h>
#include <Symbols.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <QtTest>
#include <QtDebug>
#include <QTextDocument>

//TESTED_COMPONENT=src/libs/cplusplus

/*!
    Tests for various parts of the code generation. Well, okay, currently it only
    tests the InsertionPointLocator.
 */
using namespace CPlusPlus;
using namespace CppTools;

class tst_Codegen: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void public_in_empty_class();
    void public_in_nonempty_class();
    void public_before_protected();
    void private_after_protected();
    void protected_in_nonempty_class();
    void protected_betwee_public_and_private();
    void qtdesigner_integration();
private:
    ExtensionSystem::PluginManager *pluginManager;
};

void tst_Codegen::initTestCase()
{
    pluginManager = new ExtensionSystem::PluginManager;
    QSettings *settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                 QLatin1String("Nokia"), QLatin1String("QtCreator"));
    pluginManager->setSettings(settings);
    pluginManager->setFileExtension(QLatin1String("pluginspec"));
    pluginManager->setPluginPaths(QStringList() << QLatin1String(Q_PLUGIN_PATH));
    pluginManager->loadPlugins();
}

void tst_Codegen::cleanupTestCase()
{
    pluginManager->shutdown();
    delete pluginManager;
}
/*!
    Should insert at line 3, column 1, with "public:\n" as prefix and without suffix.
 */
void tst_Codegen::public_in_empty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "};\n"
            "\n";

    Document::Ptr doc = Document::create("public_in_empty_class");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::public_in_nonempty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "};\n"        // line 4
            "\n";

    Document::Ptr doc = Document::create("public_in_nonempty_class");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::public_before_protected()
{
    const QByteArray src = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "protected:\n" // line 3
            "};\n"
            "\n";

    Document::Ptr doc = Document::create("public_before_protected");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::private_after_protected()
{
    const QByteArray src = "\n"
            "class Foo\n"  // line 1
            "{\n"
            "protected:\n" // line 3
            "};\n"
            "\n";

    Document::Ptr doc = Document::create("private_after_protected");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::protected_in_nonempty_class()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "};\n"        // line 4
            "\n";

    Document::Ptr doc = Document::create("protected_in_nonempty_class");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::protected_betwee_public_and_private()
{
    const QByteArray src = "\n"
            "class Foo\n" // line 1
            "{\n"
            "public:\n"   // line 3
            "private:\n"  // line 4
            "};\n"        // line 5
            "\n";

    Document::Ptr doc = Document::create("protected_betwee_public_and_private");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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
void tst_Codegen::qtdesigner_integration()
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

    Document::Ptr doc = Document::create("qtdesigner_integration");
    doc->setSource(src);
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
    InsertionPointLocator find(&changes);
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

QTEST_MAIN(tst_Codegen)
#include "tst_codegen.moc"
