/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "corejsextensions.h"

#include "mimedatabase.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QTemporaryFile>
#include <QVariant>

namespace Core {
namespace Internal {

QString UtilsJsExtension::toNativeSeparators(const QString &in) const
{
    return QDir::toNativeSeparators(in);
}

QString UtilsJsExtension::fromNativeSeparators(const QString &in) const
{
    return QDir::fromNativeSeparators(in);
}

QString UtilsJsExtension::baseName(const QString &in) const
{
    QFileInfo fi(in);
    return fi.baseName();
}

QString UtilsJsExtension::completeBaseName(const QString &in) const
{
    QFileInfo fi(in);
    return fi.completeBaseName();
}

QString UtilsJsExtension::suffix(const QString &in) const
{
    QFileInfo fi(in);
    return fi.suffix();
}

QString UtilsJsExtension::completeSuffix(const QString &in) const
{
    QFileInfo fi(in);
    return fi.completeSuffix();
}

QString UtilsJsExtension::path(const QString &in) const
{
    QFileInfo fi(in);
    return fi.path();
}

QString UtilsJsExtension::absoluteFilePath(const QString &in) const
{
    QFileInfo fi(in);
    return fi.absoluteFilePath();
}

QString UtilsJsExtension::preferredSuffix(const QString &mimetype) const
{
    return Core::MimeDatabase::preferredSuffixByType(mimetype);
}

QString UtilsJsExtension::fileName(const QString &path, const QString &extension) const
{
    return Utils::FileName::fromString(path, extension).toString();
}

QString UtilsJsExtension::mktemp(const QString &pattern) const
{
    QString tmp = pattern;
    if (tmp.isEmpty())
        tmp = QStringLiteral("qt_temp.XXXXXX");
    QFileInfo fi(tmp);
    if (!fi.isAbsolute()) {
        QString tempPattern = QDir::tempPath();
        if (!tempPattern.endsWith(QLatin1Char('/')))
            tempPattern += QLatin1Char('/');
        tmp = tempPattern + tmp;
    }

    QTemporaryFile file(tmp);
    file.setAutoRemove(false);
    QTC_ASSERT(file.open(), return QString());
    file.close();
    return file.fileName();
}

} // namespace Internal
} // namespace Core
