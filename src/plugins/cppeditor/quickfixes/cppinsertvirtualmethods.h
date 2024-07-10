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
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override;
#ifdef WITH_TESTS
    static QObject *createTest();
    static InsertVirtualMethods *createTestFactory();
#endif

private:
    InsertVirtualMethodsDialog *m_dialog;
};

void registerInsertVirtualMethodsQuickfix();

} // namespace Internal
} // namespace CppEditor
