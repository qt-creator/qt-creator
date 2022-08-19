// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

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
class QIODevice;
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
    QStringList mimeTypeForFileName(const QString &fileName, QString *foundSuffix = nullptr);

    mutable MimeProviderBase *m_provider;
    const QString m_defaultMimeType;
    QMutex mutex;

    int m_startupPhase = 0;
};

} // Internal
} // Utils
