// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "corejsextensions.h"

#include "icore.h"
#include "jsbind.h"
#include "messagemanager.h"

#include <utils/appinfo.h>
#include <utils/fileutils.h>
#include <utils/mimeutils.h>
#include <utils/stringutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QVersionNumber>

using namespace Utils;

namespace Core::Internal {

static QString qtQuickVersion(const QString &filePath)
{
    QDirIterator dirIt(FilePath::fromString(filePath).parentDir().path(), {"*.qml"},
                       QDir::Files, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        const Result<QByteArray> result = FilePath::fromString(dirIt.next()).fileContents();
        if (!result)
            continue;
        const QString data = QString::fromUtf8(*result);
        static const QString importString("import QtQuick");
        const int importIndex = data.indexOf(importString);
        if (importIndex == -1)
            continue;
        const int versionIndex = importIndex + importString.size();
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

void registerUtilFunctions(JsBind::Registrar &registrar)
{
    // General information
    registrar.set("qtVersion", [] { return QString::fromLatin1(qVersion()); });
    registrar.set("qtCreatorVersion", [] { return appInfo().displayVersion; });
    registrar.set("qtCreatorIdeVersion", [] { return QCoreApplication::applicationVersion(); });
    registrar.set("qtCreatorSettingsPath",
                  [] { return ICore::userResourcePath().toUrlishString(); });

    // Compares two Major.Minor.Patch version strings. Returns a negative value,
    // zero, or a positive value if version1 is less than, equal to, or greater
    // than version2. Use this instead of JavaScript's string/number comparison,
    // which gets e.g. "6.9" vs "6.10" wrong.
    registrar.set("versionCompare", [](QString version1, QString version2) {
        return QVersionNumber::compare(QVersionNumber::fromString(version1),
                                       QVersionNumber::fromString(version2));
    });

    // File name conversions:
    registrar.set("toNativeSeparators", [](QString in) { return QDir::toNativeSeparators(in); });
    registrar.set("fromNativeSeparators", [](QString in) { return QDir::fromNativeSeparators(in); });

    registrar.set("baseName", [](QString in) { return QFileInfo(in).baseName(); });
    registrar.set("completeBaseName", [](QString in) { return QFileInfo(in).completeBaseName(); });
    registrar.set("suffix", [](QString in) { return QFileInfo(in).suffix(); });
    registrar.set("completeSuffix", [](QString in) { return QFileInfo(in).completeSuffix(); });
    registrar.set("path", [](QString in) { return QFileInfo(in).path(); });
    registrar.set("absoluteFilePath", [](QString in) { return QFileInfo(in).absoluteFilePath(); });

    registrar.set("relativeFilePath", [](QString path, QString base) {
        const FilePath basePath = FilePath::fromString(base).cleanPath();
        const FilePath filePath = FilePath::fromString(path).cleanPath();
        return filePath.relativePathFromDir(basePath);
    });

    // fileName(path) returns the file name; fileName(path, extension) builds one.
    registrar.setRaw("fileName", [](const QVariantList &args) -> QVariant {
        const QString path = args.value(0).toString();
        if (args.size() >= 2)
            return FilePath::fromStringWithExtension(path, args.at(1).toString()).toUrlishString();
        return QFileInfo(path).fileName();
    });

    // File checks:
    registrar.set("exists", [](QString in) { return FilePath::fromUserInput(in).exists(); });
    registrar.set("isDirectory", [](QString in) { return FilePath::fromUserInput(in).isDir(); });
    registrar.set("isFile", [](QString in) { return FilePath::fromUserInput(in).isFile(); });

    // MimeDB:
    registrar.set("preferredSuffix", [](QString mimetype) -> QString {
        const MimeType mt = mimeTypeForName(mimetype);
        if (mt.isValid())
            return mt.preferredSuffix();
        return {};
    });

    // Generate temporary file:
    registrar.set("mktemp", [](QString pattern) -> QString {
        QString tmp = pattern;
        if (tmp.isEmpty())
            tmp = QStringLiteral("qt_temp.XXXXXX");
        const auto res = FileUtils::scratchBufferFilePath(tmp);
        if (!res) {
            MessageManager::writeDisrupting(res.error());
            return {};
        }
        return res->toFSPathString();
    });

    // Generate a ascii-only string:
    registrar.set("asciify", [](QString input) { return Utils::asciify(input); });

    // Generate a string that is safe to use as a component in a file path:
    registrar.set("fileSystemFriendly",
                  [](QString input) { return FileUtils::fileSystemFriendlyName(input); });

    // Heuristic to find out which QtQuick import version to use for the given file.
    registrar.set("qtQuickVersion", [](QString filePath) { return qtQuickVersion(filePath); });
}

} // namespace Core::Internal
