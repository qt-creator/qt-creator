/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <QtTest>
#include <QObject>
#include <QList>

#include <FullySpecifiedType.h>
#include <Type.h>
#include <CoreTypes.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <Control.h>
#include <Names.h>
#include <Literals.h>
#include <Overview.h>
#include <Scope.h>
#include <TypePrettyPrinter.h>

//TESTED_COMPONENT=src/libs/cplusplus
using namespace CPlusPlus;

class tst_TypePrettyPrinter: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basic();
    void basic_data();
};

Q_DECLARE_METATYPE(CPlusPlus::FullySpecifiedType);

TranslationUnit *unit;

const Identifier *nameId(const QString &name)
{ return new Identifier(name.toLatin1().constData(), name.toLatin1().size()); }

Argument *arg(const QString &name, const FullySpecifiedType &ty)
{
    Argument *a = new Argument(unit, 0, nameId(name));
    a->setType(ty);
    return a;
}

FullySpecifiedType voidTy()
{ return FullySpecifiedType(new VoidType); }

FullySpecifiedType intTy()
{ return FullySpecifiedType(new IntegerType(IntegerType::Int)); }

FullySpecifiedType fnTy(const QString &name, const FullySpecifiedType &ret)
{
    Function *fn = new Function(unit, 0, nameId(name));
    fn->setReturnType(ret);
    return FullySpecifiedType(fn);
}

FullySpecifiedType fnTy(const QString &name, const FullySpecifiedType &ret, const FullySpecifiedType &a0)
{
    Function *fn = new Function(unit, 0, nameId(name));
    fn->setReturnType(ret);
    fn->addMember(arg("a0", a0));
    return FullySpecifiedType(fn);
}

FullySpecifiedType ptr(const FullySpecifiedType &el)
{ return FullySpecifiedType(new PointerType(el)); }

FullySpecifiedType ref(const FullySpecifiedType &el)
{ return FullySpecifiedType(new ReferenceType(el, false)); }

FullySpecifiedType rref(const FullySpecifiedType &el)
{ return FullySpecifiedType(new ReferenceType(el, true)); }

FullySpecifiedType arr(const FullySpecifiedType &el)
{ return FullySpecifiedType(new ArrayType(el, 4)); }

FullySpecifiedType cnst(const FullySpecifiedType &el)
{ FullySpecifiedType r(el); r.setConst(true); return r; }



void addRow(const FullySpecifiedType &f, QString result, QString name = QString())
{
    QTest::newRow(result.toLatin1().constData()) << f << name << result;
}

void tst_TypePrettyPrinter::basic_data()
{
    // seems it now works without a translation unit
//    Control c;
//    TranslationUnit t(&c, 0);
//    unit = 0;

    QTest::addColumn<FullySpecifiedType>("type");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("result");

    addRow(voidTy(), "void");
    addRow(cnst(voidTy()), "const void");
    addRow(ptr(fnTy("foo", voidTy())), "void (*)()");
    addRow(ptr(fnTy("foo", voidTy())), "void (*foo)()", "foo");

    // named types
    addRow(voidTy(), "void foo", "foo");
    addRow(ptr(voidTy()), "void *foo", "foo");
    addRow(cnst(ptr(voidTy())), "void *const foo", "foo");
    addRow(arr(voidTy()), "void foo[]", "foo");
    addRow(ptr(arr(voidTy())), "void (*foo)[]", "foo");

    // pointers
    addRow(ptr(voidTy()), "void *");
    addRow(ptr(ptr(voidTy())), "void **");

    addRow(ptr(cnst(voidTy())), "const void *");
    addRow(cnst(ptr(cnst(voidTy()))), "const void *const");
    addRow(cnst(ptr(voidTy())), "void *const");

    addRow(ptr(ptr(cnst(voidTy()))), "const void **");
    addRow(ptr(cnst(ptr(cnst(voidTy())))), "const void *const*");
    addRow(cnst(ptr(ptr(cnst(voidTy())))), "const void **const");
    addRow(cnst(ptr(cnst(ptr(cnst(voidTy()))))), "const void *const*const");
    addRow(ptr(cnst(ptr(voidTy()))), "void *const*");
    addRow(cnst(ptr(ptr(voidTy()))), "void **const");
    addRow(cnst(ptr(cnst(ptr(voidTy())))), "void *const*const");

    addRow(arr(voidTy()), "void[]");
    addRow(arr(ptr(voidTy())), "void *[]");
    addRow(ptr(arr(voidTy())), "void (*)[]");
    addRow(ptr(arr(arr(voidTy()))), "void (*)[][]");
    addRow(ptr(arr(ptr(voidTy()))), "void *(*)[]");
    addRow(arr(ptr(arr(voidTy()))), "void (*[])[]");

    // references
    addRow(ref(voidTy()), "void &");
    addRow(ref(ref(voidTy())), "void & &");

    addRow(ref(cnst(voidTy())), "const void &");
    addRow(cnst(ref(cnst(voidTy()))), "const void &const");
    addRow(cnst(ref(voidTy())), "void &const");

    addRow(ref(ref(cnst(voidTy()))), "const void & &");
    addRow(ref(cnst(ref(cnst(voidTy())))), "const void &const&");
    addRow(cnst(ref(ref(cnst(voidTy())))), "const void & &const");
    addRow(cnst(ref(cnst(ref(cnst(voidTy()))))), "const void &const&const");
    addRow(ref(cnst(ref(voidTy()))), "void &const&");
    addRow(cnst(ref(ref(voidTy()))), "void & &const");
    addRow(cnst(ref(cnst(ref(voidTy())))), "void &const&const");

    addRow(arr(voidTy()), "void[]");
    addRow(arr(ref(voidTy())), "void &[]");
    addRow(ref(arr(voidTy())), "void (&)[]");
    addRow(ref(arr(arr(voidTy()))), "void (&)[][]");
    addRow(ref(arr(ref(voidTy()))), "void &(&)[]");
    addRow(arr(ref(arr(voidTy()))), "void (&[])[]");

    // rvalue references
    addRow(rref(voidTy()), "void &&");
    addRow(rref(cnst(voidTy())), "const void &&");

    addRow(rref(arr(voidTy())), "void (&&)[]");
    addRow(rref(arr(arr(voidTy()))), "void (&&)[][]");

    // simple functions
    addRow(ptr(fnTy("foo", voidTy(), intTy())), "void (*foo)(int)", "foo");
    addRow(ptr(fnTy("foo", voidTy(), ptr(voidTy()))), "void (*foo)(void *)", "foo");
    addRow(fnTy("foo", voidTy(), intTy()), "void foo(int)", "foo");
    addRow(fnTy("foo", voidTy(), ptr(voidTy())), "void foo(void *)", "foo");

    // functions with ptr or ref returns
    addRow(ptr(fnTy("foo", ptr(voidTy()), intTy())), "void *(*foo)(int)", "foo");
    addRow(ptr(fnTy("foo", ref(voidTy()), ptr(voidTy()))), "void &(*foo)(void *)", "foo");
    addRow(fnTy("foo", ptr(voidTy()), intTy()), "void *foo(int)", "foo");
    addRow(fnTy("foo", ref(voidTy()), ptr(voidTy())), "void &foo(void *)", "foo");
}

void tst_TypePrettyPrinter::basic()
{
    QFETCH(FullySpecifiedType, type);
    QFETCH(QString, name);
    QFETCH(QString, result);

    Overview o;
    o.showReturnTypes = true;
    TypePrettyPrinter pp(&o);
    QCOMPARE(pp(type, name), result);
}

QTEST_APPLESS_MAIN(tst_TypePrettyPrinter)
#include "tst_typeprettyprinter.moc"
