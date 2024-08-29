// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtCore/qshareddata.h>

#include <QtCore/qhash.h>
#include <QtCore/qstringlist.h>

namespace Utils {

class MimeBinaryProvider;

class MimeTypePrivate : public QSharedData
{
public:
    typedef QHash<QString, QString> LocaleHash;

    MimeTypePrivate() { }
    explicit MimeTypePrivate(const QString &name) : name(name) { }

    QString name;
};

} // namespace Utils
