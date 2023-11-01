// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "config.h"

#include <QFuture>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QSet>
#include <QString>

namespace CompilerExplorer::Api {

struct Compiler
{
    QString id;
    QString name;
    QString languageId;
    QString compilerType;
    QString version;
    QString instructionSet;
    QMap<QString, QString> extraFields;
};

using Compilers = QList<Compiler>;

QFuture<Compilers> compilers(const Config &config,
                             const QString &languageId = {},
                             const QSet<QString> &extraFields = {});

} // namespace CompilerExplorer::Api
