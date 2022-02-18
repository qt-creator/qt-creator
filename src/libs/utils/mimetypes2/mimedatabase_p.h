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

#include "mimeglobpattern_p.h"
#include "mimetype_p.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>

#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Utils {

class MimeDatabase;
class MimeProviderBase;

class MimeDatabasePrivate
{
public:
    Q_DISABLE_COPY_MOVE(MimeDatabasePrivate)

    MimeDatabasePrivate();
    ~MimeDatabasePrivate();

    static MimeDatabasePrivate *instance();

    inline QString defaultMimeType() const { return m_defaultMimeType; }

    bool inherits(const QString &mime, const QString &parent);

    QList<MimeType> allMimeTypes();

    QString resolveAlias(const QString &nameOrAlias);
    QStringList parents(const QString &mimeName);
    MimeType mimeTypeForName(const QString &nameOrAlias);
    MimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device, int *priorityPtr);
    MimeType findByData(const QByteArray &data, int *priorityPtr);
    QStringList mimeTypeForFileName(const QString &fileName);
    MimeGlobMatchResult findByFileName(const QString &fileName);

    // API for MimeType. Takes care of locking the mutex.
    void loadMimeTypePrivate(MimeTypePrivate &mimePrivate);
    void loadGenericIcon(MimeTypePrivate &mimePrivate);
    void loadIcon(MimeTypePrivate &mimePrivate);
    QStringList mimeParents(const QString &mimeName);
    QStringList listAliases(const QString &mimeName);
    bool mimeInherits(const QString &mime, const QString &parent);

private:
    using Providers = std::vector<std::unique_ptr<MimeProviderBase>>;
    const Providers &providers();
    bool shouldCheck();
    void loadProviders();

    mutable Providers m_providers;
    QElapsedTimer m_lastCheck;

public:
    const QString m_defaultMimeType;
    QMutex mutex;
};

} // namespace Utils
