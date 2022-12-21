// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppquickfixassistant.h"

#include <texteditor/quickfix.h>

namespace CppEditor {
namespace Internal {
class CppQuickFixInterface;

// These are generated functions that should not be offered in quickfixes.
const QStringList magicQObjectFunctions();

class CppQuickFixOperation
    : public TextEditor::QuickFixOperation,
      public Internal::CppQuickFixInterface
{
public:
    explicit CppQuickFixOperation(const CppQuickFixInterface &interface, int priority = -1);
    ~CppQuickFixOperation() override;
};

} // namespace Internal

/*!
    The QuickFixFactory is responsible for generating QuickFixOperation s which are
    applicable to the given QuickFixState.

    A QuickFixFactory should not have any state -- it can be invoked multiple times
    for different QuickFixState objects to create the matching operations, before any
    of those operations are applied (or released).

    This way, a single factory can be used by multiple editors, and a single editor
    can have multiple QuickFixCollector objects for different parts of the code.
 */

class CPPEDITOR_EXPORT CppQuickFixFactory : public QObject
{
    Q_OBJECT

public:
    CppQuickFixFactory();
    ~CppQuickFixFactory() override;

    using QuickFixOperations = TextEditor::QuickFixOperations;

    /*!
        Implement this function to match and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual void match(const Internal::CppQuickFixInterface &interface,
                       QuickFixOperations &result) = 0;

    static const QList<CppQuickFixFactory *> &cppQuickFixFactories();
};

} // namespace CppEditor
