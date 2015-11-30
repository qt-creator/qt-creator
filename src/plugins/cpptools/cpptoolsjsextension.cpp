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

#include "cpptoolsjsextension.h"

#include "cppfilesettingspage.h"

#include <coreplugin/icore.h>

#include <utils/codegeneration.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QTextStream>

namespace CppTools {
namespace Internal {

QString CppToolsJsExtension::headerGuard(const QString &in) const
{
    return Utils::headerGuard(in);
}

QString CppToolsJsExtension::fileName(const QString &path, const QString &extension) const
{
    QString raw = Utils::FileName::fromString(path, extension).toString();
    CppFileSettings settings;
    settings.fromSettings(Core::ICore::settings());
    if (!settings.lowerCaseFiles)
        return raw;

    QFileInfo fi = QFileInfo(raw);
    QString finalPath = fi.path();
    if (finalPath == QStringLiteral("."))
        finalPath.clear();
    if (!finalPath.isEmpty() && !finalPath.endsWith(QLatin1Char('/')))
        finalPath += QLatin1Char('/');
    QString name = fi.baseName().toLower();
    QString ext = fi.completeSuffix();
    if (!ext.isEmpty())
        ext = QString(QLatin1Char('.')) + ext;
    return finalPath + name + ext;
}


static QStringList parts(const QString &klass)
{
    return klass.split(QStringLiteral("::"));
}

QStringList CppToolsJsExtension::namespaces(const QString &klass) const
{
    QStringList result = parts(klass);
    result.removeLast();
    return result;
}

QString CppToolsJsExtension::className(const QString &klass) const
{
    QStringList result = parts(klass);
    return result.last();
}

QString CppToolsJsExtension::classToFileName(const QString &klass, const QString &extension) const
{
    return fileName(className(klass), extension);
}

QString CppToolsJsExtension::classToHeaderGuard(const QString &klass, const QString &extension) const
{
    return Utils::headerGuard(fileName(className(klass), extension), namespaces(klass));
}

QString CppToolsJsExtension::openNamespaces(const QString &klass) const
{
    QString result;
    QTextStream str(&result);
    Utils::writeOpeningNameSpaces(namespaces(klass), QString(), str);
    return result;
}

QString CppToolsJsExtension::closeNamespaces(const QString &klass) const
{
    QString result;
    QTextStream str(&result);
    Utils::writeClosingNameSpaces(namespaces(klass), QString(), str);
    return result;
}

} // namespace Internal
} // namespace CppTools
