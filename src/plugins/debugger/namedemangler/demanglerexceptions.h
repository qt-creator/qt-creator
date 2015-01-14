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
#ifndef DEMANGLEREXCEPTIONS_H
#define DEMANGLEREXCEPTIONS_H

#include <QtGlobal>
#include <QSharedPointer>
#include <QString>

namespace Debugger {
namespace Internal {

class ParseTreeNode;

class ParseException
{
public:
    ParseException(const QString &error) : error(error) {}

    const QString error;
};

class InternalDemanglerException
{
public:
    InternalDemanglerException(const QString &func, const QString &file, int line)
            : func(func), file(file), line(line) {}

    QString func;
    QString file;
    int line;
};

#define DEMANGLER_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            throw InternalDemanglerException(QLatin1String(Q_FUNC_INFO), QLatin1String(__FILE__), __LINE__); \
        } \
    } while (0)

template <typename T> QSharedPointer<T> demanglerCast(const QSharedPointer<ParseTreeNode> &node,
        const QString &func, const QString &file, int line)
{
    const QSharedPointer<T> out = node.dynamicCast<T>();
    if (!out)
        throw InternalDemanglerException(func, file, line);
    return out;
}

#define DEMANGLER_CAST(type, input) demanglerCast<type>(input, QLatin1String(Q_FUNC_INFO), \
        QLatin1String(__FILE__), __LINE__)

} // namespace Internal
} // namespace Debugger

#endif // DEMANGLEREXCEPTIONS_H
