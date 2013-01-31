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

#include "codegeneration.h"

#include <QTextStream>
#include <QStringList>
#include <QFileInfo>

namespace Utils {

QTCREATOR_UTILS_EXPORT QString fileNameToCppIdentifier(const QString &s)
{
    QString rc;
    const int len = s.size();
    const QChar underscore =  QLatin1Char('_');
    const QChar dot =  QLatin1Char('.');

    for (int i = 0; i < len; i++) {
        const QChar c = s.at(i);
        if (c == underscore || c.isLetterOrNumber())
            rc += c;
        else if (c == dot)
            rc += underscore;
    }
    return rc;
}

QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file)
{
    return headerGuard(file, QStringList());
}

QTCREATOR_UTILS_EXPORT QString headerGuard(const QString &file, const QStringList &namespaceList)
{
    const QChar underscore = QLatin1Char('_');
    QString rc;
    for (int i = 0; i < namespaceList.count(); i++)
        rc += namespaceList.at(i).toUpper() + underscore;

    const QFileInfo fi(file);
    rc += fileNameToCppIdentifier(fi.fileName()).toUpper();
    return rc;
}

QTCREATOR_UTILS_EXPORT
void writeIncludeFileDirective(const QString &file, bool globalInclude,
                               QTextStream &str)
{
    const QChar opening = globalInclude ?  QLatin1Char('<') : QLatin1Char('"');
    const QChar closing = globalInclude ?  QLatin1Char('>') : QLatin1Char('"');
    str << QLatin1String("#include ") << opening << file <<  closing << QLatin1Char('\n');
}

QTCREATOR_UTILS_EXPORT void writeBeginQtVersionCheck(QTextStream &str)
{
    str << QLatin1String("#if QT_VERSION >= 0x050000\n");
}

QTCREATOR_UTILS_EXPORT
QString writeOpeningNameSpaces(const QStringList &l, const QString &indent,
                               QTextStream &str)
{
    const int count = l.size();
    QString rc;
    if (count) {
        str << '\n';
        for (int i = 0; i < count; i++) {
            str << rc << "namespace " << l.at(i) << " {\n";
            rc += indent;
        }
    }
    return rc;
}

QTCREATOR_UTILS_EXPORT
void writeClosingNameSpaces(const QStringList &l, const QString &indent,
                            QTextStream &str)
{
    if (!l.empty())
        str << '\n';
    for (int i = l.size() - 1; i >= 0; i--) {
        if (i)
            str << QString(indent.size() * i, QLatin1Char(' '));
        str << "} // namespace " <<  l.at(i) << '\n';
    }
}

} // namespace Utils
