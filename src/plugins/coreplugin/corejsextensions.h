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

#ifndef COREJSEXTENSIONS_H
#define COREJSEXTENSIONS_H

#include <utils/stringutils.h>

#include <QObject>
#include <QSet>

namespace Core {

namespace Internal {

class UtilsJsExtension : public QObject
{
    Q_OBJECT

public:
    UtilsJsExtension(QObject *parent = 0) : QObject(parent) { }

    // File name conversions:
    Q_INVOKABLE QString toNativeSeparators(const QString &in) const;
    Q_INVOKABLE QString fromNativeSeparators(const QString &in) const;

    Q_INVOKABLE QString baseName(const QString &in) const;
    Q_INVOKABLE QString completeBaseName(const QString &in) const;
    Q_INVOKABLE QString suffix(const QString &in) const;
    Q_INVOKABLE QString completeSuffix(const QString &in) const;
    Q_INVOKABLE QString path(const QString &in) const;
    Q_INVOKABLE QString absoluteFilePath(const QString &in) const;

    // MimeDB:
    Q_INVOKABLE QString preferredSuffix(const QString &mimetype) const;

    // Generate filename:
    Q_INVOKABLE QString fileName(const QString &path,
                                 const QString &extension) const;

    // Generate temporary file:
    Q_INVOKABLE QString mktemp(const QString &pattern) const;
};

} // namespace Internal
} // namespace Core

#endif // COREJSEXTENSIONS_H
