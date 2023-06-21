// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "corejsextensions.h"

#include <utils/appinfo.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDirIterator>
#include <QLibraryInfo>
#include <QTemporaryFile>
#include <QVariant>
#include <QVersionNumber>

using namespace Utils;

namespace Core::Internal {

QString UtilsJsExtension::qtVersion() const
{
    return QLatin1String(qVersion());
}

QString UtilsJsExtension::qtCreatorVersion() const
{
    return appInfo().displayVersion;
}

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

QString UtilsJsExtension::fileName(const QString &in) const
{
    QFileInfo fi(in);
    return fi.fileName();
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

QString UtilsJsExtension::relativeFilePath(const QString &path, const QString &base) const
{
    const FilePath basePath = FilePath::fromString(base).cleanPath();
    const FilePath filePath = FilePath::fromString(path).cleanPath();
    return FilePath::calcRelativePath(filePath.toFSPathString(), basePath.toFSPathString());
}

bool UtilsJsExtension::exists(const QString &in) const
{
    return QFileInfo::exists(in);
}

bool UtilsJsExtension::isDirectory(const QString &in) const
{
    return QFileInfo(in).isDir();
}

bool UtilsJsExtension::isFile(const QString &in) const
{
    return QFileInfo(in).isFile();
}

QString UtilsJsExtension::preferredSuffix(const QString &mimetype) const
{
    Utils::MimeType mt = Utils::mimeTypeForName(mimetype);
    if (mt.isValid())
        return mt.preferredSuffix();
    return QString();
}

QString UtilsJsExtension::fileName(const QString &path, const QString &extension) const
{
    return Utils::FilePath::fromStringWithExtension(path, extension).toString();
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
    const bool isOpen = file.open();
    QTC_ASSERT(isOpen, return {});
    file.close();
    return file.fileName();
}

QString UtilsJsExtension::asciify(const QString &input) const
{
    QString result;
    for (const QChar &c : input) {
        if (c.isPrint() && c.unicode() < 128)
            result.append(c);
        else
            result.append(QString::fromLatin1("u%1").arg(c.unicode(), 4, 16, QChar('0')));
    }
    return result;
}

QString UtilsJsExtension::qtQuickVersion(const QString &filePath) const
{
    QDirIterator dirIt(Utils::FilePath::fromString(filePath).parentDir().path(), {"*.qml"},
                       QDir::Files, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        Utils::FileReader reader;
        if (!reader.fetch(Utils::FilePath::fromString(dirIt.next())))
            continue;
        const QString data = QString::fromUtf8(reader.data());
        static const QString importString("import QtQuick");
        const int importIndex = data.indexOf(importString);
        if (importIndex == -1)
            continue;
        const int versionIndex = importIndex + importString.length();
        const int newLineIndex = data.indexOf('\n', versionIndex);
        if (newLineIndex == -1)
            continue;
        const QString versionString = data.mid(versionIndex,
                                               newLineIndex - versionIndex).simplified();
        if (versionString.isEmpty())
            return {};
        const auto version = QVersionNumber::fromString(versionString);
        if (version.isNull())
            return {};
        return version.toString();
    }
    return QLatin1String("2.15");
}

} // Core::Internal
