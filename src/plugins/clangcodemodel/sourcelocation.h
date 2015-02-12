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

#ifndef SOURCELOCATION_H
#define SOURCELOCATION_H

#include "clang_global.h"

#include <QtCore/QString>
#include <QtCore/QDebug>

namespace ClangCodeModel {

class CLANG_EXPORT SourceLocation
{
public:
    SourceLocation();
    SourceLocation(const QString &fileName,
                   unsigned line = 0,
                   unsigned column = 0,
                   unsigned offset = 0);

    bool isNull() const { return m_fileName.isEmpty(); }
    const QString &fileName() const { return m_fileName; }
    unsigned line() const { return m_line; }
    unsigned column() const { return m_column; }
    unsigned offset() const { return m_offset; }

private:
    QString m_fileName;
    unsigned m_line;
    unsigned m_column;
    unsigned m_offset;
};

bool operator==(const SourceLocation &a, const SourceLocation &b);
bool operator!=(const SourceLocation &a, const SourceLocation &b);

QDebug operator<<(QDebug dbg, const SourceLocation &location);

} // ClangCodeModel

#endif // SOURCELOCATION_H
