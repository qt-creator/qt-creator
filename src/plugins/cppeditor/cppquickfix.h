// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppquickfixassistant.h"

#include <texteditor/quickfix.h>

#include <QVersionNumber>

#include <optional>

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

    void match(const Internal::CppQuickFixInterface &interface, QuickFixOperations &result);

    static const QList<CppQuickFixFactory *> &cppQuickFixFactories();

    std::optional<QVersionNumber> clangdReplacement() const { return m_clangdReplacement; }
    void setClangdReplacement(const QVersionNumber &version) { m_clangdReplacement = version; }

private:
    /*!
        Implement this function to doMatch and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual void doMatch(const Internal::CppQuickFixInterface &interface,
                         QuickFixOperations &result) = 0;

    std::optional<QVersionNumber> m_clangdReplacement;
};

} // namespace CppEditor
