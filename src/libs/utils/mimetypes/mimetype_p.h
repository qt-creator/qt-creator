// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "mimetype.h"

#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>

namespace Utils {
namespace Internal {

class MimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    MimeTypePrivate();
    explicit MimeTypePrivate(const MimeType &other);

    void clear();

    void addGlobPattern(const QString &pattern);

    QString name;
    LocaleHash localeComments;
    QString genericIconName;
    QString iconName;
    QStringList globPatterns;
    bool loaded;
};

} // Internal
} // Utils

#define MIMETYPE_BUILDER \
    QT_BEGIN_NAMESPACE \
    static MimeType buildMimeType ( \
                         const QString &name, \
                         const QString &genericIconName, \
                         const QString &iconName, \
                         const QStringList &globPatterns \
                     ) \
    { \
        MimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = name; \
        qMimeTypeData.genericIconName = genericIconName; \
        qMimeTypeData.iconName = iconName; \
        qMimeTypeData.globPatterns = globPatterns; \
        return MimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE

#ifdef Q_COMPILER_RVALUE_REFS
#define MIMETYPE_BUILDER_FROM_RVALUE_REFS \
    QT_BEGIN_NAMESPACE \
    static MimeType buildMimeType ( \
                         QString &&name, \
                         QString &&genericIconName, \
                         QString &&iconName, \
                         QStringList &&globPatterns \
                     ) \
    { \
        MimeTypePrivate qMimeTypeData; \
        qMimeTypeData.name = std::move(name); \
        qMimeTypeData.genericIconName = std::move(genericIconName); \
        qMimeTypeData.iconName = std::move(iconName); \
        qMimeTypeData.globPatterns = std::move(globPatterns); \
        return MimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE
#endif
