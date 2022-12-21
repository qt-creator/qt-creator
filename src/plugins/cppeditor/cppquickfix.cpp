// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfix.h"

#include "cppquickfixassistant.h"
#include "cpprefactoringchanges.h"

using namespace CPlusPlus;
using namespace TextEditor;

namespace CppEditor::Internal {

const QStringList magicQObjectFunctions()
{
    static QStringList list{"metaObject", "qt_metacast", "qt_metacall", "qt_static_metacall"};
    return list;
}

CppQuickFixOperation::CppQuickFixOperation(const CppQuickFixInterface &interface, int priority)
    : QuickFixOperation(priority), CppQuickFixInterface(interface)
{}

CppQuickFixOperation::~CppQuickFixOperation() = default;

} // namespace CppEditor::Internal
