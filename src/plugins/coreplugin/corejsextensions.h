// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/stringutils.h>

#include <QObject>
#include <QSet>

namespace Core::Internal {

class UtilsJsExtension : public QObject
{
    Q_OBJECT

public:
    UtilsJsExtension(QObject *parent = nullptr) : QObject(parent) { }

    // General information
    Q_INVOKABLE QString qtVersion() const;
    Q_INVOKABLE QString qtCreatorVersion() const;

    // File name conversions:
    Q_INVOKABLE QString toNativeSeparators(const QString &in) const;
    Q_INVOKABLE QString fromNativeSeparators(const QString &in) const;

    Q_INVOKABLE QString baseName(const QString &in) const;
    Q_INVOKABLE QString fileName(const QString &in) const;
    Q_INVOKABLE QString completeBaseName(const QString &in) const;
    Q_INVOKABLE QString suffix(const QString &in) const;
    Q_INVOKABLE QString completeSuffix(const QString &in) const;
    Q_INVOKABLE QString path(const QString &in) const;
    Q_INVOKABLE QString absoluteFilePath(const QString &in) const;

    Q_INVOKABLE QString relativeFilePath(const QString &path, const QString &base) const;

    // File checks:
    Q_INVOKABLE bool exists(const QString &in) const;
    Q_INVOKABLE bool isDirectory(const QString &in) const;
    Q_INVOKABLE bool isFile(const QString &in) const;

    // MimeDB:
    Q_INVOKABLE QString preferredSuffix(const QString &mimetype) const;

    // Generate filename:
    Q_INVOKABLE QString fileName(const QString &path,
                                 const QString &extension) const;

    // Generate temporary file:
    Q_INVOKABLE QString mktemp(const QString &pattern) const;

    // Generate a ascii-only string:
    Q_INVOKABLE QString asciify(const QString &input) const;

    // Heuristic to find out which QtQuick import version to use for the given file.
    Q_INVOKABLE QString qtQuickVersion(const QString &filePath) const;
};

} // Core::Internal
