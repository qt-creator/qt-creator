// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "../utils_global.h"

#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Utils {

class MimeTypePrivate;
class MimeType;

class QTCREATOR_UTILS_EXPORT MimeType
{
    Q_GADGET
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool isDefault READ isDefault CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QString genericIconName READ genericIconName CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QStringList globPatterns READ globPatterns CONSTANT)
    Q_PROPERTY(QStringList parentMimeTypes READ parentMimeTypes CONSTANT)
    Q_PROPERTY(QStringList allAncestors READ allAncestors CONSTANT)
    Q_PROPERTY(QStringList aliases READ aliases CONSTANT)
    Q_PROPERTY(QStringList suffixes READ suffixes CONSTANT)
    Q_PROPERTY(QString preferredSuffix READ preferredSuffix CONSTANT)
    Q_PROPERTY(QString filterString READ filterString CONSTANT)

public:
    MimeType();
    MimeType(const MimeType &other);
    MimeType &operator=(const MimeType &other);
    MimeType &operator=(MimeType &&other) noexcept
    {
        swap(other);
        return *this;
    }
    void swap(MimeType &other) noexcept
    {
        d.swap(other.d);
    }
    explicit MimeType(const MimeTypePrivate &dd);
    ~MimeType();

    bool operator==(const MimeType &other) const;

    inline bool operator!=(const MimeType &other) const
    {
        return !operator==(other);
    }

    bool isValid() const;

    bool isDefault() const;

    QString name() const;
    QString comment() const;
    QString genericIconName() const;
    QString iconName() const;
    QStringList globPatterns() const;
    QStringList parentMimeTypes() const;
    QStringList allAncestors() const;
    QStringList aliases() const;
    QStringList suffixes() const;
    QString preferredSuffix() const;

    Q_INVOKABLE bool inherits(const QString &mimeTypeName) const;

    QString filterString() const;

    // Qt Creator additions
    bool matchesName(const QString &nameOrAlias) const;
    void setPreferredSuffix(const QString &suffix);

protected:
    friend class MimeTypeParserBase;
    friend class MimeTypeMapEntry;
    friend class MimeDatabasePrivate;
    friend class MimeXMLProvider;
    friend class MimeBinaryProvider;
    friend class MimeTypePrivate;
    friend QTCREATOR_UTILS_EXPORT size_t qHash(const MimeType &key, size_t seed) noexcept;
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const MimeType &mime);

    QExplicitlySharedDataPointer<MimeTypePrivate> d;
};

} // namespace Utils

QT_BEGIN_NAMESPACE
Q_DECLARE_SHARED(Utils::MimeType)
QT_END_NAMESPACE
