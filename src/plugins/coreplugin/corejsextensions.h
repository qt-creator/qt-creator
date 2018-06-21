/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <utils/stringutils.h>

#include <QObject>
#include <QSet>

namespace Core {

namespace Internal {

class UtilsJsExtension : public QObject
{
    Q_OBJECT

public:
    UtilsJsExtension(QObject *parent = nullptr) : QObject(parent) { }

    // File name conversions:
    Q_INVOKABLE QString toNativeSeparators(const QString &in) const;
    Q_INVOKABLE QString fromNativeSeparators(const QString &in) const;

    Q_INVOKABLE QString baseName(const QString &in) const;
    Q_INVOKABLE QString fileName(const QString &in) const;
    Q_INVOKABLE QString completeBaseName(const QString &in) const;
    Q_INVOKABLE QString suffix(const QString &in) const;
    Q_INVOKABLE QString completeSuffix(const QString &in) const;
    Q_INVOKABLE QString path(const QString &in) const;
    Q_INVOKABLE QString absoluteFilePath(const QString &in) const;

    Q_INVOKABLE QString relativeFilePath(const QString &path, const QString &base) const;

    // File checks:
    Q_INVOKABLE bool exists(const QString &in) const;
    Q_INVOKABLE bool isDirectory(const QString &in) const;
    Q_INVOKABLE bool isFile(const QString &in) const;

    // MimeDB:
    Q_INVOKABLE QString preferredSuffix(const QString &mimetype) const;

    // Generate filename:
    Q_INVOKABLE QString fileName(const QString &path,
                                 const QString &extension) const;

    // Generate temporary file:
    Q_INVOKABLE QString mktemp(const QString &pattern) const;

    // Generate a ascii-only string:
    Q_INVOKABLE QString asciify(const QString &input) const;
};

} // namespace Internal
} // namespace Core
