// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcompletion_test.h"

#include "cppcompletionassist.h"
#include "cppdoxygen.h"
#include "cppmodelmanager.h"
#include "cpptoolstestcase.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/changeset.h>
#include <utils/textutils.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QTest>
#include <QTextDocument>

/*!
    Tests for code completion.
 */

using namespace Core;
using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

using _ = QByteArray;

class CompletionTestCase : public CppEditor::Tests::TestCase
{
public:
    CompletionTestCase(const QByteArray &sourceText, const QByteArray &textToInsert = QByteArray(),
                       bool isObjC = false)
    {
        QVERIFY(succeededSoFar());
        m_succeededSoFar = false;

        m_source = sourceText;
        m_position = m_source.indexOf('@');
        QVERIFY(m_position != -1);
        m_source[m_position] = ' ';

        // Write source to file
        m_temporaryDir.reset(new CppEditor::Tests::TemporaryDir());
        QVERIFY(m_temporaryDir->isValid());
        const QByteArray fileExt = isObjC ? "mm" : "h";
        const FilePath filePath = m_temporaryDir->createFile("file." + fileExt, m_source);
        QVERIFY(!filePath.isEmpty());

        // Open in editor
        m_editor = EditorManager::openEditor(filePath);
        QVERIFY(m_editor);

        closeEditorAtEndOfTestCase(m_editor);
        m_editorWidget = TextEditorWidget::fromEditor(m_editor);
        QVERIFY(m_editorWidget);

        m_textDocument = m_editorWidget->document();

        // Get Document
        const Document::Ptr document = waitForFileInGlobalSnapshot(filePath);
        QVERIFY(document);
        if (!document->diagnosticMessages().isEmpty()) {
            for (const Document::DiagnosticMessage &m : document->diagnosticMessages())
                qDebug().noquote() << m.text();
        }
        QVERIFY(document->diagnosticMessages().isEmpty());

        m_snapshot.insert(document);

        if (!textToInsert.isEmpty())
            insertText(textToInsert);

        m_succeededSoFar = true;
    }

    QStringList getCompletions(bool *replaceAccessOperator = nullptr) const
    {
        QStringList completions;
        LanguageFeatures languageFeatures = LanguageFeatures::defaultFeatures();
        languageFeatures.objCEnabled = false;
        QTextCursor textCursor = m_editorWidget->textCursor();
        textCursor.setPosition(m_position);
        m_editorWidget->setTextCursor(textCursor);
        std::unique_ptr<CppCompletionAssistInterface> ai(
            new CppCompletionAssistInterface(m_editorWidget->textDocument()->filePath(),
                                             m_editorWidget,
                                             ExplicitlyInvoked,
                                             m_snapshot,
                                             ProjectExplorer::HeaderPaths(),
                                             languageFeatures));
        ai->prepareForAsyncUse();
        ai->recreateTextDocument();
        InternalCppCompletionAssistProcessor processor;
        processor.setupAssistInterface(std::move(ai));

        const QScopedPointer<IAssistProposal> proposal(processor.performAsync());
        if (!proposal)
            return completions;
        ProposalModelPtr model = proposal->model();
        if (!model)
            return completions;
        CppAssistProposalModelPtr listmodel = model.staticCast<CppAssistProposalModel>();
        if (!listmodel)
            return completions;

        const int pos = proposal->basePosition();
        const int length = m_position - pos;
        const QString prefix = Utils::Text::textAt(m_textDocument, pos, length);
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
        change.apply(m_textDocument);
        m_position += text.length();
    }

    bool waitForSyntaxHighlighting()
    {
        TextEditor::BaseTextEditor *cppEditor = qobject_cast<TextEditor::BaseTextEditor *>(m_editor);
        if (!cppEditor)
            return false;
        if (cppEditor->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate())
            return true;
        return ::CppEditor::Tests::waitForSignalOrTimeout(
            cppEditor->textDocument()->syntaxHighlighter(), &SyntaxHighlighter::finished, 5000);
    }

private:
    QByteArray m_source;
    int m_position = -1;
    Snapshot m_snapshot;
    QScopedPointer<CppEditor::Tests::TemporaryDir> m_temporaryDir;
    TextEditorWidget *m_editorWidget = nullptr;
    QTextDocument *m_textDocument = nullptr;
    IEditor *m_editor = nullptr;
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

bool isDoxygenTagCompletion(const QStringList &list)
{
    for (int i = 1; i < T_DOXY_LAST_TAG; ++i) {
        const QString doxygenTag = QString::fromLatin1(doxygenTagSpell(i));
        if (!list.contains(doxygenTag))
            return false;
    }

    return true;
}

} // anonymous namespace

void CompletionTest::testCompletionBasic1()
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

void CompletionTest::testCompletionPrefixFirstQTCREATORBUG_8737()
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

void CompletionTest::testCompletionPrefixFirstQTCREATORBUG_9236()
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

void CompletionTest::testCompletionTemplateFunction()
{
    QFETCH(QByteArray, code);
    QFETCH(QStringList, expectedCompletions);

    CompletionTestCase test(code);
    QVERIFY(test.succeededSoFar());

    QStringList actualCompletions = test.getCompletions();
    QString errorPattern(QLatin1String("Completion not found: %1"));
    for (const QString &completion : std::as_const(expectedCompletions))  {
        QByteArray errorMessage = errorPattern.arg(completion).toUtf8();
        QVERIFY2(actualCompletions.contains(completion), errorMessage.data());
    }
}

void CompletionTest::testCompletionTemplateFunction_data()
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

void CompletionTest::testCompletion()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, expectedCompletions);

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());

    QStringList actualCompletions = test.getCompletions();
    actualCompletions.sort();
    expectedCompletions.sort();

    QEXPECT_FAIL("template_as_base: explicit typedef from base", "QTCREATORBUG-14218", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function_cxx11", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_function_in_struct_in_function_anon", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_class_accessed_in_member_func_cxx11", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("enum_in_class_accessed_in_member_func_inline_cxx11", "QTCREATORBUG-13757", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_typedef", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_double_indirection", "QTCREATORBUG-14141", Abort);
    QEXPECT_FAIL("pointer_indirect_specialization_double_indirection_with_base", "QTCREATORBUG-14141", Abort);
    QCOMPARE(actualCompletions, expectedCompletions);
}

void CompletionTest::testGlobalCompletion_data()
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
    const QStringList replacements = QStringList({"// text", "// text.", "/// text", "/// text."});
    for (const QString &replacement : replacements) {
        QByteArray code = codeTemplate;
        code.replace("<REPLACEMENT>", replacement.toUtf8());
        const QByteArray tag = _("completion after comment: ") + replacement.toUtf8();
        QTest::newRow(tag) << code << QByteArray() << QStringList("myGlobal");
    }
}

void CompletionTest::testGlobalCompletion()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, requiredCompletionItems);

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());
    const QStringList completions = test.getCompletions();
    QVERIFY(isProbablyGlobalCompletion(completions));
    QVERIFY(Utils::toSet(completions).contains(Utils::toSet(requiredCompletionItems)));
}

void CompletionTest::testDoxygenTagCompletion_data()
{
    QTest::addColumn<QByteArray>("code");

    QTest::newRow("C++ comment")
         << _("/// @");

    QTest::newRow("C comment single line")
         << _("/*! @ */");

    QTest::newRow("C comment multi line")
         << _("/*! text\n"
              " *  @\n"
              " */\n");
}

void CompletionTest::testDoxygenTagCompletion()
{
    QFETCH(QByteArray, code);

    const QByteArray prefix = "\\";

    CompletionTestCase test(code, prefix);
    QVERIFY(test.succeededSoFar());
    QVERIFY(test.waitForSyntaxHighlighting());
    const QStringList completions = test.getCompletions();
    QVERIFY(isDoxygenTagCompletion(completions));
}

static void enumTestCase(const QByteArray &tag, const QByteArray &source,
                         const QByteArray &prefix = QByteArray())
{
    QByteArray fullSource = source;
    fullSource.replace('$', "enum E { value1, value2, value3 };");
    QTest::newRow(tag) << fullSource << (prefix + "value")
            << QStringList({"value1", "value2", "value3"});

    QTest::newRow(QByteArray{tag + "_cxx11"}) << fullSource << QByteArray{prefix + "E::"}
            << QStringList({"E", "value1", "value2", "value3"});

    fullSource.replace("enum E ", "enum ");
    QTest::newRow(QByteArray{tag + "_anon"}) << fullSource << QByteArray{prefix + "value"}
            << QStringList({"value1", "value2", "value3"});
}

void CompletionTest::testCompletion_data()
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
        ) << _("Foo::Bar::") << QStringList("Bar");

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
        ) << _("((Derived *)b)->") << QStringList({"Derived", "Base", "i_derived", "i_base"});

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
        ) << _("(static_cast<Derived *>(b))->") << QStringList({"Derived", "Base", "i_derived",
                                                                "i_base"});

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
        ) << _("Foo::") << QStringList({"Foo", "Type", "foo", "m"});

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
        ) << _("l.at(0).") << QStringList({"Tupple", "a", "b"});

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
        ) << _("l.t.") << QStringList({"Tupple", "a", "b"});

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
        ) << _("l.u.") << QStringList({"Tupple", "a", "b"});

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
        ) << _("l.u.") << QStringList({"Tupple", "a", "b"});

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
        ) << _("container.get().") << QStringList({"Item", "i"});

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
        ) << _("p->") << QStringList({"Test", "i"});

    QTest::newRow("type_of_pointer_is_typedef") << _(
            "typedef struct Foo\n"
            "{\n"
            "    int foo;\n"
            "} Foo;\n"
            "Foo *bar;\n"
            "@\n"
        ) << _("bar->") << QStringList({"Foo", "foo"});

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
        ) << _("templateChar.") << QStringList({"Template", "templateChar_i"});

    QTest::newRow("template_as_base: base as template directly") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "\n"
            "void func() {\n"
            "    Other<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << QStringList({"Data", "dataMember", "Other", "otherMember"});

    QTest::newRow("template_as_base: base as class template") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << QStringList({"Data", "dataMember", "Other", "otherMember", "More",
                                     "moreMember"});

    QTest::newRow("template_as_base: base as globally qualified class template") << _(
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public ::Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "}"
        ) << _("c.") << QStringList({"Data", "dataMember", "Other", "otherMember", "More",
                                     "moreMember"});

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
        ) << _("c.") << QStringList({"Data", "dataMember", "Other", "otherMember", "More",
                                     "moreMember"});

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
        ) << _("c.") << QStringList({"Data", "dataMember", "Final", "finalMember"});

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
        ) << _("c.") << QStringList({"Data", "dataMember", "Final", "finalMember"});

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
        ) << _("c.") << QStringList({"Data", "dataMember", "Final", "finalMember", "Other",
                                     "otherMember"});

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
        ) << _("d.f.") << QStringList({"Data", "dataMember"});

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
        ) << _("c.") << QStringList({"int_global", "int_final", "Final", "Global"});

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
        ) << _("c.") << QStringList({"int_global", "int_final", "Final", "Global"});

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
        ) << _("c.") << QStringList({"int_global", "int_final", "Final", "Global"});

    QTest::newRow("base_class_has_name_the_same_as_derived: base class is derived class") << _(
            "struct A : A\n"
            "{\n"
            "   int int_a;\n"
            "};\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << QStringList({"int_a", "A"});

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
        ) << _("c.") << QStringList({"int_a", "A"});

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
        ) << _("c.") << QStringList({"int_a", "A"});

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
        ) << _("c.") << QStringList({"int_ns1_a", "int_ns2_a", "A"});

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
        ) << _("c.") << QStringList({"int_enclosing_a", "int_ns2_a", "A"});

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
        ) << _("c.") << QStringList({"int_enclosing_base_a", "int_enclosing_derived_a", "A"});

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
        ) << _("c.") << QStringList({"int_a", "A"});

    QTest::newRow("cyclic_inheritance: direct cyclic inheritance") << _(
            "struct B;\n"
            "struct A : B { int _a; };\n"
            "struct B : A { int _b; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << QStringList({"A", "_a", "B", "_b"});

    QTest::newRow("cyclic_inheritance: indirect cyclic inheritance") << _(
            "struct C;\n"
            "struct A : C { int _a; };\n"
            "struct B : A { int _b; };\n"
            "struct C : B { int _c; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << QStringList({"A", "_a", "B", "_b", "C", "_c"});

    QTest::newRow("cyclic_inheritance: indirect cyclic inheritance") << _(
            "struct B;\n"
            "struct A : B { int _a; };\n"
            "struct C { int _c; };\n"
            "struct B : C, A { int _b; };\n"
            "\n"
            "A c;\n"
            "@\n"
        ) << _("c.") << QStringList({"A", "_a", "B", "_b", "C", "_c"});

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
        ) << _("c.") << QStringList({"D", "_d_t", "_d_s", "C", "_c_t"});

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
        ) << _("c.") << QStringList({"D", "_d_t", "_d_s", "C", "_c_t", "B", "_b_t"});

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
        ) << _("c.") << QStringList({"Class", "ClassRecurse", "class_t", "class_recurse_s",
                                     "class_recurse_t"});

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
        ) << _("c.") << QStringList({"Nested", "int_nested"});

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
        ) << _("c.") << QStringList({"Nested", "int_nested"});

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
        ) << _("enclosing.nested.nested_t.") << QStringList({"Foo", "foo_i"});

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
        ) << _("enclosing.nested.nestedNested.nestedNested_t.") << QStringList({"Foo", "foo_i"});

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
        ) << _("templateWithDefaultTypeOfArgument.t.") << QStringList({"Foo", "bar"});

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
        ) << _("templateWithDefaultTypeOfArgument.s.t.") << QStringList({"Foo", "bar"});

    QTest::newRow("typedef_of_pointer") << _(
            "struct Foo { int bar; };\n"
            "typedef Foo *FooPtr;\n"
            "void main()\n"
            "{\n"
            "   FooPtr ptr;\n"
            "   @\n"
            "}"
        ) << _("ptr->") << QStringList({"Foo", "bar"});

    QTest::newRow("typedef_of_pointer_inside_function") << _(
            "struct Foo { int bar; };\n"
            "void f()\n"
            "{\n"
            "   typedef Foo *FooPtr;\n"
            "   FooPtr ptr;\n"
            "   @\n"
            "}"
        ) << _("ptr->") << QStringList({"Foo", "bar"});

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
        ) << _("ptr->") << QStringList({"Foo", "bar"});

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
        ) << _("template2.templateTypedef.") << QStringList({"Foo", "bar", "Template1"});

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
        ) << _("templ.") << QStringList({"Template", "pointer"});

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
        ) << _("s.p->") << QStringList({"Foo", "bar"});

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
        ) << _("p->") << QStringList({"Foo", "bar"});

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
        ) << _("NS::") << QStringList({"Foo1", "Foo2"});

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
        ) << _("b.") << QStringList({"c", "B"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("s.") << QStringList({"S", "i", "c"});

    QTest::newRow("instantiate_template_function") << _(
            "template <typename T>\n"
            "T* templateFunction() { return 0; }\n"
            "struct A { int a; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("templateFunction<A>()->") << QStringList({ "A", "a" });

    QTest::newRow("nested_named_class_declaration_inside_function") << _(
            "int foo()\n"
            "{\n"
            "    struct Nested\n"
            "    {\n"
            "        int i;\n"
            "    } n;\n"
            "    @;\n"
            "}\n"
        ) << _("n.") << QStringList({"Nested", "i"});

    QTest::newRow("nested_class_inside_member_function") << _(
          "struct User { void use(); };\n"
          "void User::use()\n"
          "{\n"
          "   struct Foo { int bar; };\n"
          "   Foo foo;\n"
          "   @\n"
          "}\n"
    ) << _("foo.") << QStringList({"Foo", "bar"});

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
    ) << _("myfoo->") << QStringList({"Foo", "Ptr", "bar"});

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
        ) << _("member") << QStringList({"memberNestedAnonymousClass",
                                         "memberOfEnclosingStruct"});

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
        ) << _("member") << QStringList({"memberOfNestedAnonymousClass",
                                         "memberOfNestedOfNestedAnonymousClass",
                                         "memberOfEnclosingStruct"});

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
        ) << _("nestedOfNestedAnonymousClass.") << QStringList("memberOfNestedOfNestedAnonymousClass");

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
        ) << _("foo") << QStringList({"foo1", "foo2"});

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
        ) << _("this->") << QStringList({"A", "B", "Templ", "f"});

    QTest::newRow("recursive_auto_declarations1_QTCREATORBUG9503") << _(
            "void f()\n"
            "{\n"
            "    auto object2 = object1;\n"
            "    auto object1 = object2;\n"
            "    @;\n"
            "}\n"
        ) << _("object1.") << QStringList();

    QTest::newRow("recursive_auto_declarations2_QTCREATORBUG9503") << _(
            "void f()\n"
            "{\n"
            "    auto object3 = object1;\n"
            "    auto object2 = object3;\n"
            "    auto object1 = object2;\n"
            "    @;\n"
            "}\n"
        ) << _("object1.") << QStringList();

    QTest::newRow("recursive_typedefs_declarations1") << _(
            "void f()\n"
            "{\n"
            "    typedef A B;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << QStringList();

    QTest::newRow("recursive_typedefs_declarations2") << _(
            "void f()\n"
            "{\n"
            "    typedef A C;\n"
            "    typedef C B;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << QStringList();

    QTest::newRow("recursive_using_declarations1") << _(
            "void f()\n"
            "{\n"
            "    using B = A;\n"
            "    using A = B;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << QStringList();

    QTest::newRow("recursive_using_declarations2") << _(
            "void f()\n"
            "{\n"
            "    using C = A;\n"
            "    using B = C;\n"
            "    using A = B;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << QStringList();

    QTest::newRow("recursive_using_typedef_declarations") << _(
            "void f()\n"
            "{\n"
            "    using B = A;\n"
            "    typedef B A;\n"
            "    A a;\n"
            "    @;\n"
            "}\n"
        ) << _("a.") << QStringList();

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
        ) << _("cast_retty<T1, T2>::ret_type.") << QStringList();

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
        ) << _("recursive<T1>::ret_type.foo") << QStringList();

    QTest::newRow("class_declaration_inside_function_or_block_QTCREATORBUG3620: "
                  "class definition inside function") << _(
            "void foo()\n"
            "{\n"
            "    struct C { int m; };\n"
            "    C c;\n"
            "    @\n"
            "}\n"
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m2"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("c.") << QStringList({"C", "m"});

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
        ) << _("C::") << QStringList({"C", "staticFun2", "m2"});

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
        ) << _("e.") << QStringList({"Enclosing", "i"});

    QTest::newRow("lambdaCalls_1") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[](){ return new S; } ()->") << QStringList({"S", "bar"});

    QTest::newRow("lambdaCalls_2") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[] { return new S; } ()->") << QStringList({"S", "bar"});

    QTest::newRow("lambdaCalls_3") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() ->S* { return new S; } ()->") << QStringList({"S", "bar"});

    QTest::newRow("lambdaCalls_4") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() throw() { return new S; } ()->") << QStringList({"S", "bar"});

    QTest::newRow("lambdaCalls_5") << _(
            "struct S { int bar; };\n"
            "void foo()\n"
            "{\n"
            "   @\n"
            "}\n"
        ) << _("[]() throw()->S* { return new S; } ()->") << QStringList({"S", "bar"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("lt.ot.") << QStringList({"OtherType", "otherTypeMember"});

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
        ) << _("templ.get()->") << QStringList({"B", "b"});

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
        ) << _("templ.t.") << QStringList({"B", "b"});

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
        ) << _("list.at(0).") << QStringList({"Foo", "bar"});

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
        ) << _("list.at(0).") << QStringList({"Foo", "bar"});

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
        ) << _("list.at(0).") << QStringList({"Foo", "bar"});

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
         << _("connect(myObject, SIGNAL(") << QStringList({"baseSignal1()", "baseSignal2(int)",
                                                           "hiddenSignal()", "derivedSignal1()",
                                                           "derivedSignal2(int)"});

    QTest::newRow("SLOT(")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, SIGNAL(baseSignal1()), myObject, SLOT(")
         << QStringList({"baseSlot1()", "baseSlot2(int)", "derivedSlot1()", "derivedSlot2(int)"});

    QTest::newRow("Qt5 signals: complete class after & at 2nd connect arg")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &") << QStringList("N::Derived");

    QTest::newRow("Qt5 signals: complete class after & at 4th connect arg")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &MyObject::timeout, myObject, &") << QStringList("N::Derived");

    QTest::newRow("Qt5 signals: complete signals")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived::")
         << QStringList({"baseSignal1", "baseSignal2", "hiddenSignal", "derivedSignal1",
                         "derivedSignal2"});

    QTest::newRow("Qt5 slots")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived, myObject, &N::Derived::")
         << QStringList({"baseFunction", "baseSignal1", "baseSignal2", "baseSlot1", "baseSlot2",
                         "derivedFunction", "derivedSignal1", "derivedSignal2", "derivedSlot1",
                         "derivedSlot2", "hiddenFunction", "hiddenSignal"});

    QTest::newRow("Qt5 signals: fallback to scope completion")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::") << QStringList({"Base", "Derived"});

    QTest::newRow("Qt5 slots: fallback to scope completion")
         << commonSignalSlotCompletionTestCode
         << _("connect(myObject, &N::Derived, myObject, &N::") << QStringList({"Base", "Derived"});

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
        ) << _("connect(timer, SIGNAL(") << QStringList("timeout()");

    QTest::newRow("member_of_class_accessed_by_using_QTCREATORBUG9037_1") << _(
            "namespace NS { struct S { int member; void fun(); }; }\n"
            "using NS::S;\n"
            "void S::fun()\n"
            "{\n"
            "    @\n"
            "}\n"
        ) << _("mem") << QStringList("member");

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
        ) << _("mem") << QStringList("member");

    QTest::newRow("no_binding_block_as_instantiationOrigin_QTCREATORBUG-11424") << _(
            "template <typename T>\n"
            "class QList\n"
            "{\n"
            "public:\n"
            "   inline const_iterator constBegin() const;\n"
            "};\n"
            "\n"
            "typedef struct { double value; } V;\n"
            "\n"
            "double getValue(const QList<V>& d) const {\n"
            "   typedef QList<V>::ConstIterator Iter;\n"
            "   @\n"
            "}\n"
        ) << _("double val = d.constBegin()->") << QStringList();

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
        ) << _("n2.n1.t.") << QStringList({"foo", "Foo"});

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
        ) << _("tempFoo.nested.t.") << QStringList({"foo", "Foo"});

    QTest::newRow("lambda_parameter") << _(
            "auto func = [](int arg1) { return @; };\n"
        ) << _("ar") << QStringList("arg1");

    QTest::newRow("default_arguments_for_class_templates_and_base_class_QTCREATORBUG-12605") << _(
            "struct Foo { int foo; };\n"
            "template <typename T = Foo>\n"
            "struct Derived : T {};\n"
            "void fun() {\n"
            "   Derived<> derived;\n"
            "   @\n"
            "}\n"
        ) << _("derived.") << QStringList({"Derived", "foo", "Foo"});

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
        ) << _("derived.t.") << QStringList({"foo", "Foo"});

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
        ) << _("s.t->") << QStringList({"foo", "Foo"});

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
        ) << _("s.t->") << QStringList({"foo", "Foo"});

    QTest::newRow("typedef_of_pointer_of_array_QTCREATORBUG-12703") << _(
            "struct Foo { int foo; };\n"
            "typedef Foo *FooArr[10];\n"
            "void fun() {\n"
            "   FooArr arr;\n"
            "   @\n"
            "}\n"
        ) << _("arr[0]->") << QStringList({"foo", "Foo"});

    QTest::newRow("template_specialization_with_array1") << _(
            "template <typename T>\n"
            "struct S {};\n"
            "template <typename T>\n"
            "struct S<T[]> { int foo; };\n"
            "void fun() {\n"
            "   S<int[]> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << QStringList({"foo", "S"});

    QTest::newRow("template_specialization_with_array2") << _(
            "template <typename T>\n"
            "struct S {};\n"
            "template <typename T, size_t N>\n"
            "struct S<T[N]> { int foo; };\n"
            "void fun() {\n"
            "   S<int[3]> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.") << QStringList({"foo", "S"});

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
    ) << _("s.") << QStringList("S");

    QTest::newRow("auto_declaration_in_if_condition") << _(
            "struct Foo { int bar; };\n"
            "void fun() {\n"
            "   if (auto s = new Foo) {\n"
            "      @\n"
            "   }\n"
            "}\n"
    ) << _("s->") << QStringList({"Foo", "bar"});

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
    ) << _("(*list.begin()).") << QStringList({"Foo", "bar"});

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
    ) << _("list.begin()->") << QStringList({"Foo", "bar"});

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
    ) << _("(*a).") << QStringList({"Foo", "bar"});

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
    ) << _("a->") << QStringList({"Foo", "bar"});

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
    ) << _("a.") << QStringList({"operator ->", "t", "iterator"});

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
    ) << _("t.p->") << QStringList({"Foo", "bar"});

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
    ) << _("t.p->") << QStringList({"Foo", "bar"});

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
    ) << _("t.p->") << QStringList({"Foo", "bar"});

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
    ) << _("t.p->") << QStringList({"Foo", "bar"});

    QTest::newRow("fix_code_completion_for_unique_ptr_operator_arrow") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct unique_ptr\n"
            "{\n"
            "   typedef FOO pointer;\n"
            "   pointer operator->();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::unique_ptr<Foo> ptr;\n"
            "   @\n"
            "}\n"
    ) << _("ptr->") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_unique_ptr_method_get") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct unique_ptr\n"
            "{\n"
            "   typedef FOO pointer;\n"
            "   pointer get();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::unique_ptr<Foo> ptr;\n"
            "   @\n"
            "}\n"
    ) << _("ptr.get()->") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_vector_method_at") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct vector\n"
            "{\n"
            "   typedef FOO reference;\n"
            "   reference at(size_t i);\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::vector<Foo> v;\n"
            "   @\n"
            "}\n"
    ) << _("v.at(0).") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_vector_operator_square_brackets") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct vector\n"
            "{\n"
            "   typedef FOO reference;\n"
            "   reference operator[](size_t i);\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::vector<Foo> v;\n"
            "   @\n"
            "}\n"
    ) << _("v[0].") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_list_method_front") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct list\n"
            "{\n"
            "   typedef FOO reference;\n"
            "   reference front();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::list<Foo> l;\n"
            "   @\n"
            "}\n"
    ) << _("l.front().") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_queue_method_front") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct queue\n"
            "{\n"
            "   typedef FOO reference;\n"
            "   reference front();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::queue<Foo> l;\n"
            "   @\n"
            "}\n"
    ) << _("l.front().") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_set_method_begin") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct set\n"
            "{\n"
            "   typedef FOO iterator;\n"
            "   iterator begin();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::set<Foo> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.begin()->") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_multiset_method_begin") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct multiset\n"
            "{\n"
            "   typedef FOO iterator;\n"
            "   iterator begin();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::multiset<Foo> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.begin()->") << QStringList({"Foo", "bar"});
    QTest::newRow("fix_code_completion_for_std_unordered_set_method_begin") << _(
            "namespace std {\n"
            "template<typename _Tp>\n"
            "struct unordered_set\n"
            "{\n"
            "   typedef FOO iterator;\n"
            "   iterator begin();\n"
            "};\n"
            "}\n"
            "\n"
            "struct Foo { int bar; };\n"
            "\n"
            "void func()\n"
            "{\n"
            "   std::unordered_set<Foo> s;\n"
            "   @\n"
            "}\n"
    ) << _("s.begin()->") << QStringList({"Foo", "bar"});
}

void CompletionTest::testCompletionMemberAccessOperator()
{
    QFETCH(QByteArray, code);
    QFETCH(QByteArray, prefix);
    QFETCH(QStringList, expectedCompletions);
    QFETCH(bool, isObjC);
    QFETCH(bool, expectedReplaceAccessOperator);

    CompletionTestCase test(code, prefix, isObjC);
    QVERIFY(test.succeededSoFar());

    bool replaceAccessOperator = false;
    QStringList completions = test.getCompletions(&replaceAccessOperator);

    completions.sort();
    expectedCompletions.sort();

    QCOMPARE(completions, expectedCompletions);
    QCOMPARE(replaceAccessOperator, expectedReplaceAccessOperator);
}

void CompletionTest::testCompletionMemberAccessOperator_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QByteArray>("prefix");
    QTest::addColumn<QStringList>("expectedCompletions");
    QTest::addColumn<bool>("isObjC");
    QTest::addColumn<bool>("expectedReplaceAccessOperator");

    QTest::newRow("member_access_operator") << _(
            "struct S { void t(); };\n"
            "void f() { S *s;\n"
            "@\n"
            "}\n"
        ) << _("s.") << QStringList({ "S", "t" })
        << false
        << true;

    QTest::newRow("objc_not_replacing") << _(
            "typedef struct objc_object Bar;"
            "class Foo {\n"
            "  Bar *bar;\n"
            "  void func() { @ }"
            "};\n"
        ) << _("bar.") << QStringList()
        << true
        << false;

    QTest::newRow("typedef_of_type_and_decl_of_type_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S SType;\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << false;

    QTest::newRow("typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S *SType;\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << QStringList()
        << false
        << false;

    QTest::newRow("typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S SType;\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << true;

    QTest::newRow("typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef S* SPtr;\n"
            "SPtr p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_of_pointer_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << true;

    QTest::newRow("predecl_typedef_of_type_and_decl_type_no_replace_access_operator") << _(
            "typedef struct S SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_pointer_no_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType *p;\n"
            "@\n"
        ) << _("p.") << QStringList()
        << false
        << false;

    QTest::newRow("predecl_typedef_of_pointer_and_decl_of_type_replace_access_operator") << _(
            "typedef struct S *SType;\n"
            "struct S { int m; };\n"
            "SType p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << true;

    QTest::newRow("typedef_of_pointer_of_type_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef struct S SType;\n"
            "typedef struct SType *STypePtr;\n"
            "STypePtr p;\n"
            "@\n"
        ) << _("p.") << QStringList({"S", "m"})
        << false
        << true;

    QTest::newRow("typedef_of_pointer_of_type_no_replace_access_operator") << _(
            "struct S { int m; };\n"
            "typedef struct S SType;\n"
            "typedef struct SType *STypePtr;\n"
            "STypePtr p;\n"
            "@\n"
        ) << _("p->") << QStringList({"S", "m"})
        << false
        << false;
    QTest::newRow("dot to arrow: template + reference + double typedef")
        << _("template <typename T> struct C {\n"
             "    using ref = T &;\n"
             "    ref operator[](int i);\n"
             "};\n"
             "struct S { int m; };\n"
             "template<typename T> using CS = C<T>;\n"
             "CS<S *> v;\n"
             "@\n")
        << _("v[0].") << QStringList({"S", "m"}) << false << true;
}

} // namespace CppEditor::Internal
