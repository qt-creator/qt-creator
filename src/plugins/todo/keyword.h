// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoicons.h"

#include <QColor>
#include <QString>
#include <QList>
#include <QMetaType>

namespace Todo {
namespace Internal {

class Keyword
{
public:
    Keyword();

    QString name;
    IconType iconType = IconType::Info;
    QColor color;
    bool equals(const Keyword &other) const;
};

using KeywordList = QList<Keyword>;

bool operator ==(const Keyword &k1, const Keyword &k2);
bool operator !=(const Keyword &k1, const Keyword &k2);

} // namespace Internal
} // namespace Todo
