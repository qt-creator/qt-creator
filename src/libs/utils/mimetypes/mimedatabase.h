/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "mimetype.h"
#include "mimemagicrule_p.h"

#include <utils/utils_global.h>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Utils {

// Wrapped QMimeDataBase functions
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForName(const QString &nameOrAlias);

enum class MimeMatchMode {
    MatchDefault = 0x0,
    MatchExtension = 0x1,
    MatchContent = 0x2
};

QTCREATOR_UTILS_EXPORT MimeType mimeTypeForFile(const QString &fileName, MimeMatchMode mode = MimeMatchMode::MatchDefault);
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForFile(const QFileInfo &fileInfo, MimeMatchMode mode = MimeMatchMode::MatchDefault);
QTCREATOR_UTILS_EXPORT QList<MimeType> mimeTypesForFileName(const QString &fileName);
QTCREATOR_UTILS_EXPORT MimeType mimeTypeForData(const QByteArray &data);
QTCREATOR_UTILS_EXPORT QList<MimeType> allMimeTypes();

// Qt Creator additions
// For debugging purposes.
enum class MimeStartupPhase {
    BeforeInitialize,
    PluginsLoading,
    PluginsInitializing, // Register up to here.
    PluginsDelayedInitializing, // Use from here on.
    UpAndRunning
};

QTCREATOR_UTILS_EXPORT void setMimeStartupPhase(MimeStartupPhase);
QTCREATOR_UTILS_EXPORT void addMimeTypes(const QString &id, const QByteArray &data);
QTCREATOR_UTILS_EXPORT QString allFiltersString(QString *allFilesFilter = nullptr);
QTCREATOR_UTILS_EXPORT QString allFilesFilterString();
QTCREATOR_UTILS_EXPORT QStringList allGlobPatterns();
QTCREATOR_UTILS_EXPORT QMap<int, QList<Internal::MimeMagicRule> > magicRulesForMimeType(const MimeType &mimeType); // priority -> rules
QTCREATOR_UTILS_EXPORT void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns);
QTCREATOR_UTILS_EXPORT void setMagicRulesForMimeType(const MimeType &mimeType,
                                     const QMap<int, QList<Internal::MimeMagicRule> > &rules); // priority -> rules

} // Utils
