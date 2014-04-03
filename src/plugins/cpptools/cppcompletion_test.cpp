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

#include "cppcompletionassist.h"
#include "cppmodelmanager.h"
#include "cpptoolsplugin.h"
#include "cpptoolstestcase.h"

#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/convenience.h>
#include <texteditor/plaintexteditor.h>

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

class CompletionTestCase : public CppTools::Tests::TestCase
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
        const QString fileName = QDir::tempPath() + QLatin1String("/file.h");
        QVERIFY(writeFile(fileName, m_source));

        // Open in editor
        m_editor = EditorManager::openEditor(fileName);
        QVERIFY(m_editor);
        closeEditorAtEndOfTestCase(m_editor);
        m_editorWidget = qobject_cast<TextEditor::BaseTextEditorWidget *>(m_editor->widget());
        QVERIFY(m_editorWidget);

        m_textDocument = m_editorWidget->document();

        // Get Document
        waitForFileInGlobalSnapshot(fileName);
        const Document::Ptr document = globalSnapshot().document(fileName);

        m_snapshot.insert(document);

        if (!textToInsert.isEmpty())
            insertText(textToInsert);

        m_succeededSoFar = true;
    }

    QStringList getCompletions(bool *replaceAccessOperator = 0) const
    {
        QStringList completions;
        CppCompletionAssistInterface *ai
            = new CppCompletionAssistInterface(m_editorWidget->document(), m_position,
                                               m_editorWidget->baseTextDocument()->filePath(),
                                               ExplicitlyInvoked, m_snapshot,
                                               QStringList(), QStringList());
        CppCompletionAssistProcessor processor;
        IAssistProposal *proposal = processor.perform(ai);
        if (!proposal)
            return completions;
        IAssistProposalModel *model = proposal->model();
        if (!model)
            return completions;
        CppAssistProposalModel *listmodel = dynamic_cast<CppAssistProposalModel *>(model);
        if (!listmodel)
            return completions;

        const int pos = proposal->basePosition();
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
    BaseTextEditorWidget *m_editorWidget;
    QTextDocument *m_textDocument;
    IEditor *m_editor;
};

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

    QEXPECT_FAIL("enum_in_function_in_struct_in_function", "doesn't work", Abort);
    QCOMPARE(actualCompletions, expectedCompletions);
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
            "        @\n"
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
            "        @\n"
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
            "template <typename T>\n"
            "struct Template\n"
            "{\n"
            "    T variable;\n"
            "};\n"
            "template <typename T>\n"
            "struct Template<T *>\n"
            "{\n"
            "    T *pointer;\n"
            "};\n"
            "Template<int*> templ;\n"
            "@\n"
        ) << _("templ.") << (QStringList()
            << QLatin1String("Template")
            << QLatin1String("pointer"));

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

    QTest::newRow("nested_class_declaration_with_object_name_inside_function") << _(
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
            "};\n"
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

    QTest::newRow("enum_inside_block_inside_function_cxx11_QTCREATORBUG5456") << _(
            "void foo()\n"
            "{\n"
            "   {\n"
            "       enum E { e1, e2, e3 };\n"
            "       @\n"
            "   }\n"
            "}\n"
        ) << _("E::") << (QStringList()
            << QLatin1String("E")
            << QLatin1String("e1")
            << QLatin1String("e2")
            << QLatin1String("e3"));

    QTest::newRow("enum_inside_function") << _(
            "void foo()\n"
            "{\n"
            "   enum E { val1, val2, val3 };\n"
            "   @\n"
            "}\n"
        ) << _("val") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("anon_enum_inside_function") << _(
            "void foo()\n"
            "{\n"
            "   enum { val1, val2, val3 };\n"
            "   @\n"
            "}\n"
        ) << _("val") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("enum_in_function_in_struct_in_function") << _(
            "void foo()\n"
            "{\n"
            "    struct S {\n"
            "        void fun()\n"
            "        {\n"
            "            enum E { val1, val2, val3 };\n"
            "            @\n"
            "        }\n"
            "    };\n"
            "}\n"
        ) << _("val") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("enum_inside_function_cxx11_QTCREATORBUG5456") << _(
            "void foo()\n"
            "{\n"
            "   enum E { e1, e2, e3 };\n"
            "   @\n"
            "}\n"
        ) << _("E::") << (QStringList()
            << QLatin1String("E")
            << QLatin1String("e1")
            << QLatin1String("e2")
            << QLatin1String("e3"));

    QTest::newRow("enum_inside_class") << _(
            "struct Foo\n"
            "{\n"
            "   enum E { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Foo::v") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("enum_inside_class_cxx11") << _(
            "struct Foo\n"
            "{\n"
            "   enum E { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Foo::E::") << (QStringList()
            << QLatin1String("E")
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("anon_enum_inside_class") << _(
            "struct Foo\n"
            "{\n"
            "   enum { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Foo::v") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("enum_inside_namespace") << _(
            "namespace Ns\n"
            "{\n"
            "   enum E { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Ns::v") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("enum_inside_namespace_cxx11") << _(
            "namespace Ns\n"
            "{\n"
            "   enum E { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Ns::E::") << (QStringList()
            << QLatin1String("E")
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

    QTest::newRow("anon_enum_inside_namespace") << _(
            "namespace Ns\n"
            "{\n"
            "   enum { val1, val2, val3 };\n"
            "   @\n"
            "};\n"
            "@\n"
        ) << _("Ns::v") << (QStringList()
            << QLatin1String("val1")
            << QLatin1String("val2")
            << QLatin1String("val3"));

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
            "    connect(timer, SIGNAL(@\n"
            "}\n"
        ) << _() << (QStringList()
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
            "   double val = @\n"
            "}\n"
        ) << _("d.constBegin()->") << (QStringList());
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
            "}\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;

    QTest::newRow("typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S *SType;\n"
            "SType *p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList())
        << false;

    QTest::newRow("typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S SType;\n"
            "SType *p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S* SPtr;\n"
            "SPtr p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_type_no_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
            "}\n"
        ) << _("p.") << (QStringList())
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
            "}\n"
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
            "}\n"
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
            "}\n"
        ) << _("p->") << (QStringList()
            << QLatin1String("S")
            << QLatin1String("m"))
        << false;
}
