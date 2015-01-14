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

#ifndef CLANG_DIAGNOSTIC_H
#define CLANG_DIAGNOSTIC_H

#include "clang_global.h"
#include "sourcelocation.h"

#include <QMetaType>

namespace ClangCodeModel {

class CLANG_EXPORT Diagnostic
{
public:
    enum Severity {
        Unknown = -1,
        Ignored = 0,
        Note = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };

public:
    Diagnostic();
    Diagnostic(Severity severity, const SourceLocation &location, unsigned length, const QString &spelling);

    Severity severity() const
    { return m_severity; }

    const QString severityAsString() const;

    const SourceLocation &location() const
    { return m_loc; }

    unsigned length() const
    { return m_length; }

    const QString &spelling() const
    { return m_spelling; }

private:
    Severity m_severity;
    SourceLocation m_loc;
    unsigned m_length;
    QString m_spelling;
};

} // namespace ClangCodeModel

Q_DECLARE_METATYPE(ClangCodeModel::Diagnostic)
Q_DECLARE_METATYPE(QList<ClangCodeModel::Diagnostic>)

#endif // CLANG_DIAGNOSTIC_H
