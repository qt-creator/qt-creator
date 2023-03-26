// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

class MimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    MimeTypePrivate();
    explicit MimeTypePrivate(const MimeType &other);

    void clear();

    void addGlobPattern(const QString &pattern);

    bool loaded; // QSharedData leaves a 4 byte gap, so don't put 8 byte members first
    bool fromCache; // true if this comes from the binary provider
    QString name;
    LocaleHash localeComments;
    QString genericIconName;
    QString iconName;
    QStringList globPatterns;
};

} // namespace Utils

#if 0
#define QMIMETYPE_BUILDER_FROM_RVALUE_REFS \
    QT_BEGIN_NAMESPACE \
    static QMimeType buildQMimeType ( \
                         QString &&name, \
                         QString &&genericIconName, \
                         QString &&iconName, \
                         QStringList &&globPatterns \
                     ) \
    { \
        QMimeTypePrivate qMimeTypeData; \
        qMimeTypeData.loaded = true; \
        qMimeTypeData.name = std::move(name); \
        qMimeTypeData.genericIconName = std::move(genericIconName); \
        qMimeTypeData.iconName = std::move(iconName); \
        qMimeTypeData.globPatterns = std::move(globPatterns); \
        return QMimeType(qMimeTypeData); \
    } \
    QT_END_NAMESPACE
#endif
