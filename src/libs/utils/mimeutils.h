// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

#pragma once

#include "utils_global.h"

#include <mimemagicrule_p.h>
#include <mimetype.h>

namespace Utils {

class FilePath;

// Wrapped QMimeDataBase functions
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForName(const QString &nameOrAlias);

enum class MimeMatchMode {
    MatchDefault = 0x0,
    MatchExtension = 0x1,
    MatchContent = 0x2,
    MatchDefaultAndRemote = 0x3
};

QTCREATOR_UTILS_EXPORT MimeType mimeTypeForFile(const QString &fileName,
                                                MimeMatchMode mode = MimeMatchMode::MatchDefault);
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForFile(const FilePath &filePath,
                                                MimeMatchMode mode = MimeMatchMode::MatchDefault);
QTCREATOR_UTILS_EXPORT QList<MimeType> mimeTypesForFileName(const QString &fileName);
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForData(const QByteArray &data);
QTCREATOR_UTILS_EXPORT QList<MimeType> allMimeTypes();

// Qt Creator additions
// For debugging purposes.
enum class MimeStartupPhase {
    BeforeInitialize,
    PluginsLoading,
    PluginsInitializing,        // Register up to here.
    PluginsDelayedInitializing, // Use from here on.
    UpAndRunning
};

QTCREATOR_UTILS_EXPORT void setMimeStartupPhase(MimeStartupPhase);
QTCREATOR_UTILS_EXPORT void addMimeTypes(const QString &id, const QByteArray &data);
QTCREATOR_UTILS_EXPORT QMap<int, QList<MimeMagicRule>> magicRulesForMimeType(
    const MimeType &mimeType); // priority -> rules
QTCREATOR_UTILS_EXPORT void setGlobPatternsForMimeType(const MimeType &mimeType,
                                                       const QStringList &patterns);
QTCREATOR_UTILS_EXPORT void setMagicRulesForMimeType(
    const MimeType &mimeType, const QMap<int, QList<MimeMagicRule>> &rules); // priority -> rules

} // namespace Utils
