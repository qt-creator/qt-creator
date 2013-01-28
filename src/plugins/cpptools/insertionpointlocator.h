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

#ifndef INSERTIONPOINTLOCATOR_H
#define INSERTIONPOINTLOCATOR_H

#include "cpptools_global.h"

#include <ASTfwd.h>
#include <CPlusPlusForwardDeclarations.h>

#include <cplusplus/CppDocument.h>
#include <cpptools/cpprefactoringchanges.h>

namespace CppTools {

class CPPTOOLS_EXPORT InsertionLocation
{
public:
    InsertionLocation();
    InsertionLocation(const QString &fileName, const QString &prefix,
                      const QString &suffix, unsigned line, unsigned column);

    QString fileName() const
    { return m_fileName; }

    /// \returns The prefix to insert before any other text.
    QString prefix() const
    { return m_prefix; }

    /// \returns The suffix to insert after the other inserted text.
    QString suffix() const
    { return m_suffix; }

    /// \returns The line where to insert. The line number is 1-based.
    unsigned line() const
    { return m_line; }

    /// \returns The column where to insert. The column number is 1-based.
    unsigned column() const
    { return m_column; }

    bool isValid() const
    { return !m_fileName.isEmpty() && m_line > 0 && m_column > 0; }

private:
    QString m_fileName;
    QString m_prefix;
    QString m_suffix;
    unsigned m_line;
    unsigned m_column;
};

class CPPTOOLS_EXPORT InsertionPointLocator
{
public:
    enum AccessSpec {
        Invalid = -1,
        Signals = 0,

        Public = 1,
        Protected = 2,
        Private = 3,

        SlotBit = 1 << 2,

        PublicSlot    = Public    | SlotBit,
        ProtectedSlot = Protected | SlotBit,
        PrivateSlot   = Private   | SlotBit
    };

public:
    InsertionPointLocator(const CppRefactoringChanges &refactoringChanges);

    InsertionLocation methodDeclarationInClass(const QString &fileName,
                                               const CPlusPlus::Class *clazz,
                                               AccessSpec xsSpec) const;

    QList<InsertionLocation> methodDefinition(CPlusPlus::Declaration *declaration) const;

private:
    CppRefactoringChanges m_refactoringChanges;
};

} // namespace CppTools

#endif // INSERTIONPOINTLOCATOR_H
