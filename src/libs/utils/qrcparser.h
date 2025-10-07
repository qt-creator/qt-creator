// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QStringList>

QT_BEGIN_NAMESPACE
class QLocale;
template <typename K, typename V>
class QMap;
QT_END_NAMESPACE

namespace Utils {

namespace Internal {
class QrcParserPrivate;
class QrcCachePrivate;
}

class QTCREATOR_UTILS_EXPORT QrcParser
{
public:
    struct MatchResult
    {
        int matchDepth = {};
        QStringList reversedPaths;
        FilePaths sourceFiles;
    };

    using Ptr = std::shared_ptr<QrcParser>;
    using ConstPtr = std::shared_ptr<const QrcParser>;

    ~QrcParser();

    bool parseFile(const FilePath &path, const QString &contents);
    FilePath firstFileAtPath(const QString &path, const QLocale &locale) const;
    void collectFilesAtPath(
        const QString &path, FilePaths *res, const QLocale *locale = nullptr) const;
    MatchResult longestReverseMatches(const QString &) const;
    bool hasDirAtPath(const QString &path, const QLocale *locale = nullptr) const;
    void collectFilesInPath(
        const QString &path,
        QMap<QString, FilePaths> *res,
        bool addDirs = false,
        const QLocale *locale = nullptr) const;
    void collectResourceFilesForSourceFile(const FilePath &sourceFile, QStringList *results,
                                           const QLocale *locale = nullptr) const;

    QStringList errorMessages() const;
    QStringList languages() const;
    bool isValid() const;

    static Ptr parseQrcFile(const FilePath &path, const QString &contents);
    static QString normalizedQrcFilePath(const QString &path);
    static QString normalizedQrcDirectoryPath(const QString &path);
    static QString qrcDirectoryPathForQrcFilePath(const QString &file);

private:
    QrcParser();
    QrcParser(const QrcParser &);
    Internal::QrcParserPrivate *d;
};

class QTCREATOR_UTILS_EXPORT QrcCache
{
public:
    QrcCache();
    ~QrcCache();
    QrcParser::ConstPtr addPath(const FilePath &path, const QString &contents);
    void removePath(const FilePath &path);
    QrcParser::ConstPtr updatePath(const FilePath &path, const QString &contents);
    QrcParser::ConstPtr parsedPath(const FilePath &path);
    void clear();

private:
    Internal::QrcCachePrivate *d;
};

} // Utils
