// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "todoicons.h"

#include <QColor>
#include <QString>
#include <QList>

namespace Todo::Internal {

class Keyword
{
public:
    Keyword();

    QString name;
    IconType iconType = IconType::Info;
    QColor color;
    bool equals(const Keyword &other) const;

    friend bool operator ==(const Keyword &k1, const Keyword &k2);
    friend bool operator !=(const Keyword &k1, const Keyword &k2);
};

using KeywordList = QList<Keyword>;

} // namespace Todo::Internal
