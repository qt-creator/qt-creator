/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANG_SOURCEMARKER_H
#define CLANG_SOURCEMARKER_H

#include "clang_global.h"
#include "sourcelocation.h"

namespace ClangCodeModel {

class CLANG_EXPORT SourceMarker
{
public: // TODO: remove this, it's about the same as the TextEditor::SemanticHighlighter::Result
    enum Kind {
        Unknown = 0,
        Type = 1,
        Local,
        Field,
        Enumeration,
        VirtualMethod,
        Label,
        Macro,
        Function,
        PseudoKeyword,
        ObjCString,

        ObjectiveCMessage = VirtualMethod
    };

    SourceMarker();
    SourceMarker(const SourceLocation &location,
                 unsigned length,
                 Kind kind);

    bool isValid() const
    { return m_loc.line() != 0; }

    bool isInvalid() const
    { return m_loc.line() == 0; }

    const SourceLocation &location() const
    { return m_loc; }

    unsigned length() const
    { return m_length; }

    Kind kind() const
    { return m_kind; }

    bool lessThan(const SourceMarker &other) const
    {
        if (m_loc.line() != other.m_loc.line())
            return m_loc.line() < other.m_loc.line();
        if (m_loc.column() != other.m_loc.column())
            return m_loc.column() < other.m_loc.column();
        return m_length < other.m_length;
    }

private:
    SourceLocation m_loc;
    unsigned m_length;
    Kind m_kind;
};

CLANG_EXPORT inline bool operator<(const SourceMarker &one, const SourceMarker &two)
{ return one.lessThan(two); }

} // namespace Clang

#endif // CLANG_SOURCEMARKER_H
