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

#ifndef XMIMEDATABASE_H
#define XMIMEDATABASE_H

#include "mimetype.h"
#include "mimemagicrule_p.h"

#include <utils/utils_global.h>

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE
class QByteArray;
class QFileInfo;
class QIODevice;
class QUrl;
QT_END_NAMESPACE

namespace Utils {

namespace Internal { class MimeDatabasePrivate; }

class QTCREATOR_UTILS_EXPORT MimeDatabase
{
    Q_DISABLE_COPY(MimeDatabase)

public:
    MimeDatabase();
    ~MimeDatabase();

    MimeType mimeTypeForName(const QString &nameOrAlias) const;

    enum MatchMode {
        MatchDefault = 0x0,
        MatchExtension = 0x1,
        MatchContent = 0x2
    };

    MimeType mimeTypeForFile(const QString &fileName, MatchMode mode = MatchDefault) const;
    MimeType mimeTypeForFile(const QFileInfo &fileInfo, MatchMode mode = MatchDefault) const;
    QList<MimeType> mimeTypesForFileName(const QString &fileName) const;

    MimeType mimeTypeForData(const QByteArray &data) const;
    MimeType mimeTypeForData(QIODevice *device) const;

    MimeType mimeTypeForUrl(const QUrl &url) const;
    MimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device) const;
    MimeType mimeTypeForFileNameAndData(const QString &fileName, const QByteArray &data) const;

    QString suffixForFileName(const QString &fileName) const;

    QList<MimeType> allMimeTypes() const;

    // Qt Creator additions
    static void addMimeTypes(const QString &fileName);
    static QString allFiltersString(QString *allFilesFilter = 0);
    static QStringList allGlobPatterns();
    static QMap<int, QList<Internal::MimeMagicRule> > magicRulesForMimeType(const MimeType &mimeType); // priority -> rules
    static void setGlobPatternsForMimeType(const MimeType &mimeType, const QStringList &patterns);
    static void setMagicRulesForMimeType(const MimeType &mimeType,
                                         const QMap<int, QList<Internal::MimeMagicRule> > &rules); // priority -> rules

private:
    Internal::MimeDatabasePrivate *d;
};

} // Utils

#endif   // MIMEDATABASE_H
