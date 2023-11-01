// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "config.h"

#include <QFuture>
#include <QList>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

namespace CompilerExplorer::Api {
struct Library
{
    QString id;
    QString name;
    QUrl url;

    struct Version
    {
        QString version;
        QString id;
    };
    QList<Version> versions;
};

using Libraries = QList<Library>;

QFuture<Libraries> libraries(const Config &config, const QString &languageId);
} // namespace CompilerExplorer::Api
