/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MIMETYPE_H
#define MIMETYPE_H

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

#endif   // MIMETYPE_H
