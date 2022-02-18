/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
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

#include <utils/utils_global.h>

#include <QtCore/qobjectdefs.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

namespace Utils {

class MimeTypePrivate;
class MimeType;

QTCREATOR_UTILS_EXPORT size_t qHash(const MimeType &key, size_t seed = 0) noexcept;

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
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(MimeType)
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

    QExplicitlySharedDataPointer<MimeTypePrivate> d;
};


} // namespace Utils

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug debug, const Utils::MimeType &mime);
#endif

Q_DECLARE_SHARED(Utils::MimeType)
