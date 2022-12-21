// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppquickfix.h"

namespace CppEditor {
namespace Internal {

class InsertVirtualMethodsDialog;

class InsertVirtualMethods : public CppQuickFixFactory
{
    Q_OBJECT
public:
    InsertVirtualMethods(InsertVirtualMethodsDialog *dialog = nullptr);
    ~InsertVirtualMethods() override;
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
#ifdef WITH_TESTS
    static InsertVirtualMethods *createTestFactory();
#endif

private:
    InsertVirtualMethodsDialog *m_dialog;
};

#ifdef WITH_TESTS
namespace Tests {
class InsertVirtualMethodsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data();
    void test();
    void testImplementationFile();
    void testBaseClassInNamespace();
};
} // namespace Tests
#endif // WITH_TESTS

} // namespace Internal
} // namespace CppEditor
