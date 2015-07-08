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

#include "cppcompletionassist.h"
#include "cppmodelmanager.h"
#include "cpptoolsplugin.h"
#include "cpptoolstestcase.h"

#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/convenience.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/changeset.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QTextDocument>
#include <QtTest>

/*!
    Tests for code completion.
 */
using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;
using namespace TextEditor;
using namespace Core;

namespace {

typedef QByteArray _;

class CompletionTestCase : public Tests::TestCase
{
public:
    CompletionTestCase(const QByteArray &sourceText, const QByteArray &textToInsert = QByteArray())
        : m_position(-1), m_editorWidget(0), m_textDocument(0), m_editor(0)
    {
        QVERIFY(succeededSoFar());
        m_succeededSoFar = false;

        m_source = sourceText;
        m_position = m_source.indexOf('@');
        QVERIFY(m_position != -1);
        m_source[m_position] = ' ';

        // Write source to file
        m_temporaryDir.reset(new Tests::TemporaryDir());
        QVERIFY(m_temporaryDir->isValid());
        const QString fileName = m_temporaryDir->createFile("file.h", m_source);
        QVERIFY(!fileName.isEmpty());

        // Open in editor
        m_editor = EditorManager::openEditor(fileName);
        QVERIFY(m_editor);
        closeEditorAtEndOfTestCase(m_editor);
        m_editorWidget = qobject_cast<TextEditorWidget *>(m_editor->widget());
        QVERIFY(m_editorWidget);

        m_textDocument = m_editorWidget->document();

        // Get Document
        const Document::Ptr document = waitForFileInGlobalSnapshot(fileName);
        QVERIFY(document);
        QVERIFY(document->diagnosticMessages().isEmpty());

        m_snapshot.insert(document);

        if (!textToInsert.isEmpty())
            insertText(textToInsert);

        m_succeededSoFar = true;
    }

    QStringList getCompletions(bool *replaceAccessOperator = 0) const
    {
        QStringList completions;
        LanguageFeatures languageFeatures = LanguageFeatures::defaultFeatures();
        languageFeatures.objCEnabled = false;
        CppCompletionAssistInterface *ai
            = new CppCompletionAssistInterface(m_editorWidget->textDocument()->filePath().toString(),
                                               m_editorWidget->document(), m_position,
                                               ExplicitlyInvoked, m_snapshot,
                                               ProjectPart::HeaderPaths(),
                                               languageFeatures);
        InternalCppCompletionAssistProcessor processor;

        const Tests::IAssistProposalScopedPointer proposal(processor.perform(ai));
        if (!proposal.d)
            return completions;
        IAssistProposalModel *model = proposal.d->model();
        if (!model)
            return completions;
        CppAssistProposalModel *listmodel = dynamic_cast<CppAssistProposalModel *>(model);
        if (!listmodel)
            return completions;

        const int pos = proposal.d->basePosition();
        const int length = m_position - pos;
        const QString prefix = Convenience::textAt(QTextCursor(m_editorWidget->document()), pos,
                                                   length);
        if (!prefix.isEmpty())
            listmodel->filter(prefix);
        if (listmodel->isSortable(prefix))
            listmodel->sort(prefix);

        for (int i = 0; i < listmodel->size(); ++i)
            completions << listmodel->text(i);

        if (replaceAccessOperator)
            *replaceAccessOperator = listmodel->m_replaceDotForArrow;

        return completions;
    }

    void insertText(const QByteArray &text)
    {
        Utils::ChangeSet change;
        change.insert(m_position, QLatin1String(text));
        QTextCursor cursor(m_textDocument);
        change.apply(&cursor);
        m_position += text.length();
    }

private:
    QByteArray m_source;
    int m_position;
    Snapshot m_snapshot;
    QScopedPointer<Tests::TemporaryDir> m_temporaryDir;
    TextEditorWidget *m_editorWidget;
    QTextDocument *m_textDocument;
    IEditor *m_editor;
};

bool isProbablyGlobalCompletion(const QStringList &list)
{
    const int numberOfPrimitivesAndBasicKeywords = (T_LAST_PRIMITIVE - T_FIRST_PRIMITIVE)
            + (T_FIRST_OBJC_AT_KEYWORD - T_FIRST_KEYWORD);

    return list.size() >= numberOfPrimitivesAndBasicKeywords
        && list.contains(QLatin1String("override"))
        && list.contains(QLatin1String("final"))
        && list.contains(QLatin1String("if"))
        && list.contains(QLatin1String("bool"));
}

} // anonymous namespace

void CppToolsPlugin::test_completion_basic_1()
{
    const QByteArray source =
            "class Foo\n"
            "{\n"
            "    void foo();\n"
            "    int m;\n"
            "};\n"
            "\n"
            "void func() {\n"
            "    Foo f;\n"
            "    @\n"
            "}";
    CompletionTestCase test(source);
    QVERIFY(test.succeededSoFar());

    QStringList basicCompletions = test.getCompletions();
    QVERIFY(!basicCompletions.contains(QLatin1String("foo")));
    QVERIFY(!basicCompletions.contains(QLatin1String("m")));
    QVERIFY(basicCompletions.contains(QLatin1String("Foo")));
    QVERIFY(basicCompletions.contains(QLatin1String("func")));
    QVERIFY(basicCompletions.contains(QLatin1String("f")));

    test.insertText("f.");

    QStringList memberCompletions = test.getCompletions();
    QVERIFY(memberCompletions.contains(QLatin1String("foo")));
    QVERIFY(memberCompletions.contains(QLatin1String("m")));
    QVERIFY(!memberCompletions.contains(QLatin1String("func")));
    QVERIFY(!memberCompletions.contains(QLatin1String("f")));
}

void CppToolsPlugin::test_completion_prefix_first_QTCREATORBUG_8737()
{
    const QByteArray source =
            "void f()\n"
            "{\n"
            "    int a_b_c, a_c, a_c_a;\n"
            "    @;\n"
            "}\n"
            ;
    CompletionTestCase test(source, "a_c");
    QVERIFY(test.succeededSoFar());

    QStringList completions = test.getCompletions();

    QVERIFY(completions.size() >= 2);
    QCOMPARE(completions.at(0), QLatin1String("a_c"));
    QCOMPARE(completions.at(1), QLatin1String("a_c_a"));
    QVERIFY(completions.contains(QLatin1String("a_b_c")));
}

void CppToolsPlugin::test_completion_prefix_first_QTCREATORBUG_9236()
{
    const QByteArray source =
            "class r_etclass\n"
            "{\n"
            "public:\n"
            "    int raEmTmber;\n"
            "    void r_e_t(int re_t)\n"
            "    {\n"
            "        int r_et;\n"
            "        int rETUCASE;\n"
            "        @\n"
            "    }\n"
            "};\n"
            ;
    CompletionTestCase test(source, "ret");
    QVERIFY(test.succeededSoFar());

    QStringList completions = test.getCompletions();
    QVERIFY(completions.size() >= 2);
    QCOMPARE(completions.at(0), QLatin1String("return"));
    QCOMPARE(completions.at(1), QLatin1String("rETUCASE"));
    QVERIFY(completions.contains(QLatin1String("r_etclass")));
    QVERIFY(completions.contains(QLatin1String("raEmTmber")));
    QVERIFY(completions.contains(QLatin1String("r_e_t")));
    QVERIFY(completions.contains(QLatin1String("re_t")));
    QVERIFY(completions.contains(QLatin1String("r_et")));
}

void CppToolsPlugin::test_completion_template_function()
{
    QFETCH(QByteArray, code);
    QFETCH(QStringList, expectedCompletions);

    CompletionTestCase test(code);
    QVERIFY(test.succeededSoFar());

    QStringList actualCompletions = test.getCompletions();
    QString errorPattern(QLatin1String("Completion not found: %1"));
    foreach (const QString &completion, expectedCompletions) {
        QByteArray errorMessage = errorPattern.arg(completion).toUtf8();
        QVERIFY2(actualCompletions.contains(completion), errorMessage.data());
    }
}

void CppToolsPlugin::test_completion_template_function_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QStringList>("expectedCompletions");

    QByteArray code;
    QStringList completions;

    code =
           "template <class tclass, typename tname, int tint>\n"
           "tname Hello(const tclass &e)\n"
           "{\n"
           "    tname e2 = e;\n"
           "    @\n"
           "}";

    completions.append(QLatin1String("tclass"));
    completions.append(QLatin1String("tname"));
    completions.append(QLatin1String("tint"));

    QTest::newRow("case: template parameters in template function body")
            << code << completions;

    completions.clear();

    code =
           "template <class tclass, typename tname, int tint>\n"
           "tname Hello(const tclass &e, @)\n"
           "{\n"
           "    tname e2 = e;\n"
           "}";

    completions.append(QLatin1String("tclass"));
    completions.append(QLatin1String("tname"));
    completions.append(QLatin1String("tint"));

    QTest::newRow("case: template parameters in template function parameters list")
            << code << completions;
}

void CppToolsPlugin::test_completion()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, expectedCompletions);

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());

    QStringList actualCompletions = test.getCompletions();
    actualCompletions.sort();
    expectedCompletions.sort();

    QEXPECT_FAIL("template_as_base: typedef not available in derived",
                 "We can live with that...", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function_cxx11", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function_anon", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_class_accessed_in_member_func_cxx11", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_class_accessed_in_member_func_inline_cxx11", "QTCREATORBUG-13757", Abort);
    QCOMPARE(actualCompletions, expectedCompletions);
}

void CppToolsPlugin::test_global_completion_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QByteArray>("prefix");
    QTest::addColumn<QStringList>("requiredCompletionItems");

    // Check that special completion after '&' for Qt5 signal/slots does not
    // interfere global completion after '&'
    QTest::newRow("global completion after & in return expression")
         << _("void f() { foo(myObject, @); }\n")
         << _("&")
         << QStringList();
    QTest::newRow("global completion after & in function argument")
         << _("int f() { return @; }\n")
         << _("&")
         << QStringList();

    // Check global completion after one line comments
    const QByteArray codeTemplate = "int myGlobal;\n"
                                    "<REPLACEMENT>\n"
                                    "@\n";
    const QStringList replacements = QStringList()
            << QLatin1String("// text")
            << QLatin1String("// text.")
            << QLatin1String("/// text")
            << QLatin1String("/// text.")
               ;
    foreach (const QString &replacement, replacements) {
        QByteArray code = codeTemplate;
        code.replace("<REPLACEMENT>", replacement.toUtf8());
        const QByteArray tag = _("completion after comment: ") + replacement.toUtf8();
        QTest::newRow(tag) << code << QByteArray() << QStringList(QLatin1String("myGlobal"));
    }
}

void CppToolsPlugin::test_global_completion()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, requiredCompletionItems);

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());
    const QStringList completions = test.getCompletions();
    QVERIFY(isProbablyGlobalCompletion(completions));
    QVERIFY(completions.toSet().contains(requiredCompletionItems.toSet()));
}

static void enumTestCase(const QByteArray &tag, const QByteArray &source,
                         const QByteArray &prefix = QByteArray())
{
    QByteArray fullSource = source;
    fullSource.replace('$', "enum E { val1, val2, val3 };");
    QTest::newRow(tag) << fullSource << (prefix + "val")
            << (QStringList()
                << QLatin1String("val1")
                << QLatin1String("val2")
                << QLatin1String("val3"));

    QTest::newRow(tag + "_cxx11") << fullSource << (prefix + "E::")
            << (QStringList()
                << QLatin1String("E")
                << QLatin1String("val1")
                << QLatin1String("val2")
                << QLatin1String("val3"));

    fullSource.replace("enum E ", "enum ");
    QTest::newRow(tag + "_anon") << fullSource << (prefix + "val")
            << (QStringList()
                << QLatin1String("val1")
                << QLatin1String("val2")
                << QLatin1String("val3"));
}

void CppToolsPlugin::test_completion_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QByteArray>("prefix");
    QTest::addColumn<QStringList>("expectedCompletions");

    QTest::newRow("forward_declarations_present") << _(
            "class Foo\n"
            "{\n"
            "    struct Bar;\n"
            "    int i;\n"
            "};\n"
            "\n"
            "struct Foo::Bar \n"
            "{\n"
            "    Bar() {}\n"
            "};\n"
            "\n"
            "@\n"
        ) << _("Foo::Bar::") << (QStringList()
            << QLatin1String("Bar"));

    QTest::newRow("inside_parentheses_c_style_conversion") << _(
            "class Base\n"
            "{\n"
            "    int i_base;\n"
            "};\n"
            "\n"
            "class Derived : public Base\n"
            "{\n"
            "    int i_derived;\n"
            "};\n"
            "\n"
            "void fun()\n"
            "{\n"
            "    Base *b = new Derived;\n"
            "    if (1)\n"
            "        @;\n"
            "}\n"
        ) << _("((Derived *)b)->") << (QStringList()
            << QLatin1String("Derived")
            << QLatin1String("Base")
            << QLatin1String("i_derived")
            << QLatin1String("i_base"));

    QTest::newRow("inside_parentheses_cast_operator_conversion") << _(
            "class Base\n"
            "{\n"
            "    int i_base;\n"
            "};\n"
            "\n"
            "class Derived : public Base\n"
            "{\n"
            "    int i_derived;\n"
            "};\n"
            "\n"
            "void fun()\n"
            "{\n"
            "    Base *b = new Derived;\n"
            "    if (1)\n"
            "        @;\n"
            "}\n"
        ) << _("(static_cast<Derived *>(b))->") << (QStringList()
            << QLatin1String("Derived")
            << QLatin1String("Base")
            << QLatin1String("i_derived")
            << QLatin1String("i_base"));

    QTest::newRow("template_1") << _(
            "template <class T>\n"
            "class Foo\n"
            "{\n"
            "    typedef T Type;\n"
            "    T foo();\n"
            "    T m;\n"
            "};\n"
            "\n"
            "void func() {\n"
            "    Foo f;\n"
            "    @\n"
            "}"
        ) << _("Foo::") << (QStringList()
              << QLatin1String("Foo")
              << QLatin1String("Type")
              << QLatin1String("foo")
              << QLatin1String("m"));

    QTest::newRow("template_2") << _(
            "template <class T>\n"
            "struct List\n"
            "{\n"
            "    T &at(int);\n"
            "};\n"
            "\n"
            "struct Tupple { int a; int b; };\n"
            "\n"
            "void func() {\n"
            "    List<Tupple> l;\n"
            "    @\n"
            "}"
        ) << _("l.at(0).") << (QStringList()
            << QLatin1String("Tupple")
            << QLatin1String("a")
            << QLatin1String("b"));

    QTest::newRow("template_3") << _(
            "template <class T>\n"
            "struct List\n"
            "{\n"
            "    T t;\n"
            "};\n"
            "\n"
            "struct Tupple { int a; int b; };\n"
            "\n"
            "void func() {\n"
            "    List<Tupple> l;\n"
            "    @\n"
            "}"
        ) << _("l.t.") << (QStringList()
            << QLatin1String("Tupple")
            << QLatin1String("a")
            << QLatin1String("b"));

    QTest::newRow("template_4") << _(
            "template <class T>\n"
            "struct List\n"
            "{\n"
            "    typedef T U;\n"
            "    U u;\n"
            "};\n"
            "\n"
            "struct Tupple { int a; int b; };\n"
            "\n"
            "void func() {\n"
            "    List<Tupple> l;\n"
            "    @\n"
            "}"
        ) << _("l.u.") << (QStringList()
            << QLatin1String("Tupple")
            << QLatin1String("a")
            << QLatin1String("b"));

    QTest::newRow("template_5") << _(
            "template <class T>\n"
            "struct List\n"
            "{\n"
            "    T u;\n"
            "};\n"
            "\n"
            "struct Tupple { int a; int b; };\n"
            "\n"
            "void func() {\n"
            "    typedef List<Tupple> LT;\n"
            "    LT l;"
            "    @\n"
            "}"
        ) << _("l.u.") << (QStringList()
            << QLatin1String("Tupple")
            << QLatin1String("a")
            << QLatin1String("b"));

    QTest::newRow("template_6") << _(
            "class Item\n"
            "{\n"
            "    int i;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "class Container\n"
            "{\n"
            "    T get();\n"
            "};\n"
            "\n"
            "template <typename T> class Container;\n"
            "\n"
            "class ItemContainer: public Container<Item>\n"
            "{};\n"
            "ItemContainer container;\n"
            "@\n"
        ) << _("container.get().") << (QStringList()
            << QLatin1String("Item")
            << QLatin1String("i"));

    QTest::newRow("template_7") << _(
            "struct Test\n"
            "{\n"
            "   int i;\n"
            "};\n"
            "\n"
            "template<typename T>\n"
            "struct TemplateClass\n"
            "{\n"
            "    T* ptr;\n"
            "\n"
            "    typedef T element_type;\n"
            "    TemplateClass(T* t) : ptr(t) {}\n"
            "    element_type* operator->()\n"
            "    {\n"
            "        return ptr;\n"
            "    }\n"
            "};\n"
            "\n"
            "TemplateClass<Test> p(new Test);\n"
            "@\n"
        ) << _("p->") << (QStringList()
            << QLatin1String("Test")
            << QLatin1String("i"));

    QTest::newRow("type_of_pointer_is_typedef") << _(
            "typedef struct Foo\n"
            "{\n"
            "    int foo;\n"
            "} Foo;\n"
            "Foo *bar;\n"
            "@\n"
        ) << _("bar->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("foo"));

    QTest::newRow("instantiate_full_specialization") << _(
            "template<typename T>\n"
            "struct Template\n"
            "{\n"
            "   int templateT_i;\n"
            "};\n"
            "\n"
            "template<>\n"
            "struct Template<char>\n"
            "{\n"
            "    int templateChar_i;\n"
            "};\n"
            "\n"
            "Template<char> templateChar;\n"
            "@\n"
        ) << _("templateChar.") << (QStringList()
            << QLatin1String("Template")
            << QLatin1String("templateChar_i"));

    QTest::newRow("template_as_base: base as template directly") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "\n"
            "void func() {\n"
            "    Other<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Other")
            << QLatin1String("otherMember"));

    QTest::newRow("template_as_base: base as class template") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Other")
            << QLatin1String("otherMember")
            << QLatin1String("More")
            << QLatin1String("moreMember"));

    QTest::newRow("template_as_base: base as globally qualified class template") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public ::Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Other")
            << QLatin1String("otherMember")
            << QLatin1String("More")
            << QLatin1String("moreMember"));

    QTest::newRow("template_as_base: base as namespace qualified class template") << _(
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "}\n"
            "template <class T> class More : public NS::Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Other")
            << QLatin1String("otherMember")
            << QLatin1String("More")
            << QLatin1String("moreMember"));

    QTest::newRow("template_as_base: base as nested template name") << _(
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Delegate { typedef Data<T> Type; };\n"
            "}\n"
            "template <class T> class Final : public NS::Delegate<T>::Type { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Final")
            << QLatin1String("finalMember"));

    QTest::newRow("template_as_base: base as nested template name in non-template") << _(
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Delegate { typedef Data<T> Type; };\n"
            "}\n"
            "class Final : public NS::Delegate<Data>::Type { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Final")
            << QLatin1String("finalMember"));

    QTest::newRow("template_as_base: base as template name in non-template") << _(
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "}\n"
            "class Final : public NS::Other<Data> { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final c;\n"
            "    @\n"
            "}"
        ) << _("c.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember")
            << QLatin1String("Final")
            << QLatin1String("finalMember")
            << QLatin1String("Other")
            << QLatin1String("otherMember"));

    QTest::newRow("template_as_base: typedef not available in derived") << _(
            "class Data { int dataMember; };\n"
            "template <class T> struct Base { typedef T F; };\n"
            "template <class T> struct Derived : Base<T> { F f; };\n"
            "\n"
            "void func() {\n"
            "    Derived<Data> d;\n"
            "    @\n"
            "}\n"
        ) << _("d.f.") << QStringList();

    QTest::newRow("template_as_base: explicit typedef from base") << _(
            "class Data { int dataMember; };\n"
            "template <class T> struct Base { typedef T F; };\n"
            "template <class T> struct Derived : Base<T>\n"
            "{\n"
            "   typedef typename Base<T>::F F;\n"
            "   F f;\n"
            "};\n"
            "\n"
            "void func() {\n"
            "    Derived<Data> d;\n"
            "    @\n"
            "}\n"
        ) << _("d.f.") << (QStringList()
            << QLatin1String("Data")
            << QLatin1String("dataMember"));

    QTest::newRow("explicit_instantiation") << _(
            "template<class T>\n"
            "struct Foo { T bar; };\n"
            "\n"
            "template class Foo<int>;\n"
            "\n"
            "void func()\n"
            "{\n"
            "    Foo<int> foo;\n"
            "    @\n"
            "}\n"
        ) << _("foo.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("use_global_identifier_as_base_class: derived as global and base as global") << _(
            "struct Global\n"
            "{\n"
            "    int int_global;\n"
            "};\n"
            "\n"
            "struct Final : ::Global\n"
            "{\n"
            "   int int_final;\n"
            "};\n"
            "\n"
            "Final c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_global")
            << QLatin1String("int_final")
            << QLatin1String("Final")
            << QLatin1String("Global"));

    QTest::newRow("use_global_identifier_as_base_class: derived is inside namespace. "
                  "base as global") << _(
            "struct Global\n"
            "{\n"
            "    int int_global;\n"
            "};\n"
            "\n"
            "namespace NS\n"
            "{\n"
            "struct Final : ::Global\n"
            "{\n"
            "   int int_final;\n"
            "};\n"
            "}\n"
            "\n"
            "NS::Final c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_global")
            << QLatin1String("int_final")
            << QLatin1String("Final")
            << QLatin1String("Global"));

    QTest::newRow("use_global_identifier_as_base_class: derived is enclosed by template. "
                  "base as global") << _(
            "struct Global\n"
            "{\n"
            "    int int_global;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "struct Enclosing\n"
            "{\n"
            "struct Final : ::Global\n"
            "{\n"
            "   int int_final;\n"
            "};\n"
            "};\n"
            "\n"
            "Enclosing<int>::Final c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_global")
            << QLatin1String("int_final")
            << QLatin1String("Final")
            << QLatin1String("Global"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class is derived class") << _(
            "struct A : A\n"
            "{\n"
            "   int int_a;\n"
            "};\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class is derived class. "
                  "class is in namespace") << _(
            "namespace NS\n"
            "{\n"
            "struct A : A\n"
            "{\n"
            "   int int_a;\n"
            "};\n"
            "}\n"
            "\n"
            "NS::A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class is derived class. "
                  "class is in namespace. use scope operator for base class") << _(
            "namespace NS\n"
            "{\n"
            "struct A : NS::A\n"
            "{\n"
            "   int int_a;\n"
            "};\n"
            "}\n"
            "\n"
            "NS::A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class has the same name as "
                  "derived but in different namespace") << _(
            "namespace NS1\n"
            "{\n"
            "struct A\n"
            "{\n"
            "   int int_ns1_a;\n"
            "};\n"
            "}\n"
            "namespace NS2\n"
            "{\n"
            "struct A : NS1::A\n"
            "{\n"
            "   int int_ns2_a;\n"
            "};\n"
            "}\n"
            "\n"
            "NS2::A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_ns1_a")
            << QLatin1String("int_ns2_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class has the same name as "
                  "derived (in namespace) but is nested by different class") << _(
            "struct Enclosing\n"
            "{\n"
            "struct A\n"
            "{\n"
            "   int int_enclosing_a;\n"
            "};\n"
            "};\n"
            "namespace NS2\n"
            "{\n"
            "struct A : Enclosing::A\n"
            "{\n"
            "   int int_ns2_a;\n"
            "};\n"
            "}\n"
            "\n"
            "NS2::A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_enclosing_a")
            << QLatin1String("int_ns2_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class has the same name as "
                  "derived (nested) but is nested by different class") << _(
            "struct EnclosingBase\n"
            "{\n"
            "struct A\n"
            "{\n"
            "   int int_enclosing_base_a;\n"
            "};\n"
            "};\n"
            "struct EnclosingDerived\n"
            "{\n"
            "struct A : EnclosingBase::A\n"
            "{\n"
            "   int int_enclosing_derived_a;\n"
            "};\n"
            "};\n"
            "\n"
            "EnclosingDerived::A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_enclosing_base_a")
            << QLatin1String("int_enclosing_derived_a")
            << QLatin1String("A"));

    QTest::newRow("base_class_has_name_the_same_as_derived: base class is derived class. "
                  "class is a template") << _(
            "template <typename T>\n"
            "struct A : A\n"
            "{\n"
            "   int int_a;\n"
            "};\n"
            "\n"
            "A<int> c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("int_a")
            << QLatin1String("A"));

    QTest::newRow("cyclic_inheritance: direct cyclic inheritance") << _(
            "struct B;\n"
            "struct A : B { int _a; };\n"
            "struct B : A { int _b; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("A")
            << QLatin1String("_a")
            << QLatin1String("B")
            << QLatin1String("_b"));

    QTest::newRow("cyclic_inheritance: indirect cyclic inheritance") << _(
            "struct C;\n"
            "struct A : C { int _a; };\n"
            "struct B : A { int _b; };\n"
            "struct C : B { int _c; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("A")
            << QLatin1String("_a")
            << QLatin1String("B")
            << QLatin1String("_b")
            << QLatin1String("C")
            << QLatin1String("_c"));

    QTest::newRow("cyclic_inheritance: indirect cyclic inheritance") << _(
            "struct B;\n"
            "struct A : B { int _a; };\n"
            "struct C { int _c; };\n"
            "struct B : C, A { int _b; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("A")
            << QLatin1String("_a")
            << QLatin1String("B")
            << QLatin1String("_b")
            << QLatin1String("C")
            << QLatin1String("_c"));

    QTest::newRow("cyclic_inheritance: direct cyclic inheritance with templates") << _(
            "template< typename T > struct C;\n"
            "template< typename T, typename S > struct D : C< S >\n"
            "{\n"
            "   T _d_t;\n"
            "   S _d_s;\n"
            "};\n"
            "template< typename T > struct C : D< T, int >\n"
            "{\n"
            "   T _c_t;\n"
            "};\n"
            "\n"
            "D<int, float> c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("D")
            << QLatin1String("_d_t")
            << QLatin1String("_d_s")
            << QLatin1String("C")
            << QLatin1String("_c_t"));

    QTest::newRow("cyclic_inheritance: indirect cyclic inheritance with templates") << _(
            "template< typename T > struct C;\n"
            "template< typename T, typename S > struct D : C< S >\n"
            "{\n"
            "   T _d_t;\n"
            "   S _d_s;\n"
            "};\n"
            "template< typename T > struct B : D< T, int >\n"
            "{\n"
            "   T _b_t;\n"
            "};\n"
            "template< typename T > struct C : B<T>\n"
            "{\n"
            "   T _c_t;\n"
            "};\n"
            "\n"
            "D<int, float> c;\n"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("D")
            << QLatin1String("_d_t")
            << QLatin1String("_d_s")
            << QLatin1String("C")
            << QLatin1String("_c_t")
            << QLatin1String("B")
            << QLatin1String("_b_t"));

    QTest::newRow("cyclic_inheritance: direct cyclic inheritance with templates. "
                  "more complex situation") << _(
           "namespace NS\n"
           "{\n"
           "template <typename T> struct SuperClass\n"
           "{\n"
           "    typedef T Type;\n"
           "    Type super_class_type;\n"
           "};\n"
           "}\n"
           "\n"
           "template <typename T>\n"
           "struct Class;\n"
           "\n"
           "template <typename T, typename S>\n"
           "struct ClassRecurse : Class<S>\n"
           "{\n"
           "    T class_recurse_t;\n"
           "    S class_recurse_s;\n"
           "};\n"
           "\n"
           "template <typename T>\n"
           "struct Class : ClassRecurse< T, typename ::NS::SuperClass<T>::Type >\n"
           "{\n"
           "    T class_t;\n"
           "};\n"
           "\n"
           "Class<int> c;\n"
           "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("Class")
            << QLatin1String("ClassRecurse")
            << QLatin1String("class_t")
            << QLatin1String("class_recurse_s")
            << QLatin1String("class_recurse_t"));

    QTest::newRow("enclosing_template_class: nested class with enclosing template class") << _(
            "template<typename T>\n"
            "struct Enclosing\n"
            "{\n"
            "    struct Nested { int int_nested; }; \n"
            "    int int_enclosing;\n"
            "};\n"
            "\n"
            "Enclosing<int>::Nested c;"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("Nested")
            << QLatin1String("int_nested"));

    QTest::newRow("enclosing_template_class: nested template class with enclosing template "
                  "class") << _(
            "template<typename T>\n"
            "struct Enclosing\n"
            "{\n"
            "    template<typename T> struct Nested { int int_nested; }; \n"
            "    int int_enclosing;\n"
            "};\n"
            "\n"
            "Enclosing<int>::Nested<int> c;"
            "@\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("Nested")
            << QLatin1String("int_nested"));

    QTest::newRow("instantiate_nested_class_when_enclosing_is_template") << _(
            "struct Foo \n"
            "{\n"
            "    int foo_i;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "struct Enclosing\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        T nested_t;\n"
            "    } nested;\n"
            "\n"
            "    T enclosing_t;\n"
            "};\n"
            "\n"
            "Enclosing<Foo> enclosing;\n"
            "@\n"
        ) << _("enclosing.nested.nested_t.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("foo_i"));

    QTest::newRow("instantiate_nested_of_nested_class_when_enclosing_is_template") << _(
            "struct Foo \n"
            "{\n"
            "    int foo_i;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "struct Enclosing\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        T nested_t;\n"
            "        struct NestedNested\n"
            "        {\n"
            "            T nestedNested_t;\n"
            "        } nestedNested;\n"
            "    } nested;\n"
            "\n"
            "    T enclosing_t;\n"
            "};\n"
            "\n"
            "Enclosing<Foo> enclosing;\n"
            "@\n"
        ) << _("enclosing.nested.nestedNested.nestedNested_t.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("foo_i"));

    QTest::newRow("instantiate_template_with_default_argument_type") << _(
            "struct Foo\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "\n"
            "template <typename T = Foo>\n"
            "struct Template\n"
            "{\n"
            "    T t;\n"
            "};\n"
            "\n"
            "Template<> templateWithDefaultTypeOfArgument;\n"
            "@\n"
        ) << _("templateWithDefaultTypeOfArgument.t.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("instantiate_template_with_default_argument_type_as_template") << _(
            "struct Foo\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "\n"
            "template <typename T>\n"
            "struct TemplateArg\n"
            "{\n"
            "    T t;\n"
            "};\n"
            "template <typename T, typename S = TemplateArg<T> >\n"
            "struct Template\n"
            "{\n"
            "    S s;\n"
            "};\n"
            "\n"
            "Template<Foo> templateWithDefaultTypeOfArgument;\n"
            "@\n"
        ) << _("templateWithDefaultTypeOfArgument.s.t.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedef_of_pointer") << _(
            "struct Foo { int bar; };\n"
            "typedef Foo *FooPtr;\n"
            "void main()\n"
            "{\n"
            "   FooPtr ptr;\n"
            "   @\n"
            "}"
        ) << _("ptr->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedef_of_pointer_inside_function") << _(
            "struct Foo { int bar; };\n"
            "void f()\n"
            "{\n"
            "   typedef Foo *FooPtr;\n"
            "   FooPtr ptr;\n"
            "   @\n"
            "}"
        ) << _("ptr->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedef_is_inside_function_before_declaration_block") << _(
            "struct Foo { int bar; };\n"
            "void f()\n"
            "{\n"
            "   typedef Foo *FooPtr;\n"
            "   if (true) {\n"
            "       FooPtr ptr;\n"
            "       @\n"
            "   }"
            "}"
        ) << _("ptr->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("resolve_complex_typedef_with_template") << _(
            "template <typename T>\n"
            "struct Template2\n"
            "{\n"
            "    typedef typename T::template Template1<T>::TT TemplateTypedef;\n"
            "    TemplateTypedef templateTypedef;\n"
            "};\n"
            "struct Foo\n"
            "{\n"
            "    int bar;\n"
            "    template <typename T>\n"
            "    struct Template1\n"
            "    {\n"
            "        typedef T TT;\n"
            "    };\n"
            "};\n"
            "void fun()\n"
            "{\n"
            "    Template2<Foo> template2;\n"
            "    @\n"
            "}\n"
        ) << _("template2.templateTypedef.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar")
            << QLatin1String("Template1"));

    QTest::newRow("template_specialization_with_pointer") << _(
            "template <typename T> struct Temp { T variable; };\n"
            "template <typename T> struct Temp<T *> { T *pointer; };\n"
            "void func()\n"
            "{\n"
            "    Temp<int*> templ;\n"
            "    @\n"
            "}"
        ) << _("templ.") << (QStringList()
            << QLatin1String("Temp")
            << QLatin1String("pointer"));

    QTest::newRow("template_specialization_with_reference") << _(
            "template <typename T> struct Temp { T variable; };\n"
            "template <typename T> struct Temp<T &> { T reference; };\n"
            "void func()\n"
            "{\n"
            "    Temp<int&> templ;\n"
            "    @\n"
            "}"
        ) << _("templ.") << (QStringList()
            << QLatin1String("Temp")
            << QLatin1String("reference"));

    QTest::newRow("typedef_using_templates1") << _(
            "namespace NS1\n"
            "{\n"
            "template<typename T>\n"
            "struct NS1Struct\n"
            "{\n"
            "    typedef T *pointer;\n"
            "    pointer bar;\n"
            "};\n"
            "}\n"
            "namespace NS2\n"
            "{\n"
            "using NS1::NS1Struct;\n"
            "\n"
            "template <typename T>\n"
            "struct NS2Struct\n"
            "{\n"
            "    typedef NS1Struct<T> NS1StructTypedef;\n"
            "    typedef typename NS1StructTypedef::pointer pointer;\n"
            "    pointer p;\n"
            "};\n"
            "}\n"
            "struct Foo\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "void fun()\n"
            "{\n"
            "    NS2::NS2Struct<Foo> s;\n"
            "    @\n"
            "}\n"
        ) << _("s.p->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedef_using_templates2") << _(
            "namespace NS1\n"
            "{\n"
            "template<typename T>\n"
            "struct NS1Struct\n"
            "{\n"
            "    typedef T *pointer;\n"
            "    pointer bar;\n"
            "};\n"
            "}\n"
            "namespace NS2\n"
            "{\n"
            "using NS1::NS1Struct;\n"
            "\n"
            "template <typename T>\n"
            "struct NS2Struct\n"
            "{\n"
            "    typedef NS1Struct<T> NS1StructTypedef;\n"
            "    typedef typename NS1StructTypedef::pointer pointer;\n"
            "    pointer p;\n"
            "};\n"
            "}\n"
            "struct Foo\n"
            "{\n"
            "    int bar;\n"
            "};\n"
            "void fun()\n"
            "{\n"
            "    NS2::NS2Struct<Foo>::pointer p;\n"
            "    @\n"
            "}\n"
        ) << _("p->") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("namespace_alias_with_many_namespace_declarations") << _(
            "namespace NS1\n"
            "{\n"
            "namespace NS2\n"
            "{\n"
            "struct Foo1\n"
            "{\n"
            "    int bar1;\n"
            "};\n"
            "}\n"
            "}\n"
            "namespace NS1\n"
            "{\n"
            "namespace NS2\n"
            "{\n"
            "struct Foo2\n"
            "{\n"
            "    int bar2;\n"
            "};\n"
            "}\n"
            "}\n"
            "namespace NS = NS1::NS2;\n"
            "int main()\n"
            "{\n"
            "    @\n"
            "}\n"
        ) << _("NS::") << (QStringList()
            << QLatin1String("Foo1")
            << QLatin1String("Foo2"));

    QTest::newRow("QTCREATORBUG9098") << _(
            "template <typename T>\n"
            "class B\n"
            "{\n"
            "public:\n"
            "    C<T> c;\n"
            "};\n"
            "template <typename T>\n"
            "class A\n"
            "{\n"
            "public:\n"
            "    B<T> b;\n"
            "    void fun()\n"
            "    {\n"
            "       @\n"
            "    }\n"
            "};\n"
        ) << _("b.") << (QStringList()
            << QLatin1String("c")
            << QLatin1String("B"));

    QTest::newRow("type_and_using_declaration: type and using declaration inside function") << _(
            "namespace NS\n"
            "{\n"
            "struct C { int m; };\n"
            "}\n"
            "void foo()\n"
            "{\n"
            "    using NS::C;\n"
            "    C c;\n"
            "    @\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type and using declaration in global "
                  "namespace") << _(
            "namespace NS\n"
            "{\n"
            "struct C { int m; };\n"
            "}\n"
            "using NS::C;\n"
            "void foo()\n"
            "{\n"
            "    C c;\n"
            "    @\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type in global namespace and using declaration in "
                  "NS namespace") << _(
            "struct C { int m; };\n"
            "namespace NS\n"
            "{\n"
            "   using ::C;\n"
            "   void foo()\n"
            "   {\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type in global namespace and using declaration "
                  "inside function in NS namespace") << _(
            "struct C { int m; };\n"
            "namespace NS\n"
            "{\n"
            "   void foo()\n"
            "   {\n"
            "       using ::C;\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type inside namespace NS1 and using declaration in "
                  "function inside NS2 namespace") << _(
            "namespace NS1\n"
            "{\n"
            "struct C { int m; };\n"
            "}\n"
            "namespace NS2\n"
            "{\n"
            "   void foo()\n"
            "   {\n"
            "       using NS1::C;\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type inside namespace NS1 and using declaration "
                  "inside NS2 namespace") << _(
            "namespace NS1\n"
            "{\n"
            "struct C { int m; };\n"
            "}\n"
            "namespace NS2\n"
            "{\n"
            "   using NS1::C;\n"
            "   void foo()\n"
            "   {\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("type_and_using_declaration: type in nested namespace and using in global") << _(
            "namespace Ns {\n"
            "namespace Nested {\n"
            "struct Foo\n"
            "{\n"
            "    void func();\n"
            "    int m_bar;\n"
            "};\n"
            "}\n"
            "}\n"
            "\n"
            "using namespace Ns::Nested;\n"
            "\n"
            "namespace Ns\n"
            "{\n"
            "void Foo::func()\n"
            "{\n"
            "    @\n"
            "}\n"
            "}\n"
        ) << _("m_") << (QStringList()
            << QLatin1String("m_bar"));

    QTest::newRow("instantiate_template_with_anonymous_class") << _(
            "template <typename T>\n"
            "struct S\n"
            "{\n"
            "    union { int i; char c; };\n"
            "};\n"
            "void fun()\n"
            "{\n"
            "   S<int> s;\n"
            "   @\n"
            "}\n"
        ) << _("s.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("i")
            << QLatin1String("c"));

    QTest::newRow("instantiate_template_function") << _(
            "template <typename T>\n"
            "T* templateFunction() { return 0; }\n"
            "struct A { int a; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("templateFunction<A>()->") << (QStringList()
            << QLatin1String("A")
            << QLatin1String("a"));

    QTest::newRow("nested_named_class_declaration_inside_function") << _(
            "int foo()\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        int i;\n"
            "    } n;\n"
            "    @;\n"
            "}\n"
        ) << _("n.") << (QStringList()
            << QLatin1String("Nested")
            << QLatin1String("i"));

    QTest::newRow("nested_class_inside_member_function") << _(
          "struct User { void use(); };\n"
          "void User::use()\n"
          "{\n"
          "   struct Foo { int bar; };\n"
          "   Foo foo;\n"
          "   @\n"
          "}\n"
    ) << _("foo.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("nested_typedef_inside_member_function") << _(
          "struct User { void use(); };\n"
          "template<class T>\n"
          "struct Pointer { T *operator->(); };\n"
          "struct Foo\n"
          "{\n"
          "   typedef Pointer<Foo> Ptr;\n"
          "   int bar;\n"
          "};\n"
          "\n"
          "void User::use()\n"
          "{\n"
          "   typedef Foo MyFoo;\n"
          "   MyFoo::Ptr myfoo;\n"
          "   @\n"
          "}\n"
    ) << _("myfoo->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("Ptr")
        << QLatin1String("bar"));

    QTest::newRow("nested_anonymous_class_QTCREATORBUG10876_1") << _(
            "struct EnclosingStruct\n"
            "{\n"
            "   int memberOfEnclosingStruct;\n"
            "   struct\n"
            "   {\n"
            "       int memberNestedAnonymousClass;\n"
            "   };\n"
            "   void fun()\n"
            "   {\n"
            "       @\n"
            "   }\n"
            "};\n"
        ) << _("member") << (QStringList()
            << QLatin1String("memberNestedAnonymousClass")
            << QLatin1String("memberOfEnclosingStruct"));

    QTest::newRow("nested_anonymous_class_QTCREATORBUG10876_2") << _(
            "struct EnclosingStruct\n"
            "{\n"
            "   int memberOfEnclosingStruct;\n"
            "   struct\n"
            "   {\n"
            "       int memberOfNestedAnonymousClass;\n"
            "       struct\n"
            "       {\n"
            "           int memberOfNestedOfNestedAnonymousClass;\n"
            "       };\n"
            "   };\n"
            "   void fun()\n"
            "   {\n"
            "       @\n"
            "   }\n"
            "};\n"
        ) << _("member") << (QStringList()
            << QLatin1String("memberOfNestedAnonymousClass")
            << QLatin1String("memberOfNestedOfNestedAnonymousClass")
            << QLatin1String("memberOfEnclosingStruct"));

    QTest::newRow("nested_anonymous_class_QTCREATORBUG10876_3") << _(
            "struct EnclosingStruct\n"
            "{\n"
            "   int memberOfEnclosingStruct;\n"
            "   struct\n"
            "   {\n"
            "       int memberOfNestedAnonymousClass;\n"
            "       struct\n"
            "       {\n"
            "           int memberOfNestedOfNestedAnonymousClass;\n"
            "       } nestedOfNestedAnonymousClass;\n"
            "   };\n"
            "   void fun()\n"
            "   {\n"
            "       @\n"
            "   }\n"
            "};\n"
        ) << _("nestedOfNestedAnonymousClass.") << (QStringList()
            << QLatin1String("memberOfNestedOfNestedAnonymousClass"));

    QTest::newRow("nested_anonymous_class_inside_function") << _(
            "void fun()\n"
            "{\n"
            "   union\n"
            "   {\n"
            "       int foo1;\n"
            "       int foo2;\n"
            "   };\n"
            "   @\n"
            "}\n"
        ) << _("foo") << (QStringList()
            << QLatin1String("foo1")
            << QLatin1String("foo2"));

    QTest::newRow("crash_cloning_template_class_QTCREATORBUG9329") << _(
            "struct A {};\n"
            "template <typename T>\n"
            "struct Templ {};\n"
            "struct B : A, Templ<A>\n"
            "{\n"
            "   int f()\n"
            "   {\n"
            "       @\n"
            "   }\n"
            "};\n"
        ) << _("this->") << (QStringList()
            << QLatin1String("A")
            << QLatin1String("B")
            << QLatin1String("Templ")
            << QLatin1String("f"));

    QTest::newRow("recursive_auto_declarations1_QTCREATORBUG9503") << _(
            "void f()\n"
            "{\n"
            "    auto object2 = object1;\n"
            "    auto object1 = object2;\n"
            "    @;\n"
            "}\n"
        ) << _("object1.") << (QStringList());

    QTest::newRow("recursive_auto_declarations2_QTCREATORBUG9503") << _(
            "void f()\n"
            "{\n"
            "    auto object3 = object1;\n"
            "    auto object2 = object3;\n"
            "    auto object1 = object2;\n"
            "    @;\n"
            "}\n"
        ) << _("object1.") << (QStringList());

    QTest::newRow("recursive_typedefs_declarations1") << _(
            "void f()\n"
            "{\n"
            "    typedef A B;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << (QStringList());

    QTest::newRow("recursive_typedefs_declarations2") << _(
            "void f()\n"
            "{\n"
            "    typedef A C;\n"
            "    typedef C B;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << (QStringList());

    QTest::newRow("recursive_using_declarations1") << _(
            "void f()\n"
            "{\n"
            "    using B = A;\n"
            "    using A = B;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << (QStringList());

    QTest::newRow("recursive_using_declarations2") << _(
            "void f()\n"
            "{\n"
            "    using C = A;\n"
            "    using B = C;\n"
            "    using A = B;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << (QStringList());

    QTest::newRow("recursive_using_typedef_declarations") << _(
            "void f()\n"
            "{\n"
            "    using B = A;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << (QStringList());

    QTest::newRow("recursive_typedefs_in_templates1") << _(
            "template<typename From>\n"
            "struct simplify_type {\n"
            "    typedef From SimpleType;\n"
            "};\n"
            "\n"
            "template<class To, class From>\n"
            "struct cast_retty {\n"
            "    typedef typename cast_retty_wrap<To, From,\n"
            "                     typename simplify_type<From>::SimpleType>::ret_type ret_type;\n"
            "};\n"
            "\n"
            "template<class To, class From, class SimpleFrom>\n"
            "struct cast_retty_wrap {\n"
            "    typedef typename cast_retty<To, SimpleFrom>::ret_type ret_type;\n"
            "};\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @;\n"
            "}\n"
        ) << _("cast_retty<T1, T2>::ret_type.") << (QStringList());

    QTest::newRow("recursive_typedefs_in_templates2") << _(
            "template<class T>\n"
            "struct recursive {\n"
            "  typedef typename recursive<T>::ret_type ret_type;\n"
            "};\n"
            "\n"
            "void f()\n"
            "{\n"
            "    @;\n"
            "}\n"
        ) << _("recursive<T1>::ret_type.foo") << (QStringList());

    QTest::newRow("class_declaration_inside_function_or_block_QTCREATORBUG3620: "
                  "class definition inside function") << _(
            "void foo()\n"
            "{\n"
            "    struct C { int m; };\n"
            "    C c;\n"
            "    @\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("class_declaration_inside_function_or_block_QTCREATORBUG3620: "
                  "class definition inside block inside function") << _(
            "void foo()\n"
            "{\n"
            "   {\n"
            "       struct C { int m; };\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("class_declaration_inside_function_or_block_QTCREATORBUG3620: "
                  "class definition with the same name inside different block inside function") << _(
            "void foo()\n"
            "{\n"
            "   {\n"
            "       struct C { int m1; };\n"
            "   }\n"
            "   {\n"
            "       struct C { int m2; };\n"
            "       C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m2"));

    QTest::newRow("namespace_alias_inside_function_or_block_QTCREATORBUG166: "
                  "namespace alias inside function") << _(
            "namespace NS1\n"
            "{\n"
            "namespace NS2\n"
            "{\n"
            "    struct C\n"
            "    {\n"
            "        int m;\n"
            "    };\n"
            "}\n"
            "}\n"
            "void foo()\n"
            "{\n"
            "   namespace NS = NS1::NS2;\n"
            "   NS::C c;\n"
            "   @\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("namespace_alias_inside_function_or_block_QTCREATORBUG166: "
                  "namespace alias inside block inside function") << _(
            "namespace NS1\n"
            "{\n"
            "namespace NS2\n"
            "{\n"
            "    struct C\n"
            "    {\n"
            "        int m;\n"
            "    };\n"
            "}\n"
            "}\n"
            "void foo()\n"
            "{\n"
            "   {\n"
            "       namespace NS = NS1::NS2;\n"
            "       NS::C c;\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("c.") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("m"));

    QTest::newRow("class_declaration_inside_function_or_block_QTCREATORBUG3620_static_member") << _(
            "void foo()\n"
            "{\n"
            "   {\n"
            "       struct C { static void staticFun1(); int m1; };\n"
            "   }\n"
            "   {\n"
            "       struct C { static void staticFun2(); int m2; };\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("C::") << (QStringList()
            << QLatin1String("C")
            << QLatin1String("staticFun2")
            << QLatin1String("m2"));

    enumTestCase(
            "enum_inside_block_inside_function",
             "void foo()\n"
             "{\n"
             "   {\n"
             "       $\n"
             "       @\n"
             "   }\n"
             "}\n"
    );

    enumTestCase(
            "enum_inside_function",
            "void foo()\n"
            "{\n"
            "   $\n"
            "   @\n"
            "}\n"
    );

    enumTestCase(
            "enum_in_function_in_struct_in_function",
            "void foo()\n"
            "{\n"
            "    struct S {\n"
            "        void fun()\n"
            "        {\n"
            "            $\n"
            "            @\n"
            "        }\n"
            "    };\n"
            "}\n"
    );

    enumTestCase(
            "enum_inside_class",
            "struct Foo\n"
            "{\n"
            "   $\n"
            "   @\n"
            "};\n",
            "Foo::"
    );

    enumTestCase(
            "enum_inside_namespace",
            "namespace Ns\n"
            "{\n"
            "   $\n"
            "   @\n"
            "}\n",
            "Ns::"
    );

    enumTestCase(
            "enum_inside_member_function",
            "class Foo { void func(); };\n"
            "void Foo::func()\n"
            "{\n"
            "   $\n"
            "   @\n"
            "}\n"
    );

    enumTestCase(
            "enum_in_class_accessed_in_member_func_inline",
            "class Foo\n"
            "{\n"
            "   $\n"
            "   void func()\n"
            "   {\n"
            "      @\n"
            "   }\n"
            "};\n"
    );

    enumTestCase(
            "enum_in_class_accessed_in_member_func",
            "class Foo\n"
            "{\n"
            "   $\n"
            "   void func();\n"
            "};\n"
            "void Foo::func()\n"
            "{\n"
            "   @\n"
            "}\n"
    );

    QTest::newRow("nested_anonymous_with___attribute__") << _(
            "struct Enclosing\n"
            "{\n"
            "   struct __attribute__((aligned(8)))\n"
            "   {\n"
            "       int i;\n"
            "   };\n"
            "};\n"
            "Enclosing e;\n"
            "@\n"
        ) << _("e.") << (QStringList()
            << QLatin1String("Enclosing")
            << QLatin1String("i"));

    QTest::newRow("lambdaCalls_1") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[](){ return new S; } ()->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("bar"));

    QTest::newRow("lambdaCalls_2") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[] { return new S; } ()->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("bar"));

    QTest::newRow("lambdaCalls_3") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() ->S* { return new S; } ()->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("bar"));

    QTest::newRow("lambdaCalls_4") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() throw() { return new S; } ()->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("bar"));

    QTest::newRow("lambdaCalls_5") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() throw()->S* { return new S; } ()->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("bar"));

    QTest::newRow("local_type_and_member_1") << _(
            "struct OtherType { int otherTypeMember; };\n"
            "void foo()\n"
            "{\n"
            "    struct LocalType\n"
            "    {\n"
            "        int localTypeMember;\n"
            "        OtherType ot;\n"
            "    };\n"
            "    LocalType lt;\n"
            "    @\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("local_type_and_member_2") << _(
            "void foo()\n"
            "{\n"
            "    struct OtherType { int otherTypeMember; };\n"
            "    struct LocalType\n"
            "    {\n"
            "        int localTypeMember;\n"
            "        OtherType ot;\n"
            "    };\n"
            "    LocalType lt;\n"
            "    @\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("local_type_and_member_3") << _(
            "void foo()\n"
            "{\n"
            "    struct OtherType { int otherTypeMember; };\n"
            "    {\n"
            "       struct LocalType\n"
            "       {\n"
            "           int localTypeMember;\n"
            "           OtherType ot;\n"
            "       };\n"
            "       LocalType lt;\n"
            "       @\n"
            "    }\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("local_type_and_member_4") << _(
            "namespace NS {struct OtherType { int otherTypeMember; };}\n"
            "void foo()\n"
            "{\n"
            "    struct LocalType\n"
            "    {\n"
            "        int localTypeMember;\n"
            "        NS::OtherType ot;\n"
            "    };\n"
            "    LocalType lt;\n"
            "    @\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("local_type_and_member_5") << _(
            "namespace NS {struct OtherType { int otherTypeMember; };}\n"
            "void foo()\n"
            "{\n"
            "    using namespace NS;\n"
            "    struct LocalType\n"
            "    {\n"
            "        int localTypeMember;\n"
            "        OtherType ot;\n"
            "    };\n"
            "    LocalType lt;\n"
            "    @\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("local_type_and_member_6") << _(
            "namespace NS {struct OtherType { int otherTypeMember; };}\n"
            "void foo()\n"
            "{\n"
            "    using NS::OtherType;\n"
            "    struct LocalType\n"
            "    {\n"
            "        int localTypeMember;\n"
            "        OtherType ot;\n"
            "    };\n"
            "    LocalType lt;\n"
            "    @\n"
            "}\n"
        ) << _("lt.ot.") << (QStringList()
            << QLatin1String("OtherType")
            << QLatin1String("otherTypeMember"));

    QTest::newRow("template_parameter_defined_inside_scope_of_declaration_QTCREATORBUG9169_1") << _(
            "struct A\n"
            "{\n"
            "    void foo();\n"
            "    struct B\n"
            "    {\n"
            "        int b;\n"
            "    };\n"
            "};\n"
            "template<typename T>\n"
            "struct Template\n"
            "{\n"
            "    T* get();\n"
            "};\n"
            "namespace foo\n"
            "{\n"
            "    struct B\n"
            "    {\n"
            "        int foo_b;\n"
            "    };\n"
            "}\n"
            "using namespace foo;\n"
            "void A::foo()\n"
            "{\n"
            "    Template<B> templ;\n"
            "    @\n"
            "}\n"
        ) << _("templ.get()->") << (QStringList()
            << QLatin1String("B")
            << QLatin1String("b"));

    QTest::newRow("template_parameter_defined_inside_scope_of_declaration_QTCREATORBUG9169_2") << _(
            "struct A\n"
            "{\n"
            "    void foo();\n"
            "    struct B\n"
            "    {\n"
            "        int b;\n"
            "    };\n"
            "};\n"
            "template<typename T>\n"
            "struct Template\n"
            "{\n"
            "    T t;\n"
            "};\n"
            "namespace foo\n"
            "{\n"
            "    struct B\n"
            "    {\n"
            "        int foo_b;\n"
            "    };\n"
            "}\n"
            "using namespace foo;\n"
            "void A::foo()\n"
            "{\n"
            "    Template<B> templ;\n"
            "    @\n"
            "}\n"
        ) << _("templ.t.") << (QStringList()
            << QLatin1String("B")
            << QLatin1String("b"));

    QTest::newRow("template_parameter_defined_inside_scope_of_declaration_QTCREATORBUG8852_1") << _(
            "template <typename T>\n"
            "struct QList\n"
            "{\n"
            "    T at(int i) const;\n"
            "};\n"
            "namespace ns\n"
            "{\n"
            "    struct Foo { int bar; };\n"
            "    void foo()\n"
            "    {\n"
            "        QList<Foo> list;\n"
            "       @\n"
            "    }\n"
            "}\n"
        ) << _("list.at(0).") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("template_parameter_defined_inside_scope_of_declaration_QTCREATORBUG8852_2") << _(
            "template <typename T>\n"
            "struct QList\n"
            "{\n"
            "    T at(int i) const;\n"
            "};\n"
            "namespace ns\n"
            "{\n"
            "    struct Foo { int bar; };\n"
            "    namespace nested\n"
            "    {\n"
            "       void foo()\n"
            "       {\n"
            "           QList<Foo> list;\n"
            "           @\n"
            "       }\n"
            "    }\n"
            "}\n"
        ) << _("list.at(0).") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("template_parameter_defined_inside_scope_of_declaration_QTCREATORBUG8852_3") << _(
            "template <typename T>\n"
            "struct QList\n"
            "{\n"
            "    T at(int i) const;\n"
            "};\n"
            "namespace ns\n"
            "{\n"
            "    struct Foo { int bar; };\n"
            "}\n"
            "void foo()\n"
            "{\n"
            "    using namespace ns;\n"
            "    QList<Foo> list;\n"
            "    @\n"
            "}\n"
        ) << _("list.at(0).") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    const QByteArray commonSignalSlotCompletionTestCode =
            "#define SIGNAL(a) #a\n"
            "#define SLOT(a) #a\n"
            "#define signals public\n"
            "#define slots\n"
            "#define Q_OBJECT virtual const QMetaObject *metaObject() const;"
            "\n"
            "namespace N {\n"
            "\n"
            "class Base : public QObject\n"
            "{\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    void hiddenFunction();\n"
            "    void baseFunction();\n"
            "signals:\n"
            "    void hiddenSignal();\n"
            "    void baseSignal1();\n"
            "    void baseSignal2(int newValue);\n"
            "public slots:\n"
            "    void baseSlot1();\n"
            "    void baseSlot2(int newValue);\n"
            "};\n"
            "\n"
            "class Derived : public Base\n"
            "{\n"
            "    Q_OBJECT\n"
            "public:\n"
            "    void hiddenFunction();\n"
            "    void derivedFunction();\n"
            "signals:\n"
            "    void hiddenSignal();\n"
            "    void derivedSignal1();\n"
            "    void derivedSignal2(int newValue);\n"
            "public slots:\n"
            "    void derivedSlot1();\n"
            "    void derivedSlot2(int newValue);\n"
            "};\n"
            "\n"
            "} // namespace N\n"
            "\n"
            "void client()\n"
            "{\n"
            "    N::Derived *myObject = new N::Derived;\n"
            "    @\n"
            "}\n";

    QTest::newRow("SIGNAL(")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, SIGNAL(") << (QStringList()
            << QLatin1String("baseSignal1()")
            << QLatin1String("baseSignal2(int)")
            << QLatin1String("hiddenSignal()")
            << QLatin1String("derivedSignal1()")
            << QLatin1String("derivedSignal2(int)"));

    QTest::newRow("SLOT(")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, SIGNAL(baseSignal1()), myObject, SLOT(") << (QStringList()
            << QLatin1String("baseSlot1()")
            << QLatin1String("baseSlot2(int)")
            << QLatin1String("derivedSlot1()")
            << QLatin1String("derivedSlot2(int)"));

    QTest::newRow("Qt5 signals: complete class after & at 2nd connect arg")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &") << (QStringList()
            << QLatin1String("N::Derived"));

    QTest::newRow("Qt5 signals: complete class after & at 4th connect arg")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &MyObject::timeout, myObject, &") << (QStringList()
            << QLatin1String("N::Derived"));

    QTest::newRow("Qt5 signals: complete signals")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived::") << (QStringList()
            << QLatin1String("baseSignal1")
            << QLatin1String("baseSignal2")
            << QLatin1String("hiddenSignal")
            << QLatin1String("derivedSignal1")
            << QLatin1String("derivedSignal2"));

    QTest::newRow("Qt5 slots")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived, myObject, &N::Derived::") << (QStringList()
           << QLatin1String("baseFunction")
           << QLatin1String("baseSignal1")
           << QLatin1String("baseSignal2")
           << QLatin1String("baseSlot1")
           << QLatin1String("baseSlot2")
           << QLatin1String("derivedFunction")
           << QLatin1String("derivedSignal1")
           << QLatin1String("derivedSignal2")
           << QLatin1String("derivedSlot1")
           << QLatin1String("derivedSlot2")
           << QLatin1String("hiddenFunction")
           << QLatin1String("hiddenSignal"));

    QTest::newRow("Qt5 signals: fallback to scope completion")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::") << (QStringList()
            << QLatin1String("Base")
            << QLatin1String("Derived"));

    QTest::newRow("Qt5 slots: fallback to scope completion")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived, myObject, &N::") << (QStringList()
            << QLatin1String("Base")
            << QLatin1String("Derived"));

    QTest::newRow("signals_hide_QPrivateSignal") << _(
            "#define SIGNAL(a) #a\n"
            "#define SLOT(a) #a\n"
            "#define signals public\n"
            "#define Q_OBJECT struct QPrivateSignal {};\n"
            "\n"
            "class QObject\n"
            "{\n"
            "public:\n"
            "    void connect(QObject *, char *, QObject *, char *);\n"
            "};\n"
            "\n"
            "class Timer : public QObject\n"
            "{\n"
            "    Q_OBJECT\n"
            "signals:\n"
            "    void timeout(QPrivateSignal);\n"
            "};\n"
            "\n"
            "void client()\n"
            "{\n"
            "    Timer *timer = new Timer;\n"
            "    @\n"
            "}\n"
        ) << _("connect(timer, SIGNAL(") << (QStringList()
            << QLatin1String("timeout()"));

    QTest::newRow("member_of_class_accessed_by_using_QTCREATORBUG9037_1") << _(
            "namespace NS { struct S { int member; void fun(); }; }\n"
            "using NS::S;\n"
            "void S::fun()\n"
            "{\n"
            "    @\n"
            "}\n"
        ) << _("mem") << (QStringList()
            << QLatin1String("member"));

    QTest::newRow("member_of_class_accessed_by_using_QTCREATORBUG9037_2") << _(
            "namespace NS \n"
            "{\n"
            "   namespace Internal\n"
            "   {\n"
            "   struct S { int member; void fun(); };\n"
            "   }\n"
            "   using Internal::S;\n"
            "}\n"
            "using NS::S;\n"
            "void S::fun()\n"
            "{\n"
            "    @\n"
            "}\n"
        ) << _("mem") << (QStringList()
            << QLatin1String("member"));

    QTest::newRow("no_binding_block_as_instantiationOrigin_QTCREATORBUG-11424") << _(
            "template <typename T>\n"
            "class QVector\n"
            "{\n"
            "public:\n"
            "   inline const_iterator constBegin() const;\n"
            "};\n"
            "\n"
            "typedef struct { double value; } V;\n"
            "\n"
            "double getValue(const QVector<V>& d) const {\n"
            "   typedef QVector<V>::ConstIterator Iter;\n"
            "   @\n"
            "}\n"
        ) << _("double val = d.constBegin()->") << (QStringList());

    QTest::newRow("nested_class_in_template_class_QTCREATORBUG-11752") << _(
            "template <typename T>\n"
            "struct Temp\n"
            "{\n"
            "   struct Nested1 { T t; };\n"
            "   struct Nested2 { Nested1 n1; };\n"
            "};\n"
            "struct Foo { int foo; };\n"
            "void fun() {\n"
            "   Temp<Foo>::Nested2 n2;\n"
            "   @\n"
            "}\n"
        ) << _("n2.n1.t.") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("infiniteLoopLocalTypedef_QTCREATORBUG-11999") << _(
            "template <typename T>\n"
            "struct Temp\n"
            "{\n"
            "   struct Nested\n"
            "   {\n"
            "       typedef Temp<T> TempT;\n"
            "       T t;\n"
            "   };\n"
            "   Nested nested;\n"
            "};\n"
            "struct Foo { int foo; };\n"
            "void fun() {\n"
            "   Temp<Foo> tempFoo;\n"
            "   @\n"
            "}\n"
        ) << _("tempFoo.nested.t.") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("lambda_parameter") << _(
            "auto func = [](int arg1) { return @; };\n"
        ) << _("ar") << (QStringList()
            << QLatin1String("arg1"));

    QTest::newRow("local_typedef_access_in_lambda") << _(
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "    typedef Foo F;\n"
            "    []() {\n"
            "        F f;\n"
            "        @\n"
            "    };\n"
            "}\n"
    ) << _("f.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("default_arguments_for_class_templates_and_base_class_QTCREATORBUG-12605") << _(
            "struct Foo { int foo; };\n"
            "template <typename T = Foo>\n"
            "struct Derived : T {};\n"
            "void fun() {\n"
            "   Derived<> derived;\n"
            "   @\n"
            "}\n"
        ) << _("derived.") << (QStringList()
            << QLatin1String("Derived")
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("default_arguments_for_class_templates_and_template_base_class_QTCREATORBUG-12606") << _(
            "struct Foo { int foo; };\n"
            "template <typename T>\n"
            "struct Base { T t; };\n"
            "template <typename T = Foo>\n"
            "struct Derived : Base<T> {};\n"
            "void fun() {\n"
            "   Derived<> derived;\n"
            "   @\n"
            "}\n"
        ) << _("derived.t.") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("template_specialization_and_initialization_with_pointer1") << _(
            "template <typename T>\n"
            "struct S {};\n"
            "template <typename T>\n"
            "struct S<T*> { T *t; };\n"
            "struct Foo { int foo; };\n"
            "void fun() {\n"
            "   S<Foo*> s;\n"
            "   @\n"
            "}\n"
        ) << _("s.t->") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    // this is not a valid code(is not compile) but it caused a crash
    QTest::newRow("template_specialization_and_initialization_with_pointer2") << _(
            "template <typename T1, typename T2 = int>\n"
            "struct S {};\n"
            "template <typename T1, typename T2>\n"
            "struct S<T1*> { T1 *t; };\n"
            "struct Foo { int foo; };\n"
            "void fun() {\n"
            "   S<Foo*> s;\n"
            "   @\n"
            "}\n"
        ) << _("s.t->") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("typedef_of_pointer_of_array_QTCREATORBUG-12703") << _(
            "struct Foo { int foo; };\n"
            "typedef Foo *FooArr[10];\n"
            "void fun() {\n"
            "   FooArr arr;\n"
            "   @\n"
            "}\n"
        ) << _("arr[0]->") << (QStringList()
            << QLatin1String("foo")
            << QLatin1String("Foo"));

    QTest::newRow("template_specialization_with_array1") << _(
            "template <typename T>\n"
            "struct S {};\n"
            "template <typename T>\n"
            "struct S<T[]> { int foo; };\n"
            "void fun() {\n"
            "   S<int[]> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << (QStringList()
        << QLatin1String("foo")
        << QLatin1String("S"));

    QTest::newRow("template_specialization_with_array2") << _(
            "template <typename T>\n"
            "struct S {};\n"
            "template <typename T, size_t N>\n"
            "struct S<T[N]> { int foo; };\n"
            "void fun() {\n"
            "   S<int[3]> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << (QStringList()
        << QLatin1String("foo")
        << QLatin1String("S"));

    QTest::newRow("template_specialization_with_array3") << _(
            "struct Bar {};\n"
            "template <typename T>\n"
            "struct S {};\n"
            "template <>\n"
            "struct S<Bar[]> { int foo; };\n"
            "void fun() {\n"
            "   S<int[]> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << (QStringList()
        << QLatin1String("S"));

    QTest::newRow("partial_specialization") << _(
            "struct b {};\n"
            "template<class X, class Y> struct s { float f; };\n"
            "template<class X> struct s<X, b> { int i; };\n"
            "\n"
            "void f()\n"
            "{\n"
            "    s<int, b> var;\n"
            "    @\n"
            "}\n"
    ) << _("var.") << (QStringList()
        << QLatin1String("i")
        << QLatin1String("s"));

    QTest::newRow("partial_specialization_with_pointer") << _(
            "struct b {};\n"
            "struct a : b {};\n"
            "template<class X, class Y> struct s { float f; };\n"
            "template<class X> struct s<X, b*> { int i; };\n"
            "template<class X> struct s<X, a*> { char j; };\n"
            "\n"
            "void f()\n"
            "{\n"
            "    s<int, a*> var;\n"
            "    @\n"
            "}\n"
    ) << _("var.") << (QStringList()
        << QLatin1String("j")
        << QLatin1String("s"));

    QTest::newRow("partial_specialization_templated_argument") << _(
            "template<class T> struct t {};\n"
            "\n"
            "template<class> struct s { float f; };\n"
            "template<class X> struct s<t<X>> { int i; };\n"
            "\n"
            "void f()\n"
            "{\n"
            "    s<t<char>> var;\n"
            "    @\n"
            "}\n"
    ) << _("var.") << (QStringList()
        << QLatin1String("i")
        << QLatin1String("s"));

    QTest::newRow("specialization_multiple_arguments") << _(
            "class false_type {};\n"
            "class true_type {};\n"
            "template<class T1, class T2> class and_type { false_type f; };\n"
            "template<> class and_type<true_type, true_type> { true_type t; };\n"
            "void func()\n"
            "{\n"
            "    and_type<true_type, false_type> a;\n"
            "    @;\n"
            "}\n"
     ) << _("a.") << (QStringList()
        << QLatin1String("f")
        << QLatin1String("and_type"));

    QTest::newRow("specialization_with_default_value") << _(
            "class Foo {};\n"
            "template<class T1 = Foo> class Temp;\n"
            "template<> class Temp<Foo> { int var; };\n"
            "void func()\n"
            "{\n"
            "    Temp<> t;\n"
            "    @\n"
            "}\n"
     ) << _("t.") << (QStringList()
        << QLatin1String("var")
        << QLatin1String("Temp"));

    QTest::newRow("auto_declaration_in_if_condition") << _(
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   if (auto s = new Foo) {\n"
            "      @\n"
            "   }\n"
            "}\n"
    ) << _("s->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("dereference_of_nested_type_opertor_*") << _(
            "template<typename T>\n"
            "struct QList\n"
            "{\n"
            "   struct iterator\n"
            "   {\n"
            "      T &operator*() { return t; }\n"
            "      T t;\n"
            "   };\n"
            "   iterator begin() { return iterator(); }\n"
            "};\n"
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   QList<Foo> list;\n"
            "   @\n"
            "}\n"
    ) << _("(*list.begin()).") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("dereference_of_nested_type_opertor_->") << _(
            "template<typename T>\n"
            "struct QList\n"
            "{\n"
            "   struct iterator\n"
            "   {\n"
            "      T *operator->() { return &t; }\n"
            "      T t;\n"
            "   };\n"
            "   iterator begin() { return iterator(); }\n"
            "};\n"
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   QList<Foo> list;\n"
            "   @\n"
            "}\n"
    ) << _("list.begin()->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("dereference_of_nested_type_opertor_*_and_auto") << _(
            "template<typename T>\n"
            "struct QList\n"
            "{\n"
            "   struct iterator\n"
            "   {\n"
            "      T &operator*() { return t; }\n"
            "      T t;\n"
            "   };\n"
            "   iterator begin() { return iterator(); }\n"
            "};\n"
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   QList<Foo> list;\n"
            "   auto a = list.begin();\n"
            "   @\n"
            "}\n"
    ) << _("(*a).") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("dereference_of_nested_type_opertor_->_and_auto") << _(
            "template<typename T>\n"
            "struct QList\n"
            "{\n"
            "   struct iterator\n"
            "   {\n"
            "      T *operator->() { return &t; }\n"
            "      T t;\n"
            "   };\n"
            "   iterator begin() { return iterator(); }\n"
            "};\n"
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   QList<Foo> list;\n"
            "   auto a = list.begin();\n"
            "   @\n"
            "}\n"
    ) << _("a->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("direct_nested_template_type_access") << _(
            "template<typename T>\n"
            "struct QList\n"
            "{\n"
            "   struct iterator\n"
            "   {\n"
            "      T *operator->() { return &t; }\n"
            "      T t;\n"
            "   };\n"
            "   iterator begin() { return iterator(); }\n"
            "};\n"
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   auto a = QList<Foo>::begin();\n"
            "   @\n"
            "}\n"
    ) << _("a.") << (QStringList()
        << QLatin1String("operator ->")
        << QLatin1String("t")
        << QLatin1String("iterator"));

    QTest::newRow("pointer_indirect_specialization") << _(
            "template<typename T>\n"
            "struct Traits { typedef typename T::pointer pointer; };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
            "\n"
            "template<typename T>\n"
            "class Temp\n"
            "{\n"
            "protected:\n"
            "   typedef Traits<T> TraitsT;\n"
            "\n"
            "public:\n"
            "   typedef typename TraitsT::pointer pointer;\n"
            "   pointer p;\n"
            "};\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp<Foo *> t;\n"
            "   @\n"
            "}\n"
    ) << _("t.p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("pointer_indirect_specialization_typedef") << _(
            "template<typename T>\n"
            "struct Traits { typedef typename T::pointer pointer; };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "class Temp\n"
            "{\n"
            "protected:\n"
            "   typedef Foo *FooPtr;\n"
            "   typedef Traits<FooPtr> TraitsT;\n"
            "\n"
            "public:\n"
            "   typedef typename TraitsT::pointer pointer;\n"
            "   pointer p;\n"
            "};\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp t;\n"
            "   @\n"
            "}\n"
    ) << _("t.p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("pointer_indirect_specialization_double_indirection") << _(
            "template<typename _Tp>\n"
            "struct Traits { };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct IndirectT\n"
            "{\n"
            "   typedef Traits<_Tp> TraitsT;\n"
            "   typedef typename TraitsT::pointer pointer;\n"
            "   pointer p;\n"
            "};\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Temp\n"
            "{\n"
            "   typedef _Tp *pointer;\n"
            "   typedef IndirectT<pointer> indirect;\n"
            "};\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp<Foo>::indirect t;\n"
            "   @\n"
            "}\n"
    ) << _("t.p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("instantiation_of_indirect_typedef") << _(
        "template<typename _Tp>\n"
        "struct Indirect { _Tp t; };\n"
        "\n"
        "template<typename T>\n"
        "struct Temp\n"
        "{\n"
        "   typedef T MyT;\n"
        "   typedef Indirect<MyT> indirect;\n"
        "};\n"
        "\n"
        "struct Foo { int bar; };\n"
        "\n"
        "void func()\n"
        "{\n"
        "   Temp<Foo>::indirect i;\n"
        "   @\n"
        "}\n"
    ) << _("i.t.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));;

    QTest::newRow("pointer_indirect_specialization_double_indirection_with_base") << _(
            "template<typename _Tp>\n"
            "struct Traits { };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Traits<_Tp*> { typedef _Tp *pointer; };\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct IndirectT\n"
            "{\n"
            "   typedef Traits<_Tp> TraitsT;\n"
            "   typedef typename TraitsT::pointer pointer;\n"
            "   pointer p;\n"
            "};\n"
            "\n"
            "template<typename _Tp>\n"
            "struct TempBase { typedef _Tp *pointer; };\n"
            "\n"
            "template<typename _Tp>\n"
            "struct Temp : public TempBase<_Tp>\n"
            "{\n"
            "   typedef TempBase<_Tp> _Base;\n"
            "   typedef typename _Base::pointer pointer;\n"
            "   typedef IndirectT<pointer> indirect;\n"
            "};\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp<Foo>::indirect t;\n"
            "   @\n"
            "}\n"
    ) << _("t.p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("recursive_instantiation_of_template_type") << _(
            "template<typename _Tp>\n"
            "struct Temp { typedef _Tp value_type; };\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp<Temp<Foo> >::value_type::value_type *p;\n"
            "   @\n"
            "}\n"
    ) << _("p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("recursive_instantiation_of_template_type_2") << _(
            "template<typename _Tp>\n"
            "struct Temp { typedef _Tp value_type; };\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   Temp<Temp<Foo>::value_type>::value_type *p;\n"
            "   @\n"
            "}\n"
    ) << _("p->") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("template_using_instantiation") << _(
            "template<typename _Tp>\n"
            "using T = _Tp;\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "    T<Foo> p;\n"
            "    @\n"
            "}\n"
    ) << _("p.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("nested_template_using_instantiation") << _(
            "struct Parent {\n"
            "    template<typename _Tp>\n"
            "    using T = _Tp;\n"
            "};\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "    Parent::T<Foo> p;\n"
            "    @;\n"
            "}\n"
     ) << _("p.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("nested_template_using_instantiation_in_template_class") << _(
            "template<typename ParentT>\n"
            "struct Parent {\n"
            "    template<typename _Tp>\n"
            "    using T = _Tp;\n"
            "};\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "    Parent<Foo>::T<Foo> p;\n"
            "    @;\n"
            "}\n"
     ) << _("p.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("recursive_nested_template_using_instantiation") << _(
            "struct Foo { int bar; };\n"
            "\n"
            "struct A { typedef Foo value_type; };\n"
            "\n"
            "template<typename T>\n"
            "struct Traits\n"
            "{\n"
            "    typedef Foo value_type;\n"
            "\n"
            "    template<typename _Tp>\n"
            "    using U = T;\n"
            "};\n"
            "\n"
            "template<typename T>\n"
            "struct Temp\n"
            "{\n"
            "    typedef Traits<T> TraitsT;\n"
            "    typedef typename T::value_type value_type;\n"
            "    typedef typename TraitsT::template U<Foo> rebind;\n"
            "};\n"
            "\n"
            "void func()\n"
            "{\n"
            "    typename Temp<typename Temp<A>::rebind>::value_type p;\n"
            "    @\n"
            "}\n"
    ) << _("p.") << (QStringList()
        << QLatin1String("Foo")
        << QLatin1String("bar"));

    QTest::newRow("qualified_name_in_nested_type") << _(
            "template<typename _Tp>\n"
            "struct Temp {\n"
            "    struct Nested {\n"
            "        typedef typename _Tp::Nested2 N;\n"
            "    };\n"
            "};\n"
            "\n"
            "struct Foo {\n"
            "    struct Nested2 {\n"
            "        int bar;\n"
            "    };\n"
            "};\n"
            "\n"
            "void func()\n"
            "{\n"
            "    Temp<Foo>::Nested::N p;\n"
            "    @;\n"
            "}\n"
    ) << _("p.") << (QStringList()
        << QLatin1String("Nested2")
        << QLatin1String("bar"));

    QTest::newRow("simple_decltype_declaration") << _(
            "struct Foo { int bar; };\n"
            "Foo foo;\n"
            "void fun() {\n"
            "   decltype(foo) s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedefed_decltype_declaration") << _(
            "struct Foo { int bar; };\n"
            "Foo foo;\n"
            "typedef decltype(foo) TypedefedFooWithDecltype;\n"
            "void fun() {\n"
            "   TypedefedFooWithDecltype s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("nested_instantiation_typedefed_decltype_declaration") << _(
            "template <typename T>\n"
            "struct Temp\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        static T f();\n"
            "        typedef decltype(f()) type;\n"
            "    };\n"
            "};\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void fun()\n"
            "{\n"
            "    Temp<Foo>::Nested::type s;\n"
            "    @\n"
            "}\n"
    ) << _("s.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("typedefed_decltype_of_template_function") << _(
            "template<typename T>\n"
            "static T f();\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void fun()\n"
            "{\n"
            "    decltype(f<Foo>()) s;\n"
            "    @\n"
            "}\n"
    ) << _("s.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

    QTest::newRow("nested_instantiation_typedefed_decltype_declaration_of_template_function") << _(
            "template <typename T, typename D = T>\n"
            "struct Temp\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        template<typename U> static T* __test(...);\n"
            "        typedef decltype(__test<D>(0)) type;\n"
            "    };\n"
            "};\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "    Temp<Foo>::Nested::type s;\n"
            "    @\n"
            "}\n"
    ) << _("s.") << (QStringList()
            << QLatin1String("Foo")
            << QLatin1String("bar"));

}

void CppToolsPlugin::test_completion_member_access_operator()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, expectedCompletions);
    QFETCH(bool, expectedReplaceAccessOperator);

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());

    bool replaceAccessOperator = false;
    QStringList completions = test.getCompletions(&replaceAccessOperator);

    completions.sort();
    expectedCompletions.sort();

    QCOMPARE(completions, expectedCompletions);
    QCOMPARE(replaceAccessOperator, expectedReplaceAccessOperator);
}

void CppToolsPlugin::test_completion_member_access_operator_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QByteArray>("prefix");
    QTest::addColumn<QStringList>("expectedCompletions");
    QTest::addColumn<bool>("expectedReplaceAccessOperator");

    QTest::newRow("member_access_operator") << _(
            "struct S { void t(); };\n"
            "void f() { S *s;\n"
            "@\n"
            "}\n"
        ) << _("s.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("t"))
        << true;

    QTest::newRow("typedef_of_type_and_decl_of_type_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S SType;\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;

    QTest::newRow("typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S *SType;\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << (QStringList())
        << false;

    QTest::newRow("typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S SType;\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S* SPtr;\n"
            "SPtr p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_type_no_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << (QStringList())
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("typedef_of_pointer_of_type_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef struct S SType;\n"
            "typedef struct SType *STypePtr;\n"
            "STypePtr p;\n"
            "@\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("typedef_of_pointer_of_type_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef struct S SType;\n"
            "typedef struct SType *STypePtr;\n"
            "STypePtr p;\n"
            "@\n"
        ) << _("p->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;
}
