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

#include "functionutils.h"

#include "typehierarchybuilder.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbols.h>
#include <utils/qtcassert.h>

#include <QList>
#include <QPair>

#include <limits>

#include <cplusplus/TypePrettyPrinter.h>

using namespace CPlusPlus;
using namespace CppTools;

enum VirtualType { Virtual, PureVirtual };

static bool isVirtualFunction_helper(const Function *function,
                                     const LookupContext &context,
                                     VirtualType virtualType,
                                     QList<const Function *> *firstVirtuals)
{
    enum { Unknown, False, True } res = Unknown;

    if (firstVirtuals)
        firstVirtuals->clear();

    if (!function)
        return false;

    if (virtualType == PureVirtual)
        res = function->isPureVirtual() ? True : False;

    const Class * const klass = function->enclosingScope()
            ? function->enclosingScope()->asClass() : nullptr;
    if (!klass)
        return false;

    int hierarchyDepthOfFirstVirtuals = -1;
    const auto updateFirstVirtualsList
            = [&hierarchyDepthOfFirstVirtuals, &context, firstVirtuals, klass](Function *candidate) {
        const Class * const candidateClass = candidate->enclosingScope()
                ? candidate->enclosingScope()->asClass() : nullptr;
        if (!candidateClass)
            return;
        QList<QPair<const Class *, int>> classes{{klass, 0}};
        while (!classes.isEmpty()) {
            const auto c = classes.takeFirst();
            if (c.first == candidateClass) {
                QTC_ASSERT(c.second != 0, return);
                if (c.second >= hierarchyDepthOfFirstVirtuals) {
                    if (c.second > hierarchyDepthOfFirstVirtuals) {
                        firstVirtuals->clear();
                        hierarchyDepthOfFirstVirtuals = c.second;
                    }
                    firstVirtuals->append(candidate);
                }
                return;
            }
            for (int i = 0; i < c.first->baseClassCount(); ++i) {
                const BaseClass * const baseClassSpec = c.first->baseClassAt(i);
                const ClassOrNamespace * const base = context.lookupType(baseClassSpec->name(),
                                                                         c.first->enclosingScope());
                const Class *baseClass = nullptr;
                if (base) {
                    baseClass = base->rootClass();

                    // Sometimes, BaseClass::rootClass() is null, and then the class is
                    // among the symbols. No idea why.
                    if (!baseClass) {
                        for (const auto s : base->symbols()) {
                            if (s->asClass() && Matcher::match(s->name(), baseClassSpec->name())) {
                                baseClass = s->asClass();
                                break;
                            }
                        }
                    }
                }
                if (baseClass)
                    classes.append({baseClass, c.second + 1});
            }
        }
    };

    if (function->isVirtual()) {
        if (firstVirtuals) {
            hierarchyDepthOfFirstVirtuals = 0;
            firstVirtuals->append(function);
        }
        if (res == Unknown)
            res = True;
    }

    if (!firstVirtuals && res != Unknown)
        return res == True;

    QList<LookupItem> results = context.lookup(function->name(), function->enclosingScope());
    if (!results.isEmpty()) {
        const bool isDestructor = function->name()->isDestructorNameId();
        foreach (const LookupItem &item, results) {
            if (Symbol *symbol = item.declaration()) {
                if (Function *functionType = symbol->type()->asFunctionType()) {
                    if (functionType->name()->isDestructorNameId() != isDestructor)
                        continue;
                    if (functionType == function) // already tested
                        continue;
                    if (!function->isSignatureEqualTo(functionType))
                        continue;
                    if (functionType->isFinal())
                        return res == True;
                    if (functionType->isVirtual()) {
                        if (!firstVirtuals)
                            return true;
                        if (res == Unknown)
                            res = True;
                        updateFirstVirtualsList(functionType);
                    }
                }
            }
        }
    }

    return res == True;
}

bool FunctionUtils::isVirtualFunction(const Function *function,
                                      const LookupContext &context,
                                      QList<const Function *> *firstVirtuals)
{
    return isVirtualFunction_helper(function, context, Virtual, firstVirtuals);
}

bool FunctionUtils::isPureVirtualFunction(const Function *function,
                                          const LookupContext &context,
                                          QList<const Function *> *firstVirtuals)
{
    return isVirtualFunction_helper(function, context, PureVirtual, firstVirtuals);
}

QList<Function *> FunctionUtils::overrides(Function *function, Class *functionsClass,
                                           Class *staticClass, const Snapshot &snapshot)
{
    QList<Function *> result;
    QTC_ASSERT(function && functionsClass && staticClass, return result);

    FullySpecifiedType referenceType = function->type();
    const Name *referenceName = function->name();
    QTC_ASSERT(referenceName && referenceType.isValid(), return result);

    // Find overrides
    const TypeHierarchy &staticClassHierarchy
            = TypeHierarchyBuilder::buildDerivedTypeHierarchy(staticClass, snapshot);

    QList<TypeHierarchy> l;
    if (functionsClass != staticClass)
        l.append(TypeHierarchy(functionsClass));
    l.append(staticClassHierarchy);

    while (!l.isEmpty()) {
        // Add derived
        const TypeHierarchy hierarchy = l.takeFirst();
        QTC_ASSERT(hierarchy.symbol(), continue);
        Class *c = hierarchy.symbol()->asClass();
        QTC_ASSERT(c, continue);

        foreach (const TypeHierarchy &t, hierarchy.hierarchy()) {
            if (!l.contains(t))
                l << t;
        }

        // Check member functions
        for (int i = 0, total = c->memberCount(); i < total; ++i) {
            Symbol *candidate = c->memberAt(i);
            const Name *candidateName = candidate->name();
            Function *candidateFunc = candidate->type()->asFunctionType();
            if (!candidateName || !candidateFunc)
                continue;
            if (candidateName->match(referenceName)
                    && candidateFunc->isSignatureEqualTo(function)) {
                result << candidateFunc;
            }
        }
    }

    return result;
}

#ifdef WITH_TESTS

#include <QTest>

namespace CppTools {
namespace Internal {

enum Virtuality
{
    NotVirtual,
    Virtual,
    PureVirtual
};
using VirtualityList = QList<Virtuality>;
} // Internal namespace
} // CppTools namespace

Q_DECLARE_METATYPE(CppTools::Internal::Virtuality)

namespace CppTools {
namespace Internal {

void FunctionUtilsTest::testVirtualFunctions()
{
    // Create and parse document
    QFETCH(QByteArray, source);
    QFETCH(VirtualityList, virtualityList);
    QFETCH(QList<int>, firstVirtualList);
    Document::Ptr document = Document::create(QLatin1String("virtuals"));
    document->setUtf8Source(source);
    document->check(); // calls parse();
    QCOMPARE(document->diagnosticMessages().size(), 0);
    QVERIFY(document->translationUnit()->ast());
    QList<const Function *> allFunctions;
    QList<const Function *> firstVirtuals;

    // Iterate through Function symbols
    Snapshot snapshot;
    snapshot.insert(document);
    const LookupContext context(document, snapshot);
    Control *control = document->translationUnit()->control();
    Symbol **end = control->lastSymbol();
    for (Symbol **it = control->firstSymbol(); it != end; ++it) {
        if (const Function *function = (*it)->asFunction()) {
            allFunctions.append(function);
            QTC_ASSERT(!virtualityList.isEmpty(), return);
            Virtuality virtuality = virtualityList.takeFirst();
            QTC_ASSERT(!firstVirtualList.isEmpty(), return);
            int firstVirtualIndex = firstVirtualList.takeFirst();
            bool isVirtual = FunctionUtils::isVirtualFunction(function, context, &firstVirtuals);
            bool isPureVirtual = FunctionUtils::isPureVirtualFunction(function, context,
                                                                      &firstVirtuals);

            // Test for regressions introduced by firstVirtual
            QCOMPARE(FunctionUtils::isVirtualFunction(function, context), isVirtual);
            QCOMPARE(FunctionUtils::isPureVirtualFunction(function, context), isPureVirtual);
            if (isVirtual) {
                if (isPureVirtual)
                    QCOMPARE(virtuality, PureVirtual);
                else
                    QCOMPARE(virtuality, Virtual);
            } else {
                QEXPECT_FAIL("virtual-dtor-dtor", "Not implemented", Abort);
                if (allFunctions.size() == 3)
                    QEXPECT_FAIL("dtor-virtual-dtor-dtor", "Not implemented", Abort);
                QCOMPARE(virtuality, NotVirtual);
            }
            if (firstVirtualIndex == -1)
                QVERIFY(firstVirtuals.isEmpty());
            else
                QCOMPARE(firstVirtuals, {allFunctions.at(firstVirtualIndex)});
        }
    }
    QVERIFY(virtualityList.isEmpty());
    QVERIFY(firstVirtualList.isEmpty());
}

void FunctionUtilsTest::testVirtualFunctions_data()
{
    using _ = QByteArray;
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<VirtualityList>("virtualityList");
    QTest::addColumn<QList<int> >("firstVirtualList");

    QTest::newRow("none")
            << _("struct None { void foo() {} };\n")
            << (VirtualityList() << NotVirtual)
            << (QList<int>() << -1);

    QTest::newRow("single-virtual")
            << _("struct V { virtual void foo() {} };\n")
            << (VirtualityList() << Virtual)
            << (QList<int>() << 0);

    QTest::newRow("single-pure-virtual")
            << _("struct PV { virtual void foo() = 0; };\n")
            << (VirtualityList() << PureVirtual)
            << (QList<int>() << 0);

    QTest::newRow("virtual-derived-with-specifier")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() {} };\n")
            << (VirtualityList() << Virtual << Virtual)
            << (QList<int>() << 0 << 0);

    QTest::newRow("virtual-derived-implicit")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { void foo() {} };\n")
            << (VirtualityList() << Virtual << Virtual)
            << (QList<int>() << 0 << 0);

    QTest::newRow("not-virtual-then-virtual")
            << _("struct Base { void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() {} };\n")
            << (VirtualityList() << NotVirtual << Virtual)
            << (QList<int>() << -1 << 1);

    QTest::newRow("virtual-final-not-virtual")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { void foo() final {} };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << Virtual << NotVirtual)
            << (QList<int>() << 0 << 0 << -1);

    QTest::newRow("virtual-then-pure")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() = 0; };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << PureVirtual << Virtual)
            << (QList<int>() << 0 << 0 << 0);

    QTest::newRow("virtual-virtual-final-not-virtual")
            << _("struct Base { virtual void foo() {} };\n"
                 "struct Derived : Base { virtual void foo() final {} };\n"
                 "struct Derived2 : Derived { void foo() {} };")
            << (VirtualityList() << Virtual << Virtual << NotVirtual)
            << (QList<int>() << 0 << 0 << -1);

    QTest::newRow("ctor-virtual-dtor")
            << _("struct Base { Base() {} virtual ~Base() {} };\n")
            << (VirtualityList() << NotVirtual << Virtual)
            << (QList<int>() << -1 << 1);

    QTest::newRow("virtual-dtor-dtor")
            << _("struct Base { virtual ~Base() {} };\n"
                 "struct Derived : Base { ~Derived() {} };\n")
            << (VirtualityList() << Virtual << Virtual)
            << (QList<int>() << 0 << 0);

    QTest::newRow("dtor-virtual-dtor-dtor")
            << _("struct Base { ~Base() {} };\n"
                 "struct Derived : Base { virtual ~Derived() {} };\n"
                 "struct Derived2 : Derived { ~Derived2() {} };\n")
            << (VirtualityList() << NotVirtual << Virtual << Virtual)
            << (QList<int>() << -1 << 1 << 1);
}

} // namespace Internal
} // namespace CppTools

#endif
