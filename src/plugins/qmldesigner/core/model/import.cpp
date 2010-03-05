/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QHash>

#include "import.h"

namespace QmlDesigner {

Import Import::createLibraryImport(const QString &url, const QString &version, const QString &alias)
{
    return Import(url, QString(), version, alias);
}

Import Import::createFileImport(const QString &file, const QString &version, const QString &alias)
{
    return Import(QString(), file, version, alias);
}

Import Import::empty()
{
    return Import(QString(), QString(), QString(), QString());
}

Import::Import(const QString &url, const QString &file, const QString &version, const QString &alias):
        m_url(url),
        m_file(file),
        m_version(version),
        m_alias(alias)
{
}

QString Import::toString(bool addSemicolon) const
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

    if (hasAlias())
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
