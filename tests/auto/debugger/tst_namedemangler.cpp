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
#include <namedemangler/namedemangler.h>
#include <namedemangler/parsetreenodes.h>

#include <QObject>
#include <QTest>

#define TEST_CORRECTLY_MANGLED_NAME(mangled, expectedDemangled) \
    do { \
        QVERIFY2(demangler.demangle(QLatin1String(mangled)), qPrintable(demangler.errorString())); \
        QCOMPARE(demangler.demangledName(), QLatin1String(expectedDemangled)); \
    } while (0)

const char *toString(char c) { return (QByteArray("'") + c + "'").constData(); }

using namespace Debugger::Internal;

class NameDemanglerAutoTest : public QObject
{
    Q_OBJECT
private slots:
    void testUnmangledName();
    void testDisjunctFirstSets();
    void testCorrectlyMangledNames();
    void testIncorrectlyMangledNames();

private:
    void testIncorrectlyMangledName(const QString &mangledName);
    NameDemangler demangler;
};

void NameDemanglerAutoTest::testUnmangledName()
{
    QVERIFY(demangler.demangle(QLatin1String("f"))
            && demangler.demangledName() == QLatin1String("f"));
}

void NameDemanglerAutoTest::testCorrectlyMangledNames()
{
    TEST_CORRECTLY_MANGLED_NAME("_Z1fv", "f()");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fi", "f(int)");
    TEST_CORRECTLY_MANGLED_NAME("_Z3foo3bar", "foo(bar)");
    TEST_CORRECTLY_MANGLED_NAME("_Zrm1XS_", "operator%(X, X)");
    TEST_CORRECTLY_MANGLED_NAME("_ZplR1XS0_", "operator+(X &, X &)");
    TEST_CORRECTLY_MANGLED_NAME("_ZlsRK1XS1_", "operator<<(X const &, X const &)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN3FooIA4_iE3barE", "Foo<int[4]>::bar");
    TEST_CORRECTLY_MANGLED_NAME("_Z5firstI3DuoEvS0_", "void first<Duo>(Duo)");
    TEST_CORRECTLY_MANGLED_NAME("_Z5firstI3DuoEvT_", "void first<Duo>(Duo)");
    TEST_CORRECTLY_MANGLED_NAME("_Z3fooIiPFidEiEvv",
                             "void foo<int, int (*)(double), int>()");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1N1fE", "N::f");
    TEST_CORRECTLY_MANGLED_NAME("_ZN6System5Sound4beepEv",
                             "System::Sound::beep()");
    TEST_CORRECTLY_MANGLED_NAME("_ZN5Arena5levelE", "Arena::level");
    TEST_CORRECTLY_MANGLED_NAME("_ZN5StackIiiE5levelE", "Stack<int, int>::level");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fI1XEvPVN1AIT_E1TE",
                             "void f<X>(A<X>::T volatile *)");
    TEST_CORRECTLY_MANGLED_NAME("_ZngILi42EEvN1AIXplT_Li2EEE1TE",
                             "void operator-<42>(A<42 + 2>::T)");
    TEST_CORRECTLY_MANGLED_NAME("_Z4makeI7FactoryiET_IT0_Ev",
                             "Factory<int> make<Factory, int>()");
    TEST_CORRECTLY_MANGLED_NAME("_Z3foo5Hello5WorldS0_S_",
                             "foo(Hello, World, World, Hello)");
    TEST_CORRECTLY_MANGLED_NAME("_Z3fooPM2ABi", "foo(int AB::**)");
    TEST_CORRECTLY_MANGLED_NAME("_ZlsRSoRKSs",
        "operator<<(std::basic_ostream<char, std::char_traits<char> > &, "
        "std::basic_string<char, std::char_traits<char>, "
        "std::allocator<char> > const &)");
    TEST_CORRECTLY_MANGLED_NAME("_ZTI7a_class", "typeid(a_class)");
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1A3fooEiE1B", "A::foo(int)::B");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ3foovEN1C1DE", "foo()::C::D");
    TEST_CORRECTLY_MANGLED_NAME("_ZZZ3foovEN1C3barEvEN1E3bazEv",
                             "foo()::C::bar()::E::baz()");
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1N1fEiE1p", "N::f(int)::p");
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1N1fEiEs", "N::f(int)::{string literal}");
    TEST_CORRECTLY_MANGLED_NAME("_Z41__static_initialization_and_destruction_0ii",
        "__static_initialization_and_destruction_0(int, int)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN20NameDemanglerPrivate3eoiE",
                             "NameDemanglerPrivate::eoi");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZZN20NameDemanglerPrivate15parseIdentifierEiE8__func__",
        "NameDemanglerPrivate::parseIdentifier(int)::__func__");
    TEST_CORRECTLY_MANGLED_NAME("_ZN4QSetI5QCharED1Ev", "QSet<QChar>::~QSet()");
    TEST_CORRECTLY_MANGLED_NAME("_Zne5QCharS_", "operator!=(QChar, QChar)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN20NameDemanglerPrivate17parseFunctionTypeEv",
                             "NameDemanglerPrivate::parseFunctionType()");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZNK20NameDemanglerPrivate16ArrayNewOperator8makeExprERK11QStringList",
        "NameDemanglerPrivate::ArrayNewOperator::makeExpr(QStringList const &) const");
    TEST_CORRECTLY_MANGLED_NAME("_ZN13QLatin1StringC1EPKc",
                             "QLatin1String::QLatin1String(char const *)");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEE12internalCopyIS2_EEvRKNS0_IT_EE",
        "void QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::internalCopy<NameDemanglerPrivate::Operator>(QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator> const &)");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEE11internalSetEPNS_20ExternalRefCountDataEPS2_",
        "QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::internalSet(QtSharedPointer::ExternalRefCountData *, NameDemanglerPrivate::Operator *)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN20NameDemanglerPrivate17parseUnscopedNameEv",
                             "NameDemanglerPrivate::parseUnscopedName()");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK7QString3argExiiRK5QChar",
        "QString::arg(long long, int, int, QChar const &) const");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZN20NameDemanglerPrivate8OperatorC2ERK7QStringS3_",
        "NameDemanglerPrivate::Operator::Operator(QString const &, QString const &)");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEEC2EN2Qt14InitializationE",
        "QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::ExternalRefCount(Qt::Initialization)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN7QString5clearEv", "QString::clear()");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK5QListI7QStringE2atEi",
                             "QList<QString>::at(int) const");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZNK7QString10startsWithERKS_N2Qt15CaseSensitivityE",
        "QString::startsWith(QString const &, Qt::CaseSensitivity) const");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK4QSetI5QCharE8constEndEv",
                             "QSet<QChar>::constEnd() const");
    TEST_CORRECTLY_MANGLED_NAME("_Z11qt_assert_xPKcS0_S0_i",
        "qt_assert_x(char const *, char const *, char const *, int)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN9QHashData8willGrowEv",
                             "QHashData::willGrow()");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZNK5QHashI5QChar15QHashDummyValueE14const_iteratorneERKS3_",
        "QHash<QChar, QHashDummyValue>::const_iterator::operator!=(QHash<QChar, QHashDummyValue>::const_iterator const &) const");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK13NameDemangler11errorStringEv",
                             "NameDemangler::errorString() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZN7QString7replaceERK7QRegExpRKS_",
        "QString::replace(QRegExp const &, QString const &)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN7QString4freeEPNS_4DataE",
                             "QString::free(QString::Data *)");
    TEST_CORRECTLY_MANGLED_NAME(
        "_ZTSN20NameDemanglerPrivate19ArrayAccessOperatorE",
        "typeid(NameDemanglerPrivate::ArrayAccessOperator).name()");
    TEST_CORRECTLY_MANGLED_NAME("_ZN3ns11fERKPFPKiS1_RKhE",
        "ns1::f(int const * (* const &)(int const *, unsigned char const &))");
    TEST_CORRECTLY_MANGLED_NAME("_Z9test_funcMN3ns11cImEEKFPKvPiRlmE",
        "test_func(void const * (ns1::c<unsigned long>::*)(int *, long &, unsigned long) const)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN3ns11fEPKPFPKiS1_RKhE",
        "ns1::f(int const * (* const *)(int const *, unsigned char const &))");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK1CcviEv", "C::operator int() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1CppEv", "C::operator++()");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1CmmEv", "C::operator--()");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1CppEi", "C::operator++(int)");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1CmmEi", "C::operator--(int)");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK1CcvT_IPKcEEv", "C::operator char const *<char const *>() const");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_KPFS0_S0_E", "void weirdfunc<int>(int, int (* const)(int))");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_PFS0_DtfL1p_EE", "void weirdfunc<int>(int, int (*)(decltype({param#1})))");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_S0_", "void weirdfunc<int>(int, int)");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_DtfL0p_E", "void weirdfunc<int>(int, decltype({param#1}))");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_S0_S0_", "void weirdfunc<int>(int, int, int)");
    TEST_CORRECTLY_MANGLED_NAME("_Z9weirdfuncIiEvT_S0_DtfL0p0_E", "void weirdfunc<int>(int, int, decltype({param#2}))");
    TEST_CORRECTLY_MANGLED_NAME("_Z8toStringIiESsT_",
            "std::basic_string<char, std::char_traits<char>, std::allocator<char> > toString<int>(int)");

    TEST_CORRECTLY_MANGLED_NAME("_Z4funcIRA5_iEvOT_", "void func<int (&)[5]>(int (&)[5])");
    TEST_CORRECTLY_MANGLED_NAME("_ZSt9make_pairIiRA5_KcESt4pairINSt17__decay_and_stripIT_E6__typeENS4_IT0_E6__typeEEOS5_OS8_",
            "std::pair<std::__decay_and_strip<int>::__type, std::__decay_and_strip<char const (&)[5]>::__type> std::make_pair<int, char const (&)[5]>(int &&, char const (&)[5])");

    // All examples from the ABI spec.
    TEST_CORRECTLY_MANGLED_NAME("_ZN1S1xE", "S::x");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fM1AKFvvE", "f(void (A::*)() const)");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fIiEvT_", "void f<int>(int)");
    TEST_CORRECTLY_MANGLED_NAME("_Z3fooc", "foo(char)");
    TEST_CORRECTLY_MANGLED_NAME("_Z2CBIL_Z3foocEE", "CB<foo(char)>");
    TEST_CORRECTLY_MANGLED_NAME("_Z2CBIL_Z7IsEmptyEE", "CB<IsEmpty>");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1giEN1S1fE_2i", "g(int)::S::f(int)");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1gvEN1SC1Ev", "g()::S::S()");
    TEST_CORRECTLY_MANGLED_NAME("_ZZZ1gvEN1SC1EvEs", "g()::S::S()::{string literal}");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1gvE5str4a", "g()::str4a");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1gvEs_1", "g()::{string literal}");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1gvE5str4b", "g()::str4b");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fPFvvEM1SFvvE", "f(void (*)(), void (S::*)())");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1N1TIiiE2mfES0_IddE", "N::T<int, int>::mf(N::T<double, double>)");
    TEST_CORRECTLY_MANGLED_NAME("_ZSt5state", "std::state");
    TEST_CORRECTLY_MANGLED_NAME("_ZNSt3_In4wardE", "std::_In::ward");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fN1SUt_E", "f(S::{unnamed type#1})");
    TEST_CORRECTLY_MANGLED_NAME("_ZZZ1giEN1S1fE_2iEUt1_", "g(int)::S::f(int)::{unnamed type#3}");
    TEST_CORRECTLY_MANGLED_NAME("_ZZZ1giEN1S1fE_2iENUt1_2fxEv", "g(int)::S::f(int)::{unnamed type#3}::fx()");
    TEST_CORRECTLY_MANGLED_NAME("_Z1AIcfE", "A<char, float>");
    TEST_CORRECTLY_MANGLED_NAME("_Z1fIiEvT_PDtfL0pK_E", "void f<int>(int, decltype({param#1 const}) *)");
    TEST_CORRECTLY_MANGLED_NAME("_Z1AILln42EE", "A<-42L>");
    TEST_CORRECTLY_MANGLED_NAME("_Z2f1I1QEDTpldtfp_1xdtL_Z1qE1xET_", "decltype({param#1}.x + q.x) f1<Q>(Q)");
    TEST_CORRECTLY_MANGLED_NAME("_Z2f2I1QEDTpldtfp_1xsrS0_1xET_", "decltype({param#1}.x + Q::x) f2<Q>(Q)");
    TEST_CORRECTLY_MANGLED_NAME("_Z2f3IiEDTplfp_dtL_Z1dEsr1B1XIT_EE1xES1_", "decltype({param#1} + d.B::X<int>::x) f3<int>(int)");
    TEST_CORRECTLY_MANGLED_NAME("_Z3fooILi2EEvRAplT_Li1E_i", "void foo<2>(int (&)[2 + 1])");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1giENKUlvE_clEv", "g(int)::{lambda()#1}::operator()() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZZ1giENKUlvE0_clEv", "g(int)::{lambda()#2}::operator()() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZNK1SIiE1xMUlvE_clEv", "S<int>::x::{lambda()#1}::operator()() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZN1S4funcEii", "S::func(int, int)");

    // Note: c++filt from binutils 2.22 demangles these wrong (counts default arguments from first instead of from last)
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1S1fEiiEd0_NKUlvE_clEv", "S::f(int, int)::{default arg#1}::{lambda()#1}::operator()() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1S1fEiiEd0_NKUlvE0_clEv", "S::f(int, int)::{default arg#1}::{lambda()#2}::operator()() const");
    TEST_CORRECTLY_MANGLED_NAME("_ZZN1S1fEiiEd_NKUlvE_clEv", "S::f(int, int)::{default arg#2}::{lambda()#1}::operator()() const");

     // Note: gcc 4.6.3 encodes this as "_Z2f4I7OpClassEDTadsrT_miES1_".
    TEST_CORRECTLY_MANGLED_NAME("_Z2f4I7OpClassEDTadsrT_onmiES0_", "decltype(&OpClass::operator-) f4<OpClass>(OpClass)");
}

void NameDemanglerAutoTest::testIncorrectlyMangledNames()
{
}

void NameDemanglerAutoTest::testDisjunctFirstSets()
{
    for (char c = 0x20; c < 0x7e; ++c) {

        // <encoding>
        QVERIFY(!NameNode::mangledRepresentationStartsWith(c)
                || !SpecialNameNode::mangledRepresentationStartsWith(c));

        // <name>
        QVERIFY(!NestedNameNode::mangledRepresentationStartsWith(c)
                || !UnscopedNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!NestedNameNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!NestedNameNode::mangledRepresentationStartsWith(c)
                || !LocalNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!UnscopedNameNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c) || c == 'S');
        QVERIFY(!UnscopedNameNode::mangledRepresentationStartsWith(c)
                || !LocalNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!SubstitutionNode::mangledRepresentationStartsWith(c)
                || !LocalNameNode::mangledRepresentationStartsWith(c));

        // <nested-name>
        QVERIFY(!CvQualifiersNode::mangledRepresentationStartsWith(c)
                || !PrefixNode::mangledRepresentationStartsWith(c) || c == 'r');

        // <prefix>
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateArgsNode::mangledRepresentationStartsWith(c)
                || !UnqualifiedNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !UnqualifiedNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!SubstitutionNode::mangledRepresentationStartsWith(c)
                || !UnqualifiedNameNode::mangledRepresentationStartsWith(c));

        // <template-arg>
        QVERIFY(!TypeNode::mangledRepresentationStartsWith(c)
                || !ExprPrimaryNode::mangledRepresentationStartsWith(c));

        // <expression>
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !FunctionParamNode::mangledRepresentationStartsWith(c));
        QVERIFY2(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !UnresolvedNameNode::mangledRepresentationStartsWith(c)
                 || c == 'd' || c == 'g' || c == 'o' || c == 's', toString(c));
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !ExprPrimaryNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !FunctionParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !ExprPrimaryNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !FunctionParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !UnresolvedNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionParamNode::mangledRepresentationStartsWith(c)
                || !UnresolvedNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionParamNode::mangledRepresentationStartsWith(c)
                || !ExprPrimaryNode::mangledRepresentationStartsWith(c));

        // <expr-primary>
        QVERIFY(!TypeNode::mangledRepresentationStartsWith(c)
                || !MangledNameRule::mangledRepresentationStartsWith(c));

        // <type>
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !FunctionTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY2(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !ClassEnumTypeRule::mangledRepresentationStartsWith(c) || c == 'D', toString(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !ArrayTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !PointerToMemberTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c) || c == 'D');
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !ClassEnumTypeRule::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !ArrayTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !PointerToMemberTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !ArrayTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !PointerToMemberTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c) || c == 'D');
        QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith(c)
                || !PointerToMemberTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
                || !TemplateParamNode::mangledRepresentationStartsWith(c));
        QVERIFY(!PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!PointerToMemberTypeNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!SubstitutionNode::mangledRepresentationStartsWith(c)
                || !CvQualifiersNode::mangledRepresentationStartsWith(c));
        QVERIFY(!SubstitutionNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!CvQualifiersNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));

        // <unqualified-name>
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !CtorDtorNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !SourceNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!OperatorNameNode::mangledRepresentationStartsWith(c)
                || !UnnamedTypeNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!CtorDtorNameNode::mangledRepresentationStartsWith(c)
                || !SourceNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!CtorDtorNameNode::mangledRepresentationStartsWith(c)
                || !UnnamedTypeNameNode::mangledRepresentationStartsWith(c));
        QVERIFY(!SourceNameNode::mangledRepresentationStartsWith(c)
                || !UnnamedTypeNameNode::mangledRepresentationStartsWith(c));

        // <array-type>
        QVERIFY(!NonNegativeNumberNode<10>::mangledRepresentationStartsWith(c)
                || !ExpressionNode::mangledRepresentationStartsWith(c) || std::isdigit(c));

        // <unresolved-type>
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !DeclTypeNode::mangledRepresentationStartsWith(c));
        QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));
        QVERIFY(!DeclTypeNode::mangledRepresentationStartsWith(c)
                || !SubstitutionNode::mangledRepresentationStartsWith(c));

        // <desctructor-name>
        QVERIFY(!UnresolvedTypeRule::mangledRepresentationStartsWith(c)
                || !SimpleIdNode::mangledRepresentationStartsWith(c));
    }

    // <template-args>, <template-arg>
    QVERIFY(!TemplateArgNode::mangledRepresentationStartsWith('E'));

    // <template-arg>
    QVERIFY(!TypeNode::mangledRepresentationStartsWith('X')
            && !TypeNode::mangledRepresentationStartsWith('J')
            /* && !TypeNode::mangledRepresentationStartsWith('s') */);
    QVERIFY(!ExprPrimaryNode::mangledRepresentationStartsWith('X')
             && !ExprPrimaryNode::mangledRepresentationStartsWith('J')
             && !ExprPrimaryNode::mangledRepresentationStartsWith('s'));

    // <expression>
    QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith('c')
            && !TemplateParamNode::mangledRepresentationStartsWith('s')
            && !TemplateParamNode::mangledRepresentationStartsWith('a'));
    QVERIFY(!FunctionParamNode::mangledRepresentationStartsWith('c')
            && !FunctionParamNode::mangledRepresentationStartsWith('c')
            && !FunctionParamNode::mangledRepresentationStartsWith('c'));
    QVERIFY(!ExprPrimaryNode::mangledRepresentationStartsWith('c')
            && !ExprPrimaryNode::mangledRepresentationStartsWith('s')
            && !ExprPrimaryNode::mangledRepresentationStartsWith('a'));
    QVERIFY(!ExpressionNode::mangledRepresentationStartsWith('E'));
    QVERIFY(!ExpressionNode::mangledRepresentationStartsWith('_'));
    QVERIFY(!InitializerNode::mangledRepresentationStartsWith('E'));

    // <type>
    QVERIFY(!BuiltinTypeNode::mangledRepresentationStartsWith('P')
            && !BuiltinTypeNode::mangledRepresentationStartsWith('R')
            && !BuiltinTypeNode::mangledRepresentationStartsWith('O')
            && !BuiltinTypeNode::mangledRepresentationStartsWith('C')
            && !BuiltinTypeNode::mangledRepresentationStartsWith('G')
            && !BuiltinTypeNode::mangledRepresentationStartsWith('U'));
    QVERIFY(!FunctionTypeNode::mangledRepresentationStartsWith('P')
            && !FunctionTypeNode::mangledRepresentationStartsWith('R')
            && !FunctionTypeNode::mangledRepresentationStartsWith('O')
            && !FunctionTypeNode::mangledRepresentationStartsWith('C')
            && !FunctionTypeNode::mangledRepresentationStartsWith('G')
            && !FunctionTypeNode::mangledRepresentationStartsWith('U')
            && !FunctionTypeNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!ClassEnumTypeRule::mangledRepresentationStartsWith('P')
            && !ClassEnumTypeRule::mangledRepresentationStartsWith('R')
            && !ClassEnumTypeRule::mangledRepresentationStartsWith('O')
            && !ClassEnumTypeRule::mangledRepresentationStartsWith('C')
            && !ClassEnumTypeRule::mangledRepresentationStartsWith('G')
            && !ClassEnumTypeRule::mangledRepresentationStartsWith('U')
            /* && !firstSetClassEnumType.contains('D') */);
    QVERIFY(!ArrayTypeNode::mangledRepresentationStartsWith('P')
            && !ArrayTypeNode::mangledRepresentationStartsWith('R')
            && !ArrayTypeNode::mangledRepresentationStartsWith('O')
            && !ArrayTypeNode::mangledRepresentationStartsWith('C')
            && !ArrayTypeNode::mangledRepresentationStartsWith('G')
            && !ArrayTypeNode::mangledRepresentationStartsWith('U')
            && !ArrayTypeNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!PointerToMemberTypeNode::mangledRepresentationStartsWith('P')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('R')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('O')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('C')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('G')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('U')
            && !PointerToMemberTypeNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!TemplateParamNode::mangledRepresentationStartsWith('P')
            && !TemplateParamNode::mangledRepresentationStartsWith('R')
            && !TemplateParamNode::mangledRepresentationStartsWith('O')
            && !TemplateParamNode::mangledRepresentationStartsWith('C')
            && !TemplateParamNode::mangledRepresentationStartsWith('G')
            && !TemplateParamNode::mangledRepresentationStartsWith('U')
            && !TemplateParamNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!SubstitutionNode::mangledRepresentationStartsWith('P')
            && !SubstitutionNode::mangledRepresentationStartsWith('R')
            && !SubstitutionNode::mangledRepresentationStartsWith('O')
            && !SubstitutionNode::mangledRepresentationStartsWith('C')
            && !SubstitutionNode::mangledRepresentationStartsWith('G')
            && !SubstitutionNode::mangledRepresentationStartsWith('U')
            && !SubstitutionNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!CvQualifiersNode::mangledRepresentationStartsWith('P')
            && !CvQualifiersNode::mangledRepresentationStartsWith('R')
            && !CvQualifiersNode::mangledRepresentationStartsWith('O')
            && !CvQualifiersNode::mangledRepresentationStartsWith('C')
            && !CvQualifiersNode::mangledRepresentationStartsWith('G')
            && !CvQualifiersNode::mangledRepresentationStartsWith('U')
            && !CvQualifiersNode::mangledRepresentationStartsWith('D'));
    QVERIFY(!DeclTypeNode::mangledRepresentationStartsWith('P')
            && !DeclTypeNode::mangledRepresentationStartsWith('R')
            && !DeclTypeNode::mangledRepresentationStartsWith('O')
            && !DeclTypeNode::mangledRepresentationStartsWith('C')
            && !DeclTypeNode::mangledRepresentationStartsWith('G')
            && !DeclTypeNode::mangledRepresentationStartsWith('U'));

    // <array-type>
    QVERIFY(!NonNegativeNumberNode<10>::mangledRepresentationStartsWith('_'));
    QVERIFY(!ExpressionNode::mangledRepresentationStartsWith('_'));

    // <substitution>
    QVERIFY(!NonNegativeNumberNode<36>::mangledRepresentationStartsWith('_')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('t')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('a')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('b')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('s')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('i')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('o')
            && !NonNegativeNumberNode<36>::mangledRepresentationStartsWith('d'));

    // <special-name>
    QVERIFY(!CallOffsetRule::mangledRepresentationStartsWith('V')
            && !CallOffsetRule::mangledRepresentationStartsWith('T')
            && !CallOffsetRule::mangledRepresentationStartsWith('I')
            && !CallOffsetRule::mangledRepresentationStartsWith('S')
            && !CallOffsetRule::mangledRepresentationStartsWith('c'));

    // <unscoped-name>
    QVERIFY(!UnqualifiedNameNode::mangledRepresentationStartsWith('S'));

    // <prefix>
    QVERIFY(!TemplateArgsNode::mangledRepresentationStartsWith('M'));
    QVERIFY(!UnqualifiedNameNode::mangledRepresentationStartsWith('M'));

    // <base-unresolved-name>
    QVERIFY(!SimpleIdNode::mangledRepresentationStartsWith('o'));
    QVERIFY(!SimpleIdNode::mangledRepresentationStartsWith('d'));

    // <initializer>
    QVERIFY(!ExpressionNode::mangledRepresentationStartsWith('E'));

    // <unresolved-name-node>
    QVERIFY(!BaseUnresolvedNameNode::mangledRepresentationStartsWith('g'));
    QVERIFY(!BaseUnresolvedNameNode::mangledRepresentationStartsWith('s'));
    QVERIFY(!UnresolvedTypeRule::mangledRepresentationStartsWith('N'));
    QVERIFY(!UnresolvedQualifierLevelRule::mangledRepresentationStartsWith('N'));
    QVERIFY(!UnresolvedQualifierLevelRule::mangledRepresentationStartsWith('E'));
}

void NameDemanglerAutoTest::testIncorrectlyMangledName(
    const QString &mangledName)
{
    QVERIFY(!demangler.demangle(mangledName));
}

QTEST_MAIN(NameDemanglerAutoTest)

#include "tst_namedemangler.moc"
