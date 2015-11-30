/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSQRCPARSER_H
#define QMLJSQRCPARSER_H
#include "qmljs_global.h"

#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QLocale)

namespace QmlJS {

namespace Internal {
class QrcParserPrivate;
class QrcCachePrivate;
}

class QMLJS_EXPORT QrcParser
{
public:
    typedef QSharedPointer<QrcParser> Ptr;
    typedef QSharedPointer<const QrcParser> ConstPtr;
    ~QrcParser();
    bool parseFile(const QString &path);
    QString firstFileAtPath(const QString &path, const QLocale &locale) const;
    void collectFilesAtPath(const QString &path, QStringList *res, const QLocale *locale = 0) const;
    bool hasDirAtPath(const QString &path, const QLocale *locale = 0) const;
    void collectFilesInPath(const QString &path, QMap<QString,QStringList> *res, bool addDirs = false,
                            const QLocale *locale = 0) const;
    QStringList errorMessages() const;
    QStringList languages() const;
    bool isValid() const;

    static Ptr parseQrcFile(const QString &path);
    static QString normalizedQrcFilePath(const QString &path);
    static QString normalizedQrcDirectoryPath(const QString &path);
private:
    QrcParser();
    QrcParser(const QrcParser &);
    Internal::QrcParserPrivate *d;
};

class QMLJS_EXPORT QrcCache
{
public:
    QrcCache();
    ~QrcCache();
    QrcParser::ConstPtr addPath(const QString &path);
    void removePath(const QString &path);
    QrcParser::ConstPtr updatePath(const QString &path);
    QrcParser::ConstPtr parsedPath(const QString &path);
    void clear();
private:
    Internal::QrcCachePrivate *d;
};
}
#endif // QMLJSQRCPARSER_H
