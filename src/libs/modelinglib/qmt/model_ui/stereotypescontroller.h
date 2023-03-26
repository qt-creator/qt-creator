// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/qmt_global.h"

#include <QList>
#include <QString>

namespace qmt {

class QMT_EXPORT StereotypesController : public QObject
{
    Q_OBJECT

public:
    explicit StereotypesController(QObject *parent = nullptr);

signals:
    void parseError(const QString &stereotypes);

public:
    bool isParsable(const QString &stereotypes);
    QString toString(const QList<QString> &stereotypes);
    QList<QString> fromString(const QString &stereotypes);
};

} // namespace qmt
