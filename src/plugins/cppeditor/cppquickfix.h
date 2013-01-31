/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPQUICKFIX_H
#define CPPQUICKFIX_H

#include "cppeditor_global.h"
#include <texteditor/quickfix.h>

namespace CPlusPlus {
class CppModelManagerInterface;
class Snapshot;
}

namespace CppEditor {
namespace Internal { class CppQuickFixAssistInterface; }

typedef QSharedPointer<const Internal::CppQuickFixAssistInterface> CppQuickFixInterface;

class CPPEDITOR_EXPORT CppQuickFixOperation: public TextEditor::QuickFixOperation
{
public:
    explicit CppQuickFixOperation(const CppQuickFixInterface &interface, int priority = -1);
    ~CppQuickFixOperation();

protected:
    QString fileName() const;
    CPlusPlus::Snapshot snapshot() const;
    const Internal::CppQuickFixAssistInterface *assistInterface() const;

private:
    CppQuickFixInterface m_interface;
};

class CPPEDITOR_EXPORT CppQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    CppQuickFixFactory() {}

    void matchingOperations(const TextEditor::QuickFixInterface &interface,
        TextEditor::QuickFixOperations &result);

    /*!
        Implement this method to match and create the appropriate
        CppQuickFixOperation objects.
     */
    virtual void match(const CppQuickFixInterface &interface,
        TextEditor::QuickFixOperations &result) = 0;
};

} // namespace CppEditor

#endif // CPPQUICKFIX_H
