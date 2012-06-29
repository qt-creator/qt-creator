/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
#include <name_demangler.h>

#include <QObject>
#include <QTest>

using namespace Debugger::Internal;

class NameDemanglerAutoTest : public QObject
{
    Q_OBJECT
private slots:
    void testUnmangledName();
    void testCorrectlyMangledNames();
    void testIncorrectlyMangledNames();

private:
    void testCorrectlyMangledName(const QString &mangledName,
                                  const QString &demangledName);
    void testIncorrectlyMangledName(const QString &mangledName);
    NameDemangler demangler;
};

void NameDemanglerAutoTest::testUnmangledName()
{
    QVERIFY(demangler.demangle("f") && demangler.demangledName() == "f");
}

void NameDemanglerAutoTest::testCorrectlyMangledNames()
{
    testCorrectlyMangledName("_Z1fv", "f()");
    testCorrectlyMangledName("_Z1fi", "f(int)");
    testCorrectlyMangledName("_Z3foo3bar", "foo(bar)");
    testCorrectlyMangledName("_Zrm1XS_", "operator%(X, X)");
    testCorrectlyMangledName("_ZplR1XS0_", "operator+(X &, X &)");
    testCorrectlyMangledName("_ZlsRK1XS1_", "operator<<(X const &, X const &)");
    testCorrectlyMangledName("_ZN3FooIA4_iE3barE", "Foo<int[4]>::bar");
    testCorrectlyMangledName("_Z1fIiEvi", "void f<int>(int)");
    testCorrectlyMangledName("_Z5firstI3DuoEvS0_", "void first<Duo>(Duo)");
    testCorrectlyMangledName("_Z5firstI3DuoEvT_", "void first<Duo>(Duo)");
    testCorrectlyMangledName("_Z3fooIiPFidEiEvv",
                             "void foo<int, int (*)(double), int>()");
    testCorrectlyMangledName("_ZN1N1fE", "N::f");
    testCorrectlyMangledName("_ZN6System5Sound4beepEv",
                             "System::Sound::beep()");
    testCorrectlyMangledName("_ZN5Arena5levelE", "Arena::level");
    testCorrectlyMangledName("_ZN5StackIiiE5levelE", "Stack<int, int>::level");
    testCorrectlyMangledName("_Z1fI1XEvPVN1AIT_E1TE",
                             "void f<X>(A<X>::T volatile *)");
    testCorrectlyMangledName("_ZngILi42EEvN1AIXplT_Li2EEE1TE",
                             "void operator-<42>(A<42 + 2>::T)");
    testCorrectlyMangledName("_Z4makeI7FactoryiET_IT0_Ev",
                             "Factory<int> make<Factory, int>()");
    testCorrectlyMangledName("_Z3foo5Hello5WorldS0_S_",
                             "foo(Hello, World, World, Hello)");
    testCorrectlyMangledName("_Z3fooPM2ABi", "foo(int AB::**)");
    testCorrectlyMangledName("_ZlsRSoRKSs",
        "operator<<(std::basic_ostream<char, std::char_traits<char> > &, "
        "std::basic_string<char, std::char_traits<char>, "
        "std::allocator<char> > const &)");
    testCorrectlyMangledName("_ZTI7a_class", "typeid(a_class)");
    testCorrectlyMangledName("_ZZN1A3fooEiE1B", "A::foo(int)::B");
    testCorrectlyMangledName("_ZZ3foovEN1C1DE", "foo()::C::D");
    testCorrectlyMangledName("_ZZZ3foovEN1C3barEvEN1E3bazEv",
                             "foo()::C::bar()::E::baz()");
    testCorrectlyMangledName("_ZZN1N1fEiE1p", "N::f(int)::p");
    testCorrectlyMangledName("_ZZN1N1fEiEs", "N::f(int)::\"string literal\"");
    testCorrectlyMangledName("_Z3fooc", "foo(char)");
    testCorrectlyMangledName("_Z2CBIL_Z3foocEE", "CB<foo(char)>");
    testCorrectlyMangledName("_Z2CBIL_Z7IsEmptyEE", "CB<IsEmpty>");
    testCorrectlyMangledName("_ZN1N1TIiiE2mfES0_IddE",
                             "N::T<int, int>::mf(N::T<double, double>)");
    testCorrectlyMangledName("_ZSt5state", "std::state");
    testCorrectlyMangledName("_ZNSt3_In4wardE", "std::_In::ward");
    testCorrectlyMangledName("_Z41__static_initialization_and_destruction_0ii",
        "__static_initialization_and_destruction_0(int, int)");
    testCorrectlyMangledName("_ZN20NameDemanglerPrivate3eoiE",
                             "NameDemanglerPrivate::eoi");
    testCorrectlyMangledName(
        "_ZZN20NameDemanglerPrivate15parseIdentifierEiE8__func__",
        "NameDemanglerPrivate::parseIdentifier(int)::__func__");
    testCorrectlyMangledName("_ZN4QSetI5QCharED1Ev", "QSet<QChar>::~QSet()");
    testCorrectlyMangledName("_Zne5QCharS_", "operator!=(QChar, QChar)");
    testCorrectlyMangledName("_ZN20NameDemanglerPrivate17parseFunctionTypeEv",
                             "NameDemanglerPrivate::parseFunctionType()");
    testCorrectlyMangledName(
        "_ZNK20NameDemanglerPrivate16ArrayNewOperator8makeExprERK11QStringList",
        "NameDemanglerPrivate::ArrayNewOperator::makeExpr(QStringList const &) const");
    testCorrectlyMangledName("_ZN13QLatin1StringC1EPKc",
                             "QLatin1String::QLatin1String(char const *)");
    testCorrectlyMangledName(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEE12internalCopyIS2_EEvRKNS0_IT_EE",
        "void QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::internalCopy<NameDemanglerPrivate::Operator>(QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator> const &)");
    testCorrectlyMangledName(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEE11internalSetEPNS_20ExternalRefCountDataEPS2_",
        "QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::internalSet(QtSharedPointer::ExternalRefCountData *, NameDemanglerPrivate::Operator *)");
    testCorrectlyMangledName("_ZN20NameDemanglerPrivate17parseUnscopedNameEv",
                             "NameDemanglerPrivate::parseUnscopedName()");
    testCorrectlyMangledName("_ZNK7QString3argExiiRK5QChar",
        "QString::arg(long long, int, int, QChar const &) const");
    testCorrectlyMangledName(
        "_ZN20NameDemanglerPrivate8OperatorC2ERK7QStringS3_",
        "NameDemanglerPrivate::Operator::Operator(QString const &, QString const &)");
    testCorrectlyMangledName(
        "_ZN15QtSharedPointer16ExternalRefCountIN20NameDemanglerPrivate8OperatorEEC2EN2Qt14InitializationE",
        "QtSharedPointer::ExternalRefCount<NameDemanglerPrivate::Operator>::ExternalRefCount(Qt::Initialization)");
    testCorrectlyMangledName("_ZN7QString5clearEv", "QString::clear()");
    testCorrectlyMangledName("_ZNK5QListI7QStringE2atEi",
                             "QList<QString>::at(int) const");
    testCorrectlyMangledName(
        "_ZNK7QString10startsWithERKS_N2Qt15CaseSensitivityE",
        "QString::startsWith(QString const &, Qt::CaseSensitivity) const");
    testCorrectlyMangledName("_ZNK4QSetI5QCharE8constEndEv",
                             "QSet<QChar>::constEnd() const");
    testCorrectlyMangledName("_Z11qt_assert_xPKcS0_S0_i",
        "qt_assert_x(char const *, char const *, char const *, int)");
    testCorrectlyMangledName("_ZN9QHashData8willGrowEv",
                             "QHashData::willGrow()");
    testCorrectlyMangledName(
        "_ZNK5QHashI5QChar15QHashDummyValueE14const_iteratorneERKS3_",
        "QHash<QChar, QHashDummyValue>::const_iterator::operator!=(QHash<QChar, QHashDummyValue>::const_iterator const &) const");
    testCorrectlyMangledName("_ZNK13NameDemangler11errorStringEv",
                             "NameDemangler::errorString() const");
    testCorrectlyMangledName("_ZN7QString7replaceERK7QRegExpRKS_",
        "QString::replace(QRegExp const &, QString const &)");
    testCorrectlyMangledName("_ZN7QString4freeEPNS_4DataE",
                             "QString::free(QString::Data *)");
    testCorrectlyMangledName(
        "_ZTSN20NameDemanglerPrivate19ArrayAccessOperatorE",
        "typeid(NameDemanglerPrivate::ArrayAccessOperator).name()");
    testCorrectlyMangledName("_ZN3ns11fERKPFPKiS1_RKhE",
        "ns1::f(int const * (* const &)(int const *, unsigned char const &))");
    testCorrectlyMangledName("_Z9test_funcMN3ns11cImEEKFPKvPiRlmE",
                             "test_func(void const * (ns1::c<unsigned long>::*)(int *, long &, unsigned long) const)");
    testCorrectlyMangledName("_ZN3ns11fEPKPFPKiS1_RKhE",
        "ns1::f(int const * (* const *)(int const *, unsigned char const &))");
    testCorrectlyMangledName("_ZNK1CcviEv", "C::operator int() const");
    testCorrectlyMangledName("_ZN1CppEv", "C::operator++()");
    testCorrectlyMangledName("_ZN1CmmEv", "C::operator--()");
    testCorrectlyMangledName("_ZN1CppEi", "C::operator++(int)");
    testCorrectlyMangledName("_ZN1CmmEi", "C::operator--(int)");
    testCorrectlyMangledName("_ZNK1CcvT_IPKcEEv", "C::operator char const *<char const *>() const");
}

void NameDemanglerAutoTest::testIncorrectlyMangledNames()
{
}

void NameDemanglerAutoTest::testCorrectlyMangledName(
    const QString &mangledName, const QString &demangledName)
{
    demangler.demangle(mangledName);
    QCOMPARE(demangler.demangledName(), demangledName);
}

void NameDemanglerAutoTest::testIncorrectlyMangledName(
    const QString &mangledName)
{
    QVERIFY(!demangler.demangle(mangledName));
}

QTEST_MAIN(NameDemanglerAutoTest)

#include "tst_namedemangler.moc"
