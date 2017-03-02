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

//
//  W A R N I N G
//  -------------
//
// This file is mostly copied from Qt code and should not be touched
// unless really needed.
//


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

#include <QtCore/qhash.h>
#include <QtCore/qmutex.h>
QT_BEGIN_NAMESPACE
class QFileInfo;
class QIODevice;
class QUrl;
QT_END_NAMESPACE

#include "mimetype.h"
#include "mimetype_p.h"
#include "mimeglobpattern_p.h"

namespace Utils {
namespace Internal {

class MimeProviderBase;

class MimeDatabasePrivate
{
public:
    Q_DISABLE_COPY(MimeDatabasePrivate)

    MimeDatabasePrivate();
    ~MimeDatabasePrivate();

    static MimeDatabasePrivate *instance();

    MimeProviderBase *provider();
    void setProvider(MimeProviderBase *theProvider);

    inline QString defaultMimeType() const { return m_defaultMimeType; }

    bool inherits(const QString &mime, const QString &parent);

    QList<MimeType> allMimeTypes();


    MimeType mimeTypeForName(const QString &nameOrAlias);
    MimeType mimeTypeForFileNameAndData(const QString &fileName, QIODevice *device, int *priorityPtr);
    MimeType findByData(const QByteArray &data, int *priorityPtr);
    QStringList mimeTypeForFileName(const QString &fileName, QString *foundSuffix = 0);

    mutable MimeProviderBase *m_provider;
    const QString m_defaultMimeType;
    QMutex mutex;

    int m_startupPhase = 0;
};

class MimeDatabase
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

    // For debugging purposes.
    enum StartupPhase {
        BeforeInitialize,
        PluginsLoading,
        PluginsInitializing, // Register up to here.
        PluginsDelayedInitializing, // Use from here on.
        UpAndRunning
    };
    static void setStartupPhase(StartupPhase);

private:
    Internal::MimeDatabasePrivate *d;
};

} // Internal
} // Utils
