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

#include <QHash>

#include "import.h"

namespace QmlDesigner {

Import Import::createLibraryImport(const QString &url, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(url, QString(), version, alias, importPaths);
}

Import Import::createFileImport(const QString &file, const QString &version, const QString &alias, const QStringList &importPaths)
{
    return Import(QString(), file, version, alias, importPaths);
}

Import Import::empty()
{
    return Import(QString(), QString(), QString(), QString(), QStringList());
}

Import::Import(const QString &url, const QString &file, const QString &version, const QString &alias, const QStringList &importPaths):
        m_url(url),
        m_file(file),
        m_version(version),
        m_alias(alias),
        m_importPathList(importPaths)
{
}

QString Import::toString(bool addSemicolon, bool skipAlias) const
{
    QString result = QLatin1String("import ");

    if (isFileImport())
        result += '"' + file() + '"';
    else if (isLibraryImport())
        result += url();
    else
        return QString();

    if (hasVersion())
        result += ' ' + version();

    if (hasAlias() && !skipAlias)
        result += " as " + alias();

    if (addSemicolon)
        result += ';';

    return result;
}

bool Import::operator==(const Import &other) const
{
    return url() == other.url() && file() == other.file() && version() == other.version() && alias() == other.alias();
}

uint qHash(const Import &import)
{
    return ::qHash(import.url()) ^ ::qHash(import.file()) ^ ::qHash(import.version()) ^ ::qHash(import.alias());
}

} // namespace QmlDesigner
