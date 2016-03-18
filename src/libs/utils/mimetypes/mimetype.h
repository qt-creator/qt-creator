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

#include <utils/utils_global.h>

#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE
class QFileinfo;
class QStringList;
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
