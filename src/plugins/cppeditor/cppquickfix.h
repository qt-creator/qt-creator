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

#pragma once

#include "cppeditor_global.h"
#include "cppquickfixassistant.h"

#include <texteditor/quickfix.h>

namespace CPlusPlus { class Snapshot; }

namespace CppEditor {
namespace Internal { class CppQuickFixInterface; }

class CPPEDITOR_EXPORT CppQuickFixOperation
    : public TextEditor::QuickFixOperation,
      public Internal::CppQuickFixInterface
{
public:
    explicit CppQuickFixOperation(const CppQuickFixInterface &interface, int priority = -1);
    ~CppQuickFixOperation();
};

class CPPEDITOR_EXPORT CppQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    CppQuickFixFactory() {}

    void matchingOperations(const TextEditor::QuickFixInterface &interface,
        TextEditor::QuickFixOperations &result);

    /*!
        Implement this function to match and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual void match(const Internal::CppQuickFixInterface &interface,
        TextEditor::QuickFixOperations &result) = 0;
};

} // namespace CppEditor
