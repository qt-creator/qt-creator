// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/pathlisteditor.h>

namespace Utils { class FilePath; }

namespace Debugger::Internal {

// Internal helper dialog prompting for a cache directory
// using a PathChooser.
// Note that QFileDialog does not offer a way of suggesting
// a non-existent folder, which is in turn automatically
// created. This is done here (suggest $TEMP\symbolcache
// regardless of its existence).

class CdbSymbolPathListEditor : public Utils::PathListEditor
{
public:
    enum SymbolPathMode{
        SymbolServerPath,
        SymbolCachePath
    };

    explicit CdbSymbolPathListEditor(QWidget *parent = nullptr);

    static bool promptCacheDirectory(QWidget *parent, Utils::FilePath *cacheDirectory);

    // Format a symbol path specification
    static QString symbolPath(const Utils::FilePath &cacheDir, SymbolPathMode mode);
    // Check for a symbol server path and extract local cache directory
    static bool isSymbolServerPath(const QString &path, QString *cacheDir = nullptr);
    // Check for a symbol cache path and extract local cache directory
    static bool isSymbolCachePath(const QString &path, QString *cacheDir = nullptr);
    // Check for symbol server in list of paths.
    static int indexOfSymbolPath(const QStringList &paths, SymbolPathMode mode, QString *cacheDir = nullptr);

private:
    void addSymbolPath(SymbolPathMode mode);
    void setupSymbolPaths();
};

} // Debugger::Internal
