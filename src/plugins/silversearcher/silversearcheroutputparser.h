// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/searchresultitem.h>

#include <QRegularExpression>

namespace SilverSearcher {

Utils::SearchResultItems parse(const QString &output,
                               const std::optional<QRegularExpression> &regExp = {});

} // namespace SilverSearcher
