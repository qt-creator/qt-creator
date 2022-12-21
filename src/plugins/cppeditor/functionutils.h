// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QObject>

namespace CPlusPlus {
class Class;
class Function;
class LookupContext;
class Snapshot;
class Symbol;
} // namespace CPlusPlus

namespace CppEditor::Internal {

class FunctionUtils
{
public:
    static bool isVirtualFunction(const CPlusPlus::Function *function,
                                  const CPlusPlus::LookupContext &context,
                                  QList<const CPlusPlus::Function *> *firstVirtuals = nullptr);

    static bool isPureVirtualFunction(const CPlusPlus::Function *function,
                                      const CPlusPlus::LookupContext &context,
                                      QList<const CPlusPlus::Function *> *firstVirtuals = nullptr);

    static QList<CPlusPlus::Function *> overrides(CPlusPlus::Function *function,
                                                  CPlusPlus::Class *functionsClass,
                                                  CPlusPlus::Class *staticClass,
                                                  const CPlusPlus::Snapshot &snapshot);
};

#ifdef WITH_TESTS
class FunctionUtilsTest : public QObject
{
    Q_OBJECT

private slots:
    void testVirtualFunctions();
    void testVirtualFunctions_data();
};
#endif // WITH_TESTS

} // namespace CppEditor::Internal
