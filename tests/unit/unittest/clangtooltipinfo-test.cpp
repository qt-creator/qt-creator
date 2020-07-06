/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "googletest.h"
#include "rundocumentparse-utility.h"

#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangsupport_global.h>
#include <clangtooltipinfocollector.h>
#include <clangtranslationunit.h>
#include <fixitcontainer.h>
#include <sourcelocationcontainer.h>
#include <sourcerangecontainer.h>
#include <unsavedfiles.h>

#include <utils/qtcassert.h>

#include <clang-c/Index.h>

using ::ClangBackEnd::SourceLocationContainer;
using ::ClangBackEnd::Document;
using ::ClangBackEnd::UnsavedFiles;
using ::ClangBackEnd::ToolTipInfo;
using ::ClangBackEnd::SourceRangeContainer;

namespace {

#define CHECK_MEMBER(actual, expected, memberName) \
    if (actual.memberName != expected.memberName) { \
        *result_listener << #memberName " is " + PrintToString(actual.memberName) \
                         << " and not " + PrintToString(expected.memberName); \
        return false; \
    }

MATCHER_P(IsToolTip, expected, std::string(negation ? "isn't" : "is") +  PrintToString(expected))
{
    CHECK_MEMBER(arg, expected, text);
    CHECK_MEMBER(arg, expected, briefComment);

    CHECK_MEMBER(arg, expected, qdocIdCandidates);
    CHECK_MEMBER(arg, expected, qdocMark);
    CHECK_MEMBER(arg, expected, qdocCategory);

    CHECK_MEMBER(arg, expected, sizeInBytes);

    return true;
}

MATCHER_P(IsQdocToolTip, expected, std::string(negation ? "isn't" : "is") +  PrintToString(expected))
{
    CHECK_MEMBER(arg, expected, qdocIdCandidates);
    CHECK_MEMBER(arg, expected, qdocMark);
    CHECK_MEMBER(arg, expected, qdocCategory);

    return true;
}

#undef CHECK_MEMBER

struct Data {
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR "/tooltipinfo.cpp"),
                      {Utf8StringLiteral("-std=c++14")},
                      {},
                      documents};
    UnitTest::RunDocumentParse _1{document};
};

class ToolTipInfo : public ::testing::Test
{
protected:
    ::ToolTipInfo tooltip(uint line, uint column)
    {
        return d->document.translationUnit().tooltip(d->unsavedFiles,
                                                     Utf8StringLiteral("UTF-8"),
                                                     line,
                                                     column);
    }

    static void SetUpTestCase();
    static void TearDownTestCase();

private:
    static std::unique_ptr<Data> d;
};

TEST_F(ToolTipInfo, LocalVariableInt)
{
    const ::ToolTipInfo actual = tooltip(3, 5);

    ASSERT_THAT(actual, IsToolTip(::ToolTipInfo(Utf8StringLiteral("int"))));
}

TEST_F(ToolTipInfo, LocalVariableConstInt)
{
    ASSERT_THAT(tooltip(211, 19), IsToolTip(::ToolTipInfo(Utf8StringLiteral("const int"))));
}

TEST_F(ToolTipInfo, FileScopeVariableConstInt)
{
    ASSERT_THAT(tooltip(206, 11), IsToolTip(::ToolTipInfo(Utf8StringLiteral("const int"))));
}

TEST_F(ToolTipInfo, LocalVariablePointerToConstInt)
{
    const ::ToolTipInfo actual = tooltip(4, 5);

    ASSERT_THAT(actual, IsToolTip(::ToolTipInfo(Utf8StringLiteral("const int *"))));
}

TEST_F(ToolTipInfo, LocalParameterVariableConstRefCustomType)
{
    ::ToolTipInfo expected(Utf8StringLiteral("const Foo &"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Foo")};
    expected.qdocMark = Utf8StringLiteral("Foo");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(12, 12);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, LocalNonParameterVariableConstRefCustomType)
{
    ::ToolTipInfo expected(Utf8StringLiteral("const Foo"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Foo")};
    expected.qdocMark = Utf8StringLiteral("Foo");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(14, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, MemberVariable)
{
    const ::ToolTipInfo actual = tooltip(12, 16);

    ASSERT_THAT(actual, IsToolTip(::ToolTipInfo(Utf8StringLiteral("int"))));
}

TEST_F(ToolTipInfo, MemberFunctionCall_QualifiedName)
{
    const ::ToolTipInfo actual = tooltip(21, 9);

    ASSERT_THAT(actual.text, Utf8StringLiteral("int Bar::mem()"));
}

// ChangeLog: Show extra specifiers. For functions e.g.: virtual, inline, explicit, const, volatile
TEST_F(ToolTipInfo, MemberFunctionCall_ExtraSpecifiers)
{
    const ::ToolTipInfo actual = tooltip(22, 9);

    ASSERT_THAT(actual.text, Utf8StringLiteral("virtual int Bar::virtualConstMem() const"));
}

TEST_F(ToolTipInfo, MemberFunctionCall_qdocIdCandidates)
{
    const ::ToolTipInfo actual = tooltip(21, 9);

    ASSERT_THAT(actual.qdocIdCandidates, ElementsAre(Utf8StringLiteral("Bar::mem"),
                                                     Utf8StringLiteral("mem")));
}

TEST_F(ToolTipInfo, MemberFunctionCall_qdocMark_FIXLIBCLANG_CHECKED)
{
    const ::ToolTipInfo actual = tooltip(21, 9);

    ASSERT_THAT(actual.qdocMark, Utf8StringLiteral("mem()"));
}

// TODO: Check what is really needed for qdoc before implementing this one.
TEST_F(ToolTipInfo, DISABLED_MemberFunctionCall_qdocMark_extraSpecifiers)
{
    const ::ToolTipInfo actual = tooltip(22, 9);

    ASSERT_THAT(actual.qdocMark, Utf8StringLiteral("virtualConstMem() const"));
}

TEST_F(ToolTipInfo, MemberFunctionCall_qdocCategory)
{
    const ::ToolTipInfo actual = tooltip(21, 9);

    ASSERT_THAT(actual.qdocCategory, ::ToolTipInfo::Function);
}

// TODO: Show the template parameter type, too: "template<typename T>...)"
TEST_F(ToolTipInfo, TemplateFunctionCall)
{
    const ::ToolTipInfo actual = tooltip(30, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("template<> void t<Foo>(int foo)"));
}

TEST_F(ToolTipInfo, TemplateFunctionCall_qdocIdCandidates)
{
    const ::ToolTipInfo actual = tooltip(30, 5);

    ASSERT_THAT(actual.qdocIdCandidates, ElementsAre(Utf8StringLiteral("t")));
}

TEST_F(ToolTipInfo, TemplateFunctionCall_qdocMark_FIXLIBCLANG_CHECKED)
{
    const ::ToolTipInfo actual = tooltip(30, 5);

    ASSERT_THAT(actual.qdocMark, Utf8StringLiteral("t(int)"));
}

TEST_F(ToolTipInfo, TemplateFunctionCall_qdocCategory)
{
    const ::ToolTipInfo actual = tooltip(30, 5);

    ASSERT_THAT(actual.qdocCategory, ::ToolTipInfo::Function);
}

TEST_F(ToolTipInfo, BriefComment)
{
    const ::ToolTipInfo actual = tooltip(41, 5);

    ASSERT_THAT(actual.briefComment, Utf8StringLiteral("This is a crazy function."));
}

TEST_F(ToolTipInfo, Enum)
{
    ::ToolTipInfo expected(Utf8StringLiteral("EnumType"));
    expected.qdocIdCandidates = {Utf8StringLiteral("EnumType")};
    expected.qdocMark = Utf8StringLiteral("EnumType");
    expected.qdocCategory = ::ToolTipInfo::Enum;

    const ::ToolTipInfo actual = tooltip(49, 12);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, Enumerator)
{
    ::ToolTipInfo expected(Utf8StringLiteral("6"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Custom")};
    expected.qdocMark = Utf8StringLiteral("EnumType");
    expected.qdocCategory = ::ToolTipInfo::Enum;

    const ::ToolTipInfo actual = tooltip(49, 22);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, TemplateTypeFromParameter)
{
    ::ToolTipInfo expected(Utf8StringLiteral("const Baz<int> &"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Baz")};
    expected.qdocMark = Utf8StringLiteral("Baz");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(55, 25);

    ASSERT_THAT(actual, IsQdocToolTip(expected));
}

TEST_F(ToolTipInfo, TemplateTypeFromNonParameter)
{
    ::ToolTipInfo expected(Utf8StringLiteral("Baz<int>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Baz")};
    expected.qdocMark = Utf8StringLiteral("Baz");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(56, 19);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, IncludeDirective)
{
    ::ToolTipInfo expected(
        QDir::toNativeSeparators(Utf8StringLiteral(TESTDATA_DIR "/tooltipinfo.h")));
    expected.qdocIdCandidates = {Utf8StringLiteral("tooltipinfo.h")};
    expected.qdocMark = Utf8StringLiteral("tooltipinfo.h");
    expected.qdocCategory = ::ToolTipInfo::Brief;

    const ::ToolTipInfo actual = tooltip(59, 11);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, MacroUse_WithMacroFromSameFile)
{
    const ::ToolTipInfo actual = tooltip(66, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("#define MACRO_FROM_MAINFILE(x) x + 3"));
}

TEST_F(ToolTipInfo, MacroUse_WithMacroFromHeader)
{
    const ::ToolTipInfo actual = tooltip(67, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("#define MACRO_FROM_HEADER(x) x + \\\n    x + \\\n    x"));
}

TEST_F(ToolTipInfo, MacroUse_qdoc)
{
    ::ToolTipInfo expected;
    expected.qdocIdCandidates = {Utf8StringLiteral("MACRO_FROM_MAINFILE")};
    expected.qdocMark = Utf8StringLiteral("MACRO_FROM_MAINFILE");
    expected.qdocCategory = ::ToolTipInfo::Macro;

    const ::ToolTipInfo actual = tooltip(66, 5);

    ASSERT_THAT(actual, IsQdocToolTip(expected));
}

TEST_F(ToolTipInfo, TypeNameIntroducedByUsingDirectiveIsQualified)
{
    ::ToolTipInfo expected(Utf8StringLiteral("N::Muu"));
    expected.qdocIdCandidates = {Utf8StringLiteral("N::Muu"), Utf8StringLiteral("Muu")};
    expected.qdocMark = Utf8StringLiteral("Muu");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(77, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, TypeNameIntroducedByUsingDirectiveOfAliasIsResolvedAndQualified)
{
    ::ToolTipInfo expected(Utf8StringLiteral("N::Muu"));
    expected.qdocIdCandidates = {Utf8StringLiteral("N::Muu"), Utf8StringLiteral("Muu")};
    expected.qdocMark = Utf8StringLiteral("Muu");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(82, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, TypeNameIntroducedByUsingDeclarationIsQualified)
{
    ::ToolTipInfo expected(Utf8StringLiteral("N::Muu"));
    expected.qdocIdCandidates = {Utf8StringLiteral("N::Muu"), Utf8StringLiteral("Muu")};
    expected.qdocMark = Utf8StringLiteral("Muu");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(87, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, SizeForClassDefinition)
{
    const ::ToolTipInfo actual = tooltip(92, 8);

    ASSERT_THAT(actual.sizeInBytes, Utf8StringLiteral("2"));
}

TEST_F(ToolTipInfo, SizeForMemberField)
{
    const ::ToolTipInfo actual = tooltip(95, 10);

    ASSERT_THAT(actual.sizeInBytes, Utf8StringLiteral("1"));
}

TEST_F(ToolTipInfo, SizeForEnum)
{
    const ::ToolTipInfo actual = tooltip(97, 12);

    ASSERT_THAT(actual.sizeInBytes, Utf8StringLiteral("4"));
}

TEST_F(ToolTipInfo, SizeForUnion)
{
    const ::ToolTipInfo actual = tooltip(98, 7);

    ASSERT_THAT(actual.sizeInBytes, Utf8StringLiteral("1"));
}

TEST_F(ToolTipInfo, constexprValue)
{
    // CLANG-UPGRADE-CHECK: Adapt the values below
    ASSERT_THAT(tooltip(204, 12).value.toInt(), 4);
    ASSERT_THAT(tooltip(204, 27).value.toInt(), 4); // 3 in clang 11
    ASSERT_THAT(tooltip(204, 30).value.toInt(), 4);
    ASSERT_THAT(tooltip(204, 32).value.toInt(), 4); // 1 in clang 11
}

TEST_F(ToolTipInfo, Namespace)
{
    ::ToolTipInfo expected(Utf8StringLiteral("X"));
    expected.qdocIdCandidates = {Utf8StringLiteral("X")};
    expected.qdocMark = Utf8StringLiteral("X");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(106, 11);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, NamespaceQualified)
{
    ::ToolTipInfo expected(Utf8StringLiteral("X::Y"));
    expected.qdocIdCandidates = {Utf8StringLiteral("X::Y"), Utf8StringLiteral("Y")};
    expected.qdocMark = Utf8StringLiteral("Y");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(107, 11);

    ASSERT_THAT(actual, IsToolTip(expected));
}

// TODO: Show unresolved and resolved name, for F1 try both.
TEST_F(ToolTipInfo, TypeName_ResolveTypeDef)
{
    ::ToolTipInfo expected(Utf8StringLiteral("Ptr<Nuu>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("PtrFromTypeDef")};
    expected.qdocMark = Utf8StringLiteral("PtrFromTypeDef");
    expected.qdocCategory = ::ToolTipInfo::Typedef;

    const ::ToolTipInfo actual = tooltip(122, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

// TODO: Show unresolved and resolved name, for F1 try both.
TEST_F(ToolTipInfo, TypeName_ResolveAlias)
{
    ::ToolTipInfo expected(Utf8StringLiteral("Ptr<Nuu>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("PtrFromTypeAlias")};
    expected.qdocMark = Utf8StringLiteral("PtrFromTypeAlias");
    expected.qdocCategory = ::ToolTipInfo::Typedef;

    const ::ToolTipInfo actual = tooltip(123, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

// The referenced cursor is a CXCursor_TypeAliasTemplateDecl, its type is invalid
// and so probably clang_getTypedefDeclUnderlyingType() does not return anything useful.
// TODO: Fix the cursor's type or add new API in libclang for querying the template type alias.
TEST_F(ToolTipInfo, DISABLED_TypeName_ResolveTemplateTypeAlias)
{
    const ::ToolTipInfo actual = tooltip(124, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("Ptr<Nuu>"));
}

TEST_F(ToolTipInfo, TypeName_ResolveTemplateTypeAlias_qdoc)
{
    ::ToolTipInfo expected;
    expected.qdocIdCandidates = {Utf8StringLiteral("PtrFromTemplateTypeAlias")};
    expected.qdocMark = Utf8StringLiteral("PtrFromTemplateTypeAlias");
    expected.qdocCategory = ::ToolTipInfo::Typedef;

    const ::ToolTipInfo actual = tooltip(124, 5);

    ASSERT_THAT(actual, IsQdocToolTip(expected));
}

TEST_F(ToolTipInfo, TemplateClassReference)
{
    ::ToolTipInfo expected(Utf8StringLiteral("Zii<T>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Zii")};
    expected.qdocMark = Utf8StringLiteral("Zii");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(134, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, TemplateClassQualified)
{
    ::ToolTipInfo expected(Utf8StringLiteral("U::Yii<T>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("U::Yii"), Utf8StringLiteral("Yii")};
    expected.qdocMark = Utf8StringLiteral("Yii");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(135, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, ResolveNamespaceAliasForType)
{
    ::ToolTipInfo expected(Utf8StringLiteral("A::X"));
    expected.qdocIdCandidates = {Utf8StringLiteral("A::X"), Utf8StringLiteral("X")};
    expected.qdocMark = Utf8StringLiteral("X");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(144, 8);

    ASSERT_THAT(actual, IsToolTip(expected));
}

// TODO: Show unresolved and resolved name, for F1 try both.
TEST_F(ToolTipInfo, ResolveNamespaceAlias)
{
    ::ToolTipInfo expected(Utf8StringLiteral("A"));
    expected.qdocIdCandidates = {Utf8StringLiteral("B")};
    expected.qdocMark = Utf8StringLiteral("B");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(144, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, QualificationForTemplateClassInClassInNamespace)
{
    ::ToolTipInfo expected(Utf8StringLiteral("N::Outer::Inner<int>"));
    expected.qdocIdCandidates = {Utf8StringLiteral("N::Outer::Inner"),
                                  Utf8StringLiteral("Outer::Inner"),
                                  Utf8StringLiteral("Inner")};
    expected.qdocMark = Utf8StringLiteral("Inner");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;
    expected.sizeInBytes = Utf8StringLiteral("1");

    const ::ToolTipInfo actual = tooltip(153, 16);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, Function)
{
    ::ToolTipInfo expected(Utf8StringLiteral("void f()"));
    expected.qdocIdCandidates = {Utf8StringLiteral("f")};
    expected.qdocMark = Utf8StringLiteral("f()");
    expected.qdocCategory = ::ToolTipInfo::Function;

    const ::ToolTipInfo actual = tooltip(165, 5);

    ASSERT_THAT(actual, IsToolTip(expected));
}

TEST_F(ToolTipInfo, Function_QualifiedName)
{
    const ::ToolTipInfo actual = tooltip(166, 8);

    ASSERT_THAT(actual.text, Utf8StringLiteral("void R::f()"));
}

TEST_F(ToolTipInfo, Function_qdocIdCandidatesAreQualified)
{
    const ::ToolTipInfo actual = tooltip(166, 8);

    ASSERT_THAT(actual.qdocIdCandidates, ElementsAre(Utf8StringLiteral("R::f"),
                                                     Utf8StringLiteral("f")));
}

TEST_F(ToolTipInfo, Function_HasParameterName)
{
    const ::ToolTipInfo actual = tooltip(167, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("void f(int param)"));
}

// TODO: Implement with CXPrintingPolicy
TEST_F(ToolTipInfo, DISABLED_Function_HasDefaultArgument)
{
    const ::ToolTipInfo actual = tooltip(168, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("void z(int = 1)"));
}

TEST_F(ToolTipInfo, Function_qdocMarkHasNoParameterName)
{
    const ::ToolTipInfo actual = tooltip(167, 5);

    ASSERT_THAT(actual.qdocMark, Utf8StringLiteral("f(int)"));
}

TEST_F(ToolTipInfo, Function_qdocMarkHasNoDefaultArgument)
{
    const ::ToolTipInfo actual = tooltip(168, 5);

    ASSERT_THAT(actual.qdocMark, Utf8StringLiteral("z(int)"));
}

TEST_F(ToolTipInfo, AutoTypeBuiltin)
{
    const ::ToolTipInfo actual = tooltip(176, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("int"));
}

TEST_F(ToolTipInfo, PointerToPointerToClass)
{
    ::ToolTipInfo expected(Utf8StringLiteral("Nuu **"));
    expected.qdocIdCandidates = {Utf8StringLiteral("Nuu")};
    expected.qdocMark = Utf8StringLiteral("Nuu");
    expected.qdocCategory = ::ToolTipInfo::ClassOrNamespace;

    const ::ToolTipInfo actual = tooltip(200, 12);

    ASSERT_THAT(actual, IsToolTip(expected));
}

// TODO: Test for qdoc entries, too.
TEST_F(ToolTipInfo, AutoTypeEnum)
{
    const ::ToolTipInfo actual = tooltip(177, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("EnumType"));
}

// TODO: Test for qdoc entries, too.
TEST_F(ToolTipInfo, AutoTypeClassType)
{
    const ::ToolTipInfo actual = tooltip(178, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("Bar"));
}

// TODO: Test for qdoc entries, too.
// TODO: Deduced template arguments work, too?!
TEST_F(ToolTipInfo, AutoTypeClassTemplateType)
{
    const ::ToolTipInfo actual = tooltip(179, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("Zii<int>"));
}

TEST_F(ToolTipInfo, Function_DefaultConstructor)
{
    const ::ToolTipInfo actual = tooltip(193, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("inline constexpr Con::Con() noexcept"));
}

TEST_F(ToolTipInfo, Function_ExplicitDefaultConstructor)
{
    const ::ToolTipInfo actual = tooltip(194, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("ExplicitCon::ExplicitCon() noexcept = default"));
}

TEST_F(ToolTipInfo, Function_CustomConstructor)
{
    const ::ToolTipInfo actual = tooltip(195, 5);

    ASSERT_THAT(actual.text, Utf8StringLiteral("ExplicitCon::ExplicitCon(int m)"));
}

// Overloads are problematic for the help system since the data base has not
// enough information about them. At least for constructors we can improve
// the situation a bit - provide a help system query that:
//   1) will not lead to the replacement of the constructor signature as
//      clang sees it with the wrong overload documentation
//      (signature + main help sentence). That's the qdocCategory=Unknown
//      part.
//   2) finds the documentation for the class instead of the overload,
//      so F1 will go to the class documentation.
TEST_F(ToolTipInfo, Function_ConstructorQDoc)
{
    ::ToolTipInfo expected;
    expected.qdocIdCandidates = {Utf8StringLiteral("Con")};
    expected.qdocMark = Utf8StringLiteral("Con");
    expected.qdocCategory = ::ToolTipInfo::Unknown;

    const ::ToolTipInfo actual = tooltip(193, 5);

    ASSERT_THAT(actual, IsQdocToolTip(expected));
}

std::unique_ptr<Data> ToolTipInfo::d;

void ToolTipInfo::SetUpTestCase()
{
    d.reset(new Data);
}

void ToolTipInfo::TearDownTestCase()
{
    d.reset();
}

} // anonymous namespace
