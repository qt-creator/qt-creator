// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

#pragma once

#include <utils/utils_global.h>

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE
class QFileinfo;
QT_END_NAMESPACE

namespace Utils {

namespace Internal {
class MimeTypeParserBase;
class MimeTypeMapEntry;
class MimeDatabasePrivate;
class MimeXMLProvider;
class MimeBinaryProvider;
class MimeTypePrivate;
}

class QTCREATOR_UTILS_EXPORT MimeType
{
public:
    MimeType();
    MimeType(const MimeType &other);
    MimeType &operator=(const MimeType &other);
//#ifdef Q_COMPILER_RVALUE_REFS
//    MimeType &operator=(MimeType &&other)
//    {
//        qSwap(d, other.d);
//        return *this;
//    }
//#endif
//    void swap(MimeType &other)
//    {
//        qSwap(d, other.d);
//    }
    explicit MimeType(const Internal::MimeTypePrivate &dd);
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

    bool inherits(const QString &mimeTypeName) const;

    QString filterString() const;

    // Qt Creator additions
    bool matchesName(const QString &nameOrAlias) const;
    void setPreferredSuffix(const QString &suffix);

    friend auto qHash(const MimeType &mime) { return qHash(mime.name()); }

protected:
    friend class Internal::MimeTypeParserBase;
    friend class Internal::MimeTypeMapEntry;
    friend class Internal::MimeDatabasePrivate;
    friend class Internal::MimeXMLProvider;
    friend class Internal::MimeBinaryProvider;
    friend class Internal::MimeTypePrivate;

    QExplicitlySharedDataPointer<Internal::MimeTypePrivate> d;
};

} // Utils

//Q_DECLARE_SHARED(Utils::MimeType)

#ifndef QT_NO_DEBUG_STREAM
QT_BEGIN_NAMESPACE
class QDebug;
QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const Utils::MimeType &mime);
QT_END_NAMESPACE
#endif

