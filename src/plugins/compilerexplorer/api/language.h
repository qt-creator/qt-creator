// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "config.h"

#include <QFuture>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>

namespace CompilerExplorer::Api {

struct Language
{
    QString id;
    QString name;
    QString logoUrl;
    QStringList extensions;
    QString monaco;
};

using Languages = QList<Language>;
QFuture<Languages> languages(const Config &config);

} // namespace CompilerExplorer::Api
